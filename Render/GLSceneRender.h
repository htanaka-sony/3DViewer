#ifndef RENDER_GLSCENERENDER_H
#define RENDER_GLSCENERENDER_H

#include "RenderGlobal.h"

#include "Math/Point2i.h"
#include "Math/Point3f.h"
#include "Scene/Object.h"
#include "Scene/PickObject.h"
#include "Scene/SceneView.h"

QT_BEGIN_NAMESPACE
class QOpenGLFunctions;
class QOpenGLContext;
class QOpenGLShaderProgram;
class QOpenGLWidget;
QT_END_NAMESPACE

namespace Core {
class Node;
class Mesh;
class NormalMesh;
class Voxel;
class VoxelScalar;
class Dimension;
class MultiDimension;
class RenderEditableMesh;
class RenderEditableNormalMesh;
}    // namespace Core
using namespace Core;

RENDER_NAMESPACE_BEGIN

class GLRenderText;
class RenderData;

class RENDER_EXPORT GLSceneRender : Referenced {
public:
    static GLSceneRender* createSceneRender(SceneView* scene_view);
    static void           deleteSceneRender(GLSceneRender* scene_render);

protected:
    GLSceneRender();
    ~GLSceneRender();

public:
    void setSceneView(SceneView* scene_view);

    void initializeGL(QOpenGLWidget* gl_widget, QOpenGLFunctions* function);
    void resizeGL(int w, int h);
    void paintGL(int area[4] = nullptr);
    bool pickGL(const Point2i& mouse_pos, int pixel, PickData& pick_data, PickSnap snap = PickSnap::SnapNone,
                std::vector<Node*>* all_nodes = nullptr, const Point3f* obj_pos = nullptr);
    bool pickGLFunc(const Point2i& mouse_pos, int pixel, PickData& pick_data, PickSnap snap = PickSnap::SnapNone,
                    std::vector<Node*>* all_nodes = nullptr, const Point3f* obj_pos = nullptr);
    bool dragGL(const Point2i& mouse_pos, int pixel, PickData& pick_data);

    void setDpiScale();

    void renderBackGround();
    void renderBackGroundGradient();

    bool sizeDrawColorBar(int& x_min, int& y_min, int& x_max, int& y_max);
    void paintOnlyColorBar(int view_port[4], int x_offset, int y_offset);

    void paintBackground(int x, int y, int width, int height);

    void           setBackGroundColor(const Point4f& color) { m_background_color = color; }
    const Point4f& backgroundColor() const { return m_background_color; }

    void setBackGroundColorGradient(bool grad) { m_background_color_grad = grad; }
    bool backGroundColorGradient() const { return m_background_color_grad; }

    bool setXORPointRender(const Point3f& point)
    {
        bool change = false;
        if (m_cur_point.has_value()) {
            if (m_pre_point != m_cur_point) {
                m_pre_point = m_cur_point;
                change      = true;
            }
        }
        else {
            change = true;
        }
        m_cur_point = point;
        return change;
    }
    void resetXORPointRender()
    {
        m_cur_point.reset();
        m_pre_point.reset();
        m_xor_render = false;
    }
    bool resetXORCurrent()
    {
        if (m_cur_point.has_value()) {
            m_pre_point = m_cur_point;
            m_cur_point.reset();
            return true;
        }
        return false;
    }
    void setXORRender(bool xor_render) { m_xor_render = xor_render; }
    void renderXORPoint(const Point3f& point, float point_size);
    void renderPoint(const Point3f& point, float point_size);

    /// Create
    void createRenderData(Node* node);
    void createRenderableData(RenderableNode* renderable);
    void createRenderEditableMeshData(RenderEditableMesh* mesh);
    void createRenderEditableNormalMeshData(RenderEditableNormalMesh* mesh);

    void suppressRender(bool suppress);
    bool isSuppressRender() { return m_suppress_render; }

    void                   addPickObject(PickData& pick_data);
    void                   popPickObject();
    void                   clearPickObjects();
    std::vector<PickData>& pickObjects() { return m_pick_objects; }
    void                   setPickColor(const Point4f& color) { m_pick_color = color; }

    void addDragFilter(ObjectType type) { m_drag_filter.insert(type); }
    void clearDragFilter() { m_drag_filter.clear(); }

    void setPickTargetFilter(const std::set<ObjectType>& pick_filter);
    void resetPickTargetFilter();

    void setPickNodeFilter(const std::set<Node*>& target_nodes);
    void clearPickNodeFilter();

    void setMaxPickLength(float length);

    void               set3DTextureColor(const std::vector<float>& division, const std::vector<Point4f>& color);
    void               set3DTextureDivision(const std::vector<float>& division);
    std::vector<float> textureDivision();
    void               setColorLabelCount(int label_count);
    void               setShowColorBar(bool show);
    bool               isShowColorBar() const { return m_show_colorbar; }
    void               setColorMinMaxLabel(const std::wstring& min_label, const std::wstring& max_label);

