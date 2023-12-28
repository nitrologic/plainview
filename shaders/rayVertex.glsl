#version 330

uniform mat4 view;

in vec3 xyz;

out vec4 color;

void main(){
	vec4 v = vec4( xyz.x, xyz.y, xyz.z, 1.0 );

	vec4 vv = v * view;

	vec4 i = vec4( vv.x, vv.y, 0.0, 1.0 );

	color = vec4( 0.3, 1.0, 0.4, 1.0 );

	gl_Position = i;
}
