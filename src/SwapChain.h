//
// Created by frane on 5/4/2026.
//

#ifndef GPVKFR_SWAPCHAIN_H
#define GPVKFR_SWAPCHAIN_H

#include <vector>
#include <vulkan/vulkan.h>


#include "GBuffer.h"
#include "structs.h"

class SwapChain : public IGBuffer {

    struct SwapChainAttachments {
        std::vector<VkRenderingAttachmentInfo> swapChainColorAttachments{};
        std::vector<VkFormat> swapChainFormats{};
    };

    VkSwapchainKHR m_SwapChain{};
    std::vector<VkImage> m_SwapChainImages{};
    VkFormat m_SwapChainImageFormat;
    VkExtent2D m_SwapChainExtent{};
    VkSurfaceKHR m_Surface;

    std::vector<SwapChainAttachments> m_SwapChainBuffers{};

    std::vector<VkImageView> m_SwapChainImageViews{};

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const;

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    void CreateImageViews();
public:
    SwapChain(VkSurfaceKHR surface);
    void ReCreateSwapChain();
    void Create();
    void Destroy();
    ~SwapChain() override;

    VkImageView GetImageView(uint32_t imageIndex) const;
    uint32_t GetImageCount() const;

    VkFormat GetImageFormat() const { return m_SwapChainImageFormat; }
    const VkExtent2D& GetExtent() const { return m_SwapChainExtent; }
    VkSwapchainKHR GetSwapChain() const { return m_SwapChain; }
    const std::vector<VkImage>& GetImages() const { return m_SwapChainImages; }
    const std::vector<VkImageView>& GetImageViews() const { return m_SwapChainImageViews; }

    const std::vector<VkRenderingAttachmentInfo> & GetColorAttachmentRenderingInfo() const override;

    const std::vector<VkFormat> & GetFormats() const override;
};


#endif //GPVKFR_SWAPCHAIN_H