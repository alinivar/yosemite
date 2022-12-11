
#include <assert.h>
#include <stdio.h>

#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <volk.h>

#include <fast_obj.h>

#define VK_CHECK(vkcall)					\
		{									\
			VkResult result_ = (vkcall);	\
			assert(result_ == VK_SUCCESS);	\
		}

struct Vertex
{
	float vx, vy, vz;
	float nx, ny, nz;
	float tu, tv;
};

struct Mesh
{
	std::vector<Vertex> vertices;
};

struct Swapchain
{
	VkSwapchainKHR swapchain;

	uint32_t imageCount;
	VkImage images[8];
	VkImageView imageViews[8];

	uint32_t width, height;
};

struct Buffer
{
	VkBuffer buffer;
	VkDeviceMemory memory;

	void* data;
	size_t size;
};

VkImageMemoryBarrier imageBarrier(VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkImageMemoryBarrier result = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	result.image = image;
	result.srcAccessMask = srcAccessMask;
	result.dstAccessMask = dstAccessMask;
	result.oldLayout = oldLayout;
	result.newLayout = newLayout;
	result.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	result.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	result.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	result.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

	return result;
}

VkBufferMemoryBarrier bufferBarrier(VkBuffer buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, size_t size)
{
	VkBufferMemoryBarrier result = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	result.buffer = buffer;
	result.srcAccessMask = srcAccessMask;
	result.dstAccessMask = dstAccessMask;
	result.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	result.offset = 0;
	result.size = size;

	return result;
}

VkInstance createInstance(void)
{
	VK_CHECK(volkInitialize());
	assert(volkGetInstanceVersion() >= VK_API_VERSION_1_3);

	VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.apiVersion = VK_API_VERSION_1_3;

	const char* extensions[] =
	{
		VK_KHR_SURFACE_EXTENSION_NAME,

#ifdef VK_USE_PLATFORM_WIN32_KHR
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif

#ifdef _DEBUG
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
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

	volkLoadInstance(instance);

	return instance;
}

#ifdef _DEBUG
VkBool32 VKAPI_CALL debugReportCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objectType,
	uint64_t object,
	size_t location,
	int32_t messageCode,
	const char* layerPrefix,
	const char* message,
	void* userData
)
{
	printf("[%s]: %s\n", layerPrefix, message);

	return VK_FALSE;
}

VkDebugReportCallbackEXT registerDebugCallback(VkInstance instance)
{
	VkDebugReportCallbackCreateInfoEXT createInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT };
	createInfo.flags =	VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
						VK_DEBUG_REPORT_WARNING_BIT_EXT |
						VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
						VK_DEBUG_REPORT_ERROR_BIT_EXT;
	createInfo.pfnCallback = debugReportCallback;

	VkDebugReportCallbackEXT debugCallback = 0;
	VK_CHECK(vkCreateDebugReportCallbackEXT(instance, &createInfo, 0, &debugCallback));

	return debugCallback;
}
#endif

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

uint32_t getGraphicsQueueFamily(VkPhysicalDevice physicalDevice)
{
	uint32_t queueCount = 0;
	VkQueueFamilyProperties queues[64];
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, 0);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queues);

	for (uint32_t i = 0; i < queueCount; i++)
		if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			return i;

	assert(!"No queue family supports graphics. Compute-only device?");
	return VK_QUEUE_FAMILY_IGNORED;
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
		VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
	};

	VkPhysicalDeviceVulkan13Features features13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features13.dynamicRendering = true;

	VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	createInfo.pNext = &features13;
	createInfo.queueCreateInfoCount = 1;
	createInfo.pQueueCreateInfos = &queueInfo;
	createInfo.enabledExtensionCount = ARRAYSIZE(extensions);
	createInfo.ppEnabledExtensionNames = extensions;

	VkDevice device = 0;
	VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, 0, &device));

	return device;
}

