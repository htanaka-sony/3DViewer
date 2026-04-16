#include "Picking.h"
#include "Scene/RenderMesh.h"
#include "Scene/RenderNormalMesh.h"
#include "Scene/Voxel.h"

CORE_NAMESPACE_BEGIN

Node* Picking::getNearestPoint(std::vector<Node*>& nodes, const Point3f& line_dir, const Point3f& line_pos,
                               const float distance, Point3f& out_pos, bool only_visible)
{
    Node* pick      = nullptr;
    float min_param = FLT_MAX;
    float min_dist2 = FLT_MAX;
    for (auto& node : nodes) {
        if (getNearestPoint(node, line_dir, line_pos, distance, out_pos, min_param, min_dist2, only_visible)) {
            pick = node;
        }
    }

    return pick;
}

bool Picking::getNearestPoint(Node* node, const Point3f& line_dir, const Point3f& line_pos, const float distance,
                              Point3f& nearest_pos, float& min_param, float& min_dist2, bool only_visible)
{
    if (only_visible) {
        if (!node->isVisible()) {
            return false;
        }
    }

    auto object = node->object();
    if (!object) {
        return false;
    }

    auto renderable = node->renderable();
    if (!renderable) {
        return false;
    }

    const auto& matrix     = node->pathMatrix();
    bool        has_matrix = !matrix.isIdentity();

    const float judge_dist2 = distance * distance;

    bool ret = false;

    switch (renderable->type()) {
        case RenderableType::RenderEditableMesh: {
            RenderEditableMesh* mesh = (RenderEditableMesh*)renderable;
            const auto&         vertices =
                mesh->isEnableEditDisplayData() ? mesh->displayEditVertices() : mesh->displayVertices();

            for (const auto vertex : vertices) {
                const auto& ver = has_matrix ? matrix.preMult(vertex) : vertex;

                const auto& vec   = ver - line_pos;
                const auto& cross = vec ^ line_dir;

                float dist2 = cross.length2();
                if (dist2 <= judge_dist2) {
                    float param = (vec * line_dir);
                    if ((param < min_param) || (param == min_param && dist2 < min_dist2)) {
                        const auto& chect_ver_screen = m_scene_view->projectToScreen(ver);

                        int x_pos = chect_ver_screen.x() - m_mouse_pos.x() + m_view_port_width / 2;
                        int y_pos = chect_ver_screen.y() - m_mouse_pos.y() + m_view_port_height / 2;

                        if (x_pos >= 0 && x_pos < m_view_port_width && y_pos >= 0 && y_pos < m_view_port_height) {
                            int index = (y_pos * m_view_port_width + x_pos);

                            const auto& check_ver = m_scene_view->unprojectFromScreen(
                                Point3f(chect_ver_screen.x(), chect_ver_screen.y(), m_depth_buffer[index]));
                            const auto& check_vec = check_ver - line_pos;

                            float check_param = (check_vec * line_dir);
                            if (param <= check_param + 1.0e-6f) {
                                min_param   = param;
                                min_dist2   = dist2;
                                nearest_pos = ver;
                                ret         = true;
                            }
                        }
                    }
                }
            }
        } break;
        case RenderableType::RenderEditableNormalMesh: {
            RenderEditableNormalMesh* mesh = (RenderEditableNormalMesh*)renderable;
            const auto&               vertices =
                mesh->isEnableEditDisplayData() ? mesh->displayEditVertices() : mesh->displayVertices();

            for (const auto vertex : vertices) {
                const auto& ver = has_matrix ? matrix.preMult(vertex.m_position) : vertex.m_position;

                const auto& vec   = ver - line_pos;
                const auto& cross = vec ^ line_dir;

                float dist2 = cross.length2();
                if (dist2 <= judge_dist2) {
                    float param = (vec * line_dir);
                    if ((param < min_param) || (param == min_param && dist2 < min_dist2)) {
                        const auto& chect_ver_screen = m_scene_view->projectToScreen(ver);

                        int x_pos = chect_ver_screen.x() - m_mouse_pos.x() + m_view_port_width / 2;
                        int y_pos = chect_ver_screen.y() - m_mouse_pos.y() + m_view_port_height / 2;

                        if (x_pos >= 0 && x_pos < m_view_port_width && y_pos >= 0 && y_pos < m_view_port_height) {
                            int index = (y_pos * m_view_port_width + x_pos);

                            const auto& check_ver = m_scene_view->unprojectFromScreen(
                                Point3f(chect_ver_screen.x(), chect_ver_screen.y(), m_depth_buffer[index]));
                            const auto& check_vec = check_ver - line_pos;

                            float check_param = (check_vec * line_dir);
                            if (param <= check_param + 1.0e-6f) {
                                min_param   = param;
                                min_dist2   = dist2;
                                nearest_pos = ver;
                                ret         = true;
                            }
                        }
                    }
                }
            }
        } break;
    }

    return ret;
}

