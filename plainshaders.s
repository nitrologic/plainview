# plainshaders.s

	.global fragmentShader
	.global vertextShader
	.global geometryShader

	.text

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
