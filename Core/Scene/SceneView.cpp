#include "SceneView.h"
#include "SceneGraph.h"

#include <algorithm>
#undef min
#undef max

CORE_NAMESPACE_BEGIN

RefPtr<SceneView> SceneView::createSceneView(SceneGraph* scene_graph)
{
    return RefPtr<SceneView>(new SceneView(scene_graph));
}

SceneView::SceneView(SceneGraph* scene_graph)
    : m_scene_graph(scene_graph)
    , m_view_port_x(0)
    , m_view_port_y(0)
    , m_view_port_width(600)
    , m_view_port_height(400)
    , m_proj_ortho_size(10.0f)
    , m_proj_near(-50.0f)
    , m_proj_far(50.0f)
    , m_camera_eye(10.0f, -10.0f, 10.0f)    /// 初期位置
    , m_camera_target(0.0f, 0.0f, 0.0f)     /// 初期ターゲット
    , m_camera_up(0.0f, 0.0f, 1.0f)         /// 初期上方向
    , m_rotation_center(0.0f, 0.0f, 0.0f)
    , m_zaxis_rotate_mode(false)
    , m_proj_fov(45.0f * (M_PI / 180.0f))
    , m_proj_type(ProjectionType::Ortho)
{
    setLightDir(Point3f(1.0f, 1.0f, -1.0f));
    updateProjectionMatrix();
    updateViewMatrix();

    m_temporary_root_node = Node::createNode();
    m_temporary_root_node->setName(L"Temporary Root");
}

SceneView::~SceneView() {}

SceneGraph* SceneView::sceneGraph()
{
    return m_scene_graph;
}

void SceneView::setProjectionType(ProjectionType type)
{
    if (m_proj_type == type) {
        return;
    }
    m_proj_type = type;
    fitDisplay();
}

void SceneView::setViewPort(int x, int y, int width, int height)
{
    m_view_port_x      = x;
    m_view_port_y      = y;
    m_view_port_width  = width;
    m_view_port_height = height;

    updateProjectionMatrix();
}

void SceneView::viewPort(int& x, int& y, int& width, int& height)
{
    x      = m_view_port_x;
    y      = m_view_port_y;
    width  = m_view_port_width;
    height = m_view_port_height;
}

void SceneView::setupFrustum()
{
    m_frustum.setupFrustum(mvpMatrix());
}

void SceneView::setupFrustum(const Matrix4x4f& projection_matrix)
{
    m_frustum.setupFrustum(projection_matrix * m_view_matrix * m_model_matrix);
}

bool SceneView::isBoxInFrustum(const BoundingBox3f& bbox) const
{
    return m_frustum.isBoxInFrustum(bbox);
}

bool SceneView::isBoxInFrustum(const BoundingBox3f& bbox, const Matrix4x4f& path_matrix) const
{
    if (path_matrix.isIdentity()) {
        return m_frustum.isBoxInFrustum(bbox);
    }
    else {
        return m_frustum.isBoxInFrustum(path_matrix * bbox);
    }
}

bool SceneView::isPointInFrustum(const Point3f& point, const Matrix4x4f& path_matrix) const
{
    if (path_matrix.isIdentity()) {
        return m_frustum.isPointInFrustum(point);
    }
    else {
        return m_frustum.isPointInFrustum(path_matrix * point);
    }
}

RefPtr<SceneView> SceneView::saveCameraState() const
{
    auto scene_view = createSceneView(nullptr);

    scene_view->m_projection_matrix = m_projection_matrix;
    scene_view->m_view_matrix       = m_view_matrix;
    scene_view->m_model_matrix      = m_model_matrix;

    scene_view->m_frustum = m_frustum;

    /// View port
    scene_view->m_view_port_x      = m_view_port_x;
    scene_view->m_view_port_y      = m_view_port_y;
    scene_view->m_view_port_width  = m_view_port_width;
    scene_view->m_view_port_height = m_view_port_height;

    /// Projection
    scene_view->m_proj_ortho_size = m_proj_ortho_size;
    scene_view->m_proj_near       = m_proj_near;
    scene_view->m_proj_far        = m_proj_far;

    /// 視野角（Field of View）
    scene_view->m_proj_fov = m_proj_fov;

    scene_view->m_proj_type = m_proj_type;

    /// Camere(view_matrix)
    scene_view->m_camera_eye    = m_camera_eye;
    scene_view->m_camera_target = m_camera_target;
    scene_view->m_camera_up     = m_camera_up;

    scene_view->m_rotation_center = m_rotation_center;

    scene_view->m_light_dir = m_light_dir;

    scene_view->m_zaxis_rotate_mode = m_zaxis_rotate_mode;

    return scene_view;
}

