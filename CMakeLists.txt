
cmake_minimum_required(VERSION 3.2)
project(helloworld)
set(CMAKE_CXX_STANDARD 11)
add_executable(${PROJECT_NAME} src/helloworld.cpp src/PrimeChecker.cpp)

enable_testing()
add_executable(${PROJECT_NAME}-Runner src/TestPrimeChecker.cpp src/PrimeChecker.cpp)
add_test(NAME ${PROJECT_NAME}-Runner COMMAND ${PROJECT_NAME}-Runner)
