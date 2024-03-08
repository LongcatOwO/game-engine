module;
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <ranges>
#include <limits>
#include <utility>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <pl/macro.hpp>
#include <project_resource.hpp>

module pl.vulkan;

import pl.core;

namespace pl::vulkan
{
RE<void, SimpleError> QueueInfo::query(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    Span<VkQueueFamilyProperties const> families) noexcept
{
    indices.graphicsFamily = nullIndex;
    indices.presentFamily = nullIndex;

    for (uint32_t i = 0; auto &family : families)
    {
        if (indices.graphicsFamily == nullIndex && family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphicsFamily = i;

        if (indices.presentFamily == nullIndex)
        {
            VkBool32 presentSupport;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport == VK_TRUE)
                indices.presentFamily = i;
        }
        if (isComplete()) break;
        ++i;
    }

    return {};
}


RE<void, SimpleError> SurfaceInfo::query(VkPhysicalDevice device, VkSurfaceKHR surface) noexcept
{
    uint32_t count;
    VkResult result;

    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);
    if (result != VK_SUCCESS)
    {
        std::cerr
            << "Failed to retrieve Vulkan physical device surface capabilities: "
            << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, {});
    if (result != VK_SUCCESS)
    {
        std::cerr
            << "Failed to retrieve Vulkan physical device surface formats: "
            << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }
    PL_TRY_DISCARD(formats.resize(count));
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, formats.data());
    if (result != VK_SUCCESS)
    {
        std::cerr
            << "Failed to retrieve Vulkan physical device surface formats: "
            << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, {});
    if (result != VK_SUCCESS)
    {
        std::cerr
            << "Failed to retrieve Vulkan physical device present modes: "
            << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }
    PL_TRY_DISCARD(presentModes.resize(count));
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, presentModes.data());
    if (result != VK_SUCCESS)
    {
        std::cerr
            << "Failed to retrieve Vulkan physical device present modes: "
            << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }

    return {};
}


VkSurfaceFormatKHR SurfaceInfo::getPreferredFormat() const noexcept
{
    assert(!formats.empty());
    for (auto &format : formats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB
         && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }
    return formats[0];
}


VkPresentModeKHR SurfaceInfo::getPreferredPresentMode() const noexcept
{
    assert(!presentModes.empty());
    for (auto &mode : presentModes)
    {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return mode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D SurfaceInfo::getPreferredExtent(GLFWwindow *window) const noexcept
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    auto &minExtent = capabilities.minImageExtent;
    auto &maxExtent = capabilities.maxImageExtent;

    return VkExtent2D {
        .width = std::clamp((uint32_t) width, minExtent.width, maxExtent.width),
        .height = std::clamp((uint32_t) height, minExtent.height, maxExtent.height),
    };
}


uint32_t SurfaceInfo::getPreferredImageCount() const noexcept
{
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount != 0 && imageCount > capabilities.maxImageCount)
        return capabilities.maxImageCount;
    else
        return imageCount;
}


RE<void, SimpleError> DeviceInfo::query(VkPhysicalDevice device, VkSurfaceKHR surface) noexcept
{
    uint32_t count;
    VkResult result;

    vkGetPhysicalDeviceProperties(device, &properties);

    result = vkEnumerateDeviceExtensionProperties(device, {}, &count, {});
    if (result != VK_SUCCESS)
    {
        std::cerr
            << "Failed to retrieve Vulkan device extension properties: "
            << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }

    PL_TRY_DISCARD(extensions.resize(count));
    result = vkEnumerateDeviceExtensionProperties(device, {}, &count, extensions.data());
    if (result != VK_SUCCESS)
    {
        std::cerr
            << "Failed to retrieve Vulkan device extension properties: "
            << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }

    vkGetPhysicalDeviceFeatures(device, &features);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, {});
    PL_TRY_DISCARD(queueFamiliesProperties.resize(count));
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, queueFamiliesProperties.data());

    PL_TRY_DISCARD(queues.query(device, surface, queueFamiliesProperties));

    return {};
}


