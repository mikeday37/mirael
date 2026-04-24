#include "pch.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <vector>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imgui_internal.h"

#include "app.h"
#include "fonts.h"
#include "nfd_shim.h"
#include "util.h"

#ifndef IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING
#error "Dynamic rendering not supported."
#endif

namespace Mirael
{

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
    if (appInstance) {
        throw std::runtime_error("Mirael::App singleton already constructed.");
    }
    appInstance = this;
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

    mainWindowSettings.firstRun = !std::filesystem::exists("imgui.ini");

    auto settingsHandler = getImGuiSettingsHandler();
    ImGui::AddSettingsHandler(&settingsHandler);
    ImGui::LoadIniSettingsFromDisk(ImGui::GetIO().IniFilename);

    if (mainWindowSettings.lastProjectPath)
        project.resumeLastProject(*mainWindowSettings.lastProjectPath);

    auto &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    io.Fonts->AddFontFromMemoryCompressedBase85TTF(Fonts::getInterRegularTtfBase85());

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

    if (mainWindowSettings.x) {
        glfwWindowHint(GLFW_POSITION_X, *mainWindowSettings.x);
    }
    if (mainWindowSettings.y) {
        glfwWindowHint(GLFW_POSITION_Y, *mainWindowSettings.y);
    }
    if (mainWindowSettings.maximized) {
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    }
    if (!mainWindowSettings.width || !mainWindowSettings.height) {
        mainWindowSettings.width  = 1280;
        mainWindowSettings.height = 720;
    }
    window = glfwCreateWindow(*mainWindowSettings.width, *mainWindowSettings.height, "Mirael", nullptr, nullptr);
    if (window == NULL) {
        throw std::runtime_error("GLFW: failed to create main window.");
    }

    if (!glfwVulkanSupported()) {
        throw std::runtime_error("GLFW: Vulkan not supported.");
    }

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, frameBufferResizeCallback);
    glfwSetWindowPosCallback(window, windowPosCallback);
    glfwSetWindowSizeCallback(window, windowSizeCallback);
    glfwSetWindowMaximizeCallback(window, windowMaximizeCallback);
    glfwSetWindowCloseCallback(window, windowCloseCallback);
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
    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance            = *instance;
    initInfo.PhysicalDevice      = *physicalDevice;
    initInfo.Device              = *device;
    initInfo.QueueFamily         = graphicsQueueIndex;
    initInfo.Queue               = *graphicsQueue;
    initInfo.DescriptorPoolSize  = IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE;
    initInfo.MinImageCount       = minImageCount;
    initInfo.ImageCount          = static_cast<uint32_t>(swapchainImages.size());
    initInfo.UseDynamicRendering = true;

    VkFormat surfaceFormat       = static_cast<VkFormat>(swapchainSurfaceFormat.format);
    auto &prci                   = initInfo.PipelineInfoMain.PipelineRenderingCreateInfo;
    prci.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    prci.colorAttachmentCount    = 1;
    prci.pColorAttachmentFormats = &surfaceFormat;

    ImGui_ImplVulkan_Init(&initInfo);
}

void App::mainLoop()
{
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }
}

void App::cleanup()
{
    NfdShim::Quit();

    device.waitIdle();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    // the vulkan objects we created are all raii, so they take care of themselves.
}

void App::exit() { closeRequested = true; }

