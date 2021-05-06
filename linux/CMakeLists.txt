# Cmake script for background building 64 bit release project library PinkRabbitMQLinux

CMAKE_MINIMUM_REQUIRED(VERSION 3.1 FATAL_ERROR)


PROJECT(NativeSleepLinux)

set (CMAKE_CXX_STANDARD 11)
set (ROOT_DIR ${CMAKE_SOURCE_DIR}/..)
set (CMAKE_SOURCE_DIR ${ROOT_DIR}/src)

SET(AddInNative_SRC 
    ${CMAKE_SOURCE_DIR}/AddInNative.cpp
    ${CMAKE_SOURCE_DIR}/SleepImpl.cpp
    ${CMAKE_SOURCE_DIR}/SleepNative.cpp
    )

include_directories(${CMAKE_SOURCE_DIR})

add_library(${PROJECT_NAME} SHARED ${AddInNative_SRC})

target_link_libraries(${PROJECT_NAME} -lpthread)

#tests

add_executable(unittest tests.cpp)
target_link_libraries(unittest ${PROJECT_NAME})

enable_testing()

add_test(AllTests unittest)

