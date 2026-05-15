#include "pch.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <uuid.h>
#include <vector>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imgui_internal.h"

#include "App.h"
#include "Fonts.h"
#include "ImGuiEx.h"
#include "NfdShim.h"
#include "ProjectExplorer.h"
#include "util.h"

#ifdef WIN32
#include "os_win32.h"
#endif

#ifndef IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING
#error "Dynamic rendering not supported."
#endif

namespace Mirael
{

// #define SHOW_DEBUG_TRIANGLE

const std::vector<char const *> validationLayers         = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char *> requiredDeviceExtensions = {vk::KHRSwapchainExtensionName};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

App::App()
{
    if (appInstance_) {
        throw std::runtime_error("Mirael::App singleton already constructed.");
    }
    appInstance_ = this;
}

void App::run()
{
    preInitImGui();
    initWindow();
    initVulkan();
    finishInitImGui();
    NfdShim::Init();
    mainLoop();
    cleanup();
}

void App::preInitImGui()
{
    IMGUI_CHECKVERSION();
    auto imGuiContext = ImGui::CreateContext();
    if (!imGuiContext) {
        throw std::runtime_error("Unable to create ImGui context.");
    }

    mainWindowSettings_.firstRun = !std::filesystem::exists("imgui.ini");

    auto settingsHandler = getImGuiSettingsHandler();
    ImGui::AddSettingsHandler(&settingsHandler);
    ImGui::LoadIniSettingsFromDisk(ImGui::GetIO().IniFilename);

    attemptReloadLastProject();

    auto &io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImFontConfig fontConfig{};
    fontConfig.OversampleH = 4;
    fontConfig.OversampleV = 4;
    io.Fonts->AddFontFromMemoryCompressedBase85TTF(Fonts::getInterRegularTtfBase85(), 0, &fontConfig);

    ImGui::StyleColorsDark();

    // skipping "Setup scaling" section in imgui glfw+vulkan example

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGuiStyle &style                 = ImGui::GetStyle();
        style.WindowRounding              = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
}

void App::initWindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    if (mainWindowSettings_.x) {
        glfwWindowHint(GLFW_POSITION_X, *mainWindowSettings_.x);
    }
    if (mainWindowSettings_.y) {
        glfwWindowHint(GLFW_POSITION_Y, *mainWindowSettings_.y);
    }
    if (mainWindowSettings_.maximized) {
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    }
    if (!mainWindowSettings_.width || !mainWindowSettings_.height) {
        mainWindowSettings_.width  = 1280;
        mainWindowSettings_.height = 720;
    }
    window_ = glfwCreateWindow(*mainWindowSettings_.width, *mainWindowSettings_.height, "Mirael", nullptr, nullptr);
    if (window_ == NULL) {
        throw std::runtime_error("GLFW: failed to create main window.");
    }

#ifdef WIN32
    WindowsOnly::customizeMainWindow(window_);
#endif

    if (mainWindowSettings_.fullscreen) {
        applyFullscreenSetting();
    }

    if (!glfwVulkanSupported()) {
        throw std::runtime_error("GLFW: Vulkan not supported.");
    }

    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, frameBufferResizeCallback);
    glfwSetWindowPosCallback(window_, windowPosCallback);
    glfwSetWindowSizeCallback(window_, windowSizeCallback);
    glfwSetWindowMaximizeCallback(window_, windowMaximizeCallback);
    glfwSetWindowCloseCallback(window_, windowCloseCallback);
}

void App::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createImageViews();
    createGraphicsPipeline();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
}

void App::finishInitImGui()
{
    ImGui_ImplGlfw_InitForVulkan(window_, true);

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance            = *instance_;
    initInfo.PhysicalDevice      = *physicalDevice_;
    initInfo.Device              = *device_;
    initInfo.QueueFamily         = graphicsQueueIndex_;
    initInfo.Queue               = *graphicsQueue_;
    initInfo.DescriptorPoolSize  = IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE;
    initInfo.MinImageCount       = minImageCount_;
    initInfo.ImageCount          = static_cast<uint32_t>(swapchainImages_.size());
    initInfo.UseDynamicRendering = true;

    VkFormat surfaceFormat       = static_cast<VkFormat>(swapchainSurfaceFormat_.format);
    auto &prci                   = initInfo.PipelineInfoMain.PipelineRenderingCreateInfo;
    prci.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    prci.colorAttachmentCount    = 1;
    prci.pColorAttachmentFormats = &surfaceFormat;

    ImGui_ImplVulkan_Init(&initInfo);

    auto &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        auto &platio                                      = ImGui::GetPlatformIO();
        baseImGuiPlatformHandlers_.Platform_CreateWindow  = platio.Platform_CreateWindow;
        baseImGuiPlatformHandlers_.Platform_DestroyWindow = platio.Platform_DestroyWindow;
        platio.Platform_CreateWindow                      = imGuiPlatform_CreateWindow;
        platio.Platform_DestroyWindow                     = imGuiPlatform_DestroyWindow;
    }
}

