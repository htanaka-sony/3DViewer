#include "Clipping.h"

#include <unordered_map>
#include <unordered_set>

#include "Scene/Voxel.h"
#include "Scene/VoxelScalar.h"

// #ifdef USE_QT
//     #include <QFile>
// #endif

#include "earcut.hpp"
using ECPolygon  = std::vector<std::array<float, 2>>;
using ECPolygons = std::vector<ECPolygon>;

#include <execution>

CORE_NAMESPACE_BEGIN

std::vector<Clipping::ClippingWork*> Clipping::m_work_list;

Clipping::Clipping(SceneGraph* scene_graph) : m_scene_graph(scene_graph)
{
    if (m_scene_graph) {
        const auto& length_unit = m_scene_graph->lengthUnit();

        m_valid_eps    = length_unit.epsilonValidLength();
        m_zero_eps     = length_unit.epsilonZero();
        m_valid_eps_sq = m_valid_eps * m_valid_eps;
        m_zero_eps_sq  = m_zero_eps * m_zero_eps;
    }
    else {
        m_valid_eps    = 0.0f;
        m_zero_eps     = 0.0f;
        m_valid_eps_sq = 0.0f;
        m_zero_eps_sq  = 0.0f;
    }
}

Clipping::~Clipping() {}

void Clipping::execute(const VecPlanef& planes, bool only_visible)
{
    if (!m_scene_graph) {
        return;
    }
    std::vector<TargetNode> target_list;
    collectTarget(m_scene_graph->rootNode(), m_scene_graph->rootNode()->matrix(), target_list, only_visible);

    std::vector<PlaneInfo> plane_info_list;
    for (auto& plane : planes) {
        plane_info_list.emplace_back(plane);
    }
    for (auto& plane_info : plane_info_list) {
        plane_info.setPlaneInfo();
    }

    execute(plane_info_list, target_list);
}

void Clipping::execute(const std::vector<PlaneInfo>& plane_info_list, std::vector<TargetNode>& target_list)
{
    MutexWrapper  clipping_mutex;
    std::set<int> m_work_unused;
    for (int ic = 0; ic < m_work_list.size(); ++ic) {
        m_work_unused.insert(ic);
    }

    /// 暫定 - 2D面を最初にする（その方が効率よいはず）
    std::vector<PlaneInfo> sorted_plane_info_list;
    for (const auto& plane_info : plane_info_list) {
        if (plane_info.m_only_get_plane) {
            sorted_plane_info_list.emplace_back(plane_info);
        }
    }
    for (const auto& plane_info : plane_info_list) {
        if (!plane_info.m_only_get_plane) {
            sorted_plane_info_list.emplace_back(plane_info);
        }
    }

    /// 並列化する
    std::for_each(std::execution::par, target_list.begin(), target_list.end(), [&](TargetNode& target) {
        // for (auto& target : target_list) {
        auto& node        = target.m_node;
        auto& path_matrix = target.m_path_matrix;

        auto object = node->object();
        if (!object) {
            /// 並列処理なので
            // continue;
            return;
        }

        auto renderable = node->renderable();
        if (!renderable) {
            /// 並列処理なので
            // continue;
            return;
        }

        /// 高速化のための作業領域使いまわし
        ClippingWork* work_space       = nullptr;
        int           work_space_index = -1;
        {
            clipping_mutex.lock();

            if (m_work_unused.size() > 0) {
                work_space_index = *m_work_unused.begin();
                m_work_unused.erase(m_work_unused.begin());
            }
            else {
                work_space = new ClippingWork;
                m_work_list.emplace_back(work_space);
                work_space_index = m_work_list.size() - 1;
            }

            work_space = m_work_list[work_space_index];

            clipping_mutex.unlock();
        }

        /// 対象のパスマトリックスを考慮して平面方程式をそのノードのローカルに変換
        auto target_plane_info_list = sorted_plane_info_list;
        if (!path_matrix.isIdentity()) {
            for (auto& target_plane : target_plane_info_list) {
                /// 平面方程式をローカル座標系に変換
                target_plane.m_plane = path_matrix.inverted() * target_plane.m_plane;
                target_plane.setPlaneInfo();
            }
        }

        switch (renderable->type()) {
            case RenderableType::RenderEditableMesh: {
                RenderEditableMesh* mesh = (RenderEditableMesh*)renderable;

                bool create_segment = mesh->isCreateSectionLine();

                if (mesh->isEnableEditDisplayData()) {
                    mesh->clearDisplayEditData();
                    mesh->markRenderDirty();
                    mesh->markSegmentsGroupDirty();
                    mesh->markBoxDirty();
                }

                bool after_2d_clip = false;

                for (auto plane_info : target_plane_info_list) {
                    std::vector<Point3f>&      vertex_list  = mesh->displayEditVerticesTemp();
                    std::vector<unsigned int>& index_list   = mesh->displayEditIndicesTemp();
                    std::vector<unsigned int>& segment_list = mesh->displayEditSegmentIndicesTemp();
                    vertex_list.clear();
                    index_list.clear();
                    segment_list.clear();

                    if (plane_info.m_only_get_plane) {
                        after_2d_clip = true;
                    }

                    /// VoxelScalar 2D Textureの場合
                    bool data_is_2d  = false;
                    bool create_fill = true;
                    if (object->type() == ObjectType::VoxelScalar) {
                        data_is_2d  = ((VoxelScalar*)object)->is2DTexture();
                        create_fill = !data_is_2d;
                    }

                    /// 2Dの場合は許容値を大きくする（解析結果断面の表示で消えてしまうので）
                    float valid_eps = (plane_info.m_only_get_plane || data_is_2d) ? m_valid_eps * 2 : m_valid_eps / 2.0;

                    auto check_plane = mesh->boundingBox().checkOverlapWithPlane(plane_info.m_plane, valid_eps);
                    if (check_plane == PlaneOverlap::Above) {
                        if (!plane_info.m_only_get_plane) {
                            /// 何もしない（すべて残す）
                            continue;
                        }
                        else {
                            /// トライアングルなし
                            mesh->clearDisplayEditData();
                            mesh->setEnalbeEditDisplayData(true);
                            mesh->markRenderDirty();
                            mesh->markSegmentsGroupDirty();
                            mesh->markBoxDirty();
                            break;
                        }
                    }
                    else if (check_plane == PlaneOverlap::AboveOnPlane) {
                        if (!plane_info.m_only_get_plane) {
                            /// 何もしない（すべて残す）
                            continue;
                        }
                    }
                    else if (check_plane == PlaneOverlap::Below) {
                        /// トライアングルなし
                        mesh->clearDisplayEditData();
                        mesh->setEnalbeEditDisplayData(true);
                        mesh->markRenderDirty();
                        mesh->markSegmentsGroupDirty();
                        mesh->markBoxDirty();
                        break;
                    }
                    else if (check_plane == PlaneOverlap::BelowOnPlane) {
                        if (!plane_info.m_only_get_plane) {
                            /// トライアングルなし
                            mesh->clearDisplayEditData();
                            mesh->setEnalbeEditDisplayData(true);
                            mesh->markRenderDirty();
                            mesh->markSegmentsGroupDirty();
                            mesh->markBoxDirty();
                            break;
                        }
                    }
                    else if (check_plane == PlaneOverlap::OnPlane) {
                        /// すべて平面上ならそのまま残す
                        continue;
                    }

                    /// 分割する
                    const auto& in_vertex_list =
                        mesh->isEnableEditDisplayData() ? mesh->displayEditVertices() : mesh->displayVertices();
                    const auto& in_index_list =
                        mesh->isEnableEditDisplayData() ? mesh->displayEditIndices() : mesh->displayIndices();
                    const auto& in_segment_list = mesh->isEnableEditDisplayData() ? mesh->displayEditSegmentIndices()
                                                                                  : mesh->displaySegmentIndices();

                    vertex_list.reserve(in_vertex_list.size());
                    index_list.reserve(in_index_list.size());
                    segment_list.reserve(in_segment_list.size());

                    if (plane_info.m_only_get_plane) {
                        if (clippingOnlyPlane(in_vertex_list, in_index_list, in_segment_list, plane_info, vertex_list,
                                              index_list, segment_list, create_segment, create_fill, work_space)) {
                            mesh->setEnalbeEditDisplayData(true);
                            mesh->switchEditDisplayBuffer();
                            mesh->markRenderDirty();
                            mesh->markSegmentsGroupDirty();
                            mesh->markBoxDirty();
                        }
                        else {
                            /// トライアングルなし
                            mesh->clearDisplayEditData();
                            mesh->setEnalbeEditDisplayData(true);
                            mesh->markRenderDirty();
                            mesh->markSegmentsGroupDirty();
                            mesh->markBoxDirty();
                            break;
                        }
                    }
                    else {
                        if (clipping(in_vertex_list, in_index_list, in_segment_list, plane_info, vertex_list,
                                     index_list, segment_list, create_segment, create_fill, after_2d_clip,
                                     work_space)) {
                            mesh->setEnalbeEditDisplayData(true);
                            mesh->switchEditDisplayBuffer();
                            mesh->markRenderDirty();
                            mesh->markSegmentsGroupDirty();
                            mesh->markBoxDirty();
                        }
                    }
                }

                {
                    /// Clear
                    std::vector<Point3f>&      vertex_list  = mesh->displayEditVerticesTemp();
                    std::vector<unsigned int>& index_list   = mesh->displayEditIndicesTemp();
                    std::vector<unsigned int>& segment_list = mesh->displayEditSegmentIndicesTemp();
                    vertex_list.clear();
                    std::vector<Point3f> temp;
                    vertex_list.swap(temp);
                    index_list.clear();
                    std::vector<unsigned int> temp2;
                    index_list.swap(temp2);
                    segment_list.clear();
                    std::vector<unsigned int> temp3;
                    segment_list.swap(temp3);
                }
            } break;

            case RenderableType::RenderEditableNormalMesh: {
                RenderEditableNormalMesh* mesh = (RenderEditableNormalMesh*)renderable;

                bool create_segment = mesh->isCreateSectionLine();

                if (mesh->isEnableEditDisplayData()) {
                    mesh->clearDisplayEditData();
                    mesh->markRenderDirty();
                    mesh->markSegmentsGroupDirty();
                    mesh->markBoxDirty();
                }

                bool after_2d_clip = false;

                for (auto plane_info : target_plane_info_list) {
                    std::vector<Vertexf>&      vertex_list  = mesh->displayEditVerticesTemp();
                    std::vector<unsigned int>& index_list   = mesh->displayEditIndicesTemp();
                    std::vector<unsigned int>& segment_list = mesh->displayEditSegmentIndicesTemp();
                    vertex_list.clear();
                    index_list.clear();
                    segment_list.clear();

                    if (plane_info.m_only_get_plane) {
                        after_2d_clip = true;
                    }

                    float valid_eps = plane_info.m_only_get_plane ? m_valid_eps * 2 : m_valid_eps / 2.0;

                    auto check_plane = mesh->boundingBox().checkOverlapWithPlane(plane_info.m_plane, valid_eps);
                    if (check_plane == PlaneOverlap::Above) {
                        if (!plane_info.m_only_get_plane) {
                            /// 何もしない（すべて残す）
                            continue;
                        }
                        else {
                            /// トライアングルなし
                            mesh->clearDisplayEditData();
                            mesh->setEnalbeEditDisplayData(true);
                            mesh->markRenderDirty();
                            mesh->markSegmentsGroupDirty();
                            mesh->markBoxDirty();
                            break;
                        }
                    }
                    else if (check_plane == PlaneOverlap::AboveOnPlane) {
                        if (!plane_info.m_only_get_plane) {
                            /// 何もしない（すべて残す）
                            continue;
                        }
                    }
                    else if (check_plane == PlaneOverlap::Below) {
                        /// トライアングルなし
                        mesh->clearDisplayEditData();
                        mesh->setEnalbeEditDisplayData(true);
                        mesh->markRenderDirty();
                        mesh->markSegmentsGroupDirty();
                        mesh->markBoxDirty();
                        break;
                    }
                    else if (check_plane == PlaneOverlap::BelowOnPlane) {
                        if (!plane_info.m_only_get_plane) {
                            /// トライアングルなし
                            mesh->clearDisplayEditData();
                            mesh->setEnalbeEditDisplayData(true);
                            mesh->markRenderDirty();
                            mesh->markSegmentsGroupDirty();
                            mesh->markBoxDirty();
                            break;
                        }
                    }
                    else if (check_plane == PlaneOverlap::OnPlane) {
                        /// すべて平面上ならそのまま残す
                        continue;
                    }

                    /// 分割する
                    const auto& in_vertex_list =
                        mesh->isEnableEditDisplayData() ? mesh->displayEditVertices() : mesh->displayVertices();
                    const auto& in_index_list =
                        mesh->isEnableEditDisplayData() ? mesh->displayEditIndices() : mesh->displayIndices();
                    const auto& in_segment_list = mesh->isEnableEditDisplayData()
                                                      ? mesh->displayEditSegmentIndices()
                                                      : mesh->displaySegmentIndices();

                    vertex_list.reserve(in_vertex_list.size());
                    index_list.reserve(in_index_list.size());
                    segment_list.reserve(in_segment_list.size());

                    if (plane_info.m_only_get_plane) {
                        if (clippingOnlyPlaneNormal(in_vertex_list, in_index_list, in_segment_list, plane_info,
                                                    vertex_list, index_list, segment_list, create_segment, true,
                                                    work_space)) {
                            mesh->setEnalbeEditDisplayData(true);
                            mesh->switchEditDisplayBuffer();
                            mesh->markRenderDirty();
                            mesh->markSegmentsGroupDirty();
                            mesh->markBoxDirty();
                        }
                        else {
                            /// トライアングルなし
                            mesh->clearDisplayEditData();
                            mesh->setEnalbeEditDisplayData(true);
                            mesh->markRenderDirty();
                            mesh->markSegmentsGroupDirty();
                            mesh->markBoxDirty();
                            break;
                        }
                    }
                    else {
                        if (clippingNormal(in_vertex_list, in_index_list, in_segment_list, plane_info, vertex_list,
                                           index_list, segment_list, create_segment, true, after_2d_clip,
                                           work_space)) {
                            mesh->setEnalbeEditDisplayData(true);
                            mesh->switchEditDisplayBuffer();
                            mesh->markRenderDirty();
                            mesh->markSegmentsGroupDirty();
                            mesh->markBoxDirty();
                        }
                    }
                }

                {
                    /// Clear
                    std::vector<Vertexf>&      vertex_list  = mesh->displayEditVerticesTemp();
                    std::vector<unsigned int>& index_list   = mesh->displayEditIndicesTemp();
                    std::vector<unsigned int>& segment_list = mesh->displayEditSegmentIndicesTemp();
                    vertex_list.clear();
                    std::vector<Vertexf> temp;
                    vertex_list.swap(temp);
                    index_list.clear();
                    std::vector<unsigned int> temp2;
                    index_list.swap(temp2);
                    segment_list.clear();
                    std::vector<unsigned int> temp3;
                    segment_list.swap(temp3);
                }
            } break;
        }

        clipping_mutex.lock();
        m_work_unused.insert(work_space_index);
        node->markBoxDirty();
        clipping_mutex.unlock();
        //}
    });
}

