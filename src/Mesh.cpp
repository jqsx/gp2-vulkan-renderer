//
// Created by frane on 4/19/2026.
//

#include "Mesh.h"

#include <stdexcept>

#include "Logger.h"
#include <vk_mem_alloc.h>

Mesh::Mesh(const std::vector<uint32_t> &indices, const std::vector<Vertex> &vertices) : m_Indices(indices), m_Vertices(vertices) {
    for (const Vertex& vertex : m_Vertices) {
        m_AABB.Expand(vertex.position);
    }

    m_VertexBuffer.AllocateGPUBuffer(sizeof(Vertex) * vertices.size(), (void*)vertices.data());
    m_IndexBuffer.AllocateGPUBuffer(sizeof(uint32_t) * indices.size(), (void*)indices.data(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

Mesh::~Mesh() {
    m_VertexBuffer.DeallocateBuffer();
    m_IndexBuffer.DeallocateBuffer();

    m_Vertices.clear();
    m_Indices.clear();
}
