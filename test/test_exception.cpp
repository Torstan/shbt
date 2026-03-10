#include <unistd.h>

#include <exception>
#include <iostream>

#include "shbt/shbt.h"

struct MyException : public std::exception {
  MyException() { std::cout << "Construct MyException" << std::endl; }

  virtual const char* what() const noexcept { return "MyException"; }
};

void test() { throw MyException(); }

int main() {
  try {
    test();
  } catch (std::exception& e) {
    std::cout << "exception Info:" << e.what() << std::endl;
    shbt_print_backtrace_detailed_fd(STDERR_FILENO);
  }
}