bool Clipping::clear(Node* node, bool only_visible)
{
    if (!node) {
        node = m_scene_graph->rootNode();
    }

    if (only_visible) {
        if (!node->isVisible()) {
            return false;
        }
    }

    bool ret = false;
    if (node->shape()) {
        auto renderable = node->renderable();
        if (renderable) {
            switch (renderable->type()) {
                case RenderableType::RenderEditableMesh: {
                    RenderEditableMesh* mesh = (RenderEditableMesh*)renderable;
                    if (mesh->isEnableEditDisplayData()) {
                        mesh->clearDisplayEditData();
                        mesh->markRenderDirty();
                        mesh->markSegmentsGroupDirty();
                        mesh->markBoxDirty();
                        node->markBoxDirty();
                        ret = true;
                    }
                    break;
                }
                case RenderableType::RenderEditableNormalMesh: {
                    RenderEditableNormalMesh* mesh = (RenderEditableNormalMesh*)renderable;
                    if (mesh->isEnableEditDisplayData()) {
                        mesh->clearDisplayEditData();
                        mesh->markRenderDirty();
                        mesh->markSegmentsGroupDirty();
                        mesh->markBoxDirty();
                        node->markBoxDirty();
                        ret = true;
                    }
                    break;
                }
            }
        }
        /*
        auto object = node->object();
        switch (object->type()) {
            case ObjectType::Voxel:
            case ObjectType::VoxelScalar: {
                Voxel* voxel = (Voxel*)object;
                if (voxel->isEnableEditDisplayData()) {
                    voxel->clearDisplayEditData();
                    voxel->markRenderDirty();
                    voxel->markSegmentsGroupDirty();
                    voxel->markBoxDirty();
                    node->markBoxDirty();
                    ret = true;
                }
                break;
            }
        }
        */
    }

    for (auto child : node->children()) {
        if (clear(child.ptr(), only_visible)) {
            ret = true;
        }
    }

    return ret;
}

void Clipping::deleteWorkMemory()
{
    for (auto& work : m_work_list) {
        delete work;
    }
    m_work_list.clear();
}

bool Clipping::clipping(const std::vector<Point3f>& in_vertices, const std::vector<unsigned int>& in_indices,
                        const std::vector<unsigned int>& in_segments, const PlaneInfo& plane_info,
                        std::vector<Point3f>& out_vertices, std::vector<unsigned int>& out_indices,
                        std::vector<unsigned int>& out_segments, bool create_segment, bool create_fill,
                        bool after_2d_clip, ClippingWork* work_space)
{
    std::vector<EdgeInfo> edge_list;
    if (!clipPolygonWithPlane(in_vertices, in_indices, in_segments, plane_info, out_vertices, out_indices, out_segments,
                              create_segment, create_fill, false, after_2d_clip, edge_list, work_space)) {
        return false;
    }
    if (out_indices.empty()) {
        return true;
    }
    if (after_2d_clip) {
        return true;
    }

    std::vector<ClippingLoop*> contours;
    createContour(edge_list, contours, work_space);

    std::vector<std::vector<ClippingLoop*>> outlines;
    outlinesFromLoops(contours, outlines, work_space);
    for (auto& outline : outlines) {
        triangleFromOutline(outline, out_vertices, plane_info, out_indices, work_space);
        /// 輪郭追加
        if (create_segment) {
            addSegents(outline, out_segments);
        }
    }

    for (auto& loop : contours) {
        delete loop;
    }

    return true;
}

bool Clipping::clippingOnlyPlane(const std::vector<Point3f>& in_vertices, const std::vector<unsigned int>& in_indices,
                                 const std::vector<unsigned int>& in_segments, const PlaneInfo& plane_info,
                                 std::vector<Point3f>& out_vertices, std::vector<unsigned int>& out_indices,
                                 std::vector<unsigned int>& out_segments, bool create_segment, bool create_fill,
                                 ClippingWork* work_space)
{
    work_space->m_work_vertex_list.clear();

    std::vector<EdgeInfo> edge_list;
    if (!clipPolygonWithPlane(in_vertices, in_indices, in_segments, plane_info, work_space->m_work_vertex_list,
                              out_indices, out_segments, create_segment, create_fill, true, false, edge_list,
                              work_space)) {
        return false;
    }

    std::vector<ClippingLoop*> contours;
    createContour(edge_list, contours, work_space);

    /// 元の頂点と最終的な頂点のIndex対応
    std::map<int, int> map_old_new;

    std::vector<std::vector<ClippingLoop*>> outlines;
    outlinesFromLoops(contours, outlines, work_space);
    for (auto& outline : outlines) {
        triangleFromOutline(outline, work_space->m_work_vertex_list, out_vertices, map_old_new, plane_info, out_indices,
                            work_space);
        /// 輪郭追加
        if (create_segment) {
            addSegents(outline, out_segments, work_space->m_work_vertex_list, out_vertices, map_old_new);
        }
    }

    for (auto& loop : contours) {
        delete loop;
    }

    return true;
}

bool Clipping::clippingForAutoLength(const std::vector<Point3f>&      in_vertices,
                                     const std::vector<unsigned int>& in_indices, const PlaneInfo& plane_info,
                                     std::vector<Point3f>& out_vertices, std::vector<unsigned int>& out_indices,
                                     ClippingWork* work_space)
{
    std::vector<EdgeInfo>     edge_list;
    std::vector<unsigned int> dummy;
    if (!clipPolygonWithPlane(in_vertices, in_indices, dummy, plane_info, out_vertices, out_indices, dummy, false, true,
                              false, false, edge_list, work_space)) {
        return false;
    }
    if (out_indices.empty()) {
        return false;
    }

    std::vector<ClippingLoop*> contours;
    createContour(edge_list, contours, work_space);

    std::vector<std::vector<ClippingLoop*>> outlines;
    outlinesFromLoops(contours, outlines, work_space);
    for (auto& outline : outlines) {
        triangleFromOutline(outline, out_vertices, plane_info, out_indices, work_space);
    }

    for (auto& loop : contours) {
        delete loop;
    }

    return true;
}

bool Clipping::clippingForAutoLengthNormal(const std::vector<Vertexf>&      in_vertices,
                                           const std::vector<unsigned int>& in_indices, const PlaneInfo& plane_info,
                                           std::vector<Vertexf>& out_vertices, std::vector<unsigned int>& out_indices,
                                           ClippingWork* work_space)
{
    std::vector<EdgeInfo>     edge_list;
    std::vector<unsigned int> dummy;
    if (!clipPolygonWithPlane(in_vertices, in_indices, dummy, plane_info, out_vertices, out_indices, dummy, false, true,
                              false, false, edge_list, work_space)) {
        return false;
    }
    if (out_indices.empty()) {
        return false;
    }

    std::vector<ClippingLoop*> contours;
    createContour(edge_list, contours, work_space);

    std::vector<std::vector<ClippingLoop*>> outlines;
    outlinesFromLoops(contours, outlines, work_space);
    for (auto& outline : outlines) {
        triangleFromOutline(outline, out_vertices, plane_info, out_indices, work_space);
    }

    for (auto& loop : contours) {
        delete loop;
    }

    return true;
}

bool Clipping::clippingNormal(const std::vector<Vertexf>& in_vertices, const std::vector<unsigned int>& in_indices,
                              const std::vector<unsigned int>& in_segments, const PlaneInfo& plane_info,
                              std::vector<Vertexf>& out_vertices, std::vector<unsigned int>& out_indices,
                              std::vector<unsigned int>& out_segments, bool create_segment, bool create_fill,
                              bool after_2d_clip, ClippingWork* work_space)
{
    std::vector<EdgeInfo> edge_list;
    if (!clipPolygonWithPlane(in_vertices, in_indices, in_segments, plane_info, out_vertices, out_indices, out_segments,
                              create_segment, create_fill, false, after_2d_clip, edge_list, work_space)) {
        return false;
    }
    if (out_indices.empty()) {
        return true;
    }
    if (after_2d_clip) {
        return true;
    }

    std::vector<ClippingLoop*> contours;
    createContour(edge_list, contours, work_space);

    std::vector<std::vector<ClippingLoop*>> outlines;
    outlinesFromLoops(contours, outlines, work_space);
    for (auto& outline : outlines) {
        triangleFromOutline(outline, out_vertices, plane_info, out_indices, work_space);
        /// 輪郭追加
        if (create_segment) {
            addSegents(outline, out_segments);
        }
    }

    for (auto& loop : contours) {
        delete loop;
    }

    return true;
}

