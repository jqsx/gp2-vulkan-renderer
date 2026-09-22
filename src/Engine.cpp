//
// Created by frane on 2/13/2026.
//

#include "Engine.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <vector>

#include "Logger.h"
#include "Window.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "Camera.h"
#include "ResourceLoader.h"
#include "ResourceManager.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

Engine* Engine::Instance{nullptr};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    // std::cerr << pCallbackData->pMessage << std::endl;
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << pCallbackData->pMessage << std::endl;
    }

    return VK_FALSE;
}

InitResult Engine::initialize_vkDebug() {
    if (!m_EnableValidationLayers) return INIT_SUCCESS;

    Logger::GetInstance().log("Validation layers have been enabled.");

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;

    if (CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS) {
        Logger::GetInstance().err("Failed to set up debug messenger.");
        return INIT_FAIL;
    }
    return INIT_SUCCESS;
}

InitResult Engine::initialize_vkInstance() {
    if (m_EnableValidationLayers && initialize_vkValidationLayers() == INIT_FAIL) {
        Logger::GetInstance().err("Failed to initialize Validation Layers");
        return INIT_FAIL;
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "GP2 VK Franciszek Rakowiecki";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Bonkers";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};

    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = 0;

    std::vector<const char*> extensions = getRequiredExtensions();

    unsigned int glfwExtensionCount = (unsigned int)extensions.size();
    const char** glfwExtensions = extensions.data();

    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (m_EnableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
        createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

        populateCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    VkResult vresult = vkCreateInstance(&createInfo, nullptr, &m_Instance);
    if (vresult != VK_SUCCESS) {
        Logger::GetInstance().err("Failed during vkCreateInstance.");

        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

        Logger::GetInstance().log("Available Extensions");
        for (const VkExtensionProperties& extension : availableExtensions) {
            Logger::GetInstance().log(extension.extensionName);
        }

        return INIT_FAIL;
    }

    return INIT_SUCCESS;
}

InitResult Engine::initialize_vkValidationLayers() {

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : m_ValidationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (std::strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return INIT_FAIL;
        }
    }

    return INIT_SUCCESS;
}

InitResult Engine::initialize_vkFindPhysicalDevice() {
    m_PhysicalDevice = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        Logger::GetInstance().err("Failed to find valid physical devices");
        return INIT_FAIL;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

    for (const VkPhysicalDevice& device : devices) {
        VkPhysicalDeviceProperties deviceProperties{};
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        if (isDeviceSuitable(device)) {
            m_PhysicalDevice = device;
            std::cout << deviceProperties.deviceName << std::endl;
            break;
        }
    }

    if (m_PhysicalDevice == VK_NULL_HANDLE) {
        Logger::GetInstance().err("Failed to find a suitable physical device");
        return INIT_FAIL;
    }

    return INIT_SUCCESS;
}

InitResult Engine::initialize_vkLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(m_PhysicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceDynamicRenderingFeatures dynRenderingFeatures{};
    dynRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynRenderingFeatures.dynamicRendering = true;

    VkPhysicalDeviceFeatures2 deviceFeatures{};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    deviceFeatures.pNext = &dynRenderingFeatures;

    vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &deviceFeatures);

    if (!dynRenderingFeatures.dynamicRendering) {
        Logger::GetInstance().err("Device doesn't support dynamic rendering");
        return INIT_FAIL;
    }

    if (!deviceFeatures.features.samplerAnisotropy) {
        Logger::GetInstance().err("Anisotropy not supported");
        return INIT_FAIL;
    }

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.pNext = &deviceFeatures;
    createInfo.pEnabledFeatures = nullptr;

    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

    if (m_EnableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
        createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) != VK_SUCCESS) {
        Logger::GetInstance().err("Failed to create logical device");
        return INIT_FAIL;
    }

    vkGetDeviceQueue(m_Device, indices.graphicsFamily.value(), 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, indices.presentFamily.value(), 0, &m_PresentationQueue);

    return INIT_SUCCESS;
}

InitResult Engine::initialize_vkSurface() {
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = glfwGetWin32Window(m_Window->GetWindowHandle());
    createInfo.hinstance = GetModuleHandle(nullptr);

    if (vkCreateWin32SurfaceKHR(m_Instance, &createInfo, nullptr, &m_Surface) != VK_SUCCESS) {
        Logger::GetInstance().err("Failed to create window surface");
        return INIT_FAIL;
    }

    return INIT_SUCCESS;
}

