#pragma once

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include <profileapi.h>
#endif
#ifdef __APPLE__
#include <sys/time.h>
#endif

uint64_t cpuTime() {
	uint64_t micros;
#ifdef WIN32
	LARGE_INTEGER frequency;
	LARGE_INTEGER count;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&count);
	micros = (count.QuadPart * (uint64_t)1e6) / frequency.QuadPart;
#endif
#ifdef __APPLE__
	timeval tv;
	gettimeofday( &tv, nullptr );
	micros=tv.tv_sec*1000000ULL +tv.tv_usec;
#endif
	return micros;
}