bool Clipping::clippingOnlyPlaneNormal(const std::vector<Vertexf>& in_vertices,
                                       const std::vector<unsigned int>& in_indices,
                                       const std::vector<unsigned int>& in_segments, const PlaneInfo& plane_info,
                                       std::vector<Vertexf>& out_vertices, std::vector<unsigned int>& out_indices,
                                       std::vector<unsigned int>& out_segments, bool create_segment, bool create_fill,
                                       ClippingWork* work_space)
{
    work_space->m_work_vertex_list_normal.clear();

    std::vector<EdgeInfo> edge_list;
    if (!clipPolygonWithPlane(in_vertices, in_indices, in_segments, plane_info, work_space->m_work_vertex_list_normal,
                              out_indices, out_segments, create_segment, create_fill, true, false, edge_list,
                              work_space)) {
        return false;
    }

    std::vector<ClippingLoop*> contours;
    createContour(edge_list, contours, work_space);

    /// 元の頂点と最終的な頂点のIndex対応
    std::map<int, int> map_old_new;

    std::vector<std::vector<ClippingLoop*>> outlines;
    outlinesFromLoops(contours, outlines, work_space);
    for (auto& outline : outlines) {
        triangleFromOutline(outline, work_space->m_work_vertex_list_normal, out_vertices, map_old_new, plane_info,
                            out_indices, work_space);
        /// 輪郭追加
        if (create_segment) {
            addSegents(outline, out_segments, work_space->m_work_vertex_list_normal, out_vertices, map_old_new);
        }
    }

    for (auto& loop : contours) {
        delete loop;
    }

    return true;
}

void Clipping::collectTarget(Node* node, const Matrix4x4f& parent_matrix, std::vector<TargetNode>& target_list,
                             bool only_visible)
{
    if (!node) {
        node = m_scene_graph->rootNode();
    }

    if (only_visible) {
        if (!node->isVisible()) {
            return;
        }
    }

    /// パスマトリックス設定
    Matrix4x4f  cur_parent_matrix = parent_matrix;
    const auto& matrix            = node->matrix();
    if (!matrix.isIdentity()) {
        cur_parent_matrix = cur_parent_matrix * matrix;
    }

    /// 対象を取得
    if (node->shape()) {
        target_list.emplace_back(node, cur_parent_matrix);
    }

    for (auto child : node->children()) {
        collectTarget(child.ptr(), cur_parent_matrix, target_list, only_visible);
    }
}

void Clipping::sectionLineForAutoLength(const Planef& clip_plane, const Planef& line_cut_plane, bool only_visible,
                                        std::vector<SectionInfo>& out_section_lines, BoundingBox3f* clip_area)
{
    if (!m_scene_graph) {
        return;
    }
    std::vector<TargetNode> target_list;
    collectTarget(m_scene_graph->rootNode(), m_scene_graph->rootNode()->matrix(), target_list, only_visible);

    std::vector<PlaneInfo> plane_info_list;
    plane_info_list.emplace_back(clip_plane);
    plane_info_list.emplace_back(line_cut_plane);
    for (auto& plane_info : plane_info_list) {
        plane_info.setPlaneInfo();
    }

    MutexWrapper  clipping_mutex;
    std::set<int> m_work_unused;

    for (int ic = 0; ic < m_work_list.size(); ++ic) {
        m_work_unused.insert(ic);
    }

    if (out_section_lines.size() != target_list.size()) {
        out_section_lines.resize(target_list.size());
    }

    std::vector<std::pair<TargetNode*, SectionInfo*>> target_list_2;
    for (int ic = 0; ic < target_list.size(); ++ic) {
        target_list_2.emplace_back(&target_list[ic], &out_section_lines[ic]);
    }

    /// 並列化する
    std::for_each(
        std::execution::seq, target_list_2.begin(), target_list_2.end(),
        [&](std::pair<TargetNode*, SectionInfo*>& target_info) {
            TargetNode&  target      = *target_info.first;
            SectionInfo& out_section = *target_info.second;

            out_section.m_node = target.m_node;

            auto& node        = target.m_node;
            auto& path_matrix = target.m_path_matrix;

            auto object = node->object();
            if (!object) {
                /// 並列処理なので
                return;
            }

            if (clip_area) {
                if (!clip_area->isOverlap(node->boundingBox())) {
                    return;
                }
            }

            /// 対象のパスマトリックスを考慮して平面方程式をそのノードのローカルに変換
            auto target_plane_info_list = plane_info_list;
            if (!path_matrix.isIdentity()) {
                for (auto& target_plane : target_plane_info_list) {
                    /// 平面方程式をローカル座標系に変換
                    target_plane.m_plane = path_matrix.inverted() * target_plane.m_plane;
                    target_plane.setPlaneInfo();
                }
            }
            switch (object->type()) {
                case ObjectType::Voxel: {
                    Voxel* voxel = (Voxel*)object;

                    RenderEditableMesh* mesh = (RenderEditableMesh*)node->renderable();

                    /// クリッピングしない場合（直接切断する場合）
                    if (target_plane_info_list[0].m_plane.normal() == Point3f(0, 0, 0)) {
                        clipPolygonWithPlaneSimple(mesh, target_plane_info_list[1], out_section.m_points, clip_area);
                    }
                    /// クリッピングしてから切断する場合
                    else {
                        /// 高速化のための作業領域使いまわし
                        ClippingWork* work_space       = nullptr;
                        int           work_space_index = -1;
                        {
                            clipping_mutex.lock();

                            if (m_work_unused.size() > 0) {
                                work_space_index = *m_work_unused.begin();
                                m_work_unused.erase(m_work_unused.begin());
                            }
                            else {
                                work_space = new ClippingWork;
                                m_work_list.emplace_back(work_space);
                                work_space_index = m_work_list.size() - 1;
                            }

                            work_space = m_work_list[work_space_index];

                            clipping_mutex.unlock();
                        }

                        std::vector<Point3f>&      vertex_list = mesh->displayEditVerticesTemp();
                        std::vector<unsigned int>& index_list  = mesh->displayEditIndicesTemp();
                        vertex_list.clear();
                        index_list.clear();

                        /// 平面トリア取得
                        const auto& in_vertex_list =
                            mesh->isEnableEditDisplayData() ? mesh->displayEditVertices() : mesh->displayVertices();
                        const auto& in_index_list =
                            mesh->isEnableEditDisplayData() ? mesh->displayEditIndices() : mesh->displayIndices();

                        vertex_list.reserve(in_vertex_list.size());
                        index_list.reserve(in_index_list.size());

                        if (clippingForAutoLength(in_vertex_list, in_index_list, target_plane_info_list[0], vertex_list,
                                                  index_list, work_space)) {
                            clipPolygonWithPlaneSimple(vertex_list, index_list, target_plane_info_list[1],
                                                       out_section.m_points, clip_area);
                        }
                        else {
                            clipPolygonWithPlaneSimple(mesh, target_plane_info_list[1], out_section.m_points,
                                                       clip_area);
                        }

                        {
                            /// Clear
                            std::vector<Point3f>&      vertex_list = mesh->displayEditVerticesTemp();
                            std::vector<unsigned int>& index_list  = mesh->displayEditIndicesTemp();
                            vertex_list.clear();
                            std::vector<Point3f> temp;
                            vertex_list.swap(temp);
                            index_list.clear();
                            std::vector<unsigned int> temp2;
                            index_list.swap(temp2);
                        }

                        clipping_mutex.lock();
                        m_work_unused.insert(work_space_index);
                        clipping_mutex.unlock();
                    }
                } break;
            }
            //}
        });
}

bool Clipping::clipPolygonWithPlaneSimple(const std::vector<Point3f>&      in_vertices,
                                          const std::vector<unsigned int>& in_indices, const PlaneInfo& plane_info,
                                          std::vector<std::pair<Point3f, Point3f>>& out_edges, BoundingBox3f* clip_area)
{
    // out_edges.clear();

    const auto& plane    = plane_info.m_plane;
    float       zero_eps = m_valid_eps / 2.0;    /// 暫定ーゆるくする
    bool        ret      = false;

    for (size_t i = 0; i < in_indices.size(); i += 3) {
        const unsigned int* tri_indices = &in_indices[i];
        float               dist[3];
        for (int j = 0; j < 3; ++j) {
            dist[j] = plane.distanceToPoint(in_vertices[tri_indices[j]]);
        }

        Point3f intersection[3];
        int     intersection_count = 0;

        for (int j = 0; j < 3; ++j) {
            int next = (j + 1) % 3;
            if (std::abs(dist[j]) <= zero_eps) {
                intersection[intersection_count++] = in_vertices[tri_indices[j]];
            }
            else if ((dist[j] > zero_eps && dist[next] <= zero_eps)
                     || (dist[j] < -zero_eps && dist[next] >= -zero_eps)) {
                const Point3f& v0 = in_vertices[tri_indices[j]];
                const Point3f& v1 = in_vertices[tri_indices[next]];

                float t                            = dist[j] / (dist[j] - dist[next]);
                intersection[intersection_count++] = v0 + (v1 - v0) * t;
            }
        }

        for (int j = 0; j < intersection_count - 1; ++j) {
            out_edges.emplace_back(intersection[j], intersection[(j + 1) % 3]);
        }
    }

    return ret;
}

bool Clipping::clipPolygonWithPlaneSimple(RenderEditableMesh* mesh, const PlaneInfo& plane_info,
                                          std::vector<std::pair<Point3f, Point3f>>& out_edges, BoundingBox3f* clip_area)
{
    // out_edges.clear();

    const auto& plane    = plane_info.m_plane;
    float       zero_eps = m_valid_eps / 2.0;    /// 暫定ーゆるくする
    bool        ret      = false;

    const auto& group_boxes = mesh->displayTriaGroupBox();
    const auto& group_start = mesh->displayTriaGroupStart();

    const auto& in_vertices = mesh->isEnableEditDisplayData() ? mesh->displayEditVertices() : mesh->displayVertices();
    const auto& in_indices  = mesh->isEnableEditDisplayData() ? mesh->displayEditIndices() : mesh->displayIndices();

    for (int group_index = 0; group_index < group_start.size(); ++group_index) {
        if (clip_area && !clip_area->isOverlap(group_boxes[group_index])) {
            continue;
        }

        int start_index = group_start[group_index];
        int end_index;
        if (group_index < group_start.size() - 1) {
            end_index = group_start[group_index + 1];
        }
        else {
            end_index = in_indices.size();
        }

        for (int ic = start_index; ic < end_index; ic += 3) {
            const unsigned int* tri_indices = &in_indices[ic];

            float dist[3];
            for (int j = 0; j < 3; ++j) {
                dist[j] = plane.distanceToPoint(in_vertices[tri_indices[j]]);
            }

            Point3f intersection[3];
            int     intersection_count = 0;

            for (int j = 0; j < 3; ++j) {
                int next = (j + 1) % 3;
                if (std::abs(dist[j]) <= zero_eps) {
                    intersection[intersection_count++] = in_vertices[tri_indices[j]];
                }
                else if ((dist[j] > zero_eps && dist[next] <= zero_eps)
                         || (dist[j] < -zero_eps && dist[next] >= -zero_eps)) {
                    const Point3f& v0 = in_vertices[tri_indices[j]];
                    const Point3f& v1 = in_vertices[tri_indices[next]];

                    float t                            = dist[j] / (dist[j] - dist[next]);
                    intersection[intersection_count++] = v0 + (v1 - v0) * t;
                }
            }

            for (int j = 0; j < intersection_count - 1; ++j) {
                out_edges.emplace_back(intersection[j], intersection[(j + 1) % 3]);
            }
        }
    }

    return ret;
}