bool DeviceInfo::isSuitable() const noexcept
{
    for (char const *extension : g::config.device.extensions)
    {
        bool found = false;
        for (auto &available : extensions)
        {
            if (std::strcmp(extension, available.extensionName) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    return queues.isComplete();
}


RE<ArrayList<std::byte>, SimpleError> loadFile(char const *filename) noexcept
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        std::cerr
            << "Failed to open file \"" << filename << "\". "
            << "Reason: " << std::strerror(errno) << '\n';
        return {tags::error, getSingleton<SystemError>()};
    }

    ArrayList<std::byte> buffer;
    auto fileSize = file.tellg();
    PL_TRY_DISCARD(buffer.resize((std::size_t) fileSize));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    return buffer;
}


RE<void, SimpleError> Renderer::init() noexcept
{
    bool success = false;

    PL_TRY_ASSIGN(_window, createWindow());
    PL_DEFER(if (!success) glfwDestroyWindow(_window));

    PL_TRY_DISCARD(createInstance(&_instance, &_debugExtension, &_debugMessenger));
    PL_DEFER(
    if (!success)
    {
        if (g::config.debug.enabled)
            _debugExtension.vkDestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, {});
        vkDestroyInstance(_instance, {});
    });

    PL_TRY_ASSIGN(_surface, createSurface(_instance, _window));
    PL_DEFER(if (!success) vkDestroySurfaceKHR(_instance, _surface, {}));

    PL_TRY_DISCARD(createDevice(
        _window,
        _instance,
        _surface,
        &_physicalDevice,
        &_deviceInfo,
        &_surfaceInfo,
        &_swapchainConfig,
        &_device,
        &_graphicsQueue,
        &_presentQueue));
    PL_DEFER(if (!success) vkDestroyDevice(_device, {}));

    PL_TRY_DISCARD(createCommandPool(
        _deviceInfo, 
        _device, 
        &_commandPool,
        &_commandBuffers));
    PL_DEFER(if (!success) vkDestroyCommandPool(_device, _commandPool, {}));

    PL_TRY_DISCARD(createSwapchain(
        _surface,
        _deviceInfo,
        _surfaceInfo,
        _swapchainConfig,
        _device,
        &_swapchain,
        &_swapchainImages,
        &_swapchainImageViews));
    PL_DEFER(
    if (!success)
    {
        for (VkImageView imageView : _swapchainImageViews)
            vkDestroyImageView(_device, imageView, {});

        _swapchainImageViews.clear();

        vkDestroySwapchainKHR(_device, _swapchain, {});
    });

    PL_TRY_DISCARD(createGraphicsPipeline(
        _device,
        _swapchainConfig,
        _swapchainImageViews,
        &_renderPass,
        &_swapchainFramebuffers,
        &_pipelineLayout,
        &_pipeline));
    PL_DEFER(
    if (!success)
    {
        vkDestroyPipeline(_device, _pipeline, {});
        vkDestroyPipelineLayout(_device, _pipelineLayout, {});

        for (VkFramebuffer fb : _swapchainFramebuffers)
             vkDestroyFramebuffer(_device, fb, {});
        _swapchainFramebuffers.clear();

        vkDestroyRenderPass(_device, _renderPass, {});
    });

    PL_TRY_DISCARD(createSynchronizationObjects(
        _device, 
        &_imageAvailableSemaphores, 
        &_renderFinishedSemaphores, 
        &_inFlightFences));
    PL_DEFER(
    if (!success)
    {
        for (VkFence f : _inFlightFences)
            vkDestroyFence(_device, f, {});
        _inFlightFences.clear();

        for (VkSemaphore s : _renderFinishedSemaphores)
            vkDestroySemaphore(_device, s, {});
        _renderFinishedSemaphores.clear();

        for (VkSemaphore s : _imageAvailableSemaphores)
             vkDestroySemaphore(_device, s, {});
        _imageAvailableSemaphores.clear();
    });

    success = true;
    return {};
}


void Renderer::deinit() noexcept
{
    for (VkFence f : _inFlightFences)               vkDestroyFence(_device, f, {});
    for (VkSemaphore s : _renderFinishedSemaphores) vkDestroySemaphore(_device, s, {});
    for (VkSemaphore s : _imageAvailableSemaphores) vkDestroySemaphore(_device, s, {});

    vkDestroyPipeline(_device, _pipeline, {});
    vkDestroyPipelineLayout(_device, _pipelineLayout, {});

    for (VkFramebuffer fb : _swapchainFramebuffers)
        vkDestroyFramebuffer(_device, fb, {});

    vkDestroyRenderPass(_device, _renderPass, {});

    for (VkImageView imageView : _swapchainImageViews)
        vkDestroyImageView(_device, imageView, {});

    vkDestroySwapchainKHR(_device, _swapchain, {});
    vkDestroyCommandPool(_device, _commandPool, {});
    vkDestroyDevice(_device, {});
    vkDestroySurfaceKHR(_instance, _surface, {});

    if (g::config.debug.enabled)
        _debugExtension.vkDestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, {});

    vkDestroyInstance(_instance, {});
    glfwDestroyWindow(_window);
}


