#include "MyOpenGLWidget.h"
#include <QGuiApplication>
#include <QMatrix4x4>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QPainter>

#include "CustomTooltip.h"
#include "DimensionCtrl.h"
#include "GLRenderText.h"
#include "ResultCtrl.h"
#include "UtilityCtrl.h"
#include "Vox3DForm.h"

#include "Math/CoreMath.h"
#include "Math/Matrix4x4f.h"
#include "Math/Point3f.h"
#include "Math/Point4f.h"
#include "Scene/Dimension.h"
#include "Scene/MultiDimension.h"
#include "Scene/Shape.h"
#include "Scene/Voxel.h"
#include "Scene/VoxelScalar.h"
#include "Utils/Picking.h"

#include <algorithm>
#include <execution>

#include <QClipboard>
#include <QMenu>

MyOpenGLWidget::MyOpenGLWidget(QWidget* parent) : QOpenGLWidget(parent)
{
    setAcceptDrops(true);
}

MyOpenGLWidget::~MyOpenGLWidget()
{
    GLSceneRender::deleteSceneRender(m_scene_render);
}

/// Init
void MyOpenGLWidget::initGLWidget(Vox3DForm* form)
{
    m_3DForm = form;

    /// View追加
    m_scene_view = form->sceneGraph()->addSceneView();

    /// Init View
    m_scene_view->setViewPort(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());
    m_scene_view->initView();

    /// create
    m_scene_render = GLSceneRender::createSceneRender(m_scene_view.ptr());
}

void MyOpenGLWidget::updateXOR()
{
    m_scene_render->setXORRender(true);
    update();
    qApp->processEvents();    /// XORが消えない（イベント処理されずにもう一回来てしまう）ので入れておく
}

/// 専用のデータ管理
void MyOpenGLWidget::addMaterialIdToNode(int material_id, Node* node)
{
    m_material_node_map[material_id] = node;
    m_all_vox_bbox.expandBy(node->boundingBox());
}

void MyOpenGLWidget::setMaterialIdToNode(std::vector<std::pair<int, RefPtr<Node>>>& material_id_node_list)
{
    clearMaterialIdToNode();

    for (auto& [id, node] : material_id_node_list) {
        addMaterialIdToNode(id, node.ptr());
        m_scene_render->createRenderableData(node->renderable());
    }
}

void MyOpenGLWidget::clearMaterialIdToNode(bool delete_node)
{
    if (delete_node) {
        /// Vox削除
        std::set<Node*> delete_nodes;
        auto            root_node = m_scene_view->sceneGraph()->rootNode();
        for (auto& child : root_node->children()) {
            if (child.ptr() && child->object<Voxel>()) {
                delete_nodes.insert(child.ptr());
            }
        }

        root_node->removeChild(delete_nodes);
        root_node->markBoxDirty();

        /// 自動寸法を削除
        clearAutoDimensions();
    }
    m_material_node_map.clear();
    m_all_vox_bbox.init();
}

void MyOpenGLWidget::setMaterialAllBBox(const BoundingBox3f& bbox)
{
    m_all_vox_bbox = bbox;
}

void MyOpenGLWidget::updateRenderData()
{
    m_scene_render->createRenderData(m_scene_view->sceneGraph()->rootNode());
}

void MyOpenGLWidget::setViewNearFar()
{
    m_scene_view->setViewNearFar();
}

void MyOpenGLWidget::suppressRender(bool suppress, bool update_unsuppress)
{
    m_scene_render->suppressRender(suppress);
    if (update_unsuppress && !suppress) {
        update();
    }
}

void MyOpenGLWidget::showMaterial(bool show, bool update_view)
{
    bool change_show = false;
    for (auto& [id, node] : m_material_node_map) {
        if (node.isAlive()) {
            node->setShowHierarchy(show);
            change_show = true;
        }
    }
    if (change_show) {
        setViewNearFar();
        if (update_view) {
            update();
        }
    }
}
void MyOpenGLWidget::showMaterial(int material_id, bool show, bool update_view)
{
    auto find_id = m_material_node_map.find(material_id);
    if (find_id != m_material_node_map.end()) {
        auto& node = find_id->second;
        if (node.isAlive()) {
            node->setShowHierarchy(show);
            setViewNearFar();
            if (update_view) {
                update();
            }
        }
    }
}

bool MyOpenGLWidget::noshowMaterial(QList<int>& mateial_ids, bool update_view)
{
    /// 変更があるか
    bool      change = false;
    QSet<int> material_ids_set(mateial_ids.begin(), mateial_ids.end());
    for (auto& [id, node] : m_material_node_map) {
        if (material_ids_set.contains(id)) {
            if (node.isAlive() && node->isShow()) {
                change = true;
                break;
            }
        }
        else {
            if (node.isAlive() && !node->isShow()) {
                change = true;
                break;
            }
        }
    }
    if (!change) {
        return false;
    }

    showMaterial(true, false);

    for (auto& id : mateial_ids) {
        auto find_id = m_material_node_map.find(id);
        if (find_id != m_material_node_map.end()) {
            auto& node = find_id->second;
            if (node.isAlive()) {
                node->setShowHierarchy(false);
            }
        }
    }

    if (update_view) {
        update();
    }

    return true;
}

void MyOpenGLWidget::materialIds(std::set<int>& material_ids, bool only_show)
{
    for (auto& [id, node] : m_material_node_map) {
        if (node.isAlive() && (!only_show || node->isShow())) {
            material_ids.insert(id);
        }
    }
}

bool MyOpenGLWidget::voxelCellInfo(Point3d& org, Point3d& delta, Point3i& number)
{
    for (auto& [id, node] : m_material_node_map) {
        if (node.isAlive()) {
            auto voxel = node->object<Voxel>();
            if (voxel) {
                org    = voxel->org();
                delta  = voxel->delta();
                number = voxel->number();
                return true;
            }
        }
    }
    return false;
}

Voxel* MyOpenGLWidget::firstVoxel(bool only_display)
{
    for (auto& [id, node] : m_material_node_map) {
        if (node.isAlive()) {
            if (only_display) {
                if (!node->isVisible()) {
                    continue;
                }
            }
            auto voxel = node->object<Voxel>();
            if (voxel) {
                return voxel;
            }
        }
    }

    return nullptr;
}

void MyOpenGLWidget::transparentMateial(QList<int>& mateial_ids, bool update_view)
{
    for (auto& [id, node] : m_material_node_map) {
        if (node.isAlive()) {
            node->setTransparent(1.0f);
        }
    }
    for (auto& id : mateial_ids) {
        auto find_id = m_material_node_map.find(id);
        if (find_id != m_material_node_map.end()) {
            auto& node = find_id->second;
            if (node.isAlive()) {
                node->setTransparent(0.4f);
            }
        }
    }
    if (update_view) {
        update();
    }
}

void MyOpenGLWidget::drawMode(const std::map<int, std::pair<bool, bool>>& draw_mode_list, bool update_view)
{
    for (auto& [id, node] : m_material_node_map) {
        if (node.isAlive()) {
            auto itr = draw_mode_list.find(id);
            if (itr != draw_mode_list.end()) {
                node->setDrawShading(itr->second.first);
                node->setDrawWireframe(itr->second.second);
            }
        }
    }
    if (update_view) {
        update();
    }
}

void MyOpenGLWidget::setDrawMode(int material_id, bool shading, bool wireframe, bool update_view)
{
    auto find_id = m_material_node_map.find(material_id);
    if (find_id != m_material_node_map.end()) {
        auto& node = find_id->second;
        if (node.isAlive()) {
            node->setDrawShading(shading);
            node->setDrawWireframe(wireframe);
            if (update_view) {
                update();
            }
        }
    }
}

void MyOpenGLWidget::optMateial(QList<int>& mateial_ids, bool update_view)
{
    for (auto& [id, node] : m_material_node_map) {
        if (node.isAlive()) {
            node->setProjectionNode(nullptr);
        }
    }

    if (m_opt_3d_node != nullptr) {
        for (auto& id : mateial_ids) {
            auto find_id = m_material_node_map.find(id);
            if (find_id != m_material_node_map.end()) {
                auto& node = find_id->second;
                if (node.isAlive()) {
                    node->setProjectionNode(m_opt_3d_node.ptr());
                }
            }
        }
        m_opt_3d_node->unsuppress();
        if (hasProjectOpt(false)) {
            auto result_ctrl = m_3DForm->resultCtl();
            if (result_ctrl) {
                if (result_ctrl->isHideOnProjection()) {
                    if (m_opt_3d_node != nullptr) {
                        m_opt_3d_node->suppress();
                    }
                }
            }
        }
    }

    if (update_view) {
        update();
    }
}

