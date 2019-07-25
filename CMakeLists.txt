# Cmake script for background building 64 bit release project library PinkRabbitMQ


CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)

PROJECT(PinkRabbitMQ)

set (ROOT_DIR ${CMAKE_SOURCE_DIR})
set (CMAKE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/NativeAPI)
set(AddInNative_SRC ${CMAKE_SOURCE_DIR}/NativeAPI)

SET(AddInNative_SRC 
    ${CMAKE_SOURCE_DIR}/CAddInNative.cpp
    ${CMAKE_SOURCE_DIR}/CAddInNative.h
    ${CMAKE_SOURCE_DIR}/dllmain.cpp
    ${CMAKE_SOURCE_DIR}/addin/stdafx.cpp
    ${CMAKE_SOURCE_DIR}/addin/stdafx.h
    ${CMAKE_SOURCE_DIR}/RabbitMQClient.cpp
    ${CMAKE_SOURCE_DIR}/RabbitMQClient.h
    ${CMAKE_SOURCE_DIR}/SimplePocoHandler.cpp
    ${CMAKE_SOURCE_DIR}/SimplePocoHandler.h
    ${CMAKE_SOURCE_DIR}/Utils.cpp
    ${CMAKE_SOURCE_DIR}/Utils.h
    )

include_directories(${CMAKE_SOURCE_DIR}/../include)
include_directories(${CMAKE_SOURCE_DIR}/../libs/AMQP-CPP-4.1.4/include)
include_directories(${CMAKE_SOURCE_DIR}/../libs/poco-poco-1.9.0-release/Foundation/include)
include_directories(${CMAKE_SOURCE_DIR}/../libs/poco-poco-1.9.0-release/Net/include)

SET (CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${CMAKE_SOURCE_DIR})
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
SET(AddInDef_SRC ${CMAKE_SOURCE_DIR}/AddInNative.def)
SET(CMAKE_LINK_DEF_FILE_FLAG AddInNative.def)
add_definitions(-DUNICODE -DWIN32 -DNOMINMAX)

# Include the directory where MyLibrary project is. Otherwise, we can't use the
# library
link_directories(${ROOT_DIR}/libsPrecompiled/releasePrec/x64Prec)

add_library(${PROJECT_NAME} SHARED ${AddInNative_SRC} ${AddInDef_SRC})

set_target_properties( ${PROJECT_NAME} PROPERTIES
   CLEAN_DIRECT_OUTPUT 1
   # avoid conflicts between library and binary target names
   OUTPUT_NAME $(ProjectName)Win64
   #WINDOWS_EXPORT_ALL_SYMBOLS TRUE
   )
# Link the executable and the library together
target_link_libraries(${PROJECT_NAME} amqpcpp.lib)
target_link_libraries(${PROJECT_NAME} PocoFoundationmt.lib)
target_link_libraries(${PROJECT_NAME} PocoNetmt.lib)

