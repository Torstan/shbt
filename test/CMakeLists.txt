set(TEST_SOURCES
  no_error.c
  segfault.c
  ill.c
  intdiv.c
  intdiv2.c
  intdiv3.c
  bus.c
  wait.c
  demangle.c
  test_exception.cpp
  )

foreach(src ${TEST_SOURCES})
  get_filename_component(_test_bin_name "${src}" NAME_WE)
  add_executable(${_test_bin_name} ${src})
  target_link_libraries(${_test_bin_name} PRIVATE shbt)
endforeach()

add_test(NAME no_error COMMAND no_error)

add_test(NAME demangle COMMAND demangle _Z4testv)
set_tests_properties(demangle PROPERTIES
  PASS_REGULAR_EXPRESSION "Demangling result: test\\(\\)")

add_test(NAME test_exception COMMAND test_exception)
set_tests_properties(test_exception PROPERTIES
  PASS_REGULAR_EXPRESSION "exception Info:MyException")

set(FATAL_SIGNAL_TESTS
  segfault
  intdiv
  intdiv2
  intdiv3
  bus
  )

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
  list(APPEND FATAL_SIGNAL_TESTS ill)
endif ()

foreach(test_name ${FATAL_SIGNAL_TESTS})
  add_test(NAME ${test_name}
    COMMAND "${CMAKE_COMMAND}"
      "-DTEST_EXECUTABLE=$<TARGET_FILE:${test_name}>"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/expect_failure.cmake")
endforeach()
