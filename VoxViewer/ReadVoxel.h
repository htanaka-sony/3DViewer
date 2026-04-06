#ifndef READVOXEL_H
#define READVOXEL_H

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

class ReadVoxel : public QObject {
    Q_OBJECT
public:
    ReadVoxel(QObject* parent, QProgressDialog* pd) : QObject(parent), m_pd(pd) {}
    ~ReadVoxel() {}

    bool read(const QString& path, MyOpenGLWidget* gl_view, Vox3DForm* form);

    void readFdtd(const QString& path);

    void setNoChangeGUI() { m_no_change_gui = true; }

public slots:
    void setProgMessage(int value);

protected:
    QProgressDialog* m_pd;

    QHash<QString, QString> m_fdtd_mname_to_cname;

    bool m_no_change_gui = false;

    bool m_cancel = false;
};

/// ProgressBar表示（メインスレッド）と処理を分けるようのスレッド
class ReadVoxelFileThread : public QThread {
    Q_OBJECT
public:
    ReadVoxelFileThread(QStringList* lines, std::map<int, Core::RefPtr<Core::Voxel>>* material_id_voxel_map,
                        std::map<int, QString>* material_id_name_map, std::vector<char>* compress_data,
                        int* compress_data_original_size, bool use_progress_bar, int prog_max_value, int prog_step)
        : m_lines(lines)
        , m_material_id_voxel_map(material_id_voxel_map)
        , m_material_id_name_map(material_id_name_map)
        , m_compress_data(compress_data)
        , m_compress_data_original_size(compress_data_original_size)
        , m_use_progress_bar(use_progress_bar)
        , m_prog_max_value(prog_max_value)
        , m_prog_step(prog_step)
    {
    }

    void setCancel() { m_cancel = true; }

protected:
    void run() override;

signals:
    void setProgMessage(int value);

protected:
    QStringList*                              m_lines;
    std::map<int, Core::RefPtr<Core::Voxel>>* m_material_id_voxel_map;
    std::map<int, QString>*                   m_material_id_name_map;
    std::vector<char>*                        m_compress_data;
    int*                                      m_compress_data_original_size;
    bool                                      m_use_progress_bar;
    int                                       m_prog_max_value;
    int                                       m_prog_step;

    std::atomic<bool> m_cancel = false;
};

class CreateVoxelDispDataThread : public QThread {
    Q_OBJECT
public:
    CreateVoxelDispDataThread(std::vector<std::pair<int, Core::RefPtr<Core::Node>>>* material_id_node_list,
                              bool use_progress_bar, int prog_init_value, int prog_step)
        : m_material_id_node_list(material_id_node_list)
        , m_use_progress_bar(use_progress_bar)
        , m_prog_init_value(prog_init_value)
        , m_prog_step(prog_step)
    {
    }

    void setCancel() { m_cancel = true; }

protected:
    void run() override;

signals:
    void setProgMessage(int value);

protected:
    std::vector<std::pair<int, Core::RefPtr<Core::Node>>>* m_material_id_node_list;
    bool                                                   m_use_progress_bar;
    int                                                    m_prog_init_value;
    int                                                    m_prog_step;

    std::atomic<bool> m_cancel = false;
};

#endif    // READVOXEL_H
