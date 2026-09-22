//
// Created by frane on 4/19/2026.
//

#ifndef GPVKFR_MESH_H
#define GPVKFR_MESH_H
#include <vector>

#include "AllocatedBuffer.h"
#include "structs.h"

// refector to take Vertex template instead
class Mesh {
    AllocatedBuffer m_VertexBuffer;
    AllocatedBuffer m_IndexBuffer;
    std::vector<uint32_t> m_Indices;
    std::vector<Vertex> m_Vertices;
    AABB m_AABB;

public:
    Mesh(const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices);
    ~Mesh();

    VkBuffer GetVKVertexBuffer() const { return m_VertexBuffer.GetBuffer(); }
    VkBuffer GetVKIndexBuffer() const { return m_IndexBuffer.GetBuffer(); }
    size_t GetVertexCount() const { return m_Vertices.size(); }
    size_t GetIndexCount() const { return m_Indices.size(); }
    const AABB& GetAABB() const { return m_AABB; }
};


#endif //GPVKFR_MESH_H
