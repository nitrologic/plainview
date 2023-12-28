/*

plainview.cpp

A plain view of the modern monitor video display landscape

Copyright © 2023 Simon Armstrong

All Rights Reserved

*/

const char *plainviewVersion = "0.3";

static bool terminateApp = false;

#include <iostream>
#include <map>
#include <vector>
#include <set>

#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"

#include <SDL3/SDL.h>
#define _USE_MATH_DEFINES
#include <math.h>

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

Monitors allMonitors;
Monitor NullMonitor;

Monitor *currentMonitor=&NullMonitor;

void addMonitor(S driver,S name, F scale, int x, int y, int w, int h) {
	currentMonitor = new Monitor(driver,name,scale,x,y,w,h);
	allMonitors.push_back(currentMonitor);
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

struct Driver {
	int frameCount=0;
	virtual int test() = 0;
	virtual int addWindow(int w, int h, int hz, int flags) = 0;
	virtual void closeWindow(int frame) = 0;
	virtual void clear(int frame) = 0;
	virtual void drawQuad(int frame, R dest,RGBA c) = 0;
	virtual void flip(int frame) = 0;
	virtual void quit() = 0;
};

struct QuadDriver : Driver {
	virtual void drawQuad(int frame, R dest, RGBA c) {

	}
};

#include <fstream>
#include <sstream>

typedef std::string S;

typedef GLint i32;
typedef GLuint u32;
typedef GLchar c8;
typedef GLenum E;
typedef GLsizeiptr N;

// quad order is clockwise

struct GLDisplay {
	u32 vao;
	u32 vbo;
	u32 wbo;

	void initWinding(int max_quads) {
		std::vector<uint16_t> indices(max_quads * 6);
		{
			for (uint16_t i = 0; i < max_quads; i++) {
				uint16_t* i16 = &indices[i * 6];
				uint16_t i4 = i * 4;
				i16[0] = i4 + 0;
				i16[1] = i4 + 1;
				i16[2] = i4 + 2;
				i16[3] = i4 + 2;
				i16[4] = i4 + 3;
				i16[5] = i4 + 0;
			}
		}
		N size = indices.size() * 2;
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices.data(), GL_STATIC_DRAW);
	}

	void initDisplay(i32 attribute, int max_quads) {

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		// wbo - winding buffer - winding indices as a string of quads 

		glGenBuffers(1, &wbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wbo);
		initWinding(max_quads);


		// vbo - vertex buffer - 

		// The attribute qualifier can be used only with the data types float, vec2, vec3, vec4, mat2, mat3, and mat4.

		// glEnableVertexAttribArray uses currently bound vertex array object for the operation, 
		// whereas glEnableVertexArrayAttrib updates state of the vertex array object with ID vaobj.

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		glEnableVertexAttribArray(attribute);

//		glEnableVertexArrayAttrib(vao, attribute);

//		glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
//		glVertexAttribPointer(index, 4, GL_INT, GL_FALSE, 4 * sizeof(int), (void*)0);

//		glVertexAttribPointer(attribute, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glVertexAttribPointer(attribute, 4, GL_INT, GL_FALSE, 0, 0);
	}

	// The attribute qualifier can be used only with the data types float, vec2, vec3, vec4, mat2, mat3, and mat4.

	void bufferQuads(i32 attribute, int* vertices, int count) {

		glBufferData(GL_ARRAY_BUFFER, count * 16, vertices, GL_DYNAMIC_DRAW);
	}

	void bufferFloatQuads(i32 attribute, float *vertices, int count) {
//		glBindVertexArray(vao);
		int index = attribute;
		int dim = 4; // size in components, 1,2,3 or 4
		int offset = 0;
		int stride = 0; // tightly packed
		void* pointer = 0;
//		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
//		glEnableVertexAttribArray(0);
//		glVertexAttribPointer(index, dim, GL_FLOAT, GL_FALSE, stride, pointer);
		glBufferData(GL_ARRAY_BUFFER, count * 16, vertices, GL_DYNAMIC_DRAW);
	}

	void draw() {
//		glBindVertexArray(vao);
//		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
	}

};

