#include "RenderNormalMesh.h"

#include <algorithm>
#include <cmath>

#undef min
#undef max

CORE_NAMESPACE_BEGIN

DEFINE_META_RENDERABLE(RenderNormalMesh)

RenderNormalMesh::RenderNormalMesh() {}

RenderNormalMesh::~RenderNormalMesh() {}

RenderNormalMesh::RenderNormalMesh(const RenderNormalMesh& other)
{
    m_vertices            = other.m_vertices;
    m_indices             = other.m_indices;
    m_segments_indices    = other.m_segments_indices;
    m_create_section_line = other.m_create_section_line;
}

void RenderNormalMesh::createBoxRound(const BoundingBox3f& box, float radius, float ratioX, float ratioY, float ratioZ,
                                      float tol)
{
    /*
    m_vertices.clear();
    m_indices.clear();
    m_segments_indices.clear();
    /// geom_sphereOctant_type1 と同じロジック: ratioMax で正規化した各軸の丸め半径
    const float   ratioMax = std::max({ratioX, ratioY, ratioZ});
    float         rx       = radius * ratioX / ratioMax;
    float         ry       = radius * ratioY / ratioMax;
    float         rz       = radius * ratioZ / ratioMax;
    const Point3f mn       = box.min_pos();
    const Point3f mx       = box.max_pos();
    const float   W        = mx.x() - mn.x();
    const float   H        = mx.y() - mn.y();
    const float   D        = mx.z() - mn.z();
    /// 丸め半径をボックス半分でクランプ
    rx             = std::min(rx, W * 0.5f);
    ry             = std::min(ry, H * 0.5f);
    rz             = std::min(rz, D * 0.5f);
    const float x0 = mn.x(), y0 = mn.y(), z0 = mn.z();
    /// 4頂点のquadを追加（外向き法線になるよう頂点順を指定）
    /// Triangle1: p0,p1,p2  Triangle2: p2,p3,p0
    auto addQuad = [&](const Point3f& p0, const Point3f& p1, const Point3f& p2, const Point3f& p3) {
        const unsigned int base = (unsigned int)m_vertices.size();
        m_vertices.push_back(p0);
        m_vertices.push_back(p1);
        m_vertices.push_back(p2);
        m_vertices.push_back(p3);
        m_indices.push_back(base + 0);
        m_indices.push_back(base + 1);
        m_indices.push_back(base + 2);
        m_indices.push_back(base + 2);
        m_indices.push_back(base + 3);
        m_indices.push_back(base + 0);
    };

    /// コーナーあたりの弧分割数を許容誤差から計算 (chord error = r*(1-cos(π/N)) ≤ tol)
    float r_ref = std::max(rx, std::max(ry, rz));
    int   segs  = 8;
    if (tol > 0.0f && r_ref > 0.0f) {
        segs = std::max(segs, (int)std::ceil((float)M_PI / std::sqrt(2.0f * tol / r_ref)));
    }

    // ---- 6平面フェース ----
    /// Z- 面 (法線: -Z)
    addQuad({x0 + rx, y0 + ry, z0}, {x0 + rx, y0 + H - ry, z0}, {x0 + W - rx, y0 + H - ry, z0},
            {x0 + W - rx, y0 + ry, z0});
    /// Z+ 面 (法線: +Z)
    addQuad({x0 + rx, y0 + ry, z0 + D}, {x0 + W - rx, y0 + ry, z0 + D}, {x0 + W - rx, y0 + H - ry, z0 + D},
            {x0 + rx, y0 + H - ry, z0 + D});
    /// X- 面 (法線: -X)
    addQuad({x0, y0 + ry, z0 + rz}, {x0, y0 + ry, z0 + D - rz}, {x0, y0 + H - ry, z0 + D - rz},
            {x0, y0 + H - ry, z0 + rz});
    /// X+ 面 (法線: +X)
    addQuad({x0 + W, y0 + ry, z0 + rz}, {x0 + W, y0 + H - ry, z0 + rz}, {x0 + W, y0 + H - ry, z0 + D - rz},
            {x0 + W, y0 + ry, z0 + D - rz});
    /// Y- 面 (法線: -Y)
    addQuad({x0 + rx, y0, z0 + rz}, {x0 + W - rx, y0, z0 + rz}, {x0 + W - rx, y0, z0 + D - rz},
            {x0 + rx, y0, z0 + D - rz});
    /// Y+ 面 (法線: +Y)
    addQuad({x0 + rx, y0 + H, z0 + rz}, {x0 + rx, y0 + H, z0 + D - rz}, {x0 + W - rx, y0 + H, z0 + D - rz},
            {x0 + W - rx, y0 + H, z0 + rz});

    // ---- 12辺の円弧ストリップ ----
    /// Z平行辺 (4本): XYコーナーに沿ってZ方向のストリップ
    /// P(v, z) = (cx + dx*rx*cos(v), cy + dy*ry*sin(v), z)
    for (int sx = 0; sx < 2; sx++) {
        for (int sy = 0; sy < 2; sy++) {
            const float cx     = x0 + (sx ? W - rx : rx);
            const float cy     = y0 + (sy ? H - ry : ry);
            const float dx_sgn = sx ? +1.0f : -1.0f;
            const float dy_sgn = sy ? +1.0f : -1.0f;
            // ここで符号の積で周回方向を判定
            const bool flip = (dx_sgn * dy_sgn < 0);
            for (int k = 0; k < segs; k++) {
                const float v0r = (float)k * (float)M_PI_2 / segs;
                const float v1r = (float)(k + 1) * (float)M_PI_2 / segs;
                const float px0 = cx + dx_sgn * rx * std::cos(v0r);
                const float py0 = cy + dy_sgn * ry * std::sin(v0r);
                const float px1 = cx + dx_sgn * rx * std::cos(v1r);
                const float py1 = cy + dy_sgn * ry * std::sin(v1r);
                if (flip) {
                    // 反時計回りになるように順番を反転
                    addQuad({px0, py0, z0 + rz}, {px0, py0, z0 + D - rz}, {px1, py1, z0 + D - rz}, {px1, py1, z0 + rz});
                }
                else {
                    addQuad({px0, py0, z0 + rz}, {px1, py1, z0 + rz}, {px1, py1, z0 + D - rz}, {px0, py0, z0 + D - rz});
                }
            }
        }
    }

    /// Y平行辺 (4本): XZコーナーに沿ってY方向のストリップ
    /// P(u, y) = (cx + dx*rx*cos(u), y, cz + dz*rz*sin(u))
    for (int sx = 0; sx < 2; sx++) {
        for (int sz = 0; sz < 2; sz++) {
            const float cx     = x0 + (sx ? W - rx : rx);
            const float cz     = z0 + (sz ? D - rz : rz);
            const float dx_sgn = sx ? +1.0f : -1.0f;
            const float dz_sgn = sz ? +1.0f : -1.0f;
            const bool  flip   = (dx_sgn * dz_sgn < 0);
            for (int k = 0; k < segs; k++) {
                const float u0r = (float)k * (float)M_PI_2 / segs;
                const float u1r = (float)(k + 1) * (float)M_PI_2 / segs;
                const float px0 = cx + dx_sgn * rx * std::cos(u0r);
                const float pz0 = cz + dz_sgn * rz * std::sin(u0r);
                const float px1 = cx + dx_sgn * rx * std::cos(u1r);
                const float pz1 = cz + dz_sgn * rz * std::sin(u1r);
                if (flip) {
                    addQuad({px0, y0 + ry, pz0}, {px1, y0 + ry, pz1}, {px1, y0 + H - ry, pz1}, {px0, y0 + H - ry, pz0});
                }
                else {
                    addQuad({px0, y0 + ry, pz0}, {px0, y0 + H - ry, pz0}, {px1, y0 + H - ry, pz1}, {px1, y0 + ry, pz1});
                }
            }
        }
    }
    /// X平行辺 (4本): YZコーナーに沿ってX方向のストリップ
    /// P(v, x) = (x, cy + dy*ry*cos(v), cz + dz*rz*sin(v))
    for (int sy = 0; sy < 2; sy++) {
        for (int sz = 0; sz < 2; sz++) {
            const float cy     = y0 + (sy ? H - ry : ry);
            const float cz     = z0 + (sz ? D - rz : rz);
            const float dy_sgn = sy ? +1.0f : -1.0f;
            const float dz_sgn = sz ? +1.0f : -1.0f;
            const bool  flip   = (dy_sgn * dz_sgn < 0);
            for (int k = 0; k < segs; k++) {
                const float v0r = (float)k * (float)M_PI_2 / segs;
                const float v1r = (float)(k + 1) * (float)M_PI_2 / segs;
                const float py0 = cy + dy_sgn * ry * std::cos(v0r);
                const float pz0 = cz + dz_sgn * rz * std::sin(v0r);
                const float py1 = cy + dy_sgn * ry * std::cos(v1r);
                const float pz1 = cz + dz_sgn * rz * std::sin(v1r);
                if (flip) {
                    addQuad({x0 + rx, py0, pz0}, {x0 + W - rx, py0, pz0}, {x0 + W - rx, py1, pz1}, {x0 + rx, py1, pz1});
                }
                else {
                    addQuad({x0 + rx, py0, pz0}, {x0 + rx, py1, pz1}, {x0 + W - rx, py1, pz1}, {x0 + W - rx, py0, pz0});
                }
            }
        }
    }

    // ---- 8コーナー (楕円球面の1/8パッチ) ----
    /// geom_sphereOctant_type1 の球面をポリゴンで表現
    /// P(u,v) = (cx + dx*rx*sin(u)*cos(v),
    ///           cy + dy*ry*sin(u)*sin(v),
    ///           cz + dz*rz*cos(u))
    for (int sx = 0; sx < 2; sx++) {
        for (int sy = 0; sy < 2; sy++) {
            for (int sz = 0; sz < 2; sz++) {
                const float cx     = x0 + (sx ? W - rx : rx);
                const float cy     = y0 + (sy ? H - ry : ry);
                const float cz     = z0 + (sz ? D - rz : rz);
                const float dx_sgn = sx ? +1.0f : -1.0f;
                const float dy_sgn = sy ? +1.0f : -1.0f;
                const float dz_sgn = sz ? +1.0f : -1.0f;
                auto        spt    = [&](int ui, int vi) -> Point3f {
                    const float u = (float)ui * (float)M_PI_2 / segs;
                    const float v = (float)vi * (float)M_PI_2 / segs;
                    return {cx + dx_sgn * rx * std::sin(u) * std::cos(v), cy + dy_sgn * ry * std::sin(u) * std::sin(v),
                            cz + dz_sgn * rz * std::cos(u)};
                };
                /// 符号の積 dx*dy*dz が負（マイナスの個数が奇数）のとき
                /// d/du×d/dv が内向きになるので巻き順を反転して外向き法線を保つ
                const bool flip = ((sx + sy + sz) % 2 == 0);
                for (int ui = 0; ui < segs; ui++) {
                    for (int vi = 0; vi < segs; vi++) {
                        const Point3f p00 = spt(ui, vi);
                        const Point3f p10 = spt(ui + 1, vi);
                        const Point3f p11 = spt(ui + 1, vi + 1);
                        const Point3f p01 = spt(ui, vi + 1);
                        if (flip) {
                            addQuad(p00, p01, p11, p10);
                        }
                        else {
                            addQuad(p00, p10, p11, p01);
                        }
                    }
                }
            }
        }
    }
    markRenderDirty();
    markBoxDirty();
    */
}

