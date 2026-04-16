#ifndef CORE_CLIPPING_H
#define CORE_CLIPPING_H

#include "CoreGlobal.h"
#include "Math/BoundingBox2f.h"
#include "Math/Planef.h"
#include "Math/Point2f.h"
#include "Math/PolyLoop2f.h"
#include "Math/SearchPerform.h"
#include "Scene/Node.h"
#include "Scene/RenderMesh.h"
#include "Scene/RenderNormalMesh.h"
#include "Scene/SceneGraph.h"
#include "Scene/Voxel.h"

CORE_NAMESPACE_BEGIN

class CORE_EXPORT Clipping {
public:
    struct TargetNode {
        Node*      m_node;
        Matrix4x4f m_path_matrix;
    };

    struct PlaneInfo {
        Planef     m_plane;
        Matrix4x4f m_plane_matrix;
        Matrix4x4f m_inv_plane_matrix;
        bool       m_only_get_plane = false;

        PlaneInfo(const Planef& plane) : m_plane(plane) {}

        void setPlaneInfo()
        {
            m_plane_matrix     = Matrix4x4f(m_plane);
            m_inv_plane_matrix = m_plane_matrix.inverted();
        }
    };

    Clipping(SceneGraph* scene_graph);
    ~Clipping();

    void execute(const VecPlanef& planes, bool only_visible);

    void execute(const std::vector<PlaneInfo>& plane_info_list, std::vector<TargetNode>& target_list);

    bool clear(Node* node, bool only_visible);

    static void deleteWorkMemory();

    void collectTarget(Node* node, const Matrix4x4f& parent_matrix, std::vector<TargetNode>& target_list,
                       bool only_visible);

    /// Auto Length用 - clip面で切ったのちに、それを切った断面線で測定結果にする
    /// 3次元の直線交点判定が微妙（面上だと内外判定が微妙）なので、形状内外判定間違えないはず（断面おかしくなれれば）のロジックで行う
    struct SectionInfo {
        Node*                                    m_node;
        std::vector<std::pair<Point3f, Point3f>> m_points;
    };
    void sectionLineForAutoLength(const Planef& clip_plane, const Planef& line_cut_plane, bool only_visible,
                                  std::vector<SectionInfo>& out_section_lines, BoundingBox3f* clip_area);

protected:
    struct EdgeInfo {
        Point2f m_start;
        Point2f m_end;
        int     m_origin_start_vertex_index;
        int     m_origin_end_vertex_index;
    };

    using ClippingLoop = PolyLoop2fWithData<int>;

    struct ClippingWork {
        // std::vector<int>          m_work_old_new_indices;
        int*                 m_work_old_new_indices      = nullptr;
        int                  m_work_old_new_indices_size = 0;
        std::vector<Point3f> m_work_vertex_list;
        std::vector<Vertexf> m_work_vertex_list_normal;
        // std::vector<unsigned int> m_work_index_list;
        ~ClippingWork() { delete[] m_work_old_new_indices; }
    };

    /// 通常のクリッピング
    bool clipping(const std::vector<Point3f>& in_vertices, const std::vector<unsigned int>& in_indices,
                  const std::vector<unsigned int>& in_segments, const PlaneInfo& plane_info,
                  std::vector<Point3f>& out_vertices, std::vector<unsigned int>& out_indices,
                  std::vector<unsigned int>& out_segments, bool create_segment, bool create_fill, bool after_2d_clip,
                  ClippingWork* work_space);

    /// クリッピング面だけを取得
    bool clippingOnlyPlane(const std::vector<Point3f>& in_vertices, const std::vector<unsigned int>& in_indices,
                           const std::vector<unsigned int>& in_segments, const PlaneInfo& plane_info,
                           std::vector<Point3f>& out_vertices, std::vector<unsigned int>& out_indices,
                           std::vector<unsigned int>& out_segments, bool create_segment, bool create_fill,
                           ClippingWork* work_space);

    /// 通常のクリッピング (法線付き: RenderNormalMesh用)
    bool clipping(const std::vector<Vertexf>& in_vertices, const std::vector<unsigned int>& in_indices,
                  const std::vector<unsigned int>& in_segments, const PlaneInfo& plane_info,
                  std::vector<Vertexf>& out_vertices, std::vector<unsigned int>& out_indices,
                  std::vector<unsigned int>& out_segments, bool create_segment, bool create_fill, bool after_2d_clip,
                  ClippingWork* work_space);

    /// クリッピング面だけを取得 (法線付き: RenderNormalMesh用)
    bool clippingOnlyPlane(const std::vector<Vertexf>& in_vertices, const std::vector<unsigned int>& in_indices,
                           const std::vector<unsigned int>& in_segments, const PlaneInfo& plane_info,
                           std::vector<Vertexf>& out_vertices, std::vector<unsigned int>& out_indices,
                           std::vector<unsigned int>& out_segments, bool create_segment, bool create_fill,
                           ClippingWork* work_space);

    /// 自動寸法用
    bool clippingForAutoLength(const std::vector<Point3f>& in_vertices, const std::vector<unsigned int>& in_indices,
                               const PlaneInfo& plane_info, std::vector<Point3f>& out_vertices,
                               std::vector<unsigned int>& out_indices, ClippingWork* work_space);

