#version 330

uniform vec4 palette[32];

flat in int frag_bits;

out vec4 rgba;

void main(){
	rgba = palette[frag_bits & 31];
}
