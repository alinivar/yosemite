
#include <assert.h>
#include <stdio.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

int main(void)
{
	int r = glfwInit();
	assert(r);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	GLFWwindow* window = glfwCreateWindow(1024, 720, "Yosemite", 0, 0);
	assert(window);

	glfwShowWindow(window);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}