void SceneView::loadCameraState(SceneView* scene_view)
{
    m_projection_matrix = scene_view->m_projection_matrix;
    m_view_matrix       = scene_view->m_view_matrix;
    m_model_matrix      = scene_view->m_model_matrix;

    m_frustum = scene_view->m_frustum;

    /// View port
    m_view_port_x      = scene_view->m_view_port_x;
    m_view_port_y      = scene_view->m_view_port_y;
    m_view_port_width  = scene_view->m_view_port_width;
    m_view_port_height = scene_view->m_view_port_height;

    /// Projection
    m_proj_ortho_size = scene_view->m_proj_ortho_size;
    m_proj_near       = scene_view->m_proj_near;
    m_proj_far        = scene_view->m_proj_far;

    /// 視野角（Field of View）
    m_proj_fov = scene_view->m_proj_fov;

    m_proj_type = scene_view->m_proj_type;

    /// Camere(view_matrix)
    m_camera_eye    = scene_view->m_camera_eye;
    m_camera_target = scene_view->m_camera_target;
    m_camera_up     = scene_view->m_camera_up;

    m_rotation_center = scene_view->m_rotation_center;

    m_light_dir = scene_view->m_light_dir;

    m_zaxis_rotate_mode = scene_view->m_zaxis_rotate_mode;
}

void SceneView::setRotationCenter(const Point3f& center, bool set_target)
{
    m_rotation_center = center;

    if (set_target) {
        // カメラの位置を調整
        auto camera_vec = cameraVec();
        m_camera_target = m_rotation_center;
        m_camera_eye    = m_camera_target - camera_vec;
    }

    setViewNearFar();
    updateViewMatrix();
}

Point3f SceneView::viewCenter()
{
    return Point3f();
}

bool SceneView::setZAxisRotateMode(bool mode)
{
    m_zaxis_rotate_mode = mode;

    if (m_zaxis_rotate_mode) {
        auto camera_vec   = cameraVec();
        auto camera_right = (camera_vec ^ m_camera_up).normalized();

        if (camera_right.z() != 0.0) {
            if (m_camera_up.z() >= 0.0) {
                m_camera_up = Point3f(0, 0, 1);
            }
            else {
                m_camera_up = Point3f(0, 0, -1);
            }

            camera_vec   = cameraVec();
            camera_right = (camera_vec ^ m_camera_up).normalized();

            /// カメラの「上方向」を再計算して安定化
            m_camera_up = (camera_right ^ camera_vec).normalized();

            updateViewMatrix();
            return true;
        }
    }

    return false;
}

bool SceneView::isZAxisRotateMode() const
{
    return m_zaxis_rotate_mode;
}

void SceneView::setLightDir(const Point3f& light_dir)
{
    m_light_dir = light_dir;
    m_light_dir.normalize();
}

const Point3f& SceneView::lightDir() const
{
    return m_light_dir;
}

bool SceneView::isEqaulLightDir(const Point3f& light_dir) const
{
    auto nomlized_dir = light_dir.normalized();

    if ((m_light_dir - nomlized_dir).length2() <= 1.0e-10) {
        return true;
    }
    else {
        return false;
    }
}

