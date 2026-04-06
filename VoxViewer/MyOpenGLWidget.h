#ifndef MYOPENGLWIDGET_H
#define MYOPENGLWIDGET_H

#include <QMatrix4x4>
#include <QMouseEvent>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QVector3D>
#include <QWheelEvent>
#include <map>
#include <set>
#include <vector>

#include "Vox3DForm.h"

#include "Math/Point3d.h"
#include "Math/Point3i.h"
#include "Scene/SceneView.h"
#include "Utils/Clipping.h"
using namespace Core;

#include "GLSceneRender.h"
using namespace Render;

class Vox3DForm;

class MyOpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

    /// test
    friend class ReadOpt;

public:
    MyOpenGLWidget(QWidget* parent = nullptr);
    ~MyOpenGLWidget();

    SceneView* sceneView() { return m_scene_view.ptr(); }

    /// Init
    void initGLWidget(Vox3DForm* form);

    void updateXOR();

    /// 専用のデータ管理
    void addMaterialIdToNode(int material_id, Node* node);
    void setMaterialIdToNode(std::vector<std::pair<int, RefPtr<Node>>>& material_id_node_list);
    void clearMaterialIdToNode(bool delete_node = false);
    void setMaterialAllBBox(const BoundingBox3f& bbox);

    std::map<int, WeakRefPtr<Node>>& materialIdNodes() { return m_material_node_map; }
    void setMaterialIdToNodeDirect(std::map<int, WeakRefPtr<Node>>& materials) { m_material_node_map = materials; }

    void updateRenderData();

    void setViewNearFar();

    void suppressRender(bool suppress, bool update_unsuppress = true);

    /// 表示
    void   showMaterial(bool show, bool update_view);
    void   showMaterial(int material_id, bool show, bool update_view);
    bool   noshowMaterial(QList<int>& mateial_ids, bool update_view);
    void   materialIds(std::set<int>& material_ids, bool only_show);
    bool   voxelCellInfo(Point3d& org, Point3d& delta, Point3i& number);
    Voxel* firstVoxel(bool only_display = false);
    void   transparentMateial(QList<int>& mateial_ids, bool update_view);
    void   drawMode(const std::map<int, std::pair<bool, bool>>& draw_mode_list, bool update_view);
    void   setDrawMode(int material_id, bool shading, bool wireframe, bool update_view = false);
    void   optMateial(QList<int>& mateial_ids, bool update_view);
    void   setColor(int material_id, const Point3f& color, bool update_view);

    const std::map<int, WeakRefPtr<Node>>& voxMaterials() { return m_material_node_map; }

    bool hasProjectOpt(bool display) const;

    bool hasOptOp2();

    void fitDisplay();
    void fitDisplay(int material_id);
    void fitDisplay(const std::vector<int>& material_ids);
    void fitDisplay(Node* node);

    void imageCopyToClipboard();
    void imageCopyToFile();

    void outputResultImage(int result_width, int result_height, bool voxel_scalar_priority, bool add_colorbar,
                           QImage& image);

    void calcColorBarArea(int color_bar_area[4]);

    void setLightDir(const Point3f& dir, bool update_view = true);
    bool isEqaulLightDir(const Point3f& lightDir) const;

    void initView();
    void frontView();
    void backView();
    void leftView();
    void rightView();
    void topView();
    void bottomView();
    void axoView();
    void dirView(const Point3f& dir);

    void rotateLeft();
    void rotateRight();
    void rotateUp();
    void rotateDown();
    void rotate(const Point3f& axis, float degree);

    void zoomIn();
    void zoomOut();

    void setRotateMode() { m_mouse_mode = MouseMode::Rotate; }
    void setPanMode() { m_mouse_mode = MouseMode::Pan; }
    void setNoneMode() { m_mouse_mode = MouseMode::None; }

    void setProjectionOrtho();
    void setProjectionPerspective();

    void setMouseZoomReverse(bool reverse) { m_mouse_zoom_reverse = reverse; }
    bool isMouseZoomReverse() const { return m_mouse_zoom_reverse; }

    void setZAxisRotateMode(bool mode);
    bool isZAxisRotateMode() const;

    void setRotationCenter(const QPoint& local_mouse_pos);

    void setPickMode(bool pick, RenderSnap snap);

    void setPickColor(QColor color);

    bool pickClear();

    bool pickVoxelGL(const Point3f& obj_pos, Point3f& pick_pos);

    bool adjustPickData(PickData& pick_data, bool above_pick_point, std::vector<Node*>* pick_nodes = nullptr);

    void clearDimensions();
    void clearAutoDimensions();

    void   setBackgroundColor(const QColor& color, bool update_view = true);
    QColor backgroundColor();
    void   setBackGroundColorGradient(bool grad, bool update_view = true);
    bool   backGroundColorGradient();

    void addDragFilter(ObjectType type);
    void clearDragFilter();

    void setMaxPickLength(float length);

    bool               create3DTexture(VoxelScalar* voxel_scalar);
    void               create2DTexture(VoxelScalar* voxel_scalar);
    void               set3DTextureColor(const std::vector<float>& division, const std::vector<Point4f>& color);
    void               set3DTextureDivision(const std::vector<float>& division);
    std::vector<float> textureDivision();
    void               setColorLabelCount(int label_count);
    void               setShowColorBar(bool show);
    void               setColorMinMaxLabel(const std::wstring& min_label, const std::wstring& max_label);
    void               createRenderData(Voxel* voxel);
    void               createRenderData(Node* node);

    void setVoxelScalarPriority(bool priority);
    void setClippingPlanes(const std::vector<Clipping::PlaneInfo>& clip_planes);
    void clearClippingPlanes();

    void setOpt3DNode(Node* node)
    {
        m_opt_3d_node = node;
        setProjectVoxelScalar();
    }
    void showOpt3D(bool show, bool update_view);

    void removeOpt3DNode();

    // void setVoxelDrawMode(bool shading, bool wireframe)
    //{
    //     m_scene_render->setVoxelDrawMode(shading, wireframe);
    //     update();
    // }

    void setVoxelWireframeColorShape(bool shape_color, bool update_view = true)
    {
        m_scene_render->setVoxelWireframeColorShape(shape_color);
        if (update_view) {
            update();
        }
    }

    void setVoxelWireframeColor(const QColor& color, bool update_view = true)
    {
        m_scene_render->setVoxelWireframeColor({color.redF(), color.greenF(), color.blueF(), 1.0});
        if (update_view) {
            update();
        }
    }

    void setVoxelWireframeColor(const Point4f& color, bool update_view = true)
    {
        m_scene_render->setVoxelWireframeColor({color[0], color[1], color[2], 1.0});
        if (update_view) {
            update();
        }
    }

    QColor voxelWireframeColor()
    {
        const Point4f& color = m_scene_render->voxelWireframeColor();
        QColor         qcolor;
        qcolor.setRedF(color[0]);
        qcolor.setGreenF(color[1]);
        qcolor.setBlueF(color[2]);
        return qcolor;
    }

    float voxelWireframeWidth() { return m_scene_render->voxelWireframeWidth(); }

    bool isVoxelWireframeColorShape() { return m_scene_render->isVoxelWireframeColorShape(); }

    void setVoxelWireframeWidth(float width, bool update_view = true)
    {
        m_scene_render->setVoxelWireframeWidth(width);
        if (update_view) {
            update();
        }
    }

    void setProjectVoxelScalar()
    {
        m_scene_render->setProjectVoxelScalar(m_opt_3d_node.ptr());
        update();
    }

    void setProjectOptBaseColor(const QColor& color)
    {
        m_scene_render->setProjectVoxelScalarBaseColor({color.redF(), color.greenF(), color.blueF(), 1.0});
        update();
    }

    void setProjectOptBaseColorShape(bool shape_color)
    {
        m_scene_render->setProjectVoxelScalarBaseColorShape(shape_color);
        update();
    }

    QColor projectOptBaseColor() const
    {
        const Point4f& color = m_scene_render->projectVoxelScalarBaseColor();
        QColor         qcolor;
        qcolor.setRedF(color[0]);
        qcolor.setGreenF(color[1]);
        qcolor.setBlueF(color[2]);
        return qcolor;
    }

public:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void paintPickGL(const QPoint& mousePos, int pixelRadius, Object*& pickObj, QVector3D& pickPos);

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    void dimensionMouseMove();

private:
    bool   m_enable_last_press = false;
    QPoint m_last_press_position;
    QPoint m_last_mouse_position;

    Vox3DForm* m_3DForm = nullptr;

    enum MouseMode {
        Rotate,
        Pan,
        None,
    };
    MouseMode m_mouse_mode         = Rotate;
    bool      m_mouse_zoom_reverse = false;

    bool       m_pick_mode = false;
    RenderSnap m_pick_snap = RenderSnap::SnapNone;

    GLSceneRender*    m_scene_render = nullptr;
    RefPtr<SceneView> m_scene_view;

    std::map<int, WeakRefPtr<Node>> m_material_node_map;
    BoundingBox3f                   m_all_vox_bbox;

    WeakRefPtr<Node> m_opt_3d_node;

    WeakRefPtr<Node>   m_drag_node;
    WeakRefPtr<Object> m_drag_object;
    Point3f            m_drag_start;
    Point3f            m_drag_end;
};

#endif    // MYOPENGLWIDGET_H
