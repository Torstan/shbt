/* Copyright 2019 Nikoli Dryden
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _GNU_SOURCE
#define _XOPEN_SOURCE 500
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNW_LOCAL_ONLY  // Only need the local API for libunwind.
#include <libunwind.h>

#include "shbt/shbt.h"
#include "shbt/shbt_internal.h"

bool shbt_collect_backtrace(shbt_frame_t trace[], size_t num_frames,
                            size_t* num_valid_frames) {
  unw_context_t context;
  unw_getcontext(&context);
  unw_cursor_t cursor;
  unw_init_local(&cursor, &context);
  size_t cur_frame = 0;
  const char* unknown_symbol_str = "(unknown symbol)";
  while (cur_frame < num_frames && unw_step(&cursor) > 0) {
    unw_word_t ip;
    if (unw_get_reg(&cursor, UNW_REG_IP, &ip) == 0) {
      trace[cur_frame].addr = (void*) ip;
    } else {
      trace[cur_frame].addr = NULL;
    }
    unw_word_t offp;
    if (unw_get_proc_name(&cursor, trace[cur_frame].symbol,
                          sizeof(trace[cur_frame].symbol), &offp)) {
      // Failed to get symbol name.
      strncpy(trace[cur_frame].symbol, unknown_symbol_str,
              sizeof(trace[cur_frame].symbol));
    }
    ++cur_frame;
  }
  *num_valid_frames = cur_frame;
  return true;
}

bool shbt_print_collected_backtrace_fd(shbt_frame_t trace[], size_t num_frames,
                                       int fd) {
  char str_buf[128] = {0};  // Should be sufficiently large.
  char demangled_symbol[1024] = {0};
  for (size_t cur_frame = 0; cur_frame < num_frames; ++cur_frame) {
    // Print frame number, with manual padding.
    if (cur_frame < 10) {
      shbt_safe_print("   ", fd);
    } else if (cur_frame < 100) {
      shbt_safe_print("  ", fd);
    } else if (cur_frame < 1000) {
      shbt_safe_print(" ", fd);
    }
    shbt_itoa(cur_frame, str_buf, sizeof(str_buf), 10, 0);
    shbt_safe_print(str_buf, fd);
    shbt_safe_print(": ", fd);
    if (shbt_demangle(trace[cur_frame].symbol, demangled_symbol,
                      sizeof(demangled_symbol))) {
      shbt_safe_print(demangled_symbol, fd);
#ifdef SHBT_USE_BUILTIN_IA64_DEMANGLER
      // Print the mangled symbol too, since this demangler doesn't fully
      // demangle some C++ stuff (function/template arguments, etc.).
      shbt_safe_print(" (", fd);
      shbt_safe_print(trace[cur_frame].symbol, fd);
      shbt_safe_print(")", fd);
#endif
    } else {
      shbt_safe_print(trace[cur_frame].symbol, fd);
    }
    shbt_safe_print("\n", fd);
  }
  return true;
}

bool shbt_print_collected_backtrace_detailed_fd(shbt_frame_t trace[],
                                                size_t num_frames, int fd) {
  char str_buf[128] = {0};
  char demangled_symbol[1024] = {0};
  for (size_t cur_frame = 0; cur_frame < num_frames; ++cur_frame) {
    // Resolve file:line via addr2line.
    char file_line[256] = {0};
    if (trace[cur_frame].addr != NULL) {
      Dl_info dl_info;
      if (dladdr(trace[cur_frame].addr, &dl_info) &&
          dl_info.dli_fname != NULL) {
        uintptr_t offset =
          (uintptr_t) trace[cur_frame].addr - (uintptr_t) dl_info.dli_fbase;
        char cmd[1280];
        snprintf(cmd, sizeof(cmd), "addr2line -e %s -fp 0x%lx 2>/dev/null",
                 dl_info.dli_fname, (unsigned long) offset);
        FILE* fp = popen(cmd, "r");
        if (fp != NULL) {
          if (fgets(file_line, sizeof(file_line), fp) != NULL) {
            size_t len = strlen(file_line);
            if (len > 0 && file_line[len - 1] == '\n') {
              file_line[len - 1] = '\0';
            }
            if (strstr(file_line, "??") != NULL) {
              file_line[0] = '\0';
            }
          }
          pclose(fp);
        }
      }
    }
    // Print frame.
    if (cur_frame < 10) {
      shbt_safe_print("   ", fd);
    } else if (cur_frame < 100) {
      shbt_safe_print("  ", fd);
    } else if (cur_frame < 1000) {
      shbt_safe_print(" ", fd);
    }
    shbt_itoa(cur_frame, str_buf, sizeof(str_buf), 10, 0);
    shbt_safe_print(str_buf, fd);
    shbt_safe_print(": ", fd);
    shbt_safe_print("0x", fd);
    shbt_itoa((intptr_t) trace[cur_frame].addr, str_buf, sizeof(str_buf), 16,
              12);
    shbt_safe_print(str_buf, fd);
    shbt_safe_print(" ", fd);
    if (shbt_demangle(trace[cur_frame].symbol, demangled_symbol,
                      sizeof(demangled_symbol))) {
      shbt_safe_print(demangled_symbol, fd);
    } else {
      shbt_safe_print(trace[cur_frame].symbol, fd);
    }
    if (file_line[0] != '\0') {
      shbt_safe_print(" at ", fd);
      const char* at_pos = strstr(file_line, " at ");
      if (at_pos != NULL) {
        shbt_safe_print(at_pos + 4, fd);
      } else {
        shbt_safe_print(file_line, fd);
      }
    }
    shbt_safe_print("\n", fd);
  }
  return true;
}

bool shbt_print_backtrace_detailed_fd(int fd) {
  static shbt_frame_t trace[128];
  size_t valid_depth = 0;
  if (!shbt_collect_backtrace(trace, 128, &valid_depth)) {
    return false;
  }
  if (!shbt_print_collected_backtrace_detailed_fd(trace, valid_depth, fd)) {
    return false;
  }
  return true;
}

bool shbt_print_backtrace_fd(int fd) {
  static shbt_frame_t trace[128];
  size_t valid_depth = 0;
  if (!shbt_collect_backtrace(trace, 128, &valid_depth)) {
    return false;
  }
  if (!shbt_print_collected_backtrace_fd(trace, valid_depth, fd)) {
    return false;
  }
  return true;
}

size_t shbt_get_stack_depth() {
  unw_context_t context;
  unw_getcontext(&context);
  unw_cursor_t cursor;
  unw_init_local(&cursor, &context);
  size_t cur_frame = 0;
  for (; unw_step(&cursor) > 0; ++cur_frame) {}
  return cur_frame;
}
