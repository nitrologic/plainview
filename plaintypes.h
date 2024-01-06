#pragma once

struct Rect32 {
	int x,y,w,h;
	Rect32() {
		x = y = w = h = 0;
	}
	Rect32(int left,int top,int width,int height) {
		x = left;
		y = top;
		w = width;
		h = height;
	}
};

struct VideoMode {
	int mWidth, mHeight;
	VideoMode(const VideoMode& other) {
		mWidth = other.mWidth;
		mHeight = other.mHeight;
	}
	VideoMode(int width, int height) {
		mWidth = width;
		mHeight = height;
	}
};

typedef double Hz;
typedef double Zoom;

typedef std::string S;
typedef float F;

typedef uint32_t RGBA;
typedef Rect32 R;