bool SceneView::fitDisplayViewBox(const BoundingBox3f& view_box, float fit_ratio)
{
    if (!view_box.valid()) {
        return false;
    }

    const auto min_pos = view_box.min_pos();
    const auto max_pos = view_box.max_pos();

    Point3f center    = (min_pos + max_pos) * 0.5f;
    Point3f view_size = max_pos - min_pos;
    if (view_size.x() == 0 || view_size.y() == 0) {
        return false;
    }

    center = invMvMatrix() * center;

    if (m_proj_type == ProjectionType::Ortho) {
        /// カメラの位置
        auto camera_vec = cameraVec();
        m_camera_target = center;
        m_camera_eye    = m_camera_target - camera_vec;

        /// 回転中心
        m_rotation_center = center;

        /// 投影
        float aspect_ratio = float(m_view_port_width) / m_view_port_height;
        float maxSize      = std::max(view_size.x() / aspect_ratio, view_size.y()) / 2.0;
        m_proj_ortho_size  = maxSize * fit_ratio;
    }
    else if (m_proj_type == ProjectionType::Perspective) {
        m_proj_fov = 45.0f * (M_PI / 180.0f);

        float aspect_ratio = float(m_view_port_width) / m_view_port_height;

        // 縦方向の距離
        float distY = (view_size.y() * 0.5f * fit_ratio) / std::tan(m_proj_fov * 0.5f);

        // 横方向のfovXを計算
        float fovX = 2.0f * std::atan(std::tan(m_proj_fov * 0.5f) * aspect_ratio);
        // 横方向の距離
        float distX = (view_size.x() * 0.5f * fit_ratio) / std::tan(fovX * 0.5f);

        // 必要な距離は「大きい方」
        float dist = std::max(distY, distX);

        float camera_dist = (dist + fabs((max_pos.z() - min_pos.z()) / 2.0)) + 0.1f;

        // カメラの向きに応じてeye位置を調整
        auto camera_dir = cameraDir();
        m_camera_target = center;
        m_camera_eye    = m_camera_target - camera_dir * camera_dist;

        /// 回転中心
        m_rotation_center = center;

        // near/far設定
        // float radius = std::max(view_size.x(), view_size.y()) * 0.5f;
        m_proj_near = 0.1f;                           // オブジェクトの手前
        m_proj_far  = camera_dist + view_size.z();    // オブジェクトの奥
    }

    /// Near/Farを調整して再度カメラ/投影調整
    setViewNearFar();

    updateViewMatrix();

    return true;
}

bool SceneView::fitDisplayObjectBox(const BoundingBox3f& obj_box, float fit_ratio)
{
    return fitDisplayViewBox(mvMatrix() * obj_box, fit_ratio);
}

bool SceneView::fitDisplay(float fit_ratio)
{
    auto scene_graph = sceneGraph();
    auto root_node   = scene_graph->rootNode();

    auto bbox = root_node->calculateBoundingBox(mvMatrix());

    return fitDisplayViewBox(bbox, fit_ratio);
}

bool SceneView::fitDisplay(VecNode& node_list, bool only_visible)
{
    BoundingBox3f bbox;
    const auto&   mv_matrix = mvMatrix();
    for (auto& node : node_list) {
        if (only_visible) {
            if (!node->isParentVisible()) {
                continue;
            }
        }
        Matrix4x4f cur_mv_matrix = mv_matrix * node->parentPathMatrix();
        auto       cur_bbox      = node->calculateBoundingBox(cur_mv_matrix, only_visible);
        bbox.expandBy(cur_bbox);
    }

    return fitDisplayViewBox(bbox);
}

