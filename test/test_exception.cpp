#include <exception>
#include <iostream>

#include "shbt/shbt_exception.h"

struct MyException : public shbt::Exception {
  MyException() { }

  const char* what() const noexcept override { return "MyException"; }
};

void test() { throw MyException(); }

int main() {
  try {
    test();
  } catch (std::exception& e) {
    std::cout << "exception Info:" << e.what() << std::endl;
    shbt_print_saved_backtrace([](const char* str){
        std::cout << str;
    });
  }
}
