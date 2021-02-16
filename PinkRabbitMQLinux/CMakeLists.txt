# Cmake script for background building 64 bit release project library PinkRabbitMQLinux

CMAKE_MINIMUM_REQUIRED(VERSION 3.1 FATAL_ERROR)


PROJECT(PinkRabbitMQLinux)

set (CMAKE_CXX_STANDARD 11)
set (ROOT_DIR ${CMAKE_SOURCE_DIR})
set (CMAKE_SOURCE_DIR ${ROOT_DIR}/src)

SET(AddInNative_SRC 
    ${CMAKE_SOURCE_DIR}/Utils.cpp
    ${CMAKE_SOURCE_DIR}/ConversionWchar.cpp
    ${CMAKE_SOURCE_DIR}/RabbitMQClient.cpp
    ${CMAKE_SOURCE_DIR}/MessageObject.cpp
    ${CMAKE_SOURCE_DIR}/AddInNative.cpp
    ${CMAKE_SOURCE_DIR}/AddInNative.h
    ${CMAKE_SOURCE_DIR}/dllmain.cpp
    ${CMAKE_SOURCE_DIR}/stdafx.cpp
    ${CMAKE_SOURCE_DIR}/stdafx.h
    )

include_directories(${CMAKE_SOURCE_DIR}/libevent/include)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/amqp/include)
include_directories(${CMAKE_SOURCE_DIR}/openssl/include)

SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${CMAKE_SOURCE_DIR})
SET(AddInDef_SRC ${CMAKE_SOURCE_DIR}/AddInNative.def)
# add_definitions(-DUNICODE -DWIN32 -DNOMINMAX)

add_library(${PROJECT_NAME} SHARED ${AddInNative_SRC} ${AddInDef_SRC})

find_library(LIBEVENT ./libevent/libevent.a)
find_library(LIBAMQP ./amqp/libamqpcpp.a)
find_library(LIBSSL ./openssl/libssl.a)
find_library(LIBCRYPTO ./openssl/libcrypto.a)

target_link_libraries(${PROJECT_NAME} ${LIBSSL} ${LIBCRYPTO} ${LIBAMQP} ${LIBEVENT} -lpthread -ldl -ljsoncpp)

set_target_properties( ${PROJECT_NAME} PROPERTIES
   CLEAN_DIRECT_OUTPUT 1
   OUTPUT_NAME ${PROJECT_NAME}Lin64
)

# -- tests

add_executable(unittest main.cpp RabbitMQClientTest.cpp)
target_link_libraries(unittest ${PROJECT_NAME})
