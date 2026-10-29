cmake_minimum_required(VERSION 3.31)
# iisdk:consumer-application
project(iiVoiceOverFileConsumer LANGUAGES CXX)
include(CTest)
find_package(iiVoiceOver 0.2 CONFIG REQUIRED)
find_package(iiFileProviderNative 0.8 CONFIG REQUIRED)
add_executable(iiVoiceOverFileTests App/file_test.cpp)
target_compile_features(iiVoiceOverFileTests PRIVATE cxx_std_23)
target_link_libraries(iiVoiceOverFileTests PRIVATE iiVoiceOver::iiVoiceOver iiFileProvider::Native)
add_test(NAME iiVoiceOver.FileProvider COMMAND iiVoiceOverFileTests)
set_tests_properties(iiVoiceOver.FileProvider PROPERTIES TIMEOUT 30
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../resources")