void App::mainLoop()
{
    while (!glfwWindowShouldClose(window_)) {
        ++metrics_.mainLoopIteration;

        glfwPollEvents();
        drawFrame();

        if (initialShowWindowPending_) {
            glfwShowWindow(window_);
            initialShowWindowPending_ = false;
        }
    }
}

void App::cleanup()
{
    NfdShim::Quit();

    device_.waitIdle();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window_);
    glfwTerminate();

    // the vulkan objects we created are all raii, so they take care of themselves.
}

void App::attemptReloadLastProject()
{
    if (!mainWindowSettings_.lastProjectPath)
        return;

    const auto &filepath = *mainWindowSettings_.lastProjectPath;

    if (filepath.empty() || !std::filesystem::exists(filepath))
        return;

    projectExplorer_.load(filepath);
}

void App::exit() { closeRequested_ = true; }

void App::applyFullscreenSetting()
{
    togglingFullscreen_ = true;

    if (mainWindowSettings_.fullscreen) {
        auto *monitor           = getCurrentMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        int monitorX, monitorY;
        glfwGetMonitorPos(monitor, &monitorX, &monitorY);
        glfwSetWindowAttrib(window_, GLFW_DECORATED, GLFW_FALSE);
        glfwSetWindowMonitor(window_, nullptr, monitorX, monitorY, mode->width, mode->height, GLFW_DONT_CARE);

    } else {
        const auto &s = mainWindowSettings_;
        glfwSetWindowAttrib(window_, GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowAttrib(window_, GLFW_FLOATING, GLFW_FALSE);
        if (s.maximized) {
            glfwHideWindow(window_);
        }
        glfwSetWindowMonitor(window_, nullptr, *s.x, *s.y, *s.width, *s.height, GLFW_DONT_CARE);
        if (s.maximized) {
            glfwPollEvents();
            glfwMaximizeWindow(window_);
            glfwShowWindow(window_);
        }
    }

    togglingFullscreen_ = false;
}

void App::setDestructiveAction(std::string label, std::string message, std::function<void()> postConfirmAction,
                               std::function<void()> postCancelAction)
{
    destructiveAction_ = {.modalTitle        = label,
                          .modalMessage      = message,
                          .modalCenter       = ImGui::GetWindowViewport()->GetCenter(),
                          .postConfirmAction = postConfirmAction,
                          .postCancelAction  = postCancelAction};
}

void App::showError(std::string message)
{
    // TODO: implement UI for this
    std::cerr << "Error: " << message << std::endl;
}

std::string App::getNewUuidAsString() const { return uuids::to_string(uuids::uuid_system_generator()()); }

void App::showDiagnosticRows()
{
    ImGuiEx::RowLabel("Main Loop Iteration");
    ImGui::Text("%u", metrics_.mainLoopIteration);

    ImGuiEx::RowLabel("Swapchain Build Count");
    ImGui::Text("%u", metrics_.swapChainBuildCount);

    ImGuiEx::RowLabel("Platform Windows Created");
    ImGui::Text("%u", metrics_.platformWindowCreateCount);

    ImGuiEx::RowLabel("Platform Windows Destroyed");
    ImGui::Text("%u", metrics_.platformWindowDestroyCount);
}

void App::showImGui()
{
    dockspaceId_ = ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    if (mainWindowSettings_.firstRun) {
        mainWindowSettings_.firstRun = false;
        ImGuiID leftMiddleBottomId{}, leftBottomId{};
        ImGuiID leftId       = ImGui::DockBuilderSplitNode(dockspaceId_, ImGuiDir_Left, 0.2f, nullptr, nullptr);
        ImGuiID leftTopId    = ImGui::DockBuilderSplitNode(leftId, ImGuiDir_Up, 0.33f, nullptr, &leftMiddleBottomId);
        ImGuiID leftMiddleId = ImGui::DockBuilderSplitNode(leftMiddleBottomId, ImGuiDir_Up, 0.5f, nullptr, &leftBottomId);
        ImGui::DockBuilderDockWindow(ProjectExplorer::windowName(), leftTopId);
        ImGui::DockBuilderDockWindow(Library::windowName(), leftMiddleId);
        ImGui::DockBuilderDockWindow(Properties::windowName(), leftBottomId);
        ImGui::DockBuilderFinish(dockspaceId_);
    }

    projectExplorer_.show();

    if (mainWindowSettings_.library) {
        library_.show(mainWindowSettings_.library);
    }
    if (mainWindowSettings_.properties) {
        properties_.show(mainWindowSettings_.properties);
    }
    if (mainWindowSettings_.settings) {
        settings_.show(mainWindowSettings_.settings);
    }
    if (mainWindowSettings_.diagnostics) {
        diagnostics_.show(mainWindowSettings_.diagnostics);
    }

    getProject().showGraphs();

    if (mainWindowSettings_.demo) {
        ImGui::ShowDemoWindow(&mainWindowSettings_.demo);
    }

    if (closeRequested_) {
        closeRequested_ = false;
        if (getProject().isModified()) {
            setDestructiveAction(
                "Exit with Unsaved Changes?", "Are you sure you want to discard all unsaved changes?  This cannot be undone.",
                [this]() {
                    closeConfirmed_ = true;
                    glfwSetWindowShouldClose(window_, GLFW_TRUE);
                },
                [this]() { closeRequested_ = false; });
        } else {
            closeConfirmed_ = true;
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
        }
    }

    if (destructiveAction_) {
        auto &actionInfo = *destructiveAction_;
        if (!actionInfo.opened) {
            ImGui::OpenPopup(actionInfo.modalTitle.c_str());
            actionInfo.opened = true;
        }
        ImGui::SetNextWindowPos(actionInfo.modalCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal(actionInfo.modalTitle.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text(actionInfo.modalMessage.c_str());
            ImGui::Separator();
            bool reset        = false;
            float buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2.0f;
            if (ImGui::Button("OK", ImVec2(buttonWidth, 0.0f))) {
                if (actionInfo.postConfirmAction)
                    actionInfo.postConfirmAction();
                reset = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0.0f))) {
                reset = true;
            }
            if (reset) {
                if (actionInfo.postCancelAction)
                    actionInfo.postCancelAction();
                destructiveAction_.reset();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}

ImGuiSettingsHandler App::getImGuiSettingsHandler()
{
    const char *settingsKey = "Mirael";
    ImGuiSettingsHandler handler;
    handler.UserData   = this;
    handler.TypeName   = settingsKey;
    handler.TypeHash   = ImHashStr(settingsKey);
    handler.ReadOpenFn = imGuiSettings_ReadOpen;
    handler.ReadLineFn = imGuiSettings_ReadLine;
    handler.WriteAllFn = imGuiSettings_WriteAll;
    return handler;
}

void App::imGuiSettings_WriteAll(ImGuiContext * /*ctx*/, ImGuiSettingsHandler *handler, ImGuiTextBuffer *out_buf)
{
    App &app                                = *static_cast<App *>(handler->UserData);
    app.mainWindowSettings_.lastProjectPath = app.getProject().getLastFilepath();
    const auto &settings                    = app.mainWindowSettings_;

    out_buf->appendf("[%s][MainWindow]\n", handler->TypeName);
    if (settings.x && settings.y)
        out_buf->appendf("Pos=%d,%d\n", *settings.x, *settings.y);
    if (settings.width && settings.height)
        out_buf->appendf("Size=%d,%d\n", *settings.width, *settings.height);
    out_buf->appendf("Maximized=%d\n", (int)settings.maximized);
    out_buf->appendf("Fullscreen=%d\n", (int)settings.fullscreen);
    out_buf->appendf("Library=%d\n", (int)settings.library);
    out_buf->appendf("Properties=%d\n", (int)settings.properties);
    out_buf->appendf("Settings=%d\n", (int)settings.settings);
    out_buf->appendf("Diagnostics=%d\n", (int)settings.diagnostics);
    out_buf->appendf("ImGuiDemo=%d\n", (int)settings.demo);
    if (settings.lastProjectPath)
        out_buf->appendf("LastProjectPath=%s\n", settings.lastProjectPath->string().c_str());
    out_buf->appendf("\n");
}

void *App::imGuiSettings_ReadOpen(ImGuiContext * /*ctx*/, ImGuiSettingsHandler *handler, const char * /*name*/)
{
    return handler->UserData;
}

void App::imGuiSettings_ReadLine(ImGuiContext * /*ctx*/, ImGuiSettingsHandler *handler, void *entry, const char *line)
{
    assert(entry == handler->UserData);
    App &app  = *static_cast<App *>(handler->UserData);
    auto &mws = app.mainWindowSettings_;

    int x, y, width, height, maximized, fullscreen, library, properties, settings, diagnostics, demo;
    if (sscanf_s(line, "Pos=%d,%d", &x, &y) == 2) {
        mws.x = x;
        mws.y = y;
    } else if (sscanf_s(line, "Size=%d,%d", &width, &height) == 2) {
        mws.width  = width;
        mws.height = height;
    } else if (sscanf_s(line, "Maximized=%d", &maximized) == 1) {
        mws.maximized = maximized != 0;
    } else if (sscanf_s(line, "Fullscreen=%d", &fullscreen) == 1) {
        mws.fullscreen = fullscreen != 0;
    } else if (sscanf_s(line, "Library=%d", &library) == 1) {
        mws.library = library != 0;
    } else if (sscanf_s(line, "Properties=%d", &properties) == 1) {
        mws.properties = properties != 0;
    } else if (sscanf_s(line, "Settings=%d", &settings) == 1) {
        mws.settings = settings != 0;
    } else if (sscanf_s(line, "Diagnostics=%d", &diagnostics) == 1) {
        mws.diagnostics = diagnostics != 0;
    } else if (sscanf_s(line, "ImGuiDemo=%d", &demo) == 1) {
        mws.demo = demo != 0;
    } else {
        std::string path;
        path.resize(strlen(line) + 1);
        if (sscanf_s(line, "LastProjectPath=%s", path.data(), (unsigned)path.size()) == 1) {
            mws.lastProjectPath = path;
        }
    }
}

void App::imGuiPlatform_CreateWindow(ImGuiViewport *vp)
{
    App &app = App::get();
    app.metrics_.platformWindowCreateCount++;
    app.baseImGuiPlatformHandlers_.Platform_CreateWindow(vp);

#ifdef WIN32
    WindowsOnly::customizeSeparatedWindow(static_cast<GLFWwindow *>(vp->PlatformHandle));
#endif
}

void App::imGuiPlatform_DestroyWindow(ImGuiViewport *vp)
{
    App &app = App::get();
    app.metrics_.platformWindowDestroyCount++;
    app.baseImGuiPlatformHandlers_.Platform_DestroyWindow(vp);
}

GLFWmonitor *App::getCurrentMonitor()
{
    int windowX, windowY, windowWidth, windowHeight;
    glfwGetWindowPos(window_, &windowX, &windowY);
    glfwGetWindowSize(window_, &windowWidth, &windowHeight);

    int monitorCount;
    GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);
    if (monitorCount < 1)
        throw std::runtime_error(std::format("Unable to get monitor count.  GLFW returned: {}", monitorCount));

    GLFWmonitor *bestMonitor = nullptr;
    int maxOverlap           = 0;

    for (int i : std::views::iota(0, monitorCount)) {
        const GLFWvidmode *mode = glfwGetVideoMode(monitors[i]);
        int monitorX, monitorY;
        glfwGetMonitorPos(monitors[i], &monitorX, &monitorY);

        int overlapX = std::max(0, std::min(windowX + windowWidth, monitorX + mode->width) - std::max(windowX, monitorX));
        int overlapY = std::max(0, std::min(windowY + windowHeight, monitorY + mode->height) - std::max(windowY, monitorY));
        int overlap  = overlapX * overlapY;

        if (overlap > maxOverlap) {
            maxOverlap  = overlap;
            bestMonitor = monitors[i];
        }
    }

    return bestMonitor ? bestMonitor : glfwGetPrimaryMonitor();
}

void App::frameBufferResizeCallback(GLFWwindow *window, int /*width*/, int /*height*/)
{
    auto app                 = App::fromWindow(window);
    app->frameBufferResized_ = true;
}

bool App::isWindowInSuperState(GLFWwindow *window)
{
    bool maximized = glfwGetWindowAttrib(window, GLFW_MAXIMIZED) != GLFW_FALSE;
    bool iconified = glfwGetWindowAttrib(window, GLFW_ICONIFIED) != GLFW_FALSE;
    auto app       = App::fromWindow(window);
    return maximized || iconified || app->mainWindowSettings_.fullscreen || app->togglingFullscreen_;
}

void App::windowPosCallback(GLFWwindow *window, int x, int y)
{
    // x and y are in screen coordinates
    if (isWindowInSuperState(window)) {
        return;
    }
    auto app                   = App::fromWindow(window);
    app->mainWindowSettings_.x = x;
    app->mainWindowSettings_.y = y;
}

void App::windowSizeCallback(GLFWwindow *window, int width, int height)
{
    // width and height are in screen coordinates
    if (isWindowInSuperState(window)) {
        return;
    }
    auto app                        = App::fromWindow(window);
    app->mainWindowSettings_.width  = width;
    app->mainWindowSettings_.height = height;
}

void App::windowMaximizeCallback(GLFWwindow *window, int maximized)
{
    auto app                           = App::fromWindow(window);
    app->mainWindowSettings_.maximized = maximized != GLFW_FALSE;
}

void App::windowCloseCallback(GLFWwindow *window)
{
    auto app = App::fromWindow(window);
    if (!app->closeConfirmed_) {
        app->closeRequested_ = true;
        glfwSetWindowShouldClose(window, GLFW_FALSE);
    }
}

void App::createInstance()
{
    constexpr vk::ApplicationInfo appInfo{.pApplicationName   = "Hello Triangle",
                                          .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                                          .pEngineName        = "No Engine",
                                          .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
                                          .apiVersion         = vk::ApiVersion14};

    // get required layers
    std::vector<const char *> requiredLayers;
    if constexpr (enableValidationLayers) {
        requiredLayers.assign(validationLayers.begin(), validationLayers.end());
    }

    // check that all required layers are available
    auto supportedLayers =
        context_.enumerateInstanceLayerProperties() | std::views::transform([](const auto &property) { return property.layerName; });
    auto unsupportedLayerIt = findFirstMissingString(requiredLayers, supportedLayers);
    if (unsupportedLayerIt != requiredLayers.end()) {
        throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIt));
    }

    // get required extensions
    auto requiredInstanceExtensions = getRequiredInstanceExtensions();

    // check that all required extensions are available
    auto supportedInstanceExtensions = context_.enumerateInstanceExtensionProperties() |
                                       std::views::transform([](const auto &property) { return property.extensionName; });
    auto unsupportedInstanceExtensionIt = findFirstMissingString(requiredInstanceExtensions, supportedInstanceExtensions);
    if (unsupportedInstanceExtensionIt != requiredInstanceExtensions.end()) {
        throw std::runtime_error("Required GLFW extension not supported: " + std::string(*unsupportedInstanceExtensionIt));
    }

    vk::InstanceCreateInfo createInfo{.pApplicationInfo        = &appInfo,
                                      .enabledLayerCount       = static_cast<uint32_t>(requiredLayers.size()),
                                      .ppEnabledLayerNames     = requiredLayers.data(),
                                      .enabledExtensionCount   = static_cast<uint32_t>(requiredInstanceExtensions.size()),
                                      .ppEnabledExtensionNames = requiredInstanceExtensions.data()};

    instance_ = vk::raii::Instance(context_, createInfo);
}

