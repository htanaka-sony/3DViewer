#ifndef READOP2_H
#define READOP2_H

#include <QObject>
#include <QString>
#include <QThread>
#include "Scene/Node.h"
#include "Scene/Voxel.h"

#include "MemoryMappedFile.h"

class CVoxelData;
class TriaList;
class Vox3DForm;
class MyOpenGLWidget;

class QProgressDialog;

using namespace Core;

class ReadOp2 : public QObject {
    Q_OBJECT
public:
    ReadOp2(QObject* parent, QProgressDialog* pd);
    ~ReadOp2() {}

    bool read(const QStringList& paths, MyOpenGLWidget* gl_view, Vox3DForm* form);

    static bool maxArrayXY(const QStringList& paths, int& max_x_array, int& max_y_array,
                           const std::map<QString, WeakRefPtr<Node>>* exists_2d_list);

    void setNoChangeGUI() { m_no_change_gui = true; }

    void setAutoDSection(BoundingBox3f& box, const QList<QPair<int, int>>* op2_array_list)
    {
        m_vox_box        = box;
        m_auto_dsection  = true;
        m_op2_array_list = op2_array_list;
    }

    void setOpenMode() { m_op2_import_mode = false; }

public slots:
    void setProgMessage(int value);

private:
    bool m_auto_dsection = false;
    bool m_no_change_gui = false;

    bool m_op2_import_mode = true;

    BoundingBox3f                 m_vox_box;
    const QList<QPair<int, int>>* m_op2_array_list = nullptr;

    QProgressDialog* m_pd;
    bool             m_cancel = false;
};

/// ProgressBar表示（メインスレッド）と処理を分けるようのスレッド
class ReadOp2FileThread : public QThread {
    Q_OBJECT
public:
    ReadOp2FileThread(const QStringList* file_path, MyOpenGLWidget* gl_view, Vox3DForm* form, bool no_change_gui,
                      bool d_section_cancel, bool auto_dsection, const QList<QPair<int, int>>* op2_array_list,
                      BoundingBox3f& bbox);

    std::vector<std::pair<QString, RefPtr<Node>>>& createNode() { return m_create_nodes; }
    void                                           setCancel() { m_cancel = true; }

    float e2MinNotZeroAll() { return m_e2_min_not_zero_all; }
    float e2MaxAll() { return m_e2_max_all; }

protected:
    void run() override;

signals:
    void setProgMessage(int value);

protected:
    const QStringList*            m_file_path       = nullptr;
    MyOpenGLWidget*               m_gl_view         = nullptr;
    Vox3DForm*                    m_form            = nullptr;
    bool                          m_dsection_cancel = false;
    bool                          m_no_change_gui   = false;
    bool                          m_auto_dsection   = false;
    const QList<QPair<int, int>>* m_op2_array_list  = nullptr;

    float m_e2_min_not_zero_all = FLT_MAX;
    float m_e2_max_all          = -FLT_MAX;

    BoundingBox3f m_vox_box;

    std::vector<std::pair<QString, RefPtr<Node>>> m_create_nodes;

    std::atomic<bool> m_cancel = false;
};

#endif    // READOP2_H
