
#version 460

#extension GL_EXT_shader_explicit_arithmetic_types : require

#extension GL_GOOGLE_include_directive : require

#include "mesh.h"

layout(binding = 0) readonly buffer Vertices
{
	Vertex vertices[];
};

layout(location = 0) out vec4 vColor;

void main()
{
	Vertex v = vertices[gl_VertexIndex];

	vec3 position = vec3(v.vx, -v.vy, v.vz * 0.5 + 0.5);
	vec3 normal = vec3(v.nx, v.ny, v.nz);
	vec2 texcoord = vec2(v.tu, v.tv);

	gl_Position = vec4(position, 1.0);
	
	vColor = vec4(normal * 0.5 + 0.5, 1.0);
}