struct GLProgram {
	GLDisplay display;
	i32 program1 = 0;
	i32 xyzc;
	i32 view;
//	i32 handles;
//	i32 palette;

	void setMatrix(i32 uniform, float* matrix) {
		glUniformMatrix4fv(uniform, 1, false, matrix);
		check();
	}

	void setView() {
		float mx = 0.01;
		float my = -0.01;

		float dx = -0.8;
		float dy = 0.8;

		float identity[] = 
		{ 
			mx, 0.0, 0.0, dx,

			0.0, my, 0.0, dy,

			0.0, 0.0, 1.0, 0.0,
			0.0, 0.0, 0.0, 1.0 
		};

		setMatrix(view, identity);
		check();
	}


#ifdef WIN32
	S root = "../";
#else
	S root = "../";
#endif

	int build() {

// load program
		program1 = loadProgram();
		if (program1 == -1) 
			return 1;
		glUseProgram(program1);
		check();
// fetch attributes
		xyzc = attribute("xyzc");
		view = uniform("view");
// setup display
		display.initDisplay(xyzc, 1024);
		check();
// set view
//		float identity[] = {1.0,0.0,0.0,0.0, 0.0,1.0,0.0,0.0, 0.0,0.0,1.0,0.0 ,0.0,0.0,0.0,1.0};
//		setMatrix(view, identity);
//		check();
 		setView();
// set verts
		setVertices();
		check();
//		handles = uniform("handles");
//		palette = uniform("palette");
		return 0;
	}

	void setVertices() {
		float verts[] = {
			0.0 , 0.0, 0.0, 0.0,
			20.0 , 0.0, 0.0, 0.0,
			20.0 , 20.0, 0.0, 0.0,
			0.0 , 20.0, 0.0, 0.0
		};
		int iverts[] = {
			0 , 0, 0, 0,
			20 , 0, 0, 0,
			20 , 20, 0, 0,
			0 , 20, 0, 0
		};

//		display.bufferQuads(xyzc, verts, 4);
		display.bufferQuads(xyzc, iverts, 4);
	}

	void draw() {
		setView();
		display.draw();
	}

	S loadString(S filename) {
		S path = root + filename;
		std::ifstream ifs( path );
		if (!ifs.is_open()) {
			std::cout << "loadString failure for path : " << path << std::endl;
			return "";
		}
		std::stringstream buffer;
		buffer << ifs.rdbuf();
		return buffer.str();
	}

	// https://stackoverflow.com/questions/6205981/windows-c-stack-trace-from-a-running-app

	int check() {
		E error = glGetError();
		if (error) {
			std::cout << "GL Error " << error << std::endl;
			exit(1);
		}
		return error;
	}

	i32 attribute( const char *name) {
		i32 a = glGetAttribLocation(program1, name);
		check();
		if (a == -1) {
			std::cout << "GLProgram:attribute " << name << " not found" << std::endl;
		}
		return a;
	}

	i32 uniform(const char* name) {
		i32 a = glGetUniformLocation(program1, name);
		check();
		if (a == -1) {
			std::cout << "GLProgram:uniform " << name << " not found" << std::endl;
		}
		return a;
	}

	i32 loadProgram() {
		std::string vertexGles = loadString("shaders/rayVertex.glsl");
		i32 shader1 = loadShader(GL_VERTEX_SHADER, vertexGles.data(), vertexGles.length());
		check();

		std::string fragmentGles = loadString("shaders/rayFragment.glsl");
		i32 shader2 = loadShader(GL_FRAGMENT_SHADER, fragmentGles.data(), fragmentGles.length());
		check();

		i32 program1 = glCreateProgram();
		glAttachShader(program1, shader1);
		glAttachShader(program1, shader2);
		check();

		glLinkProgram(program1);
		check();

		int status[4];
		glGetProgramiv(program1, GL_LINK_STATUS, status);
		if (status[0] == 0) {
			int status2[4];
			glGetProgramiv(program1, GL_INFO_LOG_LENGTH, status2);
			if (status2[0]) {
				i32 size = 0;
				c8 log[4096];
				glGetProgramInfoLog(program1, status2[0], &size, log);
				if (size) {
					std::cout << "program info log : " << std::string(log, size) << std::endl;
				}
			}
			glDeleteProgram(program1);
			return -1;
		}

		return program1;
	}

