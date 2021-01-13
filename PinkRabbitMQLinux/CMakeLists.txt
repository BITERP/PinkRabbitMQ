# Cmake script for background building 64 bit release project library PinkRabbitMQLinux

CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)

PROJECT(PinkRabbitMQLinux)

set (ROOT_DIR ${CMAKE_SOURCE_DIR})
set (CMAKE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(CMAKE_BUILD_TYPE Debug)

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

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/libevent/include)
include_directories(${CMAKE_SOURCE_DIR}/amqp/include)

SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${CMAKE_SOURCE_DIR})
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
SET(AddInDef_SRC ${CMAKE_SOURCE_DIR}/AddInNative.def)
# SET(CMAKE_LINK_DEF_FILE_FLAG AddInNative.def)
add_definitions(-DUNICODE -DWIN32 -DNOMINMAX)

add_library(${PROJECT_NAME} SHARED ${AddInNative_SRC} ${AddInDef_SRC})

target_link_libraries(${PROJECT_NAME} -lssl -lamqpcpp -levent -lpthread -ldl)

set_target_properties( ${PROJECT_NAME} PROPERTIES
   CLEAN_DIRECT_OUTPUT 1
   OUTPUT_NAME ${PROJECT_NAME}Lin64
)

# -- tests

add_executable(test main.cpp RabbitMQClientTest.cpp)
target_link_libraries(test ${PROJECT_NAME})
