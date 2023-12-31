cmake_minimum_required(VERSION 3.5)

if(APPLE)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -std=c++17")
add_definitions( -D__MACOSX_CORE__ )
endif(APPLE)

if(WIN32)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -std:c++17")
add_definitions( -DWIN32 )
endif(WIN32)

set(BUILD_SHARED_LIBS OFF)

#add_subdirectory(glfw)
add_subdirectory(SDL)

# project plainview 
# (c) Simon Armstrong 2023 
# all rights reserved

project(plainview)

include_directories(
    glfw/include
    SDL/include
)

ADD_EXECUTABLE(plainview
    platform.h
    platform.cpp
    glad/gl.h
    sdl3driver.h
    gl3engine.h
    plainview.h
    plainview.cpp
# shaders
    shaders/rayFragment.glsl
    shaders/rayVertex.glsl
# documentation
    notes.txt
    README.md
)

if(WIN32)
target_link_libraries(plainview SDL3-static Ws2_32 Mswsock Advapi32)
endif(WIN32)

if(APPLE)
target_link_libraries(plainview SDL3-static)
endif(APPLE)