void MyOpenGLWidget::setColor(int material_id, const Point3f& color, bool update_view)
{
    auto find_id = m_material_node_map.find(material_id);
    if (find_id != m_material_node_map.end()) {
        auto& node = find_id->second;
        if (node.isAlive()) {
            node->setColor(color);
        }
    }

    if (update_view) {
        update();
    }
}

bool MyOpenGLWidget::hasProjectOpt(bool display) const
{
    for (auto& [id, node] : m_material_node_map) {
        if (node.isAlive()) {
            if (display) {
                if (!node->isVisible()) {
                    continue;
                }
            }

            auto shape = node->shape();
            if (shape && shape->isVoxel()) {
                if (((Node*)node.ptr())->projectionNode()) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool MyOpenGLWidget::hasOptOp2()
{
    auto root_node = m_scene_view->sceneGraph()->rootNode();

    for (auto& child : root_node->children()) {
        if (child->object<VoxelScalar>()) {
            return true;
        }
    }

    return false;
}

void MyOpenGLWidget::initView()
{
    m_scene_view->viewDirection(6);

    fitDisplay();
}

void MyOpenGLWidget::rotateLeft()
{
    m_scene_view->rotateLeft();
    update();
}

void MyOpenGLWidget::rotateRight()
{
    m_scene_view->rotateRight();
    update();
}

void MyOpenGLWidget::rotateUp()
{
    m_scene_view->rotateUp();
    update();
}

void MyOpenGLWidget::rotateDown()
{
    m_scene_view->rotateDown();
    update();
}

void MyOpenGLWidget::rotate(const Point3f& axis, float degree)
{
    m_scene_view->rotateDegree(axis, degree);
    update();
}

void MyOpenGLWidget::zoomIn()
{
    m_scene_view->scale(0.8f);
    update();
}

void MyOpenGLWidget::zoomOut()
{
    m_scene_view->scale(1.0f / 0.8f);
    update();
}

void MyOpenGLWidget::setProjectionOrtho()
{
    m_scene_view->setProjectionType(SceneView::ProjectionType::Ortho);
    m_scene_view->updateProjectionMatrix();
    update();
}

void MyOpenGLWidget::setProjectionPerspective()
{
    m_scene_view->setProjectionType(SceneView::ProjectionType::Perspective);
    m_scene_view->updateProjectionMatrix();
    update();
}

void MyOpenGLWidget::setZAxisRotateMode(bool mode)
{
    if (m_scene_view->setZAxisRotateMode(mode)) {
        update();
    }
}

bool MyOpenGLWidget::isZAxisRotateMode() const
{
    return m_scene_view->isZAxisRotateMode();
}

void MyOpenGLWidget::setLightDir(const Point3f& lightDir, bool update_view)
{
    m_scene_view->setLightDir(lightDir);
    if (update_view) {
        update();
    }
}

bool MyOpenGLWidget::isEqaulLightDir(const Point3f& lightDir) const
{
    return m_scene_view->isEqaulLightDir(lightDir);
}

void MyOpenGLWidget::frontView()
{
    m_scene_view->frontView();
    update();
}

void MyOpenGLWidget::backView()
{
    m_scene_view->backView();
    update();
}

void MyOpenGLWidget::leftView()
{
    m_scene_view->leftView();
    update();
}

void MyOpenGLWidget::rightView()
{
    m_scene_view->rightView();
    update();
}

void MyOpenGLWidget::topView()
{
    m_scene_view->topView();
    update();
}

void MyOpenGLWidget::bottomView()
{
    m_scene_view->bottomView();
    update();
}

void MyOpenGLWidget::axoView()
{
    m_scene_view->axoView();
    update();
}

void MyOpenGLWidget::dirView(const Point3f& dir)
{
    if (m_scene_view->dirView(dir)) {
        update();
    }
}

void MyOpenGLWidget::initializeGL()
{
    m_scene_render->initializeGL(this, this);
}

void MyOpenGLWidget::paintGL()
{
    /// ツールチップあれば非表示
    CustomTooltipManager::close();

    m_scene_render->paintGL();
}

void MyOpenGLWidget::resizeGL(int w, int h)
{
    m_scene_render->resizeGL(w, h);

    if (m_3DForm->utilityCtrl()) {
        m_3DForm->utilityCtrl()->update();
    }
}

void MyOpenGLWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_last_mouse_position = event->pos();
        m_enable_last_press   = true;

        QPoint   global_mouse_pos = QCursor::pos();
        QPoint   local_mouse_pos  = mapFromGlobal(global_mouse_pos) * devicePixelRatio();
        PickData pick_data;
        m_scene_render->dragGL(Point2i(local_mouse_pos.x(), local_mouse_pos.y()), 5, pick_data);
        if (pick_data.pickNode() != nullptr) {
            auto object = pick_data.pickNode()->object();
            if (object && object->type() == ObjectType::Dimension) {
                Dimension* dimension = (Dimension*)object;

                m_drag_start          = dimension->strPosStart();
                m_drag_node           = pick_data.pickNode();
                m_drag_object         = dimension;
                m_last_press_position = m_last_mouse_position;
            }
            else if (object && object->type() == ObjectType::MultiDimension) {
                auto pick_node_obj = pick_data.pickNodeObject();
                if (pick_node_obj && pick_node_obj->type() == ObjectType::Dimension) {
                    Dimension* dimension = (Dimension*)pick_node_obj;

                    m_drag_start          = dimension->strPosStart();
                    m_drag_node           = pick_data.pickNode();
                    m_drag_object         = dimension;
                    m_last_press_position = m_last_mouse_position;
                }
                else {
                    assert(0);
                }
            }
        }
    }
    else if (event->button() == Qt::MiddleButton) {
        m_last_mouse_position = event->pos();
    }

    setFocus();
}

void MyOpenGLWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_drag_node   = nullptr;
        m_drag_object = nullptr;

        QString tool_tip;
        int     tool_tip_msec = 5000;

        if (m_enable_last_press) {
            QPoint             global_mouse_pos = QCursor::pos();
            QPoint             local_mouse_pos  = mapFromGlobal(global_mouse_pos) * devicePixelRatio();
            PickData           pick_data;
            std::vector<Node*> pick_nodes;
            m_scene_render->pickGL(Point2i(local_mouse_pos.x(), local_mouse_pos.y()), 5, pick_data,
                                   (PickSnap)(m_pick_mode ? (m_pick_snap | PickSnap::SnapAny) : PickSnap::SnapNone),
                                   &pick_nodes);
            if (pick_data.pickNode() != nullptr) {
                adjustPickData(pick_data, true, &pick_nodes);

                auto dimension_ctrl = m_3DForm->dimensionCtrl();

                /// 暫定
                int material_id = -1;
                for (auto& [id, node] : m_material_node_map) {
                    if (node.ptr() == pick_data.pickNode()) {
                        material_id = id;
                        break;
                    }
                }
                if (material_id >= 0) {
                    m_3DForm->selectMaterialList(material_id);

                    /// opt/op2が形状と重なっていた場合を考慮
                    /// optはisShowInformationOnClickのときだけ（現状select2dVoxel相当ないので）
                    ///  ※ピックと見た目が違うので対策
                    if (m_3DForm->resultCtl()->isShowInformationOnClick()
                        || (m_3DForm->resultCtl()->op2List().size() > 0 && m_scene_render->voxelScalarPriority())) {
                        m_scene_render->setPickVoxelScalarPriorityInvalid(false);
                        m_scene_render->setPickWireframeInvalid(true);
                        PickData           pick_data_2;
                        std::vector<Node*> pick_nodes_2;
                        m_scene_render->pickGL(Point2i(local_mouse_pos.x(), local_mouse_pos.y()), 5, pick_data_2,
                                               PickSnap::SnapNone, &pick_nodes_2);
                        m_scene_render->setPickVoxelScalarPriorityInvalid(true);
                        m_scene_render->setPickWireframeInvalid(false);

                        bool hit_voxel_scalar = false;
                        if (pick_data_2.pickNode() != nullptr) {
                            auto pick_node_object = pick_data_2.pickNodeObject();
                            if (pick_node_object && pick_node_object->type() == ObjectType::VoxelScalar) {
                                m_3DForm->resultCtl()->select2dVoxel(pick_data_2.pickNode());
                                hit_voxel_scalar = true;
                                if (m_3DForm->resultCtl()->isShowInformationOnClick()) {
                                    adjustPickData(pick_data_2, true, &pick_nodes_2);

                                    auto adj_norm = -pick_data_2.pickNorm();
                                    if (adj_norm.length2() == 0.0f) {
                                        adj_norm = m_scene_view->cameraDir(pick_data_2.pickPoint());
                                    }
                                    /// 微小にずらす
                                    auto global_point =
                                        pick_data_2.pickPoint()
                                        + adj_norm * m_scene_view->sceneGraph()->lengthUnit().epsilonValidLength();

                                    auto path_matrix = pick_data_2.pickNode()->pathMatrix();
                                    path_matrix.setToInverse();

                                    auto local_point = path_matrix * global_point;

                                    VoxelScalar* voxel_scalar = (VoxelScalar*)pick_node_object;
                                    if (!voxel_scalar->scalarData()) {
                                        auto project_node = pick_data_2.pickNode()->projectionNode();
                                        if (project_node && project_node->object<VoxelScalar>()) {
                                            voxel_scalar = project_node->object<VoxelScalar>();
                                        }
                                    }

                                    float value = voxel_scalar->value(local_point);
                                    if (value >= 0) {
                                        tool_tip      = QString::number(value, 'E', 5);
                                        tool_tip_msec = m_3DForm->resultCtl()->showInformationOnClickMsec();
                                    }
                                }
                            }
                        }

                        /// Opt投影を考慮
                        if (!hit_voxel_scalar) {
                            auto pick_node_object = pick_data.pickNodeObject();    /// 最初のピック使う
                            if (pick_node_object && pick_node_object->type() == ObjectType::Voxel) {
                                if (m_3DForm->resultCtl()->isShowInformationOnClick()) {
                                    Voxel* voxel = (Voxel*)pick_node_object;
                                    Node*  opt_node =
                                        pick_data.pickNode() ? pick_data.pickNode()->projectionNode() : nullptr;
                                    if (opt_node && opt_node->object<VoxelScalar>()) {
                                        auto adj_norm = -pick_data.pickNorm();
                                        if (adj_norm.length2() == 0.0f) {
                                            adj_norm = m_scene_view->cameraDir(pick_data.pickPoint());
                                        }
                                        /// 微小にずらす
                                        auto global_point =
                                            pick_data.pickPoint()
                                            + adj_norm * m_scene_view->sceneGraph()->lengthUnit().epsilonValidLength();

                                        auto path_matrix = pick_data.pickNode()->pathMatrix();
                                        path_matrix.setToInverse();

                                        auto local_point = path_matrix * global_point;

                                        VoxelScalar* voxel_scalar = opt_node->object<VoxelScalar>();

                                        float value = voxel_scalar->value(local_point);
                                        if (value >= 0 && voxel_scalar->isIncludingRangeMinMax(value)) {
                                            tool_tip      = QString::number(value, 'E', 5);
                                            tool_tip_msec = m_3DForm->resultCtl()->showInformationOnClickMsec();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else if (!m_pick_mode) {    /// 作成モードでヒットすると不自然だったので
                    auto pick_node_object = pick_data.pickNodeObject();
                    if (pick_node_object && pick_node_object->type() == ObjectType::Dimension) {
                        dimension_ctrl->selectDimension(pick_data.pickNode());
                    }
                    if (pick_node_object && pick_node_object->type() == ObjectType::VoxelScalar) {
                        m_3DForm->resultCtl()->select2dVoxel(pick_data.pickNode());

                        if (m_3DForm->resultCtl()->isShowInformationOnClick()) {
                            auto adj_norm = -pick_data.pickNorm();
                            if (adj_norm.length2() == 0.0f) {
                                adj_norm = m_scene_view->cameraDir(pick_data.pickPoint());
                            }
                            /// 微小にずらす
                            auto global_point =
                                pick_data.pickPoint()
                                + adj_norm * m_scene_view->sceneGraph()->lengthUnit().epsilonValidLength();

                            auto path_matrix = pick_data.pickNode()->pathMatrix();
                            path_matrix.setToInverse();

                            auto local_point = path_matrix * global_point;

                            VoxelScalar* voxel_scalar = (VoxelScalar*)pick_node_object;
                            if (!voxel_scalar->scalarData()) {
                                auto project_node = pick_data.pickNode()->projectionNode();
                                if (project_node && project_node->object<VoxelScalar>()) {
                                    voxel_scalar = project_node->object<VoxelScalar>();
                                }
                            }

                            float value = voxel_scalar->value(local_point);
                            if (value >= 0) {
                                tool_tip      = QString::number(value, 'E', 5);
                                tool_tip_msec = m_3DForm->resultCtl()->showInformationOnClickMsec();
                            }
                        }
                    }
                }

                if (m_pick_mode) {
                    m_scene_render->addPickObject(pick_data);

                    std::vector<PickData>& pick_objects = m_scene_render->pickObjects();

                    if (dimension_ctrl->isCreateDimension2Points()) {
                        if (pick_objects.size() == 2) {
                            dimension_ctrl->createDimension(m_scene_view.ptr(), pick_objects[0].pickPoint(),
                                                            pick_objects[1].pickPoint(), false, true);
                            m_scene_render->clearPickObjects();
                            dimension_ctrl->clearTemporaryDimension();
                            dimension_ctrl->clearTemporaryAutoDimension();
                        }
                    }
                    else if (dimension_ctrl->isCreateAutoDimension()) {
                        if ((pick_objects.size() == 1 && dimension_ctrl->isAutoDimension1Pick())
                            || (pick_objects.size() == 2 && !dimension_ctrl->isAutoDimension1Pick())) {
                            std::vector<Point3f> points;
                            for (auto& pick : pick_objects) {
                                points.emplace_back(pick.pickPoint());
                            }
                            bool ret = dimension_ctrl->createAutoDimension(m_scene_view.ptr(), points, false, false,
                                                                           false, true);

                            if (ret) {
                                m_scene_render->clearPickObjects();
                                dimension_ctrl->clearTemporaryDimension();
                                dimension_ctrl->clearTemporaryAutoDimension();
                                dimension_ctrl->clearTemporaryPreviewAutoDimension();

                                if (dimension_ctrl->isContinuousAutoDimension()) {
                                    Point3f point = dimension_ctrl->continousAutoDimensionPoint();
                                    pick_data.setPickObject(nullptr);
                                    pick_data.setPickPoint(point);
                                    m_scene_render->addPickObject(pick_data);
                                }
                            }
                            else {
                                m_scene_render->popPickObject();
                            }
                        }
                    }

                    update();
                }
            }

            m_enable_last_press = false;
        }

        if (!tool_tip.isEmpty()) {
            const auto clipboard = QApplication::clipboard();
            clipboard->setText(tool_tip);
            CustomTooltipManager::show(tool_tip, tool_tip_msec);
        }
    }
}

void MyOpenGLWidget::mouseMoveEvent(QMouseEvent* event)
{
    int dx = (event->x() - m_last_mouse_position.x()) * devicePixelRatio();
    int dy = (event->y() - m_last_mouse_position.y()) * devicePixelRatio();

    if (dx == 0 && dy == 0) {
        return;
    }

    m_enable_last_press = false;

    bool rotate = false;
    bool pan    = false;

    if (event->buttons() & Qt::LeftButton) {
        if (m_mouse_mode == MouseMode::Rotate) {
            if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier) {
                pan = true;
            }
            else {
                rotate = true;
            }
        }
        else if (m_mouse_mode == MouseMode::Pan) {
            if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier) {
                rotate = true;
            }
            else {
                pan = true;
            }
        }
    }
    else if (event->buttons() & Qt::MiddleButton) {
        if (m_mouse_mode == MouseMode::Rotate) {
            pan = true;
        }
        else if (m_mouse_mode == MouseMode::Pan) {
            rotate = true;
        }
    }

    // ドラッグ中の要素を移動
    if (m_drag_node != nullptr) {
        auto ep_phys = event->pos() * devicePixelRatio();
        auto lp_phys = m_last_press_position * devicePixelRatio();

        float start_depth = m_scene_view->projectToScreen(m_drag_start).z();
        auto  new_pos     = m_scene_view->unprojectFromScreen(Point3f(ep_phys.x(), ep_phys.y(), start_depth));
        auto  old_pos     = m_scene_view->unprojectFromScreen(Point3f(lp_phys.x(), lp_phys.y(), start_depth));

        auto move_obj = new_pos - old_pos;

        auto object = m_drag_node->object();
        if (object && object->type() == ObjectType::Dimension) {
            Dimension* dimension = (Dimension*)object;

            dimension->setTextPosition(move_obj + m_drag_start);
            dimension->markBoxDirty();
            m_drag_node->markBoxDirty();
            m_scene_view->setViewNearFar(false);
        }
        else if (object && object->type() == ObjectType::MultiDimension) {
            Dimension* dimension = (Dimension*)m_drag_object.ptr();

            dimension->setTextPosition(move_obj + m_drag_start);
            dimension->markBoxDirty();
            ((MultiDimension*)object)->markBoxDirty();
            m_drag_node->markBoxDirty();
            m_scene_view->setViewNearFar(false);
        }
        update();
    }
    else if (pan) {
        m_scene_view->translateMouse(dx, dy);
        update();
    }
    else if (rotate) {
        m_scene_view->rotateMouse(dx, dy);
        update();
    }
    else {
        dimensionMouseMove();
    }

    m_last_mouse_position = event->pos();
}

void MyOpenGLWidget::wheelEvent(QWheelEvent* event)
{
    int delta = event->angleDelta().y();
    if (delta != 0) {
        /// 反転なしの場合、MSOfficeとか一般的な方に合わせる
        if (!m_mouse_zoom_reverse) {
            delta = -delta;
        }

        QPoint global_mouse_pos = QCursor::pos();
        QPoint local_mouse_pos  = mapFromGlobal(global_mouse_pos) * devicePixelRatio();
        m_scene_view->scaleMouse(local_mouse_pos.x(), local_mouse_pos.y(), delta);
        update();
    }
}

class CustomMenu : public QMenu {
public:
    CustomMenu(MyOpenGLWidget* open_gl_widget, Vox3DForm* form)
        : QMenu(nullptr)
        , m_open_gl_widget(open_gl_widget)
        , m_3DForm(form)
    {
    }
    CustomMenu(const QString& str, MyOpenGLWidget* open_gl_widget, Vox3DForm* form)
        : QMenu(str, nullptr)
        , m_open_gl_widget(open_gl_widget)
        , m_3DForm(form)
    {
    }

protected:
    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->key() == Qt::Key_F) {
            close();
            m_open_gl_widget->fitDisplay();
        }
        else if (event->key() == Qt::Key_C) {
            close();
            m_open_gl_widget->keyPressEvent(event);
        }
        else if (event->key() == Qt::Key_Escape) {
            close();
            m_open_gl_widget->pickClear();
        }
        else if (event->key() == Qt::Key_I) {
            close();
            m_open_gl_widget->imageCopyToClipboard();
        }
        else if (event->key() == Qt::Key_P) {
            close();
            m_open_gl_widget->imageCopyToFile();
        }
        /*
        else if (event->key() == Qt::Key_A) {
            close();
            if (event->modifiers() & Qt::ShiftModifier) {
                m_3DForm->allShow(false);
            }
            else {
                m_3DForm->allShow(true);
            }
        }
        else if (event->key() == Qt::Key_Q) {
            close();
            if (event->modifiers() & Qt::ShiftModifier) {
                m_3DForm->voxAllShow(false);
            }
            else if (event->modifiers() & Qt::ControlModifier) {
                m_3DForm->voxOnlyShow(true);
            }
            else {
                m_3DForm->voxAllShow(true);
            }
        }
        else if (event->key() == Qt::Key_W) {
            close();
            if (event->modifiers() & Qt::ShiftModifier) {
                m_3DForm->optAllShow(false);
            }
            else if (event->modifiers() & Qt::ControlModifier) {
                m_3DForm->optOnlyShow(true);
            }
            else {
                m_3DForm->optAllShow(true);
            }
        }
        else if (event->key() == Qt::Key_E) {
            close();
            if (event->modifiers() & Qt::ShiftModifier) {
                m_3DForm->op2AllShow(false);
            }
            else if (event->modifiers() & Qt::ControlModifier) {
                m_3DForm->op2OnlyShow(true);
            }
            else {
                m_3DForm->op2AllShow(true);
            }
        }
        else if (event->key() == Qt::Key_R) {
            close();
            if (event->modifiers() & Qt::ShiftModifier) {
                m_3DForm->dimensionAllShow(false);
            }
            else if (event->modifiers() & Qt::ControlModifier) {
                m_3DForm->dimensionOnlyShow(true);
            }
            else {
                m_3DForm->dimensionAllShow(true);
            }
        }
        else if (event->key() == Qt::Key_T) {
            close();
            if (event->modifiers() & Qt::ShiftModifier) {
                m_3DForm->autoDimensionAllShow(false);
            }
            else if (event->modifiers() & Qt::ControlModifier) {
                m_3DForm->autoDimensionOnlyShow(true);
            }
            else {
                m_3DForm->autoDimensionAllShow(true);
            }
        }
        */
        else {
            QMenu::keyPressEvent(event);
        }
    }
    MyOpenGLWidget* m_open_gl_widget;
    Vox3DForm*      m_3DForm;
};

void MyOpenGLWidget::contextMenuEvent(QContextMenuEvent* event)
{
    if (m_pick_mode) {
        /// Autoで連続の時に確定処理
        auto dimension_ctrl = m_3DForm->dimensionCtrl();
        if (dimension_ctrl->isDuringContinuousAutoDimension() && !m_scene_render->pickObjects().empty()) {
            pickClear();
            return;
        }
    }

    QAction* action1 = new QAction("Fit Display", this);
    QAction* action2 = new QAction("Rotate Center", this);
    QAction* action3 = new QAction("Pick Clear", this);
    QAction* action4 = new QAction("Image Copy to Clipboard", this);
    QAction* action5 = new QAction("Image Copy to File", this);
    action1->setShortcut(QKeySequence("F"));
    action2->setShortcut(QKeySequence("C"));
    action3->setShortcut(QKeySequence("Esc"));
    action4->setShortcut(QKeySequence("I"));
    action5->setShortcut(QKeySequence("P"));
    action1->setShortcutContext(Qt::ApplicationShortcut);
    action2->setShortcutContext(Qt::ApplicationShortcut);
    action3->setShortcutContext(Qt::ApplicationShortcut);
    action4->setShortcutContext(Qt::ApplicationShortcut);
    action5->setShortcutContext(Qt::ApplicationShortcut);

    CustomMenu sub_show_menu("Show", this, m_3DForm);
    QAction*   action_show_all     = new QAction("All", this);
    QAction*   action_show_vox     = new QAction("Vox", this);
    QAction*   action_show_opt     = new QAction("Opt", this);
    QAction*   action_show_op2     = new QAction("Op2", this);
    QAction*   action_show_dim     = new QAction("寸法", this);
    QAction*   action_show_autodim = new QAction("自動寸法", this);
    /*
    action_show_all->setShortcut(QKeySequence("A"));
    action_show_vox->setShortcut(QKeySequence("Q"));
    action_show_opt->setShortcut(QKeySequence("W"));
    action_show_op2->setShortcut(QKeySequence("E"));
    action_show_dim->setShortcut(QKeySequence("R"));
    action_show_autodim->setShortcut(QKeySequence("T"));
    action_show_all->setShortcutContext(Qt::ApplicationShortcut);
    action_show_vox->setShortcutContext(Qt::ApplicationShortcut);
    action_show_opt->setShortcutContext(Qt::ApplicationShortcut);
    action_show_op2->setShortcutContext(Qt::ApplicationShortcut);
    action_show_dim->setShortcutContext(Qt::ApplicationShortcut);
    action_show_autodim->setShortcutContext(Qt::ApplicationShortcut);
    */
    sub_show_menu.addAction(action_show_all);
    sub_show_menu.addAction(action_show_vox);
    sub_show_menu.addAction(action_show_opt);
    sub_show_menu.addAction(action_show_op2);
    sub_show_menu.addAction(action_show_dim);
    sub_show_menu.addAction(action_show_autodim);

    CustomMenu sub_hide_menu("Hide", this, m_3DForm);
    QAction*   action_hide_all     = new QAction("All", this);
    QAction*   action_hide_vox     = new QAction("Vox", this);
    QAction*   action_hide_opt     = new QAction("Opt", this);
    QAction*   action_hide_op2     = new QAction("Op2", this);
    QAction*   action_hide_dim     = new QAction("寸法", this);
    QAction*   action_hide_autodim = new QAction("自動寸法", this);
    sub_hide_menu.addAction(action_hide_all);
    sub_hide_menu.addAction(action_hide_vox);
    sub_hide_menu.addAction(action_hide_opt);
    sub_hide_menu.addAction(action_hide_op2);
    sub_hide_menu.addAction(action_hide_dim);
    sub_hide_menu.addAction(action_hide_autodim);

    CustomMenu sub_only_menu("Show Only", this, m_3DForm);
    QAction*   action_only_vox     = new QAction("Vox", this);
    QAction*   action_only_opt     = new QAction("Opt", this);
    QAction*   action_only_op2     = new QAction("Op2", this);
    QAction*   action_only_dim     = new QAction("寸法", this);
    QAction*   action_only_autodim = new QAction("自動寸法", this);
    sub_only_menu.addAction(action_only_vox);
    sub_only_menu.addAction(action_only_opt);
    sub_only_menu.addAction(action_only_op2);
    sub_only_menu.addAction(action_only_dim);
    sub_only_menu.addAction(action_only_autodim);

    CustomMenu sub_hdet_menu("Hide Except", this, m_3DForm);
    QAction*   action_hdet_vox     = new QAction("Vox", this);
    QAction*   action_hdet_opt     = new QAction("Opt", this);
    QAction*   action_hdet_op2     = new QAction("Op2", this);
    QAction*   action_hdet_dim     = new QAction("寸法", this);
    QAction*   action_hdet_autodim = new QAction("自動寸法", this);
    sub_hdet_menu.addAction(action_hdet_vox);
    sub_hdet_menu.addAction(action_hdet_opt);
    sub_hdet_menu.addAction(action_hdet_op2);
    sub_hdet_menu.addAction(action_hdet_dim);
    sub_hdet_menu.addAction(action_hdet_autodim);

    /// 暫定:setShortcutが効かないので無理やり
    /// （ショートカットキー表記するのに、メニュー表示されているときは動作しないのが不自然なので）
    CustomMenu contextMenu(this, m_3DForm);
    contextMenu.addAction(action1);
    contextMenu.addAction(action2);
    contextMenu.addAction(action3);
    contextMenu.addSeparator();
    contextMenu.addMenu(&sub_show_menu);
    contextMenu.addMenu(&sub_hide_menu);
    contextMenu.addMenu(&sub_only_menu);
    contextMenu.addMenu(&sub_hdet_menu);
    contextMenu.addSeparator();
    contextMenu.addAction(action4);
    contextMenu.addAction(action5);

    QPoint globalMousePos = QCursor::pos();
    QPoint localMousePos  = mapFromGlobal(globalMousePos) * devicePixelRatio();

    connect(action1, &QAction::triggered, this, [this]() { fitDisplay(); });
    connect(action2, &QAction::triggered, this, [this, localMousePos]() { setRotationCenter(localMousePos); });
    connect(action3, &QAction::triggered, this, [this]() { pickClear(); });
    connect(action4, &QAction::triggered, this, [this]() { imageCopyToClipboard(); });
    connect(action5, &QAction::triggered, this, [this]() { imageCopyToFile(); });

    connect(action_show_all, &QAction::triggered, this, [this]() { m_3DForm->allShow(true); });
    connect(action_show_vox, &QAction::triggered, this, [this]() { m_3DForm->voxAllShow(true); });
    connect(action_show_opt, &QAction::triggered, this, [this]() { m_3DForm->optAllShow(true); });
    connect(action_show_op2, &QAction::triggered, this, [this]() { m_3DForm->op2AllShow(true); });
    connect(action_show_dim, &QAction::triggered, this, [this]() { m_3DForm->dimensionAllShow(true); });
    connect(action_show_autodim, &QAction::triggered, this, [this]() { m_3DForm->autoDimensionAllShow(true); });

    connect(action_hide_all, &QAction::triggered, this, [this]() { m_3DForm->allShow(false); });
    connect(action_hide_vox, &QAction::triggered, this, [this]() { m_3DForm->voxAllShow(false); });
    connect(action_hide_opt, &QAction::triggered, this, [this]() { m_3DForm->optAllShow(false); });
    connect(action_hide_op2, &QAction::triggered, this, [this]() { m_3DForm->op2AllShow(false); });
    connect(action_hide_dim, &QAction::triggered, this, [this]() { m_3DForm->dimensionAllShow(false); });
    connect(action_hide_autodim, &QAction::triggered, this, [this]() { m_3DForm->autoDimensionAllShow(false); });

    connect(action_only_vox, &QAction::triggered, this, [this]() { m_3DForm->voxOnlyShow(true, true); });
    connect(action_only_opt, &QAction::triggered, this, [this]() { m_3DForm->optOnlyShow(true, true); });
    connect(action_only_op2, &QAction::triggered, this, [this]() { m_3DForm->op2OnlyShow(true, true); });
    connect(action_only_dim, &QAction::triggered, this, [this]() { m_3DForm->dimensionOnlyShow(true, true); });
    connect(action_only_autodim, &QAction::triggered, this, [this]() { m_3DForm->autoDimensionOnlyShow(true, true); });

    connect(action_hdet_vox, &QAction::triggered, this, [this]() { m_3DForm->voxOnlyShow(true, false); });
    connect(action_hdet_opt, &QAction::triggered, this, [this]() { m_3DForm->optOnlyShow(true, false); });
    connect(action_hdet_op2, &QAction::triggered, this, [this]() { m_3DForm->op2OnlyShow(true, false); });
    connect(action_hdet_dim, &QAction::triggered, this, [this]() { m_3DForm->dimensionOnlyShow(true, false); });
    connect(action_hdet_autodim, &QAction::triggered, this, [this]() { m_3DForm->autoDimensionOnlyShow(true, false); });

    contextMenu.exec(event->globalPos());
}

void MyOpenGLWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (!m_3DForm) {
        return;
    }
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MyOpenGLWidget::dropEvent(QDropEvent* event)
{
    if (!m_3DForm) {
        return;
    }

    QStringList path_list;

    QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl& url : urls) {
        path_list << url.toLocalFile();
    }

    m_3DForm->allLoadForDD(path_list);
}

void MyOpenGLWidget::dimensionMouseMove()
{
    auto dimension_ctrl = m_3DForm->dimensionCtrl();

    std::vector<PickData> pick_objects = m_scene_render->pickObjects();

    if (m_pick_mode && (dimension_ctrl->isCreateDimension2Points() || dimension_ctrl->isCreateAutoDimension2Points())
        && pick_objects.size() == 1) {
        QPoint             global_mouse_pos = QCursor::pos();
        QPoint             local_mouse_pos  = mapFromGlobal(global_mouse_pos) * devicePixelRatio();
        PickData           pick_data;
        std::vector<Node*> pick_nodes;
        m_scene_render->pickGL(Point2i(local_mouse_pos.x(), local_mouse_pos.y()), 5, pick_data,
                               (PickSnap)(m_pick_snap ? (m_pick_snap | PickSnap::SnapAny) : PickSnap::SnapNone),
                               &pick_nodes);
        if (pick_data.pickNode() != nullptr) {
            auto pick_type = pick_data.type();
            adjustPickData(pick_data, true, &pick_nodes);

            if (dimension_ctrl->isCreateDimension2Points()) {
                dimension_ctrl->createDimension(m_scene_view.ptr(), pick_objects[0].pickPoint(), pick_data.pickPoint(),
                                                true);
            }
            else {
                dimension_ctrl->createAutoDimension(m_scene_view.ptr(),
                                                    {pick_objects[0].pickPoint(), pick_data.pickPoint()}, true);
            }

            if (m_pick_snap != PickSnap::SnapNone) {
                if (pick_data.pickNode() != nullptr && pick_type != PickType::TypeNone) {
                    m_scene_render->setXORPointRender(pick_data.pickPoint());
                }
                else {
                    m_scene_render->resetXORCurrent();
                }
            }

            update();
        }
    }
    else if (m_pick_mode && m_pick_snap != PickSnap::SnapNone) {
        QPoint             global_mouse_pos = QCursor::pos();
        QPoint             local_mouse_pos  = mapFromGlobal(global_mouse_pos) * devicePixelRatio();
        PickData           pick_data;
        std::vector<Node*> pick_nodes;
        m_scene_render->pickGL(Point2i(local_mouse_pos.x(), local_mouse_pos.y()), 5, pick_data, m_pick_snap,
                               &pick_nodes);
        if (pick_data.pickNode() != nullptr) {
            adjustPickData(pick_data, true, &pick_nodes);

            if (m_scene_render->setXORPointRender(pick_data.pickPoint())) {
                update();
            }
        }
        else {
            if (m_scene_render->resetXORCurrent()) {
                update();
            }
        }
    }
}

void MyOpenGLWidget::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
        case Qt::Key_F: {
            fitDisplay();
            return;
        } break;
        case Qt::Key_C: {
            QPoint global_mouse_pos = QCursor::pos();
            QPoint local_mouse_pos  = mapFromGlobal(global_mouse_pos) * devicePixelRatio();

            setRotationCenter(local_mouse_pos);
            return;
        } break;
        case Qt::Key_Escape: {
            if (!pickClear()) {
                auto dimension_ctrl = m_3DForm->dimensionCtrl();
                dimension_ctrl->modeClear();
            }
            return;
        } break;
        case Qt::Key_Control: {
            auto dimension_ctrl = m_3DForm->dimensionCtrl();
            dimension_ctrl->setCtrlPress(true);
            dimensionMouseMove();
            dimension_ctrl->setCtrlPress(false);
            return;
        } break;
        case Qt::Key_I: {
            imageCopyToClipboard();
            return;
        } break;
        case Qt::Key_P: {
            imageCopyToFile();
            return;
        } break;
        /*
        case Qt::Key_A: {
            if (event->modifiers() & Qt::ShiftModifier) {
                m_3DForm->allShow(false);
            }
            else {
                m_3DForm->allShow(true);
            }
            return;
        }
        case Qt::Key_Q: {
            if (event->modifiers() & Qt::ShiftModifier) {
                m_3DForm->voxAllShow(false);
            }
            else if (event->modifiers() & Qt::ControlModifier) {
                m_3DForm->voxOnlyShow(true);
            }
            else {
                m_3DForm->voxAllShow(true);
            }
            return;
        }
        case Qt::Key_W: {
            if (event->modifiers() & Qt::ShiftModifier) {
                m_3DForm->optAllShow(false);
            }
            else if (event->modifiers() & Qt::ControlModifier) {
                m_3DForm->optOnlyShow(true);
            }
            else {
                m_3DForm->optAllShow(true);
            }
            return;
        }
        case Qt::Key_E: {
            if (event->modifiers() & Qt::ShiftModifier) {
                m_3DForm->op2AllShow(false);
            }
            else if (event->modifiers() & Qt::ControlModifier) {
                m_3DForm->op2OnlyShow(true);
            }
            else {
                m_3DForm->op2AllShow(true);
            }
            return;
        }
        case Qt::Key_R: {
            if (event->modifiers() & Qt::ShiftModifier) {
                m_3DForm->dimensionAllShow(false);
            }
            else if (event->modifiers() & Qt::ControlModifier) {
                m_3DForm->dimensionOnlyShow(true);
            }
            else {
                m_3DForm->dimensionAllShow(true);
            }
            return;
        }
        case Qt::Key_T: {
            if (event->modifiers() & Qt::ShiftModifier) {
                m_3DForm->autoDimensionAllShow(false);
            }
            else if (event->modifiers() & Qt::ControlModifier) {
                m_3DForm->autoDimensionOnlyShow(true);
            }
            else {
                m_3DForm->autoDimensionAllShow(true);
            }
            return;
        }
        */
        default:
            break;
    }

    // 他のキーイベントを親クラスに渡す
    QOpenGLWidget::keyPressEvent(event);
}

