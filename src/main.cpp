module;
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <pl/macro.hpp>

module main;

import pl.core;
import pl.vulkan;

using namespace ::pl;
namespace plvk = ::pl::vulkan;

static RE<void, SimpleError> real_main([[maybe_unused]] Span<char *const> argv)
{
    if (glfwInit() != GLFW_TRUE)
    {
        std::cerr << "Failed to initialize GLFW\n";
        return {tags::error, getSingleton<plvk::GLFWError>()};
    }
    PL_DEFER(glfwTerminate());

    plvk::Renderer renderer;
    PL_TRY_DISCARD(renderer.init());
    PL_DEFER(renderer.deinit());
    PL_TRY_DISCARD(renderer.run());

    return {};
}

int main(int argc, char *argv [])
{
    auto result = real_main({argv, (std::size_t) argc});

    if (!result)
    {
        std::cerr << "Caught Error: " << result.error().errorType().name() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