void RenderNormalMesh::updateBoundingBox()
{
    m_bbox.init();
    for (const auto& vertex : m_vertices) {
        m_bbox.expandBy(vertex.m_position);
    }
    resetBoxDirty();
}

BoundingBox3f RenderNormalMesh::calculateBoundingBox(const Matrix4x4f& parent_matrix, bool /*only_visible*/,
                                                     bool /*including_text*/) const
{
    BoundingBox3f bbox;
    for (const auto& vertex : m_vertices) {
        bbox.expandBy(parent_matrix * vertex.m_position);
    }
    return bbox;
}

void RenderNormalMesh::appendQuad(const Point3f& p0, const Point3f& p1, const Point3f& p2, const Point3f& p3,
                                  const Point3f& norm, int get_seg_flg)
{
    unsigned int size = (unsigned int)m_vertices.size();
    m_vertices.emplace_back(p0, norm);
    m_vertices.emplace_back(p1, norm);
    m_vertices.emplace_back(p2, norm);
    m_vertices.emplace_back(p3, norm);

    m_indices.emplace_back(size);        /// 0
    m_indices.emplace_back(size + 1);    /// 1
    m_indices.emplace_back(size + 2);    /// 2

    m_indices.emplace_back(size + 2);    /// 2
    m_indices.emplace_back(size + 3);    /// 3
    m_indices.emplace_back(size);        /// 0

    if (!m_create_section_line) {
        return;
    }
    /// 線分の取得
    if (get_seg_flg == 1) {
        m_segments_indices.emplace_back(size + 1);
        m_segments_indices.emplace_back(size + 2);

        m_segments_indices.emplace_back(size + 3);
        m_segments_indices.emplace_back(size + 0);
    }
    else if (get_seg_flg == 2) {
        m_segments_indices.emplace_back(size + 0);
        m_segments_indices.emplace_back(size + 1);

        m_segments_indices.emplace_back(size + 2);
        m_segments_indices.emplace_back(size + 3);
    }
}