bool Clipping::clipPolygonWithPlane(const std::vector<Point3f>&      in_vertices,
                                    const std::vector<unsigned int>& in_indices,
                                    const std::vector<unsigned int>& in_segments, const PlaneInfo& plane_info,
                                    std::vector<Point3f>& out_vertices, std::vector<unsigned int>& out_indices,
                                    std::vector<unsigned int>& out_segments, bool create_segment, bool create_fill,
                                    bool only_get_on_plane, bool after_2d_clip, std::vector<EdgeInfo>& all_edge_list,
                                    ClippingWork* work_space)
{
    out_vertices.clear();
    out_indices.clear();
    all_edge_list.clear();
    out_segments.clear();

    /// 作業領域
    std::vector<int> clipped_indices;
    Point2f          section_line[2];
    int              section_tria_index[2];

    /// ↓隙間ができる場合が想定される（頂点が完全に共有ならできないと思うが、現状Voxelがそうなっていない（面単位で作っているため））
    ///  問題あれば戻して別の検討すべきかもしれない

    /// 厳密に計算しても正しいが非常に薄い形状ができる場合がある（ある意味正しいが）
    /// 平面とみなす領域を許容値範囲として計算する
    ///   ※全体で整合とれないとエッジループが正しく取得できないので注意
    ///
    /// 上下方向あるので、足してm_zero_epsとする（その後のエッジ接続判定とかがm_zero_epsなのでそこで収まるようにする）
    float zero_eps   = m_valid_eps / 2.0 * 0.01;    /// 暫定 - ここの調整が必要
    float zero_eps_2 = zero_eps * zero_eps;

    const auto& plane            = plane_info.m_plane;
    const auto& inv_plane_matrix = plane_info.m_inv_plane_matrix;

    const auto& plane_normal = plane.normal();

    /// すべて内側の頂点をなるべくそのまま残すため
    /// 消える頂点を除去したIndexを生成
    bool has_clipping = false;
    bool has_tria     = false;

    int*& old_new_indices           = work_space->m_work_old_new_indices;
    int&  work_old_new_indices_size = work_space->m_work_old_new_indices_size;
    if (work_old_new_indices_size < (int)in_vertices.size()) {
        work_old_new_indices_size = (int)in_vertices.size();
        delete[] old_new_indices;
        old_new_indices = new int[work_old_new_indices_size];
    }

    for (int ic = 0; ic < (int)in_vertices.size(); ++ic) {
        float dist = plane.distanceToPoint(in_vertices[ic]);
        if (dist >= -zero_eps) {
            /// 平面上判定されたものはすべて平面投影させる
            /// やらない方がいいか？上のコメントと合わせ要検討
            if (fabs(dist) <= zero_eps) {
                Point3f ver_temp(in_vertices[ic] - plane_normal * dist);
                out_vertices.emplace_back(ver_temp);
                if (only_get_on_plane) {
                    has_clipping = true;
                }
            }
            else {
                out_vertices.emplace_back(in_vertices[ic]);
            }

            old_new_indices[ic] = (int)out_vertices.size() - 1;    //.emplace_back((int)out_vertices.size() - 1);
            has_tria            = true;
        }
        else {
            has_clipping        = true;
            old_new_indices[ic] = -1;
            // old_new_indices.emplace_back(-1);
        }
    }
    /// クリッピングなし
    if (!has_clipping) {
        return false;
    }
    /// 残るデータなし
    if (!has_tria) {
        return true;
    }

    bool edit = false;

    int old_new_indices_size = (int)in_vertices.size();    //(int)old_new_indices.size();
    int in_indices_size      = (int)in_indices.size();

    bool get_edge = (create_segment || create_fill);

    for (int ic = 0; ic < in_indices_size; ic += 3) {
        const unsigned int* in_index = &in_indices[ic];

        /// 通常来ない。来るならデータにバグがある
        if ((old_new_indices_size <= in_index[0]) || (old_new_indices_size <= in_index[1])
            || (old_new_indices_size <= in_index[2])) {
            edit = true;
            continue;
        }

        int new_index[3];
        new_index[0] = old_new_indices[in_index[0]];
        new_index[1] = old_new_indices[in_index[1]];
        new_index[2] = old_new_indices[in_index[2]];

        /// すべて残す
        if (new_index[0] >= 0 && new_index[1] >= 0 && new_index[2] >= 0) {
            float dist[3];
            dist[0] = plane.distanceToPoint(in_vertices[in_index[0]]);
            dist[1] = plane.distanceToPoint(in_vertices[in_index[1]]);
            dist[2] = plane.distanceToPoint(in_vertices[in_index[2]]);

            /// 平面上は残さない
            /// 　※ 閉じた形状であれば必ず交差箇所で断面線が出て、それで塗りつぶされる(はず）
            /// 　　 ここで残すと2重になる
            if (create_fill) {
                if ((dist[0] <= zero_eps) && (dist[1] <= zero_eps) && (dist[2] <= zero_eps)) {
                    edit = true;
                    continue;
                }
            }

            /// 形状内側に入り込んでいる場合はエッジ取得
            /// 向き考慮して取得(トリア方向：反時計回り方向）
            if (get_edge) {
                for (int jc = 0; jc < 3; ++jc) {
                    if ((dist[jc] <= zero_eps) && (dist[(jc + 1) % 3] <= zero_eps) && (dist[(jc + 2) % 3] > zero_eps)) {
                        auto intersec_0 = inv_plane_matrix * out_vertices[new_index[jc]];
                        auto intersec_1 = inv_plane_matrix * out_vertices[new_index[(jc + 1) % 3]];

                        Point2f intersec_2d_0(intersec_0[0], intersec_0[1]);
                        Point2f intersec_2d_1(intersec_1[0], intersec_1[1]);
                        if ((intersec_2d_0 - intersec_2d_1).length2() > zero_eps_2) {
                            if (!after_2d_clip) {
                                all_edge_list.emplace_back(intersec_2d_0, intersec_2d_1, new_index[jc],
                                                           new_index[(jc + 1) % 3]);
                            }
                            else {
                                out_segments.emplace_back(new_index[jc]);
                                out_segments.emplace_back(new_index[(jc + 1) % 3]);
                            }
                        }
                    }
                }
            }

            /// トリア取得
            if (!only_get_on_plane) {
                out_indices.emplace_back(new_index[0]);
                out_indices.emplace_back(new_index[1]);
                out_indices.emplace_back(new_index[2]);
            }
        }
        /// 分割可能性あり
        else if (new_index[0] >= 0 || new_index[1] >= 0 || new_index[2] >= 0) {
            clipped_indices.clear();

            float dist[3];
            dist[0] = plane.distanceToPoint(in_vertices[in_index[0]]);
            dist[1] = plane.distanceToPoint(in_vertices[in_index[1]]);
            dist[2] = plane.distanceToPoint(in_vertices[in_index[2]]);

            /// 平面の外側（エッジや頂点が乗っているだけ）の場合
            if (dist[0] <= zero_eps && dist[1] <= zero_eps && dist[2] <= zero_eps) {
                edit = true;
                continue;
            }

            int section_index = 0;
            for (int jc = 0; jc < 3; ++jc) {
                /// そのまま残す場合（内側or平面上）
                if (new_index[jc] >= 0) {
                    clipped_indices.emplace_back(new_index[jc]);

                    /// 平面上の場合、交点として登録
                    if (fabs(dist[jc]) <= zero_eps) {
                        if (section_index < 2) {
                            const Point3f& p0                 = out_vertices[new_index[jc]];
                            auto           intersec_2d        = inv_plane_matrix * out_vertices[new_index[jc]];
                            section_line[section_index]       = Point2f(intersec_2d[0], intersec_2d[1]);
                            section_tria_index[section_index] = clipped_indices.size() - 1;
                            ++section_index;
                        }
                        else {
                            assert(section_index < 2);
                        }
                    }
                }

                /// 分割する頂点
                if ((dist[jc] > zero_eps && new_index[(jc + 1) % 3] < 0)
                    || (new_index[jc] < 0 && dist[(jc + 1) % 3] > zero_eps)) {
                    float d0 = dist[jc];
                    float d1 = dist[(jc + 1) % 3];

                    const Point3f& p0 = new_index[jc] >= 0 ? out_vertices[new_index[jc]] : in_vertices[in_index[jc]];
                    const Point3f& p1 = new_index[(jc + 1) % 3] >= 0 ? out_vertices[new_index[(jc + 1) % 3]]
                                                                     : in_vertices[in_index[(jc + 1) % 3]];

                    /// 頂点追加
                    float t = d0 / (d0 - d1);
                    // if (t >= 0 && t <= 1) { /// 誤差考慮でチェックしない
                    Point3f intersection = p0 + (p1 - p0) * t;
                    out_vertices.emplace_back(intersection);
                    clipped_indices.emplace_back((int)out_vertices.size() - 1);

                    if (section_index < 2) {
                        auto intersec_2d                  = inv_plane_matrix * intersection;
                        section_line[section_index]       = Point2f(intersec_2d[0], intersec_2d[1]);
                        section_tria_index[section_index] = clipped_indices.size() - 1;
                        ++section_index;
                    }
                    else {
                        assert(section_index < 2);
                    }
                    //}
                }
            }

            /// 分割のエッジを取得
            if (get_edge) {
                if (section_index > 1) {
                    if ((section_line[0] - section_line[1]).length2() > zero_eps_2) {
                        /// 向き考慮して取得(トリア方向：反時計回り方向）
                        if (clipped_indices.size() == 3) {
                            if ((section_tria_index[0] == 0 && section_tria_index[1] == 1)
                                || (section_tria_index[0] == 1 && section_tria_index[1] == 2)) {
                                if (!after_2d_clip) {
                                    all_edge_list.emplace_back(section_line[0], section_line[1],
                                                               clipped_indices[section_tria_index[0]],
                                                               clipped_indices[section_tria_index[1]]);
                                }
                                else {
                                    out_segments.emplace_back(clipped_indices[section_tria_index[0]]);
                                    out_segments.emplace_back(clipped_indices[section_tria_index[1]]);
                                }
                            }
                            else if ((section_tria_index[0] == 0 && section_tria_index[1] == 2)) {
                                if (!after_2d_clip) {
                                    all_edge_list.emplace_back(section_line[1], section_line[0],
                                                               clipped_indices[section_tria_index[1]],
                                                               clipped_indices[section_tria_index[0]]);
                                }
                                else {
                                    out_segments.emplace_back(clipped_indices[section_tria_index[1]]);
                                    out_segments.emplace_back(clipped_indices[section_tria_index[0]]);
                                }
                            }
                            else {
                                assert(false);
                            }
                        }
                        else {
                            if ((section_tria_index[0] == 0 && section_tria_index[1] == 1)
                                || (section_tria_index[0] == 1 && section_tria_index[1] == 2)
                                || (section_tria_index[0] == 2 && section_tria_index[1] == 3)) {
                                if (!after_2d_clip) {
                                    all_edge_list.emplace_back(section_line[0], section_line[1],
                                                               clipped_indices[section_tria_index[0]],
                                                               clipped_indices[section_tria_index[1]]);
                                }
                                else {
                                    out_segments.emplace_back(clipped_indices[section_tria_index[0]]);
                                    out_segments.emplace_back(clipped_indices[section_tria_index[1]]);
                                }
                            }
                            else if ((section_tria_index[0] == 0 && section_tria_index[1] == 3)) {
                                if (!after_2d_clip) {
                                    all_edge_list.emplace_back(section_line[1], section_line[0],
                                                               clipped_indices[section_tria_index[1]],
                                                               clipped_indices[section_tria_index[0]]);
                                }
                                else {
                                    out_segments.emplace_back(clipped_indices[section_tria_index[1]]);
                                    out_segments.emplace_back(clipped_indices[section_tria_index[0]]);
                                }
                            }
                            else {
                                assert(false);
                            }
                        }
                    }
                }
            }

            if (!only_get_on_plane) {
                /// 頂点１つが残る場合
                if (clipped_indices.size() == 3) {
                    out_indices.emplace_back(clipped_indices[0]);
                    out_indices.emplace_back(clipped_indices[1]);
                    out_indices.emplace_back(clipped_indices[2]);
                }
                /// 頂点２つが残る場合
                else if (clipped_indices.size() == 4) {
                    out_indices.emplace_back(clipped_indices[0]);
                    out_indices.emplace_back(clipped_indices[1]);
                    out_indices.emplace_back(clipped_indices[2]);

                    out_indices.emplace_back(clipped_indices[0]);
                    out_indices.emplace_back(clipped_indices[2]);
                    out_indices.emplace_back(clipped_indices[3]);
                }
                else {
                    assert(false);
                }
            }

            edit = true;
        }
        /// すべて削除
        else {
            edit = true;
        }
    }

    /// 暫定 - 線分（輪郭線）のカット
    //// カット頂点が二重になるので、本来は上の頂点を参照したい
    if (!only_get_on_plane && create_segment) {
        if (in_segments.size() > 0) {
            int segments_size = (int)in_segments.size();

            for (int ic = 0; ic < segments_size; ic += 2) {
                const unsigned int* in_index = &in_segments[ic];

                /// 通常来ない。来るならデータにバグがある
                if ((old_new_indices_size <= in_index[0]) || (old_new_indices_size <= in_index[1])) {
                    continue;
                }

                int new_index[2];
                new_index[0] = old_new_indices[in_index[0]];
                new_index[1] = old_new_indices[in_index[1]];

                /// 完全に置き換え
                if (new_index[0] >= 0 && new_index[1] >= 0) {
                    /// 平面上は残さない
                    const Point3f& p0 = out_vertices[new_index[0]];
                    const Point3f& p1 = out_vertices[new_index[1]];

                    float d0 = plane.distanceToPoint(p0);
                    float d1 = plane.distanceToPoint(p1);
                    if ((d0 <= zero_eps) && (d1 <= zero_eps)) {
                        continue;
                    }

                    out_segments.emplace_back((unsigned int)new_index[0]);
                    out_segments.emplace_back((unsigned int)new_index[1]);
                }
                /// 完全に除去
                else if (new_index[0] < 0 && new_index[1] < 0) {
                    continue;
                }
                /// 分割
                else {
                    const Point3f& p0 = new_index[0] >= 0 ? out_vertices[new_index[0]] : in_vertices[in_index[0]];
                    const Point3f& p1 = new_index[1] >= 0 ? out_vertices[new_index[1]] : in_vertices[in_index[1]];

                    float d0 = plane.distanceToPoint(p0);
                    float d1 = plane.distanceToPoint(p1);

                    /// 頂点追加
                    float   t            = d0 / (d0 - d1);
                    Point3f intersection = p0 + (p1 - p0) * t;
                    out_vertices.emplace_back(intersection);

                    out_segments.emplace_back(new_index[0] >= 0 ? (unsigned int)new_index[0]
                                                                : (unsigned int)out_vertices.size() - 1);
                    out_segments.emplace_back(new_index[1] >= 0 ? (unsigned int)new_index[1]
                                                                : (unsigned int)out_vertices.size() - 1);
                }
            }
        }

        /// 塗りつぶさない場合　かつ　断面線は欲しい場合
        /// ここで線分とって処理終わらす
        if (!create_fill) {
            for (auto& edge : all_edge_list) {
                out_segments.emplace_back((unsigned int)edge.m_origin_start_vertex_index);
                out_segments.emplace_back((unsigned int)edge.m_origin_end_vertex_index);
            }
        }
    }

    /*
    if (all_edge_list.size() > 0) {
        /// CSVファイルに出力
        QString outputFolder = "D:/test";

        std::vector<EdgeInfo> sort_loop;
        sort_loop.resize((int)all_edge_list.size());
        for (int ic = 0; ic < (int)all_edge_list.size(); ++ic) {
            sort_loop[ic] = all_edge_list[ic];
        }
        std::sort(sort_loop.begin(), sort_loop.end(),
                  [](const EdgeInfo& loop0, const EdgeInfo& loop1) { return loop0.m_start < loop1.m_start; });

        int count = 0;

        QString fileName = QString("%1/edge_test_%2.csv").arg(outputFolder).arg(count++);
        QFile   file(fileName);

        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out.setRealNumberPrecision(8);
            out.setRealNumberNotation(QTextStream::FixedNotation);
            out << "X,Y\n";
            for (auto& edge : sort_loop) {
                if (edge.m_start.x() < -3) {
                    //    continue;
                }
                out << edge.m_start.x() << "," << edge.m_start.y() << "\n";
                out << edge.m_end.x() << "," << edge.m_end.y() << "\n";
                out << "\n";
            }
            file.close();
        }
    }
    */

    return edit;
}

