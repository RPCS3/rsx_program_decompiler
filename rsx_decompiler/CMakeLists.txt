cmake_minimum_required(VERSION 2.8.12)

project(rsx_decompiler)

If(NOT SOURCE_DIR)
  SET(SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})
  Message("-- Initializing SOURCE_DIR=${SOURCE_DIR}")
Else()
  Message("-- Using Custom SOURCE_DIR=${SOURCE_DIR}")
EndIf()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fexceptions")
include_directories(${SOURCE_DIR})

file(GLOB_RECURSE SOURCE_FILES "${SOURCE_DIR}/*.cpp")

add_library(${PROJECT_NAME} ${SOURCE_FILES})
