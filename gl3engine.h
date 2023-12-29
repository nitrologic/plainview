#pragma once

#include "plainview.h"

#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"

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
	u32 wbo;
	u32 vbo;

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
		glEnableVertexAttribArray(attribute);
		glVertexAttribPointer(attribute, 3, GL_INT, GL_FALSE, 0, 0);
	}

	void bufferQuads(i32 attribute, int* vertices, int count) {
		glBufferData(GL_ARRAY_BUFFER, count * 12, vertices, GL_DYNAMIC_DRAW);
	}

	void draw() {
//		glBindVertexArray(vao);
//		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
	}
};

// 20.12 integer fixed point pixel positions

struct GLProgram {
	GLDisplay display;
	i32 program1 = 0;
	i32 xyz;
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
		xyz = attribute("xyz");
		view = uniform("view");
// setup display
		display.initDisplay(xyz, 1024);
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
		int iverts[] = {
			0 , 0, 0,
			20 , 0, 0,
			20 , 20, 0,
			0 , 20, 0
		};
		display.bufferQuads(xyz, iverts, 4);
	}

	void setfVertices() {
		float verts[] = {
			0.0 , 0.0, 0.0, 0.0,
			20.0 , 0.0, 0.0, 0.0,
			20.0 , 20.0, 0.0, 0.0,
			0.0 , 20.0, 0.0, 0.0
		};
//		display.bufferQuads(xyzc, verts, 4);
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

struct GL3Engine : Engine {
	GLProgram program;

	int test() {
        const char* version = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
        std::cout << "GL_SHADING_LANGUAGE_VERSION : " << version << std::endl;
		program.build();
		return 0;
	}

	void draw() {
		program.draw();
	}
};