void SceneView::setViewNearFar(bool perspective_eye_adjust)
{
    auto        scene_graph = sceneGraph();
    auto        root_node   = scene_graph->rootNode();
    const auto& bbox        = root_node->boundingBox();

    if (!bbox.valid()) {
        return;
    }

    Point3f obj_size     = bbox.max_pos() - bbox.min_pos();
    float   max_obj_size = obj_size.length();
    if (max_obj_size == 0.0f) {
        max_obj_size = 100.0f;
    }

    if (m_proj_type == ProjectionType::Ortho) {
        float margin = 1.05f;

        float nearfar_size = (max_obj_size * margin);

        /// カメラの位置
        /// 注視点をBox中心位置に合わせる。
        auto camera_dir = cameraDir();
        auto box_center = bbox.center();
        m_camera_target = m_camera_eye + camera_dir * ((box_center - m_camera_eye) * camera_dir);
        m_camera_eye    = m_camera_target - camera_dir * nearfar_size;

        /// Near / Far
        m_proj_near = -nearfar_size;
        m_proj_far  = nearfar_size * 2.0f;
    }
    else if (m_proj_type == ProjectionType::Perspective) {
        /// 視点が埋まっていたら外に出す
        float max_radius = 0.0f;
        for (int ic = 0; ic < 8; ++ic) {
            const auto& corner = bbox.corner(ic);
            float       r      = (corner - m_camera_target).length();
            if (r > max_radius) max_radius = r;
        }
        float margin = 1.15f;
        float dist   = max_radius * margin;    // / std::tan(m_proj_fov * 0.5f);

        /// 透視投影は見た目が変わるので変えたくないとき（埋まってもいい時）はフラグで制御
        float dist0 = (m_camera_eye - m_camera_target).length();
        if (perspective_eye_adjust && dist0 < dist) {
            DEBUG() << "Projection fov adjust";
            m_proj_fov   = 2.0 * atan(tan(m_proj_fov / 2.0) * (dist0 / dist));
            m_camera_eye = m_camera_target - cameraDir() * dist;
            dist0        = dist;
        }

        m_proj_near = std::max(0.1f, max_obj_size * 0.1f);    // 0.1以上
        m_proj_far  = dist0 + max_obj_size * margin;          // 余裕を持たせる
    }

    updateViewMatrix();

    updateProjectionMatrix();
}

void SceneView::initView()
{
    viewDirection(6);

    fitDisplay();
}

void SceneView::frontView()
{
    viewDirection(0);
}

void SceneView::backView()
{
    viewDirection(1);
}

void SceneView::leftView()
{
    viewDirection(2);
}

void SceneView::rightView()
{
    viewDirection(3);
}

void SceneView::topView()
{
    viewDirection(4);
}

void SceneView::bottomView()
{
    viewDirection(5);
}

void SceneView::axoView()
{
    viewDirection(6);
}

void SceneView::viewDirection(int direction)
{
    auto camera_vec = cameraVec();
    m_camera_target = m_rotation_center;

    Point3f eye_dir, up_dir;
    switch (direction) {
        case 0:    /// front
            eye_dir = Point3f(0.0f, -1.0f, 0.0f);
            up_dir  = Point3f(0.0f, 0.0f, 1.0f);
            break;
        case 1:    /// back
            eye_dir = Point3f(0.0f, 1.0f, 0.0f);
            up_dir  = Point3f(0.0f, 0.0f, 1.0f);
            break;
        case 2:    /// left
            eye_dir = Point3f(1.0f, 0.0f, 0.0f);
            up_dir  = Point3f(0.0f, 0.0f, 1.0f);
            break;
        case 3:    /// right
            eye_dir = Point3f(-1.0f, 0.0f, 0.0f);
            up_dir  = Point3f(0.0f, 0.0f, 1.0f);
            break;
        case 4:    /// top
            eye_dir = Point3f(0.0f, 0.0f, 1.0f);
            up_dir  = Point3f(0.0f, 1.0f, 0.0f);
            break;
        case 5:    /// bottom
            eye_dir = Point3f(0.0f, 0.0f, -1.0f);
            up_dir  = Point3f(0.0f, 1.0f, 0.0f);
            break;
        case 6:    /// Axo
            eye_dir = Point3f(1.0f, -1.0f, 1.0f);
            up_dir  = Point3f(0.0f, 0.0f, 1.0f);
            break;
    }

    m_camera_eye = eye_dir * camera_vec.length() + m_camera_target;
    m_camera_up  = up_dir;

    /// カメラの「右方向」を再計算
    camera_vec        = m_camera_eye - m_camera_target;
    auto camera_right = (camera_vec ^ m_camera_up).normalized();

    /// カメラの「上方向」を再計算して安定化
    m_camera_up = (camera_right ^ camera_vec).normalized();

    updateViewMatrix();
    setViewNearFar();
}

