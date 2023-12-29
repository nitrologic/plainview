#version 330

uniform mat4 view;

in vec3 xyz;

in float bits;

flat out int frag_bits;

void main(){
	vec4 v = vec4( xyz.x, xyz.y, xyz.z, 1.0 );

	vec4 vv = v * view;

	vec4 i = vec4( vv.x, vv.y, 0.0, 1.0 );

	frag_bits = int( bits );

	gl_Position = i;
}
