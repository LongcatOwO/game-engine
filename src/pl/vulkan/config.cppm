module;
#include <vulkan/vulkan.h>

export module pl.vulkan:config;

import pl.core;

export namespace pl::vulkan
{
struct Config
{
    struct
    {
        int         width;
        int         height;
        char const *title;
    } window;

    char const *applicationName;
    uint32_t    applicationVersion;
    char const *engineName;
    uint32_t    engineVersion;
    uint32_t    apiVersion;
    uint32_t    maxFramesInFlight;

    struct
    {
        bool enabled;

        VkDebugUtilsMessageSeverityFlagsEXT messageSeverity;
        VkDebugUtilsMessageTypeFlagsEXT     messageType;

        struct
        {
            Span<char const *const> layers;
            Span<char const *const> extensions;
        } instance;
        struct
        {
            Span<char const *const> layers;
        } device;
    } debug;

    struct
    {
    } instance;

    struct
    {
        Span<char const *const> extensions;
    } device;
};

namespace g
{
extern Config const config;
} // namespace g
} // export namespace pl::vulkan
