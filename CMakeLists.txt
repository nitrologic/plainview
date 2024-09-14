cmake_minimum_required(VERSION 3.5)

# SET(CMAKE_MESSAGE_CONTEXT_SHOW)

option(BUILD_SHARED_LIBS "" OFF)

if(UNIX)
option(SDL_KMSDRM ON)
endif()

add_subdirectory(SDL)

project(plainview
	VERSION 0.0.0
	DESCRIPTION "plainview SDL2 development harness (c) 2024 Simon Armstrong All rights reserved"
	HOMEPAGE_URL "https://github.com/nitrologic/plainview"
	LANGUAGES C CXX ASM
)

if(APPLE)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -std=c++17")
add_definitions( -D__MACOSX_CORE__ )
endif(APPLE)

if(WIN32)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -std=c++17")
add_definitions( -DWIN32 )
endif(WIN32)

if(UNIX)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -std=c++17 -fPIE")
endif(UNIX)

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
	shaders/rayGeometry.glsl
	shaders/rayVertex.glsl
# documentation
	notes.txt
	README.md
#	plainshaders.s
)

if(WIN32)
target_link_libraries(plainview SDL3-static Ws2_32 Mswsock Advapi32 WinHttp)
endif(WIN32)

if(APPLE)
target_link_libraries(plainview SDL3-static curl)
endif(APPLE)

if(UNIX)
target_link_libraries(plainview SDL3-static curl)
SET_TARGET_PROPERTIES(plainview PROPERTIES POSITION_INDEPENDENT_CODE OFF)
endif(UNIX)


include(CTest)

add_test(NAME plainview COMMAND plainview --test)
