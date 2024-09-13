#pragma once

#include "plainview.h"

#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"

#include <fstream>
#include <sstream>

extern "C"{
	const char *vertexShader;
	const char *fragmentShader;
	const char *geometryShader;
}

#define SHADER(name, path) const char* name = R\" \
#include path \
)";


//#include "shaders/rayGeometry.glsl"



typedef std::string S;

typedef GLint i32;
typedef GLuint u32;
typedef GLchar c8;
typedef GLenum E;
typedef GLsizeiptr N;


struct GLDisplay {
	u32 vao;
	u32 wbo;
	u32 vbo;

	int maxQuads;

	// last vert in tri is provoking vertex for flat attributes such as bits
	// quad order is clockwise - topleft, topright, bottomright, bottomleft

	void initWinding(int max_quads) {
		maxQuads = max_quads;
		std::vector<uint16_t> indices(max_quads * 6);
		{
			for (uint16_t i = 0; i < max_quads; i++) {
				uint16_t* i16 = &indices[i * 6];
				uint16_t i4 = i * 4;
				i16[0] = i4 + 1;
				i16[1] = i4 + 2;
				i16[2] = i4 + 0;
				i16[3] = i4 + 2;
				i16[4] = i4 + 3;
				i16[5] = i4 + 0;
			}
		}
		N size = indices.size() * 2;
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices.data(), GL_STATIC_DRAW);
	}

	void initDisplay(i32 xyz, i32 bits, int max_quads) {
		// vao - attributes
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		// wbo - winding buffer - winding indices as a string of quads 
		glGenBuffers(1, &wbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wbo);
		initWinding(max_quads);

		// vbo - array buffer
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		int bufferSize = maxQuads * (12 + 4);
		glBufferData(GL_ARRAY_BUFFER, bufferSize, 0, GL_DYNAMIC_DRAW);

		glEnableVertexAttribArray(xyz);
		glVertexAttribPointer(xyz, 3, GL_INT, GL_FALSE, 0, 0);
#ifdef _BITS
		glEnableVertexAttribArray(bits);
		glVertexAttribPointer(bits, 1, GL_INT, GL_FALSE, 0, (void *)(maxQuads * 3 * 4) );
#endif
	}


	int totalTris = 0;

	void bufferQuads(int* vertices, int *bits, int quadcount) {
		totalTris += quadcount * 2;
		int vertcount = quadcount * 4;
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertcount * 12, vertices);
		int offset = maxQuads * 12;
		glBufferSubData(GL_ARRAY_BUFFER, offset, vertcount * 4, bits);
	}

	void draw() {
//		glBindVertexArray(vao);
//		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_TRIANGLES, totalTris * 3, GL_UNSIGNED_SHORT, 0);
	}
};

const float palette32[] = {
	1,0,1,1,
	1,1,0,1,
	0,1,1,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,0,0,1,
	1,1,1,1,
	1,1,1,1,
	1,1,1,1,
	1,1,1,1,
	1,1,1,1,
	1,1,1,1,
	1,1,1,1,
	1,1,1,1,
	1,1,1,1,
	1,1,1,1,
	1,1,1,1,
	1,1,1,1,
	1,1,1,1,
	1,1,1,1,
	1,1,1,1,
	1,1,1,1,
	1,1,1,1,
	1,1,1,1,
	1,1,1,1,
	1,1,1,1
};


// 20.12 integer fixed point pixel positions

using SubPixel = int;

const SubPixel PXL = 1 << 12;

struct GLProgram {
	GLDisplay display;
	i32 program1 = 0;
	i32 xyz;
	i32 bits;
	i32 view;
	i32 palette;

	int build() {

		// load program
//		program1 = loadProgram();
		// embed progarm
		program1 = embedProgram();
		if (program1 == -1)
			return 1;
		glUseProgram(program1);
		check();
		// fetch attributes
		xyz = attribute("xyz");
#ifdef _BITS
		bits = attribute("bits");
#endif
		view = uniform("view");
		palette = uniform("palette");
		// setup display
		display.initDisplay(xyz, bits, 16000);
		check();
		// set verts
		setVertices();
		check();
		return 0;
	}