RE<void, SimpleError> Renderer::run() noexcept
{
    VkResult result;
    while (!glfwWindowShouldClose(_window))
    {
        glfwPollEvents();
        PL_TRY_DISCARD(drawFrame());
    }
    result = vkDeviceWaitIdle(_device);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to wait for Vulkan device to idle: " << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }
    return {};
}


VKAPI_ATTR VkBool32 VKAPI_CALL Renderer::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    VkDebugUtilsMessengerCallbackDataEXT const *callbackData,
    [[maybe_unused]] void *userData) noexcept
{
    char const *severityString;
    switch (messageSeverity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            severityString = "Info"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            severityString = "Verbose"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            severityString = "Warning"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            severityString = "Error"; break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
        default:
            severityString = "<Unknown Severity>"; break;
    }

    char const *typeString;
    switch ((VkDebugUtilsMessageTypeFlagBitsEXT) messageType)
    {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
            typeString = "General"; break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
            typeString = "Performance"; break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
            typeString = "Validation"; break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
            typeString = "DeviceAddressBinding"; break;

        case VK_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BITS_MAX_ENUM_EXT:
        default:
            typeString = "<Unknown Type>"; break;
    }

    std::clog << '[' << severityString << ", " << typeString << "]: " << callbackData->pMessage << '\n';

    return VK_FALSE;
}

RE<GLFWwindow *, SimpleError> Renderer::createWindow() noexcept
{
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto &c = g::config.window;

    auto window = glfwCreateWindow(c.width, c.height, c.title, {}, {});
    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        return {tags::error, getSingleton<GLFWError>()};
    }

    return window;
}


RE<void, SimpleError> Renderer::createInstance(
                     VkInstance               *instance,
    [[maybe_unused]] DebugExtension           *debugExtension,
                     VkDebugUtilsMessengerEXT *debugMessenger) noexcept
{
    bool success = false;
    auto &c = g::config;
    uint32_t count = 0;
    VkResult result;

    ArrayList<VkLayerProperties> layerProperties;
    result = vkEnumerateInstanceLayerProperties(&count, {});
    if (result != VK_SUCCESS)
    {
        std::cerr
            << "Failed to retrieve Vulkan instance layer properties: " <<
            ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }
    PL_TRY_DISCARD(layerProperties.resize(count));
    result = vkEnumerateInstanceLayerProperties(&count, layerProperties.data());
    if (result != VK_SUCCESS)
    {
        std::cerr
            << "Failed to retrieve Vulkan instance layer properties: " <<
            ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }

    std::clog << "Available Vulkan instance layers:\n";
    for (auto &p : layerProperties) std::clog << '\t' << p.layerName << '\n';
    std::clog << '\n';

    ArrayList<VkExtensionProperties> extensionProperties;
    result = vkEnumerateInstanceExtensionProperties({}, &count, {});
    if (result != VK_SUCCESS)
    {
        std::cerr
            << "Failed to retrieve Vulkan instance extension properties: " <<
            ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }
    PL_TRY_DISCARD(extensionProperties.resize(count));
    result = vkEnumerateInstanceExtensionProperties({}, &count, extensionProperties.data());
    if (result != VK_SUCCESS)
    {
        std::cerr
            << "Failed to retrieve Vulkan instance extension properties: " <<
            ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }

    std::clog << "Available Vulkan instance extensions:\n";
    for (auto &p : extensionProperties) std::clog << '\t' << p.extensionName << '\n';
    std::clog << '\n';

    ArrayList<char const *> layers;
#ifndef NDEBUG
    auto &debugLayers = c.debug.instance.layers;
    PL_TRY_DISCARD(layers.append(debugLayers.begin(), debugLayers.end()));
#endif

    std::clog << "Required Vulkan instance layers:\n";
    for (auto &l : layers) std::clog << '\t' << l << '\n';
    std::clog << '\n';

    std::clog << "Checking for available layers...\n";
    for (auto &layer : layers)
    {
        if (std::find_if(
            layerProperties.begin(), layerProperties.end(),
            [&](auto &p) { return std::strcmp(layer, p.layerName) == 0; })
            != layerProperties.end())
        {
            std::clog << "FOUND " << layer << '\n';
        }
        else
        {
            std::cerr << "Failed to find Vulkan instance layer :" << layer << '\n';
            return {tags::error, getSingleton<VulkanError>()};
        }
    }

    ArrayList<char const *> extensions;
#ifndef NDEBUG
    auto &debugExtensions = c.debug.instance.extensions;
    PL_TRY_DISCARD(extensions.append(debugExtensions.begin(), debugExtensions.end()));
#endif
    char const **glfwExtensions = glfwGetRequiredInstanceExtensions(&count);
    PL_TRY_DISCARD(extensions.append(glfwExtensions, glfwExtensions + count));

    std::clog << "Required Vulkan instance extensions:\n";
    for (auto &e : extensions) std::clog << '\t' << e << '\n';
    std::clog << '\n';

    std::clog << "Checking for available extensions...\n";
    for (auto &ext : extensions)
    {
        if (std::find_if(
            extensionProperties.begin(), extensionProperties.end(),
            [&](auto &p) { return std::strcmp(ext, p.extensionName) == 0; })
            != extensionProperties.end())
        {
            std::clog << "FOUND " << ext << '\n';
        }
        else
        {
            std::cerr << "Failed to find Vulkan instance extension: " << ext << '\n';
            return {tags::error, getSingleton<VulkanError>()};
        }

    }

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = {},
        .pApplicationName = c.applicationName,
        .applicationVersion = c.applicationVersion,
        .pEngineName = c.engineName,
        .engineVersion = c.engineVersion,
        .apiVersion = c.apiVersion,
    };