void RenderNormalMesh::appendQuad(const Point3f& p0, const Point3f& p1, const Point3f& p2, const Point3f& p3,
                                  int get_seg_flg)
{
    auto norm = ((p1 - p0) ^ (p2 - p0)).normalized();

    unsigned int size = (unsigned int)m_vertices.size();
    m_vertices.emplace_back(p0, norm);
    m_vertices.emplace_back(p1, norm);
    m_vertices.emplace_back(p2, norm);
    m_vertices.emplace_back(p3, norm);

    m_indices.emplace_back(size);        /// 0
    m_indices.emplace_back(size + 1);    /// 1
    m_indices.emplace_back(size + 2);    /// 2

    m_indices.emplace_back(size + 2);    /// 2
    m_indices.emplace_back(size + 3);    /// 3
    m_indices.emplace_back(size);        /// 0

    if (!m_create_section_line) {
        return;
    }
    /// 線分の取得
    if (get_seg_flg == 1) {
        m_segments_indices.emplace_back(size + 1);
        m_segments_indices.emplace_back(size + 2);

        m_segments_indices.emplace_back(size + 3);
        m_segments_indices.emplace_back(size + 0);
    }
    else if (get_seg_flg == 2) {
        m_segments_indices.emplace_back(size + 0);
        m_segments_indices.emplace_back(size + 1);

        m_segments_indices.emplace_back(size + 2);
        m_segments_indices.emplace_back(size + 3);
    }
}

