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

add_executable(TokenizerTest TokenizerTest.cpp)
target_link_libraries(TokenizerTest tokenizer GTest::gtest_main)
gtest_add_tests(TARGET TokenizerTest)

add_executable(CalculatorTest CalculatorTest.cpp)
target_link_libraries(CalculatorTest calculator GTest::gtest_main)
gtest_add_tests(TARGET CalculatorTest)