void App::setDestructiveAction(std::string label, std::string message, std::function<void()> postConfirmAction,
                               std::function<void()> postCancelAction)
{
    destructiveAction = {.modalTitle        = label,
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

void App::showImGui()
{
    dockspaceId = ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    if (mainWindowSettings.firstRun) {
        mainWindowSettings.firstRun = false;
        auto leftId                 = ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.2f, nullptr, nullptr);
        ImGui::DockBuilderDockWindow("Project Explorer", leftId);
        ImGui::DockBuilderFinish(dockspaceId);
    }

    showBackgroundContextMenu();

    if (mainWindowSettings.explorer) {
        project.showExplorer(mainWindowSettings.explorer);
    }

    project.showGraphs();

    if (mainWindowSettings.demo) {
        ImGui::ShowDemoWindow(&mainWindowSettings.demo);
    }

    if (closeRequested) {
        closeRequested = false;
        if (project.isModified()) {
            setDestructiveAction(
                "Exit with Unsaved Changes?", "Are you sure you want to discard all unsaved changes?  This cannot be undone.",
                [this]() {
                    closeConfirmed = true;
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                },
                [this]() { closeRequested = false; });
        } else {
            closeConfirmed = true;
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }

    if (destructiveAction) {
        auto &actionInfo = *destructiveAction;
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
                destructiveAction.reset();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}

void App::showBackgroundContextMenu()
{
    if (ImGui::BeginPopupContextVoid("##mainwindow_background_context_menu")) {
        ImGui::MenuItem("Project Explorer", nullptr, &mainWindowSettings.explorer);
        ImGui::Separator();
        if (ImGui::MenuItem("Exit")) {
            exit();
        }
        ImGui::EndPopup();
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
    App &app                               = *static_cast<App *>(handler->UserData);
    app.mainWindowSettings.lastProjectPath = app.project.getLastFilePath();
    const auto &settings                   = app.mainWindowSettings;

    out_buf->appendf("[%s][MainWindow]\n", handler->TypeName);
    if (settings.x && settings.y)
        out_buf->appendf("Pos=%d,%d\n", *settings.x, *settings.y);
    if (settings.width && settings.height)
        out_buf->appendf("Size=%d,%d\n", *settings.width, *settings.height);
    out_buf->appendf("Maximized=%d\n", (int)settings.maximized);
    out_buf->appendf("Explorer=%d\n", (int)settings.explorer);
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
    App &app       = *static_cast<App *>(handler->UserData);
    auto &settings = app.mainWindowSettings;

    int x, y, width, height, maximized, explorer, demo;
    if (sscanf_s(line, "Pos=%d,%d", &x, &y) == 2) {
        settings.x = x;
        settings.y = y;
    } else if (sscanf_s(line, "Size=%d,%d", &width, &height) == 2) {
        settings.width  = width;
        settings.height = height;
    } else if (sscanf_s(line, "Maximized=%d", &maximized) == 1) {
        settings.maximized = maximized != 0;
    } else if (sscanf_s(line, "Explorer=%d", &explorer) == 1) {
        settings.explorer = explorer != 0;
    } else if (sscanf_s(line, "ImGuiDemo=%d", &demo) == 1) {
        settings.demo = demo != 0;
    } else {
        std::string path;
        path.resize(strlen(line) + 1);
        if (sscanf_s(line, "LastProjectPath=%s", path.data(), (unsigned)path.size()) == 1) {
            settings.lastProjectPath = path;
        }
    }
}

void App::frameBufferResizeCallback(GLFWwindow *window, int /*width*/, int /*height*/)
{
    auto app                = reinterpret_cast<App *>(glfwGetWindowUserPointer(window));
    app->frameBufferResized = true;
}

bool App::isWindowInSuperState(GLFWwindow *window)
{
    bool maximized = glfwGetWindowAttrib(window, GLFW_MAXIMIZED) != GLFW_FALSE;
    bool iconified = glfwGetWindowAttrib(window, GLFW_ICONIFIED) != GLFW_FALSE;
    return maximized || iconified;
}

void App::windowPosCallback(GLFWwindow *window, int x, int y)
{
    // x and y are in screen coordinates
    if (isWindowInSuperState(window)) {
        return;
    }
    auto app = reinterpret_cast<App *>(glfwGetWindowUserPointer(window));
    if (!app) {
        return;
    }
    app->mainWindowSettings.x = x;
    app->mainWindowSettings.y = y;
}

void App::windowSizeCallback(GLFWwindow *window, int width, int height)
{
    // width and height are in screen coordinates
    if (isWindowInSuperState(window)) {
        return;
    }
    auto app = reinterpret_cast<App *>(glfwGetWindowUserPointer(window));
    if (!app) {
        return;
    }
    app->mainWindowSettings.width  = width;
    app->mainWindowSettings.height = height;
}

void App::windowMaximizeCallback(GLFWwindow *window, int maximized)
{
    auto app = reinterpret_cast<App *>(glfwGetWindowUserPointer(window));
    if (!app) {
        return;
    }
    app->mainWindowSettings.maximized = maximized != GLFW_FALSE;
}

void App::windowCloseCallback(GLFWwindow *window)
{
    auto app = reinterpret_cast<App *>(glfwGetWindowUserPointer(window));
    if (!app) {
        return;
    }
    if (!app->closeConfirmed) {
        app->closeRequested = true;
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
        context.enumerateInstanceLayerProperties() | std::views::transform([](const auto &property) { return property.layerName; });
    auto unsupportedLayerIt = findFirstMissingString(requiredLayers, supportedLayers);
    if (unsupportedLayerIt != requiredLayers.end()) {
        throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIt));
    }

    // get required extensions
    auto requiredInstanceExtensions = getRequiredInstanceExtensions();

    // check that all required extensions are available
    auto supportedInstanceExtensions = context.enumerateInstanceExtensionProperties() |
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

    instance = vk::raii::Instance(context, createInfo);
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

    debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
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
    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0) {
        throw std::runtime_error("Failed to create window surface!");
    }
    surface = vk::raii::SurfaceKHR(instance, _surface);
}

void App::pickPhysicalDevice()
{
    auto physicalDevices = instance.enumeratePhysicalDevices();
    auto it = std::ranges::find_if(physicalDevices, [&](auto const &physicalDevice) { return isDeviceSuitable(physicalDevice); });
    if (it == physicalDevices.end()) {
        throw std::runtime_error("Failed to find GPU with Vulkan support.");
    } else {
        physicalDevice = *it;
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
    assert(graphicsQueueIndex == ~0);
    auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
    for (uint32_t queueIndex : std::views::iota(0u, queueFamilyProperties.size())) {
        if ((queueFamilyProperties[queueIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
            physicalDevice.getSurfaceSupportKHR(queueIndex, *surface)) {
            graphicsQueueIndex = queueIndex;
            break;
        }
    }
    if (graphicsQueueIndex == ~0) {
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
        .queueFamilyIndex = graphicsQueueIndex, .queueCount = 1, .pQueuePriorities = &queuePriority};
    vk::DeviceCreateInfo deviceCreateInfo{.pNext                   = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
                                          .queueCreateInfoCount    = deviceQueueCreateInfo.queueCount,
                                          .pQueueCreateInfos       = &deviceQueueCreateInfo,
                                          .enabledExtensionCount   = static_cast<uint32_t>(requiredDeviceExtensions.size()),
                                          .ppEnabledExtensionNames = requiredDeviceExtensions.data()};

    device        = vk::raii::Device(physicalDevice, deviceCreateInfo);
    graphicsQueue = vk::raii::Queue(device, graphicsQueueIndex, 0);
}

void App::createSwapchain()
{
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
    swapchainExtent                                = chooseSwapExtent(surfaceCapabilities);
    minImageCount                                  = chooseSwapMinImageCount(surfaceCapabilities);

    auto availableFormats  = physicalDevice.getSurfaceFormatsKHR(*surface);
    swapchainSurfaceFormat = chooseSwapSurfaceFormat(availableFormats);

    auto availablePresentModes = physicalDevice.getSurfacePresentModesKHR(*surface);
    auto presentMode           = chooseSwapPresentMode(availablePresentModes);

    vk::SwapchainCreateInfoKHR swapchainCreateInfo{
        .surface          = *surface,
        .minImageCount    = minImageCount,
        .imageFormat      = swapchainSurfaceFormat.format,
        .imageColorSpace  = swapchainSurfaceFormat.colorSpace,
        .imageExtent      = swapchainExtent,
        .imageArrayLayers = 1,
        .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment, // we might later use eTransferDst if using a memory operation to
                                                                      // transfer images to the swap chain
        .imageSharingMode = vk::SharingMode::eExclusive,              // stick to this if graphics and presentation queue is the same
        .preTransform     = surfaceCapabilities.currentTransform,
        .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode      = presentMode,
        .clipped          = true};
    swapchain       = vk::raii::SwapchainKHR(device, swapchainCreateInfo);
    swapchainImages = swapchain.getImages();
}

void App::cleanupSwapchain()
{
    swapchainImageViews.clear();
    swapchain = nullptr;
}

void App::recreateSwapchain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    device.waitIdle();

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
    bool isMailboxAvailable =
        std::ranges::any_of(availablePresentModes, [](const auto value) { return vk::PresentModeKHR::eMailbox == value; });
    return isMailboxAvailable ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
}

vk::Extent2D App::chooseSwapExtent(vk::SurfaceCapabilitiesKHR const &capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
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
        .format           = swapchainSurfaceFormat.format,
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = 1}};

    for (auto &image : swapchainImages) {
        imageViewCreateInfo.image = image;
        swapchainImageViews.emplace_back(device, imageViewCreateInfo);
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
                          .width    = static_cast<float>(swapchainExtent.width),
                          .height   = static_cast<float>(swapchainExtent.height),
                          .minDepth = 0.0f,
                          .maxDepth = 1.0f};
    vk::Rect2D scissor{vk::Offset2D{0, 0}, swapchainExtent};
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
    pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

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
            .layout              = pipelineLayout, // pipeline layout is a vulkan handle rather than a struct
            .renderPass          = nullptr,
        },
        {.colorAttachmentCount = 1, .pColorAttachmentFormats = &swapchainSurfaceFormat.format}};

    // full pipeline
    graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
}