VkCommandPool createCommandPool(VkDevice device, uint32_t familyIndex)
{
	VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	createInfo.queueFamilyIndex = familyIndex;

	VkCommandPool pool = 0;
	VK_CHECK(vkCreateCommandPool(device, &createInfo, 0, &pool));

	return pool;
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

VkSurfaceFormatKHR getSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	uint32_t formatCount = 0;
	VkSurfaceFormatKHR formats[16];
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, 0));
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats));

	assert(formatCount > 0);
	return formats[0];
}

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format)
{
	VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	createInfo.subresourceRange.layerCount = 1;
	createInfo.subresourceRange.levelCount = 1;

	VkImageView view = 0;
	VK_CHECK(vkCreateImageView(device, &createInfo, 0, &view));

	return view;
}

void createSwapchain(Swapchain& swapchain, VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceFormatKHR format, VkSwapchainKHR old)
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
	createInfo.oldSwapchain = old;

	VK_CHECK(vkCreateSwapchainKHR(device, &createInfo, 0, &swapchain.swapchain));
	assert(swapchain.swapchain);

	VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain.swapchain, &swapchain.imageCount, 0));
	VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain.swapchain, &swapchain.imageCount, swapchain.images));

	for (uint32_t i = 0; i < swapchain.imageCount; i++)
	{
		swapchain.imageViews[i] = createImageView(device, swapchain.images[i], format.format);
		assert(swapchain.imageViews[i]);
	}

	swapchain.width = surfaceCaps.currentExtent.width;
	swapchain.height = surfaceCaps.currentExtent.height;
}

void destroySwapchain(VkDevice device, Swapchain& swapchain)
{
	for (uint32_t i = 0; i < swapchain.imageCount; i++)
		vkDestroyImageView(device, swapchain.imageViews[i], 0);

	vkDestroySwapchainKHR(device, swapchain.swapchain, 0);
}

void updateSwapchain(Swapchain& swapchain, VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceFormatKHR format)
{
	VkSurfaceCapabilitiesKHR surfaceCaps = {};
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps));

	if (surfaceCaps.currentExtent.width == 0 ||
		surfaceCaps.currentExtent.height == 0)
		return;

	if (swapchain.width != surfaceCaps.currentExtent.width ||
		swapchain.height != surfaceCaps.currentExtent.height)
	{
		Swapchain old = swapchain;
		createSwapchain(swapchain, device, physicalDevice, surface, format, old.swapchain);
		destroySwapchain(device, old);
		VK_CHECK(vkDeviceWaitIdle(device));
	}
}

VkShaderModule loadShaderModule(VkDevice device, const char* path)
{
	FILE* file = fopen(path, "rb");
	assert(file);

	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	assert(length >= 0);
	fseek(file, 0, SEEK_SET);

	char* buffer = new char[length];
	assert(buffer);

	size_t rc = fread(buffer, 1, length, file);
	assert(rc == size_t(length));
	fclose(file);

	VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.pCode = (uint32_t*)buffer;
	createInfo.codeSize = size_t(length);

	VkShaderModule shaderModule = 0;
	VK_CHECK(vkCreateShaderModule(device, &createInfo, 0, &shaderModule));

	delete[] buffer;

	return shaderModule;
}

VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device)
{
	VkDescriptorSetLayoutBinding bindings[1] = {};
	bindings[0].binding = 0;
	bindings[0].descriptorCount = 1;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	createInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
	createInfo.bindingCount = ARRAYSIZE(bindings);
	createInfo.pBindings = bindings;

	VkDescriptorSetLayout setLayout = 0;
	VK_CHECK(vkCreateDescriptorSetLayout(device, &createInfo, 0, &setLayout));

	return setLayout;
}

VkPipelineLayout createPipelineLayout(VkDevice device, VkDescriptorSetLayout setLayout)
{
	VkPipelineLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	createInfo.setLayoutCount = 1;
	createInfo.pSetLayouts = &setLayout;

	VkPipelineLayout layout = 0;
	VK_CHECK(vkCreatePipelineLayout(device, &createInfo, 0, &layout));

	return layout;
}

