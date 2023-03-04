cmake_minimum_required(VERSION 3.0.0)
project(mazegen VERSION 0.1.0)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

option(BUILD_EXAMPLE "Build the mazegen example.")
if(BUILD_EXAMPLE)
    set(EXAMPLE1 example_main)
    add_executable(${EXAMPLE1} example/main.cpp)
    target_include_directories(${EXAMPLE1} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${EXAMPLE1} PRIVATE mazegen)

    set(EXAMPLE2 example_custom_grid)
    add_executable(${EXAMPLE2} example/custom_grid.cpp)
    target_include_directories(${EXAMPLE2} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${EXAMPLE2} PRIVATE mazegen)
endif()