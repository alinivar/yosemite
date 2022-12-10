
#include <assert.h>
#include <stdio.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#define VK_CHECK(vkcall)					\
		{									\
			VkResult result_ = (vkcall);	\
			assert(result_ == VK_SUCCESS);	\
		}

VkInstance createInstance(void)
{
	VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &appInfo;

	VkInstance instance = 0;
	VK_CHECK(vkCreateInstance(&createInfo, 0, &instance));

	return instance;
}

int main(void)
{
	VkInstance instance = createInstance();
	assert(instance);

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

	vkDestroyInstance(instance, 0);

	return 0;
}
