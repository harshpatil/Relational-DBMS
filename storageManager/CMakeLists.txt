cmake_minimum_required(VERSION 3.6)
project(storageManager)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    additional_test_cases.c
    dberror.c
    dberror.h
    Makefile
    README.txt
    storage_mgr.c
    storage_mgr.h
    test_assign1_1.c
    test_helper.h)

add_executable(storageManager ${SOURCE_FILES})