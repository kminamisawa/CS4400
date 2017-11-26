cmake_minimum_required(VERSION 3.6)
project(malloclab)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

#set(SOURCE_FILES main.cpp)
#add_executable(malloclab ${SOURCE_FILES})

add_custom_target(malloclab COMMAND make -C ${malloclab_SOURCE_DIR}
        CLION_EXE_DIR=${PROJECT_BINARY_DIR})