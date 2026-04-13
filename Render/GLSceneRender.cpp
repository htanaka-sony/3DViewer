#include "GLSceneRender.h"
#include "GLRenderText.h"

#include "RenderData.h"

#include "Scene/Dimension.h"
#include "Scene/MultiDimension.h"
#include "Scene/SceneGraph.h"
#include "Scene/Voxel.h"
#include "Scene/VoxelScalar.h"

#include <QOpenGLExtraFunctions>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>

#include <algorithm>

#include "Utils/Picking.h"

#ifdef MEASURE_TIME
    #include <QElapsedTimer>
#endif

void printGPUMemoryInfo()
{
    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (!context) {
        return;
    }

    QOpenGLFunctions* f = context->functions();
    if (!f) {
        return;
    }

    // NVIDIA
    if (context->hasExtension("GL_NVX_gpu_memory_info")) {
        GLint total_mem_kb     = 0;
        GLint cur_avail_mem_kb = 0;
        f->glGetIntegerv(0x9048, &total_mem_kb);        // GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX
        f->glGetIntegerv(0x9049, &cur_avail_mem_kb);    // GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX
        DEBUG() << "[NVIDIA] GPU Total Memory (KB):" << total_mem_kb;
        DEBUG() << "[NVIDIA] GPU Available Memory (KB):" << cur_avail_mem_kb;
        return;
    }

    // AMD
    if (context->hasExtension("GL_ATI_meminfo")) {
        GLint vbo_free_mem[4] = {0};
        f->glGetIntegerv(0x87FB, vbo_free_mem);    // GL_VBO_FREE_MEMORY_ATI
        DEBUG() << "[AMD] VBO Free Memory (KB):" << vbo_free_mem[0];
        DEBUG() << "[AMD] VBO Largest Free Block (KB):" << vbo_free_mem[1];
        DEBUG() << "[AMD] VBO Total Memory (KB):" << vbo_free_mem[2];
        DEBUG() << "[AMD] VBO Aux Memory (KB):" << vbo_free_mem[3];
        return;
    }

    DEBUG() << "GPU Info None";
}

RENDER_NAMESPACE_BEGIN

GLSceneRender* GLSceneRender::createSceneRender(SceneView* scene_view)
{
    auto scene_render = new GLSceneRender();
    scene_render->setSceneView(scene_view);
    return scene_render;
}

void GLSceneRender::deleteSceneRender(GLSceneRender* scene_render)
{
    delete scene_render;
}

GLSceneRender::GLSceneRender()
    : m_pick_fbo(0)
    , m_pick_color_texture(0)
    , m_pick_depth_buffer(0)
    , m_gl_function(nullptr)
    , m_gl_widget(nullptr)
    , m_render_text(nullptr)
    , m_paint_shader_program(nullptr)
    , m_pick_shader_program(nullptr)
    , m_paint_shader_no_norms_program(nullptr)
    , m_paint_segment_program(nullptr)
    , m_background_shader_program(nullptr)
    , m_cur_shader_program(nullptr)
    , m_suppress_render(false)
{
}

GLSceneRender::~GLSceneRender()
{
    delete m_render_text;
    delete m_paint_shader_program;
    delete m_pick_shader_program;
    delete m_paint_segment_program;
    delete m_background_shader_program;
    delete m_paint_shader_no_norms_program;
}

void GLSceneRender::setSceneView(SceneView* scene_view)
{
    m_scene_view = scene_view;
}

void GLSceneRender::initializeGL(QOpenGLWidget* gl_widget, QOpenGLFunctions* function)
{
    m_gl_function = function;
    m_gl_widget   = gl_widget;

    /// Initialize
    m_gl_function->initializeOpenGLFunctions();

    /// GPU Info
    printGPUMemoryInfo();

    m_dpi_scale = m_gl_widget->devicePixelRatio();

    m_scene_view->setViewPort(0, 0, m_gl_widget->width() * m_dpi_scale, m_gl_widget->height() * m_dpi_scale);

    /// V-Syncを無効化
    QSurfaceFormat format = m_gl_widget->context()->format();
    format.setSwapInterval(0);
    m_gl_widget->context()->setFormat(format);

    m_gl_widget->setUpdateBehavior(QOpenGLWidget::PartialUpdate);

    renderBackGround();

    m_gl_function->glEnable(GL_DEPTH_TEST);
    m_gl_function->glEnable(GL_BLEND);
    m_gl_function->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_gl_function->glDepthFunc(GL_LEQUAL);

    m_render_text = new GLRenderText(gl_widget, m_scene_view.ptr());

    m_paint_shader_program = new QOpenGLShaderProgram;
    m_paint_shader_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/vertex.glsl");
    m_paint_shader_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/fragment.glsl");
    m_paint_shader_program->link();

    m_paint_shader_no_norms_program = new QOpenGLShaderProgram;
    m_paint_shader_no_norms_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/voxel_vertex.glsl");
    m_paint_shader_no_norms_program->addShaderFromSourceFile(QOpenGLShader::Geometry, ":/shaders/geometry.glsl");
    m_paint_shader_no_norms_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/fragment.glsl");
    m_paint_shader_no_norms_program->link();

    m_paint_segment_program = new QOpenGLShaderProgram;
    m_paint_segment_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/voxel_vertex.glsl");
    m_paint_segment_program->addShaderFromSourceFile(QOpenGLShader::Geometry, ":/shaders/segment_geometry.glsl");
    m_paint_segment_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/fragment.glsl");
    m_paint_segment_program->link();

    m_pick_shader_program = new QOpenGLShaderProgram;
    m_pick_shader_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/pick_vertex.glsl");
    m_pick_shader_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/pick_fragment.glsl");
    m_pick_shader_program->link();

    m_background_shader_program = new QOpenGLShaderProgram;
    m_background_shader_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/background_vertex.glsl");
    m_background_shader_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/background_fragment.glsl");
    m_background_shader_program->link();

    // FBOの作成
    m_gl_function->glGenFramebuffers(1, &m_pick_fbo);
    m_gl_function->glBindFramebuffer(GL_FRAMEBUFFER, m_pick_fbo);

    // カラーバッファ用のテクスチャを作成
    m_gl_function->glGenTextures(1, &m_pick_color_texture);
    m_gl_function->glBindTexture(GL_TEXTURE_2D, m_pick_color_texture);
    m_gl_function->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_scene_view->viewPortWidth(),
                                m_scene_view->viewPortHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    m_gl_function->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    m_gl_function->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    m_gl_function->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pick_color_texture, 0);

    m_gl_function->glGenRenderbuffers(1, &m_pick_depth_buffer);
    m_gl_function->glBindRenderbuffer(GL_RENDERBUFFER, m_pick_depth_buffer);
    m_gl_function->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_scene_view->viewPortWidth(),
                                         m_scene_view->viewPortHeight());
    m_gl_function->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_pick_depth_buffer);
}

void GLSceneRender::resizeGL(int w, int h)
{
    m_dpi_scale = m_gl_widget->devicePixelRatio();

    m_scene_view->setViewPort(0, 0, w * m_dpi_scale, h * m_dpi_scale);

    /// ピックエリア
    /// カラーバッファのサイズを更新
    m_gl_function->glBindTexture(GL_TEXTURE_2D, m_pick_color_texture);
    m_gl_function->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w * m_dpi_scale, h * m_dpi_scale, 0, GL_RGB, GL_UNSIGNED_BYTE,
                                nullptr);

    /// 深度バッファのサイズを更新
    m_gl_function->glBindRenderbuffer(GL_RENDERBUFFER, m_pick_depth_buffer);
    m_gl_function->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w * m_dpi_scale, h * m_dpi_scale);
}

void GLSceneRender::paintGL(int area[4])
{
    if (m_suppress_render) {
        return;
    }
    // m_gl_widget->makeCurrent();

#ifdef MEASURE_TIME
    QElapsedTimer elapsedTime;
    elapsedTime.start();
#endif
    setDpiScale();

    /// Shader
    m_cur_shader_program = m_paint_shader_program;

    float point_size = 12.0f;

    int viewport_x, viewport_y, viewport_width, viewport_height;
    m_scene_view->viewPort(viewport_x, viewport_y, viewport_width, viewport_height);
    if (area) {
        m_gl_function->glViewport(area[0], area[1], area[2], area[3]);
    }
    else {
        m_gl_function->glViewport(viewport_x, viewport_y, viewport_width, viewport_height);
    }

    if (!m_xor_render) {
        /// Init
        m_gl_function->glEnable(GL_DEPTH_TEST);
        m_gl_function->glEnable(GL_BLEND);
        m_gl_function->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if (!area) {
            renderBackGround();
            m_gl_function->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            if (m_background_color_grad) {
                renderBackGroundGradient();
                m_gl_function->glClear(GL_DEPTH_BUFFER_BIT);
            }
        }

        m_scene_view->setupFrustum();

        const auto& light_dir = m_scene_view->lightDir();

        const auto& camera_dir = m_scene_view->cameraDir();
        const auto& camera_pos = m_scene_view->cameraEye();

        /// Voxel描画用の設定
        m_paint_shader_no_norms_program->bind();
        m_paint_shader_no_norms_program->setUniformValue("light_dir", light_dir.x(), light_dir.y(),
                                                         -light_dir.z());                     /// 光源の位置
        m_paint_shader_no_norms_program->setUniformValue("light_color", 1.0f, 1.0f, 1.0f);    /// 光源の色（白色）
        m_paint_shader_no_norms_program->setUniformValue("screen_size", (float)m_scene_view->viewPortWidth(),
                                                         (float)m_scene_view->viewPortHeight());
        m_paint_shader_no_norms_program->setUniformValue("disable_light", false);
        m_paint_shader_no_norms_program->setUniformValue("point_display", false);
        m_paint_shader_no_norms_program->setUniformValue("point_display_line", false);
        m_paint_shader_no_norms_program->setUniformValue("point_size", (float)(8.0f * m_dpi_scale));
        m_paint_shader_no_norms_program->setUniformValue("point_display_line_width", (float)(1.0f / 12.0f));
        m_paint_shader_no_norms_program->setUniformValue("point_display_line_dir", 1.0f, 0.0f);
        m_paint_shader_no_norms_program->setUniformValue("dashed_line", false);
        m_paint_shader_no_norms_program->setUniformValue("discard_out_range", false);
        m_paint_shader_no_norms_program->setUniformValue("direct_offset", false);
        m_paint_shader_no_norms_program->setUniformValue("direct_offset_value", 0.0f);
        m_paint_shader_no_norms_program->setUniformValue("camera_dir", camera_dir.x(), camera_dir.y(), camera_dir.z());
        m_paint_shader_no_norms_program->setUniformValue("camera_pos", camera_pos.x(), camera_pos.y(), camera_pos.z());
        m_paint_shader_no_norms_program->setUniformValue("is_perspective", m_scene_view->isPerspective());

        m_paint_shader_no_norms_program->release();

        m_paint_segment_program->bind();
        m_paint_segment_program->setUniformValue("light_dir", light_dir.x(), light_dir.y(),
                                                 -light_dir.z());                     /// 光源の位置
        m_paint_segment_program->setUniformValue("light_color", 1.0f, 1.0f, 1.0f);    /// 光源の色（白色）
        m_paint_segment_program->setUniformValue("screen_size", (float)m_scene_view->viewPortWidth(),
                                                 (float)m_scene_view->viewPortHeight());
        m_paint_segment_program->setUniformValue("disable_light", false);
        m_paint_segment_program->setUniformValue("point_display", false);
        m_paint_segment_program->setUniformValue("point_display_line", false);
        m_paint_segment_program->setUniformValue("point_size", (float)(8.0f * m_dpi_scale));
        m_paint_segment_program->setUniformValue("point_display_line_width", (float)(1.0f / 12.0f));
        m_paint_segment_program->setUniformValue("point_display_line_dir", 1.0f, 0.0f);
        m_paint_segment_program->setUniformValue("dashed_line", false);
        m_paint_segment_program->setUniformValue("discard_out_range", false);
        m_paint_segment_program->setUniformValue("direct_offset", false);
        m_paint_segment_program->setUniformValue("direct_offset_value", 0.0f);
        m_paint_segment_program->setUniformValue("camera_dir", camera_dir.x(), camera_dir.y(), camera_dir.z());
        m_paint_segment_program->setUniformValue("camera_pos", camera_pos.x(), camera_pos.y(), camera_pos.z());
        m_paint_segment_program->setUniformValue("is_perspective", m_scene_view->isPerspective());
        m_paint_segment_program->release();

        /// Bind
        m_cur_shader_program->bind();
        m_cur_shader_program->setUniformValue("light_dir", light_dir.x(), light_dir.y(),
                                              -light_dir.z());                     /// 光源の位置
        m_cur_shader_program->setUniformValue("light_color", 1.0f, 1.0f, 1.0f);    /// 光源の色（白色）
        m_cur_shader_program->setUniformValue("screen_size", (float)m_scene_view->viewPortWidth(),
                                              (float)m_scene_view->viewPortHeight());
        m_cur_shader_program->setUniformValue("disable_light", false);
        m_cur_shader_program->setUniformValue("point_display", false);
        m_cur_shader_program->setUniformValue("point_display_line", false);
        m_cur_shader_program->setUniformValue("point_size", (float)(8.0f * m_dpi_scale));
        m_cur_shader_program->setUniformValue("point_display_line_width", (float)(1.0f / 12.0f));
        m_cur_shader_program->setUniformValue("point_display_line_dir", 1.0f, 0.0f);
        m_cur_shader_program->setUniformValue("dashed_line", false);
        m_cur_shader_program->setUniformValue("discard_out_range", false);
        m_cur_shader_program->setUniformValue("direct_offset", false);
        m_cur_shader_program->setUniformValue("direct_offset_value", 0.0f);
        m_cur_shader_program->setUniformValue("camera_dir", camera_dir.x(), camera_dir.y(), camera_dir.z());
        m_cur_shader_program->setUniformValue("camera_pos", camera_pos.x(), camera_pos.y(), camera_pos.z());
        m_cur_shader_program->setUniformValue("is_perspective", m_scene_view->isPerspective());
        reset3DTextureShader(-1);

        /// Matrix
        pushMatrix();

        /// Render
        renderScene();

        /// End
        popMatrix();

        /// Relese
        m_cur_shader_program->release();

        /// 暫定
        /// Render Pick Objects
        if (!m_pick_objects.empty()) {
            /// Init
            m_gl_function->glEnable(GL_DEPTH_TEST);
            m_gl_function->glDisable(GL_BLEND);
            m_gl_function->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            m_scene_view->setupFrustum();

            /// Bind
            m_cur_shader_program->bind();
            m_cur_shader_program->setUniformValue("screen_size", (float)m_scene_view->viewPortWidth(),
                                                  (float)m_scene_view->viewPortHeight());
            m_cur_shader_program->setUniformValue("disable_light", true);
            m_cur_shader_program->setUniformValue("point_display", false);
            m_cur_shader_program->setUniformValue("line_strip", false);
            m_cur_shader_program->setUniformValue("color", m_pick_color[0], m_pick_color[1], m_pick_color[2]);
            m_cur_shader_program->setUniformValue("transparency", m_pick_color[3]);

            for (auto& pick_data : m_pick_objects) {
                auto pick_object = pick_data.pickObject();
                if (pick_object && pick_object->type() == PickType::Vertex) {
                    renderPoint(((PickVertex*)pick_object)->point(), point_size);
                }
                else if (!pick_object) {
                    renderPoint(pick_data.pickPoint(), point_size);
                }
            }

            /// Relese
            m_cur_shader_program->release();
        }

        if (m_cur_point.has_value()) {
            /// Init
            m_gl_function->glEnable(GL_DEPTH_TEST);
            m_gl_function->glDisable(GL_BLEND);
            m_gl_function->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            m_scene_view->setupFrustum();

            /// Bind
            m_cur_shader_program->bind();
            m_cur_shader_program->setUniformValue("screen_size", (float)m_scene_view->viewPortWidth(),
                                                  (float)m_scene_view->viewPortHeight());
            m_cur_shader_program->setUniformValue("disable_light", true);
            m_cur_shader_program->setUniformValue("point_display", false);
            m_cur_shader_program->setUniformValue("line_strip", false);
            m_cur_shader_program->setUniformValue("color", 1.0f, 1.0f, 0.0f);
            m_cur_shader_program->setUniformValue("transparency", 1.0f);

            renderXORPoint(m_cur_point.value(), point_size);

            /// Relese
            m_cur_shader_program->release();
        }
        /// XORは初期化
        // resetXORPointRender();

        drawColorBar();

        drawCoordinateSystem();
    }
    else {
        /// Init
        m_gl_function->glEnable(GL_DEPTH_TEST);
        m_gl_function->glDisable(GL_BLEND);
        m_gl_function->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        m_scene_view->setupFrustum();

        /// Bind
        m_cur_shader_program->bind();
        m_cur_shader_program->setUniformValue("screen_size", (float)m_scene_view->viewPortWidth(),
                                              (float)m_scene_view->viewPortHeight());
        m_cur_shader_program->setUniformValue("disable_light", true);
        m_cur_shader_program->setUniformValue("point_display", false);
        m_cur_shader_program->setUniformValue("line_strip", false);
        m_cur_shader_program->setUniformValue("color", 1.0f, 1.0f, 0.0f);
        m_cur_shader_program->setUniformValue("transparency", 1.0f);

        if (m_pre_point.has_value()) {
            renderXORPoint(m_pre_point.value(), point_size);
        }
        if (m_cur_point.has_value()) {
            renderXORPoint(m_cur_point.value(), point_size);
        }

        /// Relese
        m_cur_shader_program->release();

        setXORRender(false);
    }

#ifdef MEASURE_TIME
    m_gl_function->glFinish();

    // フレームバッファのスワップ
    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (context) {
        context->swapBuffers(context->surface());
    }
    TextQuad text_quad;
    m_render_text->renderText(
        10, 20, 0,
        QString("Elapsed Time: %1 ms").arg((double)elapsedTime.nsecsElapsed() / 1000000.0, 0, 'f', 3).toStdWString(),
        std::wstring(L"Arial"), Point4f(1.0f, 0.0f, 0.0f, 1.0f), 12, text_quad);
#endif

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) DEBUG() << "GL ERROR Paint: " << err;
    // m_gl_widget->doneCurrent();
}