InitResult Engine::initialize_vkSwapChain() {
    if (m_SwapChain == nullptr) {
        m_SwapChain = std::make_unique<SwapChain>(m_Surface);
        m_SwapChain->Create();
        return INIT_SUCCESS;
    }

    m_SwapChain->ReCreateSwapChain();

    return INIT_SUCCESS;
}

InitResult Engine::initialize_vkCommandPool() {
    QueueFamilyIndices indices = findQueueFamilies(m_PhysicalDevice);

    m_GraphicsCommandPool.SetQueue(m_GraphicsQueue, indices.graphicsFamily.value());
    if (m_GraphicsCommandPool.Create() != INIT_SUCCESS) {
        Logger::GetInstance().err("Failed to create command pool.");
        return INIT_FAIL;
    }

    m_AllocationCommandPool.SetQueue(m_GraphicsQueue, indices.graphicsFamily.value());
    if (m_AllocationCommandPool.Create() != INIT_SUCCESS) {
        Logger::GetInstance().err("Failed to create command pool.");
        return INIT_FAIL;
    }

    return INIT_SUCCESS;
}

InitResult Engine::initialize_vkCommandBuffer() {
    m_GraphicsCommandPool.AllocateBuffers(MAX_FRAMES_IN_FLIGHT);
    return INIT_SUCCESS;
}

InitResult Engine::initialize_vkSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_RenderFinishedSemaphores.resize(m_SwapChain->GetImageCount());
    m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    for (int index = 0; index < MAX_FRAMES_IN_FLIGHT; ++index) {
        if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[index]) != VK_SUCCESS) {
            Logger::GetInstance().err("Failed to initialize image available semaphore.");
            return INIT_FAIL;
        }
        if (vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[index]) != VK_SUCCESS) {
            Logger::GetInstance().err("Failed to initialize in-flight fence.");
            return INIT_FAIL;
        }
    }

    for (int index = 0; index < m_SwapChain->GetImageCount(); ++index) {
        if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[index]) != VK_SUCCESS) {
            Logger::GetInstance().err("Failed to initialize render finished semaphore.");
            return INIT_FAIL;
        }
    }

    return INIT_SUCCESS;
}

InitResult Engine::initialize_vkAllocator() {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = m_PhysicalDevice;
    allocatorInfo.device = m_Device;
    allocatorInfo.instance = m_Instance;
    if (vmaCreateAllocator(&allocatorInfo, &m_Allocator) != VK_SUCCESS) {
        Logger::GetInstance().err("Failed to create vma allocator.");
        return INIT_FAIL;
    }
    return INIT_SUCCESS;
}

InitResult Engine::initialize_depthBuffer() {
    m_DepthBuffer.InitDefaults();
    m_DepthBuffer.Create();
    return INIT_SUCCESS;
}

std::vector<const char*> Engine::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (m_EnableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool Engine::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

SwapChainSupportDetails Engine::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.formats.data());
    }
    else {
        Logger::GetInstance().err("No format available.");
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, details.presentModes.data());
    }
    else {
        Logger::GetInstance().err("No presentation mode available.");
    }

    return details;
}

VkSurfaceFormatKHR Engine::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const VkSurfaceFormatKHR& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    Logger::GetInstance().err("No desired available format.");

    return availableFormats[0];
}

VkPresentModeKHR Engine::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const VkPresentModeKHR& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    Logger::GetInstance().err("No desired available present mode.");

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Engine::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(m_Window->GetWindowHandle(), &width, &height);

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

std::vector<char> Engine::readFile(const std::string &path) {
    std::ifstream file(path, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file " + path);
    }

    uintmax_t size = std::filesystem::file_size(path);

    std::vector<char> buffer;

    buffer.resize(size);

    file.read(buffer.data(), size);

    file.close();
    //
    // for (char c : buffer) {
    //     std::cout << c;
    // }
    // std::cout << std::endl;

    return buffer;
}

VkShaderModule Engine::createShaderModule(const std::vector<char> &code) {
    VkShaderModuleCreateInfo createInfo = {};

    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_Device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module.");
    }

    return shaderModule;
}

QueueFamilyIndices Engine::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const VkQueueFamilyProperties& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete())
            break;

        i++;
    }

    return indices;
}

VkResult Engine::CreateDebugUtilsMessengerEXT(VkInstance instance,
                                              const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                                              VkDebugUtilsMessengerEXT*pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void Engine::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                           const VkAllocationCallbacks *pAllocator)  {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void Engine::populateCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;
}