bool SceneView::dirView(const Point3f& dir)
{
    if (dir.length() == 0.0f) {
        return false;
    }

    /// 基本Z上？と思われるので
    // auto org_camera_up = m_camera_up;

    m_camera_target = m_rotation_center;

    Point3f eye_dir = -dir.normalized();
    Point3f up_dir  = (std::abs(eye_dir.x()) > std::abs(eye_dir.y()))
                        ? Point3f(-eye_dir.z(), 0, eye_dir.x()).normalized()
                        : Point3f(0, -eye_dir.z(), eye_dir.y()).normalized();

    auto camera_vec = cameraVec();
    m_camera_eye    = eye_dir * camera_vec.length() + m_camera_target;
    m_camera_up     = up_dir;

    /// カメラの「右方向」を再計算
    camera_vec        = m_camera_eye - m_camera_target;
    auto camera_right = (camera_vec ^ m_camera_up).normalized();

    /// カメラの「上方向」を再計算して安定化
    m_camera_up = (camera_right ^ camera_vec).normalized();

    /// Z軸固定ビューでも問題ないようにZ軸調整する
    camera_vec   = cameraVec();
    camera_right = (camera_vec ^ m_camera_up).normalized();
    if (camera_right.z() != 0.0) {
        // if (org_camera_up.z() >= 0.0f) {
        m_camera_up = Point3f(0, 0, 1);
        //}
        // else {
        //    m_camera_up = Point3f(0, 0, -1);
        //}

        camera_vec   = cameraVec();
        camera_right = (camera_vec ^ m_camera_up).normalized();

        /// カメラの「上方向」を再計算して安定化
        m_camera_up = (camera_right ^ camera_vec).normalized();
    }
    else {
        if (m_camera_up.z() < 0) {
            m_camera_up *= -1.0f;
        }
    }

    updateViewMatrix();
    setViewNearFar();
    return true;
}

void SceneView::rotateLeft()
{
    if (m_zaxis_rotate_mode) {
        if (m_camera_up.z() > 0.0) {
            rotateDegree(Point3f(0, 0, 1), 15.0);
        }
        else if (m_camera_up.z() == 0.0) {
            if (cameraVec().z() >= 0) {
                rotateDegree(Point3f(0, 0, -1), 15.0);
            }
            else {
                rotateDegree(Point3f(0, 0, 1), 15.0);
            }
        }
        else {
            rotateDegree(Point3f(0, 0, -1), 15.0);
        }
    }
    else {
        rotateDegree(m_camera_up, 15.0);
    }
}

void SceneView::rotateRight()
{
    if (m_zaxis_rotate_mode) {
        if (m_camera_up.z() > 0.0) {
            rotateDegree(Point3f(0, 0, 1), -15.0);
        }
        else if (m_camera_up.z() == 0.0) {
            if (cameraVec().z() >= 0) {
                rotateDegree(Point3f(0, 0, -1), -15.0);
            }
            else {
                rotateDegree(Point3f(0, 0, 1), -15.0);
            }
        }
        else {
            rotateDegree(Point3f(0, 0, -1), -15.0);
        }
    }
    else {
        rotateDegree(m_camera_up, -15.0);
    }
}

void SceneView::rotateUp()
{
    auto camera_vec   = cameraVec();
    auto camera_right = (camera_vec ^ m_camera_up).normalized();

    rotateDegree(camera_right, 15.0);
}

void SceneView::rotateDown()
{
    auto camera_vec   = cameraVec();
    auto camera_right = (camera_vec ^ m_camera_up).normalized();

    rotateDegree(camera_right, -15.0);
}

