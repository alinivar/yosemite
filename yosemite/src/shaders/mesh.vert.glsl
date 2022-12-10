
#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;

layout(location = 0) out vec4 vColor;

void main()
{
	gl_Position = vec4(position * vec3(1.0, -1.0, 1.0), 1.0);
	
	vColor = vec4(1.0, 0.8, 0.5, 1.0);
}
