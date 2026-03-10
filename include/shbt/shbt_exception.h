#include "shbt/shbt.h"

namespace shbt {
class Exception : public std::exception {
public:
  Exception() {
    shbt_save_backtrace();
  }
};

}
