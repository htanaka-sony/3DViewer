#include "Mesh.h"

CORE_NAMESPACE_BEGIN

DEFINE_META_OBJECT(Mesh)

Mesh::Mesh() : Voxel() {}

Mesh::Mesh(const Mesh& other) : Voxel(other) {}

Mesh::~Mesh() {}

void Mesh::createBoxShape(const Point3f& min_pos, const Point3f& max_pos)
{
    m_vertices.clear();
    m_indices.clear();
    m_segments_indices.clear();

    /// 8頂点（ボックスの各コーナー）
    ///  v0 = (x0, y0, z0)  v1 = (x1, y0, z0)
    ///  v2 = (x1, y1, z0)  v3 = (x0, y1, z0)
    ///  v4 = (x0, y0, z1)  v5 = (x1, y0, z1)
    ///  v6 = (x1, y1, z1)  v7 = (x0, y1, z1)
    const float x0 = min_pos.x(), y0 = min_pos.y(), z0 = min_pos.z();
    const float x1 = max_pos.x(), y1 = max_pos.y(), z1 = max_pos.z();

    m_vertices.emplace_back(x0, y0, z0);    // 0
    m_vertices.emplace_back(x1, y0, z0);    // 1
    m_vertices.emplace_back(x1, y1, z0);    // 2
    m_vertices.emplace_back(x0, y1, z0);    // 3
    m_vertices.emplace_back(x0, y0, z1);    // 4
    m_vertices.emplace_back(x1, y0, z1);    // 5
    m_vertices.emplace_back(x1, y1, z1);    // 6
    m_vertices.emplace_back(x0, y1, z1);    // 7

    /// 面描画用インデックス（6面 × 2三角形 × 3頂点 = 36インデックス）
    /// Z- 面 (法線: Z-)
    m_indices.emplace_back(0);
    m_indices.emplace_back(3);
    m_indices.emplace_back(2);
    m_indices.emplace_back(2);
    m_indices.emplace_back(1);
    m_indices.emplace_back(0);
    /// Z+ 面 (法線: Z+)
    m_indices.emplace_back(4);
    m_indices.emplace_back(5);
    m_indices.emplace_back(6);
    m_indices.emplace_back(6);
    m_indices.emplace_back(7);
    m_indices.emplace_back(4);
    /// X- 面 (法線: X-)
    m_indices.emplace_back(0);
    m_indices.emplace_back(4);
    m_indices.emplace_back(7);
    m_indices.emplace_back(7);
    m_indices.emplace_back(3);
    m_indices.emplace_back(0);
    /// X+ 面 (法線: X+)
    m_indices.emplace_back(1);
    m_indices.emplace_back(2);
    m_indices.emplace_back(6);
    m_indices.emplace_back(6);
    m_indices.emplace_back(5);
    m_indices.emplace_back(1);
    /// Y- 面 (法線: Y-)
    m_indices.emplace_back(0);
    m_indices.emplace_back(1);
    m_indices.emplace_back(5);
    m_indices.emplace_back(5);
    m_indices.emplace_back(4);
    m_indices.emplace_back(0);
    /// Y+ 面 (法線: Y+)
    m_indices.emplace_back(3);
    m_indices.emplace_back(7);
    m_indices.emplace_back(6);
    m_indices.emplace_back(6);
    m_indices.emplace_back(2);
    m_indices.emplace_back(3);

    /// 線描画用インデックス（12辺 × 2頂点 = 24インデックス）
    /// 底面 (Z-)
    m_segments_indices.emplace_back(0);
    m_segments_indices.emplace_back(1);
    m_segments_indices.emplace_back(1);
    m_segments_indices.emplace_back(2);
    m_segments_indices.emplace_back(2);
    m_segments_indices.emplace_back(3);
    m_segments_indices.emplace_back(3);
    m_segments_indices.emplace_back(0);
    /// 上面 (Z+)
    m_segments_indices.emplace_back(4);
    m_segments_indices.emplace_back(5);
    m_segments_indices.emplace_back(5);
    m_segments_indices.emplace_back(6);
    m_segments_indices.emplace_back(6);
    m_segments_indices.emplace_back(7);
    m_segments_indices.emplace_back(7);
    m_segments_indices.emplace_back(4);
    /// 縦辺 (Z方向)
    m_segments_indices.emplace_back(0);
    m_segments_indices.emplace_back(4);
    m_segments_indices.emplace_back(1);
    m_segments_indices.emplace_back(5);
    m_segments_indices.emplace_back(2);
    m_segments_indices.emplace_back(6);
    m_segments_indices.emplace_back(3);
    m_segments_indices.emplace_back(7);

    markRenderDirty();
    markBoxDirty();
}

CORE_NAMESPACE_END
