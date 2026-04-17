#ifndef RENDER_RENDERDATA_H
#define RENDER_RENDERDATA_H

#include "RenderGlobal.h"

#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>

#include "Math/BoundingBox3f.h"
#include "Math/Point3i.h"

using namespace Core;

RENDER_NAMESPACE_BEGIN

class RENDER_EXPORT ManagedOpenGLBuffer : public QOpenGLBuffer {
public:
    ManagedOpenGLBuffer(Type type = VertexBuffer) : QOpenGLBuffer(type), m_size(0) {}

    void allocate(const void* data, int count)
    {
        QOpenGLBuffer::allocate(data, count);
        m_size = count;
    }

    void allocate(int count)
    {
        QOpenGLBuffer::allocate(count);
        m_size = count;
    }

    int size() const { return m_size; }

private:
    int m_size;
};

class RENDER_EXPORT RenderData {
public:
    ManagedOpenGLBuffer      m_vbo;        /// 頂点バッファ
    ManagedOpenGLBuffer      m_ibo;        /// インデックスバッファ
    QOpenGLVertexArrayObject m_vao;        /// VAO
    bool                     m_use_vbo;    /// VBOを使用するかどうか

    int m_triangle_index_size;    /// インデックスバッファのトライアングル領域

    QOpenGLWidget* m_gl_widget;    /// メモリ開放時に必要

    RenderData(QOpenGLWidget* widget)
        : m_vbo(QOpenGLBuffer::VertexBuffer)
        , m_ibo(QOpenGLBuffer::IndexBuffer)
        , m_use_vbo(false)
        , m_triangle_index_size(0)
        , m_gl_widget(widget)
    {
    }

    ~RenderData()
    {
        QOpenGLContext* current = QOpenGLContext::currentContext();
        if (!current || current != m_gl_widget->context()) {
            m_gl_widget->makeCurrent();
            m_vbo.destroy();
            m_ibo.destroy();
            m_vao.destroy();
            m_gl_widget->doneCurrent();
        }
        else {
            m_vbo.destroy();
            m_ibo.destroy();
            m_vao.destroy();
        }
    }
};

class RENDER_EXPORT RenderTextureData {
public:
    BoundingBox3f m_3d_range;    /// テクスチャの描画領域
    Point3i       m_number;      /// テクスチャ数

    std::vector<unsigned int> m_texture_IDs;
    std::vector<int>          m_offset_x;
    std::vector<int>          m_offset_y;
    std::vector<int>          m_offset_z;
    std::vector<int>          m_offset_wx;
    std::vector<int>          m_offset_wy;
    std::vector<int>          m_offset_wz;
    int                       m_xcount = 0;
    int                       m_ycount = 0;
    int                       m_zcount = 0;

    QOpenGLWidget* m_gl_widget;    /// メモリ開放時に必要
    RenderTextureData(QOpenGLWidget* widget) : m_gl_widget(widget) {}

    ~RenderTextureData()
    {
        QOpenGLContext* current = QOpenGLContext::currentContext();
        if (!current || current != m_gl_widget->context()) {
            m_gl_widget->makeCurrent();
            for (int ic = 0; ic < m_texture_IDs.size(); ++ic) {
                if (m_texture_IDs[ic] != 0) {
                    glDeleteTextures(1, &m_texture_IDs[ic]);
                }
            }
            m_gl_widget->doneCurrent();
        }
        else {
            for (int ic = 0; ic < m_texture_IDs.size(); ++ic) {
                if (m_texture_IDs[ic] != 0) {
                    glDeleteTextures(1, &m_texture_IDs[ic]);
                }
            }
        }
    }
};

RENDER_NAMESPACE_END

#endif    // RENDER_RENDERDATA_H
