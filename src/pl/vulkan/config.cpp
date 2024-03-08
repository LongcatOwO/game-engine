module;
#include <vulkan/vulkan.h>

module pl.vulkan;

import pl.core;

namespace pl::vulkan
{
namespace
{
constexpr auto debugInstanceLayers = makeArray<char const *>(
{
    "VK_LAYER_KHRONOS_validation",
});
constexpr auto debugInstanceExtensions = makeArray<char const *>(
{
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
});
constexpr auto debugDeviceLayers = debugInstanceLayers;
constexpr auto deviceExtensions = makeArray<char const *>(
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
});
} // namespace

Config const g::config   = {
    .window              = {
        .width           = 800,
        .height          = 600,
        .title           = "Vulkan Window",
    },
    .applicationName     = "Vulkan Application",
    .applicationVersion  = VK_MAKE_API_VERSION(0, 1, 0, 0),
    .engineName          = "No Engine",
    .engineVersion       = VK_MAKE_API_VERSION(0, 1, 0, 0),
    .apiVersion          = VK_MAKE_API_VERSION(0, 1, 3, 0),
    .maxFramesInFlight   = 2,
    .debug = {
#ifdef NDEBUG
        .enabled         = false,
#else
        .enabled         = true,
#endif
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
        .instance        = {
            .layers      = debugInstanceLayers,
            .extensions  = debugInstanceExtensions,
        },
        .device          = {
            .layers      = debugDeviceLayers,
        },
    },

    .instance = {},

    .device              = {
        .extensions      = deviceExtensions,
    },
};
} // namespace pl::vulkan