void RenderNormalMesh::clearDisplayData()
{
    m_vertices.clear();
    m_indices.clear();
    m_segments_indices.clear();
    markRenderDirty();
    markBoxDirty();
}

DEFINE_META_RENDERABLENODE(RenderEditableNormalMesh, RenderNormalMesh)

RenderEditableNormalMesh::RenderEditableNormalMesh(RenderNormalMesh* mesh)
{
    setRenderableObject(mesh);
}

RenderEditableNormalMesh::~RenderEditableNormalMesh() {}

RenderEditableNormalMesh::RenderEditableNormalMesh(const RenderEditableNormalMesh& other)
{
    setRenderableObject(other.renderableObject());
    if (other.m_projection_node.isAlive()) {
        m_projection_node = other.m_projection_node;
    }
}

void RenderEditableNormalMesh::updateBoundingBox()
{
    m_bbox.init();
    if (isEnableEditDisplayData()) {
        const auto& vertices = displayEditVertices();
        for (const auto& vertex : vertices) {
            m_bbox.expandBy(vertex.m_position);
        }
    }
    else {
        for (const auto& vertex : originalMesh()->m_vertices) {
            m_bbox.expandBy(vertex.m_position);
        }
    }
    createGroupBoundingBox();
    resetBoxDirty();
}