void MyOpenGLWidget::keyReleaseEvent(QKeyEvent* event)
{
    switch (event->key()) {
        case Qt::Key_Control: {
            auto dimension_ctrl = m_3DForm->dimensionCtrl();
            dimension_ctrl->setCtrlRelease(true);
            dimensionMouseMove();
            dimension_ctrl->setCtrlRelease(false);
        }
        default:
            break;
    }

    QOpenGLWidget::keyReleaseEvent(event);
}

void MyOpenGLWidget::setRotationCenter(const QPoint& local_mouse_pos)
{
    PickData pick_data;
    m_scene_render->pickGL(Point2i(local_mouse_pos.x(), local_mouse_pos.y()), 15, pick_data);
    if (pick_data.pickNode() != nullptr) {
        // DEBUG() << "PICK X " << pick_data.pickPoint().m_x;
        // DEBUG() << "PICK Y " << pick_data.pickPoint().m_y;
        // DEBUG() << "PICK Z " << pick_data.pickPoint().m_z;

        m_scene_view->setRotationCenter(pick_data.pickPoint(), true);
        update();

        /// 暫定
        int material_id = -1;
        for (auto& [id, node] : m_material_node_map) {
            if (node.ptr() == pick_data.pickNode()) {
                material_id = id;
                break;
            }
        }
        if (material_id >= 0) {
            m_3DForm->selectMaterialList(material_id);
        }
    }
}

