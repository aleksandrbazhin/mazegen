cmake_minimum_required(VERSION 3.0.0)
project(mazegen VERSION 0.1.0)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

option(BUILD_EXAMPLE "Build the mazegen example.")
if(BUILD_EXAMPLE)
    set(EXAMPLE_EXECUTABLE mazegen-example)
    add_executable(${EXAMPLE_EXECUTABLE} example/main.cpp)
    target_include_directories(${EXAMPLE_EXECUTABLE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${EXAMPLE_EXECUTABLE} PRIVATE mazegen)
endif()