	void draw(int w, int h) {

		setPalette(palette32);
		setView(w,h);
		display.draw();
	}

	void setVertices() {
		int iverts[] = {
			PXL * 10 , PXL * 10, 0,
			PXL * 200 , PXL * 10, 0,
			PXL * 200 , PXL * 500, 0,
			PXL * 10 , PXL * 500, 0,
			PXL * 410 , PXL * 10, 0,
			PXL * 500 , PXL * 10, 0,
			PXL * 500 , PXL * 500, 0,
			PXL * 410 , PXL * 500, 0
		};

		int ibits[] = { 
			2,0,0,0,
			1,0,0,0
		};

		display.bufferQuads(iverts, ibits, 2);
	}

	void setMatrix(i32 uniform, float* matrix) {
		glUniformMatrix4fv(uniform, 1, false, matrix);
		check();
	}

	void setView(int w,int h) {
		float mx = 2.0 / ( w * PXL);
		float my = -2.0 / ( h * PXL);
		float dx = -1.0;
		float dy = 1.0;
		float identity[] =  { 
			mx, 0.0, 0.0, dx,
			0.0, my, 0.0, dy,
			0.0, 0.0, 1.0, 0.0,
			0.0, 0.0, 0.0, 1.0 
		};
		setMatrix(view, identity);
		check();
	}

	void setPalette(const float *colortable32){
		i32 uniform = palette;
		glUniform4fv(uniform, 32, colortable32);
	}

#ifdef WIN32
	S root = "../";
#else
	S root = "../";
#endif

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
			std::cout << "[GLProgram] attribute \"" << name << "\" not found" << std::endl;
		}
		return a;
	}

	i32 uniform(const char* name) {
		i32 a = glGetUniformLocation(program1, name);
		check();
		if (a == -1) {
			std::cout << "[GLProgram] uniform \"" << name << "\" not found" << std::endl;
		}
		return a;
	}

	i32 embedProgram() {
		i32 shader1 = loadShader(GL_VERTEX_SHADER, vertexShader);
		check();

		i32 shader2 = loadShader(GL_FRAGMENT_SHADER, fragmentShader);
		check();

		i32 shader3 = loadShader(GL_GEOMETRY_SHADER, geometryShader);
		check();

		return buildShaders(shader1, shader2, shader3);
	}


	i32 loadProgram() {
		std::string vertexGles = loadString("shaders/rayVertex.glsl");
		i32 shader1 = loadShader(GL_VERTEX_SHADER, vertexGles);// .data(), vertexGles.length());
		check();

		std::string fragmentGles = loadString("shaders/rayFragment.glsl");
		i32 shader2 = loadShader(GL_FRAGMENT_SHADER, fragmentGles);// .data(), fragmentGles.length());
		check();

		std::string geometryGles = loadString("shaders/rayGeometry.glsl");
		i32 shader3 = loadShader(GL_GEOMETRY_SHADER, geometryGles);// .data(), geometryGles.length());
		check();

		return buildShaders(shader1, shader2, shader3);
	}

	i32 buildShaders(i32 shader1,i32 shader2, i32 shader3){

		i32 program1 = glCreateProgram();

		glAttachShader(program1, shader1);
		glAttachShader(program1, shader3);
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

	i32 loadShader(GLenum shaderType,std::string s) {
		const char* src = s.data();
		int bytes = s.length();
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

struct GL3Engine : Engine {
	GLProgram program;

	int test() {
        const char* version = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
        std::cout << "GL_SHADING_LANGUAGE_VERSION : " << version << std::endl;

		int maxgeom;
		
		glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &maxgeom);
//		const char* maxgeom = (const char*)glGetString(GL_MAX_GEOMETRY_OUTPUT_VERTICES);
		std::cout << "GL_MAX_GEOMETRY_OUTPUT_VERTICES : " << maxgeom << std::endl;

		program.build();
		return 0;
	}

	void draw(int w, int h) {
		program.draw(w,h);
	}
};