std::vector<const char *> App::getRequiredInstanceExtensions()
{
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions         = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if constexpr (enableValidationLayers) {
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }
    return extensions;
}

void App::setupDebugMessenger()
{
    if constexpr (!enableValidationLayers)
        return;

    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
        .messageSeverity = severityFlags, .messageType = messageTypeFlags, .pfnUserCallback = &debugCallback};

    debugMessenger_ = instance_.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL App::debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                    vk::DebugUtilsMessageTypeFlagsEXT type,
                                                    const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *)
{
    if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
        std::cerr << "validation layer: " << to_string(severity) << ", " << to_string(type) << ": " << pCallbackData->pMessage
                  << std::endl;
    }
    return vk::False;
}

void App::createSurface()
{
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(*instance_, window_, nullptr, &surface) != 0) {
        throw std::runtime_error("Failed to create window surface!");
    }
    surface_ = vk::raii::SurfaceKHR(instance_, surface);
}

void App::pickPhysicalDevice()
{
    auto physicalDevices = instance_.enumeratePhysicalDevices();
    auto it = std::ranges::find_if(physicalDevices, [&](auto const &physicalDevice) { return isDeviceSuitable(physicalDevice); });
    if (it == physicalDevices.end()) {
        throw std::runtime_error("Failed to find GPU with Vulkan support.");
    } else {
        physicalDevice_ = *it;
    }
}