bool MyOpenGLWidget::pickClear()
{
    if (!m_scene_render->pickObjects().empty()) {
        m_scene_render->clearPickObjects();

        auto dimension_ctrl = m_3DForm->dimensionCtrl();
        dimension_ctrl->clearTemporaryDimension();
        dimension_ctrl->clearTemporaryAutoDimension();

        if (dimension_ctrl->isDuringContinuousAutoDimension()) {
            dimension_ctrl->fixContinuousAutoDimension();
        }

        update();
        return true;
    }

    return false;
}

bool MyOpenGLWidget::pickVoxelGL(const Point3f& obj_pos, Point3f& pick_pos)
{
    DEBUG() << "pickVoxelGL";

    /// 暫定 - 一旦ピックして対象Node絞る
    m_scene_render->setPickTargetFilter({ObjectType::Voxel, ObjectType::VoxelScalar});
    const auto         local_mouse_pos = m_scene_view->projectToScreen(obj_pos);
    PickData           pick_data;
    std::vector<Node*> target_nodes;
    m_scene_render->pickGL(Point2i(local_mouse_pos.x(), local_mouse_pos.y()), 5, pick_data, PickSnap::SnapNone,
                           &target_nodes, &obj_pos);
    m_scene_render->resetPickTargetFilter();
    if (pick_data.pickNode()) {
        /// 点補正
        Point3f pick_pos_1 =
            pointOnLine(pick_data.pickPoint(), obj_pos, m_scene_view->cameraDir(pick_data.pickPoint()));
        pick_data.setPickPoint(pick_pos_1);
        if (adjustPickData(pick_data, true, &target_nodes)) {
            pick_pos = pick_data.pickPoint();

            return true;
        }
    }

    return false;
}

