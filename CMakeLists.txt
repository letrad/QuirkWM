cmake_minimum_required(VERSION 3.25)
project(QuirkWM C)

set(CMAKE_C_STANDARD 11)

find_package(X11 REQUIRED)
include_directories(${X11_INCLUDE_DIR})

file(GLOB SOURCES "src/*.c")

add_executable(QuirkWM ${SOURCES})
target_link_libraries(QuirkWM ${X11_LIBRARIES})
