#ifndef CORE_SCENEVIEW_H
#define CORE_SCENEVIEW_H

#include <memory>
#include <vector>
#include "CoreGlobal.h"
#include "Math/Frustum.h"
#include "Math/Matrix4x4f.h"
#include "Memory/RefPtr.h"
#include "Memory/Referenced.h"
#include "Memory/WeakRefPtr.h"
#include "Scene/Node.h"
#include "Scene/Object.h"

CORE_NAMESPACE_BEGIN

class SceneGraph;

class CORE_EXPORT SceneView : public Referenced {
public:
    enum class ProjectionType { Ortho = 0, Perspective };

    static RefPtr<SceneView> createSceneView(SceneGraph* scene_graph);

    SceneGraph* sceneGraph();

    ProjectionType projectionType() const { return m_proj_type; }
    void           setProjectionType(ProjectionType type);
    bool           isPerspective() const { return m_proj_type == ProjectionType::Perspective; }

    Node* temporaryRootNode() { return m_temporary_root_node.ptr(); }

    void setRotationCenter(const Point3f& center, bool set_target);

    Point3f viewCenter();

    bool setZAxisRotateMode(bool mode);
    bool isZAxisRotateMode() const;

    void           setLightDir(const Point3f& light_dir);
    bool           isEqaulLightDir(const Point3f& light_dir) const;
    const Point3f& lightDir() const;

    bool fitDisplayViewBox(const BoundingBox3f& view_box, float fit_ratio = 1.05f);
    bool fitDisplayObjectBox(const BoundingBox3f& obj_box, float fit_ratio = 1.05f);
    bool fitDisplay(float fit_ratio = 1.05f);
    bool fitDisplay(VecNode& node_list, bool only_visible);

    void setViewNearFar(bool perspective_eye_adjust = true);

    void initView();
    void frontView();
    void backView();
    void leftView();
    void rightView();
    void topView();
    void bottomView();
    void axoView();
    void viewDirection(int direction);
    bool dirView(const Point3f& dir);

    void rotateLeft();
    void rotateRight();
    void rotateUp();
    void rotateDown();
    void rotateDegree(const Point3f& axis, float degree);
    void rotateMouse(int mouse_move_x, int mouse_move_y);

    Point3f objectMove(int mouse_move_x, int mouse_move_y);
    Point3f objectMove(int mouse_move_x, int mouse_move_y, const Point3f& obj_pos);
    void    translateMouse(int mouse_move_x, int mouse_move_y);
    void    translateObject(const Point3f& object_move);

    void    nearFarPos(int mouse_pos_x, int mouse_pos_y, Point3f& near_pos, Point3f& far_pos) const;
    Point3f cameraDir(const Point3f& point) const;
    Point3f cameraUp(const Point3f& point) const;

    void scaleMouse(int mouse_pos_x, int mouse_pos_y, int mouse_delta);
    void scaleMouse(int mouse_delta);
    void scale(float scale_factor);

    /// 投影
    Matrix4x4f mMatrix() const;
    Matrix4x4f mvMatrix() const;
    Matrix4x4f invMvMatrix() const;
    Matrix4x4f vpMatrix() const;
    Matrix4x4f mvpMatrix() const;
    Matrix4x4f invMvpMatrix() const;
    Point3f    projectToScreen(const Point3f& object_pos) const;
    Point3f    unprojectFromScreen(const Point3f& screen_pos) const;
    Point3f    projectToNdc(const Point3f& object_pos) const;
    Point3f    unprojectFromNdc(const Point3f& ndc_pos) const;
    Point3f    screenToNdc(const Point3f& screen_pos) const;
    Point3f    ndcToScreen(const Point3f& ndc_pos) const;

    float             projNear() const;
    float             projFar() const;
    float             projOrthoSize() const;
    float             projFov() const;
    const Matrix4x4f& projectionMatrix() const;
    void              setProjectionMatrix(const Matrix4x4f& projection_matrix);

    /// Matrix
    void updateProjectionMatrix();
    void updateViewMatrix();

    void viewPort(int& x, int& y, int& width, int& height);
    void setViewPort(int x, int y, int width, int height);
    int  viewPortWidth() const { return m_view_port_width; }
    int  viewPortHeight() const { return m_view_port_height; }

    void setupFrustum();
    void setupFrustum(const Matrix4x4f& projection_matrix);

    const Frustum& frustum() const { return m_frustum; }

    bool isBoxInFrustum(const BoundingBox3f& bbox) const;
    bool isBoxInFrustum(const BoundingBox3f& bbox, const Matrix4x4f& path_matrix) const;
    bool isPointInFrustum(const Point3f& point, const Matrix4x4f& path_matrix) const;

    const Point3f& cameraEye() const { return m_camera_eye; }
    const Point3f& cameraTarget() const { return m_camera_target; }
    const Point3f& cameraUp() const { return m_camera_up; }
    const Point3f  cameraVec() const { return (m_camera_target - m_camera_eye); }
    const Point3f  cameraDir() const { return cameraVec().normalized(); }
    const Point3f  cameraRight() const { return (cameraUp() ^ cameraVec()).normalized(); }

    RefPtr<SceneView> saveCameraState() const;
    void              loadCameraState(SceneView* scene_view);

protected:
    SceneView(SceneGraph* scene_graph);
    ~SceneView();

protected:
    SceneGraph* m_scene_graph;

    RefPtr<Node> m_temporary_root_node;

    Matrix4x4f m_projection_matrix;
    Matrix4x4f m_view_matrix;
    Matrix4x4f m_model_matrix;

    Frustum m_frustum;

    /// View port
    int m_view_port_x;
    int m_view_port_y;
    int m_view_port_width;
    int m_view_port_height;

    /// Projection
    float m_proj_ortho_size;
    float m_proj_near;
    float m_proj_far;

    /// 視野角（Field of View）
    float m_proj_fov;

    ProjectionType m_proj_type;

    /// Camere(view_matrix)
    Point3f m_camera_eye;
    Point3f m_camera_target;
    Point3f m_camera_up;

    Point3f m_rotation_center;

    Point3f m_light_dir;

    bool m_zaxis_rotate_mode;
};

typedef std::vector<RefPtr<SceneView>> VecSceneView;

CORE_NAMESPACE_END

#endif    // CORE_SCENEVIEW_H