#ifndef NDEBUG
    VkDebugUtilsMessengerCreateInfoEXT debugInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = {},
        .flags = {},
        .messageSeverity = c.debug.messageSeverity,
        .messageType = c.debug.messageType,
        .pfnUserCallback = debugCallback,
        .pUserData = {},
    };
#endif

    VkInstanceCreateInfo instanceInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifdef NDEBUG
        .pNext = {},
#else
        .pNext = &debugInfo,
#endif
        .flags = {},
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = (uint32_t) layers.size(),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = (uint32_t) extensions.size(),
        .ppEnabledExtensionNames = extensions.data(),
    };

    result = vkCreateInstance(&instanceInfo, {}, instance);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan instance: " << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }
    PL_DEFER(if (!success) vkDestroyInstance(*instance, {}));

#ifdef NDEBUG
    *debugMessenger = VK_NULL_HANDLE;
#else
    PL_TRY_DISCARD(debugExtension->load(*instance));
    result = debugExtension->vkCreateDebugUtilsMessengerEXT(*instance, &debugInfo, {}, debugMessenger);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan debug messenger: " << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }
    PL_DEFER(if (!success)
        debugExtension->vkDestroyDebugUtilsMessengerEXT(*instance, *debugMessenger, {}));
#endif
    success = true;
    return {};
}


RE<VkSurfaceKHR, SimpleError> Renderer::createSurface(VkInstance instance, GLFWwindow *window) noexcept
{
    VkSurfaceKHR surface;
    VkResult result = glfwCreateWindowSurface(instance, window, {}, &surface);
    if (result != VK_SUCCESS)
    {
        std::cerr
            << "Failed to create Vulkan window surface for GLFW window: "
            << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }
    return surface;
}


RE<void, SimpleError> Renderer::createDevice(
    GLFWwindow             *window,
    VkInstance              instance,
    VkSurfaceKHR            surface,
    VkPhysicalDevice       *physicalDevice,
    DeviceInfo             *deviceInfo,
    SurfaceInfo            *surfaceInfo,
    SwapchainConfiguration *swapchainConfig,
    VkDevice               *device,
    VkQueue                *graphicsQueue,
    VkQueue                *presentQueue) noexcept
{
    bool success = false;

    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, {});
    ArrayList<VkPhysicalDevice> devices;
    PL_TRY_DISCARD(devices.resize(count));
    vkEnumeratePhysicalDevices(instance, &count, devices.data());

    bool found = false;
    for (VkPhysicalDevice candidate : devices)
    {
        PL_TRY_DISCARD(deviceInfo->query(candidate, surface));
        PL_TRY_DISCARD(surfaceInfo->query(candidate, surface));
        if (deviceInfo->isSuitable() && surfaceInfo->isSuitable())
        {
            *physicalDevice = candidate;
            swapchainConfig->query(*surfaceInfo, window);
            found = true;
            break;
        }
    }
    if (!found)
    {
        std::cerr << "Cannot find a suitable Vulkan GPU\n";
        return {tags::error, getSingleton<VulkanError>()};
    }

    float queuePriority = 1.f;
    const auto createQueueInfo = [&](uint32_t familyIndex)
    {
        return VkDeviceQueueCreateInfo
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = {},
            .flags = {},
            .queueFamilyIndex = familyIndex,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        };
    };
    ArrayList<VkDeviceQueueCreateInfo> queueInfos;
    auto &queueIndices = deviceInfo->queues.indices;
    PL_TRY_DISCARD(queueInfos.push_back(createQueueInfo(queueIndices.graphicsFamily)));
    if (queueIndices.graphicsFamily != queueIndices.presentFamily)
    {
        PL_TRY_DISCARD(queueInfos.push_back(createQueueInfo(queueIndices.presentFamily)));
    }

    VkPhysicalDeviceFeatures features{};
    deviceInfo->features = features;

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = {},
        .flags = {},
        .queueCreateInfoCount = (uint32_t) queueInfos.size(),
        .pQueueCreateInfos = queueInfos.data(),