bool App::isDeviceSuitable(const vk::raii::PhysicalDevice &physicalDevice)
{
    // check for API v1.3+
    auto physicalDeviceProperties = physicalDevice.getProperties();
    bool supportsVulkan1_3        = physicalDeviceProperties.apiVersion >= vk::ApiVersion13;
    if (!supportsVulkan1_3) {
        return false;
    }

    // check for graphics support
    auto queueFamilyProps = physicalDevice.getQueueFamilyProperties();
    bool supportsGraphics =
        std::ranges::any_of(queueFamilyProps, [](const auto &qfp) { return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });
    if (!supportsGraphics) {
        return false;
    }

    // check for required device extensions
    auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties() |
                                     std::views::transform([&](const auto &deviceExtension) { return deviceExtension.extensionName; });
    if (!areAllRequiredStringsPresent(requiredDeviceExtensions, availableDeviceExtensions)) {
        return false;
    }

    // check for required physical device features
    auto features =
        physicalDevice.template getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features,
                                             vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
    bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
                                    features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                                    features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
                                    features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;
    if (!supportsRequiredFeatures) {
        return false;
    }

    std::cout << "Found suitable device: " << physicalDeviceProperties.deviceName << std::endl;
    return true;
}

void App::createLogicalDevice()
{
    assert(graphicsQueueIndex_ == ~0);
    auto queueFamilyProperties = physicalDevice_.getQueueFamilyProperties();
    for (uint32_t queueIndex : std::views::iota(0u, queueFamilyProperties.size())) {
        if ((queueFamilyProperties[queueIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
            physicalDevice_.getSurfaceSupportKHR(queueIndex, *surface_)) {
            graphicsQueueIndex_ = queueIndex;
            break;
        }
    }
    if (graphicsQueueIndex_ == ~0) {
        throw std::runtime_error("Could not find a single queue for both graphics and present!");
    }

    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan13Features,
                       vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
        featureChain = {{},                                                   // vk::PhysicalDeviceFeatures2, empty for now
                        {.shaderDrawParameters = true},                       // vk::PhysicalDeviceVulkan11Features
                        {.synchronization2 = true, .dynamicRendering = true}, // vk::PhysicalDeviceVulkan13Features
                        {.extendedDynamicState = true}};                      // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT

    constexpr float queuePriority = 0.5f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
        .queueFamilyIndex = graphicsQueueIndex_, .queueCount = 1, .pQueuePriorities = &queuePriority};
    vk::DeviceCreateInfo deviceCreateInfo{.pNext                   = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
                                          .queueCreateInfoCount    = deviceQueueCreateInfo.queueCount,
                                          .pQueueCreateInfos       = &deviceQueueCreateInfo,
                                          .enabledExtensionCount   = static_cast<uint32_t>(requiredDeviceExtensions.size()),
                                          .ppEnabledExtensionNames = requiredDeviceExtensions.data()};

    device_        = vk::raii::Device(physicalDevice_, deviceCreateInfo);
    graphicsQueue_ = vk::raii::Queue(device_, graphicsQueueIndex_, 0);
}

void App::createSwapchain()
{
    ++metrics_.swapChainBuildCount;

    vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice_.getSurfaceCapabilitiesKHR(*surface_);
    swapchainExtent_                               = chooseSwapExtent(surfaceCapabilities);
    minImageCount_                                 = chooseSwapMinImageCount(surfaceCapabilities);

    auto availableFormats   = physicalDevice_.getSurfaceFormatsKHR(*surface_);
    swapchainSurfaceFormat_ = chooseSwapSurfaceFormat(availableFormats);

    auto availablePresentModes = physicalDevice_.getSurfacePresentModesKHR(*surface_);
    auto presentMode           = chooseSwapPresentMode(availablePresentModes);

    vk::SwapchainCreateInfoKHR swapchainCreateInfo{
        .surface          = *surface_,
        .minImageCount    = minImageCount_,
        .imageFormat      = swapchainSurfaceFormat_.format,
        .imageColorSpace  = swapchainSurfaceFormat_.colorSpace,
        .imageExtent      = swapchainExtent_,
        .imageArrayLayers = 1,
        .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment, // we might later use eTransferDst if using a memory operation to
                                                                      // transfer images to the swap chain
        .imageSharingMode = vk::SharingMode::eExclusive,              // stick to this if graphics and presentation queue is the same
        .preTransform     = surfaceCapabilities.currentTransform,
        .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode      = presentMode,
        .clipped          = true};
    swapchain_       = vk::raii::SwapchainKHR(device_, swapchainCreateInfo);
    swapchainImages_ = swapchain_.getImages();
}

void App::cleanupSwapchain()
{
    swapchainImageViews_.clear();
    swapchain_ = nullptr;
}

void App::recreateSwapchain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window_, &width, &height);
        glfwWaitEvents();
    }

    device_.waitIdle();

    cleanupSwapchain();

    createSwapchain();
    createImageViews();
}

