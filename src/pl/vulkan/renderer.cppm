module;
#include <cstddef>
#include <fstream>
#include <iostream>
#include <limits>
#include <utility>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <pl/macro.hpp>

export module pl.vulkan:renderer;

import pl.core;

import :error;

export namespace pl::vulkan
{
struct DebugExtension
{
#define PL_VULKAN_DECL_PFN(name) PFN_##name name;
    PL_VULKAN_DECL_PFN(vkCreateDebugUtilsMessengerEXT)
    PL_VULKAN_DECL_PFN(vkDestroyDebugUtilsMessengerEXT)

    RE<void, SimpleError> load(VkInstance instance) noexcept
    {
#define PL_VULKAN_LOAD_PFN(name) \
        if (!(name = PFN_##name(vkGetInstanceProcAddr(instance, #name)))) \
        { \
            std::cerr << "Failed to load DebugExtension PFN: " << #name << '\n'; \
            return {tags::error, getSingleton<VulkanError>()}; \
        }
        PL_VULKAN_LOAD_PFN(vkCreateDebugUtilsMessengerEXT)
        PL_VULKAN_LOAD_PFN(vkDestroyDebugUtilsMessengerEXT)
        return {};
    }
};

struct QueueInfo
{
    static constexpr auto nullIndex = std::numeric_limits<uint32_t>::max();
    struct
    {
        uint32_t graphicsFamily;
        uint32_t presentFamily;
    } indices;

    bool isComplete() const noexcept
    {
        return indices.graphicsFamily != nullIndex
            && indices.presentFamily != nullIndex;
    }

    RE<void, SimpleError> query(
        VkPhysicalDevice device,
        VkSurfaceKHR surface,
        Span<VkQueueFamilyProperties const> families) noexcept;
};

struct SurfaceInfo
{
    VkSurfaceCapabilitiesKHR      capabilities;
    ArrayList<VkSurfaceFormatKHR> formats;
    ArrayList<VkPresentModeKHR>   presentModes;

    RE<void, SimpleError> query(VkPhysicalDevice device, VkSurfaceKHR surface) noexcept;

    bool isSuitable() const noexcept
    {
        return !formats.empty() && !presentModes.empty();
    }

    VkSurfaceFormatKHR getPreferredFormat() const noexcept;
    VkPresentModeKHR getPreferredPresentMode() const noexcept;
    VkExtent2D getPreferredExtent(GLFWwindow *window) const noexcept;
    uint32_t getPreferredImageCount() const noexcept;
};

struct SwapchainConfiguration
{
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    VkExtent2D extent;
    uint32_t imageCount;

    void query(SurfaceInfo const &surfaceInfo, GLFWwindow *window) noexcept
    {
        surfaceFormat = surfaceInfo.getPreferredFormat();
        presentMode   = surfaceInfo.getPreferredPresentMode();
        extent        = surfaceInfo.getPreferredExtent(window);
        imageCount    = surfaceInfo.getPreferredImageCount();
    }
};

struct DeviceInfo
{
    VkPhysicalDeviceProperties         properties;
    ArrayList<VkExtensionProperties>   extensions;
    VkPhysicalDeviceFeatures           features;
    ArrayList<VkQueueFamilyProperties> queueFamiliesProperties;
    QueueInfo                          queues;

    RE<void, SimpleError> query(VkPhysicalDevice device, VkSurfaceKHR surface) noexcept;

    bool isSuitable() const noexcept;
};

RE<ArrayList<std::byte>, SimpleError> loadFile(char const *filename) noexcept;

class Renderer
{
public:
    Renderer() = default;

    Renderer           (Renderer const &) = delete;
    Renderer &operator=(Renderer const &) = delete;

    RE<void, SimpleError> init() noexcept;
    void deinit() noexcept;

    RE<void, SimpleError> run() noexcept;

private:
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        VkDebugUtilsMessengerCallbackDataEXT const *callbackData,
        void *userData) noexcept;

    static RE<GLFWwindow *, SimpleError> createWindow() noexcept;

    static RE<void, SimpleError> createInstance(
        VkInstance               *instance,
        DebugExtension           *debugExtension,
        VkDebugUtilsMessengerEXT *debugMessenger) noexcept;

    static RE<VkSurfaceKHR, SimpleError> createSurface(
        VkInstance instance,
        GLFWwindow *window) noexcept;

    static RE<void, SimpleError> createDevice(
        GLFWwindow             *window,
        VkInstance              instance,
        VkSurfaceKHR            surface,
        VkPhysicalDevice       *physicalDevice,
        DeviceInfo             *deviceInfo,
        SurfaceInfo            *surfaceInfo,
        SwapchainConfiguration *swapchainConfig,
        VkDevice               *device,
        VkQueue                *graphicsQueue,
        VkQueue                *presentQueue) noexcept;

    static RE<void, SimpleError> createCommandPool(
        DeviceInfo const           &deviceInfo,
        VkDevice                    device,
        VkCommandPool              *commandPool,
        ArrayList<VkCommandBuffer> *commandBuffers) noexcept;

    static RE<void, SimpleError> createSwapchain(
        VkSurfaceKHR                  surface,
        DeviceInfo const             &deviceInfo,
        SurfaceInfo const            &surfaceInfo,
        SwapchainConfiguration const &config,
        VkDevice                      device,
        VkSwapchainKHR               *swapchain,
        ArrayList<VkImage>           *swapchainImages,
        ArrayList<VkImageView>       *swapchainImageViews) noexcept;

    static RE<VkShaderModule, SimpleError> createShaderModule(
        VkDevice              device,
        Span<std::byte const> byteCode) noexcept;

    static RE<void, SimpleError> createGraphicsPipeline(
        VkDevice                      device,
        SwapchainConfiguration const &swapchainConfig,
        Span<VkImageView const>       swapchainImageViews,
        VkRenderPass                 *renderPass,
        ArrayList<VkFramebuffer>     *swapchainFramebuffers,
        VkPipelineLayout             *pipelineLayout,
        VkPipeline                   *pipeline) noexcept;

    static RE<void, SimpleError> createSynchronizationObjects(
        VkDevice     device,
        ArrayList<VkSemaphore>       *imageAvailableSemaphore,
        ArrayList<VkSemaphore>       *renderFinishedSemaphore,
        ArrayList<VkFence>           *inFlightFence) noexcept;

    RE<void, SimpleError> recordCommandBuffer(
        VkCommandBuffer commandBuffer,
        uint32_t imageIndex) noexcept;

    RE<void, SimpleError> drawFrame() noexcept;

    RE<void, SimpleError> regenerateSwapchain() noexcept;

    GLFWwindow                *_window                   = {};
    VkInstance                 _instance                 = {};
    DebugExtension             _debugExtension;
    VkDebugUtilsMessengerEXT   _debugMessenger           = {};
    VkSurfaceKHR               _surface                  = {};
    VkPhysicalDevice           _physicalDevice           = {};
    DeviceInfo                 _deviceInfo;
    SurfaceInfo                _surfaceInfo;
    SwapchainConfiguration     _swapchainConfig;
    VkDevice                   _device                   = {};
    VkQueue                    _graphicsQueue            = {};
    VkQueue                    _presentQueue             = {};
    VkCommandPool              _commandPool              = {};
    ArrayList<VkCommandBuffer> _commandBuffers;
    VkSwapchainKHR             _swapchain                = {};
    ArrayList<VkImage>         _swapchainImages;
    ArrayList<VkImageView>     _swapchainImageViews;
    VkRenderPass               _renderPass               = {};
    ArrayList<VkFramebuffer>   _swapchainFramebuffers;   
    VkPipelineLayout           _pipelineLayout           = {};
    VkPipeline                 _pipeline                 = {};

    ArrayList<VkSemaphore>     _imageAvailableSemaphores;
    ArrayList<VkSemaphore>     _renderFinishedSemaphores;
    ArrayList<VkFence>         _inFlightFences;
    uint32_t                   _currentFrame = 0;
};
} // namespace pl::vulkan
