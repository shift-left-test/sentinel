include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(StackTest StackTest.cpp)
target_link_libraries(StackTest stack GTest::gtest_main)

add_test(
  NAME StackTest
  COMMAND StackTest --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/StackTest.xml
)
