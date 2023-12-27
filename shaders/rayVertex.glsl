#version 410

uniform mat4 view;

in ivec4 xyzc;
out vec4 color;

void main(){
	vec4 v=vec4(xyzc.x,xyzc.y,xyzc.z,1.0);
//	v=v*model;
	v=v*view;
	float zbias=0.0;
	float d=zbias+v.z;
	d=0.0;

	vec4 i=vec4(v.x,v.y,d,1.0);

	color = vec4(1.0,1.0,1.0,1.0);

	gl_Position=i;
}
