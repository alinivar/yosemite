
#version 460

#extension GL_EXT_shader_explicit_arithmetic_types : require
#extension GL_NV_mesh_shader : require

#extension GL_GOOGLE_include_directive : require

#include "mesh.h"

layout(local_size_x = 32) in;
layout(triangles, max_vertices = 64, max_primitives = 124) out;

layout(binding = 0) readonly buffer Vertices
{
	Vertex vertices[];
};

layout(binding = 1) readonly buffer Meshlets
{
	Meshlet meshlets[];
};

in taskNV block
{
	uint32_t meshletIndices[32];
};

layout(location = 0) out vec4 vColor[];

void main()
{
	uint ti = gl_LocalInvocationID.x;
	uint mi = meshletIndices[gl_WorkGroupID.x];

	uint vertexCount = meshlets[mi].vertexCount;
	uint triangleCount = meshlets[mi].triangleCount;
	uint indexCount = triangleCount * 3;

	for (uint i = ti; i < vertexCount; i += 3)
	{
		uint vi = meshlets[mi].vertices[i];

		vec3 position = vec3(vertices[vi].vx, -vertices[vi].vy, vertices[vi].vz * 0.5 + 0.5);
		vec3 normal = vec3(vertices[vi].nx, vertices[vi].ny, vertices[vi].nz);
		vec2 texcoord = vec2(vertices[vi].tu, vertices[vi].tv);

		gl_MeshVerticesNV[i].gl_Position = vec4(position, 1.0);
	
		vColor[i] = vec4(normal * 0.5 + 0.5, 1.0);
	}

	uint indexGroupCount = (indexCount + 3) / 4;

	for (uint i = ti; i < indexGroupCount; i += 32)
	{
		writePackedPrimitiveIndices4x8NV(i * 4, meshlets[mi].indicesPacked[i]);
	}

	if (ti == 0)
		gl_PrimitiveCountNV = uint(meshlets[mi].triangleCount);
}
