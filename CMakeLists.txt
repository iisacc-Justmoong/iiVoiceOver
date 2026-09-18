cmake_minimum_required(VERSION 3.24)
project(iiVoiceOver VERSION 0.1.0 LANGUAGES CXX)

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
find_package(Qt6 6.8.3 EXACT REQUIRED COMPONENTS Core)

add_library(iiVoiceOver SHARED src/iiVoiceOver.cpp src/iiVoiceOver.h)
add_library(iiVoiceOver::iiVoiceOver ALIAS iiVoiceOver)
target_compile_features(iiVoiceOver PUBLIC cxx_std_20)
target_compile_definitions(iiVoiceOver PRIVATE IIVOICEOVER_BUILDING_LIBRARY)
target_link_libraries(iiVoiceOver PUBLIC Qt6::Core)
target_include_directories(iiVoiceOver PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
set_target_properties(iiVoiceOver PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    INSTALL_RPATH_USE_LINK_PATH TRUE)

if(BUILD_TESTING)
    add_executable(iiVoiceOver_hello_world tests/hello_world.cpp)
    target_link_libraries(iiVoiceOver_hello_world PRIVATE iiVoiceOver::iiVoiceOver)
    set_target_properties(iiVoiceOver_hello_world PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME iiVoiceOver.hello_world COMMAND iiVoiceOver_hello_world)
endif()

install(TARGETS iiVoiceOver EXPORT iiVoiceOverTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES src/iiVoiceOver.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES README.md DESTINATION ${CMAKE_INSTALL_DATADIR}/iiVoiceOver)
set(iiVoiceOver_PACKAGE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/iiVoiceOver")
configure_package_config_file(cmake/iiVoiceOverConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/iiVoiceOverConfig.cmake"
    INSTALL_DESTINATION "${iiVoiceOver_PACKAGE_DIR}")
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/iiVoiceOverConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
install(EXPORT iiVoiceOverTargets
    NAMESPACE iiVoiceOver::
    DESTINATION "${iiVoiceOver_PACKAGE_DIR}")
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/iiVoiceOverConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/iiVoiceOverConfigVersion.cmake"
    DESTINATION "${iiVoiceOver_PACKAGE_DIR}")
