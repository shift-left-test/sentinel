include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(StackTest StackTest.cpp)
target_link_libraries(StackTest stack GTest::gtest_main)

gtest_add_tests(TARGET StackTest)
