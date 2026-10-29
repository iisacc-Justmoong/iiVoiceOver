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
option(IIVOICEOVER_BUILD_EXAMPLES "Deprecated: configure examples/native-cli separately" OFF)
if(IIVOICEOVER_BUILD_EXAMPLES)
    message(FATAL_ERROR "The native CLI is a separate consumer: configure examples/native-cli independently")
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
add_library(iiVoiceOver SHARED src/iiVoiceOver.cpp src/iiVoiceOver.h)
add_library(iiVoiceOver::iiVoiceOver ALIAS iiVoiceOver)
target_compile_features(iiVoiceOver PUBLIC cxx_std_23)
target_compile_definitions(iiVoiceOver PRIVATE IIVOICEOVER_BUILDING_LIBRARY)
target_include_directories(iiVoiceOver PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
if(APPLE)
    set(iivo_origin "@loader_path")
elseif(UNIX)
    set(iivo_origin "$ORIGIN")
endif()
set_target_properties(iiVoiceOver PROPERTIES
    VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    CXX_EXTENSIONS OFF CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES
    INSTALL_RPATH "${iivo_origin}"
    INSTALL_RPATH_USE_LINK_PATH TRUE)
if(BUILD_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME iiVoiceOver.sdk_boundary COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/sdk_boundary.py")
    add_executable(iiVoiceOver_tests tests/tts_tests.cpp)
    target_link_libraries(iiVoiceOver_tests PRIVATE iiVoiceOver::iiVoiceOver)
    set_target_properties(iiVoiceOver_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME iiVoiceOver.contracts COMMAND iiVoiceOver_tests)
    set_tests_properties(iiVoiceOver.contracts PROPERTIES TIMEOUT 30
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources")
endif()
install(TARGETS iiVoiceOver EXPORT iiVoiceOverTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES src/iiVoiceOver.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES README.md THIRD_PARTY_NOTICES.md DESTINATION ${CMAKE_INSTALL_DATADIR}/iiVoiceOver)
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DATADIR}/iiVoiceOver/docs
    FILES_MATCHING PATTERN "*.md" PATTERN "*.json")
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

configure_file(cmake/RemoveRetiredRuntimes.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/RemoveRetiredRuntimes.cmake" @ONLY)
install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/RemoveRetiredRuntimes.cmake")
