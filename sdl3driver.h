#pragma once

#include "plainview.h"
#include "gl3engine.h"
#include "platform.h"

#include <SDL3/SDL.h>

#define USE_OPENGL_3_2

struct SDLDriver : Driver {

//	GL4Engine engine;

	Uint32 sdlFlags;

    SDLDriver() {
        sdlFlags = SDL_INIT_VIDEO;
        if (SDL_Init(sdlFlags) < 0) {
            std::cout << "SDL failure" << std::endl;
            return;
        }
        std::cout << "SDL " << (int)SDL_MAJOR_VERSION  << "." << (int)SDL_MINOR_VERSION << "." << (int)SDL_MICRO_VERSION << std::endl;
        std::cout << "SDL_Platform " << SDL_GetPlatform() << std::endl;
		std::cout << SDL_GetPlatform() << std::endl;
        
#ifdef USE_OPENGL_3_2
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif

//		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES );
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    }

	void quit() {
		SDL_QuitSubSystem(sdlFlags);
	}

	struct SDLFrame {
		SDL_Window* window;
		SDL_Surface* surface;
		SDL_Renderer* renderer;
		SDL_Texture* texture;
	};

	std::vector<SDLFrame> frames;

	void closeWindow(int surface) {
		SDLFrame& sdlFrame = frames[surface];
		SDL_Window* window = sdlFrame.window;
		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	// todo: assert ( flags & SDL_WINDOW_OPENGL )

	int addWindow(int w, int h, int freq, int window_flags) {
		SDL_Window* window = NULL;
		window = SDL_CreateWindow("plainview", w, h, window_flags);
		if (window == NULL) {
			const char* error = SDL_GetError();
			std::cout << "could not create window " << error << std::endl;
			return -1;
		}

		SDL_GLContext glContext = SDL_GL_CreateContext(window);
		int version = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
		SDL_GL_SetSwapInterval(1);
		int frameIndex = (int)frames.size();
		frames.push_back({ window,NULL,NULL,NULL });
		return frameIndex;
	}

	void clear(int frame) {
	}

	void drawQuad(int frame, R dest, RGBA c) {
		SDLFrame& sdlFrame = frames[frame];
	}

	static const int MAX_RECT = 8192;

	void drawQuads(int frame, R* dests, RGBA c, const int count) {
		SDLFrame& sdlFrame = frames[frame];
	}

	void drawPlatter(int frame, int x, int y, int c, int radius, int denominator, int depth, float prot)
	{
		double rr = 2 * M_PI / denominator;
		for (int i = 0; i < denominator; i++) {
			for (int d = 0; d < depth; d++) {
				int r = radius + d * 6;
				int tx = x + r * sin(rr * i - prot);
				int ty = y + r * cos(rr * i - prot);
				R dest = { tx,ty,4,2 };
				drawQuad(frame, dest, c);
			}
		}
	}

	void flip(int frame) {
		SDLFrame& sdlFrame = frames[frame];
		SDL_Window* window = sdlFrame.window;
		int w, h;
		SDL_GetWindowSizeInPixels(window, &w, &h);

		int period = 148;
		int wide = 7;
		int high = 24;
		int x = 10 + frameCount % period;
		glClearColor(0.1f, 0.f, 0.4f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);
		glEnable(GL_SCISSOR_TEST);
		while (x < w) {
			glScissor(x, 0, wide, high);
			glClearColor(0.8f, 0.8f, 1.f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT);
			x += period;
		}
		glDisable(GL_SCISSOR_TEST);

		_engine->draw(w,h);

		SDL_GL_SwapWindow(window);
	}

	enum fullscreenMode {
		WINDOWED,
		FULLSCREEN,
		TOGGLE_FULLSCREEN
	};

	void fullscreen(int frame, fullscreenMode mode) {
		SDLFrame& sdlFrame = frames[frame];
		SDL_Window* window = sdlFrame.window;
		if (mode == TOGGLE_FULLSCREEN){
//			SDL_GetWindowFullscreenMode(window):
			auto flags = SDL_GetWindowFlags(window);
			mode = flags & SDL_WINDOW_FULLSCREEN ? WINDOWED : FULLSCREEN;
		}
		SDL_SetWindowFullscreen(window, mode?true:false);
	}
	
	// https://discourse.libsdl.org/t/vsync-while-software-rendering-my-solution/26824

	int fps;
	int fpsFrames=0;
	uint64_t frameTime=cpuTime();
		
	void updateFPS(){
		fpsFrames++;
		if(fpsFrames>=40){
			uint64_t t=cpuTime();
			double elapsed = (t-frameTime)/1e6;
			frameTime=t;
			fps= (0.5 + (fpsFrames / elapsed));
			fpsFrames=0;
			std::cout << "fps:" << fps << std::endl;
		}
	}
	int config() {
		return 0;
	}
	int test() {
		Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;    // | SDL_WINDOW_HIGH_PIXEL_DENSITY;
/*
#ifdef NDEBUG
		Uint32 flags = SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL;	// | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#else
		Uint32 flags = SDL_WINDOW_OPENGL;	// | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#endif
*/
//		int frame = addWindow(1280, 960, 75, flags);
		int frame = addWindow(1280, 800, 75, flags);
//		int frame = addWindow(3024, 1964, 120, flags);
		if (frame < 0) {
			std::cout << "addWindow failure" << std::endl;
			return -1;
		}

		_engine->test();
		bool running = true;
		int surface = frame;
		while (running) {
//			clear(surface);
			int count = (frameCount++) % 100;
			int g = count * 2;
			flip(surface);
			updateFPS();
			SDL_Event event;
//            if (SDL_WaitEventTimeout(&event, 5)) {
			if (SDL_PollEvent(&event)) {
				switch (event.type) {
				case SDL_EVENT_WINDOW_MAXIMIZED: {
					fullscreen(0,FULLSCREEN);
				}break;
				case SDL_EVENT_KEY_DOWN:{
					SDL_KeyboardEvent &key = event.key;
					if (key.scancode == SDL_SCANCODE_F1) {
						fullscreen(0,TOGGLE_FULLSCREEN);
					}
					if (key.scancode == SDL_SCANCODE_F10) {
						running = false;
					}
					}break;
				case SDL_EVENT_QUIT:
					running = false;
					break;
				}
			}
			SDL_Delay(2);
//			std::cout << "." << std::endl;
		}

		closeWindow(frame);
	}
};