bool MyOpenGLWidget::adjustPickData(PickData& pick_data, bool above_pick_point, std::vector<Node*>* pick_nodes)
{
    /// 暫定: Pick補正
    auto pick_node_object = pick_data.pickNodeObject();

    if (pick_node_object && pick_node_object->isVoxel()) {
        /// ピック位置のトリア取得
        std::vector<Picking::SIntersectionInfo> intersections;
        Picking                                 picking(m_scene_view.ptr());
        picking.setOnlyMinimum(true);

        std::vector<Node*> voxels;
        if (pick_nodes) {
            for (auto pick_node : *pick_nodes) {
                auto object = pick_node->object();
                if (object && object->isVoxel()) {
                    voxels.emplace_back(pick_node);
                }
            }
        }
        std::vector<Node*>* target_nodes = voxels.size() > 0 ? &voxels : nullptr;

        picking.bodyLineIntersection(pick_data.pickPoint(), m_scene_view->cameraDir(pick_data.pickPoint()), true,
                                     intersections, m_scene_view->sceneGraph()->lengthUnit().epsilonValidLength(),
                                     above_pick_point, target_nodes);

        if (!intersections.empty()) {
            Point3f new_point = m_scene_view->cameraDir(pick_data.pickPoint()) * intersections[0].m_dist_line_dir
                              + pick_data.pickPoint();

            auto pick_vertex = PickVertex::createObject();
            pick_vertex->setVertex(new_point);
            pick_data.setPickObject(pick_vertex.ptr());
            pick_data.setPickPoint(new_point);
            pick_data.setPickNorm(intersections[0].orgNorm(true));
            return true;
        }
    }
    else if (pick_node_object && !pick_node_object->isVoxel()) {
        return true;
    }
    else {
        /// ピック位置のトリア取得
        std::vector<Picking::SIntersectionInfo> intersections;
        Picking                                 picking(m_scene_view.ptr());
        picking.setOnlyMinimum(true);

        std::vector<Node*> voxels;
        if (pick_nodes) {
            for (auto pick_node : *pick_nodes) {
                auto object = pick_node->object();
                if (object && object->isVoxel()) {
                    voxels.emplace_back(pick_node);
                }
            }
        }
        std::vector<Node*>* target_nodes = voxels.size() > 0 ? &voxels : nullptr;

        picking.bodyLineIntersection(pick_data.pickPoint(), m_scene_view->cameraDir(pick_data.pickPoint()), true,
                                     intersections, m_scene_view->sceneGraph()->lengthUnit().epsilonValidLength(),
                                     above_pick_point, target_nodes);

        if (!intersections.empty()) {
            Point3f new_point = m_scene_view->cameraDir(pick_data.pickPoint()) * intersections[0].m_dist_line_dir
                              + pick_data.pickPoint();

            auto pick_vertex = PickVertex::createObject();
            pick_vertex->setVertex(new_point);
            pick_data.setPickObject(pick_vertex.ptr());
            pick_data.setPickPoint(new_point);
            pick_data.setPickNorm(intersections[0].orgNorm(true));
            return true;
        }
    }

    return false;
}