#ifdef NDEBUG
        .enabledLayerCount = {},
        .ppEnabledLayerNames = {},
#else
        .enabledLayerCount = (uint32_t) g::config.debug.device.layers.size(),
        .ppEnabledLayerNames = g::config.debug.device.layers.data(),
#endif
        .enabledExtensionCount = (uint32_t) g::config.device.extensions.size(),
        .ppEnabledExtensionNames = g::config.device.extensions.data(),
        .pEnabledFeatures = &deviceInfo->features,
    };

    VkResult result = vkCreateDevice(*physicalDevice, &deviceCreateInfo, {}, device);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan device: " << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }
    PL_DEFER(if (!success) vkDestroyDevice(*device, {}));

    vkGetDeviceQueue(*device, queueIndices.graphicsFamily, 0, graphicsQueue);
    vkGetDeviceQueue(*device, queueIndices.presentFamily, 0, presentQueue);

    success = true;
    return {};
}


RE<void, SimpleError> Renderer::createCommandPool(
    DeviceInfo const           &deviceInfo,
    VkDevice                    device,
    VkCommandPool              *commandPool,
    ArrayList<VkCommandBuffer> *commandBuffers) noexcept
{
    bool success = false;
    VkResult result;
    VkCommandPoolCreateInfo commandPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = {},
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = deviceInfo.queues.indices.graphicsFamily,
    };

    result = vkCreateCommandPool(device, &commandPoolInfo, {}, commandPool);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan command pool: " << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }
    PL_DEFER(if (!success) vkDestroyCommandPool(device, *commandPool, {}));

    PL_TRY_DISCARD(commandBuffers->resize(g::config.maxFramesInFlight));
    VkCommandBufferAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = *commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = (uint32_t) commandBuffers->size(),
    };

    result = vkAllocateCommandBuffers(device, &allocInfo, commandBuffers->data());
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to allocate Vulkan command buffer: " << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }

    success = true;
    return {};
}


RE<void, SimpleError> Renderer::createSwapchain(
    VkSurfaceKHR                  surface,
    DeviceInfo const             &deviceInfo,
    SurfaceInfo const            &surfaceInfo,
    SwapchainConfiguration const &config,
    VkDevice                      device,
    VkSwapchainKHR               *swapchain,
    ArrayList<VkImage>           *swapchainImages,
    ArrayList<VkImageView>       *swapchainImageViews) noexcept
{
    bool success = false;

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = {},
        .flags = {},
        .surface = surface,
        .minImageCount = config.imageCount,
        .imageFormat = config.surfaceFormat.format,
        .imageColorSpace = config.surfaceFormat.colorSpace,
        .imageExtent = config.extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    };

    auto &indices = deviceInfo.queues.indices;
    uint32_t queueFamilies[] = {indices.graphicsFamily, indices.presentFamily};
    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilies;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        createInfo.queueFamilyIndexCount = {};
        createInfo.pQueueFamilyIndices = {};
    }

    createInfo.preTransform = surfaceInfo.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = config.presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(device, &createInfo, {}, swapchain);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan swapchain: " << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }
    PL_DEFER(if (!success) vkDestroySwapchainKHR(device, *swapchain, {}));

    uint32_t count;
    result = vkGetSwapchainImagesKHR(device, *swapchain, &count, {});
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to retrieve Vulkan swapchain images: " << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }
    PL_TRY_DISCARD(swapchainImages->resize(count));
    result = vkGetSwapchainImagesKHR(device, *swapchain, &count, swapchainImages->data());
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to retrieve Vulkan swapchain images: " << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }

    PL_TRY_DISCARD(swapchainImageViews->resize(swapchainImages->size()));
    unsigned numImageViewsCreated = 0;
    PL_DEFER(
    if (!success)
    {
        for (unsigned i = 0; i < numImageViewsCreated; ++i)
            vkDestroyImageView(device, (*swapchainImageViews)[i], {});
        swapchainImageViews->clear();
    });
    VkImageViewCreateInfo imageViewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = {},
        .flags = {},
        .image = {},
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = config.surfaceFormat.format,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
        },
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    for (unsigned i = 0; i < swapchainImages->size(); ++i)
    {
        imageViewInfo.image = (*swapchainImages)[i];
        result = vkCreateImageView(device, &imageViewInfo, {}, swapchainImageViews->data() + i);
        if (result != VK_SUCCESS)
        {
            std::cerr
                << "Failed to create Vulkan swapchain image view: "
                << ::string_VkResult(result) << '\n';
            return {tags::error, getSingleton<VulkanError>()};
        }
        ++numImageViewsCreated;
    }

    success = true;

    return {};
}