double Engine::getTimeSinceEpoch() {
    return std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

void Engine::cleanupSwapChain() {
    // for (VkFramebuffer framebuffer : m_SwapChainFramebuffers) {
    //     vkDestroyFramebuffer(m_Device, framebuffer, nullptr);
    // }
    //
    // for (VkImageView imageView : m_SwapChainImageViews) {
    //     vkDestroyImageView(m_Device, imageView, nullptr);
    // }
    //
    // vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
    m_SwapChain->Destroy();
}

void Engine::recreateSwapChain() {

    m_SwapChain->ReCreateSwapChain();

    for (VkSemaphore semaphore : m_RenderFinishedSemaphores) {
        vkDestroySemaphore(m_Device, semaphore, nullptr);
    }
    m_RenderFinishedSemaphores.clear();

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    m_RenderFinishedSemaphores.resize(m_SwapChain->GetImageCount());
    for (VkSemaphore& semaphore : m_RenderFinishedSemaphores) {
        if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS) {
            throw std::runtime_error("Failed to recreate render finished semaphore.");
        }
    }

    m_DepthBuffer.Destroy();
    initialize_depthBuffer();
    if (m_Renderer != nullptr) {
        m_Renderer->RecreateFrameResources();
    }
    // if (initialize_vkFrameBuffers() == INIT_FAIL) {
    //     throw std::runtime_error("Failed to recreate frame buffers");
    // }
}

bool Engine::isDeviceSuitable(VkPhysicalDevice device) {
    // Locating queues
    QueueFamilyIndices indices = findQueueFamilies(device);

    // Swap chain
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        bool hasDesiredPresentMode = false;
        for (auto & presentMode : swapChainSupport.presentModes) {
            if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                hasDesiredPresentMode = true;
            }
        }
        swapChainAdequate = swapChainAdequate && hasDesiredPresentMode;
    }

    // Find an actual gpu and all the required queues along with the swapchain support
    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

void Engine::initialize_engineAssets() {
    m_Scene = std::make_unique<Scene>();

    m_ResourceLoader = std::make_unique<ResourceLoader>(m_Device);
    m_ResourceManager = std::make_unique<ResourceManager>();

    m_FallbackTexture = m_ResourceLoader->LoadImage2D("resources/images/FallbackTexture.png");
    m_Renderer = std::make_unique<Renderer>();
}

