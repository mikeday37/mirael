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

#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include "imgui.h"
#include "imgui_internal.h"

#include "Diagnostics.h"
#include "Library.h"
#include "NodeTypeRegistry.h"
#include "Project.h"
#include "ProjectExplorer.h"
#include "Properties.h"
#include "Settings.h"

namespace Mirael
{

class App
{
public:
    App();

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
    // UI
    //
public:
    static App &get() { return *appInstance_; }
    static App *fromWindow(GLFWwindow *window) { return static_cast<App *>(glfwGetWindowUserPointer(window)); }

    void attemptReloadLastProject();
    Project &getProject() { return projectExplorer_.getProject(); }
    Library &getLibrary() { return library_; }
    void exit();
    bool *getLibraryFlag() { return &mainWindowSettings_.library; }
    bool *getPropertiesFlag() { return &mainWindowSettings_.properties; }
    bool *getSettingsFlag() { return &mainWindowSettings_.settings; }
    bool *getDiagnosticsFlag() { return &mainWindowSettings_.diagnostics; }
    bool *getFullscreenFlag() { return &mainWindowSettings_.fullscreen; }
    bool *getImGuiDemoFlag() { return &mainWindowSettings_.demo; }
    void applyFullscreenSetting();

    void setDestructiveAction(std::string label, std::string message, std::function<void()> postConfirmAction,
                              std::function<void()> postCancelAction = nullptr);
    void showError(std::string message);
    ImGuiID getDockspaceId() const { return dockspaceId_; }
    const NodeTypeRegistry &nodeTypes() const { return nodeTypeRegistry_; }

    std::string getNewUuidAsString() const;

    struct Metrics {
        uint64_t mainLoopIteration, swapChainBuildCount, platformWindowCreateCount, platformWindowDestroyCount, windowRefreshCount;
    };
    Metrics metrics_{};
    void showDiagnosticRows();

    struct ChangeTrackingSettings {
        bool panZoom = false, moveNode = true, graphVisibility = false;
    };
    ChangeTrackingSettings &getChangeTrackingSettings() { return changeTrackingSettings_; }

    struct Style {
        struct Values {
            float pinIconSize = 20.0f;
        };
        struct Colors {
            ImVec4 nodeHeaderFill = ImColor(66, 230, 218, 127);
            ImVec4 pinIconColor   = ImColor(134, 134, 134, 255);
        };
        Values values{};
        Colors colors{};
    };
    Style &getStyle() { return style_; }

private:
    static inline App *appInstance_ = nullptr;
    void showImGui();
    ProjectExplorer projectExplorer_;
    Library library_;
    Properties properties_;
    Settings settings_;
    Diagnostics diagnostics_;
    ImGuiID dockspaceId_{};
    Style style_{};

    // interaction
    bool closeRequested_ = false;
    bool closeConfirmed_ = false;
    ChangeTrackingSettings changeTrackingSettings_{};

    // registries
    NodeTypeRegistry nodeTypeRegistry_;

    // desctructive action (for requiring user confirmation)
    struct DestructiveAction {
        std::string modalTitle;
        std::string modalMessage;
        ImVec2 modalCenter;
        std::function<void()> postConfirmAction;
        std::function<void()> postCancelAction = nullptr;
        bool opened                            = false;
    };
    std::optional<DestructiveAction> destructiveAction_;

    //
    // main window settings persistence
    //
    struct MainWindowSettings {
        std::optional<int> x, y, width, height; // screen coordinates
        bool maximized, fullscreen, library, properties, settings, diagnostics, demo, firstRun;
        std::optional<std::filesystem::path> lastProjectPath{};
        std::optional<GraphId> lastFocusedGraphId{};
    };
    MainWindowSettings mainWindowSettings_ = {.maximized   = false,
                                              .fullscreen  = false,
                                              .library     = true,
                                              .properties  = true,
                                              .settings    = false,
                                              .diagnostics = false,
                                              .demo        = false,
                                              .firstRun    = true};
    ImGuiSettingsHandler getImGuiSettingsHandler();
    static void imGuiSettings_WriteAll(ImGuiContext *ctx, ImGuiSettingsHandler *handler, ImGuiTextBuffer *out_buf);
    static void *imGuiSettings_ReadOpen(ImGuiContext *ctx, ImGuiSettingsHandler *handler, const char *name);
    static void imGuiSettings_ReadLine(ImGuiContext *ctx, ImGuiSettingsHandler *handler, void *entry, const char *line);

    // main window handling
    static void imGuiPlatform_CreateWindow(ImGuiViewport *vp);
    static void imGuiPlatform_DestroyWindow(ImGuiViewport *vp);
    GLFWmonitor *getCurrentMonitor();
    bool togglingFullscreen_       = false;
    bool initialShowWindowPending_ = true;
    struct BaseImGuiPlatformHandlers {
        void (*Platform_CreateWindow)(ImGuiViewport *vp)  = nullptr;
        void (*Platform_DestroyWindow)(ImGuiViewport *vp) = nullptr;
    };
    BaseImGuiPlatformHandlers baseImGuiPlatformHandlers_{};

    //
    // GLFW setup, management and hooks
    //
    GLFWwindow *window_      = nullptr;
    bool frameBufferResized_ = false;
    static void frameBufferResizeCallback(GLFWwindow *window, int width, int height);
    static void windowPosCallback(GLFWwindow *window, int x, int y);
    static void windowSizeCallback(GLFWwindow *window, int width, int height);
    static void windowMaximizeCallback(GLFWwindow *window, int maximized);
    static void windowCloseCallback(GLFWwindow *window);
    // super state = iconified, maximized, or full screen, where pos/size should be ignored
    static bool isWindowInSuperState(GLFWwindow *window);

    //
    // Vulkan objects and variables
    //
    vk::raii::Context context_{};
    vk::raii::Instance instance_                     = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger_ = nullptr;
    vk::raii::SurfaceKHR surface_                    = nullptr;
    vk::raii::PhysicalDevice physicalDevice_         = nullptr;
    vk::raii::Device device_                         = nullptr;
    vk::raii::Queue graphicsQueue_                   = nullptr;
    uint32_t graphicsQueueIndex_                     = ~0;
    vk::raii::SwapchainKHR swapchain_                = nullptr;
    vk::raii::PipelineLayout pipelineLayout_         = nullptr;
    vk::raii::Pipeline graphicsPipeline_             = nullptr;
    vk::raii::CommandPool commandPool_               = nullptr;

    uint32_t minImageCount_ = 0;
    vk::SurfaceFormatKHR swapchainSurfaceFormat_;
    vk::Extent2D swapchainExtent_;
    std::vector<vk::Image> swapchainImages_;
    std::vector<vk::raii::ImageView> swapchainImageViews_;

    std::vector<vk::raii::CommandBuffer> commandBuffers_;
    std::vector<vk::raii::Semaphore> presentCompleteSemaphores_;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores_;
    std::vector<vk::raii::Fence> inFlightFences_;
    uint32_t frameIndex_ = 0;

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

} // namespace Mirael