bool GLSceneRender::pickGL(const Point2i& mouse_pos, int pixel, PickData& pick_data, RenderSnap snap,
                           std::vector<Node*>* all_nodes, const Point3f* obj_pos)
{
    if (!all_nodes || snap == RenderSnap::SnapNone) {
        return pickGLFunc(mouse_pos, pixel, pick_data, snap, all_nodes, obj_pos);
    }
    else {
        PickData dummy;
        pickGLFunc(mouse_pos, pixel, dummy, RenderSnap::SnapNone, all_nodes, obj_pos);
        if (all_nodes->size() > 0) {
            setPickNodeFilter(std::set<Node*>(all_nodes->begin(), all_nodes->end()));
            bool ret = pickGLFunc(mouse_pos, pixel, pick_data, snap);
            clearPickNodeFilter();
            return ret;
        }
        else {
            return false;
        }
    }

    return false;
}

bool GLSceneRender::pickGLFunc(const Point2i& mouse_pos, int pixel, PickData& pick_data, RenderSnap snap,
                               std::vector<Node*>* all_nodes, const Point3f* /*obj_pos*/)
{
    setDpiScale();

    m_gl_widget->makeCurrent();

    /// Shader
    m_cur_shader_program = m_pick_shader_program;

    const auto& camera_dir = m_scene_view->cameraDir();
    const auto& camera_pos = m_scene_view->cameraEye();

    pixel *= m_dpi_scale;

    /// ピクセル→オブジェクト移動量
    Point3f object_move = m_scene_view->objectMove(pixel, 0);

    float move_length = object_move.length();
    if (move_length > m_max_pick_length) {
        pixel = (int)((float)pixel * (m_max_pick_length / move_length));
        if (pixel < 2) pixel = 2;
        object_move = m_scene_view->objectMove(pixel, 0);
        move_length = object_move.length();
    }

    m_pick_pixel = pixel;

    m_stack_pick_objects.clear();

    pick_data = PickData();

    m_gl_function->glBindFramebuffer(GL_FRAMEBUFFER, m_pick_fbo);

    /// フレームバッファクリア
    renderBackGround();
    m_gl_function->glEnable(GL_DEPTH_TEST);
    m_gl_function->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_gl_function->glEnable(GL_BLEND);
    m_gl_function->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /// ViewPort
    int viewport[4];
    m_scene_view->viewPort(viewport[0], viewport[1], viewport[2], viewport[3]);

    /// Projection Matrixを取得
    Matrix4x4f projection_matrix = m_scene_view->projectionMatrix();

    /// ピック領域
    int viewport_x      = mouse_pos.x() - pixel;
    int viewport_y      = viewport[3] - mouse_pos.y() - pixel;
    int viewport_width  = pixel * 2;
    int viewport_height = pixel * 2;

    /// ピック行列を作成
    Matrix4x4f pick_matrix;
    pick_matrix.pickMatrix(viewport_x + viewport_width / 2, viewport_y + viewport_height / 2, viewport_width,
                           viewport_height, viewport);

    /// ピック領域に限定
    m_gl_function->glEnable(GL_SCISSOR_TEST);
    m_gl_function->glScissor(viewport_x, viewport_y, viewport_width, viewport_height);

    /// 描画処理の限定
    m_scene_view->setupFrustum(pick_matrix * projection_matrix);

    /// Bind
    m_cur_shader_program->bind();
    m_cur_shader_program->setUniformValue("direct_offset", false);
    m_cur_shader_program->setUniformValue("direct_offset_value", 0.0f);
    m_cur_shader_program->setUniformValue("camera_dir", camera_dir.x(), camera_dir.y(), camera_dir.z());
    m_cur_shader_program->setUniformValue("camera_pos", camera_pos.x(), camera_pos.y(), camera_pos.z());
    m_cur_shader_program->setUniformValue("is_perspective", m_scene_view->isPerspective());

    /// Matrix
    pushMatrix();

    /// ピック描画
    renderBackGround();
    m_gl_function->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderCondition(false, true, false, false, snap);
    renderNode(m_scene_view->sceneGraph()->rootNode());

    /// ピクセルデータ読み取り
    std::vector<GLubyte> pixels_obj(viewport_width * viewport_height * 4);    /// RGBAの4成分
    glReadPixels(viewport_x, viewport_y, viewport_width, viewport_height, GL_RGBA, GL_UNSIGNED_BYTE, pixels_obj.data());

    /// デプスバッファ読み取り
    std::vector<GLfloat> depth_buffer_obj(viewport_width * viewport_height);
    glReadPixels(viewport_x, viewport_y, viewport_width, viewport_height, GL_DEPTH_COMPONENT, GL_FLOAT,
                 depth_buffer_obj.data());

    //// テキスト描画（前面描画）
    renderBackGround();
    m_gl_function->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderCondition(false, true, true, false, snap);
    renderNode(m_scene_view->sceneGraph()->rootNode());

    /// ピクセルデータ読み取り
    std::vector<GLubyte> pixels_text(viewport_width * viewport_height * 4);    /// RGBAの4成分
    glReadPixels(viewport_x, viewport_y, viewport_width, viewport_height, GL_RGBA, GL_UNSIGNED_BYTE,
                 pixels_text.data());

    /// デプスバッファ読み取り
    std::vector<GLfloat> depth_buffer_text(viewport_width * viewport_height);
    glReadPixels(viewport_x, viewport_y, viewport_width, viewport_height, GL_DEPTH_COMPONENT, GL_FLOAT,
                 depth_buffer_text.data());

    m_gl_function->glDisable(GL_SCISSOR_TEST);

    /// マウス位置
    int center_x     = viewport_width / 2;
    int center_y     = viewport_height / 2;
    int center_index = (center_y * viewport_width + center_x) * 4;    /// RGBAの4成分
    int depth_index  = (center_y * viewport_width + center_x);

    float min_depth = 1.0f;

    if (snap == RenderSnap::SnapNone) {
        m_gl_function->glBindFramebuffer(GL_FRAMEBUFFER, 0);

        popMatrix();

        m_cur_shader_program->release();

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) DEBUG() << "GL ERROR PickGL: " << err;
        m_gl_widget->doneCurrent();

        int target_x = center_x;
        int target_y = center_y;

        for (int ic = 0; ic < 2; ++ic) {
            auto& pixels       = ic == 0 ? pixels_text : pixels_obj;
            auto& depth_buffer = ic == 0 ? depth_buffer_text : depth_buffer_obj;

            if (depth_index >= 0 && depth_index < depth_buffer.size()) {
                if (depth_buffer[depth_index] != 1.0f) {
                    min_depth         = depth_buffer[depth_index];
                    GLubyte red       = pixels[center_index];
                    GLubyte green     = pixels[center_index + 1];
                    GLubyte blue      = pixels[center_index + 2];
                    GLuint  picked_id = red | (green << 8) | (blue << 16);
                    if (picked_id < m_stack_pick_objects.size() && picked_id >= 0) {
                        pick_data = m_stack_pick_objects[picked_id];
                    }
                }
            }
            if (pick_data.pickNode() == nullptr) {
                /// 領域全体をスキャンして最小の深度値を持つピクセルを取得
                min_depth        = FLT_MAX;
                GLuint picked_id = INT_MAX;
                float  min_dist  = FLT_MAX;
                for (int y = 0; y < viewport_height; ++y) {
                    for (int x = 0; x < viewport_width; ++x) {
                        int index = (y * viewport_width + x);
                        if (depth_buffer[index] < min_depth) {
                            float dist = (center_x - x) * (center_x - x) + (center_y - y) * (center_y - y);
                            if (dist < min_dist) {
                                min_dist            = dist;
                                min_depth           = depth_buffer[index];
                                int     color_index = index * 4;
                                GLubyte red         = pixels[color_index];
                                GLubyte green       = pixels[color_index + 1];
                                GLubyte blue        = pixels[color_index + 2];
                                picked_id           = red | (green << 8) | (blue << 16);
                                target_x            = x;
                                target_y            = y;
                            }
                        }
                    }
                }
                if (picked_id < m_stack_pick_objects.size() && picked_id >= 0) {
                    pick_data = m_stack_pick_objects[picked_id];
                }
            }

            if (pick_data.pickNode() != nullptr) {
                break;
            }
        }

        if (pick_data.pickNode() != nullptr) {
            pick_data.setPickPoint(m_scene_view->unprojectFromScreen(
                Point3f(mouse_pos.x() - pixel + target_x, mouse_pos.y() - pixel + target_y, min_depth)));
        }

        if (all_nodes) {
            std::set<Node*> all_set_nodes;
            for (int ic = 0; ic < 2; ++ic) {
                auto& pixels       = ic == 0 ? pixels_text : pixels_obj;
                auto& depth_buffer = ic == 0 ? depth_buffer_text : depth_buffer_obj;

                GLuint picked_id = INT_MAX;
                for (int y = 0; y < viewport_height; ++y) {
                    for (int x = 0; x < viewport_width; ++x) {
                        int index = (y * viewport_width + x);
                        if (depth_buffer[index] != 1.0f) {
                            int     color_index = index * 4;
                            GLubyte red         = pixels[color_index];
                            GLubyte green       = pixels[color_index + 1];
                            GLubyte blue        = pixels[color_index + 2];
                            picked_id           = red | (green << 8) | (blue << 16);
                            if (picked_id < m_stack_pick_objects.size() && picked_id >= 0) {
                                Node* node = m_stack_pick_objects[picked_id].pickNode();
                                all_set_nodes.insert(node);
                            }
                        }
                    }
                }
            }
            *all_nodes = std::vector<Node*>(all_set_nodes.begin(), all_set_nodes.end());
        }

        m_stack_pick_objects.clear();
        return true;
    }
    else {
        m_gl_function->glBindFramebuffer(GL_FRAMEBUFFER, 0);

        popMatrix();

        m_cur_shader_program->release();

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) DEBUG() << "GL ERROR PickGL: " << err;
        m_gl_widget->doneCurrent();

        if (snap & RenderSnap::SnapVertex) {
            for (int ic = 0; ic < 2; ++ic) {
                auto& pixels       = ic == 0 ? pixels_text : pixels_obj;
                auto& depth_buffer = ic == 0 ? depth_buffer_text : depth_buffer_obj;

                if (depth_index >= 0 && depth_index < depth_buffer.size()) {
                    if (depth_buffer[depth_index] != 1.0f) {
                        GLubyte red       = pixels[center_index];
                        GLubyte green     = pixels[center_index + 1];
                        GLubyte blue      = pixels[center_index + 2];
                        GLuint  picked_id = red | (green << 8) | (blue << 16);
                        if (picked_id < m_stack_pick_objects.size() && picked_id >= 0) {
                            if (m_stack_pick_objects[picked_id].type() == PickType::Vertex) {
                                min_depth = depth_buffer[depth_index];
                                pick_data = m_stack_pick_objects[picked_id];
                                pick_data.setPickPoint(((PickVertex*)pick_data.pickObject())->point());
                            }
                        }
                    }
                }
                if (pick_data.pickNode() == nullptr) {
                    /// 領域全体をスキャンして最小の深度値を持つピクセルを取得
                    min_depth = FLT_MAX;
                    for (int y = 0; y < viewport_height; ++y) {
                        for (int x = 0; x < viewport_width; ++x) {
                            int index = (y * viewport_width + x);
                            if (depth_buffer[index] < min_depth) {
                                int     color_index = index * 4;
                                GLubyte red         = pixels[color_index];
                                GLubyte green       = pixels[color_index + 1];
                                GLubyte blue        = pixels[color_index + 2];
                                GLuint  picked_id   = red | (green << 8) | (blue << 16);
                                if (picked_id < m_stack_pick_objects.size() && picked_id >= 0) {
                                    if (m_stack_pick_objects[picked_id].type() == PickType::Vertex) {
                                        /// 周囲に2通りの描画ある（自分以外で

                                        min_depth = depth_buffer[index];
                                        pick_data = m_stack_pick_objects[picked_id];
                                        pick_data.setPickPoint(((PickVertex*)pick_data.pickObject())->point());
                                    }
                                }
                            }
                        }
                    }
                }

                if (pick_data.pickNode() != nullptr) {
                    break;
                }
            }
        }

        if (pick_data.pickNode() == nullptr && (snap & RenderSnap::SnapEdge)) {
            for (int ic = 0; ic < 2; ++ic) {
                auto& pixels       = ic == 0 ? pixels_text : pixels_obj;
                auto& depth_buffer = ic == 0 ? depth_buffer_text : depth_buffer_obj;

                int target_x = center_x;
                int target_y = center_y;

                if (depth_index >= 0 && depth_index < depth_buffer.size()) {
                    if (depth_buffer[depth_index] != 1.0f) {
                        GLubyte red       = pixels[center_index];
                        GLubyte green     = pixels[center_index + 1];
                        GLubyte blue      = pixels[center_index + 2];
                        GLuint  picked_id = red | (green << 8) | (blue << 16);
                        if (picked_id < m_stack_pick_objects.size() && picked_id >= 0) {
                            if (m_stack_pick_objects[picked_id].type() == PickType::Edge) {
                                min_depth = depth_buffer[depth_index];
                                pick_data = m_stack_pick_objects[picked_id];
                                // pick_data.setPickPoint(((PickEdge*)pick_data.pickObject())->point());
                            }
                        }
                    }
                }

                if (pick_data.pickNode() == nullptr) {
                    /// 領域全体をスキャンして最小の深度値を持つピクセルを取得
                    min_depth = FLT_MAX;
                    for (int y = 0; y < viewport_height; ++y) {
                        for (int x = 0; x < viewport_width; ++x) {
                            int index = (y * viewport_width + x);
                            if (depth_buffer[index] < min_depth) {
                                int     color_index = index * 4;
                                GLubyte red         = pixels[color_index];
                                GLubyte green       = pixels[color_index + 1];
                                GLubyte blue        = pixels[color_index + 2];
                                GLuint  picked_id   = red | (green << 8) | (blue << 16);
                                target_x            = x;
                                target_y            = y;
                                if (picked_id < m_stack_pick_objects.size() && picked_id >= 0) {
                                    if (m_stack_pick_objects[picked_id].type() == PickType::Edge) {
                                        min_depth = depth_buffer[index];
                                        pick_data = m_stack_pick_objects[picked_id];
                                        // pick_data.setPickPoint(((PickEdge*)pick_data.pickObject())->point());
                                    }
                                }
                            }
                        }
                    }
                }

                if (pick_data.pickNode() != nullptr) {
                    Point3f object_point = m_scene_view->unprojectFromScreen(
                        Point3f(mouse_pos.x() - pixel + target_x, mouse_pos.y() - pixel + target_y, min_depth));
                    pick_data.setPickPoint(((PickEdge*)pick_data.pickObject())->closestPoint(object_point));
                    break;
                }
            }
        }

        if (pick_data.pickNode() == nullptr && (snap & RenderSnap::SnapAny)) {
            int target_x = center_x;
            int target_y = center_y;

            for (int ic = 0; ic < 2; ++ic) {
                auto& pixels       = ic == 0 ? pixels_text : pixels_obj;
                auto& depth_buffer = ic == 0 ? depth_buffer_text : depth_buffer_obj;

                if (depth_index >= 0 && depth_index < depth_buffer.size()) {
                    if (depth_buffer[depth_index] != 1.0f) {
                        GLubyte red       = pixels[center_index];
                        GLubyte green     = pixels[center_index + 1];
                        GLubyte blue      = pixels[center_index + 2];
                        GLuint  picked_id = red | (green << 8) | (blue << 16);
                        if (picked_id < m_stack_pick_objects.size() && picked_id >= 0) {
                            min_depth = depth_buffer[depth_index];
                            pick_data = m_stack_pick_objects[picked_id];
                        }
                    }
                }
                if (pick_data.pickNode() == nullptr) {
                    /// 領域全体をスキャンして最小の深度値を持つピクセルを取得
                    min_depth      = FLT_MAX;
                    float min_dist = FLT_MAX;
                    for (int y = 0; y < viewport_height; ++y) {
                        for (int x = 0; x < viewport_width; ++x) {
                            int index = (y * viewport_width + x);
                            if (depth_buffer[index] < min_depth) {
                                float dist = (center_x - x) * (center_x - x) + (center_y - y) * (center_y - y);
                                if (dist < min_dist) {
                                    min_dist            = dist;
                                    int     color_index = index * 4;
                                    GLubyte red         = pixels[color_index];
                                    GLubyte green       = pixels[color_index + 1];
                                    GLubyte blue        = pixels[color_index + 2];
                                    GLuint  picked_id   = red | (green << 8) | (blue << 16);
                                    target_x            = x;
                                    target_y            = y;
                                    if (picked_id < m_stack_pick_objects.size() && picked_id >= 0) {
                                        min_depth = depth_buffer[index];
                                        pick_data = m_stack_pick_objects[picked_id];
                                    }
                                }
                            }
                        }
                    }
                }

                if (pick_data.pickNode() != nullptr) {
                    break;
                }
            }

            if (pick_data.pickNode() != nullptr) {
                pick_data.setPickPoint(m_scene_view->unprojectFromScreen(
                    Point3f(mouse_pos.x() - pixel + target_x, mouse_pos.y() - pixel + target_y, min_depth)));
            }
        }

        /*
        if (all_nodes) {
            std::set<Node*> all_set_nodes;
            for (int ic = 0; ic < 2; ++ic) {
                auto& pixels       = ic == 0 ? pixels_text : pixels_obj;
                auto& depth_buffer = ic == 0 ? depth_buffer_text : depth_buffer_obj;

                GLuint picked_id = INT_MAX;
                for (int y = 0; y < viewport_height; ++y) {
                    for (int x = 0; x < viewport_width; ++x) {
                        int index = (y * viewport_width + x);
                        if (depth_buffer[index] != 1.0f) {
                            int     color_index = index * 4;
                            GLubyte red         = pixels[color_index];
                            GLubyte green       = pixels[color_index + 1];
                            GLubyte blue        = pixels[color_index + 2];
                            picked_id           = red | (green << 8) | (blue << 16);
                            if (picked_id < m_stack_pick_objects.size() && picked_id >= 0) {
                                Node* node = m_stack_pick_objects[picked_id].pickNode();
                                all_set_nodes.insert(node);
                            }
                        }
                    }
                }
            }
            *all_nodes = std::vector<Node*>(all_set_nodes.begin(), all_set_nodes.end());
        }*/

        m_stack_pick_objects.clear();
        return true;
    }
}

