module;
#include <pl/macro.hpp>

export module pl.vulkan:error;

import pl.core;

export namespace pl::vulkan
{
PL_DECLARE_ERROR_TYPE(void, VulkanError, "VulkanError");
PL_DECLARE_ERROR_TYPE(void, GLFWError, "GLFWError");
PL_DECLARE_ERROR_TYPE(void, SystemError, "SystemError");
} // export namespace pl::vulkan
