/*

plainview.cpp

A plain view of the modern monitor video display landscape

Copyright © 2023 Simon Armstrong

All Rights Reserved

*/

const char *plainviewVersion = "0.4";

static bool terminateApp = false;

#include "plainview.h"
#include "sdl3driver.h"
#include "gl3engine.h"

#include "platform.h"

Monitors allMonitors;
Monitor NullMonitor;

Monitor *currentMonitor=&NullMonitor;

void addMonitor(S driver,S name, F scale, int x, int y, int w, int h) {
	currentMonitor = new Monitor(driver,name,scale,x,y,w,h);
	allMonitors.push_back(currentMonitor);
}

Driver* sdlOpen() {
	SDLDriver* driver = new SDLDriver();
	int displayCount;
	SDL_DisplayID* sdlDisplays = SDL_GetDisplays(&displayCount);
	for (int i = 0; i < displayCount; i++) {
		SDL_DisplayID displayId = sdlDisplays[i];
		SDL_PropertiesID props = SDL_GetDisplayProperties(displayId);
		SDL_Rect rect;
		const char* name = SDL_GetDisplayName(displayId);
		float scale = SDL_GetDisplayContentScale(displayId);
		int ok = SDL_GetDisplayBounds(displayId, &rect);
		//		std::cout << "display #" << i << " \"" << name << "\" " << rect.x << "," << rect.y << "," << rect.w<< "," << rect.h << " x" << scale << std::endl;
		addMonitor("SDL", name, scale, rect.x, rect.y, rect.w, rect.h);
		int modeCount;
		const SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(displayId, &modeCount);
		while (const SDL_DisplayMode* mode = *modes++) {
			//			std::cout << mode->w << "," << mode->h << " @" << mode->refresh_rate << "hz ";
			//			std::cout << " x" << mode->pixel_density;
			//			std::cout << std::endl;
			currentMonitor->addMode(mode->w, mode->h, mode->pixel_density, mode->refresh_rate);
		}
		displayId++;
	}
	return driver;
}

void dumpModes() {
	for (Monitor *m : allMonitors) {
		std::cout << m->driver << " " << m->name << " ";
		std::cout << m->rect.x << "," << m->rect.y << "," << m->rect.w << "," << m->rect.h;
		std::cout << std::endl;
		for (const DensityFrequency &mode : m->modeTypes) {
			std::cout << "\t" << mode.first << "x " << mode.second << " Hz" << std::endl;
			for (const VideoMode &videoMode : m->modeList[mode]) {
				std::cout << "\t\t" << videoMode.mWidth << " x " << videoMode.mHeight << std::endl;
			}
		}
	}
	std::cout << std::endl;
}

Socket *s;

struct TestConnection:Connection{
	virtual void onConnect(Socket* link, std::string address) {
		std::cout << "onConnect" << std::endl;
	}
};

TestConnection t;

int testServe(){
	s=Socket::open(8080,0);
	s->serve(&t);
}

int testListen() {
//	l.listen(8080, 0, nullptr);
	return 0;
}

int testLoopback() {
	Socket* c = Socket::connect("localhost", 8080);
	if (c == nullptr) return 1;
	c->write("hell", 4);
	c->close();
	return 0;
}

int main() {
	std::cout << "plainview " << plainviewVersion << std::endl;

//	int oscin = Socket::testOSCIn();
//	testListen();
	testServe();

//	testLoopback();

	Driver *sdlDriver = sdlOpen();
//	dumpModes();
	Engine* engine = new GL3Engine();
	sdlDriver->setEngine(engine);
	sdlDriver->test();
	sdlDriver->quit();
	return 0;
}