bool GLSceneRender::dragGL(const Point2i& mouse_pos, int pixel, PickData& pick_data)
{
    setDpiScale();

    /// Shader
    m_cur_shader_program = m_pick_shader_program;

    const auto& camera_dir = m_scene_view->cameraDir();
    const auto& camera_pos = m_scene_view->cameraEye();
    m_cur_shader_program->setUniformValue("direct_offset", false);
    m_cur_shader_program->setUniformValue("direct_offset_value", 0.0f);
    m_cur_shader_program->setUniformValue("camera_dir", camera_dir.x(), camera_dir.y(), camera_dir.z());
    m_cur_shader_program->setUniformValue("camera_pos", camera_pos.x(), camera_pos.y(), camera_pos.z());
    m_cur_shader_program->setUniformValue("is_perspective", m_scene_view->isPerspective());

    pixel *= m_dpi_scale;
    m_pick_pixel = pixel;

    m_stack_pick_objects.clear();

    pick_data = PickData();

    m_gl_widget->makeCurrent();

    m_gl_function->glBindFramebuffer(GL_FRAMEBUFFER, m_pick_fbo);

    /// フレームバッファクリア
    renderBackGround();
    m_gl_function->glEnable(GL_DEPTH_TEST);
    m_gl_function->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_gl_function->glEnable(GL_BLEND);
    m_gl_function->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /// ViewPortを保持
    int viewport[4];
    m_scene_view->viewPort(viewport[0], viewport[1], viewport[2], viewport[3]);

    /// Projection Matrixを取得
    Matrix4x4f projection_matrix = m_scene_view->projectionMatrix();

    /// ピック領域
    int viewport_x      = mouse_pos.x() - pixel;
    int viewport_y      = viewport[3] - mouse_pos.y() - pixel;
    int viewport_width  = pixel * 2;
    int viewport_height = pixel * 2;

    /// ピック行列を作成
    Matrix4x4f pick_matrix;
    pick_matrix.pickMatrix(viewport_x + viewport_width / 2, viewport_y + viewport_height / 2, viewport_width,
                           viewport_height, viewport);

    /// ピック領域に限定
    m_gl_function->glEnable(GL_SCISSOR_TEST);
    m_gl_function->glScissor(viewport_x, viewport_y, viewport_width, viewport_height);

    /// 描画処理の限定
    m_scene_view->setupFrustum(pick_matrix * projection_matrix);

    /// Bind
    m_cur_shader_program->bind();

    /// Matrix
    pushMatrix();

    //// テキスト描画（前面描画）
    renderBackGround();
    m_gl_function->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderCondition(false, true, true, true, RenderSnap::SnapNone);
    renderNode(m_scene_view->sceneGraph()->rootNode());

    /// ピクセルデータ読み取り
    std::vector<GLubyte> pixels_text(viewport_width * viewport_height * 4);    /// RGBAの4成分
    glReadPixels(viewport_x, viewport_y, viewport_width, viewport_height, GL_RGBA, GL_UNSIGNED_BYTE,
                 pixels_text.data());

    /// デプスバッファ読み取り
    std::vector<GLfloat> depth_buffer_text(viewport_width * viewport_height);
    glReadPixels(viewport_x, viewport_y, viewport_width, viewport_height, GL_DEPTH_COMPONENT, GL_FLOAT,
                 depth_buffer_text.data());

    m_gl_function->glDisable(GL_SCISSOR_TEST);

    /// マウス位置
    int center_x     = viewport_width / 2;
    int center_y     = viewport_height / 2;
    int center_index = (center_y * viewport_width + center_x) * 4;    /// RGBAの4成分
    int depth_index  = (center_y * viewport_width + center_x);

    float min_depth = 1.0f;

    m_gl_function->glBindFramebuffer(GL_FRAMEBUFFER, 0);

    popMatrix();

    m_cur_shader_program->release();

    m_gl_widget->doneCurrent();

    auto& pixels       = pixels_text;
    auto& depth_buffer = depth_buffer_text;

    if (depth_index >= 0 && depth_index < depth_buffer.size()) {
        if (depth_buffer[depth_index] != 1.0f) {
            min_depth         = depth_buffer[depth_index];
            GLubyte red       = pixels[center_index];
            GLubyte green     = pixels[center_index + 1];
            GLubyte blue      = pixels[center_index + 2];
            GLuint  picked_id = red | (green << 8) | (blue << 16);
            if (picked_id < m_stack_pick_objects.size() && picked_id >= 0) {
                pick_data = m_stack_pick_objects[picked_id];
            }
        }
    }
    int target_x, target_y;
    if (pick_data.pickNode() == nullptr) {
        /// 領域全体をスキャンして最小の深度値を持つピクセルを取得
        min_depth        = FLT_MAX;
        GLuint picked_id = INT_MAX;
        for (int y = 0; y < viewport_height; ++y) {
            for (int x = 0; x < viewport_width; ++x) {
                int index = (y * viewport_width + x);
                if (depth_buffer[index] < min_depth) {
                    min_depth           = depth_buffer[index];
                    int     color_index = index * 4;
                    GLubyte red         = pixels[color_index];
                    GLubyte green       = pixels[color_index + 1];
                    GLubyte blue        = pixels[color_index + 2];
                    picked_id           = red | (green << 8) | (blue << 16);
                    target_x            = x;
                    target_y            = y;
                }
            }
        }
        if (picked_id < m_stack_pick_objects.size() && picked_id >= 0) {
            pick_data = m_stack_pick_objects[picked_id];
        }
    }

    if (pick_data.pickNode() != nullptr) {
        pick_data.setPickPoint(m_scene_view->unprojectFromScreen(Point3f(mouse_pos.x(), mouse_pos.y(), min_depth)));
    }

    m_stack_pick_objects.clear();

    return true;
}

void GLSceneRender::setDpiScale()
{
    float dpi_ratio = (float)m_gl_widget->devicePixelRatio();
    if (m_dpi_scale != dpi_ratio) {
        resizeGL(m_gl_widget->width(), m_gl_widget->height());
    }
}

void GLSceneRender::renderBackGround()
{
    m_gl_function->glClearColor(m_background_color[0], m_background_color[1], m_background_color[2],
                                m_background_color[3]);
}

void GLSceneRender::renderBackGroundGradient()
{
    QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();

    float fullscreenQuad[8] = {-1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f};

    unsigned int quadIndices[6] = {0, 1, 2, 2, 3, 0};

    m_background_shader_program->bind();

    float rTop = m_background_color[0], gTop = m_background_color[1], bTop = m_background_color[2];
    m_background_shader_program->setUniformValue("color_top", QVector3D(rTop, gTop, bTop));
    m_background_shader_program->setUniformValue("color_bottom", QVector3D(1.0, 1.0, 1.0));

    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, fullscreenQuad);

    f->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, quadIndices);

    f->glDisableVertexAttribArray(0);

    m_background_shader_program->release();
}

void GLSceneRender::applyMatrix()
{
    if (m_stack_mvp_matrix.empty()) {
        return;
    }

    const auto& cur_mvp_matrix = m_stack_mvp_matrix.back();
    m_cur_shader_program->setUniformValue("mvp_matrix", reinterpret_cast<const GLfloat(*)[4]>(cur_mvp_matrix.ptr()));

    m_cur_shader_program->setUniformValue("vp_matrix", reinterpret_cast<const GLfloat(*)[4]>(m_stack_vp_matrix.ptr()));

    const auto& cur_m_matrix = m_stack_m_matrix.back();
    m_cur_shader_program->setUniformValue("m_matrix", reinterpret_cast<const GLfloat(*)[4]>(cur_m_matrix.ptr()));

    const auto& normalMatrix = m_stack_normal_matrix.back();
    m_cur_shader_program->setUniformValue("nrm_matrix", normalMatrix.m_matrix);
}

void GLSceneRender::pushMatrix(const Matrix4x4f& matrix)
{
    /// 初回のMVP行列
    if (m_stack_mvp_matrix.empty()) {
        const Matrix4x4f& mMatrix   = m_scene_view->mMatrix();
        const Matrix4x4f& mvMatrix  = m_scene_view->mvMatrix();
        const Matrix4x4f& vpMatrix  = m_scene_view->vpMatrix();
        const Matrix4x4f& mvpMatrix = vpMatrix * mMatrix;

        m_cur_shader_program->setUniformValue("mvp_matrix", reinterpret_cast<const GLfloat(*)[4]>(mvpMatrix.ptr()));

        m_cur_shader_program->setUniformValue("vp_matrix", reinterpret_cast<const GLfloat(*)[4]>(vpMatrix.ptr()));

        m_cur_shader_program->setUniformValue("m_matrix", reinterpret_cast<const GLfloat(*)[4]>(mMatrix.ptr()));

        float normalMatrix[3][3];
        mvMatrix.normalMatrix(normalMatrix);
        m_cur_shader_program->setUniformValue("nrm_matrix", normalMatrix);

        m_stack_mvp_matrix.emplace_back(mvpMatrix);
        m_stack_mv_matrix.emplace_back(mvMatrix);
        m_stack_m_matrix.emplace_back(mMatrix);
        m_stack_normal_matrix.emplace_back(normalMatrix);
        m_stack_path_matrix.emplace_back(Matrix4x4f());
        m_stack_vp_matrix = vpMatrix;
    }
    /// ローカル座標系の適用
    else {
        auto cur_mvp_matrix = m_stack_mvp_matrix.back();

        cur_mvp_matrix = cur_mvp_matrix * matrix;

        m_cur_shader_program->setUniformValue("mvp_matrix",
                                              reinterpret_cast<const GLfloat(*)[4]>(cur_mvp_matrix.ptr()));

        auto cur_mv_matrix = m_stack_mv_matrix.back();

        cur_mv_matrix = cur_mv_matrix * matrix;

        float normalMatrix[3][3];
        cur_mv_matrix.normalMatrix(normalMatrix);
        m_cur_shader_program->setUniformValue("nrm_matrix", normalMatrix);

        auto cur_m_matrix = m_stack_m_matrix.back();

        cur_m_matrix = cur_m_matrix * matrix;

        m_cur_shader_program->setUniformValue("m_matrix", reinterpret_cast<const GLfloat(*)[4]>(cur_m_matrix.ptr()));

        m_stack_path_matrix.emplace_back(m_stack_path_matrix.back() * matrix);
        m_stack_mvp_matrix.emplace_back(cur_mvp_matrix);
        m_stack_mv_matrix.emplace_back(cur_mv_matrix);
        m_stack_m_matrix.emplace_back(cur_m_matrix);
        m_stack_normal_matrix.emplace_back(normalMatrix);
    }
}

void GLSceneRender::popMatrix()
{
    m_stack_mvp_matrix.pop_back();
    m_stack_mv_matrix.pop_back();
    m_stack_m_matrix.pop_back();
    m_stack_normal_matrix.pop_back();
    m_stack_path_matrix.pop_back();

    if (!m_stack_mvp_matrix.empty()) {
        m_cur_shader_program->setUniformValue("mvp_matrix",
                                              reinterpret_cast<const GLfloat(*)[4]>(m_stack_mvp_matrix.back().ptr()));
        m_cur_shader_program->setUniformValue("m_matrix",
                                              reinterpret_cast<const GLfloat(*)[4]>(m_stack_m_matrix.back().ptr()));
        m_cur_shader_program->setUniformValue("nrm_matrix", m_stack_normal_matrix.back().m_matrix);
    }
}

const Matrix4x4f& GLSceneRender::curPathMatrix() const
{
    if (m_stack_path_matrix.empty()) {
        return Matrix4x4f::identityMatrix();
    }
    return m_stack_path_matrix.back();
}

const GLSceneRender::Float3x3f& GLSceneRender::curNormalMatrix() const
{
    if (m_stack_normal_matrix.empty()) {
        static Float3x3f identity(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        return identity;
    }
    return m_stack_normal_matrix.back();
}

void GLSceneRender::renderCondition(bool transparent, bool pick_render, bool text_render, bool drag, RenderSnap snap)
{
    m_cond_transparent = transparent;
    m_cond_pick_render = pick_render;
    m_cond_text_render = text_render;
    m_cond_drag        = drag;
    m_cond_snap        = snap;
}

bool GLSceneRender::renderScene()
{
    m_stack_transparent_objects.clear();

    /// 通常描画
    renderCondition(false, false, false, false, RenderSnap::SnapNone);
    renderNode(m_scene_view->sceneGraph()->rootNode());

    Node* temporary_root = m_scene_view->temporaryRootNode();
    if (!temporary_root->children().empty()) {
        m_temporary_draw = true;
        renderNode(temporary_root);
        m_temporary_draw = false;
    }

    /// 透明描画
    if (m_stack_transparent_objects.size() > 0) {
        renderCondition(true, false, false, false, RenderSnap::SnapNone);

        std::sort(m_stack_transparent_objects.begin(), m_stack_transparent_objects.end(),
                  [](const TransparentObject& a, const TransparentObject& b) { return a.m_depth > b.m_depth; });

        /// デプスオフセットの有効化
        m_gl_function->glEnable(GL_POLYGON_OFFSET_FILL);

        float offset_add = 0.1f / m_stack_transparent_objects.size();
        float offset     = 0.0f;

        for (auto& transparent_object : m_stack_transparent_objects) {
            auto& node        = transparent_object.m_node;
            auto& object      = transparent_object.m_object;
            auto& path_matrix = transparent_object.m_path_matrix;

            /// Matrix設定
            bool push_matrix = false;
            if (!path_matrix.isIdentity()) {
                pushMatrix(path_matrix);
                push_matrix = true;
            }

            m_gl_function->glPolygonOffset(0.1f + offset, 0.1f);
            offset += offset_add;

            switch (object->type()) {
                case ObjectType::Voxel: {
                    renderVoxel(node, (Voxel*)object);
                } break;
                default:
                    break;
            }

            /// Matrix戻す
            if (push_matrix) {
                popMatrix();
            }
        }

        m_gl_function->glDisable(GL_POLYGON_OFFSET_FILL);

        m_stack_transparent_objects.clear();
    }

    /// テキスト描画
    renderCondition(false, false, true, false, RenderSnap::SnapNone);
    renderNode(m_scene_view->sceneGraph()->rootNode());

    if (!temporary_root->children().empty()) {
        /// テキスト描画
        m_temporary_draw = true;
        renderNode(temporary_root);
        m_temporary_draw = false;
    }

    return true;
}

bool GLSceneRender::renderNode(Node* node)
{
    if (!node->isVisible()) {
        return true;
    }

    /// Matrix設定
    bool        push_matrix = false;
    const auto& matrix      = node->matrix();
    if (!matrix.isIdentity()) {
        if (!m_scene_view->isBoxInFrustum(node->boundingBox(), curPathMatrix() * matrix)) {
            return true;
        }
        pushMatrix(matrix);
        push_matrix = true;
    }
    else {
        if (!m_scene_view->isBoxInFrustum(node->boundingBox(), curPathMatrix())) {
            return true;
        }
    }

    /// 以降はreturn しない
    /// 必要ならgotoとか。。。

    /// Render
    if (Object* object = node->object()) {
        /// 暫定
        if (m_cond_pick_render && !m_pick_target_filter.empty()) {
            if (!m_pick_target_filter.count(object->type())) {
                goto NEXT;
            }
        }
        if (m_cond_pick_render && !m_pick_target_node.empty()) {
            if (!m_pick_target_node.count(node)) {
                goto NEXT;
            }
        }

        switch (object->type()) {
            case ObjectType::Voxel: {
                renderVoxel(node, (Voxel*)object);
            } break;
            case ObjectType::VoxelScalar: {
                renderVoxelScalar(node, (VoxelScalar*)object);
            } break;
            case ObjectType::Dimension: {
                if (m_cond_drag) {
                    if (!m_drag_filter.count(object->type())) {
                        break;
                    }
                }
                renderDimension(node, (Dimension*)object);
            } break;
            case ObjectType::MultiDimension: {
                if (m_cond_drag) {
                    if (!m_drag_filter.count(object->type())) {
                        break;
                    }
                }
                renderMultiDimension(node, (MultiDimension*)object);
            } break;
            default:
                break;
        }
    }

NEXT:

    if (int num_chidlren = node->numChildren()) {
        for (int ic = 0; ic < num_chidlren; ++ic) {
            renderNode(node->child(ic));
        }
    }

    /// Matrix戻す
    if (push_matrix) {
        popMatrix();
    }

    return true;
}

void GLSceneRender::createRenderData(Node* node)
{
    if (Object* object = node->object()) {
        switch (object->type()) {
            case ObjectType::Voxel:
            case ObjectType::VoxelScalar: {
                createRenderVoxelData((Voxel*)object);
            } break;
            default:
                break;
        }
    }

    if (int num_chidlren = node->numChildren()) {
        for (int ic = 0; ic < num_chidlren; ++ic) {
            createRenderData(node->child(ic));
        }
    }
}

void GLSceneRender::createRenderVoxelData(Voxel* voxel)
{
    // DEBUG() << "createRenderVoxelData";

    if (!voxel->isRenderDirty()) {
        return;
    }

    voxel->resetRenderDirty();

    const auto& vertices = voxel->isEnableEditDisplayData() ? voxel->displayEditVertices() : voxel->displayVertices();
    const auto& indices  = voxel->isEnableEditDisplayData() ? voxel->displayEditIndices() : voxel->displayIndices();

    if (indices.size() == 0) {
        return;
    }

    m_gl_widget->makeCurrent();

    /// 初回
    RenderData* render_data = (RenderData*)voxel->renderData();
    if (!render_data) {
        render_data = new RenderData(m_gl_widget);
        render_data->m_vao.create();
        render_data->m_vbo.create();
        render_data->m_ibo.create();
        voxel->setRenderData(render_data, [](void* ptr, bool direct_delete) {
            if (direct_delete) {
                delete static_cast<RenderData*>(ptr);
            }
            else {
                /// m_gl_widgetのスレッド（メインスレッド）で削除
                QMetaObject::invokeMethod(
                    static_cast<RenderData*>(ptr)->m_gl_widget, [ptr]() { delete static_cast<RenderData*>(ptr); },
                    Qt::QueuedConnection);
            }
        });
    }

    render_data->m_use_vbo = false;

    if (voxel->isVboUse()) {
        render_data->m_vao.bind();

        if (render_data->m_vbo.isCreated()) {
            render_data->m_vbo.bind();
            int vertex_size = (int)vertices.size() * sizeof(Point3f);
            if (vertex_size > render_data->m_vbo.size()) {
                render_data->m_vbo.allocate((int)((double)vertex_size * 1.2));
            }
            render_data->m_vbo.write(0, vertices.data(), vertex_size);

            if (render_data->m_ibo.create()) {
                render_data->m_ibo.bind();
#if 0
            int indices_size = (int)indices.size() * sizeof(GLuint);
            if (indices_size > render_data->m_ibo.size()) {
                render_data->m_ibo.allocate((int)((double)(indices_size) * 1.2));
            }
            render_data->m_ibo.write(0, indices.data(), indices_size);
#else
                /// Edge表示
                const auto& seg_indices = voxel->isEnableEditDisplayData() ? voxel->displayEditSegmentIndices()
                                                                           : voxel->displaySegmentIndices();
                if (seg_indices.size() > 0) {
                    int indices_size     = (int)indices.size() * sizeof(GLuint);
                    int seg_indices_size = (int)seg_indices.size() * sizeof(GLuint);
                    if (indices_size + seg_indices_size > render_data->m_ibo.size()) {
                        render_data->m_ibo.allocate((int)((double)(indices_size + seg_indices_size) * 1.2));
                    }
                    render_data->m_ibo.write(0, indices.data(), indices_size);
                    render_data->m_ibo.write(indices_size, seg_indices.data(), seg_indices_size);
                }
                else {
                    int indices_size = (int)indices.size() * sizeof(GLuint);
                    if (indices_size > render_data->m_ibo.size()) {
                        render_data->m_ibo.allocate((int)((double)(indices_size) * 1.2));
                    }
                    render_data->m_ibo.write(0, indices.data(), indices_size);
                }
#endif
                render_data->m_use_vbo = true;
            }
        }

        if (render_data->m_use_vbo) {
            /// 作成中エラーーならこうなる？要確認
            if (!render_data->m_vao.isCreated()       //
                || !render_data->m_vbo.isCreated()    //
                || !render_data->m_ibo.isCreated()) {
                DEBUG() << "createRenderVoxelData VBO not created";
                render_data->m_use_vbo = false;
            }
        }

        if (!render_data->m_use_vbo) {
            render_data->m_vbo.release();
            render_data->m_ibo.release();
            render_data->m_vao.release();
        }
    }

    // DEBUG() << "createRenderVoxelData Allocated";

    QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();

    if (render_data->m_use_vbo) {
        /// 頂点属性の設定
        f->glEnableVertexAttribArray(0);
        f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point3f), 0);

        /// ダミー法線の設定
        f->glEnableVertexAttribArray(1);
        f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        f->glVertexAttribDivisor(1, 1);

        render_data->m_vao.release();
        render_data->m_vbo.release();
        render_data->m_ibo.release();
    }
    else {
        m_gl_function->glBindBuffer(GL_ARRAY_BUFFER, 0);
        m_gl_function->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        /// 頂点属性の設定
        f->glEnableVertexAttribArray(0);
        f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point3f), 0);

        /// ダミー法線の設定
        f->glEnableVertexAttribArray(1);
        f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        f->glVertexAttribDivisor(1, 1);
    }

    m_gl_widget->doneCurrent();
}