bool Clipping::clipPolygonWithPlane(const std::vector<Vertexf>&      in_vertices,
                                    const std::vector<unsigned int>& in_indices,
                                    const std::vector<unsigned int>& in_segments, const PlaneInfo& plane_info,
                                    std::vector<Vertexf>& out_vertices, std::vector<unsigned int>& out_indices,
                                    std::vector<unsigned int>& out_segments, bool create_segment, bool create_fill,
                                    bool only_get_on_plane, bool after_2d_clip, std::vector<EdgeInfo>& all_edge_list,
                                    ClippingWork* work_space)
{
    out_vertices.clear();
    out_indices.clear();
    all_edge_list.clear();
    out_segments.clear();

    std::vector<int> clipped_indices;
    Point2f          section_line[2];
    int              section_tria_index[2];

    float zero_eps   = m_valid_eps / 2.0 * 0.01;
    float zero_eps_2 = zero_eps * zero_eps;

    const auto& plane            = plane_info.m_plane;
    const auto& inv_plane_matrix = plane_info.m_inv_plane_matrix;
    const auto& plane_normal     = plane.normal();

    bool has_clipping = false;
    bool has_tria     = false;

    int*& old_new_indices           = work_space->m_work_old_new_indices;
    int&  work_old_new_indices_size = work_space->m_work_old_new_indices_size;
    if (work_old_new_indices_size < (int)in_vertices.size()) {
        work_old_new_indices_size = (int)in_vertices.size();
        delete[] old_new_indices;
        old_new_indices = new int[work_old_new_indices_size];
    }

    for (int ic = 0; ic < (int)in_vertices.size(); ++ic) {
        float dist = plane.distanceToPoint(in_vertices[ic].m_position);
        if (dist >= -zero_eps) {
            if (fabs(dist) <= zero_eps) {
                Point3f ver_temp(in_vertices[ic].m_position - plane_normal * dist);
                out_vertices.emplace_back(ver_temp, in_vertices[ic].m_normal);
                if (only_get_on_plane) {
                    has_clipping = true;
                }
            }
            else {
                out_vertices.emplace_back(in_vertices[ic]);
            }

            old_new_indices[ic] = (int)out_vertices.size() - 1;
            has_tria            = true;
        }
        else {
            has_clipping        = true;
            old_new_indices[ic] = -1;
        }
    }

    if (!has_clipping) {
        return false;
    }
    if (!has_tria) {
        return true;
    }

    bool edit = false;

    int old_new_indices_size = (int)in_vertices.size();
    int in_indices_size      = (int)in_indices.size();

    bool get_edge = (create_segment || create_fill);

    for (int ic = 0; ic < in_indices_size; ic += 3) {
        const unsigned int* in_index = &in_indices[ic];

        if ((old_new_indices_size <= in_index[0]) || (old_new_indices_size <= in_index[1])
            || (old_new_indices_size <= in_index[2])) {
            edit = true;
            continue;
        }

        int new_index[3];
        new_index[0] = old_new_indices[in_index[0]];
        new_index[1] = old_new_indices[in_index[1]];
        new_index[2] = old_new_indices[in_index[2]];

        if (new_index[0] >= 0 && new_index[1] >= 0 && new_index[2] >= 0) {
            float dist[3];
            dist[0] = plane.distanceToPoint(in_vertices[in_index[0]].m_position);
            dist[1] = plane.distanceToPoint(in_vertices[in_index[1]].m_position);
            dist[2] = plane.distanceToPoint(in_vertices[in_index[2]].m_position);

            if (create_fill) {
                if ((dist[0] <= zero_eps) && (dist[1] <= zero_eps) && (dist[2] <= zero_eps)) {
                    edit = true;
                    continue;
                }
            }

            if (get_edge) {
                for (int jc = 0; jc < 3; ++jc) {
                    if ((dist[jc] <= zero_eps) && (dist[(jc + 1) % 3] <= zero_eps)
                        && (dist[(jc + 2) % 3] > zero_eps)) {
                        auto intersec_0 = inv_plane_matrix * out_vertices[new_index[jc]].m_position;
                        auto intersec_1 = inv_plane_matrix * out_vertices[new_index[(jc + 1) % 3]].m_position;

                        Point2f intersec_2d_0(intersec_0[0], intersec_0[1]);
                        Point2f intersec_2d_1(intersec_1[0], intersec_1[1]);
                        if ((intersec_2d_0 - intersec_2d_1).length2() > zero_eps_2) {
                            if (!after_2d_clip) {
                                all_edge_list.emplace_back(intersec_2d_0, intersec_2d_1, new_index[jc],
                                                           new_index[(jc + 1) % 3]);
                            }
                            else {
                                out_segments.emplace_back(new_index[jc]);
                                out_segments.emplace_back(new_index[(jc + 1) % 3]);
                            }
                        }
                    }
                }
            }

            if (!only_get_on_plane) {
                out_indices.emplace_back(new_index[0]);
                out_indices.emplace_back(new_index[1]);
                out_indices.emplace_back(new_index[2]);
            }
        }
        else if (new_index[0] >= 0 || new_index[1] >= 0 || new_index[2] >= 0) {
            clipped_indices.clear();

            float dist[3];
            dist[0] = plane.distanceToPoint(in_vertices[in_index[0]].m_position);
            dist[1] = plane.distanceToPoint(in_vertices[in_index[1]].m_position);
            dist[2] = plane.distanceToPoint(in_vertices[in_index[2]].m_position);

            if (dist[0] <= zero_eps && dist[1] <= zero_eps && dist[2] <= zero_eps) {
                edit = true;
                continue;
            }

            int section_index = 0;
            for (int jc = 0; jc < 3; ++jc) {
                if (new_index[jc] >= 0) {
                    clipped_indices.emplace_back(new_index[jc]);

                    if (fabs(dist[jc]) <= zero_eps) {
                        if (section_index < 2) {
                            auto intersec_2d =
                                inv_plane_matrix * out_vertices[new_index[jc]].m_position;
                            section_line[section_index]       = Point2f(intersec_2d[0], intersec_2d[1]);
                            section_tria_index[section_index] = clipped_indices.size() - 1;
                            ++section_index;
                        }
                        else {
                            assert(section_index < 2);
                        }
                    }
                }

                /// カット位置で2頂点分割: 法線を線形補間して新頂点を生成
                if ((dist[jc] > zero_eps && new_index[(jc + 1) % 3] < 0)
                    || (new_index[jc] < 0 && dist[(jc + 1) % 3] > zero_eps)) {
                    float d0 = dist[jc];
                    float d1 = dist[(jc + 1) % 3];

                    const Vertexf& v0 =
                        new_index[jc] >= 0 ? out_vertices[new_index[jc]] : in_vertices[in_index[jc]];
                    const Vertexf& v1 = new_index[(jc + 1) % 3] >= 0 ? out_vertices[new_index[(jc + 1) % 3]]
                                                                      : in_vertices[in_index[(jc + 1) % 3]];

                    float   t    = d0 / (d0 - d1);
                    Point3f pos  = v0.m_position + (v1.m_position - v0.m_position) * t;
                    Point3f norm = (v0.m_normal + (v1.m_normal - v0.m_normal) * t).normalized();
                    out_vertices.emplace_back(pos, norm);
                    clipped_indices.emplace_back((int)out_vertices.size() - 1);

                    if (section_index < 2) {
                        auto intersec_2d              = inv_plane_matrix * pos;
                        section_line[section_index]   = Point2f(intersec_2d[0], intersec_2d[1]);
                        section_tria_index[section_index] = clipped_indices.size() - 1;
                        ++section_index;
                    }
                    else {
                        assert(section_index < 2);
                    }
                }
            }

            if (get_edge) {
                if (section_index > 1) {
                    if ((section_line[0] - section_line[1]).length2() > zero_eps_2) {
                        if (clipped_indices.size() == 3) {
                            if ((section_tria_index[0] == 0 && section_tria_index[1] == 1)
                                || (section_tria_index[0] == 1 && section_tria_index[1] == 2)) {
                                if (!after_2d_clip) {
                                    all_edge_list.emplace_back(section_line[0], section_line[1],
                                                               clipped_indices[section_tria_index[0]],
                                                               clipped_indices[section_tria_index[1]]);
                                }
                                else {
                                    out_segments.emplace_back(clipped_indices[section_tria_index[0]]);
                                    out_segments.emplace_back(clipped_indices[section_tria_index[1]]);
                                }
                            }
                            else if ((section_tria_index[0] == 0 && section_tria_index[1] == 2)) {
                                if (!after_2d_clip) {
                                    all_edge_list.emplace_back(section_line[1], section_line[0],
                                                               clipped_indices[section_tria_index[1]],
                                                               clipped_indices[section_tria_index[0]]);
                                }
                                else {
                                    out_segments.emplace_back(clipped_indices[section_tria_index[1]]);
                                    out_segments.emplace_back(clipped_indices[section_tria_index[0]]);
                                }
                            }
                            else {
                                assert(false);
                            }
                        }
                        else {
                            if ((section_tria_index[0] == 0 && section_tria_index[1] == 1)
                                || (section_tria_index[0] == 1 && section_tria_index[1] == 2)
                                || (section_tria_index[0] == 2 && section_tria_index[1] == 3)) {
                                if (!after_2d_clip) {
                                    all_edge_list.emplace_back(section_line[0], section_line[1],
                                                               clipped_indices[section_tria_index[0]],
                                                               clipped_indices[section_tria_index[1]]);
                                }
                                else {
                                    out_segments.emplace_back(clipped_indices[section_tria_index[0]]);
                                    out_segments.emplace_back(clipped_indices[section_tria_index[1]]);
                                }
                            }
                            else if ((section_tria_index[0] == 0 && section_tria_index[1] == 3)) {
                                if (!after_2d_clip) {
                                    all_edge_list.emplace_back(section_line[1], section_line[0],
                                                               clipped_indices[section_tria_index[1]],
                                                               clipped_indices[section_tria_index[0]]);
                                }
                                else {
                                    out_segments.emplace_back(clipped_indices[section_tria_index[1]]);
                                    out_segments.emplace_back(clipped_indices[section_tria_index[0]]);
                                }
                            }
                            else {
                                assert(false);
                            }
                        }
                    }
                }
            }

            if (!only_get_on_plane) {
                if (clipped_indices.size() == 3) {
                    out_indices.emplace_back(clipped_indices[0]);
                    out_indices.emplace_back(clipped_indices[1]);
                    out_indices.emplace_back(clipped_indices[2]);
                }
                else if (clipped_indices.size() == 4) {
                    out_indices.emplace_back(clipped_indices[0]);
                    out_indices.emplace_back(clipped_indices[1]);
                    out_indices.emplace_back(clipped_indices[2]);

                    out_indices.emplace_back(clipped_indices[0]);
                    out_indices.emplace_back(clipped_indices[2]);
                    out_indices.emplace_back(clipped_indices[3]);
                }
                else {
                    assert(false);
                }
            }

            edit = true;
        }
        else {
            edit = true;
        }
    }

    /// 暫定 - 線分（輪郭線）のカット
    if (!only_get_on_plane && create_segment) {
        if (in_segments.size() > 0) {
            int segments_size = (int)in_segments.size();

            for (int ic = 0; ic < segments_size; ic += 2) {
                const unsigned int* in_index = &in_segments[ic];

                if ((old_new_indices_size <= in_index[0]) || (old_new_indices_size <= in_index[1])) {
                    continue;
                }

                int new_index[2];
                new_index[0] = old_new_indices[in_index[0]];
                new_index[1] = old_new_indices[in_index[1]];

                if (new_index[0] >= 0 && new_index[1] >= 0) {
                    const Point3f& p0 = out_vertices[new_index[0]].m_position;
                    const Point3f& p1 = out_vertices[new_index[1]].m_position;

                    float d0 = plane.distanceToPoint(p0);
                    float d1 = plane.distanceToPoint(p1);
                    if ((d0 <= zero_eps) && (d1 <= zero_eps)) {
                        continue;
                    }

                    out_segments.emplace_back((unsigned int)new_index[0]);
                    out_segments.emplace_back((unsigned int)new_index[1]);
                }
                else if (new_index[0] < 0 && new_index[1] < 0) {
                    continue;
                }
                else {
                    const Vertexf& v0 = new_index[0] >= 0 ? out_vertices[new_index[0]] : in_vertices[in_index[0]];
                    const Vertexf& v1 = new_index[1] >= 0 ? out_vertices[new_index[1]] : in_vertices[in_index[1]];

                    float d0 = plane.distanceToPoint(v0.m_position);
                    float d1 = plane.distanceToPoint(v1.m_position);

                    float   t    = d0 / (d0 - d1);
                    Point3f pos  = v0.m_position + (v1.m_position - v0.m_position) * t;
                    Point3f norm = (v0.m_normal + (v1.m_normal - v0.m_normal) * t).normalized();
                    out_vertices.emplace_back(pos, norm);

                    out_segments.emplace_back(new_index[0] >= 0 ? (unsigned int)new_index[0]
                                                                : (unsigned int)out_vertices.size() - 1);
                    out_segments.emplace_back(new_index[1] >= 0 ? (unsigned int)new_index[1]
                                                                : (unsigned int)out_vertices.size() - 1);
                }
            }
        }

        if (!create_fill) {
            for (auto& edge : all_edge_list) {
                out_segments.emplace_back((unsigned int)edge.m_origin_start_vertex_index);
                out_segments.emplace_back((unsigned int)edge.m_origin_end_vertex_index);
            }
        }
    }

    return edit;
}