void MyOpenGLWidget::clearDimensions()
{
    auto root_node = m_scene_view->sceneGraph()->rootNode();

    ///
    std::set<Node*> nodes;
    for (auto& child : root_node->children()) {
        auto object = child->object();
        if (object && object->type() == ObjectType::Dimension) {
            nodes.insert(child.ptr());
        }
    }

    root_node->removeChild(nodes);
    root_node->markBoxDirty();

    auto dimension_ctrl = m_3DForm->dimensionCtrl();
    dimension_ctrl->clearDimensions();

    update();
}

void MyOpenGLWidget::clearAutoDimensions()
{
    auto root_node = m_scene_view->sceneGraph()->rootNode();

    ///
    std::set<Node*> nodes;
    for (auto& child : root_node->children()) {
        auto object = child->object();
        if (object && object->type() == ObjectType::MultiDimension) {
            nodes.insert(child.ptr());
        }
    }

    root_node->removeChild(nodes);
    root_node->markBoxDirty();

    auto dimension_ctrl = m_3DForm->dimensionCtrl();
    dimension_ctrl->clearAutoDimension();

    update();
}

void MyOpenGLWidget::setBackgroundColor(const QColor& color, bool update_view)
{
    if (backgroundColor() != color) {
        m_scene_render->setBackGroundColor(Point4f(color.redF(), color.greenF(), color.blueF(), 1.0f));
        if (update_view) {
            update();
        }
    }
}

