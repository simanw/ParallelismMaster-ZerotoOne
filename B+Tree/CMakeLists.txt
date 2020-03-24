cmake_minimum_required(VERSION 3.15)
project(b_tree_winter20)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(b_tree_winter20
        btree.cc
        btree.h
        rwmutex.h
        main.cc)
