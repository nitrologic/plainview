cmake_minimum_required(VERSION 3.5)

if(APPLE)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -std=c++17")
add_definitions( -D__MACOSX_CORE__ )
endif(APPLE)

if(WIN32)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -std:c++17")
add_definitions( -DWIN32 )
endif(WIN32)

if(UNIX)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -std=c++17")
# add_definitions( -D__MACOSX_CORE__ )
endif(UNIX)

set(BUILD_SHARED_LIBS OFF)

#add_subdirectory(glfw)

add_subdirectory(SDL)

set(BOX2D_BUILD_UNIT_TESTS OFF)
set(BOX2D_BUILD_TESTBED OFF)

add_subdirectory(box2d)

# project plainview 
# (c) Simon Armstrong 2023 
# all rights reserved

project(plainview)

include_directories(
    glfw/include
    SDL/include
    box2d/include
)

ADD_EXECUTABLE(plainview
    plainbox2d.h
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
target_link_libraries(plainview box2d SDL3-static Ws2_32 Mswsock Advapi32 WinHttp)
endif(WIN32)

if(APPLE)
target_link_libraries(plainview SDL3-static curl)
endif(APPLE)

if(UNIX)
target_link_libraries(plainview SDL3-static curl)
endif(UNIX)


include(CTest)

add_test(NAME plainview COMMAND plainview --test)
