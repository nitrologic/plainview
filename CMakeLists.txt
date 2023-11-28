cmake_minimum_required(VERSION 3.5)

add_subdirectory(glfw)
add_subdirectory(SDL)

project(plainview)

if(APPLE)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -std=c++17")
add_definitions( -D__MACOSX_CORE__ )
endif(APPLE)

if(WIN32)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -std:c++17")
add_definitions( -DWIN32 )
add_definitions( -DWIN32_LEAN_AND_MEAN)
endif(WIN32)

include_directories(
    glfw/include
    SDL/include
)

ADD_EXECUTABLE(plainview
    glad/gl.h
    plainview.cpp 
)

if(WIN32)
target_link_libraries(plainview glfw SDL3-static)
endif(WIN32)

if(APPLE)
target_link_libraries(plainview glfw SDL3-static)
endif(APPLE)