RE<VkShaderModule, SimpleError> Renderer::createShaderModule(
    VkDevice              device,
    Span<std::byte const> byteCode) noexcept
{
    bool success = false;

    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = {},
        .flags = {},
        .codeSize = byteCode.size(),
        .pCode = reinterpret_cast<uint32_t const *>(byteCode.data()),
    };

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(device, &createInfo, {}, &shaderModule);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan shader module: " << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }
    PL_DEFER(if (!success) vkDestroyShaderModule(device, shaderModule, {}));


    success = true;
    return shaderModule;
}

RE<void, SimpleError> Renderer::createGraphicsPipeline(
    VkDevice                      device,
    SwapchainConfiguration const &swapchainConfig,
    Span<VkImageView const>         swapchainImageViews,
    VkRenderPass                 *renderPass,
    ArrayList<VkFramebuffer>     *swapchainFramebuffers,
    VkPipelineLayout             *pipelineLayout,
    VkPipeline                   *pipeline) noexcept
{
    bool success = false;
    VkResult result;

    VkAttachmentDescription attachment = {
        .format = swapchainConfig.surfaceFormat.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference attachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentRef,
    };

    VkSubpassDependency subpassDependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = {},
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = {},
        .flags = {},
        .attachmentCount = 1,
        .pAttachments = &attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &subpassDependency,
    };

    result = vkCreateRenderPass(device, &renderPassInfo, {}, renderPass);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan render pass: " << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }
    PL_DEFER(if (!success) vkDestroyRenderPass(device, *renderPass, {}));


    VkFramebufferCreateInfo framebufferInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = {},
        .flags = {},
        .renderPass = *renderPass,
        .attachmentCount = 1,
        .pAttachments = {},
        .width = swapchainConfig.extent.width,
        .height = swapchainConfig.extent.height,
        .layers = 1,
    };

    PL_TRY_DISCARD(swapchainFramebuffers->resize(swapchainImageViews.size()));
    unsigned numSwapchainFramebuffersCreated = 0;
    PL_DEFER(
    if (!success)
    {
        for (unsigned i = 0; i < numSwapchainFramebuffersCreated; ++i)
             vkDestroyFramebuffer(device, (*swapchainFramebuffers)[i], {});
        swapchainFramebuffers->clear();
    });

    for (unsigned i = 0; i < swapchainFramebuffers->size(); ++i)
    {
        framebufferInfo.pAttachments = &swapchainImageViews[i];
        result = vkCreateFramebuffer(device, &framebufferInfo, {}, swapchainFramebuffers->data() + i);
        if (result != VK_SUCCESS)
        {
            std::cerr
                << "Failed to create Vulkan swapchain framebuffer: "
                << ::string_VkResult(result) << '\n';
            return {tags::error, getSingleton<VulkanError>()};
        }
        ++numSwapchainFramebuffersCreated;
    }


    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = {},
        .flags = {},
        .setLayoutCount = {},
        .pSetLayouts = {},
        .pushConstantRangeCount = {},
        .pPushConstantRanges = {},
    };

    result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, {}, pipelineLayout);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan pipeline layout: " << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }
    PL_DEFER(if (!success) vkDestroyPipelineLayout(device, *pipelineLayout, {}));

    PL_TRY_ASSIGN(auto vertShaderCode, loadFile(PL_RESOURCE_DIR "/shaders/shader.vert.spv"));
    PL_TRY_ASSIGN(auto fragShaderCode, loadFile(PL_RESOURCE_DIR "/shaders/shader.frag.spv"));

    PL_TRY_ASSIGN(VkShaderModule vertShaderModule, createShaderModule(device, vertShaderCode));
    PL_DEFER(vkDestroyShaderModule(device, vertShaderModule, {}));

    PL_TRY_ASSIGN(VkShaderModule fragShaderModule, createShaderModule(device, fragShaderCode));
    PL_DEFER(vkDestroyShaderModule(device, fragShaderModule, {}));

    VkPipelineShaderStageCreateInfo shaderStageInfos[] = {
    {
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext               = {},
        .flags               = {},
        .stage               = VK_SHADER_STAGE_VERTEX_BIT,
        .module              = vertShaderModule,
        .pName               = "main",
        .pSpecializationInfo = {},
    },
    {
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext               = {},
        .flags               = {},
        .stage               = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module              = fragShaderModule,
        .pName               = "main",
        .pSpecializationInfo = {},
    },
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = {},
        .flags = {},
        .vertexBindingDescriptionCount = {},
        .pVertexBindingDescriptions = {},
        .vertexAttributeDescriptionCount = {},
        .pVertexAttributeDescriptions = {},
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembleInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = {},
        .flags = {},
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkPipelineViewportStateCreateInfo viewportInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = {},
        .flags = {},
        .viewportCount = 1,
        .pViewports = {},
        .scissorCount = 1,
        .pScissors = {},
    };

    VkPipelineRasterizationStateCreateInfo rasterizationInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = {},
        .flags = {},
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = {},
        .depthBiasClamp = {},
        .depthBiasSlopeFactor = {},
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampleInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = {},
        .flags = {},
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = {},
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                        | VK_COLOR_COMPONENT_G_BIT
                        | VK_COLOR_COMPONENT_B_BIT
                        | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo colorBlendInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = {},
        .flags = {},
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},
    };

    Array dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = {},
        .flags = {},
        .dynamicStateCount = (uint32_t) dynamicStates.size(),
        .pDynamicStates = dynamicStates.data(),
    };

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = {},
        .flags = {},
        .stageCount = 2,
        .pStages = shaderStageInfos,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembleInfo,
        .pViewportState = &viewportInfo,
        .pRasterizationState = &rasterizationInfo,
        .pMultisampleState = &multisampleInfo,
        .pDepthStencilState = {},
        .pColorBlendState = &colorBlendInfo,
        .pDynamicState = &dynamicInfo,
        .layout = *pipelineLayout,
        .renderPass = *renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, {}, pipeline);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan graphics pipeline: " << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }
    PL_DEFER(if (!success) vkDestroyPipeline(device, *pipeline, {}));

    success = true;

    return {};
}