QColor MyOpenGLWidget::backgroundColor()
{
    auto color = m_scene_render->backgroundColor();

    QColor qcolor;
    qcolor.setRedF(color[0]);
    qcolor.setGreenF(color[1]);
    qcolor.setBlueF(color[2]);
    return qcolor;
}

void MyOpenGLWidget::setBackGroundColorGradient(bool grad, bool update_view)
{
    if (backGroundColorGradient() != grad) {
        m_scene_render->setBackGroundColorGradient(grad);
        if (update_view) {
            update();
        }
    }
}

bool MyOpenGLWidget::backGroundColorGradient()
{
    return m_scene_render->backGroundColorGradient();
}

void MyOpenGLWidget::addDragFilter(ObjectType type)
{
    m_scene_render->addDragFilter(type);
}

void MyOpenGLWidget::clearDragFilter()
{
    m_scene_render->clearDragFilter();
}

void MyOpenGLWidget::setMaxPickLength(float length)
{
    m_scene_render->setMaxPickLength(length);
}

bool MyOpenGLWidget::create3DTexture(VoxelScalar* voxel_scalar)
{
    return m_scene_render->create3DTexture(voxel_scalar);
}

void MyOpenGLWidget::create2DTexture(VoxelScalar* voxel_scalar)
{
    m_scene_render->create2DTexture(voxel_scalar);
}

void MyOpenGLWidget::set3DTextureColor(const std::vector<float>& division, const std::vector<Point4f>& color)
{
    m_scene_render->set3DTextureColor(division, color);

    /// 暫定 - 作り変える場合（圧縮の場合）
    // if (m_opt_3d_node.isAlive()) {
    //     m_scene_render->create3DTexture((VoxelScalar*)m_opt_3d_node->object());
    // }
}

void MyOpenGLWidget::set3DTextureDivision(const std::vector<float>& division)
{
    m_scene_render->set3DTextureDivision(division);

    /// 暫定 - 作り変える場合（圧縮の場合）
    // if (m_opt_3d_node.isAlive()) {
    //     m_scene_render->create3DTexture((VoxelScalar*)m_opt_3d_node->object());
    // }
}

std::vector<float> MyOpenGLWidget::textureDivision()
{
    return m_scene_render->textureDivision();
}

void MyOpenGLWidget::setColorLabelCount(int label_count)
{
    m_scene_render->setColorLabelCount(label_count);
}

void MyOpenGLWidget::setShowColorBar(bool show)
{
    m_scene_render->setShowColorBar(show);
}

void MyOpenGLWidget::setColorMinMaxLabel(const std::wstring& min_label, const std::wstring& max_label)
{
    m_scene_render->setColorMinMaxLabel(min_label, max_label);
}

void MyOpenGLWidget::createRenderData(RenderableNode* renderable)
{
    m_scene_render->createRenderableData(renderable);
}

void MyOpenGLWidget::createRenderData(Node* node)
{
    m_scene_render->createRenderData(node);
}

void MyOpenGLWidget::setVoxelScalarPriority(bool priority)
{
    m_scene_render->setVoxelScalarPriority(priority);
}

/// 暫定 -
/// Op2が前面に出すと、Vox内部に収まっていてもエッジがはみ出るので、クリッピング面orVox面にのっているときだけ前面に出すようにする
void MyOpenGLWidget::setClippingPlanes(const std::vector<Clipping::PlaneInfo>& clip_planes)
{
    std::vector<Planef> planes;

    /// Vox (clipping_planesだけで基本十分なのだが、領域全体とサイズ違う場合もあり得るので）
    if (m_all_vox_bbox.valid()) {
        planes.emplace_back(Planef(Point3f(1, 0, 0), m_all_vox_bbox.min_pos()));
        planes.emplace_back(Planef(Point3f(0, 1, 0), m_all_vox_bbox.min_pos()));
        planes.emplace_back(Planef(Point3f(0, 0, 1), m_all_vox_bbox.min_pos()));
        planes.emplace_back(Planef(Point3f(1, 0, 0), m_all_vox_bbox.max_pos()));
        planes.emplace_back(Planef(Point3f(0, 1, 0), m_all_vox_bbox.max_pos()));
        planes.emplace_back(Planef(Point3f(0, 0, 1), m_all_vox_bbox.max_pos()));
    }
    for (const auto& clip : clip_planes) {
        planes.emplace_back(clip.m_plane);
    }
    m_scene_render->setClippingPlanes(planes);
}

