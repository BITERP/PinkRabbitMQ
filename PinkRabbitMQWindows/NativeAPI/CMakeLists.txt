# Cmake script for project AddInNative
# autor: 1C

CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)

PROJECT(AddInNative)

SET(AddInNative_SRC 
    AddInNative.cpp
    AddInNative.h
    dllmain.cpp
    stdafx.cpp
    stdafx.h
    )
  
include_directories(${CMAKE_SOURCE_DIR}/../include)

SET (CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${CMAKE_SOURCE_DIR})
if (UNIX)
    set_source_files_properties(${AddInNative_SRC}
                              PROPERTIES COMPILE_FLAGS
                              "-fPIC ")
else()
    add_definitions(-DUNICODE -DWIN32 )
endif()

add_library(${PROJECT_NAME} SHARED ${AddInNative_SRC} )

set_target_properties( ${PROJECT_NAME} PROPERTIES
   CLEAN_DIRECT_OUTPUT 1
   # avoid conflicts between library and binary target names
   OUTPUT_NAME ${PROJECT_NAME}${MySuffix1}${MySuffix2} 
   )

