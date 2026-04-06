#ifndef READOPT_H
#define READOPT_H

#include <QObject>
#include <QString>
#include <QThread>
#include "Scene/Node.h"
#include "Scene/Voxel.h"

class CVoxelData;
class TriaList;
class Vox3DForm;
class MyOpenGLWidget;

class QProgressDialog;

using namespace Core;

class ReadOpt : public QObject {
    Q_OBJECT
public:
    ReadOpt(QObject* parent, QProgressDialog* pd);
    ~ReadOpt() {}

    bool read(const std::wstring& file_path, MyOpenGLWidget* gl_view, Vox3DForm* form);

    void setNoChangeGUI() { m_no_change_gui = true; }

public slots:
    void setProgMessage(int value);

protected:
    QProgressDialog* m_pd;
    bool             m_cancel = false;

    bool m_no_change_gui = false;
};

/// ProgressBar表示（メインスレッド）と処理を分けるようのスレッド
class ReadOptFileThread : public QThread {
    Q_OBJECT
public:
    ReadOptFileThread(const std::wstring& file_path, MyOpenGLWidget* gl_view, bool memory_limit, size_t memory_size,
                      int memory_method)
        : m_file_path(file_path)
        , m_gl_view(gl_view)
        , m_memory_limit(memory_limit)
        , m_memory_size(memory_size)
        , m_memory_method(memory_method)
    {
    }

    RefPtr<Node> createNode() { return m_create_node; }
    void         setCancel() { m_cancel = true; }

protected:
    void run() override;

signals:
    void setProgMessage(int value);

protected:
    std::wstring    m_file_path = nullptr;
    MyOpenGLWidget* m_gl_view   = nullptr;

    bool   m_memory_limit  = false;
    size_t m_memory_size   = 0;
    int    m_memory_method = 0;

    RefPtr<Node> m_create_node;

    std::atomic<bool> m_cancel = false;
};

#endif    // READOPT_H