VkPipeline createGraphicsPipeline(VkDevice device, VkPipelineCache cache, VkPipelineLayout layout, const VkPipelineRenderingCreateInfo* renderingInfo, VkShaderModule vertShader, VkShaderModule fragShader)
{
	VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	createInfo.pNext = renderingInfo;
	createInfo.layout = layout;

	VkPipelineShaderStageCreateInfo stages[2] = {};
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].module = vertShader;
	stages[0].pName = "main";
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	
	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].module = fragShader;
	stages[1].pName = "main";
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;

	createInfo.stageCount = ARRAYSIZE(stages);
	createInfo.pStages = stages;

	VkPipelineVertexInputStateCreateInfo vertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	createInfo.pVertexInputState = &vertexInputState;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	createInfo.pInputAssemblyState = &inputAssemblyState;

	VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	createInfo.pViewportState = &viewportState;

	VkPipelineRasterizationStateCreateInfo rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.lineWidth = 1.0f;
	createInfo.pRasterizationState = &rasterizationState;

	VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.pMultisampleState = &multisampleState;

	VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	createInfo.pDepthStencilState = &depthStencilState;

	VkPipelineColorBlendAttachmentState attachments[1] = {};
	attachments[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlendState.attachmentCount = ARRAYSIZE(attachments);
	colorBlendState.pAttachments = attachments;
	createInfo.pColorBlendState = &colorBlendState;

	VkDynamicState dynamicStates[] =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicState.dynamicStateCount = ARRAYSIZE(dynamicStates);
	dynamicState.pDynamicStates = dynamicStates;
	createInfo.pDynamicState = &dynamicState;

	VkPipeline pipeline = 0;
	VK_CHECK(vkCreateGraphicsPipelines(device, cache, 1, &createInfo, 0, &pipeline));

	return pipeline;
}

uint32_t chooseMemoryType(const VkPhysicalDeviceMemoryProperties& memoryProperties, uint32_t memoryTypeBits, VkMemoryPropertyFlags flags)
{
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		if ((memoryTypeBits & (1 << i)) && ((memoryProperties.memoryTypes[i].propertyFlags & flags) == flags))
			return i;

	assert(!"No compatible memory type found.");
	return ~0u;
}

void createBuffer(Buffer& buffer, VkDevice device, const VkPhysicalDeviceMemoryProperties& memoryProperties, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags)
{
	VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	createInfo.size = size;
	createInfo.usage = usage;

	VK_CHECK(vkCreateBuffer(device, &createInfo, 0, &buffer.buffer));
	assert(buffer.buffer);

	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(device, buffer.buffer, &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.allocationSize = size;
	allocateInfo.memoryTypeIndex = chooseMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, memoryFlags);

	VK_CHECK(vkAllocateMemory(device, &allocateInfo, 0, &buffer.memory));
	VK_CHECK(vkBindBufferMemory(device, buffer.buffer, buffer.memory, 0));

	if (memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
		VK_CHECK(vkMapMemory(device, buffer.memory, 0, size, 0, &buffer.data));

	buffer.size = size;
}

void uploadBuffer(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuffer, const Buffer& src, const Buffer& dst, size_t size)
{
	VK_CHECK(vkResetCommandPool(device, commandPool, 0));

	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;

	vkCmdCopyBuffer(commandBuffer, src.buffer, dst.buffer, 1, &copyRegion);

	VK_CHECK(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, 0));
	VK_CHECK(vkDeviceWaitIdle(device));
}

void destroyBuffer(VkDevice device, Buffer& buffer)
{
	vkFreeMemory(device, buffer.memory, 0);
	vkDestroyBuffer(device, buffer.buffer, 0);
}

VkSemaphore createSemaphore(VkDevice device)
{
	VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	VkSemaphore semaphore = 0;
	VK_CHECK(vkCreateSemaphore(device, &createInfo, 0, &semaphore));

	return semaphore;
}