void MyOpenGLWidget::clearClippingPlanes()
{
    std::vector<Planef> planes;

    /// Vox (clipping_planesだけで基本十分なのだが、領域全体とサイズ違う場合もあり得るので）
    if (m_all_vox_bbox.valid()) {
        planes.emplace_back(Planef(Point3f(1, 0, 0), m_all_vox_bbox.min_pos()));
        planes.emplace_back(Planef(Point3f(0, 1, 0), m_all_vox_bbox.min_pos()));
        planes.emplace_back(Planef(Point3f(0, 0, 1), m_all_vox_bbox.min_pos()));
        planes.emplace_back(Planef(Point3f(1, 0, 0), m_all_vox_bbox.max_pos()));
        planes.emplace_back(Planef(Point3f(0, 1, 0), m_all_vox_bbox.max_pos()));
        planes.emplace_back(Planef(Point3f(0, 0, 1), m_all_vox_bbox.max_pos()));
    }
    m_scene_render->setClippingPlanes(planes);
}

void MyOpenGLWidget::showOpt3D(bool show, bool update_view)
{
    if (m_opt_3d_node != nullptr) {
        if (show)
            m_opt_3d_node->show();
        else
            m_opt_3d_node->hide();

        setViewNearFar();
        if (update_view) {
            update();
        }
    }
}

void MyOpenGLWidget::removeOpt3DNode()
{
    if (m_opt_3d_node != nullptr) {
        auto root_node = m_scene_view->sceneGraph()->rootNode();
        root_node->removeChild(m_opt_3d_node.ptr());
        setOpt3DNode(nullptr);
        root_node->markBoxDirty();
    }
}

void MyOpenGLWidget::setPickMode(bool pick, PickSnap snap)
{
    m_pick_mode = pick;
    m_pick_snap = snap;
    if (m_pick_mode) {
        setMouseTracking(true);
    }
    else {
        setMouseTracking(false);
    }

    if (snap == PickSnap::SnapNone) {
        m_scene_render->resetXORCurrent();
    }

    if (!m_pick_mode) {
        m_scene_render->resetXORCurrent();
        m_scene_render->clearPickObjects();

        auto dimension_ctrl = m_3DForm->dimensionCtrl();
        dimension_ctrl->clearTemporaryDimension();
        dimension_ctrl->clearTemporaryAutoDimension();
        // dimension_ctrl->clearTemporaryPreviewAutoDimension();
        // m_scene_view->temporaryRootNode()->removeAllChild();

        update();
    }
}

void MyOpenGLWidget::setPickColor(QColor color)
{
    m_scene_render->setPickColor(Point4f(color.redF(), color.greenF(), color.blueF(), 1.0f));
    if (!m_scene_render->pickObjects().empty()) {
        update();
    }
}

void MyOpenGLWidget::fitDisplay()
{
    m_scene_view->fitDisplay();
    update();
}

void MyOpenGLWidget::fitDisplay(int material_id)
{
    auto find_id = m_material_node_map.find(material_id);
    if (find_id != m_material_node_map.end()) {
        auto& node = find_id->second;
        if (node.isAlive()) {
            fitDisplay(node.ptr());
        }
    }
}

void MyOpenGLWidget::fitDisplay(Node* node)
{
    if (!m_scene_view->fitDisplay({node}, true)) {
        m_scene_view->fitDisplay({node}, false);
    }
    update();
}

void MyOpenGLWidget::imageCopyToClipboard()
{
    QImage image = grabFramebuffer();

    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setImage(image);
}

void MyOpenGLWidget::imageCopyToFile()
{
    static QString image_selected_filter = tr("PNG Files (*.png)");
    QString        fileName              = QFileDialog::getSaveFileName(this, tr("Save Image"), "",
                                                                        tr("PNG Files (*.png);;JPEG Files (*.jpg);;Bitmap Files (*.bmp)"),
                                                                        &image_selected_filter);

    if (!fileName.isEmpty()) {
        if (!fileName.endsWith(".png", Qt::CaseInsensitive) && !fileName.endsWith(".jpg", Qt::CaseInsensitive)
            && !fileName.endsWith(".bmp", Qt::CaseInsensitive)) {
            fileName += ".png";    /// デフォルトで .png
        }

        QImage image = grabFramebuffer();
        image.save(fileName);
    }
}

/// (result_width, result_height) + 左にcolorbarのサイズで出力
void MyOpenGLWidget::outputResultImage(int result_width, int result_height, bool voxel_scalar_priority,
                                       bool add_colorbar, QImage& image)
{
    /// Nodeの表示非表示、Voxelのwireframe表示・クリッピングなど上位で設定 -> 戻す

    makeCurrent();

    bool coordinate_system          = m_scene_render->isShowCoordinateSystem();
    bool color_bar                  = m_scene_render->isShowColorBar();
    bool save_voxel_scalar_priority = m_scene_render->voxelScalarPriority();
    bool line_smooth                = m_scene_render->isVoxelLineSmooth();
    m_scene_render->showCoordinateSystem(false);
    m_scene_render->setShowColorBar(false);
    // m_scene_render->setVoxelWireframeColor(Point4f(0, 0, 0, 0));
    // m_scene_render->setVoxelWireframeWidth(1.0f / devicePixelRatio());
    // m_scene_render->setVoxelWireframeColorShape(false);
    m_scene_render->setVoxelScalarPriority(voxel_scalar_priority);
    m_scene_render->setVoxelLineSmooth(false);

    int color_bar_area[4];
    if (add_colorbar) {
        calcColorBarArea(color_bar_area);
    }

    int width  = add_colorbar ? (result_width + color_bar_area[2]) : result_width;
    int height = add_colorbar ? std::max(result_height, color_bar_area[3]) : result_height;

    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(0);
    QOpenGLFramebufferObject fbo(width, height, format);
    fbo.bind();

    int  area_except_color_bar[4];
    int* pointer_area = nullptr;
    if (add_colorbar) {
        area_except_color_bar[0] = color_bar_area[2];
        area_except_color_bar[1] = height - result_height;
        area_except_color_bar[2] = result_width;
        area_except_color_bar[3] = result_height;
        pointer_area             = area_except_color_bar;
    }

    bool is_suppress_render = m_scene_render->isSuppressRender();
    if (is_suppress_render) {
        m_scene_render->suppressRender(false);
    }

    /// カラーバーを足す場合、背景がずれるので先に全体背景描画
    if (pointer_area) {
        m_scene_render->paintBackground(0, 0, width, height);
    }

    /// 結果を描画
    m_scene_render->paintGL(pointer_area);

    /// カラーバーを描画
    if (add_colorbar) {
        int viewport[4] = {0, 0, color_bar_area[2], height};
        m_scene_render->paintOnlyColorBar(
            viewport, -color_bar_area[0],
            -color_bar_area[1] + (int)(((float)height - (float)color_bar_area[3]) / (float)2));
    }

    if (is_suppress_render) {
        m_scene_render->suppressRender(true);
    }

    image = fbo.toImage();
    fbo.release();

    doneCurrent();

    m_scene_render->showCoordinateSystem(coordinate_system);
    m_scene_render->setShowColorBar(color_bar);
    // m_scene_render->setVoxelWireframeColor(wireframe_color);
    // m_scene_render->setVoxelWireframeWidth(wireframe_width);
    // m_scene_render->setVoxelWireframeColorShape(wireframe_color_shape);
    m_scene_render->setVoxelScalarPriority(save_voxel_scalar_priority);
    m_scene_render->setVoxelLineSmooth(line_smooth);
}

void MyOpenGLWidget::calcColorBarArea(int color_bar_area[4])
{
    int x_min = 0;
    int y_min = 0;
    int x_max = 0;
    int y_max = 0;

    m_scene_render->sizeDrawColorBar(x_min, y_min, x_max, y_max);
    color_bar_area[0] = x_min;
    color_bar_area[1] = y_min;
    color_bar_area[2] = x_max - x_min + 2;    /// 余白
    color_bar_area[3] = y_max - y_min;
}
