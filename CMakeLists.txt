cmake_minimum_required(VERSION 3.0)
project(smolog)

option(SMOLOG_DISABLE_MACROS "Disable macros" OFF)
option(SMOLOG_USE_LOCAL_TIME "Use local time" OFF)

add_library(smolog STATIC smolog.hpp smolog.cpp)

target_include_directories(smolog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(smolog PRIVATE _CRT_SECURE_NO_WARNINGS)

if(SMOLOG_USE_LOCAL_TIME)
	target_compile_definitions(smolog PRIVATE SMOLOG_USE_LOCAL_TIME)
endif(SMOLOG_USE_LOCAL_TIME)

if(SMOLOG_DISABLE_MACROS)
	target_compile_definitions(smolog PRIVATE SMOLOG_DISABLE_MACROS)
endif(SMOLOG_DISABLE_MACROS)

add_executable(smolog_examples examples.cpp)

target_link_libraries(smolog_examples smolog)
