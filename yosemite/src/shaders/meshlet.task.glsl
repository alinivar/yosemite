
#version 460

#extension GL_EXT_shader_explicit_arithmetic_types : require
#extension GL_NV_mesh_shader : require

#extension GL_GOOGLE_include_directive : require

#include "mesh.h"

layout(local_size_x = 32) in;

layout(binding = 1) readonly buffer Meshlets
{
	Meshlet meshlets[];
};

out taskNV block
{
	uint32_t meshletIndices[32];
};

shared uint meshletCount;

void main()
{
	uint ti = gl_LocalInvocationID.x;
	uint mgi = gl_WorkGroupID.x;
	uint mi = mgi * 32 + ti;

	meshletIndices[ti] = mi;

	if (ti == 0)
		gl_TaskCountNV = 32;
}