    /// 自動寸法用 (法線付き: RenderNormalMesh用)
    bool clippingForAutoLength(const std::vector<Vertexf>& in_vertices, const std::vector<unsigned int>& in_indices,
                               const PlaneInfo& plane_info, std::vector<Vertexf>& out_vertices,
                               std::vector<unsigned int>& out_indices, ClippingWork* work_space);

    bool clipPolygonWithPlaneSimple(const std::vector<Point3f>&      in_vertices,
                                    const std::vector<unsigned int>& in_indices, const PlaneInfo& plane_info,
                                    std::vector<std::pair<Point3f, Point3f>>& out_edges, BoundingBox3f* clip_area);
    bool clipPolygonWithPlaneSimple(const std::vector<Vertexf>&      in_vertices,
                                    const std::vector<unsigned int>& in_indices, const PlaneInfo& plane_info,
                                    std::vector<std::pair<Point3f, Point3f>>& out_edges, BoundingBox3f* clip_area);

    bool clipPolygonWithPlaneSimple(RenderEditableMesh* mesh, const PlaneInfo& plane_info,
                                    std::vector<std::pair<Point3f, Point3f>>& out_edges, BoundingBox3f* clip_area);
    bool clipPolygonWithPlaneSimple(RenderEditableNormalMesh* mesh, const PlaneInfo& plane_info,
                                    std::vector<std::pair<Point3f, Point3f>>& out_edges, BoundingBox3f* clip_area);

    bool clipPolygonWithPlane(const std::vector<Point3f>& in_vertices, const std::vector<unsigned int>& in_indices,
                              const std::vector<unsigned int>& in_segments, const PlaneInfo& plane_info,
                              std::vector<Point3f>& out_vertices, std::vector<unsigned int>& out_indices,
                              std::vector<unsigned int>& out_segments, bool create_segment, bool create_fill,
                              bool only_get_on_plane, bool after_2d_clip, std::vector<EdgeInfo>& edge_list,
                              ClippingWork* work_space);

    /// 法線付き版 (RenderNormalMesh用): カット位置で法線を補間して新頂点を生成
    bool clipPolygonWithPlane(const std::vector<Vertexf>& in_vertices, const std::vector<unsigned int>& in_indices,
                              const std::vector<unsigned int>& in_segments, const PlaneInfo& plane_info,
                              std::vector<Vertexf>& out_vertices, std::vector<unsigned int>& out_indices,
                              std::vector<unsigned int>& out_segments, bool create_segment, bool create_fill,
                              bool only_get_on_plane, bool after_2d_clip, std::vector<EdgeInfo>& edge_list,
                              ClippingWork* work_space);

    void createContour(const std::vector<EdgeInfo>& edge_list, std::vector<ClippingLoop*>& contours,
                       ClippingWork* work_space);

    void outlinesFromLoops(const std::vector<ClippingLoop*>& loops, std::vector<std::vector<ClippingLoop*>>& outlines,
                           ClippingWork* work_space);

    void triangleFromOutline(const std::vector<ClippingLoop*>& outline, std::vector<Point3f>& out_vertices,
                             const PlaneInfo& plane_info, std::vector<unsigned int>& out_indices,
                             ClippingWork* work_space);

    void triangleFromOutline(const std::vector<ClippingLoop*>& outline, std::vector<Point3f>& out_vertices_input,
                             std::vector<Point3f>& out_vertices, std::map<int, int>& out_index_old_new,
                             const PlaneInfo& plane_info, std::vector<unsigned int>& out_indices,
                             ClippingWork* work_space);

    /// 法線付き版 (RenderNormalMesh用): 断面塗りつぶし頂点に平面法線を設定
    void triangleFromOutline(const std::vector<ClippingLoop*>& outline, std::vector<Vertexf>& out_vertices,
                             const PlaneInfo& plane_info, std::vector<unsigned int>& out_indices,
                             ClippingWork* work_space);

    void triangleFromOutline(const std::vector<ClippingLoop*>& outline, std::vector<Vertexf>& out_vertices_input,
                             std::vector<Vertexf>& out_vertices, std::map<int, int>& out_index_old_new,
                             const PlaneInfo& plane_info, std::vector<unsigned int>& out_indices,
                             ClippingWork* work_space);

    void addSegents(const std::vector<ClippingLoop*>& outline, std::vector<unsigned int>& out_segments);
    void addSegents(const std::vector<ClippingLoop*>& outline, std::vector<unsigned int>& out_segments,
                    std::vector<Point3f>& out_vertices_input, std::vector<Point3f>& out_vertices,
                    std::map<int, int>& out_index_old_new);

    /// 法線付き版 (RenderNormalMesh用)
    void addSegents(const std::vector<ClippingLoop*>& outline, std::vector<unsigned int>& out_segments,
                    std::vector<Vertexf>& out_vertices_input, std::vector<Vertexf>& out_vertices,
                    std::map<int, int>& out_index_old_new);

protected:
    SceneGraph* m_scene_graph;
    float       m_valid_eps;
    float       m_zero_eps;
    float       m_valid_eps_sq;
    float       m_zero_eps_sq;

    static std::vector<ClippingWork*> m_work_list;
};

CORE_NAMESPACE_END

#endif    // CORE_CLIPPING_H
