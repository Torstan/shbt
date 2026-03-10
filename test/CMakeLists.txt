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