void loadObj(std::vector<Vertex>& vertices, const char* path)
{
	fastObjMesh* obj = fast_obj_read(path);
	assert(obj);

	size_t index_count = 0;

	for (uint32_t i = 0; i < obj->face_count; i++)
		index_count += 3 * (obj->face_vertices[i] - 2);

	vertices.resize(index_count);

	size_t vertex_offset = 0;
	size_t index_offset = 0;

	for (uint32_t i = 0; i < obj->face_count; i++)
	{
		for (uint32_t j = 0; j < obj->face_vertices[i]; j++)
		{
			fastObjIndex gi = obj->indices[index_offset + j];

			if (j >= 3)
			{
				vertices[vertex_offset + 0] = vertices[vertex_offset - 3];
				vertices[vertex_offset + 1] = vertices[vertex_offset - 1];
				vertex_offset += 2;
			}

			Vertex& v = vertices[vertex_offset++];

			v.vx = obj->positions[gi.p * 3 + 0];
			v.vy = obj->positions[gi.p * 3 + 1];
			v.vz = obj->positions[gi.p * 3 + 2];

			v.nx = obj->normals[gi.n * 3 + 0];
			v.ny = obj->normals[gi.n * 3 + 1];
			v.nz = obj->normals[gi.n * 3 + 2];
			
			v.tu = obj->texcoords[gi.t * 2 + 0];
			v.tv = obj->texcoords[gi.t * 2 + 1];
		}

		index_offset += obj->face_vertices[i];
	}

	assert(vertex_offset == index_offset);

	fast_obj_destroy(obj);
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		printf("Usage: %s <obj_file>\n", argv[0]);
		return 1;
	}

	VkInstance instance = createInstance();
	assert(instance);

#ifdef _DEBUG
	VkDebugReportCallbackEXT debugCallback = registerDebugCallback(instance);
	assert(debugCallback);
