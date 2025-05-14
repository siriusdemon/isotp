cmake_minimum_required(VERSION 3.5)

project(rkcanfd VERSION 1.0)

set(CMAKE_C 11)
set(CMAKE_C_STANDARD_REQUIRED true)



add_library(rkcanfd SHARED rkcanfd.c)


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(rkcanfd PUBLIC Threads::Threads)

add_executable(testiostp main.c)
target_link_libraries(testiostp rkcanfd)
set_target_properties(testiostp PROPERTIES
    INSTALL_RPATH "$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE
)

