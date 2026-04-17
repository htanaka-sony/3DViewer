#ifndef RENDERMESH_H
#define RENDERMESH_H

#include "Renderable.h"
#include "Scene/Node.h"

CORE_NAMESPACE_BEGIN

class CORE_EXPORT RenderMesh : public RenderableObject {
    friend class RenderEditableMesh;
    DECLARE_META_RENDERABLE(RenderMesh)
protected:
    RenderMesh();
    ~RenderMesh();

    RenderMesh(const RenderMesh& other);

public:
    void createBoxShape(const BoundingBox3f& box);
    void createBoxTaper(const BoundingBox3f& box, float taperDistance);

public:
    virtual void          updateBoundingBox();
    virtual BoundingBox3f calculateBoundingBox(const Matrix4x4f& parent_matrix, bool only_visible,
                                               bool including_text) const;

    const std::vector<Point3f>&      displayVertices() const { return m_vertices; }
    const std::vector<unsigned int>& displayIndices() const { return m_indices; }
    const std::vector<unsigned int>& displaySegmentIndices() const { return m_segments_indices; }

    std::vector<Point3f>&      displayVertices() { return m_vertices; }
    std::vector<unsigned int>& displayIndices() { return m_indices; }
    std::vector<unsigned int>& displaySegmentIndices() { return m_segments_indices; }

    void setCreateSectionLine(bool create) { m_create_section_line = create; }
    bool isCreateSectionLine() const { return m_create_section_line; }

    void appendQuad(const Point3f& p0, const Point3f& p1, const Point3f& p2, const Point3f& p3, /*const Point3f& norm,*/
                    int get_seg_flg);

    virtual void clearDisplayData() override;

protected:
    /// Display Data
    std::vector<Point3f>      m_vertices;
    std::vector<unsigned int> m_indices;
    std::vector<unsigned int> m_segments_indices;

    bool m_create_section_line = true;
};

class CORE_EXPORT RenderEditableMesh : public RenderableNode {
    DECLARE_META_RENDERABLENODE(RenderEditableMesh, RenderMesh)
protected:
    RenderEditableMesh(RenderMesh* mesh);
    ~RenderEditableMesh();

    RenderEditableMesh(const RenderEditableMesh& other);

public:
    virtual void          updateBoundingBox();
    virtual BoundingBox3f calculateBoundingBox(const Matrix4x4f& parent_matrix, bool only_visible,
                                               bool including_text) const;

    const std::vector<Point3f>&      displayVertices() const { return originalMesh()->m_vertices; }
    const std::vector<unsigned int>& displayIndices() const { return originalMesh()->m_indices; }
    const std::vector<unsigned int>& displaySegmentIndices() const { return originalMesh()->m_segments_indices; }

    std::vector<Point3f>&      displayVertices() { return originalMesh()->m_vertices; }
    std::vector<unsigned int>& displayIndices() { return originalMesh()->m_indices; }
    std::vector<unsigned int>& displaySegmentIndices() { return originalMesh()->m_segments_indices; }

    void setCreateSectionLine(bool create) { originalMesh()->m_create_section_line = create; }

    bool isCreateSectionLine() const { return originalMesh()->m_create_section_line; }

    const std::vector<Point3f>& displayEditVertices() const
    {
        return m_edit_buffer_2 ? m_edit_vertices_2 : m_edit_vertices;
    }
    const std::vector<unsigned int>& displayEditIndices() const
    {
        return m_edit_buffer_2 ? m_edit_indices_2 : m_edit_indices;
    }
    const std::vector<unsigned int>& displayEditSegmentIndices() const
    {
        return m_edit_buffer_2 ? m_edit_segments_indices_2 : m_edit_segments_indices;
    }

    std::vector<Point3f>& displayEditVerticesTemp() { return !m_edit_buffer_2 ? m_edit_vertices_2 : m_edit_vertices; }
    std::vector<unsigned int>& displayEditIndicesTemp() { return !m_edit_buffer_2 ? m_edit_indices_2 : m_edit_indices; }
    std::vector<unsigned int>& displayEditSegmentIndicesTemp()
    {
        return !m_edit_buffer_2 ? m_edit_segments_indices_2 : m_edit_segments_indices;
    }