void GLSceneRender::suppressRender(bool suppress)
{
    m_suppress_render = suppress;
}

bool GLSceneRender::renderVoxel(Node* node, Voxel* voxel)
{
    /// Text描画なし
    if (m_cond_text_render) {
        return true;
    }
    if (m_cond_drag) {
        return true;
    }

    RenderData* render_data = (RenderData*)voxel->renderData();
    if (!render_data) {
        return true;
    }
    const auto& vertices = voxel->isEnableEditDisplayData() ? voxel->displayEditVertices() : voxel->displayVertices();
    const auto& indices  = voxel->isEnableEditDisplayData() ? voxel->displayEditIndices() : voxel->displayIndices();
    if (vertices.empty() || indices.empty()) {
        return true;
    }

    if (!m_cond_transparent && !m_cond_pick_render) {
        if (voxel->transparent() != 1.0f) {
            /// 暫定
            Point3f     camera_dir      = m_scene_view->cameraDir();
            Point3f     camera_eye      = m_scene_view->cameraEye();
            const auto& cur_path_matrix = curPathMatrix();

            float min_depth   = -FLT_MAX;
            int   numVertices = vertices.size();
            int   step        = numVertices / 1000;
            if (step <= 0) step = 1;
            if (cur_path_matrix.isIdentity()) {
                for (int ic = 0; ic < numVertices; ic += step) {
                    float depth = camera_dir * (vertices[ic] - camera_eye);
                    if (depth > min_depth) {
                        min_depth = depth;
                    }
                }
            }
            else {
                for (int ic = 0; ic < numVertices; ic += step) {
                    float depth = camera_dir * (cur_path_matrix * vertices[ic] - camera_eye);
                    if (depth > min_depth) {
                        min_depth = depth;
                    }
                }
            }

            m_stack_transparent_objects.emplace_back(node, voxel, curPathMatrix(), min_depth);
            return true;
        }
    }

    const bool voxel_draw_shading   = voxel->isDrawShading();
    const bool voxel_draw_wireframe = voxel->isDrawWireframe();

    if (m_cond_pick_render) {
        /// ワイヤーフレームのときピック除外する場合
        if (m_pick_wireframe_invalid) {
            if (!voxel_draw_shading) {    /// シェーディング表示なければ
                return true;
            }
            else if (voxel->transparent() != 1.0f) {    /// 透明も除外
                return true;
            }
        }

        int object_id = m_stack_pick_objects.size();
        m_stack_pick_objects.emplace_back(node, voxel);

        Point3f object_id_color((object_id & 0xFF) / 255.0f,           // R成分
                                ((object_id >> 8) & 0xFF) / 255.0f,    // G成分
                                ((object_id >> 16) & 0xFF) / 255.0f    // B成分
        );

        m_cur_shader_program->setUniformValue("objectIDColor", object_id_color);
    }

    // 関数内関数（ラムダ式）
    auto getMaxDepth = [=](float fov) -> float {
        float baseFovRad = M_PI / 4.0f;    // 45度
        float base_depth = 0.999995f;      // 45度での値

        float maxDepth = (base_depth - 1.0f) / baseFovRad * fov + 1.0f;

        if (maxDepth > 0.999999f) maxDepth = 0.999999f;
        if (maxDepth < 0.99999f) maxDepth = 0.99999f;

        return maxDepth;
    };

    if (render_data->m_use_vbo) {
        render_data->m_vao.bind();

        /// 透明でないときは先
        if (!m_cond_transparent) {
            /// 暫定 - 法線データ削除による高速化で、シェーダーがかなりぐちゃぐちゃ。。。整理したい
            auto current_shader = m_cur_shader_program;
            if (!m_cond_pick_render) {
                current_shader->release();
                m_cur_shader_program = m_paint_shader_no_norms_program;
                m_cur_shader_program->bind();
                applyMatrix();
            }

            /// Shading表示のとき または ピック描画のとき
            if (voxel_draw_shading || m_cond_pick_render) {
                int texture_draw = -1;
                if (!m_cond_pick_render) {
                    const Point4f& color = voxel->color();
                    m_cur_shader_program->setUniformValue("color", color[0], color[1], color[2]);

                    /// テクスチャ描画の場合
                    if (auto project_node = voxel->projectionOpt()) {
                        texture_draw = set3DTextureShader(project_node->object<VoxelScalar>());
                    }

                    m_cur_shader_program->setUniformValue("transparency", color[3]);
                }
                m_gl_function->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);

                if (texture_draw >= 0) {
                    reset3DTextureShader(texture_draw);
                }
            }
            /// Wireフレーム表示のみで、3D結果投影がある場合
            else if (!voxel_draw_shading && voxel_draw_wireframe && !m_cond_pick_render) {
                if (auto project_node = voxel->projectionOpt()) {
                    auto texture_draw = set3DTextureShader(project_node->object<VoxelScalar>());
                    m_cur_shader_program->setUniformValue("discard_out_range", true);
                    m_gl_function->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
                    m_cur_shader_program->setUniformValue("discard_out_range", false);
                    reset3DTextureShader(texture_draw);
                }
            }

            if (current_shader != m_cur_shader_program) {
                m_cur_shader_program->release();
                m_cur_shader_program = current_shader;
                m_cur_shader_program->bind();
            }
        }

        /// Wireframe表示のとき かつ ピック描画ではないとき
        if (voxel_draw_wireframe && !m_cond_pick_render) {
            const auto& seg_indices =
                voxel->isEnableEditDisplayData() ? voxel->displayEditSegmentIndices() : voxel->displaySegmentIndices();
            if (seg_indices.size() > 0) {
                auto current_shader = m_cur_shader_program;
                if (!m_cond_pick_render) {
                    current_shader->release();
                    m_cur_shader_program = m_paint_segment_program;
                    m_cur_shader_program->bind();
                    applyMatrix();
                }

                bool gl_multisample_enabled = glIsEnabled(GL_MULTISAMPLE) ? true : false;
                bool gl_line_smooth_enabled = glIsEnabled(GL_LINE_SMOOTH) ? true : false;
                if (m_line_smooth) {
                    if (!gl_line_smooth_enabled) {
                        glEnable(GL_LINE_SMOOTH);
                    }
                    if (!gl_multisample_enabled) {
                        glEnable(GL_MULTISAMPLE);
                    }
                }
                else {
                    if (gl_line_smooth_enabled) {
                        glDisable(GL_LINE_SMOOTH);
                    }
                    if (gl_multisample_enabled) {
                        glDisable(GL_MULTISAMPLE);
                    }
                }

                glLineWidth(m_voxel_draw_wireframe_width * m_dpi_scale);

                m_cur_shader_program->setUniformValue("line_offset_value_front", -0.0002f);
                m_cur_shader_program->setUniformValue("line_offset_value_back", -0.0002f);

                if (m_voxel_draw_wireframe_color_shape) {
                    /// 形状色
                    const Point4f& color = voxel->color();
                    m_cur_shader_program->setUniformValue("color", color[0], color[1], color[2]);
                }
                else {
                    /// 指定色
                    m_cur_shader_program->setUniformValue("color", m_voxel_draw_wireframe_color[0],
                                                          m_voxel_draw_wireframe_color[1],
                                                          m_voxel_draw_wireframe_color[2]);
                }

                m_cur_shader_program->setUniformValue("transparency", 1.0f);
                m_cur_shader_program->setUniformValue("disable_light", true);
                /// 深度範囲を圧縮してワイヤーフレームが必ず面メッシュより手前に描画されるようにする
                glDepthRange(0.0, 0.9999);
                m_gl_function->glDrawElements(GL_LINES, seg_indices.size(), GL_UNSIGNED_INT,
                                              (void*)(indices.size() * sizeof(GLuint)));
                glDepthRange(0.0, 1.0);
                m_cur_shader_program->setUniformValue("disable_light", false);

                if (m_line_smooth) {
                    if (!gl_line_smooth_enabled) {
                        glDisable(GL_LINE_SMOOTH);
                    }
                    if (!gl_multisample_enabled) {
                        glDisable(GL_MULTISAMPLE);
                    }
                }
                else {
                    if (gl_line_smooth_enabled) {
                        glEnable(GL_LINE_SMOOTH);
                    }
                    if (gl_multisample_enabled) {
                        glEnable(GL_MULTISAMPLE);
                    }
                }

                if (current_shader != m_cur_shader_program) {
                    m_cur_shader_program->release();
                    m_cur_shader_program = current_shader;
                    m_cur_shader_program->bind();
                }
            }
        }

        /// 透明のときは後
        if (m_cond_transparent) {
            /// 暫定 - 法線データ削除による高速化で、シェーダーがかなりぐちゃぐちゃ。。。整理したい
            auto current_shader = m_cur_shader_program;
            if (!m_cond_pick_render) {
                current_shader->release();
                m_cur_shader_program = m_paint_shader_no_norms_program;
                m_cur_shader_program->bind();
                applyMatrix();
            }

            /// Shading表示のとき または ピック描画のとき
            if (voxel_draw_shading || m_cond_pick_render) {
                int texture_draw = -1;
                if (!m_cond_pick_render) {
                    const Point4f& color = voxel->color();
                    m_cur_shader_program->setUniformValue("color", color[0], color[1], color[2]);

                    /// テクスチャ描画の場合
                    if (auto project_node = voxel->projectionOpt()) {
                        texture_draw = set3DTextureShader(project_node->object<VoxelScalar>());
                    }

                    m_cur_shader_program->setUniformValue("transparency", color[3]);
                }
                m_gl_function->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);

                if (texture_draw >= 0) {
                    reset3DTextureShader(texture_draw);
                }
            }
            /// Wireフレーム表示のみで、3D結果投影がある場合
            else if (!voxel_draw_shading && voxel_draw_wireframe && !m_cond_pick_render) {
                if (auto project_node = voxel->projectionOpt()) {
                    auto           texture_draw = set3DTextureShader(project_node->object<VoxelScalar>());
                    const Point4f& color        = voxel->color();
                    m_cur_shader_program->setUniformValue("transparency", color[3]);
                    m_cur_shader_program->setUniformValue("discard_out_range", true);
                    m_gl_function->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
                    m_cur_shader_program->setUniformValue("discard_out_range", false);
                    reset3DTextureShader(texture_draw);
                }
            }

            if (current_shader != m_cur_shader_program) {
                m_cur_shader_program->release();
                m_cur_shader_program = current_shader;
                m_cur_shader_program->bind();
            }
        }

        render_data->m_vao.release();
    }
    else {
        GLfloat dummyNormal[3] = {0.0f, 0.0f, 1.0f};

        /// 透明でないときは先
        if (!m_cond_transparent) {
            /// 暫定 - 法線データ削除による高速化で、シェーダーがかなりぐちゃぐちゃ。。。整理したい
            auto current_shader = m_cur_shader_program;
            if (!m_cond_pick_render) {
                current_shader->release();
                m_cur_shader_program = m_paint_shader_no_norms_program;
                m_cur_shader_program->bind();
                applyMatrix();
            }
            m_cur_shader_program->enableAttributeArray(0);
            m_cur_shader_program->setAttributeArray(0, GL_FLOAT, &vertices[0], 3, sizeof(Point3f));
            m_cur_shader_program->enableAttributeArray(1);
            m_cur_shader_program->setAttributeArray(1, GL_FLOAT, dummyNormal, 3, 0);

            /// Shading表示のとき または ピック描画のとき
            if (voxel_draw_shading || m_cond_pick_render) {
                int texture_draw = -1;
                if (!m_cond_pick_render) {
                    const Point4f& color = voxel->color();
                    m_cur_shader_program->setUniformValue("color", color[0], color[1], color[2]);

                    /// テクスチャ描画の場合
                    if (auto project_node = voxel->projectionOpt()) {
                        texture_draw = set3DTextureShader(project_node->object<VoxelScalar>());
                    }

                    m_cur_shader_program->setUniformValue("transparency", color[3]);
                }
                m_gl_function->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, indices.data());

                if (texture_draw >= 0) {
                    reset3DTextureShader(texture_draw);
                }
            }
            /// Wireフレーム表示のみで、3D結果投影がある場合
            else if (!voxel_draw_shading && voxel_draw_wireframe && !m_cond_pick_render) {
                if (auto project_node = voxel->projectionOpt()) {
                    auto texture_draw = set3DTextureShader(project_node->object<VoxelScalar>());
                    m_cur_shader_program->setUniformValue("discard_out_range", true);
                    m_gl_function->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, indices.data());
                    m_cur_shader_program->setUniformValue("discard_out_range", false);
                    reset3DTextureShader(texture_draw);
                }
            }

            m_cur_shader_program->disableAttributeArray(0);
            m_cur_shader_program->disableAttributeArray(1);

            if (current_shader != m_cur_shader_program) {
                m_cur_shader_program->release();
                m_cur_shader_program = current_shader;
                m_cur_shader_program->bind();
            }
        }

        /// Wireframe表示のとき かつ ピック描画ではないとき
        if (voxel_draw_wireframe && !m_cond_pick_render) {
            /// Edge表示
            const auto& seg_indices =
                voxel->isEnableEditDisplayData() ? voxel->displayEditSegmentIndices() : voxel->displaySegmentIndices();
            if (seg_indices.size() > 0) {
                auto current_shader = m_cur_shader_program;
                if (!m_cond_pick_render) {
                    current_shader->release();
                    m_cur_shader_program = m_paint_segment_program;
                    m_cur_shader_program->bind();
                    applyMatrix();
                }
                m_cur_shader_program->enableAttributeArray(0);
                m_cur_shader_program->setAttributeArray(0, GL_FLOAT, &vertices[0], 3, sizeof(Point3f));
                m_cur_shader_program->enableAttributeArray(1);
                m_cur_shader_program->setAttributeArray(1, GL_FLOAT, dummyNormal, 3, 0);

                m_cur_shader_program->setUniformValue("line_offset_value_front", -0.0002f);
                m_cur_shader_program->setUniformValue("line_offset_value_back", -0.0002f);

                bool gl_multisample_enabled = glIsEnabled(GL_MULTISAMPLE) ? true : false;
                bool gl_line_smooth_enabled = glIsEnabled(GL_LINE_SMOOTH) ? true : false;
                if (m_line_smooth) {
                    if (!gl_line_smooth_enabled) {
                        glEnable(GL_LINE_SMOOTH);
                    }
                    if (!gl_multisample_enabled) {
                        glEnable(GL_MULTISAMPLE);
                    }
                }
                else {
                    if (gl_line_smooth_enabled) {
                        glDisable(GL_LINE_SMOOTH);
                    }
                    if (gl_multisample_enabled) {
                        glDisable(GL_MULTISAMPLE);
                    }
                }
                glLineWidth(m_voxel_draw_wireframe_width * m_dpi_scale);

                if (m_voxel_draw_wireframe_color_shape) {
                    /// 形状色
                    const Point4f& color = voxel->color();
                    m_cur_shader_program->setUniformValue("color", color[0], color[1], color[2]);
                }
                else {
                    /// 指定色
                    m_cur_shader_program->setUniformValue("color", m_voxel_draw_wireframe_color[0],
                                                          m_voxel_draw_wireframe_color[1],
                                                          m_voxel_draw_wireframe_color[2]);
                }

                m_cur_shader_program->setUniformValue("transparency", 1.0f);
                m_cur_shader_program->setUniformValue("disable_light", true);
                /// 深度範囲を圧縮してワイヤーフレームが必ず面メッシュより手前に描画されるようにする
                glDepthRange(0.0, 0.9999);
                m_gl_function->glDrawElements(GL_LINES, seg_indices.size(), GL_UNSIGNED_INT, seg_indices.data());
                glDepthRange(0.0, 1.0);
                m_cur_shader_program->setUniformValue("disable_light", false);

                m_cur_shader_program->disableAttributeArray(0);
                m_cur_shader_program->disableAttributeArray(1);

                if (m_line_smooth) {
                    if (!gl_line_smooth_enabled) {
                        glDisable(GL_LINE_SMOOTH);
                    }
                    if (!gl_multisample_enabled) {
                        glDisable(GL_MULTISAMPLE);
                    }
                }
                else {
                    if (gl_line_smooth_enabled) {
                        glEnable(GL_LINE_SMOOTH);
                    }
                    if (gl_multisample_enabled) {
                        glEnable(GL_MULTISAMPLE);
                    }
                }

                if (current_shader != m_cur_shader_program) {
                    m_cur_shader_program->release();
                    m_cur_shader_program = current_shader;
                    m_cur_shader_program->bind();
                }
            }
        }

        /// 透明のときは後
        if (m_cond_transparent) {
            /// 暫定 - 法線データ削除による高速化で、シェーダーがかなりぐちゃぐちゃ。。。整理したい
            auto current_shader = m_cur_shader_program;
            if (!m_cond_pick_render) {
                current_shader->release();
                m_cur_shader_program = m_paint_shader_no_norms_program;
                m_cur_shader_program->bind();
                applyMatrix();
            }

            m_cur_shader_program->enableAttributeArray(0);
            m_cur_shader_program->setAttributeArray(0, GL_FLOAT, &vertices[0], 3, sizeof(Point3f));
            m_cur_shader_program->enableAttributeArray(1);
            m_cur_shader_program->setAttributeArray(1, GL_FLOAT, dummyNormal, 3, 0);

            /// Shading表示のとき または ピック描画のとき
            if (voxel_draw_shading || m_cond_pick_render) {
                int texture_draw = -1;
                if (!m_cond_pick_render) {
                    const Point4f& color = voxel->color();
                    m_cur_shader_program->setUniformValue("color", color[0], color[1], color[2]);

                    /// テクスチャ描画の場合
                    if (auto project_node = voxel->projectionOpt()) {
                        texture_draw = set3DTextureShader(project_node->object<VoxelScalar>());
                    }

                    m_cur_shader_program->setUniformValue("transparency", color[3]);
                }
                m_gl_function->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, indices.data());

                if (texture_draw >= 0) {
                    reset3DTextureShader(texture_draw);
                }
            }
            /// Wireフレーム表示のみで、3D結果投影がある場合
            else if (!voxel_draw_shading && voxel_draw_wireframe && !m_cond_pick_render) {
                if (auto project_node = voxel->projectionOpt()) {
                    auto           texture_draw = set3DTextureShader(project_node->object<VoxelScalar>());
                    const Point4f& color        = voxel->color();
                    m_cur_shader_program->setUniformValue("transparency", color[3]);
                    m_cur_shader_program->setUniformValue("discard_out_range", true);
                    m_gl_function->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, indices.data());
                    m_cur_shader_program->setUniformValue("discard_out_range", false);
                    reset3DTextureShader(texture_draw);
                }
            }

            m_cur_shader_program->disableAttributeArray(0);
            m_cur_shader_program->disableAttributeArray(1);

            if (current_shader != m_cur_shader_program) {
                m_cur_shader_program->release();
                m_cur_shader_program = current_shader;
                m_cur_shader_program->bind();
            }
        }
    }

    if (m_cond_pick_render) {
        /* /// 遅いので線分データ（グループのボックス判定）を通す
        if (m_cond_snap & RenderSnap::SnapVertex) {
            const Matrix4x4f& matrix = curPathMatrix();

            for (const auto& vertex : vertices) {
                if (m_scene_view->isPointInFrustum(vertex.m_position, matrix)) {
                    int object_id = m_stack_pick_objects.size();

                    auto pick_vertex = PickVertex::createObject();
                    pick_vertex->setVertex(vertex);
                    m_stack_pick_objects.emplace_back(node, voxel, pick_vertex.ptr());

                    Point3f object_id_color((object_id & 0xFF) / 255.0f,           // R成分
                                            ((object_id >> 8) & 0xFF) / 255.0f,    // G成分
                                            ((object_id >> 16) & 0xFF) / 255.0f    // B成分
                    );

                    m_cur_shader_program->setUniformValue("objectIDColor", object_id_color);

                    glPointSize((float)m_pick_pixel * m_dpi_scale);
                    glBegin(GL_POINTS);
                    glVertex3f(vertex.m_position.x(), vertex.m_position.y(), vertex.m_position.z());
                    glEnd();
                }
            }
        }
        */
        if (m_cond_snap & RenderSnap::SnapEdge || m_cond_snap & RenderSnap::SnapVertex) {
            const Matrix4x4f& matrix = curPathMatrix();

            /// 元々線分が多かったのでグループ分けしていたが、だいぶ減りはした（重複をすべて消した）。とりあえずこの仕組みは残しておく
            voxel->createDisplaySegmentsGroupData();
            const auto& seg_indices =
                voxel->isEnableEditDisplayData() ? voxel->displayEditSegmentIndices() : voxel->displaySegmentIndices();

            const Matrix4x4f inv_matrix = matrix.inverted();

            auto frustum = m_scene_view->frustum();
            frustum.preMulti(inv_matrix);

            // int         group_num   = voxel->displaySegmentsGroupCount();
            const auto& group_boxes = voxel->displaySegmentsGroupBox();
            const auto& group_start = voxel->displaySegmentsGroupStart();

            for (int group_index = 0; group_index < group_start.size(); ++group_index) {
                if (!frustum.isBoxInFrustum(group_boxes[group_index])) {
                    continue;
                }

                int start_index = group_start[group_index];
                int end_index;
                if (group_index < group_start.size() - 1) {
                    end_index = group_start[group_index + 1];
                }
                else {
                    end_index = seg_indices.size();
                }

                // グループ内のセグメントを処理
                for (int ic = start_index; ic < end_index; ic += 2) {
                    const auto&   seg_point_0 = vertices[seg_indices[ic]];
                    const auto&   seg_point_1 = vertices[seg_indices[ic + 1]];
                    BoundingBox3f seg_box(seg_point_0, seg_point_1);
                    if (!frustum.isBoxInFrustum(seg_box)) {
                        continue;
                    }

                    if (m_cond_snap & RenderSnap::SnapVertex) {
                        if (m_scene_view->isPointInFrustum(seg_point_0, matrix)) {
                            int object_id = m_stack_pick_objects.size();

                            auto pick_vertex = PickVertex::createObject();
                            pick_vertex->setVertex(seg_point_0);
                            m_stack_pick_objects.emplace_back(node, voxel, pick_vertex.ptr());

                            Point3f object_id_color((object_id & 0xFF) / 255.0f,           // R成分
                                                    ((object_id >> 8) & 0xFF) / 255.0f,    // G成分
                                                    ((object_id >> 16) & 0xFF) / 255.0f    // B成分
                            );

                            m_cur_shader_program->setUniformValue("objectIDColor", object_id_color);

                            glPointSize((float)m_pick_pixel);
                            glBegin(GL_POINTS);
                            glVertex3f(seg_point_0.x(), seg_point_0.y(), seg_point_0.z());
                            glEnd();
                        }
                        if (m_scene_view->isPointInFrustum(seg_point_1, matrix)) {
                            int object_id = m_stack_pick_objects.size();

                            auto pick_vertex = PickVertex::createObject();
                            pick_vertex->setVertex(seg_point_1);
                            m_stack_pick_objects.emplace_back(node, voxel, pick_vertex.ptr());

                            Point3f object_id_color((object_id & 0xFF) / 255.0f,           // R成分
                                                    ((object_id >> 8) & 0xFF) / 255.0f,    // G成分
                                                    ((object_id >> 16) & 0xFF) / 255.0f    // B成分
                            );

                            m_cur_shader_program->setUniformValue("objectIDColor", object_id_color);

                            glPointSize((float)m_pick_pixel);
                            glBegin(GL_POINTS);
                            glVertex3f(seg_point_1.x(), seg_point_1.y(), seg_point_1.z());
                            glEnd();
                        }
                    }

                    int object_id = m_stack_pick_objects.size();

                    auto pick_edge = PickEdge::createObject();
                    pick_edge->setStartVertex(seg_point_0);
                    pick_edge->setEndVertex(seg_point_1);
                    m_stack_pick_objects.emplace_back(node, voxel, pick_edge.ptr());

                    Point3f object_id_color((object_id & 0xFF) / 255.0f,           // R成分
                                            ((object_id >> 8) & 0xFF) / 255.0f,    // G成分
                                            ((object_id >> 16) & 0xFF) / 255.0f    // B成分
                    );

                    m_cur_shader_program->setUniformValue("objectIDColor", object_id_color);

                    glLineWidth(2 * m_dpi_scale);
                    glBegin(GL_LINES);
                    glVertex3f(seg_point_0.x(), seg_point_0.y(), seg_point_0.z());
                    glVertex3f(seg_point_1.x(), seg_point_1.y(), seg_point_1.z());
                    glEnd();
                }
            }
        }
    }

    return true;
}

