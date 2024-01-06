#version 330

layout (triangles) in;

layout (triangle_strip, max_vertices = 3) out;

void main() 
{    
	for (int v = 0 ; v < 3; v++){
		gl_Position = gl_in[v].gl_Position + vec4( 0.1, -0.1, 0.0, 0.0 ); 
		EmitVertex();
	}
	
	EndPrimitive();
} 
