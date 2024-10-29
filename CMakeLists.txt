cmake_minimum_required(VERSION 3.5)

if(WIN32)
add_definitions( -DWIN32 )
endif()

option(BUILD_SHARED_LIBS "" OFF)

# add_subdirectory(SDL)

if(APPLE)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -std=c++17")
add_definitions( -D__MACOSX_CORE__ )
endif(APPLE)

if(WIN32)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -std=c++17")
add_definitions( -DWIN32 )
endif(WIN32)

if(UNIX AND NOT APPLE)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -std=c++17 -fPIE")

set(USE_OPENGLES 1)

add_compile_definitions(USE_OPENGLES=1)

include_directories(
	${CMAKE_SYSROOT}/usr/include/libdrm
	${CMAKE_SYSROOT}/usr/include/libxml2
	${CMAKE_SYSROOT}/usr/include/gbm
	${CMAKE_SYSROOT}/usr/include/hiredis
)

link_directories(
	${CMAKE_SYSROOT}/home/ec2-user/setupredis/redis-stable/deps/hiredis
)

project(drmview
	LANGUAGES C CXX ASM
)

add_definitions( -DUSE_ALSA_AUDIO )

ADD_EXECUTABLE(drmview
	drmview.cpp
)

#target_link_libraries(drmview m GLESv2 EGL asound cec pthread rt udev drm gbm dl -static-libstdc++)

target_link_libraries(drmview m GLESv2 EGL asound pthread rt udev drm gbm dl stdc++)

endif(UNIX AND NOT APPLE)

#endif(UNIX)

if(APPLE)

endif(APPLE)

project(lwoview
	LANGUAGES C CXX ASM
)

ADD_EXECUTABLE(lwoview
	parselwo2.cpp
)

# PROJECT PLAINVIEW

add_subdirectory(SDL)

project(plainview
	VERSION 0.0.0
	DESCRIPTION "plainview SDL2 remote (c) 2024 Simon Armstrong All rights reserved"
	HOMEPAGE_URL "https://github.com/nitrologic/plainview"
	LANGUAGES C CXX ASM
)

include_directories(
	/usr/include/drm
	glfw/include
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

