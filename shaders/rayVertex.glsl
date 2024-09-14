#version 330

uniform mat4 view;

in vec3 xyz;

flat out int vert_bits;

void main(){
	vec4 v = vec4( xyz.x, xyz.y, xyz.z, 1.0 );
	vec4 vv = v * view;
	vec4 i = vec4( vv.x, vv.y, 0.0, 1.0 );
	vert_bits = 0;//int( bits );
	gl_Position = i;
}
