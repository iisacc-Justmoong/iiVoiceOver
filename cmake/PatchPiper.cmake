# Local changes to the pinned native dependency. A new revision must revalidate
# this patch. Runtime code is compiled from C/C++; no Python is executed.
set(source "${SOURCE_DIR}/libpiper/src/piper.cpp")
file(READ "${source}" text)
if(NOT text MATCHES "iivo_native_owner")
    string(REPLACE "auto *synth = new piper_synthesizer();"
        "// iiVoiceOver: release partially constructed native sessions on exceptions.\n  std::unique_ptr<piper_synthesizer, decltype(&piper_free)> iivo_native_owner(new piper_synthesizer(), &piper_free);\n  auto *synth = iivo_native_owner.get();" text "${text}")
    string(REPLACE "piper_free(synth);" "iivo_native_owner.reset();" text "${text}")
    string(REPLACE "return synth;" "return iivo_native_owner.release();" text "${text}")
    string(REPLACE "static Ort::Env ort_env{ORT_LOGGING_LEVEL_WARNING, \"piper\"};"
        "static Ort::Env ort_env = [] { Ort::Env env{ORT_LOGGING_LEVEL_WARNING, \"piper\"}; env.DisableTelemetryEvents(); return env; }();"
        text "${text}")
    string(REPLACE "MultiByteToWideChar(CP_ACP" "MultiByteToWideChar(CP_UTF8" text "${text}")
    if(NOT text MATCHES "iivo_native_owner.release")
        message(FATAL_ERROR "The native Piper construction patch no longer applies")
    endif()
    file(WRITE "${source}" "${text}")
endif()
set(cmake_source "${SOURCE_DIR}/libpiper/CMakeLists.txt")
file(READ "${cmake_source}" cmake_text)
string(REPLACE "set(CMAKE_CXX_STANDARD 17)" "set(CMAKE_CXX_STANDARD 23)" cmake_text "${cmake_text}")
# PATHS alone is searched after global installation prefixes. Never silently
# link another SDK's ONNX binary while using our pinned 1.22 headers.
string(REPLACE "find_library(ONNXRUNTIME_LIB NAMES onnxruntime PATHS \${ONNXRUNTIME_DIR}/lib REQUIRED)"
    "unset(ONNXRUNTIME_LIB CACHE)\n    find_library(ONNXRUNTIME_LIB NAMES onnxruntime PATHS \${ONNXRUNTIME_DIR}/lib NO_DEFAULT_PATH REQUIRED)"
    cmake_text "${cmake_text}")
file(WRITE "${cmake_source}" "${cmake_text}")
