
#include <assert.h>
#include <stdio.h>

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define VK_CHECK(vkcall)					\
		{									\
			VkResult result_ = (vkcall);	\
			assert(result_ == VK_SUCCESS);	\
		}

struct Swapchain
{
	VkSwapchainKHR swapchain;

	uint32_t imageCount;
	VkImage images[8];
};

VkInstance createInstance(void)
{
	VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.apiVersion = VK_API_VERSION_1_3;

	const char* extensions[] =
	{
		VK_KHR_SURFACE_EXTENSION_NAME,

#ifdef VK_USE_PLATFORM_WIN32_KHR
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
	};

	VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = ARRAYSIZE(extensions);
	createInfo.ppEnabledExtensionNames = extensions;

#ifdef _DEBUG
	const char* layers[] =
	{
		"VK_LAYER_KHRONOS_validation",
	};

	createInfo.enabledLayerCount = ARRAYSIZE(layers);
	createInfo.ppEnabledLayerNames = layers;
#endif

	VkInstance instance = 0;
	VK_CHECK(vkCreateInstance(&createInfo, 0, &instance));

	return instance;
}

VkPhysicalDevice pickPhysicalDevice(const VkPhysicalDevice* physicalDevices, uint32_t physicalDeviceCount)
{
	for (uint32_t i = 0; i < physicalDeviceCount; i++)
	{
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(physicalDevices[i], &props);

		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			printf("Picking discrete GPU: %s\n", props.deviceName);
			return physicalDevices[i];
		}
	}

	if (physicalDeviceCount > 0)
	{
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(physicalDevices[0], &props);

		printf("Picking fallback GPU: %s\n", props.deviceName);
		return physicalDevices[0];
	}

	assert(!"No physical devices available");
	return VK_NULL_HANDLE;
}

VkDevice createDevice(VkPhysicalDevice physicalDevice, uint32_t familyIndex)
{
	float queuePriority = { 1.0f };

	VkDeviceQueueCreateInfo queueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	queueInfo.queueCount = 1;
	queueInfo.queueFamilyIndex = familyIndex;
	queueInfo.pQueuePriorities = &queuePriority;

	const char* extensions[] =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	createInfo.queueCreateInfoCount = 1;
	createInfo.pQueueCreateInfos = &queueInfo;
	createInfo.enabledExtensionCount = ARRAYSIZE(extensions);
	createInfo.ppEnabledExtensionNames = extensions;

	VkDevice device = 0;
	VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, 0, &device));

	return device;
}

VkSurfaceKHR createSurface(VkInstance instance, GLFWwindow* window)
{
	VkSurfaceKHR surface = 0;

#if VK_USE_PLATFORM_WIN32_KHR
	VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	createInfo.hinstance = GetModuleHandleA(0);
	createInfo.hwnd = glfwGetWin32Window(window);

	VK_CHECK(vkCreateWin32SurfaceKHR(instance, &createInfo, 0, &surface));
#endif

	return surface;
}

void createSwapchain(Swapchain& swapchain, VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceFormatKHR format)
{
	VkSurfaceCapabilitiesKHR surfaceCaps;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps));

	VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	createInfo.surface = surface;
	createInfo.minImageCount = surfaceCaps.minImageCount;
	createInfo.imageFormat = format.format;
	createInfo.imageColorSpace = format.colorSpace;
	createInfo.imageExtent = surfaceCaps.currentExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.preTransform = surfaceCaps.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;

	VK_CHECK(vkCreateSwapchainKHR(device, &createInfo, 0, &swapchain.swapchain));
	assert(swapchain.swapchain);

	VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain.swapchain, &swapchain.imageCount, 0));
	VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain.swapchain, &swapchain.imageCount, swapchain.images));
}

void destroySwapchain(VkDevice device, Swapchain& swapchain)
{
	vkDestroySwapchainKHR(device, swapchain.swapchain, 0);
}

VkSemaphore createSemaphore(VkDevice device)
{
	VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	VkSemaphore semaphore = 0;
	VK_CHECK(vkCreateSemaphore(device, &createInfo, 0, &semaphore));

	return semaphore;
}

int main(void)
{
	VkInstance instance = createInstance();
	assert(instance);

	uint32_t physicalDeviceCount = 0;
	VkPhysicalDevice physicalDevices[16];
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, 0));
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices));

	VkPhysicalDevice physicalDevice = pickPhysicalDevice(physicalDevices, physicalDeviceCount);
	assert(physicalDevice);

	// SHORTCUT: This needs to be computed from queue family properties.
	uint32_t familyIndex = 0;
	assert(familyIndex != VK_QUEUE_FAMILY_IGNORED);

	VkDevice device = createDevice(physicalDevice, familyIndex);
	assert(device);

	VkQueue queue;
	vkGetDeviceQueue(device, familyIndex, 0, &queue);

	int r = glfwInit();
	assert(r);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	GLFWwindow* window = glfwCreateWindow(1024, 720, "Yosemite", 0, 0);
	assert(window);

	VkSurfaceKHR surface = createSurface(instance, window);
	assert(surface);

	uint32_t surfaceFormatCount = 0;
	VkSurfaceFormatKHR surfaceFormats[16];
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, 0));
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, surfaceFormats));

	// SHORTCUT: surfaceFormats[0].format might be VK_FORMAT_UNDEFINED
	VkSurfaceFormatKHR surfaceFormat = surfaceFormats[0];
	assert(surfaceFormat.format != VK_FORMAT_UNDEFINED);

	Swapchain swapchain = {};
	createSwapchain(swapchain, device, physicalDevice, surface, surfaceFormat);

	VkSemaphore acquireSemaphore = createSemaphore(device);
	assert(acquireSemaphore);

	glfwShowWindow(window);

	while (!glfwWindowShouldClose(window))
	{
		uint32_t imageIndex = 0;
		VK_CHECK(vkAcquireNextImageKHR(device, swapchain.swapchain, ~0ull, acquireSemaphore, 0, &imageIndex));

		VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapchain.swapchain;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &acquireSemaphore;
		VK_CHECK(vkQueuePresentKHR(queue, &presentInfo));

		VK_CHECK(vkDeviceWaitIdle(device));

		glfwPollEvents();
	}

	vkDestroySemaphore(device, acquireSemaphore, 0);

	destroySwapchain(device, swapchain);
	vkDestroySurfaceKHR(instance, surface, 0);

	glfwTerminate();

	vkDestroyDevice(device, 0);
	vkDestroyInstance(instance, 0);

	return 0;
}
