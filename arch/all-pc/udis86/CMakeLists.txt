cmake_minimum_required(VERSION 3.12)
project(udis86 LANGUAGES C)

include_directories("${PROJECT_SOURCE_DIR}")

add_subdirectory(libudis86)
add_subdirectory(udcli)

add_library(udis86 INTERFACE)
target_include_directories(udis86 INTERFACE ${CMAKE_SOURCE_DIR})
target_link_libraries(udis86 INTERFACE libudis86)