    void switchEditDisplayBuffer() { m_edit_buffer_2 = !m_edit_buffer_2; }
    bool isEditDisplayBuffer2() { return m_edit_buffer_2; }

    void setDisplayEditData(std::vector<Point3f>& verticies, std::vector<unsigned int>& indices)
    {
        if (m_edit_buffer_2) {
            m_edit_vertices_2 = verticies;
            m_edit_indices_2  = indices;
        }
        else {
            m_edit_vertices = verticies;
            m_edit_indices  = indices;
        }
    }
    void clearDisplayEditData()
    {
        setEnalbeEditDisplayData(false);
        m_edit_vertices.clear();
        m_edit_indices.clear();
        m_edit_vertices_2.clear();
        m_edit_indices_2.clear();
        m_edit_segments_indices.clear();
        m_edit_segments_indices_2.clear();
        m_edit_buffer_2 = false;
    }

    void markSegmentsGroupDirty() { m_segments_group_dirty = true; }
    void resetSegmentsGroupDirty() { m_segments_group_dirty = false; }
    bool isSegmentsGroupDirty() const { return m_segments_group_dirty; }
    void createDisplaySegmentsGroupData();

    const std::vector<BoundingBox3f>& displaySegmentsGroupBox() const { return m_segments_group_box; }
    const std::vector<unsigned int>&  displaySegmentsGroupStart() const { return m_segments_group_start_index; }

    void markTriaGroupDirty() { m_tria_group_dirty = true; }
    void resetTriaGroupDirty() { m_tria_group_dirty = false; }
    bool isTriaGroupDirty() const { return m_tria_group_dirty; }

    const std::vector<BoundingBox3f>& displayTriaGroupBox()
    {
        createGroupBoundingBox();
        return m_tria_group_box;
    }
    const std::vector<unsigned int>& displayTriaGroupStart()
    {
        createGroupBoundingBox();
        return m_tria_group_start_index;
    }
    void createGroupBoundingBox();

    virtual void clearDisplayData() override;

    void  setProjectionNode(Node* project_node) override { m_projection_node = project_node; }
    Node* projectionNode() override;

    bool isDrawShading() const override { return m_draw_shading; }
    bool isDrawWireframe() const override { return m_draw_wireframe; }

    void setDrawShading(bool shading) override { m_draw_shading = shading; }
    void setDrawWireframe(bool wireframe) override { m_draw_wireframe = wireframe; }

    /// Todo: Edit固定(Editした状態で、それをオリジナルとみなす)をここでやるか、RenderableNodeでやる
    RenderMesh*       originalMesh() { return (RenderMesh*)renderableObject(); }
    const RenderMesh* originalMesh() const { return (const RenderMesh*)renderableObject(); }

    bool isVboUse() { return m_vbo_use; }
    void setVboUse(bool use) { m_vbo_use = use; }

    int  drawPriority() const override { return m_draw_priority; }
    void setDrawPriority(int priority) override { m_draw_priority = priority; }

protected:
    /// TODO: 暫定: 描画高速化のため線分をグループで分ける
    std::vector<BoundingBox3f> m_segments_group_box;
    std::vector<unsigned int>  m_segments_group_start_index;

    /// TODO: 暫定: 高速化のためトリアをグループで分ける
    std::vector<BoundingBox3f> m_tria_group_box;
    std::vector<unsigned int>  m_tria_group_start_index;

    /// Display Edit Data(ClippingData)
    std::vector<Point3f>      m_edit_vertices;
    std::vector<unsigned int> m_edit_indices;
    std::vector<unsigned int> m_edit_segments_indices;

    /// TODO: 暫定　高速化のため、バッファを二つもつ
    std::vector<Point3f>      m_edit_vertices_2;
    std::vector<unsigned int> m_edit_indices_2;
    std::vector<unsigned int> m_edit_segments_indices_2;

    WeakRefPtr<Node> m_projection_node;

    /// フラグ
    bool m_eneble_edit_display  = false;
    bool m_edit_buffer_2        = false;
    bool m_segments_group_dirty = true;
    bool m_tria_group_dirty     = true;
    bool m_draw_shading         = true;
    bool m_draw_wireframe       = true;

    int m_draw_priority = 0;    /// 描画優先調整

    bool m_vbo_use = true;
};

CORE_NAMESPACE_END

#endif    // RENDERMESH_H
