cmake_minimum_required(VERSION 3.5)

project(debug_demo)

add_compile_options(-std=c++14)


include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/libelfin
    ${CMAKE_CURRENT_SOURCE_DIR}/linenoise
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(SRC_FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/minidbg.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/linenoise/linenoise.c
)

add_executable(minidbg ${SRC_FILE})

add_custom_target(
   libelfin
   COMMAND make
   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/libelfin
)

target_link_libraries(minidbg
                      ${PROJECT_SOURCE_DIR}/libelfin/dwarf/libdwarf++.so
                      ${PROJECT_SOURCE_DIR}/libelfin/elf/libelf++.so)

add_dependencies(minidbg libelfin)