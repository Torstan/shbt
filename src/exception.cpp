#include "shbt.h"
namespace exception_bt {}

namespace std {
class exception {
 private:
  thread_local shbt_frame_t trace[128];
  thread_local size_t num_trace;
  exception() { shbt_collect_backtrace(trace, 128, &num_trace); }
}
}  // namespace std