
#version 460

#extension GL_EXT_shader_explicit_arithmetic_types : require
#extension GL_NV_mesh_shader : require

#extension GL_GOOGLE_include_directive : require

#extension GL_KHR_shader_subgroup_ballot: require

#include "mesh.h"

#define CULL 1

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

bool coneCull(vec4 cone, vec3 view)
{
	return dot(cone.xyz, view) > cone.w;
}

void main()
{
	uint ti = gl_LocalInvocationID.x;
	uint mgi = gl_WorkGroupID.x;
	uint mi = mgi * 32 + ti;

#if CULL
	bool accept = !coneCull(meshlets[mi].cone, vec3(0, 0, -1));
	uvec4 ballot = subgroupBallot(accept);

	uint index = subgroupBallotExclusiveBitCount(ballot);

	if (accept)
		meshletIndices[index] = mi;

	uint count = subgroupBallotBitCount(ballot);

	if (ti == 0)
		gl_TaskCountNV = count;
#else
	meshletIndices[ti] = mi;

	if (ti == 0)
		gl_TaskCountNV = 32;
#endif
}
