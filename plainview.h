#pragma once

/*

plainview.h

A plain view of the modern monitor video display landscape

Copyright © 2023 Simon Armstrong

All Rights Reserved

*/

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <set>

#define _USE_MATH_DEFINES
#include <math.h>

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

typedef std::pair<Zoom, Hz> DensityFrequency;
typedef std::set<DensityFrequency> ModeTypes;
typedef std::vector<VideoMode> ModeList;
typedef std::map< DensityFrequency, ModeList> Modes;

typedef std::string S;
typedef float F;

typedef uint32_t RGBA;
typedef Rect32 R;



struct Engine {
	virtual void draw(int w, int h) = 0;
	virtual int test() = 0;
};

struct Driver {
	Engine* _engine = nullptr;
	int frameCount = 0;

	void setEngine(Engine* engine) {
		_engine = engine;
	}

	virtual int test() = 0;
	virtual int addWindow(int w, int h, int hz, int flags) = 0;
	virtual void closeWindow(int frame) = 0;
	virtual void clear(int frame) = 0;
	virtual void drawQuad(int frame, R dest, RGBA c) = 0;
	virtual void flip(int frame) = 0;
	virtual void quit() = 0;
};

struct Monitor {
	S driver;
	S name;
	Zoom zoom;
	R rect;
	ModeTypes modeTypes;
	Modes modeList;

	Monitor() {
		name = "not a display monitor";
		zoom = 0.0;
	}
	Monitor(S api,S monitorName,float scale,int x, int y,int w,int h) {
		driver = api;
		name = monitorName;
		zoom = scale;
		rect = { x,y,w,h };
	}
	Monitor(const Monitor& from) {
		driver = from.driver;
		name = from.name;
		zoom = from.zoom;

	}
	void addMode(int w, int h, Zoom mag, Hz freq) {
		DensityFrequency df(mag,freq);
		if (modeTypes.count(df)==0) {
			modeTypes.insert(df);
		}
		modeList[df].push_back({w,h});
	}
};

typedef std::vector<Monitor*> Monitors;

