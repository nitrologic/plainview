#pragma once

#include "nitro.h"

#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>


const char *FragmentShader=R"(
#version 100
precision mediump float;
uniform vec4 color;
void main(){
	gl_FragColor=color;
}
)";
const char *VertexShader=R"(
#version 100
attribute vec4 xyzw;
void main(){
	vec4 v=vec4(xyzw.x,xyzw.y,xyzw.z,1.0);
	gl_Position=v;
}
)";

#define SignalError(num,err,msg) {if(err){std::cout << "ERROR "<< msg << std::endl;raise(SIGSTOP);return num;}}

#define CheckError(num,err,msg) {if(err){std::cout << "ERROR "<< msg << std::endl;return num;}}

#define GLSignalError(num,msg) {int error=glGetError();if(error){std::cout << "GLERROR "<<msg << ":" << error << std::endl;raise(SIGSTOP);return num;}}

#define GLThrowError(num,msg) {int error=glGetError();if(error){std::cout << "GLERROR "<<msg << ":" << error << std::endl;return num;}}

#define GLCheckError(num,msg) {int error=glGetError();if(error){std::cout << "GLERROR "<<msg << ":" << error << std::endl;return num;}}

struct EGLGraphics{

	std::string staticDir;
	char infoLog[8000];

	int embedGLSL(std::string source,GLenum shaderType,GLuint &name){
		GLuint shader = glCreateShader(shaderType);
		GLCheckError(3,"loadGLSL glCreateShader");

		const GLchar* sources[2]={(const GLchar*)source.c_str(),0};

		glShaderSource(shader,1,sources,0);
		GLCheckError(4,"loadGLSL glShaderSource");

		glCompileShader(shader);
		GLCheckError(5,"loadGLSL glCompileShader");

		GLint compileStatus;
		glGetShaderiv(shader,GL_COMPILE_STATUS,&compileStatus);
		if (!compileStatus) {
			GLsizei result=0;
			glGetShaderInfoLog(shader,8000,&result,infoLog);
			std::cout << "loadGLSL shader compile error " << infoLog << std::endl;
			return 6;
		}

		name=shader;
		return 0;
	}

	int loadGLSL(std::string path,GLenum shaderType,GLuint &name){

		struct stat filestat;
		int statError=stat(path.c_str(),&filestat);
		ErrorCheck(statError,"logGLSL stat error for "<<path);
		ssize_t size=filestat.st_size;

		char *chars=new char[size+1];
		int fd=open(path.c_str(),O_RDONLY);
		FDWarnCheck(fd,1,"loadGLSL open failure for path " << path);
		ssize_t red=read(fd,chars,size);
		if(red<size){
			std::cout << "loadGLSL error reading file" << std::endl;
			return 2;
		}
		close(fd);
		chars[size]=0;

		GLuint shader = glCreateShader(shaderType);
		GLCheckError(3,"loadGLSL glCreateShader");

		glShaderSource(shader,1,&chars,0);
		GLCheckError(4,"loadGLSL glShaderSource");

		glCompileShader(shader);
		GLCheckError(5,"loadGLSL glCompileShader");

		GLint compileStatus;
		glGetShaderiv(shader,GL_COMPILE_STATUS,&compileStatus);
		if (!compileStatus) {
			GLsizei result=0;
			glGetShaderInfoLog(shader,8000,&result,infoLog);
			std::cout << "loadGLSL shader compile error " << infoLog << std::endl;
			return 6;
		}

		name=shader;
		return 0;
	}

	int createBuffer(GLenum bufferType,GLint buffer){
		GLuint bo;
		glGenBuffers(1,&bo);
		glBindBuffer(bufferType,bo);
		buffer=bo;
		return 0;
	}

	int initEGL(std::string shaderPath){
		staticDir=shaderPath;

		GLuint fragment;
		GLuint vertex;

		int loadFragment=embedGLSL(FragmentShader,GL_FRAGMENT_SHADER,fragment);
		CheckError(1,loadFragment,"EGLGraphics::init");
		int loadVertex=embedGLSL(VertexShader,GL_VERTEX_SHADER,vertex);
		CheckError(2,loadVertex,"EGLGraphics::init");



#ifdef DYNAMIC_SHADERS
//		int loadFragment=loadGLSL(staticDir+"/point.fragment.glsl",GL_FRAGMENT_SHADER,fragment);
		int loadFragment=loadGLSL(staticDir+"/rayFragment.glsl",GL_FRAGMENT_SHADER,fragment);
		CheckError(1,loadFragment,"EGLGraphics::init");
//		int loadVertex=loadGLSL(staticDir+"/point.vertex.glsl",GL_VERTEX_SHADER,vertex);
		int loadVertex=loadGLSL(staticDir+"/rayVertex.glsl",GL_VERTEX_SHADER,vertex);
		CheckError(2,loadVertex,"EGLGraphics::init");
#endif

		GLuint program;
		
		program=glCreateProgram();
		GLCheckError(3,"loadGLSL glCreateProgram");

		glAttachShader(program,fragment);
		GLCheckError(4,"loadGLSL glAttachShader");
		glAttachShader(program,vertex);
		GLCheckError(5,"loadGLSL glAttachShader");

		glLinkProgram(program);
		GLCheckError(6,"loadGLSL glLinkProgram");

		GLint linkStatus;
		glGetProgramiv(program,GL_LINK_STATUS,&linkStatus);
		if (!linkStatus){
			GLsizei result=0;
			glGetProgramInfoLog(program,8000,&result,infoLog);
//			glDeleteProgram(program0);
			std::cout << "loadGLSL shader link error " << infoLog << std::endl;
			return 7;
		}

		glUseProgram(program);
		GLCheckError(8,"loadGLSL glUseProgram");

		GLint buffer;

		int bufferError=createBuffer(GL_ARRAY_BUFFER,buffer);
		CheckError(9,bufferError,"loadGLSL createBuffer");

//		glBindBuffer(GL_ARRAY_BUFFER, buffer);
//		GLCheckError(10,"loadGLSL glBindBuffer");

		GLint xyzwIndex=glGetAttribLocation(program,"xyzw");

		glVertexAttribPointer(xyzwIndex, 4, GL_SHORT, false, 0, 0);
		GLCheckError(12,"loadGLSL glVertexAttribPointer");

		glEnableVertexAttribArray(xyzwIndex);
		GLCheckError(13,"loadGLSL enableVertexAttribArray");

		return 0;
	}

	void clear(float r,float g,float b,float a){
		glClearColor(r,g,b,a);
		glClear(GL_COLOR_BUFFER_BIT);
		spamLines();
	}

	int spamLines(){
		uint16_t h=100;
		uint16_t w=200;		
		uint16_t coords[]={h,h,0,w,0,0,0,w};
		
		glBufferData(GL_ARRAY_BUFFER, 16, coords, GL_STATIC_DRAW);
		GLCheckError(11,"loadGLSL glBufferData");
		glDrawArrays(GL_LINES, 0, 2);
		return 0;
	}
};
