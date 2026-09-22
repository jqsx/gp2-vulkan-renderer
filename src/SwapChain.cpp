//
// Created by frane on 5/4/2026.
//

#include "SwapChain.h"

#include "Engine.h"
#include <stdexcept>
#include <algorithm>

void SwapChain::CreateImageViews() {
    VkDevice device = Engine::GetInstance()->GetDevice();
    m_SwapChainBuffers.clear();
    m_SwapChainImageViews.resize(m_SwapChainImages.size());

    for (int index = 0; index < m_SwapChainImages.size(); ++index) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_SwapChainImages[index];

        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_SwapChainImageFormat;

        // Default mapping
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // No mipmapping or multiple layers
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &m_SwapChainImageViews[index]) != VK_SUCCESS) {
            Logger::GetInstance().err("Failed to create image views");
        }
    }

    for (int index = 0; index < m_SwapChainImageViews.size(); ++index) {
        SwapChainAttachments& attachments = m_SwapChainBuffers.emplace_back();

        VkRenderingAttachmentInfo colorAttachmentRenderingInfo = {};
        colorAttachmentRenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachmentRenderingInfo.imageView = m_SwapChainImageViews[index];
        colorAttachmentRenderingInfo.clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

        colorAttachmentRenderingInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentRenderingInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentRenderingInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        attachments.swapChainColorAttachments.emplace_back(std::move(colorAttachmentRenderingInfo));
        attachments.swapChainFormats.emplace_back(m_SwapChainImageFormat);
    }
}

SwapChain::SwapChain(VkSurfaceKHR surface) : m_Surface(surface) {

}

void SwapChain::ReCreateSwapChain() {
    Window* window = Engine::GetInstance()->GetWindow();
    VkDevice device = Engine::GetInstance()->GetDevice();
    int width = 0, height = 0;
    glfwGetFramebufferSize(window->GetWindowHandle(), &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window->GetWindowHandle(), &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    Destroy();
    Create();
}

void SwapChain::Create() {
    VkDevice device = Engine::GetInstance()->GetDevice();
    VkPhysicalDevice physicalDevice = Engine::GetInstance()->GetPhysicalDevice();

    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) {
        throw std::runtime_error("Swapchain support is incomplete");
    }

    if (extent.width == 0 || extent.height == 0) {
        throw std::runtime_error("Swapchain extent is zero");
    }

    VkSwapchainCreateInfoKHR createInfo{};

    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_Surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (!indices.graphicsFamily.has_value() || !indices.presentFamily.has_value()) {
        throw std::runtime_error("Missing required queue families");
    }

    VkBool32 isSupported{0};
    if (vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, indices.presentFamily.value(), m_Surface, &isSupported) != VK_SUCCESS) {
        throw std::runtime_error("Failed to fetch physical device surface support khr");
    }

    if (!isSupported) {
        Logger::GetInstance().err("Physical device doesn't support surface.");
    }

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    const VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
    };

    for (auto flag : compositeAlphaFlags) {
        if (swapChainSupport.capabilities.supportedCompositeAlpha & flag) {
            compositeAlpha = flag;
            break;
        }
    }

    createInfo.compositeAlpha = compositeAlpha;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_SwapChain);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain " + std::to_string(result));
    }

    vkGetSwapchainImagesKHR(device, m_SwapChain, &imageCount, nullptr);
    m_SwapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, m_SwapChain, &imageCount, m_SwapChainImages.data());

    m_SwapChainImageFormat = surfaceFormat.format;
    m_SwapChainExtent = extent;

    CreateImageViews();
}

void SwapChain::Destroy() {
    VkDevice device = Engine::GetInstance()->GetDevice();
    // for (VkFramebuffer framebuffer : m_SwapChainFramebuffers) {
    //     vkDestroyFramebuffer(device, framebuffer, nullptr);
    // }

    for (VkImageView imageView : m_SwapChainImageViews) {
        if (imageView != VK_NULL_HANDLE)
            vkDestroyImageView(device, imageView, nullptr);
    }

    m_SwapChainImageViews.clear();
    m_SwapChainImages.clear();
    m_SwapChainBuffers.clear();

    if (m_SwapChain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(device, m_SwapChain, nullptr);
    m_SwapChain = VK_NULL_HANDLE;
}

SwapChain::~SwapChain() = default;

VkImageView SwapChain::GetImageView(uint32_t imageIndex) const {
    return m_SwapChainImageViews[imageIndex];
}

uint32_t SwapChain::GetImageCount() const {
    return m_SwapChainImages.size();
}

const std::vector<VkRenderingAttachmentInfo> & SwapChain::GetColorAttachmentRenderingInfo() const {
    return m_SwapChainBuffers[Engine::GetInstance()->GetCurrentFrameIndex()].swapChainColorAttachments;
}

const std::vector<VkFormat> & SwapChain::GetFormats() const {
    return m_SwapChainBuffers[0].swapChainFormats;
}

SwapChainSupportDetails SwapChain::querySwapChainSupport(VkPhysicalDevice device) {
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

VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    for (const VkSurfaceFormatKHR& availableFormat : availableFormats) {
        // sRGB automatically converting to linear space [0,1]
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    Logger::GetInstance().err("No desired available format.");

    return availableFormats[0];
}

VkPresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
    for (const VkPresentModeKHR& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    Logger::GetInstance().err("No desired available present mode.");

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {
    Window* window = Engine::GetInstance()->GetWindow();
    if (capabilities.currentExtent.width != UINT32_MAX) { // numeric limits were causing issue
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window->GetWindowHandle(), &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

QueueFamilyIndices SwapChain::findQueueFamilies(VkPhysicalDevice device) {
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