Point4f getColor(float value, const float divisions[], const Point4f colors[])
{
    // 二分探索による範囲判定
    int low  = 0;
    int high = 9;    // 配列の最後のインデックス
    while (low <= high) {
        int mid = (low + high) / 2;    // 中央のインデックスを計算
        if (value > divisions[mid]) {
            high = mid - 1;    // 左側を探索
        }
        else {
            low = mid + 1;    // 右側を探索
        }
    }
    if (low > 9) return Point4f(1, 1, 1, 1);
    if (low < 0) return Point4f(0, 0, 0, 1);

    // `low` が属する範囲のインデックス
    return colors[low];

    /*
    // 10段階の範囲を計算
    float range = (maxValue - minValue) / 10.0;

    // 各範囲に対応する色を設定
    if (value < minValue + range * 1.0) return vec4(1.0, 0.0, 0.0, 1.0);  // 赤
    else if (value < minValue + range * 2.0) return vec4(1.0, 0.5, 0.0, 1.0);  // オレンジ
    else if (value < minValue + range * 3.0) return vec4(1.0, 1.0, 0.0, 1.0);  // 黄色
    else if (value < minValue + range * 4.0) return vec4(0.5, 1.0, 0.0, 1.0);  // 黄緑
    else if (value < minValue + range * 5.0) return vec4(0.0, 1.0, 0.0, 1.0);  // 緑
    else if (value < minValue + range * 6.0) return vec4(0.0, 1.0, 0.5, 1.0);  // 青緑
    else if (value < minValue + range * 7.0) return vec4(0.0, 1.0, 1.0, 1.0);  // 水色
    else if (value < minValue + range * 8.0) return vec4(0.0, 0.5, 1.0, 1.0);  // 青
    else if (value < minValue + range * 9.0) return vec4(0.0, 0.0, 1.0, 1.0);  // 濃い青
    else return vec4(0.5, 0.0, 1.0, 1.0);  // 紫
    */
}

bool GLSceneRender::renderVoxelScalar(Node* node, VoxelScalar* voxel)
{
    /// Text描画なし
    if (m_cond_text_render) {
        return true;
    }
    if (m_cond_drag) {
        return true;
    }

    RenderData* render_data = (RenderData*)voxel->renderData();
    if (!render_data) {
        return true;
    }
    const auto& vertices = voxel->isEnableEditDisplayData() ? voxel->displayEditVertices() : voxel->displayVertices();
    const auto& indices  = voxel->isEnableEditDisplayData() ? voxel->displayEditIndices() : voxel->displayIndices();
    if (vertices.empty() || indices.empty()) {
        return true;
    }

    int                texture_draw        = -1;
    RenderTextureData* render_texture_data = (RenderTextureData*)voxel->renderTextureData();

    /// 暫定 - 法線データ削除による高速化で、シェーダーがかなりぐちゃぐちゃ。。。整理したい
    auto current_shader = m_cur_shader_program;
    if (!m_cond_pick_render) {
        current_shader->release();
        m_cur_shader_program = m_paint_shader_no_norms_program;
        m_cur_shader_program->bind();
        applyMatrix();
    }

    if (m_cond_pick_render) {
        int object_id = m_stack_pick_objects.size();
        m_stack_pick_objects.emplace_back(node, voxel);

        Point3f object_id_color((object_id & 0xFF) / 255.0f,           // R成分
                                ((object_id >> 8) & 0xFF) / 255.0f,    // G成分
                                ((object_id >> 16) & 0xFF) / 255.0f    // B成分
        );

        m_cur_shader_program->setUniformValue("objectIDColor", object_id_color);
    }
    else {
        /// テクスチャ描画の場合
        if (render_texture_data) {
            texture_draw = set3DTextureShader(voxel);
        }
        else {
            const Point4f& color = voxel->color();

            m_cur_shader_program->setUniformValue("color", color[0], color[1], color[2]);
            m_cur_shader_program->setUniformValue("transparency", color[3]);
        }
    }

    bool voxel_scalar_priority = m_voxel_scalar_priority;
    if (m_cond_pick_render) {
        if (m_pick_voxel_scalar_priority_invalid) {
            voxel_scalar_priority = false;
        }
    }

    m_gl_function->glEnable(GL_POLYGON_OFFSET_FILL);
    if (voxel_scalar_priority && (!voxel->is2DTexture() || (voxel->is2DTexture() && is2DTexturePriority()))) {
        m_cur_shader_program->setUniformValue("direct_offset", true);
        m_cur_shader_program->setUniformValue("direct_offset_value", -0.0001f);
        m_gl_function->glPolygonOffset(-0.05f, -0.05f);
    }
    else {
        m_cur_shader_program->setUniformValue("direct_offset", true);
        m_cur_shader_program->setUniformValue("direct_offset_value", 0.0001f);
        m_gl_function->glPolygonOffset(0.05f, 0.05f);
    }

    /// 特殊（自身でテクスチャ持たず投影する場合）
    if (!m_cond_pick_render) {
        if (texture_draw < 0) {
            if (auto project_node = voxel->projectionOpt()) {
                texture_draw = set3DTextureShader(project_node->object<VoxelScalar>());
                m_cur_shader_program->setUniformValue("rangeCount", 0);    /// 自身で描画するときは無効にする
            }
        }
    }

    if (render_data->m_use_vbo) {
        render_data->m_vao.bind();
        m_gl_function->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
        render_data->m_vao.release();
    }
    else {
        m_cur_shader_program->enableAttributeArray(0);
        m_cur_shader_program->setAttributeArray(0, GL_FLOAT, &vertices[0], 3, sizeof(Point3f));

        GLfloat dummyNormal[3] = {0.0f, 0.0f, 1.0f};
        m_cur_shader_program->enableAttributeArray(1);
        m_cur_shader_program->setAttributeArray(1, GL_FLOAT, dummyNormal, 3, 0);

        m_gl_function->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, indices.data());

        m_cur_shader_program->disableAttributeArray(0);
        m_cur_shader_program->disableAttributeArray(1);
    }

    if (texture_draw >= 0) {
        reset3DTextureShader(texture_draw);
    }

    m_gl_function->glDisable(GL_POLYGON_OFFSET_FILL);
    m_cur_shader_program->setUniformValue("direct_offset", false);
    m_cur_shader_program->setUniformValue("direct_offset_value", 0.0f);

    if (current_shader != m_cur_shader_program) {
        current_shader->release();
        m_cur_shader_program = current_shader;
        m_cur_shader_program->bind();
    }
    //// 検証コード
    /*
    glFlush();
    {
        std::vector<GLubyte> pixels(m_scene_view->viewPortWidth() * m_scene_view->viewPortHeight()
                                    * 4);    /// RGBAの4成分
        glReadPixels(0, 0, m_scene_view->viewPortWidth(), m_scene_view->viewPortHeight(), GL_RGBA, GL_UNSIGNED_BYTE,
                     pixels.data());
        /// デプスバッファ読み取り
        std::vector<GLfloat> depth_buffer(m_scene_view->viewPortWidth() * m_scene_view->viewPortHeight());
        glReadPixels(0, 0, m_scene_view->viewPortWidth(), m_scene_view->viewPortHeight(), GL_DEPTH_COMPONENT,
    GL_FLOAT, depth_buffer.data());

        int Nx = voxel->nX();
        int Ny = voxel->nY();
        int Nz = voxel->nZ();
        for (int y = 0; y < m_scene_view->viewPortHeight(); ++y) {
            for (int x = 0; x < m_scene_view->viewPortWidth(); ++x) {
                int index = (y * m_scene_view->viewPortWidth() + x);
                if (depth_buffer[index] != 1.0f) {
                    int     color_index = index * 4;
                    GLubyte red         = pixels[color_index];
                    GLubyte green       = pixels[color_index + 1];
                    GLubyte blue        = pixels[color_index + 2];

                    bool valid_data = false;
                    for (int ix = -10; ix <= 10; ++ix) {
                        for (int iy = -10; iy <= 10; ++iy) {
                            Point3f obj = m_scene_view->unprojectFromScreen(Point3f(
                                x + (float)ix / 10.0f, m_scene_view->viewPortHeight() - 1 - y + (float)iy / 10.0f,
                                depth_buffer[index]));
                            {
                                std::vector<Picking::SIntersectionInfo> intersections;
                                Picking                                 picking(m_scene_view.ptr());
                                picking.setOnlyMinimum(true);
                                picking.bodyLineIntersection(obj, m_scene_view->cameraDir(), true,
    intersections,0.0); if (intersections.size()) { obj = m_scene_view->cameraDir() *
    intersections[0].m_dist_line_dir + obj;
                                }
                            }

                            Point3i obj_index = voxel->index(obj.x(), obj.y(), obj.z());
                            if (obj_index.x() == -1) obj_index.setX(0);
                            if (obj_index.y() == -1) obj_index.setY(0);
                            if (obj_index.z() == -1) obj_index.setZ(0);
                            if (obj_index.x() == Nx) obj_index.setX(Nx - 1);
                            if (obj_index.y() == Ny) obj_index.setY(Ny - 1);
                            if (obj_index.z() == Nz) obj_index.setZ(Nz - 1);

                            if (obj_index.x() >= 0 && obj_index.x() < Nx && obj_index.y() >= 0 && obj_index.y() < Ny
                                && obj_index.z() >= 0 && obj_index.z() < Nz) {
                                size_t ind = (size_t)(obj_index.x() * (size_t)(Ny * Nz) + (size_t)obj_index.y() * Nz
                                                      + (size_t)obj_index.z());

                                const float* data  = voxel->scalarData();
                                Point4f      color = getColor(data[ind], m_division, m_tex_color);

                                if ((float)red / 255.0f == color[0] && (float)green / 255.0f == color[1]
                                    && (float)blue / 255.0f == color[2]) {
                                    pixels[color_index]     = 0xFF;
                                    pixels[color_index + 1] = 0xFF;
                                    pixels[color_index + 2] = 0xFF;
                                    valid_data              = true;
                                    break;
                                }
                                else {
                                }
                            }
                            else {
                                // DEBUG() << "NG " << obj_index.x() << " " << obj_index.y() << " " <<
    obj_index.z();
                            }
                        }
                        if (valid_data) {
                            break;
                        }
                    }
                    if (!valid_data) {
                        DEBUG() << "NG";
                    }
                }
            }
        }

        QImage image(pixels.data(), m_scene_view->viewPortWidth(), m_scene_view->viewPortHeight(),
                     QImage::Format_RGBA8888);
        QImage flipped = image.mirrored();    // 上下反転

        // ファイル保存（PNGなど）
        flipped.save("D:/output.png");
    }*/

    return true;
}

