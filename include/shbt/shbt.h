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

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "shbt_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*print_func_t)(const char*);

/**
 * Save backtrace in thread local storage
 */
void shbt_save_backtrace();

/**
 * Print a collected backtrace using a print function.
 *
 * This function is safe to call from a signal handler and is thread-safe.
 *
 * @param print_func Function to use for printing.
 */
bool shbt_print_saved_backtrace(print_func_t print_func);

/**
 * Print backtrace of current calling point
 *
 * @param print_func Function to use for printing.
 *
 */
void shbt_print_backtrace(print_func_t print_func);

/** Exit action for signal handlers. */
typedef enum shbt_exit_action {
  /** Exit the program after the signal handler completes. */
  SHBT_EXIT_ACTION_EXIT = 0,
  /** Return from the signal handler. */
  SHBT_EXIT_ACTION_RETURN,
  /** Reraise the signal after the handler completes (for e.g. core dumps). */
  SHBT_EXIT_ACTION_RERAISE
} shbt_exit_action_t;
/**
 * Register a signal handler for a signal.
 *
 * This signal handler will automatically print signal information and a
 * backtrace to stderr. It can also invoke an optional callback after this
 *
 * It can then take one of three actions:
 *   1. Exit the program (default).
 *   2. Return from the signal handler and allow the program to continue.
 *   3. Re-raise the signal with the default signal handler. This is useful
 *      if you want to produce a core dump or something similar.
 *
 * The exit action can be overridden for specific signals with the
 *   shbt_register_signal_exit_action
 * function, or for all signals by the SHBT_SIGNAL_EXIT_ACTION environment
 * variable. Note this environment variable is checked when this function
 * is called, not during the signal handler invokation. Using
 * shbt_register_signal_exit_action takes precedence over environment
 * variables.
 *
 * @param sig_num The signal number.
 * @param exit_action One of SHBT_EXIT_ACTION_*.
 * @param callback Function pointer to the callback to invoke (may be NULL).
 */
bool shbt_register_signal_handler(int sig_num, shbt_exit_action_t exit_action,
                                  void (*callback)(int));
/**
 * Register multiple signal handlers.
 *
 * This operates identically to shbt_register_signal_handler.
 *
 * All handlers will use the same exit action and callback.
 *
 * @param sig_nums Array of signal numbers.
 * @param num_sigs Number of elements in sig_nums.
 * @param exit_action One of SHBT_EXIT_ACTION_*.
 * @param callback Function pointer to the callback to invoke (may be NULL).
 */
bool shbt_register_signal_handlers(const int sig_nums[], size_t num_sigs,
                                   shbt_exit_action_t exit_action,
                                   void (*callback)(int));
/**
 * Register signal handlers for fatal signals.
 *
 * This operates similarly to shbt_register_signal_handler.
 *
 * This currently registers handlers for the following signals (when present):
 *
 *   SIGABRT, SIGALRM, SIGBUS, SIGEMT, SIGFPE, SIGHUP, SIGILL, SIGINT, SIGIO,
 *   SIGLOST, SIGPIPE, SIGPROF, SIGPWR, SIGQUIT, SIGSEGV, SIGSTKFLT, SIGSYS,
 *   SIGTERM, SIGTRAP, SIGUSR1, SIGUSR2, SIGVTALRM, SIGXCPU, and SIGXFSZ.
 *
 * These are the signals that either terminate or dump core when received.
 */
bool shbt_register_fatal_handlers();

/**
 * Set the exit action for a signal.
 *
 * Note that you must be careful with this, since a signal handler may
 * already be established.
 *
 * @param sig_num The signal number.
 * @param exit_action One of SHBT_EXIT_ACTION_*.
 */
bool shbt_register_signal_exit_action(int sig_num,
                                      shbt_exit_action_t exit_action);

#ifdef __cplusplus
}  // extern "C"
#endif