	i32 loadShader(GLenum shaderType,char *src, int bytes) {
		u32 shader = glCreateShader(shaderType);
		const c8* sources[] = { src,0 };
		i32 lengths[] = {bytes,0};
		glShaderSource(shader, 1, sources, lengths);
		glCompileShader(shader);
		check();
		int status[4];
		glGetShaderiv(shader, GL_COMPILE_STATUS, status);
		if (status[0] == 0) {
			int status2[4];
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, status2);
			if (status2[0]) {
				i32 size = 0;
				c8 log[4096];
				glGetShaderInfoLog(shader, status2[0], &size, log);
				if (size) {
					std::cout << "shader info log : " << std::string(log, size) << std::endl;
				}
			}
			glDeleteShader(shader);
			return 0;
		}
		check();
		return shader;
	}

};

struct GL4Engine {
	GLProgram program;

	int test() {
        const char* version = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
        std::cout << "GL_SHADING_LANGUAGE_VERSION : " << version << std::endl;
		program.build();
		return 0;
	}

	int draw() {
		program.draw();
		return 0;
	}
};

struct SDLDriver : Driver {

	GL4Engine engine;

	Uint32 sdlFlags;

    SDLDriver() {
        sdlFlags = SDL_INIT_VIDEO;
        if (SDL_Init(sdlFlags) < 0) {
            std::cout << "SDL failure" << std::endl;
            return;
        }
        SDL_version version;
        SDL_GetVersion(&version);
        std::cout << "SDL " << (int)version.major << "." << (int)version.minor << "." << (int)version.patch << std::endl;
        
		// 3.2 or 4.0

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);


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

		int period = 48;
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

		engine.draw();

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

		engine.test();
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
					SDL_Keysym key = event.key.keysym;
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

Driver *sdlOpen() {
	SDLDriver *driver = new SDLDriver();
	int displayCount;
 	SDL_DisplayID* sdlDisplays = SDL_GetDisplays(&displayCount);
	for(int i=0;i<displayCount;i++){
		SDL_DisplayID displayId = sdlDisplays[i];
		SDL_PropertiesID props = SDL_GetDisplayProperties(displayId);
		SDL_Rect rect;
		const char *name = SDL_GetDisplayName(displayId);
		float scale = SDL_GetDisplayContentScale(displayId);
		int ok = SDL_GetDisplayBounds(displayId, &rect);
//		std::cout << "display #" << i << " \"" << name << "\" " << rect.x << "," << rect.y << "," << rect.w<< "," << rect.h << " x" << scale << std::endl;
		addMonitor("SDL", name, scale, rect.x, rect.y, rect.w, rect.h);
		int modeCount;
		const SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(displayId, &modeCount);
		while (const SDL_DisplayMode *mode = *modes++) {
//			std::cout << mode->w << "," << mode->h << " @" << mode->refresh_rate << "hz ";
//			std::cout << " x" << mode->pixel_density;
//			std::cout << std::endl;
			currentMonitor->addMode(mode->w, mode->h, mode->pixel_density, mode->refresh_rate);
		}
		displayId++;
	}
	return driver;
}

int testDriver(Driver* driver) {
	return driver->test();
}

int sdlMain() {
	std::cout << "plainview " << plainviewVersion << std::endl;
	Driver *sdlDriver = sdlOpen();
	dumpModes();
	testDriver(sdlDriver);
	sdlDriver->quit();
	return 0;
}

int main() {
	return sdlMain();
}
