/*

plainview.cpp

A plain view of the modern monitor video display landscape

Copyright © 2023 Simon Armstrong

All Rights Reserved

*/

const char *plainviewVersion = "0.4";

static bool terminateApp = false;

#include "json.h"
#include "plainview.h"
#include "sdl3driver.h"
#include "gl3engine.h"

#include "platform.h"

#ifndef __unixsucks__
//#define RUN_TEST
#define RUN_PROMPT
#endif

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
		
		SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(displayId, &modeCount);
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

void dumpHelp(){
	std::cout << std::endl;
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

// for each monitor { name shape driver modes }

utf8 jsonifyMonitors() {
	std::stringstream ss;	
	bool inmon=false;
	ss<<"[";
	for (Monitor *m : allMonitors) {
		if (inmon) ss << ",";
		else inmon=true;
		ss << "{\"name\":"<< quoted(m->name) << ",";
		ss << "\"shape\":[";
		ss << m->rect.x << "," << m->rect.y << "," << m->rect.w << "," << m->rect.h;
		ss << "],";
		ss << "\"driver\":" << quoted(m->driver) << ",";
		ss << "\"modes\":[";
		bool inarray=false;
		for (const DensityFrequency &mode : m->modeTypes) {
			for (const VideoMode &videoMode : m->modeList[mode]) {
				if (inarray) ss << ",";
				else inarray=true;
				ss << "[" << videoMode.mWidth << "," << videoMode.mHeight << "," << mode.first << "," << mode.second << "]";
			}
		}
		ss << "]}" << std::endl;
	}
	ss<<"]";
	return ss.str();
}


Socket *s;



struct TestConnection:Connection{
	virtual void onConnect(Socket* link, std::string address) {
		std::cout << "onConnect" << std::endl;
		Device *connection = openSocket(address, link);
	}
};

TestConnection t;

int testServe(){
	s=Socket::open(8080,0);
	s->serve(&t);
	return 0;
}

void startServe(){
//	std::thread 
}

Socket l(0);

int testListen() {
	l.listen(8080, 0, nullptr);
	int accept = l.accept();
	return 0;
}

int testLoopback() {
	Socket* c = Socket::connect("localhost", 8080);
	if (c == nullptr) return 1;
	c->send("hell", 4);
	c->close();
	return 0;
}

int testPing(){
//	pingHost(L"PlainViewPeer", L"localhost", 8080);
//	pingHost(L"PlainViewPeer", L"google.com", 80);
	pingHost("PlainViewPeer", "example.com", 80);
	return 0;
}

//#include "plainbox2d.h"

//#include <box2d/box2d.h>

int main(int argc, char* argv[]) {

	uint64_t t = cpuTime();

	initSystem();

	std::cout << "plainview " << plainviewVersion << std::endl;

	t=cpuTime()-t;

	std::cout << "elapsed micros " << t << std::endl;

	if(argc>1 && strcmp(argv[1],"--test")==0){
		return 0;
	}

//	Box2DSim *sim = new Box2DSim();

//	int oscin = Socket::testOSCIn();
//	int oscout = Socket::testOSCOut();

	//	testListen();
//	testServe();
//	testPing();
//	testLoopback();

#ifdef RUN_PROMPT

	Driver* sdlDriver = sdlOpen();

	if (sdlDriver == nullptr) {
		std::cout << "sdlDriver failure" << std::endl;
		return 1;
	}

	Engine* engine = new GL3Engine();
	sdlDriver->setEngine(engine);

	while (true)
	{
		std::cout << "]" << std::flush;

		S line;

		std::getline(std::cin, line);

		if (line == "help") {
			dumpHelp();
		}
		if (line == "modes") {
			S modes = jsonifyMonitors();
			std::cout << modes << std::endl;
//			dumpModes();
//			sdlDriver->config();
		}
		if (line == "test") {
			sdlDriver->test();
		}
		if (line == "exit") {
			break;
		}
	}

	sdlDriver->quit();

#endif


#ifdef RUN_TEST

	Driver *sdlDriver = sdlOpen();

	if(sdlDriver==nullptr){
		std::cout << "sdlDriver failure" << std::endl;
		return 1;
	}

//	dumpModes();
	Engine* engine = new GL3Engine();
	sdlDriver->setEngine(engine);
	sdlDriver->test();
	sdlDriver->quit();

	return 0;
#endif

	return 1;
}
