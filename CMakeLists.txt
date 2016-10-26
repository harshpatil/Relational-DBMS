cmake_minimum_required(VERSION 3.6)
project(Relational_DBMS)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        storageManager/dberror.c
        storageManager/dberror.h
        storageManager/storage_mgr.c
        storageManager/storage_mgr.h
        storageManager/test_assign1_1.c
        storageManager/test_helper.h
        storageManager/additional_test_cases.c
        bufferManager/buffer_mgr.c)

add_executable(Relational_DBMS ${SOURCE_FILES})