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

if(UNIX)

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

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -std=c++17 -fPIE")

project(drmview
	LANGUAGES C CXX ASM
)

add_definitions( -DUSE_ALSA_AUDIO )

ADD_EXECUTABLE(drmview
	drmview.cpp
)

target_link_libraries(drmview m GLESv2 EGL asound cec pthread rt udev drm gbm dl -static-libstdc++)

endif(UNIX)

project(lwoview
	LANGUAGES C CXX ASM
)

ADD_EXECUTABLE(lwoview
	parselwo2.cpp
)