bool GLSceneRender::renderDimension(Node* node, Dimension* dimension, bool multi_dimension, bool multi_dimension_name)
{
    if (!m_cond_text_render && m_cond_pick_render) {
        if (m_cond_snap & RenderSnap::SnapVertex) {
            for (int ic = 0; ic < 2; ++ic) {
                const auto& vertex = ic == 0 ? dimension->posStart() : dimension->posEnd();

                int object_id = m_stack_pick_objects.size();

                auto pick_vertex = PickVertex::createObject();
                pick_vertex->setVertex(vertex);
                m_stack_pick_objects.emplace_back(node, dimension, pick_vertex.ptr());

                Point3f object_id_color((object_id & 0xFF) / 255.0f,           // R成分
                                        ((object_id >> 8) & 0xFF) / 255.0f,    // G成分
                                        ((object_id >> 16) & 0xFF) / 255.0f    // B成分
                );

                m_cur_shader_program->setUniformValue("objectIDColor", object_id_color);

                glPointSize((float)m_pick_pixel);
                glBegin(GL_POINTS);
                glVertex3f(vertex.x(), vertex.y(), vertex.z());
                glEnd();
            }

            // return true;
        }
        // return true;
    }

    if (m_cond_text_render) {
        if (m_cond_pick_render) {
            /// Texture描画がうまくいかないので領域を四角で描画
            int object_id = m_stack_pick_objects.size();
            m_stack_pick_objects.emplace_back(node, dimension);

            Point3f object_id_color((object_id & 0xFF) / 255.0f,           // R成分
                                    ((object_id >> 8) & 0xFF) / 255.0f,    // G成分
                                    ((object_id >> 16) & 0xFF) / 255.0f    // B成分
            );

            m_cur_shader_program->setUniformValue("objectIDColor", object_id_color);

            const TextQuad& text_quad = dimension->textQuad();
            const auto&     pos0      = text_quad.m_pos[0];
            const auto&     pos1      = text_quad.m_pos[1];
            const auto&     pos2      = text_quad.m_pos[2];
            const auto&     pos3      = text_quad.m_pos[3];

            glBegin(GL_QUADS);
            glVertex3f(pos0.x(), pos0.y(), pos0.z());
            glVertex3f(pos1.x(), pos1.y(), pos1.z());
            glVertex3f(pos2.x(), pos2.y(), pos2.z());
            glVertex3f(pos3.x(), pos3.y(), pos3.z());
            glEnd();
        }
        else {
            const Point4f& color = dimension->color();

            m_cur_shader_program->setUniformValue("color", color[0], color[1], color[2]);
            m_cur_shader_program->setUniformValue("transparency", color[3]);

            // 頂点配列を設定
            const auto& path_matrix = curPathMatrix();

            const auto& global_vec_0 = path_matrix.preMult(dimension->strPosStart());
            const auto& mouse_vec_0  = m_scene_view->projectToScreen(global_vec_0);

            bool init_start_pos = dimension->isInitSrtPos();
            if (init_start_pos && !multi_dimension_name) {
                dimension->setTextStartPosAlign(TextAlignment::CenterLeft);
            }
            // auto text_pos = GLRenderText::TextAlignment::CenterLeft;

            auto text_align = dimension->textAlign();

            /// 向き判定
            const auto& global_pos_s = path_matrix.preMult(dimension->posStart());
            const auto& mouse_vec_s  = m_scene_view->projectToScreen(global_pos_s);

            const auto& global_pos_e = path_matrix.preMult(dimension->posEnd());
            const auto& mouse_vec_e  = m_scene_view->projectToScreen(global_pos_e);

            auto edge_type = dimension->edgeType();

            float x_dif = mouse_vec_s.x() - mouse_vec_e.x();
            float y_dif = mouse_vec_s.y() - mouse_vec_e.y();
            if (x_dif != 0.0) {
                // if (fabs(y_dif / x_dif) < 1.0) {    /// 45度
                if (fabs(y_dif / x_dif) < 0.57735) {    /// 30度
                    if (text_align == Dimension::TextAlign::Auto) {
                        text_align = Dimension::TextAlign::Vertical;
                    }
                    if (init_start_pos && text_align != Dimension::TextAlign::Vertical) {
                        if (!multi_dimension) {
                            dimension->setTextStartPosAlign(TextAlignment::BottomCenter);
                        }
                        else {
                            dimension->setTextStartPosAlign(TextAlignment::BottomLeft);
                        }
                    }

                    if (edge_type == Dimension::EdgeType::LineAuto) {
                        x_dif = 1;
                        y_dif = 0;
                    }
                }
                else {
                    if (edge_type == Dimension::EdgeType::LineAuto) {
                        x_dif = 0;
                        y_dif = 1;
                    }
                }
            }

            if (edge_type == Dimension::EdgeType::LineVertical) {
                x_dif = 0;
                y_dif = 1;
            }
            else if (edge_type == Dimension::EdgeType::LineHorizontal) {
                x_dif = 1;
                y_dif = 0;
            }

            auto text_pos = dimension->textStartPosAlign();

            int offset_x = 0;
            int offset_y = 0;
            if (text_align != Dimension::TextAlign::Vertical && text_pos == TextAlignment::CenterLeft) {
                offset_x = 2;
            }
            else {
                offset_y = -2;
            }

            dimension->setTextLastDisplayAlign(text_align == Dimension::TextAlign::Vertical
                                                   ? Dimension::TextAlign::Vertical
                                                   : Dimension::TextAlign::Horizontal);

            glDisable(GL_DEPTH_TEST);

            TextQuad text_quad;
            m_cur_shader_program->release();
            m_render_text->renderText(mouse_vec_0.x() + offset_x, mouse_vec_0.y() + offset_y, mouse_vec_0.z(),
                                      dimension->dispStr(), std::wstring(L"Arial"), color, dimension->strSize(),
                                      text_quad, false, text_align == Dimension::TextAlign::Vertical ? 90.0 : 0.0,
                                      text_pos);
            m_cur_shader_program->bind();

            text_quad.m_pos[0] = m_scene_view->unprojectFromScreen(text_quad.m_pos[0]);
            text_quad.m_pos[1] = m_scene_view->unprojectFromScreen(text_quad.m_pos[1]);
            text_quad.m_pos[2] = m_scene_view->unprojectFromScreen(text_quad.m_pos[2]);
            text_quad.m_pos[3] = m_scene_view->unprojectFromScreen(text_quad.m_pos[3]);
            dimension->setTextQuad(text_quad);
            dimension->markBoxDirty();
            node->markBoxDirty();

            glEnable(GL_DEPTH_TEST);

            /// デプスゼロの描画もこっちで行う
            {
                // シェーダープログラムの設定
                m_cur_shader_program->setUniformValue("disable_light", true);

                Point3f offset;
                if (m_temporary_draw) {
                    offset =
                        -m_scene_view->cameraDir() * m_scene_view->sceneGraph()->lengthUnit().epsilonValidLength() * 10;
                }
                else {
                    offset =
                        -m_scene_view->cameraDir() * m_scene_view->sceneGraph()->lengthUnit().epsilonValidLength() * 10;
                }

                // 頂点配列を設定
                Point3f vertices[4] = {dimension->posStart() + offset, dimension->posEnd() + offset,
                                       (dimension->posStart() + dimension->posEnd()) / 2.0 + offset,
                                       dimension->strPosStart() + offset};

                m_cur_shader_program->enableAttributeArray(0);
                m_cur_shader_program->setAttributeArray(0, GL_FLOAT, &vertices[0], 3);

                // 線を描画
                // 線の幅を設定
                glLineWidth(dimension->lineWidth() * m_dpi_scale);
                // m_gl_function->glDrawArrays(GL_LINES, 0, 2);

                glDisable(GL_DEPTH_TEST);

                m_cur_shader_program->setUniformValue("dashed_line", true);
                m_cur_shader_program->setUniformValue("dashed_line_pixel", 12.0f * m_dpi_scale);
                m_cur_shader_program->setUniformValue("line_start", vertices[2].x(), vertices[2].y(), vertices[2].z());
                m_gl_function->glDrawArrays(GL_LINES, 2, 2);
                m_cur_shader_program->setUniformValue("dashed_line", false);

                m_cur_shader_program->setUniformValue("point_display", true);

                if (dimension->edgeType() == Dimension::EdgeType::Circle) {    /// Circle
                    // 点のサイズを設定
                    m_cur_shader_program->setUniformValue("point_display_line", false);
                    m_cur_shader_program->setUniformValue("point_size", (float)(8.0f * m_dpi_scale));
                }
                else {    /// LINE
                    // 線のサイズを設定
                    m_cur_shader_program->setUniformValue("point_display_line", true);
                    m_cur_shader_program->setUniformValue("point_size", (float)(12.0f * m_dpi_scale));
                    m_cur_shader_program->setUniformValue(
                        "point_display_line_width",
                        (float)(std::min(dimension->lineWidth(), 2.0f) / 12.0f));    /// 1/point_sizeが1px
                    Point2f line_dir(x_dif, y_dif);
                    line_dir.normalize();
                    m_cur_shader_program->setUniformValue("point_display_line_dir", line_dir[0], line_dir[1]);
                }

                glEnable(GL_PROGRAM_POINT_SIZE);    // 必須設定
                glEnable(GL_POINT_SPRITE);

                if (dimension->startEdgeDisp() && dimension->endEdgeDisp()) {
                    m_gl_function->glDrawArrays(GL_POINTS, 0, 2);
                }
                else if (dimension->startEdgeDisp()) {
                    m_gl_function->glDrawArrays(GL_POINTS, 0, 1);
                }
                else if (dimension->endEdgeDisp()) {
                    m_gl_function->glDrawArrays(GL_POINTS, 1, 1);
                }

                glDisable(GL_PROGRAM_POINT_SIZE);

                // シェーダーの後処理
                m_cur_shader_program->disableAttributeArray(0);

                m_cur_shader_program->setUniformValue("point_display", false);

                m_cur_shader_program->setUniformValue("disable_light", false);
                glEnable(GL_DEPTH_TEST);
            }
        }
    }
    else {
        // シェーダープログラムの設定
        if (m_cond_pick_render) {
            int object_id = m_stack_pick_objects.size();
            m_stack_pick_objects.emplace_back(node, dimension);

            Point3f object_id_color((object_id & 0xFF) / 255.0f,           // R成分
                                    ((object_id >> 8) & 0xFF) / 255.0f,    // G成分
                                    ((object_id >> 16) & 0xFF) / 255.0f    // B成分
            );

            m_cur_shader_program->setUniformValue("objectIDColor", object_id_color);
        }
        else {
            const Point4f& color = dimension->color();

            m_cur_shader_program->setUniformValue("color", color[0], color[1], color[2]);
            m_cur_shader_program->setUniformValue("transparency", color[3]);

            m_cur_shader_program->setUniformValue("disable_light", true);
        }

        Point3f offset;
        if (m_temporary_draw) {
            offset = -m_scene_view->cameraDir() * m_scene_view->sceneGraph()->lengthUnit().epsilonValidLength() * 10;
        }
        else {
            offset = -m_scene_view->cameraDir() * m_scene_view->sceneGraph()->lengthUnit().epsilonValidLength() * 10;
        }

        // 頂点配列を設定
        Point3f vertices[4] = {dimension->posStart() + offset, dimension->posEnd() + offset,
                               (dimension->posStart() + dimension->posEnd()) / 2.0 + offset,
                               dimension->strPosStart() + offset};

        m_cur_shader_program->enableAttributeArray(0);
        m_cur_shader_program->setAttributeArray(0, GL_FLOAT, &vertices[0], 3);

        // 線を描画
        // 線の幅を設定
        glLineWidth(dimension->lineWidth() * m_dpi_scale);
        m_gl_function->glDrawArrays(GL_LINES, 0, 2);

        // ピックで必要なら
        if (m_cond_pick_render) {
            m_cur_shader_program->setUniformValue("dashed_line", true);
            m_cur_shader_program->setUniformValue("dashed_line_pixel", 12.0f * m_dpi_scale);
            m_cur_shader_program->setUniformValue("line_start", vertices[2].x(), vertices[2].y(), vertices[2].z());
            m_gl_function->glDrawArrays(GL_LINES, 2, 2);
            m_cur_shader_program->setUniformValue("dashed_line", false);

            if (!(m_cond_snap & RenderSnap::SnapVertex)) {
                m_cur_shader_program->setUniformValue("point_display", true);

                // 点のサイズを設定
                m_cur_shader_program->setUniformValue("point_size", (float)(8.0f * m_dpi_scale));

                glEnable(GL_PROGRAM_POINT_SIZE);    // 必須設定
                glEnable(GL_POINT_SPRITE);
                m_gl_function->glDrawArrays(GL_POINTS, 0, 2);
                glDisable(GL_PROGRAM_POINT_SIZE);

                m_cur_shader_program->setUniformValue("point_display", false);
            }
        }
        else {
            m_cur_shader_program->setUniformValue("disable_light", false);
        }

        // シェーダーの後処理
        m_cur_shader_program->disableAttributeArray(0);
    }

    return true;
}

bool GLSceneRender::renderMultiDimension(Node* node, MultiDimension* dimension)
{
    /*
    if (m_cond_pick_render) {
        if (m_cond_snap != RenderSnap::SnapNone) {
            return true;
        }
    }*/

    if (m_temporary_draw) {
        const auto& base_points = dimension->basePoints();
        if (base_points.size() == 2) {
            const Point4f& color = dimension->color();

            m_cur_shader_program->setUniformValue("color", color[0], color[1], color[2]);
            m_cur_shader_program->setUniformValue("transparency", color[3]);
            m_cur_shader_program->setUniformValue("disable_light", true);

            Point3f offset =
                -m_scene_view->cameraDir() * m_scene_view->sceneGraph()->lengthUnit().epsilonValidLength() * 100;

            /// ビューのオブジェクトサイズ
            const auto& obj_corner0 = m_scene_view->unprojectFromScreen(Point3f(0, 0, m_scene_view->projNear()));
            const auto& obj_corner1 = m_scene_view->unprojectFromScreen(
                Point3f(m_scene_view->viewPortWidth(), m_scene_view->viewPortHeight(), m_scene_view->projNear()));
            float extend_length = (obj_corner0 - obj_corner1).length();

            // 頂点配列を設定
            Point3f vertices[6] = {
                base_points[0] + offset,
                base_points[1] + offset,
                base_points[0] + offset,
                (base_points[0] - base_points[1]).normalized() * extend_length + base_points[0] + offset,
                base_points[1] + offset,
                (base_points[1] - base_points[0]).normalized() * extend_length + base_points[1] + offset};

            m_cur_shader_program->enableAttributeArray(0);
            m_cur_shader_program->setAttributeArray(0, GL_FLOAT, &vertices[0], 3);

            if (dimension->isOnSectionPlane()) {
                glDisable(GL_DEPTH_TEST);
            }

            // 線を描画
            // 線の幅を設定
            glLineWidth(dimension->lineWidth() * m_dpi_scale);
            m_gl_function->glDrawArrays(GL_LINES, 0, 2);

            if (dimension->isExtendStart() || dimension->isExtendEnd()) {
                m_cur_shader_program->setUniformValue("dashed_line", true);
                m_cur_shader_program->setUniformValue("dashed_line_pixel", 12.0f * m_dpi_scale);
                m_cur_shader_program->setUniformValue("line_start", vertices[2].x(), vertices[2].y(), vertices[2].z());
                if (dimension->isExtendStart()) m_gl_function->glDrawArrays(GL_LINES, 2, 2);
                if (dimension->isExtendEnd()) m_gl_function->glDrawArrays(GL_LINES, 4, 2);
                m_cur_shader_program->setUniformValue("dashed_line", false);
            }

            if (!dimension->isOnSectionPlane()) {
                glDisable(GL_DEPTH_TEST);
            }

            m_cur_shader_program->setUniformValue("point_display", true);

            // 点のサイズを設定
            m_cur_shader_program->setUniformValue("point_size", (float)(8.0f * m_dpi_scale));

            glEnable(GL_PROGRAM_POINT_SIZE);    // 必須設定
            glEnable(GL_POINT_SPRITE);
            m_gl_function->glDrawArrays(GL_POINTS, 0, 2);
            glDisable(GL_PROGRAM_POINT_SIZE);

            // シェーダーの後処理
            m_cur_shader_program->disableAttributeArray(0);

            m_cur_shader_program->setUniformValue("point_display", false);

            m_cur_shader_program->setUniformValue("disable_light", false);
            glEnable(GL_DEPTH_TEST);
        }
        else if (base_points.size() == 1) {
            const Point4f& color = dimension->color();

            const auto& temporary_mode = dimension->temporaryMode();

            m_cur_shader_program->setUniformValue("color", color[0], color[1], color[2]);
            m_cur_shader_program->setUniformValue("transparency", color[3]);
            m_cur_shader_program->setUniformValue("disable_light", true);

            Point3f offset =
                -m_scene_view->cameraDir() * m_scene_view->sceneGraph()->lengthUnit().epsilonValidLength() * 100;

            Point3f point0 = base_points[0];
            Point3f point1;

            switch (temporary_mode) {
                case MultiDimension::TemporaryMode::X:
                    point1 = point0 + Point3f(1, 0, 0);
                    break;
                case MultiDimension::TemporaryMode::Y:
                    point1 = point0 + Point3f(0, 1, 0);
                    break;
                case MultiDimension::TemporaryMode::Z:
                    point1 = point0 + Point3f(0, 0, 1);
                    break;
                case MultiDimension::TemporaryMode::Vertical:
                    point1 = point0 + m_scene_view->cameraUp();
                    break;
                case MultiDimension::TemporaryMode::Horizontal:
                    point1 = point0 + m_scene_view->cameraRight();
                    break;
            }

            // 頂点配列を設定
            Point3f vertices[3] = {point0 + offset, (point0 - point1) * 100 + point0 + offset,
                                   (point1 - point0) * 100 + point0 + offset};

            m_cur_shader_program->enableAttributeArray(0);
            m_cur_shader_program->setAttributeArray(0, GL_FLOAT, &vertices[0], 3);

            if (dimension->isOnSectionPlane()) {
                glDisable(GL_DEPTH_TEST);
            }

            // 線を描画
            // 線の幅を設定
            glLineWidth(dimension->lineWidth() * m_dpi_scale);
            m_gl_function->glDrawArrays(GL_LINES, 1, 2);

            if (!dimension->isOnSectionPlane()) {
                glDisable(GL_DEPTH_TEST);
            }

            m_cur_shader_program->setUniformValue("point_display", true);

            // 点のサイズを設定
            m_cur_shader_program->setUniformValue("point_size", (float)(8.0f * m_dpi_scale));

            glEnable(GL_PROGRAM_POINT_SIZE);    // 必須設定
            glEnable(GL_POINT_SPRITE);
            m_gl_function->glDrawArrays(GL_POINTS, 0, 1);
            glDisable(GL_PROGRAM_POINT_SIZE);

            // シェーダーの後処理
            m_cur_shader_program->disableAttributeArray(0);

            m_cur_shader_program->setUniformValue("point_display", false);

            m_cur_shader_program->setUniformValue("disable_light", false);
            glEnable(GL_DEPTH_TEST);
        }
    }
    else {
        auto& dimensions = dimension->dimensions();
        for (unsigned int ic = 0; ic < dimensions.size(); ++ic) {
            renderDimension(node, dimensions[ic].m_dimension.ptr(), true);
        }

        if (dimensions.size() > 0 && dimension->dispName()) {
            auto name_dimension = dimension->nameDimension();
            if (name_dimension) {
                name_dimension->setTextAlign(dimensions[0].m_dimension->textLastDisplayAlign());
                name_dimension->setTextStartPosAlign(dimensions[0].m_dimension->textStartPosAlign()
                                                             == TextAlignment::CenterLeft
                                                         ? TextAlignment::BottomLeft
                                                         : dimensions[0].m_dimension->textStartPosAlign());
                renderDimension(node, name_dimension, true, true);
            }
        }

        if (node->isBoxDirty()) {
            dimension->markBoxDirty();
        }
    }

    return true;
}