void SceneView::rotateDegree(const Point3f& axis, float degree)
{
    /// 回転中心のビュー上の位置
    auto screen_center = projectToScreen(m_rotation_center);

    /// 回転行列を作成
    Matrix4x4f rotationMatrix;
    rotationMatrix.rotateDegree(degree, axis);

    /// カメラの位置 (eye) を回転
    m_camera_eye = rotationMatrix * m_camera_eye;

    /// ターゲットの位置 (target) を回転
    m_camera_target = rotationMatrix * m_camera_target;

    /// Upも回転
    m_camera_up = rotationMatrix * m_camera_up;

    /// カメラの「右方向」を再計算
    auto camera_vec   = cameraVec();
    auto camera_right = (camera_vec ^ m_camera_up).normalized();

    /// カメラの「上方向」を再計算して安定化
    m_camera_up = (camera_right ^ camera_vec).normalized();

    updateViewMatrix();

    /// 現在の回転中心
    auto cur_rotation_center = unprojectFromScreen(screen_center);

    /// 回転中心位置がずれるので戻す
    auto center_vec = cur_rotation_center - m_rotation_center;

    m_camera_eye -= center_vec;
    m_camera_target -= center_vec;
    updateViewMatrix();

    setViewNearFar();
}

void SceneView::rotateMouse(int mouse_move_x, int mouse_move_y)
{
    /// 角度制限(90度超えると動作おかしくなるので）
    float max_angle = 89.0f;     /// 最大回転角度（度）
    float min_angle = -89.0f;    /// 最小回転角度（度）
    float angle_x   = std::clamp(-mouse_move_x * 0.5f, min_angle, max_angle);
    float angle_y   = std::clamp(-mouse_move_y * 0.5f, min_angle, max_angle);

    auto camera_vec   = cameraVec();
    auto camera_right = (camera_vec ^ m_camera_up).normalized();

    /// 回転中心のビュー上の位置
    auto screen_center = projectToScreen(m_rotation_center);

    /// 回転行列を作成
    Matrix4x4f rotation_matrix;
    if (m_zaxis_rotate_mode) {
        /*
        if (m_camera_up.z() > 0.0) {
            rotation_matrix.rotateDegree(angle_x, Point3f(0, 0, 1));
        }
        else if (m_camera_up.z() == 0.0) {
            if (cameraVec().z() >= 0) {
                rotation_matrix.rotateDegree(angle_x, Point3f(0, 0, -1));
            }
            else {
                rotation_matrix.rotateDegree(angle_x, Point3f(0, 0, 1));
            }
        }
        else {
            rotation_matrix.rotateDegree(angle_x, Point3f(0, 0, -1));
        }
        */
        rotation_matrix.rotateDegree(angle_x, Point3f(0, 0, 1));
    }
    else {
        rotation_matrix.rotateDegree(angle_x, m_camera_up);
    }
    rotation_matrix.rotateDegree(angle_y, camera_right);

    /// カメラの位置 (eye) を回転
    m_camera_eye = rotation_matrix * m_camera_eye;

    /// ターゲットの位置 (target) を回転
    m_camera_target = rotation_matrix * m_camera_target;

    m_camera_up = rotation_matrix * m_camera_up;

    /// カメラの「右方向」を再計算
    camera_vec   = cameraVec();
    camera_right = (camera_vec ^ m_camera_up).normalized();

    // カメラの「上方向」を再計算して安定化
    m_camera_up = (camera_right ^ camera_vec).normalized();

    updateViewMatrix();

    /// 現在の回転中心
    auto cur_rotation_center = unprojectFromScreen(screen_center);

    /// 回転中心位置がずれるので戻す
    auto center_vec = cur_rotation_center - m_rotation_center;

    m_camera_eye -= center_vec;
    m_camera_target -= center_vec;
    updateViewMatrix();

    setViewNearFar();
}

