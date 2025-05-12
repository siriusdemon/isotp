cmake_minimum_required(VERSION 3.5)

project(rkcanfd VERSION 1.0)

set(CMAKE_C 11)
set(CMAKE_C_STANDARD_REQUIRED true)



add_library(rkcanfd SHARED rkcanfd.cpp)


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(rkcanfd PUBLIC Threads::Threads)