vk::SurfaceFormatKHR App::chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const &availableFormats)
{
    const auto formatIt = std::ranges::find_if(availableFormats, [](const auto &format) {
        return format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
    });
    return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}

vk::PresentModeKHR App::chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const &availablePresentModes)
{
    /* TOOD: consider making eMailbox a user option when available
    bool isMailboxAvailable =
        std::ranges::any_of(availablePresentModes, [](const auto value) { return vk::PresentModeKHR::eMailbox == value; });
    return isMailboxAvailable ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
    */
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D App::chooseSwapExtent(vk::SurfaceCapabilitiesKHR const &capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);
    return {std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
}

uint32_t App::chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities)
{
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount)) {
        minImageCount = surfaceCapabilities.maxImageCount;
    }
    return minImageCount;
}

void App::createImageViews()
{
    vk::ImageViewCreateInfo imageViewCreateInfo{
        .viewType         = vk::ImageViewType::e2D,
        .format           = swapchainSurfaceFormat_.format,
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = 1}};

    for (auto &image : swapchainImages_) {
        imageViewCreateInfo.image = image;
        swapchainImageViews_.emplace_back(device_, imageViewCreateInfo);
    }
}

void App::createGraphicsPipeline()
{
    // shader stages
    auto shaderCode   = readSmallFile("shaders/slang.spv");
    auto shaderModule = createShaderModule(shaderCode);
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage  = vk::ShaderStageFlagBits::eVertex,
        .module = shaderModule,
        .pName  = "vertMain"}; // in the future, you could use .pSpecializationInfo to set shader constants
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain"};
    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // dynamic states
    std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicState{.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
                                                    .pDynamicStates    = dynamicStates.data()};

    // input assembly
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo{.topology = vk::PrimitiveTopology::eTriangleList};

    // viewport & scissor
    vk::Viewport viewport{.x        = 0.0f,
                          .y        = 0.0f,
                          .width    = static_cast<float>(swapchainExtent_.width),
                          .height   = static_cast<float>(swapchainExtent_.height),
                          .minDepth = 0.0f,
                          .maxDepth = 1.0f};
    vk::Rect2D scissor{vk::Offset2D{0, 0}, swapchainExtent_};
    vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1, .scissorCount = 1};

    // rasterizer
    vk::PipelineRasterizationStateCreateInfo rasterizer{.depthClampEnable        = vk::False,
                                                        .rasterizerDiscardEnable = vk::False,
                                                        .polygonMode             = vk::PolygonMode::eFill,
                                                        .cullMode                = vk::CullModeFlagBits::eBack,
                                                        .frontFace               = vk::FrontFace::eClockwise,
                                                        .depthBiasEnable         = vk::False,
                                                        .lineWidth               = 1.0f};

    // multisampling
    vk::PipelineMultisampleStateCreateInfo multisampling{.rasterizationSamples = vk::SampleCountFlagBits::e1,
                                                         .sampleShadingEnable  = vk::False};

    // not using depth/stencil testing, so no vk::PipelineDepthStencilStateCreateInfo

    // color blending
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{.blendEnable = vk::False,
                                                               .colorWriteMask =
                                                                   vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                                   vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &colorBlendAttachment};

    // pipeline layout
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{.setLayoutCount         = 0,  // for "uniform" values used in shaders
                                                    .pushConstantRangeCount = 0}; // for "push constants" for use in shaders
    pipelineLayout_ = vk::raii::PipelineLayout(device_, pipelineLayoutInfo);

    // rendering
    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
        {
            .stageCount          = 2,
            .pStages             = shaderStages,
            .pVertexInputState   = &vertexInputInfo,
            .pInputAssemblyState = &inputAssemblyInfo,
            .pViewportState      = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState   = &multisampling,
            .pColorBlendState    = &colorBlending,
            .pDynamicState       = &dynamicState,
            .layout              = pipelineLayout_, // pipeline layout is a vulkan handle rather than a struct
            .renderPass          = nullptr,
        },
        {.colorAttachmentCount = 1, .pColorAttachmentFormats = &swapchainSurfaceFormat_.format}};

    // full pipeline
    graphicsPipeline_ = vk::raii::Pipeline(device_, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
}