BoundingBox3f RenderEditableNormalMesh::calculateBoundingBox(const Matrix4x4f& parent_matrix, bool only_visible,
                                                             bool /*including_text*/) const
{
    BoundingBox3f bbox;
    if (only_visible && isEnableEditDisplayData()) {
        const auto& vertices = displayEditVertices();
        for (const auto& vertex : vertices) {
            bbox.expandBy(parent_matrix * vertex.m_position);
        }
    }
    else {
        for (const auto& vertex : originalMesh()->m_vertices) {
            bbox.expandBy(parent_matrix * vertex.m_position);
        }
    }
    return bbox;
}

void RenderEditableNormalMesh::createDisplaySegmentsGroupData()
{
    if (!isSegmentsGroupDirty()) {
        return;
    }
    const auto& vertex_list      = isEnableEditDisplayData() ? displayEditVertices() : displayVertices();
    const auto& segments_indices = isEnableEditDisplayData() ? displayEditSegmentIndices() : displaySegmentIndices();

    /// 高速化のため
    int segments_group_count = (segments_indices.size() / 2 + 99) / 100;
    m_segments_group_box.resize(segments_group_count);
    m_segments_group_start_index.resize(segments_group_count, INT_MAX);
    for (size_t i = 0; i < segments_indices.size(); i += 2) {
        int group_count = (i / 2) / 100;

        m_segments_group_box[group_count].expandBy(vertex_list[segments_indices[i]].m_position);
        m_segments_group_box[group_count].expandBy(vertex_list[segments_indices[i + 1]].m_position);
        if (m_segments_group_start_index[group_count] == INT_MAX) {
            m_segments_group_start_index[group_count] = i;
        }
    }

    resetSegmentsGroupDirty();
}

void RenderEditableNormalMesh::createGroupBoundingBox()
{
    const auto& vertex_list = isEnableEditDisplayData() ? displayEditVertices() : displayVertices();
    const auto& index_list  = isEnableEditDisplayData() ? displayEditIndices() : displayIndices();

    /// 高速化のため
    int trias_group_count = (index_list.size() / 3 + 99) / 100;
    m_tria_group_box.resize(trias_group_count);
    m_tria_group_start_index.resize(trias_group_count, INT_MAX);
    for (size_t i = 0; i < index_list.size(); i += 3) {
        int group_count = (i / 3) / 100;

        m_tria_group_box[group_count].expandBy(vertex_list[index_list[i]].m_position);
        m_tria_group_box[group_count].expandBy(vertex_list[index_list[i + 1]].m_position);
        m_tria_group_box[group_count].expandBy(vertex_list[index_list[i + 2]].m_position);
        if (m_tria_group_start_index[group_count] == INT_MAX) {
            m_tria_group_start_index[group_count] = i;
        }
    }
}

void RenderEditableNormalMesh::clearDisplayData()
{
    originalMesh()->clearDisplayData();
    clearDisplayEditData();
    markSegmentsGroupDirty();
    markRenderDirty();
    markBoxDirty();
}

Node* RenderEditableNormalMesh::projectionNode()
{
    /// 暫定 - 表示フラグはGUIと連動させる
    if (m_projection_node.isAlive() && m_projection_node->isShow()) {
        return m_projection_node.ptr();
    }
    else {
        return (Node*)nullptr;
    }
}

CORE_NAMESPACE_END
