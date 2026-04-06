#ifndef CORE_PIKKING_H
#define CORE_PIKKING_H

#include "CoreGlobal.h"
#include "Math/BoundingBox2f.h"
#include "Math/Planef.h"
#include "Math/Point2f.h"
#include "Math/Point2i.h"
#include "Math/PolyLoop2f.h"
#include "Math/SearchPerform.h"
#include "Scene/Node.h"
#include "Scene/SceneGraph.h"

#include <execution>

CORE_NAMESPACE_BEGIN

class CORE_EXPORT Picking {
public:
    struct CORE_EXPORT SIntersectionInfo {
        Node*   m_node;
        int     m_tria_index;
        float   m_dist_line_dir;    /// Line上の原点からの距離
        float   m_dist_line_tol;    /// Lineに対するずれ（Line上なら0.0f）
        Point3f m_tria_norm;

        bool    orgTria(bool global, Point3f& point0, Point3f& point1, Point3f& point2);
        Point3f orgNorm(bool global);
    };

    struct TargetNodeInfo {
        Node*      m_node;
        float      m_box_dist_line_dir;
        Matrix4x4f m_matrix;

        TargetNodeInfo(Node* node, float dist, const Matrix4x4f& matrix)
            : m_node(node)
            , m_box_dist_line_dir(dist)
            , m_matrix(matrix)
        {
        }
    };

    Picking(SceneView* scene_view, const std::vector<float>& depth_buffer, int view_port_width, int view_port_height,
            Point2i mouse_pos)
        : m_scene_view(scene_view)
        , m_depth_buffer(depth_buffer)
        , m_view_port_width(view_port_width)
        , m_view_port_height(view_port_height)
        , m_mouse_pos(mouse_pos)
    {
    }

    Picking(SceneView* scene_view)
        : m_scene_view(scene_view)
        , m_depth_buffer(std::vector<float>())
        , m_view_port_width(0)
        , m_view_port_height(0)
        , m_mouse_pos(Point2i())
    {
    }

    Node* getNearestPoint(std::vector<Node*>& nodes, const Point3f& line_dir, const Point3f& line_pos,
                          const float distance, Point3f& out_pos, bool only_visible = true);

    void setOnlyMinimum(bool minimum) { m_only_get_minimum = minimum; }
    void setExceptOnPlane(bool except_on_plane, float except_on_plane_cos = 0.0f)
    {
        m_except_on_plane     = except_on_plane;
        m_except_on_plane_cos = except_on_plane_cos;
    }
    void bodyLineIntersection(const Point3f& line_pos, const Point3f& line_vec, bool only_visible,
                              std::vector<SIntersectionInfo>& intersetions, float fTol, bool above_line_pos = false,
                              std::vector<Node*>* target_nodes = nullptr);
    void bodyLineIntersection(const Point3f& line_pos, const Point3f& line_vec, Node* target_node,
                              std::vector<SIntersectionInfo>& intersetions, float fTol, bool above_line_pos = false);

protected:
    bool getNearestPoint(Node* node, const Point3f& line_dir, const Point3f& line_pos, const float distance,
                         Point3f& nearest_pos, float& min_param, float& min_dist2, bool only_visible = true);

    void bodyLineIntersectionMinimumTarget(const Matrix4x4f& inv_line_matrix, Node* node, bool only_visible, float fTol,
                                           bool above_line_pos, std::vector<TargetNodeInfo>& targets);
    void bodyLineIntersectionMinimumTarget(const Matrix4x4f& inv_line_matrix, std::vector<Node*>& target_nodes,
                                           bool only_visible, float fTol, bool above_line_pos,
                                           std::vector<TargetNodeInfo>& targets);

    void bodyLineIntersectionMinimum(std::vector<TargetNodeInfo>& targets, float fTol, bool above_line_pos,
                                     std::vector<SIntersectionInfo>& intersetions);

    void bodyLineIntersection(const Matrix4x4f& inv_line_matrix, Node* node, bool only_visible,
                              std::vector<SIntersectionInfo>& intersetions, float fTol, bool above_line_pos);

    bool triaZeroLineInersection(const Point3f& v0, const Point3f& v1, const Point3f& v2, float& dist_line_dir,
                                 float& dist_line_tol, float fTol);

protected:
    SceneView*                m_scene_view;
    const std::vector<float>& m_depth_buffer;
    const int                 m_view_port_width;
    const int                 m_view_port_height;
    const Point2i             m_mouse_pos;

    bool  m_only_get_minimum    = false;
    bool  m_except_on_plane     = false;
    float m_except_on_plane_cos = 0.0174524f;
    float m_line_dir_minimum    = 0;
};

CORE_NAMESPACE_END

#endif    // CORE_PIKKING_H
