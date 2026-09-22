//
// Created by frane on 4/19/2026.
//

#ifndef GPVKFR_TEXTURE2D_H
#define GPVKFR_TEXTURE2D_H

#include <vulkan/vulkan.h>

#include "AllocatedImage.h"

// Immutable 2d texture storage class (transferring ownership of image and view)
// Basically just an encapsulation of AllocatedImage in RAII ish
class Texture2D final {
    AllocatedImage m_Image;

public:
    Texture2D(AllocatedImage&& source);
    ~Texture2D();

    VkImageView GetImageView() const { return m_Image.GetImageView(); }
};


#endif //GPVKFR_TEXTURE2D_H