#endif

	uint32_t physicalDeviceCount = 0;
	VkPhysicalDevice physicalDevices[16];
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, 0));
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices));

	VkPhysicalDevice physicalDevice = pickPhysicalDevice(physicalDevices, physicalDeviceCount);
	assert(physicalDevice);

	uint32_t familyIndex = getGraphicsQueueFamily(physicalDevice);
	assert(familyIndex != VK_QUEUE_FAMILY_IGNORED);

	VkDevice device = createDevice(physicalDevice, familyIndex);
	assert(device);

	VkQueue queue;
	vkGetDeviceQueue(device, familyIndex, 0, &queue);

	VkCommandPool commandPool = createCommandPool(device, familyIndex);
	assert(commandPool);

	VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandBufferCount = 1;
	allocateInfo.commandPool = commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	
	VkCommandBuffer commandBuffer = 0;
	VK_CHECK(vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer));

	int r = glfwInit();
	assert(r);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	GLFWwindow* window = glfwCreateWindow(1024, 720, "Yosemite", 0, 0);
	assert(window);

	VkSurfaceKHR surface = createSurface(instance, window);
	assert(surface);

	VkSurfaceFormatKHR surfaceFormat = getSurfaceFormat(physicalDevice, surface);

	Swapchain swapchain = {};
	createSwapchain(swapchain, device, physicalDevice, surface, surfaceFormat, VK_NULL_HANDLE);

	VkShaderModule meshVertShader = loadShaderModule(device, "src/shaders/mesh.vert.spv");
	assert(meshVertShader);

	VkShaderModule meshFragShader = loadShaderModule(device, "src/shaders/mesh.frag.spv");
	assert(meshFragShader);

	VkDescriptorSetLayout meshSetLayout = createDescriptorSetLayout(device);
	assert(meshSetLayout);

	VkPipelineLayout meshLayout = createPipelineLayout(device, meshSetLayout);
	assert(meshLayout);

	VkFormat colorFormats[] = { surfaceFormat.format };

	VkPipelineRenderingCreateInfo meshRenderingInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	meshRenderingInfo.colorAttachmentCount = ARRAYSIZE(colorFormats);
	meshRenderingInfo.pColorAttachmentFormats = colorFormats;

	VkPipeline meshPipeline = createGraphicsPipeline(device, 0, meshLayout, &meshRenderingInfo, meshVertShader, meshFragShader);
	assert(meshPipeline);

	VkPhysicalDeviceMemoryProperties memoryProperties = {};
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	std::vector<Vertex> vertices;
	loadObj(vertices, argv[1]);

	Buffer scratch = {};
	createBuffer(scratch, device, memoryProperties, 128 * 1024 * 1024, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	Buffer vb = {};
	createBuffer(vb, device, memoryProperties, 128 * 1024 * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	Buffer ib = {};
	createBuffer(ib, device, memoryProperties, 128 * 1024 * 1024, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	memcpy(scratch.data, vertices.data(), vertices.size()  * sizeof(Vertex));
	uploadBuffer(device, queue, commandPool, commandBuffer, scratch, vb, vertices.size() * sizeof(Vertex));

	VkSemaphore acquireSemaphore = createSemaphore(device);
	assert(acquireSemaphore);

	VkSemaphore submitSemaphore = createSemaphore(device);
	assert(submitSemaphore);

	glfwShowWindow(window);

	while (!glfwWindowShouldClose(window))
	{
		{
			int width, height;
			glfwGetWindowSize(window, &width, &height);

			while (width == 0 || height == 0)
			{
				glfwWaitEvents();
				glfwGetWindowSize(window, &width, &height);
			}

			updateSwapchain(swapchain, device, physicalDevice, surface, surfaceFormat);
		}

		uint32_t imageIndex = 0;
		VK_CHECK(vkAcquireNextImageKHR(device, swapchain.swapchain, ~0ull, acquireSemaphore, 0, &imageIndex));

		VK_CHECK(vkResetCommandPool(device, commandPool, 0));

		VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		VkRenderingAttachmentInfo colorAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.imageView = swapchain.imageViews[imageIndex];
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.clearValue.color = { 0.1f, 0.1f, 0.15f, 1.0f };

		VkRenderingInfo passInfo = { VK_STRUCTURE_TYPE_RENDERING_INFO };
		passInfo.layerCount = 1;
		passInfo.colorAttachmentCount = 1;
		passInfo.pColorAttachments = &colorAttachment;
		passInfo.renderArea.extent = { swapchain.width, swapchain.height };

		VkImageMemoryBarrier renderBarrier = imageBarrier(swapchain.images[imageIndex], 0, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &renderBarrier);

		vkCmdBeginRendering(commandBuffer, &passInfo);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline);

		VkViewport viewport = { 0.0f, 0.0f, float(swapchain.width), float(swapchain.height), 0.0f, 1.0f };
		VkRect2D scissor = { {0, 0}, {swapchain.width, swapchain.height} };

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkDescriptorBufferInfo vbInfo = {};
		vbInfo.buffer = vb.buffer;
		vbInfo.offset = 0;
		vbInfo.range = vb.size;

		VkWriteDescriptorSet descriptors[1] = {};
		descriptors[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptors[0].dstBinding = 0;
		descriptors[0].descriptorCount = 1;
		descriptors[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptors[0].pBufferInfo = &vbInfo;

		vkCmdPushDescriptorSetKHR(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshLayout, 0, ARRAYSIZE(descriptors), descriptors);

		vkCmdDraw(commandBuffer, vertices.size(), 1, 0, 0);

		vkCmdEndRendering(commandBuffer);
		
		VkImageMemoryBarrier presentBarrier = imageBarrier(swapchain.images[imageIndex], 0, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &presentBarrier);

		VK_CHECK(vkEndCommandBuffer(commandBuffer));

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.pWaitDstStageMask = &waitStage;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &acquireSemaphore;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &submitSemaphore;
		VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapchain.swapchain;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &submitSemaphore;
		VK_CHECK(vkQueuePresentKHR(queue, &presentInfo));

		VK_CHECK(vkDeviceWaitIdle(device));

		glfwPollEvents();
	}

	vkDestroySemaphore(device, submitSemaphore, 0);
	vkDestroySemaphore(device, acquireSemaphore, 0);

	destroyBuffer(device, ib);
	destroyBuffer(device, vb);
	destroyBuffer(device, scratch);

	vkDestroyPipeline(device, meshPipeline, 0);
	vkDestroyPipelineLayout(device, meshLayout, 0);
	vkDestroyDescriptorSetLayout(device, meshSetLayout, 0);
	vkDestroyShaderModule(device, meshFragShader, 0);
	vkDestroyShaderModule(device, meshVertShader, 0);

	destroySwapchain(device, swapchain);
	vkDestroySurfaceKHR(instance, surface, 0);

	glfwTerminate();

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	vkDestroyCommandPool(device, commandPool, 0);

	vkDestroyDevice(device, 0);

#ifdef _DEBUG
	vkDestroyDebugReportCallbackEXT(instance, debugCallback, 0);
#endif

	vkDestroyInstance(instance, 0);

	return 0;
}
