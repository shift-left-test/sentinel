add_library(stack Stack.cpp)
target_include_directories(stack PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(stack PUBLIC $<$<BOOL:${CMAKE_TESTING_ENABLED}>:coverage_options>)

add_library(tokenizer Tokenizer.cpp)
target_include_directories(tokenizer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tokenizer PUBLIC $<$<BOOL:${CMAKE_TESTING_ENABLED}>:coverage_options>)

add_library(calculator Calculator.cpp)
target_include_directories(calculator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(calculator PUBLIC stack tokenizer $<$<BOOL:${CMAKE_TESTING_ENABLED}>:coverage_options>)

add_executable(calc main.cpp)
target_link_libraries(calc calculator)