void Picking::bodyLineIntersectionMinimumTarget(const Matrix4x4f& inv_line_matrix, Node* node, bool only_visible,
                                                float fTol, bool above_line_pos, std::vector<TargetNodeInfo>& targets)
{
    if (only_visible) {
        if (!node->isVisible()) {
            return;
        }
    }

    Matrix4x4f cur_matrix = inv_line_matrix * node->matrix();

    auto renderable = node->renderable();
    if (renderable) {
        switch (renderable->type()) {
            case RenderableType::RenderEditableMesh:
            case RenderableType::RenderEditableNormalMesh: {
                auto bbox = cur_matrix.preMult(renderable->boundingBox());

                if (bbox.valid()) {
                    if (bbox.xMin() > 0.0f + fTol || bbox.xMax() < 0.0f - fTol || bbox.yMin() > 0.0f + fTol
                        || bbox.yMax() < 0.0f - fTol) {
                        return;
                    }
                    if (above_line_pos) {
                        if (bbox.zMin() > 0.0f + fTol) {
                            return;
                        }
                    }

                    targets.emplace_back(node, bbox.zMin(), cur_matrix);
                }
            } break;
            default:
                break;
        }
    }

    for (int ic = 0; ic < node->numChildren(); ++ic) {
        bodyLineIntersectionMinimumTarget(cur_matrix, node->child(ic), only_visible, fTol, above_line_pos, targets);
    }
}

