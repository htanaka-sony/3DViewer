#include "RenderMesh.h"

CORE_NAMESPACE_BEGIN

DEFINE_META_RENDERABLE(RenderMesh)

RenderMesh::RenderMesh() {}

RenderMesh::~RenderMesh() {}

RenderMesh::RenderMesh(const RenderMesh& other)
{
    m_vertices            = other.m_vertices;
    m_indices             = other.m_indices;
    m_segments_indices    = other.m_segments_indices;
    m_create_section_line = other.m_create_section_line;
}

void RenderMesh::updateBoundingBox()
{
    m_bbox.init();
    for (const auto& vertex : m_vertices) {
        m_bbox.expandBy(vertex);
    }
    resetBoxDirty();
}

BoundingBox3f RenderMesh::calculateBoundingBox(const Matrix4x4f& parent_matrix, bool /*only_visible*/,
                                               bool /*including_text*/) const
{
    BoundingBox3f bbox;
    for (const auto& vertex : m_vertices) {
        bbox.expandBy(parent_matrix * vertex);
    }
    return bbox;
}

void RenderMesh::appendQuad(const Point3f& p0, const Point3f& p1, const Point3f& p2, const Point3f& p3, int get_seg_flg)
{
    unsigned int size = (unsigned int)m_vertices.size();
    m_vertices.emplace_back(p0);
    m_vertices.emplace_back(p1);
    m_vertices.emplace_back(p2);
    m_vertices.emplace_back(p3);

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

void RenderMesh::clearDisplayData()
{
    m_vertices.clear();
    m_indices.clear();
    m_segments_indices.clear();
    markRenderDirty();
    markBoxDirty();
}

DEFINE_META_RENDERABLE(RenderEditableMesh)

RenderEditableMesh::RenderEditableMesh() {}

RenderEditableMesh::~RenderEditableMesh() {}

RenderEditableMesh::RenderEditableMesh(const RenderEditableMesh& other)
{
    m_mesh = other.m_mesh;
    m_mesh->m_use_count++;    /// Editなしでコピー

    /*
    m_segments_group_box         = other.m_segments_group_box;
    m_segments_group_start_index = other.m_segments_group_start_index;

    m_tria_group_box         = other.m_tria_group_box;
    m_tria_group_start_index = other.m_tria_group_start_index;

    m_edit_vertices         = other.m_edit_vertices;
    m_edit_indices          = other.m_edit_indices;
    m_edit_segments_indices = other.m_edit_segments_indices;

    m_edit_vertices_2         = other.m_edit_vertices_2;
    m_edit_indices_2          = other.m_edit_indices_2;
    m_edit_segments_indices_2 = other.m_edit_segments_indices_2;

    m_eneble_edit_display  = other.m_eneble_edit_display;
    m_edit_buffer_2        = other.m_edit_buffer_2;
    m_render_dirty         = other.m_render_dirty;
    m_segments_group_dirty = other.m_segments_group_dirty;
    */

    // m_create_section_line = other.m_create_section_line;
    // m_vbo_use             = other.m_vbo_use;
}

void RenderEditableMesh::updateBoundingBox()
{
    m_bbox.init();
    if (isEnableEditDisplayData()) {
        const auto& vertices = displayEditVertices();
        for (const auto& vertex : vertices) {
            m_bbox.expandBy(vertex);
        }
    }
    else {
        for (const auto& vertex : m_mesh->m_vertices) {
            m_bbox.expandBy(vertex);
        }
    }
    createGroupBoundingBox();
    resetBoxDirty();
}

BoundingBox3f RenderEditableMesh::calculateBoundingBox(const Matrix4x4f& parent_matrix, bool only_visible,
                                                       bool /*including_text*/) const
{
    BoundingBox3f bbox;
    if (only_visible && isEnableEditDisplayData()) {
        const auto& vertices = displayEditVertices();
        for (const auto& vertex : vertices) {
            bbox.expandBy(parent_matrix * vertex);
        }
    }
    else {
        for (const auto& vertex : m_mesh->m_vertices) {
            bbox.expandBy(parent_matrix * vertex);
        }
    }
    return bbox;
}

void RenderEditableMesh::createDisplaySegmentsGroupData()
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

        m_segments_group_box[group_count].expandBy(vertex_list[segments_indices[i]]);
        m_segments_group_box[group_count].expandBy(vertex_list[segments_indices[i + 1]]);
        if (m_segments_group_start_index[group_count] == INT_MAX) {
            m_segments_group_start_index[group_count] = i;
        }
    }

    resetSegmentsGroupDirty();
}

void RenderEditableMesh::createGroupBoundingBox()
{
    const auto& vertex_list = isEnableEditDisplayData() ? displayEditVertices() : displayVertices();
    const auto& index_list  = isEnableEditDisplayData() ? displayEditIndices() : displayIndices();

    /// 高速化のため
    int trias_group_count = (index_list.size() / 3 + 99) / 100;
    m_tria_group_box.resize(trias_group_count);
    m_tria_group_start_index.resize(trias_group_count, INT_MAX);
    for (size_t i = 0; i < index_list.size(); i += 3) {
        int group_count = (i / 3) / 100;

        m_tria_group_box[group_count].expandBy(vertex_list[index_list[i]]);
        m_tria_group_box[group_count].expandBy(vertex_list[index_list[i + 1]]);
        m_tria_group_box[group_count].expandBy(vertex_list[index_list[i + 2]]);
        if (m_tria_group_start_index[group_count] == INT_MAX) {
            m_tria_group_start_index[group_count] = i;
        }
    }
}

void RenderEditableMesh::clearDisplayData()
{
    m_mesh->clearDisplayData();
    clearDisplayEditData();
    markSegmentsGroupDirty();
    markRenderDirty();
    markBoxDirty();
}

CORE_NAMESPACE_END