    bool create3DTexture(VoxelScalar* voxel_scalar);
    void create2DTexture(VoxelScalar* voxel_scalar);

    void set3DTextureColoaMap(const std::vector<float>& division, const std::vector<Point4f>& colors);

    void setVoxelScalarPriority(bool priority) { m_voxel_scalar_priority = priority; }
    bool voxelScalarPriority() const { return m_voxel_scalar_priority; }

    void setPickVoxelScalarPriorityInvalid(bool invalid) { m_pick_voxel_scalar_priority_invalid = invalid; }
    bool pickVoxelScalarPriorityInvalid() const { return m_pick_voxel_scalar_priority_invalid; }

    void setPickWireframeInvalid(bool invalid) { m_pick_wireframe_invalid = invalid; }
    bool pickWireframeInvalid() const { return m_pick_wireframe_invalid; }

    void setClippingPlanes(std::vector<Planef>& planes) { m_clipping_planes = planes; }
    void clearClippingPlanes() { m_clipping_planes.clear(); }
    // void setVoxelDrawMode(bool shading, bool wireframe)
    //{
    //     m_voxel_draw_shading   = shading;
    //     m_voxel_draw_wireframe = wireframe;
    // }

    void setVoxelWireframeColorShape(bool shape_color) { m_voxel_draw_wireframe_color_shape = shape_color; }

    bool isVoxelWireframeColorShape() const { return m_voxel_draw_wireframe_color_shape; }

    void setVoxelWireframeColor(const Point4f& color) { m_voxel_draw_wireframe_color = color; }

    const Point4f& voxelWireframeColor() const { return m_voxel_draw_wireframe_color; }

    void  setVoxelWireframeWidth(float width) { m_voxel_draw_wireframe_width = width; }
    float voxelWireframeWidth() const { return m_voxel_draw_wireframe_width; }

    void setProjectVoxelScalar(Node* voxel_scalar_node) { m_voxel_scalar = voxel_scalar_node; }

    void setProjectVoxelScalarBaseColorShape(bool shape_color) { m_project_voxel_scalar_color_shape = shape_color; }

    void setProjectVoxelScalarBaseColor(const Point4f& color) { m_project_voxel_scalar_color = color; }

    const Point4f& projectVoxelScalarBaseColor() const { return m_project_voxel_scalar_color; }

    void showCoordinateSystem(bool show) { m_show_coordinate_system = show; }
    bool isShowCoordinateSystem() const { return m_show_coordinate_system; }
    void setVoxelLineSmooth(bool smooth) { m_line_smooth = smooth; }
    bool isVoxelLineSmooth() const { return m_line_smooth; }

    void setMinRenderPixelSize(int pixels) { m_min_render_pixel_size = pixels; }
    int  minRenderPixelSize() const { return m_min_render_pixel_size; }

protected:
    struct Float3x3f {
        float m_matrix[3][3];
        Float3x3f(float a00, float a01, float a02, float a10, float a11, float a12, float a20, float a21, float a22)
        {
            m_matrix[0][0] = a00;
            m_matrix[0][1] = a01;
            m_matrix[0][2] = a02;
            m_matrix[1][0] = a10;
            m_matrix[1][1] = a11;
            m_matrix[1][2] = a12;
            m_matrix[2][0] = a20;
            m_matrix[2][1] = a21;
            m_matrix[2][2] = a22;
        }
        Float3x3f() {}
        Float3x3f(float (*matrix)[3]) { memcpy(m_matrix, matrix, sizeof(m_matrix)); }

        inline Point3f operator*(const Point3f point)
        {
            return Point3f(m_matrix[0][0] * point.x() + m_matrix[0][1] * point.y() + m_matrix[0][2] * point.z(),
                           m_matrix[1][0] * point.x() + m_matrix[1][1] * point.y() + m_matrix[1][2] * point.z(),
                           m_matrix[2][0] * point.x() + m_matrix[2][1] * point.y() + m_matrix[2][2] * point.z());
        }
    };
    struct TransparentObject {
        Node*      m_node;
        Object*    m_object;
        Matrix4x4f m_path_matrix;
        float      m_depth;
        int        m_priority;
        TransparentObject(Node* node, Object* object, const Matrix4x4f& path_matrix, float depth, int priority)
            : m_node(node)
            , m_object(object)
            , m_path_matrix(path_matrix)
            , m_depth(depth)
            , m_priority(priority)
        {
        }
    };

    void applyMatrix();
    void pushMatrix(const Matrix4x4f& matrix = Matrix4x4f());
    void popMatrix();

    const Matrix4x4f& curPathMatrix() const;
    const Float3x3f&  curNormalMatrix() const;