Point3f SceneView::objectMove(int mouse_move_x, int mouse_move_y)
{
    if (m_proj_type == ProjectionType::Ortho) {
        return unprojectFromScreen(Point3f(mouse_move_x, mouse_move_y, 0.5f))
             - unprojectFromScreen(Point3f(0, 0, 0.5f));
    }
    else if (m_proj_type == ProjectionType::Perspective) {
        float camera_dist = (m_camera_eye - m_camera_target).length();
        float move_scale  = 2.0f * camera_dist * std::tan(m_proj_fov * 0.5f) / m_view_port_height;

        Point3f right = cameraRight();
        Point3f up    = cameraUp();

        return right * (-mouse_move_x) * move_scale + up * (-mouse_move_y) * move_scale;
    }
    else {
        assert(false);
        return Point3f();
    }
}

Point3f SceneView::objectMove(int mouse_move_x, int mouse_move_y, const Point3f& obj_pos)
{
    if (m_proj_type == ProjectionType::Ortho) {
        return unprojectFromScreen(Point3f(mouse_move_x, mouse_move_y, 0.5f))
             - unprojectFromScreen(Point3f(0, 0, 0.5f));
    }
    else if (m_proj_type == ProjectionType::Perspective) {
        float camera_dist = (m_camera_eye - obj_pos).length();
        float move_scale  = 2.0f * camera_dist * std::tan(m_proj_fov * 0.5f) / m_view_port_height;

        Point3f right = cameraRight();
        Point3f up    = cameraUp();

        return right * (-mouse_move_x) * move_scale + up * (-mouse_move_y) * move_scale;
    }
    else {
        assert(false);
        return Point3f();
    }
}

void SceneView::translateMouse(int mouse_move_x, int mouse_move_y)
{
    Point3f object_move = objectMove(mouse_move_x, mouse_move_y);
    translateObject(object_move);
}

void SceneView::translateObject(const Point3f& object_move)
{
    m_camera_eye -= object_move;
    m_camera_target -= object_move;

    updateViewMatrix();
}

void SceneView::nearFarPos(int mouse_pos_x, int mouse_pos_y, Point3f& near_pos, Point3f& far_pos) const
{
    near_pos = unprojectFromScreen(Point3f(mouse_pos_x, mouse_pos_y, 0.0f));
    far_pos  = unprojectFromScreen(Point3f(mouse_pos_x, mouse_pos_y, 1.0f));
}

Point3f SceneView::cameraDir(const Point3f& point) const
{
    if (m_proj_type == ProjectionType::Ortho) {
        return cameraDir();
    }

    auto screen_pt = projectToScreen(point);

    auto near_pt = unprojectFromScreen(Point3f(screen_pt.x(), screen_pt.y(), 0.0f));
    auto far_pt  = unprojectFromScreen(Point3f(screen_pt.x(), screen_pt.y(), 1.0f));

    return (far_pt - near_pt).normalized();
}

Point3f SceneView::cameraUp(const Point3f& point) const
{
    if (m_proj_type == ProjectionType::Ortho) {
        return cameraUp();
    }

    auto screen_pt = projectToScreen(point);

    auto screen_pt_up = unprojectFromScreen(Point3f(screen_pt.x(), screen_pt.y() + 1, screen_pt.z()));

    return (screen_pt_up - screen_pt).normalized();
}

void SceneView::scaleMouse(int mouse_pos_x, int mouse_pos_y, int mouse_delta)
{
    if (m_proj_type == ProjectionType::Ortho) {
        auto org_object_pos = unprojectFromScreen(Point3f(mouse_pos_x, mouse_pos_y, 0.5f));

        scaleMouse(mouse_delta);

        auto new_object_pos = unprojectFromScreen(Point3f(mouse_pos_x, mouse_pos_y, 0.5f));

        translateObject(new_object_pos - org_object_pos);
    }
    else if (m_proj_type == ProjectionType::Perspective) {
        scaleMouse(mouse_delta);
    }
}

void SceneView::scaleMouse(int mouse_delta)
{
    float scale_factor = (mouse_delta > 0) ? 1.1f : 0.9f;
    scale(scale_factor);
}