void Clipping::createContour(const std::vector<EdgeInfo>& edge_list, std::vector<ClippingLoop*>& contours,
                             ClippingWork* work_space)
{
    if (edge_list.size() < 3) {
        return;
    }

    float zero_eps   = m_zero_eps;
    float zero_eps_2 = zero_eps * zero_eps;
    float valid_eps  = m_valid_eps;

    SearchXYData<const EdgeInfo*, float> search_edge;
    search_edge.setEPS(zero_eps);
    for (auto& edge : edge_list) {
        search_edge.appendData(edge.m_start.x(), edge.m_start.y(), &edge);
    }
    SearchXYXYCount<float> edge_count;
    edge_count.setEPS(zero_eps);

    /// TODO: 不要か？直接ポインタでも使用済みを判断
    std::unordered_set<const EdgeInfo*> checked_edge;

    for (const auto& edge : edge_list) {
        /// エッジ登録済みか
        if (edge_count.count(edge.m_start.x(), edge.m_start.y(), edge.m_end.x(), edge.m_end.y(), false)) {
            continue;
        }

        checked_edge.insert(&edge);

        contours.emplace_back(new ClippingLoop);
        ClippingLoop* contour = contours.back();

        Point2f current_0      = edge.m_start;
        Point2f current_1      = edge.m_end;
        int     end_edge_index = edge.m_origin_end_vertex_index;
        contour->addPointWithDataCheckSameLine(current_0, edge.m_origin_start_vertex_index, zero_eps);
        auto start = current_0;
        while ((current_1 - start).length2() > zero_eps_2) {
            /// エッジ取得＆取得済み登録
            contour->addPointWithDataCheckSameLine(current_1, end_edge_index, zero_eps);
            edge_count.appendData(current_0.x(), current_0.y(), current_1.x(), current_1.y(), false);

            bool found_next = false;

            float eps = zero_eps;
            while (1) {
                std::set<const EdgeInfo*> searched_edge_info;
                search_edge.search(current_1.x(), current_1.y(), searched_edge_info, eps);

                /// エッジの接続先が１つの場合
                if (searched_edge_info.size() == 1) {
                    const auto  end_edge  = *searched_edge_info.begin();
                    const auto& end_point = end_edge->m_end;
                    if (checked_edge.count(end_edge) == 0
                        && edge_count.count(current_1.x(), current_1.y(), end_point.x(), end_point.y(), false) == 0) {
                        current_0      = current_1;
                        current_1      = end_point;
                        end_edge_index = end_edge->m_origin_end_vertex_index;
                        found_next     = true;

                        checked_edge.insert(end_edge);
                    }
                }
                /// エッジの接続先が２つ以上ある場合
                else if (searched_edge_info.size() > 1) {
                    ///
                    /// 形状の内側に入り込んでいる方(同じ外形の方)を取得
                    /// 　※ 外形同士が接している場合にここにくる
                    ///
                    /// TODO:要確認
                    /// 穴同士が接続することがあった場合、連続穴を１つの穴とみなすがそれでいいか（輪郭が自己交差がない前提だが、交差ではないからいいのか？とかPoly2Tri側とか）
                    /// 任意のファセット（ソリッドのもの。シェルは除外が必要）で成立するか
                    ///
                    auto edge_dir = current_1 - current_0;
                    // edge_dir.normalize();

                    float min_angle = FLT_MAX;

                    const EdgeInfo* next_point = nullptr;

                    /// 角度が小さい方（エッジの外形を共有している方）を取得
                    for (auto it = searched_edge_info.begin(); it != searched_edge_info.end(); ++it) {
                        const auto  end_edge  = *it;
                        const auto& end_point = end_edge->m_end;
                        if (checked_edge.count(end_edge) == 0
                            && edge_count.count(current_1.x(), current_1.y(), end_point.x(), end_point.y(), false)
                                   == 0) {
                            auto edge_dir_2 = end_point - current_1;
                            // edge_dir_2.normalize();

                            /// 2ベクトルの角度
                            double angle = std::atan2(edge_dir ^ edge_dir_2, edge_dir * edge_dir_2);
                            if (angle < 0) {
                                angle += 2.0 * M_PI;
                            }

                            if (angle == min_angle) {
                                /// 近い方
                                if ((current_1 - end_edge->m_start).length2()
                                    < (current_1 - next_point->m_start).length2()) {
                                    next_point = end_edge;
                                    found_next = true;
                                    min_angle  = angle;
                                }
                            }
                            else if (angle < min_angle) {
                                next_point = end_edge;
                                found_next = true;
                                min_angle  = angle;
                            }
                        }
                    }

                    if (found_next) {
                        current_0      = current_1;
                        current_1      = next_point->m_end;
                        end_edge_index = next_point->m_origin_end_vertex_index;

                        checked_edge.insert(next_point);
                    }
                }

                if (found_next) {
                    break;
                }

                /// TODO: 暫定 - 見つからない場合にRetry
                if (eps == 0.0f) {
                    eps = valid_eps * 1.0e-2f;
                    if (eps == 0.0f) {
                        break;
                    }
                }
                else {
                    eps *= 10.0f;
                }

                /// 暫定
                if (eps > valid_eps * 10) {
                    break;
                }
            }

            if (!found_next) {
                /*
                {
                    /// CSVファイルに出力
                    QString outputFolder = "D:/test";

                    std::vector<EdgeInfo> sort_loop;
                    sort_loop.resize((int)edge_list.size());
                    for (int ic = 0; ic < (int)edge_list.size(); ++ic) {
                        sort_loop[ic] = edge_list[ic];
                    }
                    std::sort(sort_loop.begin(), sort_loop.end(), [](const EdgeInfo& loop0, const EdgeInfo& loop1) {
                        return loop0.m_start < loop1.m_start;
                    });

                    int count = 0;

                    QString fileName = QString("%1/edge_test_%2.csv").arg(outputFolder).arg(count++);
                    QFile   file(fileName);

                    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        QTextStream out(&file);
                        out.setRealNumberPrecision(8);
                        out.setRealNumberNotation(QTextStream::FixedNotation);
                        out << "X,Y\n";
                        for (auto& edge : sort_loop) {
                            if (edge.m_start.x() < -3) {
                                //    continue;
                            }
                            out << edge.m_start.x() << "," << edge.m_start.y() << "\n";
                            out << edge.m_end.x() << "," << edge.m_end.y() << "\n";
                            out << "\n";
                        }
                        file.close();
                    }
                }
                {
                    /// CSVファイルに出力
                    QString outputFolder = "D:/test";

                    int count = 0;

                    QString fileName = QString("%1/loop_test_%2.csv").arg(outputFolder).arg(count++);
                    QFile   file(fileName);
                    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        QTextStream out(&file);
                        out.setRealNumberPrecision(8);                            // 必要に応じて桁数を調整
                        out.setRealNumberNotation(QTextStream::FixedNotation);    // 固定小数点表記を使用

                        out << "X,Y\n";
                        for (auto& loop : contours) {
                            for (const auto& point : loop->points()) {
                                out << point.x() << "," << point.y() << "\n";
                            }
                            out << loop->point(0).x() << "," << loop->point(0).y() << "\n";
                            out << "\n";
                        }
                        file.close();
                    }
                }
                */
                /// TODO:浮いたエッジ(微小なエッジで許容値の関係で接続スキップされたものとか）が来るケースがある
                /// 描画に問題あるケースがあれば対応必要
                // assert(false);
                break;
            }
        }

        /// 末尾閉じる必要ないので閉じていないが、取得済みエッジの登録は必要
        if (contour->count() > 1) {
            const auto& loop_end = contour->point(contour->count() - 1);
            edge_count.appendData(loop_end.x(), loop_end.y(), start.x(), start.y(), false);
        }

        contour->removeDuplicateEndWithData(zero_eps);

        if (contour->count() < 3) {
            delete contour;
            contours.pop_back();
            continue;
        }

        contour->setLoop();

        if (contour->area() == 0.0f) {
            delete contour;
            contours.pop_back();
            continue;
        }
    }
}

