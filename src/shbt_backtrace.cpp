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

constexpr size_t kNumTrace = 128;
static thread_local shbt_frame_t tls_trace[kNumTrace];
static thread_local size_t tls_num_valid_frames = 0;

bool shbt_collect_backtrace_frames(shbt_frame_t trace[], size_t num_frames,
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
      trace[cur_frame].addr = ip;
    } else {
      trace[cur_frame].addr = 0;
    }
    unw_word_t offp = 0;
    if (unw_get_proc_name(&cursor, trace[cur_frame].symbol,
                          sizeof(trace[cur_frame].symbol), &offp)) {
      // Failed to get symbol name.
      strncpy(trace[cur_frame].symbol, unknown_symbol_str,
              sizeof(trace[cur_frame].symbol));
    }
    trace[cur_frame].offset = offp;
    ++cur_frame;
  }
  *num_valid_frames = cur_frame;
  return true;
}

bool shbt_print_saved_backtrace(print_func_t print_func) {
  shbt_frame_t *trace = tls_trace;
  size_t num_frames = tls_num_valid_frames;
  char str_buf[128];  // Should be sufficiently large.
  char demangled_symbol[512];
  char bt_buf[1024];

  for (size_t cur_frame = 0; cur_frame < num_frames; ++cur_frame) {
    // Print frame number, with manual padding.
    if (cur_frame < 10) {
      print_func("   ");
    } else if (cur_frame < 100) {
      print_func("  ");
    } else if (cur_frame < 1000) {
      print_func(" ");
    }
    shbt_itoa(cur_frame, str_buf, sizeof(str_buf), 10, 0);
    print_func(str_buf);
    print_func(": ");
    if (shbt_demangle(trace[cur_frame].symbol, demangled_symbol,
                      sizeof(demangled_symbol))) {
      snprintf(bt_buf, sizeof(bt_buf), "0x%016lx <%s+0x%lx>\n", trace[cur_frame].addr, demangled_symbol, trace[cur_frame].offset);
    } else {
      snprintf(bt_buf, sizeof(bt_buf), "0x%016lx <%s+0x%lx>\n", trace[cur_frame].addr, trace[cur_frame].symbol, trace[cur_frame].offset);
    }
    print_func(bt_buf);
  }
  return true;
}

void shbt_save_backtrace() {
  shbt_collect_backtrace_frames(tls_trace, kNumTrace, &tls_num_valid_frames);
}

void shbt_print_backtrace(print_func_t print_func) {
  shbt_collect_backtrace_frames(tls_trace, kNumTrace, &tls_num_valid_frames);
  shbt_print_saved_backtrace(print_func);
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