[[nodiscard]] vk::raii::ShaderModule App::createShaderModule(const std::vector<char> &code) const
{
    vk::ShaderModuleCreateInfo createInfo{.codeSize = code.size() * sizeof(char),
                                          .pCode    = reinterpret_cast<const uint32_t *>(code.data())};
    vk::raii::ShaderModule shaderModule(device_, createInfo);
    return shaderModule;
}

void App::createCommandPool()
{
    vk::CommandPoolCreateInfo poolInfo{.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                       .queueFamilyIndex = graphicsQueueIndex_};
    commandPool_ = vk::raii::CommandPool(device_, poolInfo);
}

void App::createCommandBuffers()
{
    vk::CommandBufferAllocateInfo allocInfo{.commandPool = commandPool_,
                                            .level = vk::CommandBufferLevel::ePrimary, // primary can be submitted, secondary can be
                                                                                       // called by primary but not submitted directly
                                            .commandBufferCount = MAX_FRAMES_IN_FLIGHT};
    commandBuffers_ = vk::raii::CommandBuffers(device_, allocInfo);
}

void App::createSyncObjects()
{
    assert(presentCompleteSemaphores_.empty() && renderFinishedSemaphores_.empty() && inFlightFences_.empty());

    for (size_t i : std::views::iota(0u, swapchainImages_.size())) {
        renderFinishedSemaphores_.emplace_back(device_, vk::SemaphoreCreateInfo());
    }

    for (size_t i : std::views::iota(0u, MAX_FRAMES_IN_FLIGHT)) {
        presentCompleteSemaphores_.emplace_back(device_, vk::SemaphoreCreateInfo());
        inFlightFences_.emplace_back(device_, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }
}

void App::transitionImageLayout(uint32_t imageIndex, vk::ImageLayout old_layout, vk::ImageLayout new_layout,
                                vk::AccessFlags2 src_access_mask, vk::AccessFlags2 dst_access_mask,
                                vk::PipelineStageFlags2 src_stage_mask, vk::PipelineStageFlags2 dst_stage_mask)
{
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask        = src_stage_mask,
        .srcAccessMask       = src_access_mask,
        .dstStageMask        = dst_stage_mask,
        .dstAccessMask       = dst_access_mask,
        .oldLayout           = old_layout,
        .newLayout           = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = swapchainImages_[imageIndex],
        .subresourceRange    = {
               .aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};

    vk::DependencyInfo dependencyInfo = {.dependencyFlags = {}, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier};
    commandBuffers_[frameIndex_].pipelineBarrier2(dependencyInfo);
}

void App::recordCommandBuffer(uint32_t imageIndex)
{
    auto &commandBuffer = commandBuffers_[frameIndex_];
    commandBuffer.begin({});

    transitionImageLayout(imageIndex,

                          // layout
                          vk::ImageLayout::eUndefined,              // src
                          vk::ImageLayout::eColorAttachmentOptimal, // dst

                          // access mask
                          {},                                         // src - don't care
                          vk::AccessFlagBits2::eColorAttachmentWrite, // dst

                          // stage mask
                          vk::PipelineStageFlagBits2::eColorAttachmentOutput, // src
                          vk::PipelineStageFlagBits2::eColorAttachmentOutput  // dst
    );

    vk::ClearValue clearColor                  = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::RenderingAttachmentInfo attachmentInfo = {.imageView   = swapchainImageViews_[imageIndex],
                                                  .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                                  .loadOp      = vk::AttachmentLoadOp::eClear,
                                                  .storeOp     = vk::AttachmentStoreOp::eStore,
                                                  .clearValue  = clearColor};
    vk::RenderingInfo renderingInfo            = {.renderArea           = {.offset = {0, 0}, .extent = swapchainExtent_},
                                                  .layerCount           = 1,
                                                  .colorAttachmentCount = 1,
                                                  .pColorAttachments    = &attachmentInfo};
    commandBuffer.beginRendering(renderingInfo);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline_);

#ifdef SHOW_DEBUG_TRIANGLE
    commandBuffer.setViewport(
        0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapchainExtent_.width), static_cast<float>(swapchainExtent_.height)));
    commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapchainExtent_));
    commandBuffer.draw(3, 1, 0, 0);
