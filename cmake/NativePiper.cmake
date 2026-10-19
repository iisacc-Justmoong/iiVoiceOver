include(FetchContent)
include(ExternalProject)
function(iivo_download url filename sha)
    set(destination "${CMAKE_CURRENT_BINARY_DIR}/downloads/${filename}")
    if(EXISTS "${destination}")
        file(SHA256 "${destination}" existing_hash)
    endif()
    if(NOT existing_hash STREQUAL sha)
        file(DOWNLOAD "${url}" "${destination}" EXPECTED_HASH "SHA256=${sha}"
            TLS_VERIFY ON STATUS status SHOW_PROGRESS)
        list(GET status 0 status_code)
        if(NOT status_code EQUAL 0)
            message(FATAL_ERROR "Download failed: ${url}: ${status}")
        endif()
    endif()
    set(iivo_downloaded "${destination}" PARENT_SCOPE)
endfunction()
set(iivo_piper_revision 619f513580c04dff5ef9a680f6f3f6979ed94d2e)
FetchContent_Declare(iivo_piper
    URL "https://codeload.github.com/OHF-Voice/piper1-gpl/tar.gz/${iivo_piper_revision}"
    URL_HASH SHA256=8e0c990332895f23fdba02c7d332e637890f7175b4a4d51db3cf207136d4b304
    SOURCE_SUBDIR unused-no-add-subdirectory)
FetchContent_MakeAvailable(iivo_piper)
set(iivo_piper_source "${iivo_piper_SOURCE_DIR}")
set(IIVOICEOVER_ORT_ROOT "" CACHE PATH "Existing native ONNX Runtime include/ and lib/ distribution")
if(NOT IIVOICEOVER_ORT_ROOT)
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" iivo_arch)
    if(APPLE AND iivo_arch MATCHES "^(arm64|aarch64)$")
        set(iivo_ort_archive onnxruntime-osx-arm64-1.22.0.tgz)
        set(iivo_ort_hash cab6dcbd77e7ec775390e7b73a8939d45fec3379b017c7cb74f5b204c1a1cc07)
    elseif(APPLE AND iivo_arch MATCHES "^(x86_64|amd64)$")
        set(iivo_ort_archive onnxruntime-osx-x86_64-1.22.0.tgz)
        set(iivo_ort_hash e4ec94a7696de74fb1b12846569aa94e499958af6ffa186022cfde16c9d617f0)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND iivo_arch MATCHES "^(x86_64|amd64)$")
        set(iivo_ort_archive onnxruntime-linux-x64-1.22.0.tgz)
        set(iivo_ort_hash 8344d55f93d5bc5021ce342db50f62079daf39aaafb5d311a451846228be49b3)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND iivo_arch MATCHES "^(arm64|aarch64)$")
        set(iivo_ort_archive onnxruntime-linux-aarch64-1.22.0.tgz)
        set(iivo_ort_hash bb76395092d150b52c7092dc6b8f2fe4d80f0f3bf0416d2f269193e347e24702)
    elseif(WIN32 AND iivo_arch MATCHES "^(x86_64|amd64)$")
        set(iivo_ort_archive onnxruntime-win-x64-1.22.0.zip)
        set(iivo_ort_hash 174c616efc0271194488642a72f1a514e01487da4dfe84c49296d66e40ebe0da)
    else()
        message(FATAL_ERROR "Supply IIVOICEOVER_ORT_ROOT for this native platform; no emulated fallback is provided")
    endif()
    FetchContent_Declare(iivo_ort
        URL "https://github.com/microsoft/onnxruntime/releases/download/v1.22.0/${iivo_ort_archive}"
        URL_HASH "SHA256=${iivo_ort_hash}" SOURCE_SUBDIR unused-no-add-subdirectory)
    FetchContent_MakeAvailable(iivo_ort)
    set(iivo_ort_root "${iivo_ort_SOURCE_DIR}")
else()
    set(iivo_ort_root "${IIVOICEOVER_ORT_ROOT}")
endif()
if(APPLE)
    set(iivo_piper_library "${CMAKE_CURRENT_BINARY_DIR}/lib/libpiper.dylib")
    set(iivo_piper_rpath "@loader_path")
elseif(WIN32)
    set(iivo_piper_library "${CMAKE_CURRENT_BINARY_DIR}/bin/piper.dll")
else()
    set(iivo_piper_library "${CMAKE_CURRENT_BINARY_DIR}/lib/libpiper.so")
    set(iivo_piper_rpath "$ORIGIN")
endif()
set(iivo_piper_byproducts "${iivo_piper_library}")
if(WIN32)
    # Ninja also needs a producer for the import library used by consumers.
    list(APPEND iivo_piper_byproducts "${CMAKE_CURRENT_BINARY_DIR}/lib/piper.lib")