RE<void, SimpleError> Renderer::createSynchronizationObjects(
    VkDevice     device,
    ArrayList<VkSemaphore> *imageAvailableSemaphores,
    ArrayList<VkSemaphore> *renderFinishedSemaphores,
    ArrayList<VkFence>     *inFlightFences) noexcept
{
    bool success = false;
    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = {},
        .flags = {},
    };

    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = {},
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    VkResult result;

    PL_TRY_DISCARD(imageAvailableSemaphores->resize(g::config.maxFramesInFlight));
    PL_TRY_DISCARD(renderFinishedSemaphores->resize(g::config.maxFramesInFlight));
    PL_TRY_DISCARD(inFlightFences          ->resize(g::config.maxFramesInFlight));

    uint32_t numImageAvailableSemaphoresCreated = 0;
    uint32_t numRenderFinishedSemaphoresCreated = 0;
    uint32_t numInFlightFencesCreated           = 0;

    PL_DEFER(
    if (!success)
    {
        for (uint32_t i = 0; i < numImageAvailableSemaphoresCreated; ++i)
            vkDestroySemaphore(device, (*imageAvailableSemaphores)[i], {});
        imageAvailableSemaphores->clear();

        for (uint32_t i = 0; i < numRenderFinishedSemaphoresCreated; ++i)
            vkDestroySemaphore(device, (*renderFinishedSemaphores)[i], {});
        renderFinishedSemaphores->clear();

        for (uint32_t i = 0; i < numInFlightFencesCreated; ++i)
            vkDestroyFence(device, (*inFlightFences)[i], {});
        inFlightFences->clear();
    });

    for (VkSemaphore &s : *imageAvailableSemaphores)
    {
        result = vkCreateSemaphore(device, &semaphoreInfo, {}, &s);
        if (result != VK_SUCCESS)
        {
            std::cerr << "Failed to create Vulkan semaphore: " << ::string_VkResult(result) << '\n';
            return {tags::error, getSingleton<VulkanError>()};
        }
        ++numImageAvailableSemaphoresCreated;
    }

    for (VkSemaphore &s : *renderFinishedSemaphores)
    {
        result = vkCreateSemaphore(device, &semaphoreInfo, {}, &s);
        if (result != VK_SUCCESS)
        {
            std::cerr << "Failed to create Vulkan semaphore: " << ::string_VkResult(result) << '\n';
            return {tags::error, getSingleton<VulkanError>()};
        }
        ++numRenderFinishedSemaphoresCreated;
    }

    for (VkFence &f : *inFlightFences)
    {
        result = vkCreateFence(device, &fenceInfo, {}, &f);
        if (result != VK_SUCCESS)
        {
            std::cerr << "Failed to create Vulkan fence: " << ::string_VkResult(result) << '\n';
            return {tags::error, getSingleton<VulkanError>()};
        }
        ++numInFlightFencesCreated;
    }

    success = true;
    return {};
}


