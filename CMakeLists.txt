cmake_minimum_required(VERSION 3.31)
project(iiVoiceOver VERSION 0.2.0 LANGUAGES CXX)
if(PROJECT_IS_TOP_LEVEL AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(DEFINED ENV{HOME})
        set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local/SDK/iiVoiceOver" CACHE PATH "Installation prefix" FORCE)
    elseif(DEFINED ENV{USERPROFILE})
        set(CMAKE_INSTALL_PREFIX "$ENV{USERPROFILE}/.local/SDK/iiVoiceOver" CACHE PATH "Installation prefix" FORCE)
    endif()
endif()
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)
find_package(iiSDKCore 0.18 CONFIG REQUIRED HINTS "$ENV{HOME}/.local/SDK/iiSDKCore")
find_package(iiFileProviderNative 0.8 CONFIG REQUIRED HINTS "$ENV{HOME}/.local/SDK/iiFileProvider")
find_package(Threads REQUIRED)
option(IIVOICEOVER_DOWNLOAD_VOICE "Download and install the pinned English demo voice" ON)
option(IIVOICEOVER_BUILD_EXAMPLES "Build the native Hello World speech example" ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
include(cmake/NativePiper.cmake)
add_library(iiVoiceOver SHARED src/iiVoiceOver.cpp src/iiVoiceOver.h)
add_library(iiVoiceOver::iiVoiceOver ALIAS iiVoiceOver)
target_compile_features(iiVoiceOver PUBLIC cxx_std_23)
target_compile_definitions(iiVoiceOver PRIVATE IIVOICEOVER_BUILDING_LIBRARY)
target_link_libraries(iiVoiceOver PUBLIC iiSDKCore::Contracts
    PRIVATE iiVoiceOver_piper_native iiFileProvider::Native Threads::Threads ${CMAKE_DL_LIBS})
target_include_directories(iiVoiceOver PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    PRIVATE "${iivo_piper_source}/libpiper/include")
add_dependencies(iiVoiceOver iiVoiceOver_piper)
if(APPLE)
    set(iivo_origin "@loader_path")
elseif(UNIX)
    set(iivo_origin "$ORIGIN")
endif()
set_target_properties(iiVoiceOver PROPERTIES
    VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    CXX_EXTENSIONS OFF CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES
    INSTALL_RPATH "${iivo_origin};${iivo_origin}/../../iiFileProvider/lib"
    INSTALL_RPATH_USE_LINK_PATH TRUE)
if(IIVOICEOVER_BUILD_EXAMPLES)
    add_executable(iiVoiceOver_hello_world examples/hello_world.cpp)
    target_link_libraries(iiVoiceOver_hello_world PRIVATE iiVoiceOver::iiVoiceOver iiSDKCore::iiSDKCore)
    set_target_properties(iiVoiceOver_hello_world PROPERTIES OUTPUT_NAME ii-voice-over
        CXX_EXTENSIONS OFF INSTALL_RPATH "${iivo_origin}/../${CMAKE_INSTALL_LIBDIR}"
        INSTALL_RPATH_USE_LINK_PATH TRUE)
    install(TARGETS iiVoiceOver_hello_world RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(BUILD_TESTING)
    add_executable(iiVoiceOver_tests tests/tts_tests.cpp)
    target_link_libraries(iiVoiceOver_tests PRIVATE iiVoiceOver::iiVoiceOver iiFileProvider::Native Threads::Threads)
    set_target_properties(iiVoiceOver_tests PROPERTIES CXX_EXTENSIONS OFF)
    if(IIVOICEOVER_DOWNLOAD_VOICE)
        foreach(test IN ITEMS contracts inference concurrent corrupt)
            add_test(NAME iiVoiceOver.${test} COMMAND iiVoiceOver_tests ${test})
            set_tests_properties(iiVoiceOver.${test} PROPERTIES TIMEOUT 120)
        endforeach()
    endif()
endif()
install(TARGETS iiVoiceOver EXPORT iiVoiceOverTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES src/iiVoiceOver.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES README.md THIRD_PARTY_NOTICES.md DESTINATION ${CMAKE_INSTALL_DATADIR}/iiVoiceOver)
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/iiVoiceOver)
set(iiVoiceOver_PACKAGE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/iiVoiceOver")
configure_package_config_file(cmake/iiVoiceOverConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/iiVoiceOverConfig.cmake"
    INSTALL_DESTINATION "${iiVoiceOver_PACKAGE_DIR}")
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/iiVoiceOverConfigVersion.cmake"
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMinorVersion)
install(EXPORT iiVoiceOverTargets NAMESPACE iiVoiceOver:: DESTINATION "${iiVoiceOver_PACKAGE_DIR}")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/iiVoiceOverConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/iiVoiceOverConfigVersion.cmake" DESTINATION "${iiVoiceOver_PACKAGE_DIR}")