endif()
set(iivo_external_build "${CMAKE_CURRENT_BINARY_DIR}/native/build")
ExternalProject_Add(iiVoiceOver_piper
    SOURCE_DIR "${iivo_piper_source}/libpiper"
    BINARY_DIR "${iivo_external_build}"
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/native/prefix"
    DOWNLOAD_COMMAND "" UPDATE_COMMAND ""
    PATCH_COMMAND "${CMAKE_COMMAND}" "-DSOURCE_DIR=${iivo_piper_source}" -P "${CMAKE_CURRENT_LIST_DIR}/PatchPiper.cmake"
    CMAKE_ARGS
        "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}"
        "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
        "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
        "-DCMAKE_INSTALL_RPATH=${iivo_piper_rpath}"
        "-DCMAKE_INSTALL_RPATH_USE_LINK_PATH=OFF"
        "-DONNXRUNTIME_DIR=${iivo_ort_root}"
        "-DCMAKE_DISABLE_FIND_PACKAGE_onnxruntime=TRUE"
        "-DPIPER_BUILD_TESTS=OFF" "-DENABLE_CLANG_TIDY=OFF"
    BUILD_BYPRODUCTS ${iivo_piper_byproducts}
    LOG_DOWNLOAD ON LOG_CONFIGURE ON LOG_BUILD ON LOG_INSTALL ON LOG_OUTPUT_ON_FAILURE ON)
ExternalProject_Add_StepDependencies(iiVoiceOver_piper patch "${CMAKE_CURRENT_LIST_DIR}/PatchPiper.cmake")
add_library(iiVoiceOver_piper_native SHARED IMPORTED)
set_target_properties(iiVoiceOver_piper_native PROPERTIES IMPORTED_LOCATION "${iivo_piper_library}")
if(WIN32)
    set_property(TARGET iiVoiceOver_piper_native PROPERTY IMPORTED_IMPLIB "${CMAKE_CURRENT_BINARY_DIR}/lib/piper.lib")
endif()
add_dependencies(iiVoiceOver_piper_native iiVoiceOver_piper)
if(IIVOICEOVER_DOWNLOAD_VOICE)
    set(iivo_voice_revision c10ece1aade47bb51c153c893d14e5bf8e5b7117)
    set(iivo_voice_base "https://huggingface.co/rhasspy/piper-voices/resolve/${iivo_voice_revision}/en/en_US/ljspeech/medium")
    set(iivo_voice_directory "${CMAKE_CURRENT_BINARY_DIR}/share/iiVoiceOver/voices/en_US-ljspeech-medium")
    file(MAKE_DIRECTORY "${iivo_voice_directory}")
    iivo_download("${iivo_voice_base}/en_US-ljspeech-medium.onnx" en_US-ljspeech-medium.onnx
        6f52a751e2349abe7a76735eb09dc1875298c77ea2342ffd2fef79ff81b87f22)
    configure_file("${iivo_downloaded}" "${iivo_voice_directory}/en_US-ljspeech-medium.onnx" COPYONLY)
    iivo_download("${iivo_voice_base}/en_US-ljspeech-medium.onnx.json" en_US-ljspeech-medium.onnx.json
        141d612cc0a95ed7efc1ca936b845c2364967f2e9217c5dbfcf69fc4d6c65860)
    configure_file("${iivo_downloaded}" "${iivo_voice_directory}/en_US-ljspeech-medium.onnx.json" COPYONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/models/en_US-ljspeech-medium/MODEL_CARD"
        "${iivo_voice_directory}/MODEL_CARD" COPYONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/models/en_US-ljspeech-medium/provenance.json"
        "${iivo_voice_directory}/provenance.json" COPYONLY)
    install(DIRECTORY "${iivo_voice_directory}" DESTINATION ${CMAKE_INSTALL_DATADIR}/iiVoiceOver/voices)
endif()
if(WIN32)
    install(FILES "${iivo_piper_library}" DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin/" DESTINATION ${CMAKE_INSTALL_BINDIR}
        FILES_MATCHING PATTERN "onnxruntime*.dll")
else()
    install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib/" DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FILES_MATCHING PATTERN "libpiper.*" PATTERN "libonnxruntime*")
endif()
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/share/espeak-ng-data" DESTINATION ${CMAKE_INSTALL_DATADIR})
set(iivo_license_directory "${CMAKE_INSTALL_DATADIR}/licenses/iiVoiceOver")
install(FILES "${iivo_piper_source}/COPYING" DESTINATION "${iivo_license_directory}/piper")
install(FILES "${iivo_piper_source}/libpiper/include/json.hpp" "${iivo_piper_source}/libpiper/include/uni_algo.h"
    DESTINATION "${iivo_license_directory}/piper-vendored-headers")
install(FILES licenses/nlohmann-json-MIT.txt DESTINATION "${iivo_license_directory}/piper-vendored-headers")
install(FILES "${iivo_ort_root}/LICENSE" "${iivo_ort_root}/ThirdPartyNotices.txt" "${iivo_ort_root}/Privacy.md"
    DESTINATION "${iivo_license_directory}/onnxruntime")
install(FILES "${iivo_external_build}/espeak_ng/src/espeak_ng_external/COPYING"
    DESTINATION "${iivo_license_directory}/espeak-ng")
install(FILES "${iivo_external_build}/espeak_ng/src/espeak_ng_external/src/ucd-tools/COPYING"
    "${iivo_external_build}/espeak_ng/src/espeak_ng_external/src/ucd-tools/COPYING.UCD"
    DESTINATION "${iivo_license_directory}/ucd-tools")
install(DIRECTORY "${iivo_external_build}/open_jtalk/src/openjtalk_external/"
    DESTINATION "${iivo_license_directory}/open-jtalk" FILES_MATCHING
    PATTERN "COPYING*" PATTERN "LICENSE*" PATTERN ".git" EXCLUDE)
install(FILES models/en_US-ljspeech-medium/MODEL_CARD models/en_US-ljspeech-medium/provenance.json
    DESTINATION "${iivo_license_directory}/en_US-ljspeech-medium")
