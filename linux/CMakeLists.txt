# Cmake script for background building 64 bit release project library PinkRabbitMQLinux

CMAKE_MINIMUM_REQUIRED(VERSION 3.1 FATAL_ERROR)


PROJECT(PinkRabbitMQLinux)

set (CMAKE_CXX_STANDARD 11)
set (ROOT_DIR ${CMAKE_SOURCE_DIR}/..)
set (CMAKE_SOURCE_DIR ${ROOT_DIR}/src)
set (LIB_DIR ${ROOT_DIR}/lib/linux)

SET(AddInNative_SRC 
    ${CMAKE_SOURCE_DIR}/linux/ConnectionImpl.cpp
    ${CMAKE_SOURCE_DIR}/Connection.cpp
    ${CMAKE_SOURCE_DIR}/RabbitMQClient.cpp
    ${CMAKE_SOURCE_DIR}/RabbitMQClientNative.cpp
    ${CMAKE_SOURCE_DIR}/AddInNative.cpp
    )

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/linux/libevent)
include_directories(${CMAKE_SOURCE_DIR}/linux/openssl)

add_library(${PROJECT_NAME} SHARED ${AddInNative_SRC})

find_library(LIBEVENT ${LIB_DIR}/libevent.a)
find_library(LIBAMQP ${LIB_DIR}/libamqpcpp.a)
find_library(LIBSSL ${LIB_DIR}/libssl.a)
find_library(LIBCRYPTO ${LIB_DIR}/libcrypto.a)


target_link_libraries(${PROJECT_NAME} ${LIBSSL} ${LIBCRYPTO} ${LIBAMQP} ${LIBEVENT} -lpthread)

#tests

add_executable(unittest tests.cpp common.cpp)
target_link_libraries(unittest ${PROJECT_NAME})

enable_testing()

add_test(AllTests unittest)

