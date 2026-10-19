execute_process(COMMAND "${NINJA_EXECUTABLE}" -C "${BUILD_DIRECTORY}" -t query lib/piper.lib
    RESULT_VARIABLE result OUTPUT_VARIABLE graph ERROR_VARIABLE errors)
if(NOT result EQUAL 0 OR NOT graph MATCHES "input: CUSTOM_COMMAND")
    message(FATAL_ERROR "Piper import library has no native build producer: ${graph}${errors}")
endif()
