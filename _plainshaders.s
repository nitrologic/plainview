# todo: investigate CMAKE_ASM for building glsl

# plainshaders.s
#	.text
#	.data

	.globl fragmentShader2
	.globl vertextShader2
	.globl geometryShader2

fragmentShader: 
	.long .
fragment1:
	.incbin "../shaders/rayFragment.glsl"
	.byte 0

vertexShader: 
	.long .
vertex1:
	.incbin "../shaders/rayVertex.glsl"
	.byte 0

geometryShader:
	.long .
geometry1:
	.incbin "../shaders/rayGeometry.glsl"
	.byte 0

