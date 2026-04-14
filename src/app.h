#pragma once

#ifndef VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#error "Missing expected definition: VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS"
// this should be defined at the project level in CMakeLists.txt
#endif

#ifndef VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#endif
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <limits>
#include <vector>

#include "imgui.h"
#include "imgui_internal.h"

namespace Mirael
{

class App
{
public:
    //
    // entry point
    //
    void run();

private:
    //
    // main app lifecycle
    //
    void preInitImGui();
    void initWindow();
    void initVulkan();
    void finishInitImGui();
    void mainLoop();
    void cleanup();

    //
    // UI logic
    //
    bool showDemo = true;
    void showImGui();

    //
    // main window settings persistence
    //
    struct MainWindowSettings {
        int x, y, w, h; // screen coordinates
        bool maximized;
    };
    MainWindowSettings mainWindowSettings = {
        .x = std::numeric_limits<int>::min(), // ::min() is used to represent 'uninitialized' so that a default can be used instead
        .y = std::numeric_limits<int>::min(),
        .w = std::numeric_limits<int>::min(),
        .h = std::numeric_limits<int>::min(),
        .maximized = false};
    ImGuiSettingsHandler getImGuiSettingsHandler();
    static void imGuiSettings_WriteAll(ImGuiContext *ctx, ImGuiSettingsHandler *handler, ImGuiTextBuffer *out_buf);
    static void *imGuiSettings_ReadOpen(ImGuiContext *ctx, ImGuiSettingsHandler *handler, const char *name);
    static void imGuiSettings_ReadLine(ImGuiContext *ctx, ImGuiSettingsHandler *handler, void *entry, const char *line);

    //
    // GLFW setup, management and hooks
    //
    GLFWwindow *window      = nullptr;
    bool frameBufferResized = false;
    static void frameBufferResizeCallback(GLFWwindow *window, int width, int height);
    static void windowPosCallback(GLFWwindow *window, int x, int y);
    static void windowSizeCallback(GLFWwindow *window, int width, int height);
    static void windowMaximizeCallback(GLFWwindow *window, int maximized);
    // super state = iconified, maximized, or full screen, where pos/size should be ignored
    static bool isWindowInSuperState(GLFWwindow *window); 

    //
    // Vulkan objects and variables
    //
    vk::raii::Context context{};
    vk::raii::Instance instance                     = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
    vk::raii::SurfaceKHR surface                    = nullptr;
    vk::raii::PhysicalDevice physicalDevice         = nullptr;
    vk::raii::Device device                         = nullptr;
    vk::raii::Queue graphicsQueue                   = nullptr;
    uint32_t graphicsQueueIndex                     = ~0;
    vk::raii::SwapchainKHR swapchain                = nullptr;
    vk::raii::PipelineLayout pipelineLayout         = nullptr;
    vk::raii::Pipeline graphicsPipeline             = nullptr;
    vk::raii::CommandPool commandPool               = nullptr;

    uint32_t minImageCount = 0;
    vk::SurfaceFormatKHR swapchainSurfaceFormat;
    vk::Extent2D swapchainExtent;
    std::vector<vk::Image> swapchainImages;
    std::vector<vk::raii::ImageView> swapchainImageViews;

    std::vector<vk::raii::CommandBuffer> commandBuffers;
    std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence> inFlightFences;
    uint32_t frameIndex = 0;

    //
    // Vulkan setup, mangement, rendering
    //
    void createInstance();
    std::vector<const char *> getRequiredInstanceExtensions();
    void setupDebugMessenger();
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                          vk::DebugUtilsMessageTypeFlagsEXT type,
                                                          const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *);
    void createSurface();
    void pickPhysicalDevice();
    static bool isDeviceSuitable(const vk::raii::PhysicalDevice &physicalDevice);
    void createLogicalDevice();
    void createSwapchain();
    void cleanupSwapchain();
    void recreateSwapchain();
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const &availableFormats);
    vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const &availablePresentModes);
    vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const &capabilities);
    uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities);
    void createImageViews();
    void createGraphicsPipeline();
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char> &code) const;
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();

    void transitionImageLayout(uint32_t imageIndex, vk::ImageLayout old_layout, vk::ImageLayout new_layout,
                               vk::AccessFlags2 src_access_mask, vk::AccessFlags2 dst_access_mask,
                               vk::PipelineStageFlags2 src_stage_mask, vk::PipelineStageFlags2 dst_stage_mask);

    void recordCommandBuffer(uint32_t imageIndex);
    void drawFrame();
};

}; // namespace Mirael
