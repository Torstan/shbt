if (NOT DEFINED TEST_EXECUTABLE)
  message(FATAL_ERROR "TEST_EXECUTABLE is required")
endif ()

execute_process(
  COMMAND "${TEST_EXECUTABLE}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE error)

set(combined_output "${output}${error}")

if (result STREQUAL "0")
  message(FATAL_ERROR "Expected ${TEST_EXECUTABLE} to fail")
endif ()

if (NOT combined_output MATCHES "Received signal")
  message(FATAL_ERROR "Expected signal information in output:\n${combined_output}")
endif ()

if (NOT combined_output MATCHES "Backtrace:")
  message(FATAL_ERROR "Expected backtrace in output:\n${combined_output}")
endif ()
