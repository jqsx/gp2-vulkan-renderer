//
// Created by frane on 4/19/2026.
//

#include "Texture2D.h"

#include <utility>

#include "AllocatedImage.h"

Texture2D::Texture2D(AllocatedImage&& source) : m_Image(std::move(source)) {
}

Texture2D::~Texture2D() {
    m_Image.DeallocateImage();
}