#endif

    auto drawData = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(drawData, *commandBuffer);

    commandBuffer.endRendering();

    transitionImageLayout(imageIndex,

                          // layout
                          vk::ImageLayout::eColorAttachmentOptimal, // src
                          vk::ImageLayout::ePresentSrcKHR,          // dst

                          // access mask
                          vk::AccessFlagBits2::eColorAttachmentWrite, // src
                          {},                                         // dst

                          // stage mask
                          vk::PipelineStageFlagBits2::eColorAttachmentOutput, // src
                          vk::PipelineStageFlagBits2::eBottomOfPipe           // dst
    );

    commandBuffer.end();
}

void App::drawFrame()
{
    auto fenceResult = device_.waitForFences(*inFlightFences_[frameIndex_], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for fence.");
    }

    auto [result, imageIndex] = swapchain_.acquireNextImage(UINT64_MAX, *presentCompleteSemaphores_[frameIndex_], nullptr);
    switch (result) {
    case vk::Result::eErrorOutOfDateKHR:
        recreateSwapchain();
        return;
    case vk::Result::eSuccess:
        break;
    case vk::Result::eSuboptimalKHR:
        break;
    default:
        throw new std::runtime_error(std::format("Failed to acquire swap chain image!  Result = {}", to_string(result)));
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    showImGui();
    ImGui::Render();

    device_.resetFences(*inFlightFences_[frameIndex_]); // only reset the fence if we are submitting work, otherwise we can deadlock

    commandBuffers_[frameIndex_].reset();
    recordCommandBuffer(imageIndex);

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submitInfo{.waitSemaphoreCount   = 1,
                                    .pWaitSemaphores      = &*presentCompleteSemaphores_[frameIndex_],
                                    .pWaitDstStageMask    = &waitDestinationStageMask,
                                    .commandBufferCount   = 1,
                                    .pCommandBuffers      = &*commandBuffers_[frameIndex_],
                                    .signalSemaphoreCount = 1,
                                    .pSignalSemaphores    = &*renderFinishedSemaphores_[imageIndex]};

    graphicsQueue_.submit(submitInfo, *inFlightFences_[frameIndex_]);

    auto &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    const vk::PresentInfoKHR presentInfoKHR{.waitSemaphoreCount = 1,
                                            .pWaitSemaphores    = &*renderFinishedSemaphores_[imageIndex],
                                            .swapchainCount     = 1,
                                            .pSwapchains        = &*swapchain_,
                                            .pImageIndices      = &imageIndex};
    result = graphicsQueue_.presentKHR(presentInfoKHR);

    if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR || frameBufferResized_) {
        frameBufferResized_ = false;
        recreateSwapchain();
    }

    frameIndex_ = (frameIndex_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

}; // namespace Mirael