    /// Render
    void renderCondition(bool transparent, bool pick_render, bool text_render, bool drag, PickSnap snap);
    bool renderScene();
    bool renderNode(Node* node);
    bool renderMesh(Node* node, Mesh* mesh);
    bool renderNormalMesh(Node* node, NormalMesh* mesh);
    bool renderVoxel(Node* node, Voxel* voxel);
    bool renderVoxelScalar(Node* node, VoxelScalar* voxel);
    bool renderDimension(Node* node, Dimension* dimension, bool multi_dimension = false,
                         bool multi_dimension_name = false);
    bool renderMultiDimension(Node* node, MultiDimension* dimension);

    bool renderRenderableMesh(Node* node, RenderEditableMesh* mesh, Object* object);
    bool renderRenderableNormalMesh(Node* node, RenderEditableNormalMesh* mesh, Object* object);

    void drawCoordinateSystem();

    void drawColorBar(int x_offset = 0, int y_offset = 0);

    int  set3DTextureShader(const VoxelScalar* voxel_scalar);
    void reset3DTextureShader(int tile_count);

    bool is2DTexturePriority();

    bool isBoxTinyOnScreen(const BoundingBox3f& bbox, const Matrix4x4f& path_matrix) const;

protected:
    RefPtr<SceneView> m_scene_view;

    GLRenderText* m_render_text;

    QOpenGLShaderProgram* m_paint_shader_program;
    QOpenGLShaderProgram* m_pick_shader_program;
    QOpenGLShaderProgram* m_paint_shader_no_norms_program;
    QOpenGLShaderProgram* m_paint_segment_program;
    QOpenGLShaderProgram* m_cur_shader_program;
    QOpenGLShaderProgram* m_background_shader_program;

    /// Qt Render
    QOpenGLFunctions* m_gl_function;
    QOpenGLWidget*    m_gl_widget;

    unsigned int m_pick_fbo;
    unsigned int m_pick_color_texture;
    unsigned int m_pick_depth_buffer;

    /// Render条件
    bool                 m_cond_transparent = false;
    bool                 m_cond_pick_render = false;
    bool                 m_cond_text_render = false;
    bool                 m_cond_drag        = false;
    PickSnap             m_cond_snap        = PickSnap::SnapNone;
    std::set<ObjectType> m_drag_filter;
    bool                 m_temporary_draw = false;

    /// Stack Matrix
    VecMatrix4x4f          m_stack_m_matrix;
    VecMatrix4x4f          m_stack_mv_matrix;
    VecMatrix4x4f          m_stack_mvp_matrix;
    VecMatrix4x4f          m_stack_path_matrix;
    Matrix4x4f             m_stack_vp_matrix;
    std::vector<Float3x3f> m_stack_normal_matrix;

    /// Stack Transparent
    std::vector<TransparentObject> m_stack_transparent_objects;

    /// 処理中のStack Pick
    std::vector<PickData> m_stack_pick_objects;

    /// ピック保持しているオブジェクト
    std::vector<PickData> m_pick_objects;
    Point4f               m_pick_color;

    /// 指定タイプのみ
    std::set<ObjectType> m_pick_target_filter;

    /// ワイヤーフレームを除外
    bool m_pick_wireframe_invalid = false;

    std::set<Node*> m_pick_target_node;

    float m_max_pick_length = FLT_MAX;

    /// 描画抑制
    bool m_suppress_render;
    /// XOR一時描画
    bool                   m_xor_render = false;
    std::optional<Point3f> m_pre_point;
    std::optional<Point3f> m_cur_point;

    /// 背景色
    Point4f m_background_color      = Point4f(1.0f, 1.0f, 1.0f, 1.0f);
    bool    m_background_color_grad = false;

    float m_dpi_scale  = 1.0f;
    int   m_pick_pixel = 5.0f;

    /// 微小オブジェクトのカリング閾値（ピクセル）。0以下は無効
    int m_min_render_pixel_size = 0;

    /// Shading + Wireframe設定
    // bool    m_voxel_draw_shading               = true;
    // bool    m_voxel_draw_wireframe             = false;
    bool    m_voxel_draw_wireframe_color_shape = false;
    Point4f m_voxel_draw_wireframe_color;
    float   m_voxel_draw_wireframe_width = 1;

    bool    m_project_voxel_scalar_color_shape = false;
    Point4f m_project_voxel_scalar_color;

    /// 暫定
    std::vector<float>   m_division;
    std::vector<Point4f> m_tex_color;

    /// 暫定
    std::vector<Planef> m_clipping_planes;

    WeakRefPtr<Node> m_voxel_scalar;
    bool             m_voxel_scalar_priority              = true;
    bool             m_pick_voxel_scalar_priority_invalid = true;

    /// カラーバー（暫定）
    bool         m_show_colorbar    = false;
    int          m_colorlabel_count = 0;
    std::wstring m_color_label_min;
    std::wstring m_color_label_max;

    /// 描画調整（イメージ出力など）
    bool m_line_smooth            = true;
    bool m_show_coordinate_system = true;
};

RENDER_NAMESPACE_END

#endif    // RENDER_GLSCENERENDER_H