void Engine::renderFrame() {
    vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex{0};
    VkResult queueResult = vkAcquireNextImageKHR(m_Device, m_SwapChain->GetSwapChain(), UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);

    if (queueResult == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (queueResult != VK_SUCCESS && queueResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);

    CommandPool& graphicsCommandPool = GetGraphicsCMD();
    graphicsCommandPool.ResetBuffer(m_CurrentFrame);

    VkCommandBuffer commandBuffer = graphicsCommandPool.GetBuffer(m_CurrentFrame);
    m_Renderer->RenderFrame(imageIndex, commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {m_ImageAvailableSemaphores[m_CurrentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkSemaphore signalSemaphores[] = {m_RenderFinishedSemaphores[imageIndex]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit to graphics queue");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {m_SwapChain->GetSwapChain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr;

    VkResult presentResult = vkQueuePresentKHR(m_PresentationQueue, &presentInfo);

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || m_FrameBufferResized) {
        m_FrameBufferResized = false;
        recreateSwapChain();
    } else if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Engine::OnFrameBufferResize(int width, int height) {
    m_FrameBufferResized = true;
}

Engine::Engine(const EngineConfig& config) : m_Config(config) {
    Instance = this;
    InitResult result{};

    m_Window = new Window(result, config, this);

    if (result != INIT_SUCCESS) {
        Logger::GetInstance().err("Failed during window initialization.");
        throw std::runtime_error("FAILED WINDOW INIT");
    }

    if (initialize_vkInstance() != INIT_SUCCESS) {
        Logger::GetInstance().err("Failed during vk instance initialization.");
        throw std::runtime_error("FAILED VKINSTANCE INIT");
    }

    if (initialize_vkDebug() != INIT_SUCCESS) {
        Logger::GetInstance().err("Failed during vk debug initialization.");
        throw std::runtime_error("FAILED VKDEBUG INIT");
    }

    if (initialize_vkSurface() != INIT_SUCCESS) {
        Logger::GetInstance().err("Failed during vk surface initialization.");
        return;
    }

    if (initialize_vkFindPhysicalDevice() != INIT_SUCCESS) {
        Logger::GetInstance().err("Failed during vkFindPhysicalDevice initialization.");
        throw std::runtime_error("FAILED VK INIT");
    }

    if (initialize_vkLogicalDevice() != INIT_SUCCESS) {
        Logger::GetInstance().err("Failed during vkLogicalDevice initialization.");
        throw std::runtime_error("FAILED VKLOGICALDEVICE INIT");
    }

    if (initialize_vkAllocator() != INIT_SUCCESS) {
        Logger::GetInstance().err("Failed during vkAllocator initialization.");
        throw std::runtime_error("FAILED VKALLOC INIT");
    }

    if (initialize_vkSwapChain() != INIT_SUCCESS) {
        Logger::GetInstance().err("Failed during vkSwapChain initialization.");
        throw std::runtime_error("FAILED VKSWAPCHAIN INIT");
    }

    // if (initialize_vkImageViews() != INIT_SUCCESS) {
    //     Logger::GetInstance().err("Failed during vkImageViews initialization.");
    //     throw std::runtime_error("FAILED VKIMAGEVIEWS INIT");
    // }

    if (initialize_vkCommandPool() != INIT_SUCCESS) {
        Logger::GetInstance().err("Failed during vkCommandPool initialization.");
        throw std::runtime_error("FAILED VKCOMMANDPOOL INIT");
    }

    initialize_depthBuffer();

    // if (initialize_vkFrameBuffers() != INIT_SUCCESS) {
    //     Logger::GetInstance().err("Failed during vkFrameBuffers initialization.");
    //     throw std::runtime_error("FAILED VKFRAMEBUFFER INIT");
    // }

    if (initialize_vkCommandBuffer() != INIT_SUCCESS) {
        Logger::GetInstance().err("Failed during vkCommandBuffer initialization.");
        throw std::runtime_error("FAILED VKCOMMANDBUFFER INIT");
    }

    if (initialize_vkSyncObjects() != INIT_SUCCESS) {
        Logger::GetInstance().err("Failed during vkSyncObjects initialization.");
        throw std::runtime_error("FAILED VKSYNC INIT");
    }

    initialize_engineAssets();

    m_IsInitialized = true;
}

Engine::~Engine() {
    for (int index = 0; index < MAX_FRAMES_IN_FLIGHT; ++index) {
        vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[index], nullptr);
        vkDestroyFence(m_Device, m_InFlightFences[index], nullptr);
    }
    for (VkSemaphore semaphore : m_RenderFinishedSemaphores) {
        vkDestroySemaphore(m_Device, semaphore, nullptr);
    }

    m_UploadPool.Dispose();

    m_Renderer.reset();

    m_GraphicsCommandPool.Destroy();
    m_AllocationCommandPool.Destroy();

    // for (VkFramebuffer frameBuffer : m_SwapChainFramebuffers) {
    //     vkDestroyFramebuffer(m_Device, frameBuffer, nullptr);
    // }
    m_DepthBuffer.Destroy();

    m_Scene.reset(); // just delete it here to make sure
    // idk feels safer that way
    m_ResourceManager.reset();
    m_ResourceLoader.reset();
    // It also makes sure that all the assets here are getting disposed in this order

    vmaDestroyAllocator(m_Allocator);

    // for (VkImageView imageView : m_SwapChainImageViews) {
    //     vkDestroyImageView(m_Device, imageView, nullptr);
    // }

    // if (m_Device != VK_NULL_HANDLE && m_SwapChain != VK_NULL_HANDLE)
    //     vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
    m_SwapChain->Destroy();

    if (m_Device != VK_NULL_HANDLE)
        vkDestroyDevice(m_Device, nullptr);
    if (m_Instance != VK_NULL_HANDLE && m_Surface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

    if (m_EnableValidationLayers)
        DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);

    if (m_Instance != VK_NULL_HANDLE)
        vkDestroyInstance(m_Instance, nullptr);

    if (m_Window) {
        delete m_Window;
        m_Window = nullptr;
    }
}

void Engine::Begin() {
    if (m_IsInitialized) {
        double start = getTimeSinceEpoch();
        double lastFrame = getTimeSinceEpoch();
        double lastDebug = getTimeSinceEpoch();
        while (!glfwWindowShouldClose(m_Window->GetWindowHandle())) {
            double deltaTime = getTimeSinceEpoch() - lastFrame;
            lastFrame = getTimeSinceEpoch();

            m_Time = float(getTimeSinceEpoch() - start);

            glfwPollEvents();

            Camera::GetInstance().Update(deltaTime);
            Camera::GetInstance().RecalculateProjectionMatrix();

            m_UploadPool.SubmitBatch();
            m_UploadPool.Update();

            renderFrame();
            if (lastDebug < getTimeSinceEpoch()) {
                std::cout << "dFPS " << int(floor(1.0 / deltaTime)) << "\n";

                lastDebug = getTimeSinceEpoch() + 0.5;
            }
        }
        vkDeviceWaitIdle(m_Device);
    }
}
