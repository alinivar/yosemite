
#ifndef MESH_H_
#define MESH_H_ 1

struct Vertex
{
	float vx, vy, vz;
	float nx, ny, nz;
	float tu, tv;
};

struct Meshlet
{
	vec4 cone;
	uint32_t vertices[64];
	uint32_t indicesPacked[124*3/4];
	uint8_t triangleCount;
	uint8_t vertexCount;
};

#endif
