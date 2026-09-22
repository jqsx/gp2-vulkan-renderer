//
// Created by frane on 2/13/2026.
//
#pragma once

#ifndef ENGINE_H
#define ENGINE_H

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "AllocatedBuffer.h"
#include "AllocatedImage.h"
#include "UploadPool.h"
#include "CommandPool.h"
#include "DepthBuffer.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "structs.h"
#include "SwapChain.h"
#include "vk_mem_alloc.h"
#include "Window.h"

// class VkInstance;

class ResourceLoader;

class Engine final {
private:
    static Engine* Instance;

    const bool m_EnableValidationLayers = true;

    std::unique_ptr<Scene> m_Scene;

    std::unique_ptr<ResourceLoader> m_ResourceLoader;
    std::unique_ptr<ResourceManager> m_ResourceManager;

    Texture2D* m_FallbackTexture;

    float m_Time{0.0f};

#pragma region Params
    bool m_IsInitialized = false;

    Window* m_Window;
    VkInstance m_Instance;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device;

    VkQueue m_GraphicsQueue;
    VkQueue m_PresentationQueue;

    VkSurfaceKHR m_Surface;

    std::unique_ptr<SwapChain> m_SwapChain;
    std::unique_ptr<Renderer> m_Renderer;

    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<VkFence> m_InFlightFences;

    DepthBuffer m_DepthBuffer;

    VkDebugUtilsMessengerEXT m_DebugMessenger;

    CommandPool m_GraphicsCommandPool;
    CommandPool m_AllocationCommandPool;

    UploadPool m_UploadPool;

    VmaAllocator m_Allocator;

    EngineConfig m_Config;

    const std::vector<const char*> m_ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> m_DeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    }; // Synchronization 2

    const int MAX_FRAMES_IN_FLIGHT = 2;
    int m_CurrentFrame = 0;
    bool m_FrameBufferResized = false;

#pragma endregion Params

#pragma region vkInit
    InitResult initialize_vkDebug();
    InitResult initialize_vkInstance();
    InitResult initialize_vkValidationLayers();
    InitResult initialize_vkFindPhysicalDevice();
    InitResult initialize_vkLogicalDevice();
    InitResult initialize_vkSurface();
    InitResult initialize_vkSwapChain();
    InitResult initialize_vkCommandPool();
    InitResult initialize_vkCommandBuffer();
    InitResult initialize_vkSyncObjects();
    InitResult initialize_vkAllocator();
    InitResult initialize_depthBuffer();

#pragma endregion vkInit

#pragma region utils

    std::vector<const char*> getRequiredExtensions();

    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

    static std::vector<char> readFile(const std::string& path);
    VkShaderModule createShaderModule(const std::vector<char>& code);

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

    void populateCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    double getTimeSinceEpoch();

    // The swap chain will not be used outside the engine class, requires private members of engine
    // therefore from my pov it's better to keep it as a function here rather than creating a detailed solution to deal with it
    void cleanupSwapChain();
    void recreateSwapChain();

    bool isDeviceSuitable(VkPhysicalDevice device);
#pragma endregion utils

    void initialize_engineAssets();

    void renderFrame();

    void OnFrameBufferResize(int width, int height);

public:
    Engine(const EngineConfig& config);
    ~Engine();

    Engine(const Engine& engine) = delete;
    Engine& operator=(const Engine& engine) = delete;
    Engine& operator=(Engine&& engine) = delete;
    Engine(Engine&& engine) = delete;

    static Engine* GetInstance() { return Instance; }

    VmaAllocator GetAllocator() const { return m_Allocator; }
    VkDevice GetDevice() const { return m_Device; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
    VkFormat GetSwapChainFormat() const { return m_SwapChain->GetImageFormat(); }
    VkExtent2D GetSwapChainExtent() const { return m_SwapChain->GetExtent(); }
    SwapChain* GetSwapChain() const { return m_SwapChain.get(); }
    DepthBuffer& GetDepthBuffer() { return m_DepthBuffer; }
    uint32_t GetMaxFramesInFlight() const { return MAX_FRAMES_IN_FLIGHT; }
    Texture2D* GetFallbackTexture() const { return m_FallbackTexture; }
    uint32_t GetCurrentFrameIndex() const { return m_CurrentFrame; }
    Renderer& GetRenderer() const { return *m_Renderer; }
    UploadPool& GetUploadPool() { return m_UploadPool; }
    CommandPool& GetGraphicsCMD() { return m_GraphicsCommandPool; }
    CommandPool& GetAllocCMD() { return m_AllocationCommandPool; }

    ResourceLoader& GetResourceLoader() const { return *m_ResourceLoader; }
    ResourceManager& GetResourceManager() const { return *m_ResourceManager; }

    Scene* GetScene() { return m_Scene.get(); }

    Window* GetWindow() const { return m_Window; }

    void Begin();

    friend void Window::OnFrameBufferSizeChanged(int, int);
};



#endif //ENGINE_H
