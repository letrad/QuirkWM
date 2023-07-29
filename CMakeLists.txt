cmake_minimum_required(VERSION 3.25)
project(what C)

set(CMAKE_C_STANDARD 11)
find_package(X11 REQUIRED)
include_directories(${X11_INCLUDE_DIR})

add_executable(what main.c)
target_link_libraries(what ${X11_LIBRARIES})