/// 多角形ループ群を輪郭線(先頭要素がアウター、残りがインナー(穴))に分ける
void Clipping::outlinesFromLoops(const std::vector<ClippingLoop*>&        loops,
                                 std::vector<std::vector<ClippingLoop*>>& outlines, ClippingWork* work_space)
{
    outlines.clear();
    if (loops.size() == 0) {
        return;
    }
    if (loops.size() == 1) {
        std::vector<ClippingLoop*> loop;
        loop.emplace_back(loops[0]);
        outlines.emplace_back(loop);
        return;
    }

    /// 面積順でソート(アウターは一番大きくなる)
    std::vector<ClippingLoop*> sort_loop;
    sort_loop.resize((int)loops.size());
    for (int ic = 0; ic < (int)loops.size(); ++ic) {
        sort_loop[ic] = loops[ic];
    }
    std::sort(sort_loop.begin(), sort_loop.end(),
              [](const ClippingLoop* loop0, const ClippingLoop* loop1) { return loop0->area() > loop1->area(); });

    float zero_eps = m_zero_eps;

    /// 内包するもので分ける(交差は考慮しない)
    for (int ic = 0; ic < (int)sort_loop.size(); ++ic) {
        ClippingLoop* target0 = sort_loop[ic];
        if (!target0) continue;    /// 取得済み

        std::vector<ClippingLoop*> group_loop;
        group_loop.emplace_back(target0);

        for (int jc = ic + 1; jc < (int)sort_loop.size(); ++jc) {
            ClippingLoop* target1 = sort_loop[jc];
            if (!target1) continue;    /// 取得済み

            /// Boxが内包されない
            if (!target0->isInclude(target1)) {
                continue;
            }

            /// 内包する - 交差を考慮しないので、一点だけでチェック
            const auto& target1_points = target1->points();
            if (target1_points.size() > 0 && target0->isPointInLoop(target1_points[0], zero_eps)) {
                /// 取得済みの他の穴に内包される - 面積で大きいものに対してチェックが必要
                bool in_other_inner_loops = false;
                for (unsigned int kc = 1; kc < group_loop.size(); ++kc) {    /// 0番目(外形)は除く
                    /// Boxが内包されない
                    if (!group_loop[kc]->isInclude(target1)) {
                        continue;
                    }

                    /// 内包する
                    if (group_loop[kc]->isPointInLoop(target1_points[0], zero_eps)) {
                        in_other_inner_loops = true;
                        break;
                    }
                }
                if (!in_other_inner_loops) {
                    group_loop.emplace_back(target1);
                    sort_loop[jc] = nullptr;    /// 取得済み
                }
            }
        }

        outlines.emplace_back(group_loop);
    }
}

void Clipping::triangleFromOutline(const std::vector<ClippingLoop*>& outline, std::vector<Point3f>& out_vertices,
                                   const PlaneInfo& plane_info, std::vector<unsigned int>& out_indices,
                                   ClippingWork* work_space)
{
    if (outline.empty()) {
        return;
    }

    auto& outer = outline[0];
    int   count = outer->count();
    if (count < 3) {
        return;
    }

    /// 例外は出さなそうだがとりあえず入れておく
    try {
        ECPolygons                                  polygons;
        std::vector<std::pair<const Point2f*, int>> index_to_origin_point;

        ECPolygon polygon;

        /// Outer
        for (int ic = 0; ic < count; ++ic) {
            const auto& point = outer->clockWisePoint(ic, true);

            polygon.emplace_back(std::array<float, 2>{point.x(), point.y()});
            index_to_origin_point.emplace_back(&point, outer->clockWiseData(ic, true));
        }
        polygons.emplace_back(polygon);

        /// Inner
        for (int ic = 1; ic < (int)outline.size(); ++ic) {
            polygon.clear();

            auto& inner = outline[ic];

            int count = inner->count();
            for (int jc = 0; jc < count; ++jc) {
                const auto& point = inner->clockWisePoint(jc, false);
                polygon.emplace_back(std::array<float, 2>{point.x(), point.y()});
                index_to_origin_point.emplace_back(&point, inner->clockWiseData(jc, false));
            }

            if (!polygon.empty()) {
                polygons.emplace_back(polygon);
            }
        }

        /// Triangulation
        const std::vector<unsigned int>& indices = mapbox::earcut<unsigned int>(polygons);

        /// 法線方向が違うので頂点は新規で作るが、新規で同じものは共有化する
        std::map<int, int> index_to_tria_index;

        for (int ic = 0; ic < indices.size(); ic += 3) {
            /// 基本は反時計回りでとれるが
            /// 保障はされない（実際反対になっている場合がある）ので向きチェック
            const auto& point_0 = *index_to_origin_point[indices[ic]].first;
            const auto& point_1 = *index_to_origin_point[indices[ic + 1]].first;
            const auto& point_2 = *index_to_origin_point[indices[ic + 2]].first;
            /// 時計回り
            bool clockwise = (((point_1 - point_0) ^ (point_2 - point_0)) < 0.0f);

            for (int jc = 0; jc < 3; ++jc) {
                /// トリア法線方向が平面法線と逆なので、2次元で時計回り→3次元は反時計回りになっているので、反転して取得
                int origin_point_index =
                    index_to_origin_point[clockwise ? indices[ic + jc] : indices[ic + (2 - jc)]].second;

                /// 追加 or 検索を同時に行う
                auto itr_insert =
                    index_to_tria_index.insert(std::make_pair(origin_point_index, (int)out_vertices.size()));
                if (itr_insert.second) {
                    out_indices.emplace_back((unsigned int)out_vertices.size());
                    out_vertices.emplace_back(out_vertices[origin_point_index]);
                }
                else {
                    out_indices.emplace_back((unsigned int)itr_insert.first->second);
                }
            }
        }

        /*
        if (indices.size() == 0) {
            /// CSVファイルに出力
            QString outputFolder = "D:/test";

            static int count = 0;

            QString fileName = QString("%1/tria_test_%2.csv").arg(outputFolder).arg(count++);
            QFile   file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out.setRealNumberPrecision(8);                            // 必要に応じて桁数を調整
                out.setRealNumberNotation(QTextStream::FixedNotation);    // 固定小数点表記を使用

                out << "X,Y\n";
                for (auto& loop : outline) {
                    for (const auto& point : loop->points()) {
                        out << point.x() << "," << point.y() << "\n";
                    }
                    out << loop->point(0).x() << "," << loop->point(0).y() << "\n";
                    out << "\n";
                }
                file.close();
            }
        }
        */
    }
    catch (...) {
        assert(false);
    }
}

void Clipping::triangleFromOutline(const std::vector<ClippingLoop*>& outline, std::vector<Point3f>& out_vertices_input,
                                   std::vector<Point3f>& out_vertices, std::map<int, int>& out_index_old_new,
                                   const PlaneInfo& plane_info, std::vector<unsigned int>& out_indices,
                                   ClippingWork* work_space)
{
    if (outline.empty()) {
        return;
    }

    auto& outer = outline[0];
    int   count = outer->count();
    if (count < 3) {
        return;
    }

    /// 例外は出さなそうだがとりあえず入れておく
    try {
        ECPolygons                                  polygons;
        std::vector<std::pair<const Point2f*, int>> index_to_origin_point;

        ECPolygon polygon;

        /// Outer
        for (int ic = 0; ic < count; ++ic) {
            const auto& point = outer->clockWisePoint(ic, true);

            polygon.emplace_back(std::array<float, 2>{point.x(), point.y()});
            index_to_origin_point.emplace_back(&point, outer->clockWiseData(ic, true));
        }
        polygons.emplace_back(polygon);

        /// Inner
        for (int ic = 1; ic < (int)outline.size(); ++ic) {
            polygon.clear();

            auto& inner = outline[ic];

            int count = inner->count();
            for (int jc = 0; jc < count; ++jc) {
                const auto& point = inner->clockWisePoint(jc, false);
                polygon.emplace_back(std::array<float, 2>{point.x(), point.y()});
                index_to_origin_point.emplace_back(&point, inner->clockWiseData(jc, false));
            }

            if (!polygon.empty()) {
                polygons.emplace_back(polygon);
            }
        }

        /// Triangulation
        const std::vector<unsigned int>& indices = mapbox::earcut<unsigned int>(polygons);

        /// 法線方向が違うので頂点は新規で作るが、新規で同じものは共有化する
        std::map<int, int> index_to_tria_index;

        for (int ic = 0; ic < indices.size(); ic += 3) {
            /// 基本は反時計回りでとれるが
            /// 保障はされない（実際反対になっている場合がある）ので向きチェック
            const auto& point_0 = *index_to_origin_point[indices[ic]].first;
            const auto& point_1 = *index_to_origin_point[indices[ic + 1]].first;
            const auto& point_2 = *index_to_origin_point[indices[ic + 2]].first;
            /// 時計回り
            bool clockwise = (((point_1 - point_0) ^ (point_2 - point_0)) < 0.0f);

            for (int jc = 0; jc < 3; ++jc) {
                /// トリア法線方向が平面法線と逆なので、2次元で時計回り→3次元は反時計回りになっているので、反転して取得
                int origin_point_index =
                    index_to_origin_point[clockwise ? indices[ic + jc] : indices[ic + (2 - jc)]].second;

                /// 追加 or 検索を同時に行う
                auto itr_insert =
                    index_to_tria_index.insert(std::make_pair(origin_point_index, (int)out_vertices.size()));
                if (itr_insert.second) {
                    out_indices.emplace_back((unsigned int)out_vertices.size());
                    out_vertices.emplace_back(out_vertices_input[origin_point_index]);
                    out_index_old_new[origin_point_index] = out_vertices.size() - 1;
                }
                else {
                    out_indices.emplace_back((unsigned int)itr_insert.first->second);
                }
            }
        }

        /*
        if (indices.size() == 0) {
            /// CSVファイルに出力
            QString outputFolder = "D:/test";

            static int count = 0;

            QString fileName = QString("%1/tria_test_%2.csv").arg(outputFolder).arg(count++);
            QFile   file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out.setRealNumberPrecision(8);                            // 必要に応じて桁数を調整
                out.setRealNumberNotation(QTextStream::FixedNotation);    // 固定小数点表記を使用

                out << "X,Y\n";
                for (auto& loop : outline) {
                    for (const auto& point : loop->points()) {
                        out << point.x() << "," << point.y() << "\n";
                    }
                    out << loop->point(0).x() << "," << loop->point(0).y() << "\n";
                    out << "\n";
                }
                file.close();
            }
        }
        */
    }
    catch (...) {
        assert(false);
    }
}

