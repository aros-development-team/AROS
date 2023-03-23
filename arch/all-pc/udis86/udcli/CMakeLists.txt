cmake_minimum_required(VERSION 3.12)
project(udcli LANGUAGES C)

add_executable(udcli EXCLUDE_FROM_ALL udcli.c)
target_link_libraries(udcli udis86)