void SceneView::scale(float scale_factor)
{
    if (m_proj_type == ProjectionType::Ortho) {
        m_proj_ortho_size *= scale_factor;
    }
    else if (m_proj_type == ProjectionType::Perspective) {
        m_proj_fov *= scale_factor;
        m_proj_fov = std::clamp(m_proj_fov, (float)(0.01f * M_PI / 180.0f),
                                (float)(179.0f * M_PI / 180.0f));    // 5度～120度
    }

    updateProjectionMatrix();
}

Matrix4x4f SceneView::mMatrix() const
{
    return m_model_matrix;
}

Matrix4x4f SceneView::mvMatrix() const
{
    return m_view_matrix * m_model_matrix;
}

Matrix4x4f SceneView::invMvMatrix() const
{
    return (m_view_matrix * m_model_matrix).inverted();
}

Matrix4x4f SceneView::vpMatrix() const
{
    return m_projection_matrix * m_view_matrix;
}

Matrix4x4f SceneView::mvpMatrix() const
{
    return m_projection_matrix * m_view_matrix * m_model_matrix;
}

Matrix4x4f SceneView::invMvpMatrix() const
{
    return (m_projection_matrix * m_view_matrix * m_model_matrix).inverted();
}

Point3f SceneView::projectToScreen(const Point3f& object_pos) const
{
    return ndcToScreen(projectToNdc(object_pos));
}

Point3f SceneView::unprojectFromScreen(const Point3f& screen_pos) const
{
    return unprojectFromNdc(screenToNdc(screen_pos));
}

Point3f SceneView::projectToNdc(const Point3f& object_pos) const
{
    const auto& mvp_matrix = mvpMatrix();
    return mvp_matrix * object_pos;
}

Point3f SceneView::unprojectFromNdc(const Point3f& ndc_pos) const
{
    const auto& inv_mvp_matrix = invMvpMatrix();
    return inv_mvp_matrix * ndc_pos;
}

Point3f SceneView::screenToNdc(const Point3f& screen_pos) const
{
    return Point3f(((screen_pos.x() - m_view_port_x) / m_view_port_width) * 2.0f - 1.0f,     /// X座標(-1.0～1.0)
                   1.0f - ((screen_pos.y() - m_view_port_y) / m_view_port_height) * 2.0f,    /// Y座標(-1.0～1.0)
                   2.0f * screen_pos.z() - 1.0);                                             /// Z座標(-1.0～1.0)
}

Point3f SceneView::ndcToScreen(const Point3f& ndc_pos) const
{
    return Point3f((ndc_pos.x() + 1.0f) * m_view_port_width / 2.0f + m_view_port_x,     /// X座標(x～x+width)
                   (1.0f - ndc_pos.y()) * m_view_port_height / 2.0f + m_view_port_y,    /// X座標(y～y+height)
                   (ndc_pos.z() + 1.0f) / 2.0f);                                        /// Z座標(0.0～1.0)
}

float SceneView::projNear() const
{
    return m_proj_near;
}

float SceneView::projFar() const
{
    return m_proj_far;
}

float SceneView::projOrthoSize() const
{
    return m_proj_ortho_size;
}

float SceneView::projFov() const
{
    return m_proj_fov;
}

const Matrix4x4f& SceneView::projectionMatrix() const
{
    return m_projection_matrix;
}

void SceneView::setProjectionMatrix(const Matrix4x4f& projection_matrix)
{
    m_projection_matrix = projection_matrix;
}

void SceneView::updateProjectionMatrix()
{
    float aspect_ratio = float(m_view_port_width) / m_view_port_height;

    if (m_proj_type == ProjectionType::Ortho) {
        m_projection_matrix.ortho(-m_proj_ortho_size * aspect_ratio, m_proj_ortho_size * aspect_ratio,
                                  -m_proj_ortho_size, m_proj_ortho_size, m_proj_near, m_proj_far);
    }
    else if (m_proj_type == ProjectionType::Perspective) {
        m_projection_matrix.perspective(m_proj_fov, aspect_ratio, m_proj_near, m_proj_far);
    }
}

void SceneView::updateViewMatrix()
{
    m_view_matrix.lookAt(m_camera_eye, m_camera_target, m_camera_up);
}

CORE_NAMESPACE_END