RE<void, SimpleError> Renderer::recordCommandBuffer(
    VkCommandBuffer commandBuffer,
    uint32_t        imageIndex) noexcept
{
    VkResult result;
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = {},
        .flags = {},
        .pInheritanceInfo = {},
    };

    result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to begin Vulkan command buffer: " << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }

    VkClearValue clearColor = {.color{.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}};
    VkRenderPassBeginInfo renderPassInfo = {
        .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext       = {},
        .renderPass  = _renderPass,
        .framebuffer = _swapchainFramebuffers[imageIndex],
        .renderArea  = {
            .offset  = {0, 0},
            .extent  = _swapchainConfig.extent,
        },
        .clearValueCount = 1,
        .pClearValues = &clearColor,
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float) _swapchainConfig.extent.width,
        .height = (float) _swapchainConfig.extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = _swapchainConfig.extent,
    };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    result = vkEndCommandBuffer(commandBuffer);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to record Vulkan command buffer: " << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }

    return {};
}


RE<void, SimpleError> Renderer::drawFrame() noexcept
{
    VkResult result;
    result = vkWaitForFences(_device, 1, &_inFlightFences[_currentFrame], VK_FALSE, UINT64_MAX);
    if (result != VK_SUCCESS && result != VK_TIMEOUT)
    {
        std::cerr << "Failed to wait for Vulkan fence: " << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }

    result = vkResetFences(_device, 1, &_inFlightFences[_currentFrame]);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to wait for Vulkan fence: " << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }

    uint32_t imageIndex;
    result = vkAcquireNextImageKHR(
            _device,
            _swapchain,
            UINT64_MAX,
            _imageAvailableSemaphores[_currentFrame],
            VK_NULL_HANDLE,
            &imageIndex);

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        std::cerr
            << "Failed to acquire next image from Vulkan swapchain: "
            << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }

    result = vkResetCommandBuffer(_commandBuffers[_currentFrame], {});
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to wait for Vulkan fence: " << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }
    PL_TRY_DISCARD(recordCommandBuffer(_commandBuffers[_currentFrame], imageIndex));

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo   = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext                = {},
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &_imageAvailableSemaphores[_currentFrame],
        .pWaitDstStageMask    = waitStages,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &_commandBuffers[_currentFrame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &_renderFinishedSemaphores[_currentFrame],
    };
    result = vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _inFlightFences[_currentFrame]);
    if (result != VK_SUCCESS)
    {
        std::cerr
            << "Failed to submit Vulkan command buffer to graphics queue: "
            << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = {},
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &_renderFinishedSemaphores[_currentFrame],
        .swapchainCount = 1,
        .pSwapchains = &_swapchain,
        .pImageIndices = &imageIndex,
        .pResults = {},
    };
    result = vkQueuePresentKHR(_presentQueue, &presentInfo);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        std::cerr << "Failed to present Vulkan swapchain image: " << ::string_VkResult(result) << '\n';
        return {tags::error, getSingleton<VulkanError>()};
    }

    _currentFrame = (_currentFrame + 1) % g::config.maxFramesInFlight;

    return {};
}


RE<void, SimpleError> Renderer::regenerateSwapchain() noexcept
{
    vkDeviceWaitIdle(_device);

    return {};
}
} // namespace pl::vulkan
