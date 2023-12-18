#version 410

uniform mat4 view;
uniform vec4 palette[16];
uniform ivec4 style[16];
uniform mat4 handles[32];

in ivec4 xyzc;
out vec4 color;

void main(){
	int w=int(xyzc.w);
	color=palette[w&15];
	mat4 model=handles[0];
	vec4 v=vec4(xyzc.x,xyzc.y,xyzc.z,1.0);
	v=v*model;
	v=v*view;
	float zbias=0.0;
	float d=zbias+v.z;
	vec4 i=vec4(v.x,v.y,d,1.0);
	gl_Position=i;
}
