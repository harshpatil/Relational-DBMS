cmake_minimum_required(VERSION 3.6)
project(Relational_DBMS)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    bufferManager/buffer_mgr.c
    bufferManager/buffer_mgr.h
    bufferManager/buffer_mgr_stat.c
    bufferManager/buffer_mgr_stat.h
    bufferManager/dberror.c
    bufferManager/dberror.h
    bufferManager/dt.h
    bufferManager/storage_mgr.c
    bufferManager/storage_mgr.h
    bufferManager/test_assign2_1.c
    bufferManager/test_helper.h
    storageManager/additional_test_cases.c
    storageManager/dberror.c
    storageManager/dberror.h
    storageManager/storage_mgr.c
    storageManager/storage_mgr.h
    storageManager/test_assign1_1.c
    storageManager/test_helper.h)

add_executable(Relational_DBMS ${SOURCE_FILES})