void Clipping::addSegents(const std::vector<ClippingLoop*>& outline, std::vector<unsigned int>& out_segments)
{
    if (outline.empty()) {
        return;
    }

    auto& outer = outline[0];
    int   count = outer->count();
    if (count < 3) {
        return;
    }

    /// Outer
    for (int ic = 0; ic < count; ++ic) {
        int index_0 = outer->clockWiseData(ic, true);
        int index_1 = outer->clockWiseData((ic + 1) % count, true);
        if (index_0 != index_1) {
            out_segments.emplace_back((unsigned int)index_0);
            out_segments.emplace_back((unsigned int)index_1);
        }
    }
    /// Inner
    for (int ic = 1; ic < (int)outline.size(); ++ic) {
        auto& inner = outline[ic];

        int count = inner->count();
        for (int jc = 0; jc < count; ++jc) {
            int index_0 = inner->clockWiseData(jc, false);
            int index_1 = inner->clockWiseData((jc + 1) % count, false);
            if (index_0 != index_1) {
                out_segments.emplace_back((unsigned int)index_0);
                out_segments.emplace_back((unsigned int)index_1);
            }
        }
    }
}

void Clipping::addSegents(const std::vector<ClippingLoop*>& outline, std::vector<unsigned int>& out_segments,
                          std::vector<Point3f>& out_vertices_input, std::vector<Point3f>& out_vertices,
                          std::map<int, int>& out_index_old_new)
{
    if (outline.empty()) {
        return;
    }

    auto& outer = outline[0];
    int   count = outer->count();
    if (count < 3) {
        return;
    }

    /// Outer
    for (int ic = 0; ic < count; ++ic) {
        int index_0 = outer->clockWiseData(ic, true);
        int index_1 = outer->clockWiseData((ic + 1) % count, true);
        if (index_0 != index_1) {
            auto itr_find_0 = out_index_old_new.find(index_0);
            if (itr_find_0 != out_index_old_new.end()) {
                out_segments.emplace_back((unsigned int)itr_find_0->second);
            }
            else {
                out_vertices.emplace_back(out_vertices_input[index_0]);
                out_index_old_new[index_0] = out_vertices.size() - 1;
                out_segments.emplace_back((unsigned int)(out_vertices.size() - 1));
            }

            auto itr_find_1 = out_index_old_new.find(index_1);
            if (itr_find_1 != out_index_old_new.end()) {
                out_segments.emplace_back((unsigned int)itr_find_1->second);
            }
            else {
                out_vertices.emplace_back(out_vertices_input[index_1]);
                out_index_old_new[index_1] = out_vertices.size() - 1;
                out_segments.emplace_back((unsigned int)(out_vertices.size() - 1));
            }
        }
    }
    /// Inner
    for (int ic = 1; ic < (int)outline.size(); ++ic) {
        auto& inner = outline[ic];

        int count = inner->count();
        for (int jc = 0; jc < count; ++jc) {
            int index_0 = inner->clockWiseData(jc, false);
            int index_1 = inner->clockWiseData((jc + 1) % count, false);
            if (index_0 != index_1) {
                auto itr_find_0 = out_index_old_new.find(index_0);
                if (itr_find_0 != out_index_old_new.end()) {
                    out_segments.emplace_back((unsigned int)itr_find_0->second);
                }
                else {
                    out_vertices.emplace_back(out_vertices_input[index_0]);
                    out_index_old_new[index_0] = out_vertices.size() - 1;
                    out_segments.emplace_back((unsigned int)(out_vertices.size() - 1));
                }

                auto itr_find_1 = out_index_old_new.find(index_1);
                if (itr_find_1 != out_index_old_new.end()) {
                    out_segments.emplace_back((unsigned int)itr_find_1->second);
                }
                else {
                    out_vertices.emplace_back(out_vertices_input[index_1]);
                    out_index_old_new[index_1] = out_vertices.size() - 1;
                    out_segments.emplace_back((unsigned int)(out_vertices.size() - 1));
                }
            }
        }
    }
}

/// 法線付き版: 断面塗りつぶし頂点に平面法線を設定 (RenderNormalMesh用)
void Clipping::triangleFromOutline(const std::vector<ClippingLoop*>& outline, std::vector<Vertexf>& out_vertices,
                                   const PlaneInfo& plane_info, std::vector<unsigned int>& out_indices,
                                   ClippingWork* work_space)
{
    if (outline.empty()) {
        return;
    }

    auto& outer = outline[0];
    int   count = outer->count();
    if (count < 3) {
        return;
    }

    try {
        ECPolygons                                  polygons;
        std::vector<std::pair<const Point2f*, int>> index_to_origin_point;

        ECPolygon polygon;

        for (int ic = 0; ic < count; ++ic) {
            const auto& point = outer->clockWisePoint(ic, true);
            polygon.emplace_back(std::array<float, 2>{point.x(), point.y()});
            index_to_origin_point.emplace_back(&point, outer->clockWiseData(ic, true));
        }
        polygons.emplace_back(polygon);

        for (int ic = 1; ic < (int)outline.size(); ++ic) {
            polygon.clear();

            auto& inner = outline[ic];

            int count = inner->count();
            for (int jc = 0; jc < count; ++jc) {
                const auto& point = inner->clockWisePoint(jc, false);
                polygon.emplace_back(std::array<float, 2>{point.x(), point.y()});
                index_to_origin_point.emplace_back(&point, inner->clockWiseData(jc, false));
            }

            if (!polygon.empty()) {
                polygons.emplace_back(polygon);
            }
        }

        const std::vector<unsigned int>& indices = mapbox::earcut<unsigned int>(polygons);

        std::map<int, int> index_to_tria_index;
        const Point3f      fill_normal = plane_info.m_plane.normal();

        for (int ic = 0; ic < (int)indices.size(); ic += 3) {
            const auto& point_0 = *index_to_origin_point[indices[ic]].first;
            const auto& point_1 = *index_to_origin_point[indices[ic + 1]].first;
            const auto& point_2 = *index_to_origin_point[indices[ic + 2]].first;
            bool        clockwise = (((point_1 - point_0) ^ (point_2 - point_0)) < 0.0f);

            for (int jc = 0; jc < 3; ++jc) {
                int origin_point_index =
                    index_to_origin_point[clockwise ? indices[ic + jc] : indices[ic + (2 - jc)]].second;

                auto itr_insert =
                    index_to_tria_index.insert(std::make_pair(origin_point_index, (int)out_vertices.size()));
                if (itr_insert.second) {
                    out_indices.emplace_back((unsigned int)out_vertices.size());
                    Vertexf v = out_vertices[origin_point_index];
                    v.m_normal = fill_normal;
                    out_vertices.emplace_back(v);
                }
                else {
                    out_indices.emplace_back((unsigned int)itr_insert.first->second);
                }
            }
        }
    }
    catch (...) {
        assert(false);
    }
}

void Clipping::triangleFromOutline(const std::vector<ClippingLoop*>& outline, std::vector<Vertexf>& out_vertices_input,
                                   std::vector<Vertexf>& out_vertices, std::map<int, int>& out_index_old_new,
                                   const PlaneInfo& plane_info, std::vector<unsigned int>& out_indices,
                                   ClippingWork* work_space)
{
    if (outline.empty()) {
        return;
    }

    auto& outer = outline[0];
    int   count = outer->count();
    if (count < 3) {
        return;
    }

    try {
        ECPolygons                                  polygons;
        std::vector<std::pair<const Point2f*, int>> index_to_origin_point;

        ECPolygon polygon;

        for (int ic = 0; ic < count; ++ic) {
            const auto& point = outer->clockWisePoint(ic, true);
            polygon.emplace_back(std::array<float, 2>{point.x(), point.y()});
            index_to_origin_point.emplace_back(&point, outer->clockWiseData(ic, true));
        }
        polygons.emplace_back(polygon);

        for (int ic = 1; ic < (int)outline.size(); ++ic) {
            polygon.clear();

            auto& inner = outline[ic];

            int count = inner->count();
            for (int jc = 0; jc < count; ++jc) {
                const auto& point = inner->clockWisePoint(jc, false);
                polygon.emplace_back(std::array<float, 2>{point.x(), point.y()});
                index_to_origin_point.emplace_back(&point, inner->clockWiseData(jc, false));
            }

            if (!polygon.empty()) {
                polygons.emplace_back(polygon);
            }
        }

        const std::vector<unsigned int>& indices = mapbox::earcut<unsigned int>(polygons);

        std::map<int, int> index_to_tria_index;
        const Point3f      fill_normal = plane_info.m_plane.normal();

        for (int ic = 0; ic < (int)indices.size(); ic += 3) {
            const auto& point_0 = *index_to_origin_point[indices[ic]].first;
            const auto& point_1 = *index_to_origin_point[indices[ic + 1]].first;
            const auto& point_2 = *index_to_origin_point[indices[ic + 2]].first;
            bool        clockwise = (((point_1 - point_0) ^ (point_2 - point_0)) < 0.0f);

            for (int jc = 0; jc < 3; ++jc) {
                int origin_point_index =
                    index_to_origin_point[clockwise ? indices[ic + jc] : indices[ic + (2 - jc)]].second;

                auto itr_insert =
                    index_to_tria_index.insert(std::make_pair(origin_point_index, (int)out_vertices.size()));
                if (itr_insert.second) {
                    out_indices.emplace_back((unsigned int)out_vertices.size());
                    Vertexf v = out_vertices_input[origin_point_index];
                    v.m_normal = fill_normal;
                    out_vertices.emplace_back(v);
                    out_index_old_new[origin_point_index] = out_vertices.size() - 1;
                }
                else {
                    out_indices.emplace_back((unsigned int)itr_insert.first->second);
                }
            }
        }
    }
    catch (...) {
        assert(false);
    }
}

void Clipping::addSegents(const std::vector<ClippingLoop*>& outline, std::vector<unsigned int>& out_segments,
                          std::vector<Vertexf>& out_vertices_input, std::vector<Vertexf>& out_vertices,
                          std::map<int, int>& out_index_old_new)
{
    if (outline.empty()) {
        return;
    }

    auto& outer = outline[0];
    int   count = outer->count();
    if (count < 3) {
        return;
    }

    /// Outer
    for (int ic = 0; ic < count; ++ic) {
        int index_0 = outer->clockWiseData(ic, true);
        int index_1 = outer->clockWiseData((ic + 1) % count, true);
        if (index_0 != index_1) {
            auto itr_find_0 = out_index_old_new.find(index_0);
            if (itr_find_0 != out_index_old_new.end()) {
                out_segments.emplace_back((unsigned int)itr_find_0->second);
            }
            else {
                out_vertices.emplace_back(out_vertices_input[index_0]);
                out_index_old_new[index_0] = out_vertices.size() - 1;
                out_segments.emplace_back((unsigned int)(out_vertices.size() - 1));
            }

            auto itr_find_1 = out_index_old_new.find(index_1);
            if (itr_find_1 != out_index_old_new.end()) {
                out_segments.emplace_back((unsigned int)itr_find_1->second);
            }
            else {
                out_vertices.emplace_back(out_vertices_input[index_1]);
                out_index_old_new[index_1] = out_vertices.size() - 1;
                out_segments.emplace_back((unsigned int)(out_vertices.size() - 1));
            }
        }
    }
    /// Inner
    for (int ic = 1; ic < (int)outline.size(); ++ic) {
        auto& inner = outline[ic];

        int count = inner->count();
        for (int jc = 0; jc < count; ++jc) {
            int index_0 = inner->clockWiseData(jc, false);
            int index_1 = inner->clockWiseData((jc + 1) % count, false);
            if (index_0 != index_1) {
                auto itr_find_0 = out_index_old_new.find(index_0);
                if (itr_find_0 != out_index_old_new.end()) {
                    out_segments.emplace_back((unsigned int)itr_find_0->second);
                }
                else {
                    out_vertices.emplace_back(out_vertices_input[index_0]);
                    out_index_old_new[index_0] = out_vertices.size() - 1;
                    out_segments.emplace_back((unsigned int)(out_vertices.size() - 1));
                }

                auto itr_find_1 = out_index_old_new.find(index_1);
                if (itr_find_1 != out_index_old_new.end()) {
                    out_segments.emplace_back((unsigned int)itr_find_1->second);
                }
                else {
                    out_vertices.emplace_back(out_vertices_input[index_1]);
                    out_index_old_new[index_1] = out_vertices.size() - 1;
                    out_segments.emplace_back((unsigned int)(out_vertices.size() - 1));
                }
            }
        }
    }
}

CORE_NAMESPACE_END