void GLSceneRender::renderXORPoint(const Point3f& point, float point_size)
{
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_XOR);

    glDisable(GL_DEPTH_TEST);
    glPointSize(point_size * m_dpi_scale);
    glBegin(GL_POINTS);
    glVertex3f(point.x(), point.y(), point.z());
    glEnd();
    glEnable(GL_DEPTH_TEST);

    glDisable(GL_COLOR_LOGIC_OP);
}

void GLSceneRender::renderPoint(const Point3f& point, float point_size)
{
    glDisable(GL_DEPTH_TEST);
    glPointSize(point_size * m_dpi_scale);
    glBegin(GL_POINTS);
    glVertex3f(point.x(), point.y(), point.z());
    glEnd();
    glEnable(GL_DEPTH_TEST);
}

void GLSceneRender::drawCoordinateSystem()
{
    if (!m_show_coordinate_system) {
        return;
    }
    /// 固定の定義
    float pixel_size  = 105.0f * m_dpi_scale;    /// 左下特定ピクセル数の領域
    float axis_length = 0.65f;                   /// 領域内のサイズ

    /// 座標系の色データ
    constexpr Point3f axis_color[] = {
        Point3f(1.0f, 0.0f, 0.0f),    // X軸（赤）
        Point3f(0.0f, 1.0f, 0.0f),    // Y軸（緑）
        Point3f(0.0f, 0.0f, 1.0f)     // Z軸（青）
    };

    /// View Area
    float vwidth  = (float)m_scene_view->viewPortWidth();
    float vheight = (float)m_scene_view->viewPortHeight();
    float aspect  = vheight / vwidth;

    float vscale = vwidth / pixel_size;

    /// 座標系の頂点データ
    Point3f axis_org(0.0f, 0.0f, 0.0f);
    Point3f axis_vec[] = {
        Point3f(axis_length, 0.0f, 0.0f),    // X軸
        Point3f(0.0f, axis_length, 0.0f),    // Y軸
        Point3f(0.0f, 0.0f, axis_length)     // Z軸
    };

    const auto& mv_matrix    = m_scene_view->mvMatrix();
    auto        rotateMatrix = mv_matrix.rotateMatrix();
    for (int ic = 0; ic < 3; ++ic) {
        axis_vec[ic] = rotateMatrix * axis_vec[ic];
    }

    /// 文字の描画順を奥行で決定
    struct AxisInfo {
        int   index;
        float depth;    /// Z値
    };
    std::vector<AxisInfo> axisOrder;
    for (int ic = 0; ic < 3; ++ic) {
        axisOrder.push_back({ic, axis_vec[ic].z()});
    }
    /// 手前順に
    std::sort(axisOrder.begin(), axisOrder.end(),
              [](const AxisInfo& a, const AxisInfo& b) { return a.depth < b.depth; });

    /// 描画の初期化
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    /// 画面の左下に領域限定
    glViewport(0, 0, vwidth / vscale, vwidth / vscale);

    /// 座標系の描画
    float dpiScale = m_gl_widget->devicePixelRatio();
    glLineWidth(4.0f * dpiScale);
    glBegin(GL_LINES);
    for (const AxisInfo& ax : axisOrder) {
        int ic = ax.index;
        glColor3f(axis_color[ic].x(), axis_color[ic].y(), axis_color[ic].z());
        glVertex3f(axis_org.x(), axis_org.y(), axis_org.z());
        glVertex3f(axis_org.x() + axis_vec[ic].x(), axis_org.y() + axis_vec[ic].y(), axis_org.z() + axis_vec[ic].z());
    }
    glEnd();

    /// ビューポート戻す
    glViewport(0, 0, vwidth, vheight);

    /// 文字の描画
    TextQuad dummy;
    for (const AxisInfo& ax : axisOrder) {
        int          ic = ax.index;
        std::wstring str;
        Point4f      color;
        switch (ic) {
            case 0:
                color.set(1.0f, 0.0f, 0.0f, 1.0f);
                str = L"X";
                break;
            case 1:
                color.set(0.0f, 1.0f, 0.0f, 1.0f);
                str = L"Y";
                break;
            case 2:
                color.set(0.0f, 0.0f, 1.0f, 1.0f);
                str = L"Z";
                break;
        }

        /// OpenGL座標系が中心0,0, サイズ-1～1（かつview_scaleで左下に縮小)
        /// Qt座標系が左上原点、サイズ(0～widht, 0～height)なので、それに合わせて調整
        QVector3D offset = (axis_vec[ic].normalized()) * 10.0f;    /// 文字と軸の間
        float     x_pos  = vwidth / (vscale * 2.0) + axis_vec[ic].x() * (vwidth / (vscale * 2.0));
        float     y_pos  = vheight / (vscale * 2.0 * aspect) + axis_vec[ic].y() * (vheight / (vscale * 2.0 * aspect));
        m_render_text->renderText(x_pos + offset.x(), vheight - y_pos - offset.y(), 0.0, str, std::wstring(L"Arial"),
                                  color, 12, dummy, true, 0.0, TextAlignment::CenterCenter);
    }
}

void GLSceneRender::drawColorBar(int x_offset, int y_offset)
{
    if (!m_show_colorbar) {
        return;
    }

    int n = m_tex_color.size();
    if (n == 0) return;

    float barX = 10 + x_offset;
    float barY = 50 + y_offset;
    float barW = 40;
    float barH = 300;

    float edge_color_size = (float)barH / 11.0f;    /// 両端のカラーサイズ
    float color_size      = (float)(barH - edge_color_size * 2.0f) / (float)(n - 2);

    float widgetW = m_scene_view->viewPortWidth();
    float widgetH = m_scene_view->viewPortHeight();

    // ピクセル座標→OpenGL座標（-1.0～1.0）変換関数
    auto toGLX = [widgetW](float x) { return 2.0f * x / widgetW - 1.0f; };
    auto toGLY = [widgetH](float y) { return 1.0f - 2.0f * y / widgetH; };

    for (int i = 0; i < n; ++i) {
        float y0_px, y1_px;
        if (i == 0) {
            y0_px = barY;
            y1_px = barY + edge_color_size;
        }
        else if (i == n - 1) {
            y0_px = barY + barH - edge_color_size;
            y1_px = barY + barH;
        }
        else {
            y0_px = barY + (float)(i - 1) * color_size + edge_color_size;
            y1_px = barY + (float)(i)*color_size + edge_color_size;
        }

        float x0 = toGLX(barX);
        float x1 = toGLX(barX + barW);
        float y0 = toGLY(y0_px);
        float y1 = toGLY(y1_px);

        Point4f c = m_tex_color[i];
        glColor3f(c[0], c[1], c[2]);

        glBegin(GL_QUADS);
        glVertex2f(x0, y0);    // 左上
        glVertex2f(x1, y0);    // 右上
        glVertex2f(x1, y1);    // 右下
        glVertex2f(x0, y1);    // 左下
        glEnd();
    }

    // 境界ラベル描画
    if (m_division.size() == n - 1) {    // 境界はn+1個

        float    labelX, labelY;
        TextQuad dummy;

        labelX = (barX + barW + 8);

        labelY = (barY);
        m_render_text->renderText(
            labelX, labelY, 0.0, m_color_label_max.empty() ? std::wstring(L"MAX") : m_color_label_max,
            std::wstring(L"Arial"), Point4f(0, 0, 0, 1), 10, dummy, true, 0.0, TextAlignment::CenterLeft);

        labelY = (barY + barH);
        m_render_text->renderText(
            labelX, labelY, 0.0, m_color_label_min.empty() ? std::wstring(L"MIN") : m_color_label_min,
            std::wstring(L"Arial"), Point4f(0, 0, 0, 1), 10, dummy, true, 0.0, TextAlignment::CenterLeft);

        float step       = 1;
        int   loop_count = m_division.size();

        if (m_colorlabel_count != 0 && m_colorlabel_count != n) {
            loop_count = m_colorlabel_count - 1;
            step       = (float)(m_division.size() - 1) / (float)(loop_count - 1);
        }

        for (int i = 0; i < loop_count; ++i) {
            int index = i;
            if (i != 0 && i != loop_count - 1) {
                index = (int)((float)step * (float)(i) + 0.5);
            }
            else if (i == loop_count - 1) {
                index = m_division.size() - 1;
            }

            labelY = (barY + (float)(index)*color_size + edge_color_size);

            glColor3f(0, 0, 0);
            glLineWidth(2 * m_dpi_scale);

            glBegin(GL_LINES);
            glVertex2f(toGLX(barX + barW + 1), toGLY(labelY));
            glVertex2f(toGLX(barX + barW + 5), toGLY(labelY));
            glEnd();

            QString label = QString::number(m_division[index], 'E', 5);
            m_render_text->renderText(labelX, labelY, 0.0, label.toStdWString(), std::wstring(L"Arial"),
                                      Point4f(0, 0, 0, 1), 10, dummy, true, 0.0, TextAlignment::CenterLeft);
        }
    }
}

bool GLSceneRender::sizeDrawColorBar(int& x_min, int& y_min, int& x_max, int& y_max)
{
    int n = m_tex_color.size();
    if (n == 0) return false;

    float barX = 10;
    float barY = 50;
    float barW = 40;
    float barH = 300;

    x_min = barX;
    y_min = barY;
    x_max = barX + barW;
    y_max = barY + barH;

    float edge_color_size = (float)barH / 11.0f;    /// 両端のカラーサイズ
    float color_size      = (float)(barH - edge_color_size * 2.0f) / (float)(n - 2);

    // 境界ラベル描画
    if (m_division.size() == n - 1) {    // 境界はn+1個

        float    labelX, labelY;
        TextQuad quad;

        labelX = (barX + barW + 8);

        labelY = (barY);
        m_render_text->calcTextQuad(labelX, labelY, 0.0,
                                    m_color_label_max.empty() ? std::wstring(L"MAX") : m_color_label_max,
                                    std::wstring(L"Arial"), 10, quad, true, 0.0, TextAlignment::CenterLeft);

        for (int ic = 0; ic < 4; ++ic) {
            if (x_min > quad.m_pos[ic].x()) {
                x_min = quad.m_pos[ic].x();
            }
            if (x_max < quad.m_pos[ic].x()) {
                x_max = quad.m_pos[ic].x();
            }
            if (y_min > quad.m_pos[ic].y()) {
                y_min = quad.m_pos[ic].y();
            }
            if (y_max < quad.m_pos[ic].y()) {
                y_max = quad.m_pos[ic].y();
            }
        }

        labelY = (barY + barH);
        m_render_text->calcTextQuad(labelX, labelY, 0.0,
                                    m_color_label_min.empty() ? std::wstring(L"MIN") : m_color_label_min,
                                    std::wstring(L"Arial"), 10, quad, true, 0.0, TextAlignment::CenterLeft);

        for (int ic = 0; ic < 4; ++ic) {
            if (x_min > quad.m_pos[ic].x()) {
                x_min = quad.m_pos[ic].x();
            }
            if (x_max < quad.m_pos[ic].x()) {
                x_max = quad.m_pos[ic].x();
            }
            if (y_min > quad.m_pos[ic].y()) {
                y_min = quad.m_pos[ic].y();
            }
            if (y_max < quad.m_pos[ic].y()) {
                y_max = quad.m_pos[ic].y();
            }
        }

        float step       = 1;
        int   loop_count = m_division.size();

        if (m_colorlabel_count != 0 && m_colorlabel_count != n) {
            loop_count = m_colorlabel_count - 1;
            step       = (float)(m_division.size() - 1) / (float)(loop_count - 1);
        }

        for (int i = 0; i < loop_count; ++i) {
            int index = i;
            if (i != 0 && i != loop_count - 1) {
                index = (int)((float)step * (float)(i) + 0.5);
            }
            else if (i == loop_count - 1) {
                index = m_division.size() - 1;
            }

            labelY = (barY + (float)(index)*color_size + edge_color_size);

            QString label = QString::number(m_division[index], 'E', 5);
            m_render_text->calcTextQuad(labelX, labelY, 0.0, label.toStdWString(), std::wstring(L"Arial"), 10, quad,
                                        true, 0.0, TextAlignment::CenterLeft);

            for (int ic = 0; ic < 4; ++ic) {
                if (x_min > quad.m_pos[ic].x()) {
                    x_min = quad.m_pos[ic].x();
                }
                if (x_max < quad.m_pos[ic].x()) {
                    x_max = quad.m_pos[ic].x();
                }
                if (y_min > quad.m_pos[ic].y()) {
                    y_min = quad.m_pos[ic].y();
                }
                if (y_max < quad.m_pos[ic].y()) {
                    y_max = quad.m_pos[ic].y();
                }
            }
        }
    }

    return true;
}

void GLSceneRender::paintOnlyColorBar(int view_port[4], int x_offset, int y_offset)
{
    int save_viewport[4];
    m_scene_view->viewPort(save_viewport[0], save_viewport[1], save_viewport[2], save_viewport[3]);
    m_scene_view->setViewPort(view_port[0], view_port[1], view_port[2], view_port[3]);
    m_gl_function->glViewport(view_port[0], view_port[1], view_port[2], view_port[3]);

    bool save_colorbar = m_show_colorbar;
    m_show_colorbar    = true;
    drawColorBar(x_offset, y_offset);
    m_show_colorbar = save_colorbar;

    m_scene_view->setViewPort(save_viewport[0], save_viewport[1], save_viewport[2], save_viewport[3]);
}

void GLSceneRender::paintBackground(int x, int y, int width, int height)
{
    //// 背景描画部分のみ必要
    m_gl_function->glViewport(x, y, width, height);

    /// Init
    m_gl_function->glEnable(GL_DEPTH_TEST);
    m_gl_function->glEnable(GL_BLEND);
    m_gl_function->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    renderBackGround();
    m_gl_function->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_background_color_grad) {
        renderBackGroundGradient();
        m_gl_function->glClear(GL_DEPTH_BUFFER_BIT);
    }
}

int GLSceneRender::set3DTextureShader(const VoxelScalar* voxel_scalar)
{
    RenderTextureData* render_texture_data = (RenderTextureData*)voxel_scalar->renderTextureData();
    if (!render_texture_data) {
        return -1;
    }

    if (voxel_scalar->is2DTexture()) {
        m_cur_shader_program->setUniformValue("texture_2d", true);
    }
    else {
        m_cur_shader_program->setUniformValue("texture_3d", true);
    }
    m_cur_shader_program->setUniformValue("division_size", (int)m_division.size());
    m_cur_shader_program->setUniformValueArray("divisions", (float*)m_division.data(), m_division.size(), 1);
    m_cur_shader_program->setUniformValueArray("colors", (QVector4D*)m_tex_color.data(), m_tex_color.size());
    m_cur_shader_program->setUniformValue("transparency", 1.0f);

    QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();

    Point3f voxel_size(render_texture_data->m_number.x(), render_texture_data->m_number.y(),
                       render_texture_data->m_number.z());
    m_cur_shader_program->setUniformValue("voxel_size", voxel_size);
    m_cur_shader_program->setUniformValue("voxel_range_min", render_texture_data->m_3d_range.min_pos());
    m_cur_shader_program->setUniformValue("voxel_range_max", render_texture_data->m_3d_range.max_pos());
    m_cur_shader_program->setUniformValue("rangeCount", (int)voxel_scalar->rangeMin().size());
    m_cur_shader_program->setUniformValueArray("rangeStart", (float*)voxel_scalar->rangeMin().data(),
                                               voxel_scalar->rangeMin().size(), 1);
    m_cur_shader_program->setUniformValueArray("rangeEnd", (float*)voxel_scalar->rangeMax().data(),
                                               voxel_scalar->rangeMax().size(), 1);

    /// 複数タイルに対応
    int tileCount = (int)render_texture_data->m_texture_IDs.size();
    if (tileCount > 0) {
        for (int i = 0; i < tileCount; ++i) {
            const auto& tile = render_texture_data->m_texture_IDs[i];
            f->glActiveTexture(GL_TEXTURE0 + i);    /// 複数にする場合
            glBindTexture(GL_TEXTURE_3D, tile);
            std::string uniformName = QString("volumeTex[%1]").arg(i).toStdString();
            m_cur_shader_program->setUniformValue(uniformName.c_str(), i);
        }

        std::vector<float> offsetsX(tileCount);
        std::vector<float> offsetsY(tileCount);
        std::vector<float> tileSizesFlatX(tileCount);
        std::vector<float> tileSizesFlatY(tileCount);
        for (int i = 0; i < tileCount; ++i) {
            offsetsX[i]       = render_texture_data->m_offset_x[i] / (float)render_texture_data->m_number.x();
            offsetsY[i]       = render_texture_data->m_offset_y[i] / (float)render_texture_data->m_number.y();
            tileSizesFlatX[i] = render_texture_data->m_offset_wx[i] / float(render_texture_data->m_number.x());
            tileSizesFlatY[i] = render_texture_data->m_offset_wy[i] / float(render_texture_data->m_number.y());
        }
        m_cur_shader_program->setUniformValueArray("tileOffsetX", offsetsX.data(), tileCount, 1);
        m_cur_shader_program->setUniformValueArray("tileOffsetY", offsetsY.data(), tileCount, 1);
        m_cur_shader_program->setUniformValueArray("tileSizeX", tileSizesFlatX.data(), tileCount, 1);
        m_cur_shader_program->setUniformValueArray("tileSizeY", tileSizesFlatY.data(), tileCount, 1);
        if (!voxel_scalar->is2DTexture()) {
            std::vector<float> offsetsZ(tileCount);
            std::vector<float> tileSizesFlatZ(tileCount);
            for (int i = 0; i < tileCount; ++i) {
                offsetsZ[i]       = render_texture_data->m_offset_z[i] / (float)render_texture_data->m_number.z();
                tileSizesFlatZ[i] = render_texture_data->m_offset_wz[i] / float(render_texture_data->m_number.z());
            }
            m_cur_shader_program->setUniformValueArray("tileOffsetZ", offsetsZ.data(), tileCount, 1);
            m_cur_shader_program->setUniformValueArray("tileSizeZ", tileSizesFlatZ.data(), tileCount, 1);
        }
    }
    m_cur_shader_program->setUniformValue("tileCount", tileCount);
    m_cur_shader_program->setUniformValue("tileXCount", render_texture_data->m_xcount);
    m_cur_shader_program->setUniformValue("tileYCount", render_texture_data->m_ycount);
    m_cur_shader_program->setUniformValue("tileZCount", render_texture_data->m_zcount);

    if (m_project_voxel_scalar_color_shape) {
    }
    else {
        /// 指定色
        m_cur_shader_program->setUniformValue("color", m_project_voxel_scalar_color[0], m_project_voxel_scalar_color[1],
                                              m_project_voxel_scalar_color[2]);
    }

    return tileCount;
}