void Picking::bodyLineIntersectionMinimumTarget(const Matrix4x4f& inv_line_matrix, std::vector<Node*>& target_nodes,
                                                bool only_visible, float fTol, bool above_line_pos,
                                                std::vector<TargetNodeInfo>& targets)
{
    for (auto& node : target_nodes) {
        if (only_visible) {
            if (!node->isVisible()) {
                continue;
            }
        }

        Matrix4x4f cur_matrix = inv_line_matrix * node->pathMatrix();

        auto renderable = node->renderable();
        if (renderable) {
            switch (renderable->type()) {
                case RenderableType::RenderEditableMesh:
                case RenderableType::RenderEditableNormalMesh: {
                    auto bbox = cur_matrix.preMult(renderable->boundingBox());

                    if (bbox.valid()) {
                        if (bbox.xMin() > 0.0f + fTol || bbox.xMax() < 0.0f - fTol || bbox.yMin() > 0.0f + fTol
                            || bbox.yMax() < 0.0f - fTol) {
                            return;
                        }
                        if (above_line_pos) {
                            if (bbox.zMin() > 0.0f + fTol) {
                                return;
                            }
                        }

                        targets.emplace_back(node, bbox.zMin(), cur_matrix);
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
}

void Picking::bodyLineIntersectionMinimum(std::vector<TargetNodeInfo>& targets, float fTol, bool above_line_pos,
                                          std::vector<SIntersectionInfo>& intersetions)
{
    std::vector<TargetNodeInfo*> sorted_list;
    for (auto& target_node_info : targets) {
        sorted_list.emplace_back(&target_node_info);
    }

    std::sort(sorted_list.begin(), sorted_list.end(), [](const TargetNodeInfo* info0, const TargetNodeInfo* info1) {
        return info0->m_box_dist_line_dir < info1->m_box_dist_line_dir;
    });

    struct TargetIndexInfo {
        int   m_index;
        float m_box_dist_line_dir;

        TargetIndexInfo(int index, float dist) : m_index(index), m_box_dist_line_dir(dist) {}
    };

    int loop_count = 0;
    for (auto& target : sorted_list) {
        Node*             node              = target->m_node;
        const Matrix4x4f& cur_matrix        = target->m_matrix;
        float             node_box_line_dir = target->m_box_dist_line_dir;

        if (m_line_dir_minimum != FLT_MAX) {
            if (node_box_line_dir > m_line_dir_minimum) {
                /// ループ自体終わる
                return;
            }
        }
        loop_count++;

        auto renderable = node->renderable();
        if (!renderable) {
            continue;
        }

        switch (renderable->type()) {
            case RenderableType::RenderEditableMesh: {
                RenderEditableMesh* mesh = (RenderEditableMesh*)renderable;

                const auto& vertex_list =
                    mesh->isEnableEditDisplayData() ? mesh->displayEditVertices() : mesh->displayVertices();
                const auto& index_list =
                    mesh->isEnableEditDisplayData() ? mesh->displayEditIndices() : mesh->displayIndices();

                SIntersectionInfo temp;
                temp.m_node = node;

                const auto& group_boxes = mesh->displayTriaGroupBox();
                const auto& group_start = mesh->displayTriaGroupStart();

                std::vector<TargetIndexInfo> target_index;

                for (int group_index = 0; group_index < group_start.size(); ++group_index) {
                    auto bbox = cur_matrix.preMult(group_boxes[group_index]);

                    if (bbox.valid()) {
                        if (bbox.xMin() > 0.0f + fTol || bbox.xMax() < 0.0f - fTol || bbox.yMin() > 0.0f + fTol
                            || bbox.yMax() < 0.0f - fTol) {
                            continue;
                        }
                        if (above_line_pos) {
                            if (bbox.zMin() > 0.0f + fTol) {
                                continue;
                            }
                        }
                        if (m_line_dir_minimum != FLT_MAX) {
                            if (bbox.zMin() > m_line_dir_minimum) {
                                continue;
                            }
                        }
                        target_index.emplace_back(group_index, bbox.zMin());
                    }
                }
                std::sort(target_index.begin(), target_index.end(),
                          [](const TargetIndexInfo& info0, const TargetIndexInfo& info1) {
                              return info0.m_box_dist_line_dir < info1.m_box_dist_line_dir;
                          });

                for (int index = 0; index < target_index.size(); ++index) {
                    int group_index = target_index[index].m_index;

                    if (m_line_dir_minimum != FLT_MAX) {
                        if (target_index[index].m_box_dist_line_dir > m_line_dir_minimum) {
                            /// ループ自体終わる
                            break;
                        }
                    }

                    int start_index = group_start[group_index];
                    int end_index;
                    if (group_index < group_start.size() - 1) {
                        end_index = group_start[group_index + 1];
                    }
                    else {
                        end_index = index_list.size();
                    }

                    for (int ic = start_index; ic < end_index; ic += 3) {
                        const unsigned int* tri_indices = &index_list[ic];

                        const auto point0 = cur_matrix * vertex_list[tri_indices[0]];
                        const auto point1 = cur_matrix * vertex_list[tri_indices[1]];
                        const auto point2 = cur_matrix * vertex_list[tri_indices[2]];

                        float dist_line_dir, dist_line_tol;
                        if (triaZeroLineInersection(point0, point1, point2, dist_line_dir, dist_line_tol, fTol)) {
                            Point3f triaNorm = (point1 - point0) ^ (point2 - point0);
                            triaNorm.normalize();

                            /// オプション　- 面上の接触を除外
                            if (m_except_on_plane) {
                                /// triaNorm * Point3f(0,0,1) -> Z成分
                                if (fabs(triaNorm[2]) <= m_except_on_plane_cos) {
                                    continue;
                                }
                            }

                            /// 最小の一つのみ取得
                            if (m_only_get_minimum) {
                                if (!intersetions.empty()) {
                                    if (dist_line_dir < intersetions[0].m_dist_line_dir) {
                                        temp.m_dist_line_dir = dist_line_dir;
                                        temp.m_dist_line_tol = dist_line_tol;
                                        temp.m_tria_index    = ic;
                                        temp.m_tria_norm     = triaNorm;
                                        intersetions[0]      = temp;
                                        m_line_dir_minimum   = dist_line_dir;
                                    }
                                    if (dist_line_dir == intersetions[0].m_dist_line_dir) {
                                        if (fabs(triaNorm[2]) > fabs(intersetions[0].m_tria_norm[2])) {
                                            temp.m_dist_line_dir = dist_line_dir;
                                            temp.m_dist_line_tol = dist_line_tol;
                                            temp.m_tria_index    = ic;
                                            temp.m_tria_norm     = triaNorm;
                                            intersetions[0]      = temp;
                                            m_line_dir_minimum   = dist_line_dir;
                                        }
                                    }
                                    continue;
                                }
                                m_line_dir_minimum = dist_line_dir;
                            }

                            temp.m_dist_line_dir = dist_line_dir;
                            temp.m_dist_line_tol = dist_line_tol;
                            temp.m_tria_index    = ic;
                            temp.m_tria_norm     = triaNorm;
                            intersetions.emplace_back(temp);
                        }
                    }
                }
            } break;
            case RenderableType::RenderEditableNormalMesh: {
                RenderEditableNormalMesh* mesh = (RenderEditableNormalMesh*)renderable;

                const auto& vertex_list =
                    mesh->isEnableEditDisplayData() ? mesh->displayEditVertices() : mesh->displayVertices();
                const auto& index_list =
                    mesh->isEnableEditDisplayData() ? mesh->displayEditIndices() : mesh->displayIndices();

                SIntersectionInfo temp;
                temp.m_node = node;

                const auto& group_boxes = mesh->displayTriaGroupBox();
                const auto& group_start = mesh->displayTriaGroupStart();

                std::vector<TargetIndexInfo> target_index;

                for (int group_index = 0; group_index < group_start.size(); ++group_index) {
                    auto bbox = cur_matrix.preMult(group_boxes[group_index]);

                    if (bbox.valid()) {
                        if (bbox.xMin() > 0.0f + fTol || bbox.xMax() < 0.0f - fTol || bbox.yMin() > 0.0f + fTol
                            || bbox.yMax() < 0.0f - fTol) {
                            continue;
                        }
                        if (above_line_pos) {
                            if (bbox.zMin() > 0.0f + fTol) {
                                continue;
                            }
                        }
                        if (m_line_dir_minimum != FLT_MAX) {
                            if (bbox.zMin() > m_line_dir_minimum) {
                                continue;
                            }
                        }
                        target_index.emplace_back(group_index, bbox.zMin());
                    }
                }
                std::sort(target_index.begin(), target_index.end(),
                          [](const TargetIndexInfo& info0, const TargetIndexInfo& info1) {
                              return info0.m_box_dist_line_dir < info1.m_box_dist_line_dir;
                          });

                for (int index = 0; index < target_index.size(); ++index) {
                    int group_index = target_index[index].m_index;

                    if (m_line_dir_minimum != FLT_MAX) {
                        if (target_index[index].m_box_dist_line_dir > m_line_dir_minimum) {
                            /// ループ自体終わる
                            break;
                        }
                    }

                    int start_index = group_start[group_index];
                    int end_index;
                    if (group_index < group_start.size() - 1) {
                        end_index = group_start[group_index + 1];
                    }
                    else {
                        end_index = index_list.size();
                    }

                    for (int ic = start_index; ic < end_index; ic += 3) {
                        const unsigned int* tri_indices = &index_list[ic];

                        const auto point0 = cur_matrix * vertex_list[tri_indices[0]].m_position;
                        const auto point1 = cur_matrix * vertex_list[tri_indices[1]].m_position;
                        const auto point2 = cur_matrix * vertex_list[tri_indices[2]].m_position;

                        float dist_line_dir, dist_line_tol;
                        if (triaZeroLineInersection(point0, point1, point2, dist_line_dir, dist_line_tol, fTol)) {
                            Point3f triaNorm = (point1 - point0) ^ (point2 - point0);
                            triaNorm.normalize();

                            /// オプション　- 面上の接触を除外
                            if (m_except_on_plane) {
                                /// triaNorm * Point3f(0,0,1) -> Z成分
                                if (fabs(triaNorm[2]) <= m_except_on_plane_cos) {
                                    continue;
                                }
                            }

                            /// 最小の一つのみ取得
                            if (m_only_get_minimum) {
                                if (!intersetions.empty()) {
                                    if (dist_line_dir < intersetions[0].m_dist_line_dir) {
                                        temp.m_dist_line_dir = dist_line_dir;
                                        temp.m_dist_line_tol = dist_line_tol;
                                        temp.m_tria_index    = ic;
                                        temp.m_tria_norm     = triaNorm;
                                        intersetions[0]      = temp;
                                        m_line_dir_minimum   = dist_line_dir;
                                    }
                                    if (dist_line_dir == intersetions[0].m_dist_line_dir) {
                                        if (fabs(triaNorm[2]) > fabs(intersetions[0].m_tria_norm[2])) {
                                            temp.m_dist_line_dir = dist_line_dir;
                                            temp.m_dist_line_tol = dist_line_tol;
                                            temp.m_tria_index    = ic;
                                            temp.m_tria_norm     = triaNorm;
                                            intersetions[0]      = temp;
                                            m_line_dir_minimum   = dist_line_dir;
                                        }
                                    }
                                    continue;
                                }
                                m_line_dir_minimum = dist_line_dir;
                            }

                            temp.m_dist_line_dir = dist_line_dir;
                            temp.m_dist_line_tol = dist_line_tol;
                            temp.m_tria_index    = ic;
                            temp.m_tria_norm     = triaNorm;
                            intersetions.emplace_back(temp);
                        }
                    }
                }
            } break;
            default:
                break;
        }
    }
}

void Picking::bodyLineIntersection(const Matrix4x4f& inv_line_matrix, Node* node, bool only_visible,
                                   std::vector<SIntersectionInfo>& intersetions, float fTol, bool above_line_pos)
{
    if (only_visible) {
        if (!node->isVisible()) {
            return;
        }
    }

    Matrix4x4f cur_matrix = inv_line_matrix * node->matrix();

    auto renderable = node->renderable();
    if (renderable) {
        switch (renderable->type()) {
            case RenderableType::RenderEditableMesh: {
                auto bbox = cur_matrix.preMult(renderable->boundingBox());

                if (bbox.valid()) {
                    if (bbox.xMin() > 0.0f + fTol || bbox.xMax() < 0.0f - fTol || bbox.yMin() > 0.0f + fTol
                        || bbox.yMax() < 0.0f - fTol) {
                        return;
                    }
                    if (above_line_pos) {
                        if (bbox.zMin() > 0.0f + fTol) {
                            return;
                        }
                    }
                    if (m_line_dir_minimum != FLT_MAX) {
                        if (bbox.zMin() > m_line_dir_minimum) {
                            return;
                        }
                    }
                }

                RenderEditableMesh* mesh = (RenderEditableMesh*)renderable;

                const auto& vertex_list =
                    mesh->isEnableEditDisplayData() ? mesh->displayEditVertices() : mesh->displayVertices();
                const auto& index_list =
                    mesh->isEnableEditDisplayData() ? mesh->displayEditIndices() : mesh->displayIndices();

                SIntersectionInfo temp;
                temp.m_node = node;

                const auto& group_boxes = mesh->displayTriaGroupBox();
                const auto& group_start = mesh->displayTriaGroupStart();

                for (int group_index = 0; group_index < group_start.size(); ++group_index) {
                    auto bbox = inv_line_matrix.preMult(group_boxes[group_index]);

                    if (bbox.valid()) {
                        if (bbox.xMin() > 0.0f + fTol || bbox.xMax() < 0.0f - fTol || bbox.yMin() > 0.0f + fTol
                            || bbox.yMax() < 0.0f - fTol) {
                            continue;
                        }
                        if (above_line_pos) {
                            if (bbox.zMin() > 0.0f + fTol) {
                                continue;
                            }
                        }
                        if (m_line_dir_minimum != FLT_MAX) {
                            if (bbox.zMin() > m_line_dir_minimum) {
                                continue;
                            }
                        }
                    }

                    int start_index = group_start[group_index];
                    int end_index;
                    if (group_index < group_start.size() - 1) {
                        end_index = group_start[group_index + 1];
                    }
                    else {
                        end_index = index_list.size();
                    }

                    for (int ic = start_index; ic < end_index; ic += 3) {
                        const unsigned int* tri_indices = &index_list[ic];

                        const auto point0 = cur_matrix * vertex_list[tri_indices[0]];
                        const auto point1 = cur_matrix * vertex_list[tri_indices[1]];
                        const auto point2 = cur_matrix * vertex_list[tri_indices[2]];

                        float dist_line_dir, dist_line_tol;
                        if (triaZeroLineInersection(point0, point1, point2, dist_line_dir, dist_line_tol, fTol)) {
                            Point3f triaNorm = (point1 - point0) ^ (point2 - point0);
                            triaNorm.normalize();

                            /// オプション　- 面上の接触を除外
                            if (m_except_on_plane) {
                                /// triaNorm * Point3f(0,0,1) -> Z成分
                                if (fabs(triaNorm[2]) <= m_except_on_plane_cos) {
                                    continue;
                                }
                            }

                            /// 最小の一つのみ取得
                            if (m_only_get_minimum) {
                                if (!intersetions.empty()) {
                                    if (dist_line_dir < intersetions[0].m_dist_line_dir) {
                                        temp.m_dist_line_dir = dist_line_dir;
                                        temp.m_dist_line_tol = dist_line_tol;
                                        temp.m_tria_index    = ic;
                                        temp.m_tria_norm     = triaNorm;
                                        intersetions[0]      = temp;
                                        m_line_dir_minimum   = dist_line_dir;
                                    }
                                    if (dist_line_dir == intersetions[0].m_dist_line_dir) {
                                        if (fabs(triaNorm[2]) > fabs(intersetions[0].m_tria_norm[2])) {
                                            temp.m_dist_line_dir = dist_line_dir;
                                            temp.m_dist_line_tol = dist_line_tol;
                                            temp.m_tria_index    = ic;
                                            temp.m_tria_norm     = triaNorm;
                                            intersetions[0]      = temp;
                                            m_line_dir_minimum   = dist_line_dir;
                                        }
                                    }
                                    continue;
                                }
                                m_line_dir_minimum = dist_line_dir;
                            }

                            temp.m_dist_line_dir = dist_line_dir;
                            temp.m_dist_line_tol = dist_line_tol;
                            temp.m_tria_index    = ic;
                            temp.m_tria_norm     = triaNorm;
                            intersetions.emplace_back(temp);
                        }
                    }
                }
            } break;
            case RenderableType::RenderEditableNormalMesh: {
                auto bbox = cur_matrix.preMult(renderable->boundingBox());

                if (bbox.valid()) {
                    if (bbox.xMin() > 0.0f + fTol || bbox.xMax() < 0.0f - fTol || bbox.yMin() > 0.0f + fTol
                        || bbox.yMax() < 0.0f - fTol) {
                        return;
                    }
                    if (above_line_pos) {
                        if (bbox.zMin() > 0.0f + fTol) {
                            return;
                        }
                    }
                    if (m_line_dir_minimum != FLT_MAX) {
                        if (bbox.zMin() > m_line_dir_minimum) {
                            return;
                        }
                    }
                }

                RenderEditableNormalMesh* mesh = (RenderEditableNormalMesh*)renderable;

                const auto& vertex_list =
                    mesh->isEnableEditDisplayData() ? mesh->displayEditVertices() : mesh->displayVertices();
                const auto& index_list =
                    mesh->isEnableEditDisplayData() ? mesh->displayEditIndices() : mesh->displayIndices();

                SIntersectionInfo temp;
                temp.m_node = node;

                const auto& group_boxes = mesh->displayTriaGroupBox();
                const auto& group_start = mesh->displayTriaGroupStart();

                for (int group_index = 0; group_index < group_start.size(); ++group_index) {
                    auto bbox = inv_line_matrix.preMult(group_boxes[group_index]);

                    if (bbox.valid()) {
                        if (bbox.xMin() > 0.0f + fTol || bbox.xMax() < 0.0f - fTol || bbox.yMin() > 0.0f + fTol
                            || bbox.yMax() < 0.0f - fTol) {
                            continue;
                        }
                        if (above_line_pos) {
                            if (bbox.zMin() > 0.0f + fTol) {
                                continue;
                            }
                        }
                        if (m_line_dir_minimum != FLT_MAX) {
                            if (bbox.zMin() > m_line_dir_minimum) {
                                continue;
                            }
                        }
                    }

                    int start_index = group_start[group_index];
                    int end_index;
                    if (group_index < group_start.size() - 1) {
                        end_index = group_start[group_index + 1];
                    }
                    else {
                        end_index = index_list.size();
                    }

                    for (int ic = start_index; ic < end_index; ic += 3) {
                        const unsigned int* tri_indices = &index_list[ic];

                        const auto point0 = cur_matrix * vertex_list[tri_indices[0]].m_position;
                        const auto point1 = cur_matrix * vertex_list[tri_indices[1]].m_position;
                        const auto point2 = cur_matrix * vertex_list[tri_indices[2]].m_position;

                        float dist_line_dir, dist_line_tol;
                        if (triaZeroLineInersection(point0, point1, point2, dist_line_dir, dist_line_tol, fTol)) {
                            Point3f triaNorm = (point1 - point0) ^ (point2 - point0);
                            triaNorm.normalize();

                            /// オプション　- 面上の接触を除外
                            if (m_except_on_plane) {
                                /// triaNorm * Point3f(0,0,1) -> Z成分
                                if (fabs(triaNorm[2]) <= m_except_on_plane_cos) {
                                    continue;
                                }
                            }

                            /// 最小の一つのみ取得
                            if (m_only_get_minimum) {
                                if (!intersetions.empty()) {
                                    if (dist_line_dir < intersetions[0].m_dist_line_dir) {
                                        temp.m_dist_line_dir = dist_line_dir;
                                        temp.m_dist_line_tol = dist_line_tol;
                                        temp.m_tria_index    = ic;
                                        temp.m_tria_norm     = triaNorm;
                                        intersetions[0]      = temp;
                                        m_line_dir_minimum   = dist_line_dir;
                                    }
                                    if (dist_line_dir == intersetions[0].m_dist_line_dir) {
                                        if (fabs(triaNorm[2]) > fabs(intersetions[0].m_tria_norm[2])) {
                                            temp.m_dist_line_dir = dist_line_dir;
                                            temp.m_dist_line_tol = dist_line_tol;
                                            temp.m_tria_index    = ic;
                                            temp.m_tria_norm     = triaNorm;
                                            intersetions[0]      = temp;
                                            m_line_dir_minimum   = dist_line_dir;
                                        }
                                    }
                                    continue;
                                }
                                m_line_dir_minimum = dist_line_dir;
                            }

                            temp.m_dist_line_dir = dist_line_dir;
                            temp.m_dist_line_tol = dist_line_tol;
                            temp.m_tria_index    = ic;
                            temp.m_tria_norm     = triaNorm;
                            intersetions.emplace_back(temp);
                        }
                    }
                }
            } break;
            default:
                break;
        }
    }

    for (int ic = 0; ic < node->numChildren(); ++ic) {
        bodyLineIntersection(cur_matrix, node->child(ic), only_visible, intersetions, fTol, above_line_pos);
    }
}

void Picking::bodyLineIntersection(const Point3f& line_pos, const Point3f& line_vec, bool only_visible,
                                   std::vector<SIntersectionInfo>& intersetions, float fTol, bool above_line_pos,
                                   std::vector<Node*>* target_nodes)
{
    Point3f line_dir = line_vec;
    line_dir.normalize();

    Point3f axis(0.0f, 0.0f, 1.0f);    // 2
    if (fabs(line_dir[2]) > fabs(line_dir[1])) {
        if (fabs(line_dir[2]) > fabs(line_dir[0])) {
            axis.set(1.0f, 0.0f, 0.0f);    // 2以外
        }
    }

    Point3f axis2 = line_dir ^ axis;
    axis          = axis2 ^ line_dir;

    Matrix4x4f line_matrix;
    line_matrix.setRow(0, axis[0], axis[1], axis[2], 0.0f);
    line_matrix.setRow(1, axis2[0], axis2[1], axis2[2], 0.0f);
    line_matrix.setRow(2, line_dir[0], line_dir[1], line_dir[2], 0.0f);
    line_matrix.setRow(3, line_pos[0], line_pos[1], line_pos[2], 1.0f);

    Matrix4x4f inv_line_matrix = line_matrix.inverted();

    if (m_only_get_minimum) {
        m_line_dir_minimum = FLT_MAX;
        Node* root_node    = m_scene_view->sceneGraph()->rootNode();

        std::vector<TargetNodeInfo> targets;
        if (target_nodes) {
            bodyLineIntersectionMinimumTarget(inv_line_matrix, *target_nodes, only_visible, fTol, above_line_pos,
                                              targets);
        }
        else {
            bodyLineIntersectionMinimumTarget(inv_line_matrix, root_node, only_visible, fTol, above_line_pos, targets);
        }
        bodyLineIntersectionMinimum(targets, fTol, above_line_pos, intersetions);
    }
    else {
        m_line_dir_minimum = FLT_MAX;
        Node* root_node    = m_scene_view->sceneGraph()->rootNode();
        bodyLineIntersection(inv_line_matrix, root_node, only_visible, intersetions, fTol, above_line_pos);
    }
}

void Picking::bodyLineIntersection(const Point3f& line_pos, const Point3f& line_vec, Node* target_node,
                                   std::vector<SIntersectionInfo>& intersetions, float fTol, bool above_line_pos)
{
    Point3f line_dir = line_vec;
    line_dir.normalize();

    Point3f axis(0.0f, 0.0f, 1.0f);    // 2
    if (fabs(line_dir[2]) > fabs(line_dir[1])) {
        if (fabs(line_dir[2]) > fabs(line_dir[0])) {
            axis.set(1.0f, 0.0f, 0.0f);    // 2以外
        }
    }

    Point3f axis2 = line_dir ^ axis;
    axis          = axis2 ^ line_dir;

    Matrix4x4f line_matrix;
    line_matrix.setRow(0, axis[0], axis[1], axis[2], 0.0f);
    line_matrix.setRow(1, axis2[0], axis2[1], axis2[2], 0.0f);
    line_matrix.setRow(2, line_dir[0], line_dir[1], line_dir[2], 0.0f);
    line_matrix.setRow(3, line_pos[0], line_pos[1], line_pos[2], 1.0f);

    Matrix4x4f inv_line_matrix = line_matrix.inverted();

    m_line_dir_minimum    = FLT_MAX;
    Matrix4x4f cur_matrix = inv_line_matrix * target_node->parentPathMatrix();
    bodyLineIntersection(cur_matrix, target_node, true, intersetions, fTol, above_line_pos);
}

bool Picking::triaZeroLineInersection(const Point3f& v0, const Point3f& v1, const Point3f& v2, float& dist_line_dir,
                                      float& dist_line_tol, float fTol)
{
    /// Triangle Box
    float box2d[4] = {min(v0[0], min(v1[0], v2[0])), min(v0[1], min(v1[1], v2[1])), max(v0[0], max(v1[0], v2[0])),
                      max(v0[1], max(v1[1], v2[1]))};

    // 2次元で三角形が原点を内包するか（誤差考慮）
    if (box2d[0] <= fTol && box2d[2] >= -fTol && box2d[0] <= fTol && box2d[2] >= -fTol && box2d[1] <= fTol
        && box2d[3] >= -fTol && box2d[1] <= fTol && box2d[3] >= -fTol) {
        float f0 = (v1[0] - v0[0]) * (-v0[1]) - (v1[1] - v0[1]) * (-v0[0]);
        float f1 = (v2[0] - v1[0]) * (-v1[1]) - (v2[1] - v1[1]) * (-v1[0]);
        float f2 = (v0[0] - v2[0]) * (-v2[1]) - (v0[1] - v2[1]) * (-v2[0]);

        // 三角形が原点を内包する
        if ((f0 >= 0.0f && f1 >= 0.0f && f2 >= 0.0f) || (f0 <= 0.0f && f1 <= 0.0f && f2 <= 0.0f)) {
            // 三角形上のZ値
            Point3f triaNorm = (v1 - v0) ^ (v2 - v0);
            float   fPlane   = triaNorm[0] * v0[0] + triaNorm[1] * v0[1] + triaNorm[2] * v0[2];
            if (triaNorm[2] != 0.0f)    // つぶれていない
            {
                dist_line_dir = fPlane / triaNorm[2];    // Z値
                dist_line_tol = 0.0f;
                return true;
            }
        }

        // 三角形の各エッジと直線の距離
        if (fTol > 0.0f) {
            float t1, t2, t3, fPos[2];
            float fDist1 = dist2ZeroSegment2D(v0, v1, t1, fPos);
            float fDist2 = dist2ZeroSegment2D(v1, v2, t2, fPos);
            float fDist3 = dist2ZeroSegment2D(v2, v0, t3, fPos);

            if (fDist1 < fDist2)    // 1 < 2
            {
                if (fDist3 < fDist1)    // 3 < 1
                {
                    // 3
                    if (fDist3 <= fTol * fTol) {
                        Point3f vec   = v2 + (v0 - v2) * t3;
                        dist_line_dir = vec[2];
                        dist_line_tol = fDist3;
                        return true;
                    }
                }
                else    // 1 <= 3
                {
                    // 1
                    if (fDist1 <= fTol * fTol) {
                        Point3f vec   = v0 + (v1 - v0) * t1;
                        dist_line_dir = vec[2];
                        dist_line_tol = fDist1;
                        return true;
                    }
                }
            }
            else    // 2 <= 1
            {
                if (fDist3 < fDist2)    // 3 < 2
                {
                    // 3
                    if (fDist3 <= fTol * fTol) {
                        Point3f vec   = v2 + (v0 - v2) * t3;
                        dist_line_dir = vec[2];
                        dist_line_tol = fDist3;
                        return true;
                    }
                }
                else {
                    // 2
                    if (fDist2 <= fTol * fTol) {
                        Point3f vec   = v1 + (v2 - v1) * t2;
                        dist_line_dir = vec[2];
                        dist_line_tol = fDist2;
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool Picking::SIntersectionInfo::orgTria(bool global, Point3f& point0, Point3f& point1, Point3f& point2)
{
    if (!m_node) {
        return false;
    }

    auto renderable = m_node->renderable();
    if (!renderable) {
        return false;
    }

    switch (renderable->type()) {
        case RenderableType::RenderEditableMesh: {
            RenderEditableMesh* mesh = (RenderEditableMesh*)renderable;

            const auto& vertex_list =
                mesh->isEnableEditDisplayData() ? mesh->displayEditVertices() : mesh->displayVertices();
            const auto& index_list =
                mesh->isEnableEditDisplayData() ? mesh->displayEditIndices() : mesh->displayIndices();

            if (m_tria_index >= 0 && m_tria_index < index_list.size()) {
                const unsigned int* tri_indices = &index_list[m_tria_index];

                if (global) {
                    const Matrix4x4f& path_matrix = m_node->pathMatrix();

                    point0 = path_matrix * vertex_list[tri_indices[0]];
                    point1 = path_matrix * vertex_list[tri_indices[1]];
                    point2 = path_matrix * vertex_list[tri_indices[2]];
                }
                else {
                    point0 = vertex_list[tri_indices[0]];
                    point1 = vertex_list[tri_indices[1]];
                    point2 = vertex_list[tri_indices[2]];
                }
                return true;
            }
        } break;
        case RenderableType::RenderEditableNormalMesh: {
            RenderEditableNormalMesh* mesh = (RenderEditableNormalMesh*)renderable;

            const auto& vertex_list =
                mesh->isEnableEditDisplayData() ? mesh->displayEditVertices() : mesh->displayVertices();
            const auto& index_list =
                mesh->isEnableEditDisplayData() ? mesh->displayEditIndices() : mesh->displayIndices();

            if (m_tria_index >= 0 && m_tria_index < index_list.size()) {
                const unsigned int* tri_indices = &index_list[m_tria_index];

                if (global) {
                    const Matrix4x4f& path_matrix = m_node->pathMatrix();

                    point0 = path_matrix * vertex_list[tri_indices[0]].m_position;
                    point1 = path_matrix * vertex_list[tri_indices[1]].m_position;
                    point2 = path_matrix * vertex_list[tri_indices[2]].m_position;
                }
                else {
                    point0 = vertex_list[tri_indices[0]].m_position;
                    point1 = vertex_list[tri_indices[1]].m_position;
                    point2 = vertex_list[tri_indices[2]].m_position;
                }
                return true;
            }
        } break;
    }

    return false;
}

Point3f Picking::SIntersectionInfo::orgNorm(bool global)
{
    if (!m_node) {
        return Point3f();
    }

    auto renderable = m_node->renderable();
    if (!renderable) {
        return Point3f();
    }

    switch (renderable->type()) {
        case RenderableType::RenderEditableMesh: {
            RenderEditableMesh* mesh = (RenderEditableMesh*)renderable;

            const auto& vertex_list =
                mesh->isEnableEditDisplayData() ? mesh->displayEditVertices() : mesh->displayVertices();
            const auto& index_list =
                mesh->isEnableEditDisplayData() ? mesh->displayEditIndices() : mesh->displayIndices();

            if (m_tria_index >= 0 && m_tria_index < index_list.size()) {
                const unsigned int* tri_indices = &index_list[m_tria_index];

                Point3f point0, point1, point2;
                if (global) {
                    const Matrix4x4f& path_matrix = m_node->pathMatrix();

                    point0 = path_matrix * vertex_list[tri_indices[0]];
                    point1 = path_matrix * vertex_list[tri_indices[1]];
                    point2 = path_matrix * vertex_list[tri_indices[2]];
                }
                else {
                    point0 = vertex_list[tri_indices[0]];
                    point1 = vertex_list[tri_indices[1]];
                    point2 = vertex_list[tri_indices[2]];
                }

                Point3f triaNorm = (point1 - point0) ^ (point2 - point0);
                triaNorm.normalize();
                return triaNorm;
            }
        } break;
        case RenderableType::RenderEditableNormalMesh: {
            RenderEditableNormalMesh* mesh = (RenderEditableNormalMesh*)renderable;

            const auto& vertex_list =
                mesh->isEnableEditDisplayData() ? mesh->displayEditVertices() : mesh->displayVertices();
            const auto& index_list =
                mesh->isEnableEditDisplayData() ? mesh->displayEditIndices() : mesh->displayIndices();

            if (m_tria_index >= 0 && m_tria_index < index_list.size()) {
                const unsigned int* tri_indices = &index_list[m_tria_index];

                Point3f point0, point1, point2;
                if (global) {
                    const Matrix4x4f& path_matrix = m_node->pathMatrix();

                    point0 = path_matrix * vertex_list[tri_indices[0]].m_position;
                    point1 = path_matrix * vertex_list[tri_indices[1]].m_position;
                    point2 = path_matrix * vertex_list[tri_indices[2]].m_position;
                }
                else {
                    point0 = vertex_list[tri_indices[0]].m_position;
                    point1 = vertex_list[tri_indices[1]].m_position;
                    point2 = vertex_list[tri_indices[2]].m_position;
                }

                /// とりあえず三角形の法線を返す（normalの平均返す？）
                Point3f triaNorm = (point1 - point0) ^ (point2 - point0);
                triaNorm.normalize();
                return triaNorm;
            }
        } break;
    }

    return Point3f();
}

CORE_NAMESPACE_END