[[nodiscard]] vk::raii::ShaderModule App::createShaderModule(const std::vector<char> &code) const
{
    vk::ShaderModuleCreateInfo createInfo{.codeSize = code.size() * sizeof(char),
                                          .pCode    = reinterpret_cast<const uint32_t *>(code.data())};
    vk::raii::ShaderModule shaderModule(device, createInfo);
    return shaderModule;
}

void App::createCommandPool()
{
    vk::CommandPoolCreateInfo poolInfo{.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                       .queueFamilyIndex = graphicsQueueIndex};
    commandPool = vk::raii::CommandPool(device, poolInfo);
}

void App::createCommandBuffers()
{
    vk::CommandBufferAllocateInfo allocInfo{.commandPool = commandPool,
                                            .level = vk::CommandBufferLevel::ePrimary, // primary can be submitted, secondary can be
                                                                                       // called by primary but not submitted directly
                                            .commandBufferCount = MAX_FRAMES_IN_FLIGHT};
    commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
}

void App::createSyncObjects()
{
    assert(presentCompleteSemaphores.empty() && renderFinishedSemaphores.empty() && inFlightFences.empty());

    for (size_t i : std::views::iota(0u, swapchainImages.size())) {
        renderFinishedSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
    }

    for (size_t i : std::views::iota(0u, MAX_FRAMES_IN_FLIGHT)) {
        presentCompleteSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
        inFlightFences.emplace_back(device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
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
        .image               = swapchainImages[imageIndex],
        .subresourceRange    = {
               .aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};

    vk::DependencyInfo dependencyInfo = {.dependencyFlags = {}, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier};
    commandBuffers[frameIndex].pipelineBarrier2(dependencyInfo);
}

void App::recordCommandBuffer(uint32_t imageIndex)
{
    auto &commandBuffer = commandBuffers[frameIndex];
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
    vk::RenderingAttachmentInfo attachmentInfo = {.imageView   = swapchainImageViews[imageIndex],
                                                  .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                                  .loadOp      = vk::AttachmentLoadOp::eClear,
                                                  .storeOp     = vk::AttachmentStoreOp::eStore,
                                                  .clearValue  = clearColor};
    vk::RenderingInfo renderingInfo            = {.renderArea           = {.offset = {0, 0}, .extent = swapchainExtent},
                                                  .layerCount           = 1,
                                                  .colorAttachmentCount = 1,
                                                  .pColorAttachments    = &attachmentInfo};
    commandBuffer.beginRendering(renderingInfo);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);

    commandBuffer.setViewport(
        0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapchainExtent.width), static_cast<float>(swapchainExtent.height)));
    commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapchainExtent));

    commandBuffer.draw(3, 1, 0, 0);

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
    auto fenceResult = device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for fence.");
    }

    auto [result, imageIndex] = swapchain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);
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

    device.resetFences(*inFlightFences[frameIndex]); // only reset the fence if we are submitting work, otherwise we can deadlock

    commandBuffers[frameIndex].reset();
    recordCommandBuffer(imageIndex);

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submitInfo{.waitSemaphoreCount   = 1,
                                    .pWaitSemaphores      = &*presentCompleteSemaphores[frameIndex],
                                    .pWaitDstStageMask    = &waitDestinationStageMask,
                                    .commandBufferCount   = 1,
                                    .pCommandBuffers      = &*commandBuffers[frameIndex],
                                    .signalSemaphoreCount = 1,
                                    .pSignalSemaphores    = &*renderFinishedSemaphores[imageIndex]};

    graphicsQueue.submit(submitInfo, *inFlightFences[frameIndex]);

    auto &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    const vk::PresentInfoKHR presentInfoKHR{.waitSemaphoreCount = 1,
                                            .pWaitSemaphores    = &*renderFinishedSemaphores[imageIndex],
                                            .swapchainCount     = 1,
                                            .pSwapchains        = &*swapchain,
                                            .pImageIndices      = &imageIndex};
    result = graphicsQueue.presentKHR(presentInfoKHR);

    if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR || frameBufferResized) {
        frameBufferResized = false;
        recreateSwapchain();
    }

    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

}; // namespace Mirael