void GLSceneRender::reset3DTextureShader(int tile_count)
{
    m_cur_shader_program->setUniformValue("texture_3d", false);
    m_cur_shader_program->setUniformValue("texture_2d", false);

    QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();

    if (tile_count > 0) {
        for (int i = 0; i < tile_count; ++i) {
            f->glActiveTexture(GL_TEXTURE0 + i);
            f->glBindTexture(GL_TEXTURE_3D, 0);
        }
    }
    f->glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, 0);
    // glBindTexture(GL_TEXTURE_2D, 0);
}

bool GLSceneRender::is2DTexturePriority()
{
    /// 2D Textureは、現在のマトリックス設定が面方向として計算

    const auto& cur_path_matrix = this->curPathMatrix();

    const auto& plane = cur_path_matrix.toPlane();

    const auto& norm = plane.normal();
    const auto& org  = plane.origin();

    /// 最大14 (Vox6 + Clip8)なので、線形で比較する
    /// 高速化するならXYZAnyで分けて管理
    /// あるいは座標の高速検索

    float eps = m_scene_view->sceneGraph()->lengthUnit().epsilonValidLength() * 10;

    for (auto& check_plane : m_clipping_planes) {
        const auto& check_norm = check_plane.normal();
        if (fabs(norm * check_norm) >= 1.0f - 1e-3f) {
            if (fabs(check_plane.distanceToPoint(org)) <= eps) {
                // DEBUG() << "On Plane";
                return true;
            }
            else {
            }
        }
    }
    return false;
}

void GLSceneRender::setPickTargetFilter(const std::set<ObjectType>& pick_filter)
{
    m_pick_target_filter = pick_filter;
}

void GLSceneRender::resetPickTargetFilter()
{
    m_pick_target_filter.clear();
}

void GLSceneRender::setPickNodeFilter(const std::set<Node*>& target_nodes)
{
    m_pick_target_node = target_nodes;
}

void GLSceneRender::clearPickNodeFilter()
{
    m_pick_target_node.clear();
}

void GLSceneRender::setMaxPickLength(float length)
{
    m_max_pick_length = length;
}

void GLSceneRender::set3DTextureColor(const std::vector<float>& division, const std::vector<Point4f>& color)
{
    m_division  = division;
    m_tex_color = color;
}

void GLSceneRender::set3DTextureDivision(const std::vector<float>& division)
{
    m_division = division;
}

std::vector<float> GLSceneRender::textureDivision()
{
    return m_division;
}

void GLSceneRender::setColorLabelCount(int label_count)
{
    m_colorlabel_count = label_count;
}

void GLSceneRender::setShowColorBar(bool show)
{
    m_show_colorbar = show;
}

void GLSceneRender::setColorMinMaxLabel(const std::wstring& min_label, const std::wstring& max_label)
{
    m_color_label_min = min_label;
    m_color_label_max = max_label;
}

void GLSceneRender::addPickObject(PickData& pick_data)
{
    m_pick_objects.emplace_back(pick_data);
}

void GLSceneRender::popPickObject()
{
    m_pick_objects.pop_back();
}

void GLSceneRender::clearPickObjects()
{
    m_pick_objects.clear();
}

unsigned char getColor(float value, const std::vector<float>& divisions, int division_size)
{
    // 二分探索による範囲判定
    int low  = 0;
    int high = division_size - 1;    // 配列の最後のインデックス
    while (low <= high) {
        int mid = (low + high) / 2;    // 中央のインデックスを計算
        if (value > divisions[mid]) {
            high = mid - 1;    // 左側を探索
        }
        else {
            low = mid + 1;    // 右側を探索
        }
    }
    if (low > division_size) return (unsigned char)division_size;
    if (low < 0) return 0;

    // `low` が属する範囲のインデックス
    return (unsigned char)low;
}

bool GLSceneRender::create3DTexture(VoxelScalar* voxel_scalar)
{
    const float* data = voxel_scalar->scalarData();

    /// 現状のOpt構造の読み込みがこうなっている.シェーダーで調整する
    /// 軸: width = nZ, height = nY, depth = nX (Z, Y, Xの順)
    int nZ = voxel_scalar->nZ();
    int nY = voxel_scalar->nY();
    int nX = voxel_scalar->nX();

    m_gl_widget->makeCurrent();

    GLint maxWidth;
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &maxWidth);    // 幅・高さ・奥行き共通
    DEBUG() << "GL_MAX_3D_TEXTURE_SIZE: " << maxWidth;

    // maxWidth = 2048;    /// TEST

    int width  = nZ;
    int height = nY;
    int depth  = nX;

    DEBUG() << "width: " << width;
    DEBUG() << "height: " << height;
    DEBUG() << "depth: " << depth;

    RenderTextureData* render_data = (RenderTextureData*)voxel_scalar->renderTextureData();
    if (!render_data) {
        /// 初回
        render_data = new RenderTextureData(m_gl_widget);
        voxel_scalar->setRenderTextureData(render_data, [](void* ptr, bool direct_delete) {
            if (direct_delete) {
                delete static_cast<RenderTextureData*>(ptr);
            }
            else {
                /// m_gl_widgetのスレッド（メインスレッド）で削除
                QMetaObject::invokeMethod(
                    static_cast<RenderTextureData*>(ptr)->m_gl_widget,
                    [ptr]() { delete static_cast<RenderTextureData*>(ptr); }, Qt::QueuedConnection);
            }
        });
    }
    else {
        /// 再作成
        voxel_scalar->deleteRenderTextureData(true);
        render_data = new RenderTextureData(m_gl_widget);
        voxel_scalar->setRenderTextureData(render_data, [](void* ptr, bool direct_delete) {
            if (direct_delete) {
                delete static_cast<RenderTextureData*>(ptr);
            }
            else {
                /// m_gl_widgetのスレッド（メインスレッド）で削除
                QMetaObject::invokeMethod(
                    static_cast<RenderTextureData*>(ptr)->m_gl_widget,
                    [ptr]() { delete static_cast<RenderTextureData*>(ptr); }, Qt::QueuedConnection);
            }
        });
    }

    render_data->m_3d_range.init();
    render_data->m_3d_range.expandBy(voxel_scalar->x(), voxel_scalar->y(), voxel_scalar->z());
    render_data->m_3d_range.expandBy(voxel_scalar->nX() * voxel_scalar->dX(), voxel_scalar->nY() * voxel_scalar->dY(),
                                     voxel_scalar->nZ() * voxel_scalar->dZ());
    render_data->m_number.set(voxel_scalar->nX(), voxel_scalar->nY(), voxel_scalar->nZ());

    int tileW = std::min(width, maxWidth);
    int tileH = std::min(height, maxWidth);
    int tileD = std::min(depth, maxWidth);

    int numTilesX = (depth + maxWidth - 1) / maxWidth;     // X(=nX=depth)方向
    int numTilesY = (height + maxWidth - 1) / maxWidth;    // Y(=nY)
    int numTilesZ = (width + maxWidth - 1) / maxWidth;     // Z(=nZ)

    render_data->m_xcount = numTilesX;
    render_data->m_ycount = numTilesY;
    render_data->m_zcount = numTilesZ;

    DEBUG() << "numTilesX: " << numTilesX;
    DEBUG() << "numTilesY: " << numTilesY;
    DEBUG() << "numTilesZ: " << numTilesZ;

    GLint maxTextures = 0;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextures);
    DEBUG() << "maxTextures: " << maxTextures;
    if (maxTextures <= 0 || maxTextures > 31) {
        maxTextures = 31;
    }

    bool has_error = false;

    QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();

    for (int tileX = 0; tileX < numTilesX; ++tileX) {            // X: nX, depth
        for (int tileY = 0; tileY < numTilesY; ++tileY) {        // Y: nY, height
            for (int tileZ = 0; tileZ < numTilesZ; ++tileZ) {    // Z: nZ, width

                int xOffset = tileX * maxWidth;    // X軸（最外側）
                int yOffset = tileY * maxWidth;    // Y軸
                int zOffset = tileZ * maxWidth;    // Z軸（最内側）

                int curBlockD = std::min(tileD, depth - xOffset);     // X: curBlockD
                int curBlockH = std::min(tileH, height - yOffset);    // Y
                int curBlockW = std::min(tileW, width - zOffset);     // Z

                GLuint texId = 0;
                glGenTextures(1, &texId);
                glBindTexture(GL_TEXTURE_3D, texId);

                glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

                glGetError();    /// 念のためエラー初期化

                if (numTilesX == 1 && numTilesY == 1 && numTilesZ == 1) {
                    f->glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, curBlockW, curBlockH, curBlockD,    /// z,y,x
                                    0, GL_RED, GL_FLOAT, data);
                }
                else {
                    std::vector<float> subBlock((__int64)curBlockW * (__int64)curBlockH * (__int64)curBlockD, 0.0f);

                    for (int x = 0; x < curBlockD; ++x) {
                        for (int y = 0; y < curBlockH; ++y) {
                            int srcX = xOffset + x;
                            int srcY = yOffset + y;
                            // Z列分まとめてコピー
                            __int64 dstBase =
                                (__int64)x * (__int64)curBlockW * (__int64)curBlockH + (__int64)y * (__int64)curBlockW;
                            __int64 srcBase = (__int64)srcX * (__int64)nY * (__int64)nZ + (__int64)srcY * (__int64)nZ
                                            + (__int64)zOffset;
                            std::memcpy(&subBlock[dstBase], &data[srcBase], sizeof(float) * curBlockW);
                        }
                    }
                    f->glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, curBlockW, curBlockH, curBlockD,    /// z,y,x
                                    0, GL_RED, GL_FLOAT, subBlock.data());
                }

                GLenum err = glGetError();
                if (err != GL_NO_ERROR) {
                    DEBUG() << "Create Texture Error: " << err;
                    maxTextures = render_data->m_texture_IDs.size();    /// loop抜ける
                    has_error   = true;
                    break;
                }

                render_data->m_texture_IDs.push_back(texId);
                render_data->m_offset_x.push_back(xOffset);
                render_data->m_offset_y.push_back(yOffset);
                render_data->m_offset_z.push_back(zOffset);
                render_data->m_offset_wx.push_back(curBlockD);
                render_data->m_offset_wy.push_back(curBlockH);
                render_data->m_offset_wz.push_back(curBlockW);

                if (maxTextures == render_data->m_texture_IDs.size()) {
                    break;
                }
            }
            if (maxTextures == render_data->m_texture_IDs.size()) {
                break;
            }
        }
        if (maxTextures == render_data->m_texture_IDs.size()) {
            DEBUG() << "Over max textures: " << maxTextures;
            has_error = true;
            break;
        }
    }

    m_gl_widget->doneCurrent();

    return !has_error;
}

void GLSceneRender::create2DTexture(VoxelScalar* voxel_scalar)
{
    const float* data = voxel_scalar->scalarData();

    int width  = voxel_scalar->nX();
    int height = voxel_scalar->nY();
    DEBUG() << "width: " << width;
    DEBUG() << "height: " << height;

    m_gl_widget->makeCurrent();

    GLint maxWidth;
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &maxWidth);    // 幅・高さ・奥行き共通
    DEBUG() << "GL_MAX_3D_TEXTURE_SIZE: " << maxWidth;

    RenderTextureData* render_data = (RenderTextureData*)voxel_scalar->renderTextureData();
    if (!render_data) {
        /// 初回
        render_data = new RenderTextureData(m_gl_widget);
        voxel_scalar->setRenderTextureData(render_data, [](void* ptr, bool direct_delete) {
            if (direct_delete) {
                delete static_cast<RenderTextureData*>(ptr);
            }
            else {
                /// m_gl_widgetのスレッド（メインスレッド）で削除
                QMetaObject::invokeMethod(
                    static_cast<RenderTextureData*>(ptr)->m_gl_widget,
                    [ptr]() { delete static_cast<RenderTextureData*>(ptr); }, Qt::QueuedConnection);
            }
        });
    }
    else {
        /// 再作成
        voxel_scalar->deleteRenderTextureData(true);
        render_data = new RenderTextureData(m_gl_widget);
        voxel_scalar->setRenderTextureData(render_data, [](void* ptr, bool direct_delete) {
            if (direct_delete) {
                delete static_cast<RenderTextureData*>(ptr);
            }
            else {
                /// m_gl_widgetのスレッド（メインスレッド）で削除
                QMetaObject::invokeMethod(
                    static_cast<RenderTextureData*>(ptr)->m_gl_widget,
                    [ptr]() { delete static_cast<RenderTextureData*>(ptr); }, Qt::QueuedConnection);
            }
        });
    }

    render_data->m_3d_range.init();
    render_data->m_3d_range.expandBy(voxel_scalar->x(), voxel_scalar->y(), 0);
    render_data->m_3d_range.expandBy(voxel_scalar->nX() * voxel_scalar->dX(), voxel_scalar->nY() * voxel_scalar->dY(),
                                     0);
    render_data->m_number.set(voxel_scalar->nX(), voxel_scalar->nY(), 1);

    QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();

    // maxWidth = 250;    /// TEST

    int blockWidth  = std::min(width, maxWidth);
    int blockHeight = std::min(height, maxWidth);
    int numTilesX   = (width + maxWidth - 1) / maxWidth;
    int numTilesY   = (height + maxWidth - 1) / maxWidth;

    DEBUG() << "numTilesX: " << numTilesX;
    DEBUG() << "numTilesY: " << numTilesY;

    GLint maxTextures = 0;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextures);
    DEBUG() << "maxTextures: " << maxTextures;
    if (maxTextures <= 0 || maxTextures > 31) {
        maxTextures = 31;
    }

    render_data->m_xcount = numTilesX;
    render_data->m_ycount = numTilesY;

    for (int tileY = 0; tileY < numTilesY; ++tileY) {
        for (int tileX = 0; tileX < numTilesX; ++tileX) {
            int xOffset        = tileX * maxWidth;
            int yOffset        = tileY * maxWidth;
            int curBlockWidth  = std::min(blockWidth, width - xOffset);
            int curBlockHeight = std::min(blockHeight, height - yOffset);

            GLuint texId = 0;
            glGenTextures(1, &texId);
            /*
            glBindTexture(GL_TEXTURE_2D, texId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            if (numTilesX == 1 && numTilesY == 1) {
                f->glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, curBlockWidth, curBlockHeight, 0, GL_RED, GL_FLOAT,
            data);
            }
            else {
                std::vector<float> subImage(curBlockWidth * curBlockHeight, 0.0f);
                for (int y = 0; y < curBlockHeight; ++y) {
                    memcpy(&subImage[(__int64)y * (__int64)curBlockWidth],
                           &data[((__int64)yOffset + (__int64)y) * (__int64)width + (__int64)xOffset],
                           sizeof(float) * curBlockWidth);
                }
                f->glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, curBlockWidth, curBlockHeight, 0, GL_RED, GL_FLOAT,
                                subImage.data());
            }
            */
            glBindTexture(GL_TEXTURE_3D, texId);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            if (numTilesX == 1 && numTilesY == 1) {
                f->glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, curBlockWidth, curBlockHeight, 1, 0, GL_RED, GL_FLOAT, data);
            }
            else {
                std::vector<float> subImage((__int64)curBlockWidth * (__int64)curBlockHeight, 0.0f);
                for (int y = 0; y < curBlockHeight; ++y) {
                    memcpy(&subImage[(__int64)y * (__int64)curBlockWidth],
                           &data[((__int64)yOffset + (__int64)y) * (__int64)width + (__int64)xOffset],
                           sizeof(float) * curBlockWidth);
                }
                f->glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, curBlockWidth, curBlockHeight, 1, 0, GL_RED, GL_FLOAT,
                                subImage.data());
            }

            render_data->m_texture_IDs.push_back(texId);
            render_data->m_offset_x.push_back(xOffset);
            render_data->m_offset_y.push_back(yOffset);
            render_data->m_offset_wx.push_back(curBlockWidth);
            render_data->m_offset_wy.push_back(curBlockHeight);

            if (maxTextures == render_data->m_texture_IDs.size()) {
                break;
            }
        }
        if (maxTextures == render_data->m_texture_IDs.size()) {
            DEBUG() << "Over max textures: " << maxTextures;
            break;
        }
    }

    /*
    // 3Dテクスチャの作成
    glGenTextures(1, &render_data->m_texture_ID);
    glBindTexture(GL_TEXTURE_3D, render_data->m_texture_ID);

    // 16ビットデータを使用してテクスチャをアップロード
    f->glTexImage3D(GL_TEXTURE_3D,    // ターゲット
                    0,                // ミップマップレベル
                    GL_R32F,          // 内部フォーマット（32ビット浮動小数点の赤チャンネル）
                    width, height, 1,    // テクスチャの幅、高さ、深さ
                    0,                   // 境界幅（常に0）
                    GL_RED,              // テクスチャのフォーマット（赤チャンネルのみ）
                    GL_FLOAT,            // データ型（32ビット浮動小数点）
                    data                 // テクスチャデータ
    );
    */

    m_gl_widget->doneCurrent();
}

RENDER_NAMESPACE_END
