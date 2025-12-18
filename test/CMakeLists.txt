include(GoogleTest)

file(GLOB TEST_SOURCES "*.cpp")

add_executable(iec61850_tests ${TEST_SOURCES})
target_link_libraries(iec61850_tests iec61850_sv gtest_main)

gtest_discover_tests(iec61850_tests)