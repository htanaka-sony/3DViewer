#ifndef VOXOUTPUTDLG_H
#define VOXOUTPUTDLG_H

#include <QCheckBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QProgressDialog>
#include <QThread>

#include "Scene/Node.h"
#include "Scene/VoxelScalar.h"

namespace Ui {
class VoxOutputDlg;
}

class Vox3DForm;
class ResultCtrl;
class MyOpenGLWidget;
class OutputFileThread;
class FileExplorerDialog;

using namespace Core;

class VoxOutputDlg : public QDialog {
    Q_OBJECT

public:
    explicit VoxOutputDlg(Vox3DForm* parent, MyOpenGLWidget* gl_widget);
    ~VoxOutputDlg();

    QStringList searchFilesWithProgress(const QString& dir);

    void setTarget();
    void backTarget();

    void collectTarget(Node* node);
    void outputPicture();

    void updateDlg();

    struct PlaneInfo {
        int   m_dir;         /// 0:xz, 1:yz, 2:xy
        int   m_cell;        /// セル指定
        float m_position;    /// 実数指定
        PlaneInfo(int dir, int cell, float position) : m_dir(dir), m_cell(cell), m_position(position) {}

        int cell(Voxel* voxel) const
        {
            if (m_cell < 0) {
                switch (m_dir) {
                    case 0:
                        return (int)std::floor(m_position / voxel->originalDY());
                    case 1:
                        return (int)std::floor(m_position / voxel->originalDX());
                    case 2:
                        return (int)std::floor(m_position / voxel->originalDZ());
                    default:
                        return 0;
                }
            }
            else {
                return m_cell;
            }
        }

        int cellConsiderCompress(Voxel* voxel) const
        {
            if (m_cell < 0) {
                switch (m_dir) {
                    case 0:
                        return (int)std::floor(m_position / voxel->dY());
                    case 1:
                        return (int)std::floor(m_position / voxel->dX());
                    case 2:
                        return (int)std::floor(m_position / voxel->dZ());
                    default:
                        return 0;
                }
            }
            else {
                return m_cell;
            }
        }
    };

    struct OneDirecitonInfo {
        int   m_dir;         /// 0:xz, 1:yz, 2:xy, 3:dz
        bool  m_height;      /// 高さ(Z)方向か
        int   m_cell;        /// セル指定
        float m_position;    /// 実数指定
        OneDirecitonInfo(int dir, bool height, int cell, float position)
            : m_dir(dir)
            , m_height(height)
            , m_cell(cell)
            , m_position(position)
        {
        }

        double delta(VoxelScalar* voxel) const
        {
            if (voxel->is2DTexture()) {
                if (m_height)
                    return voxel->originalDY();
                else
                    return voxel->originalDX();
            }
            else {
                switch (m_dir) {
                    case 0:
                        if (m_height)
                            return voxel->originalDZ();
                        else
                            return voxel->originalDX();
                    case 1:
                        if (m_height)
                            return voxel->originalDZ();
                        else
                            return voxel->originalDY();
                    case 2:
                        if (m_height)
                            return voxel->originalDY();
                        else
                            return voxel->originalDX();
                    default:
                        return 1;
                }
            }
        }

        double deltaConsiderCompress(VoxelScalar* voxel) const
        {
            if (voxel->is2DTexture()) {
                if (m_height)
                    return voxel->dY();
                else
                    return voxel->dX();
            }
            else {
                switch (m_dir) {
                    case 0:
                        if (m_height)
                            return voxel->dZ();
                        else
                            return voxel->dX();
                    case 1:
                        if (m_height)
                            return voxel->dZ();
                        else
                            return voxel->dY();
                    case 2:
                        if (m_height)
                            return voxel->dY();
                        else
                            return voxel->dX();
                    default:
                        return 1;
                }
            }
        }

        float ratioCompress(VoxelScalar* voxel) const
        {
            if (!voxel->isDataCompress()) {
                return 1.0f;
            }

            if (voxel->is2DTexture()) {
                if (m_height)
                    return (float)voxel->dY() / (float)voxel->originalDY();
                else
                    return (float)voxel->dX() / (float)voxel->originalDX();
            }
            else {
                switch (m_dir) {
                    case 0:
                        if (m_height)
                            return (float)voxel->dZ() / (float)voxel->originalDZ();
                        else
                            return (float)voxel->dX() / (float)voxel->originalDX();
                    case 1:
                        if (m_height)
                            return (float)voxel->dZ() / (float)voxel->originalDZ();
                        else
                            return (float)voxel->dY() / (float)voxel->originalDY();
                    case 2:
                        if (m_height)
                            return (float)voxel->dY() / (float)voxel->originalDY();
                        else
                            return (float)voxel->dX() / (float)voxel->originalDX();
                    default:
                        return 1;
                }
            }
        }

        QString nameheader() const
        {
            switch (m_dir) {
                case 0:
                    if (m_height)
                        return QString("_distZ_X");
                    else
                        return QString("_distX_Z");
                case 1:
                    if (m_height)
                        return QString("_distZ_Y");
                    else
                        return QString("_distY_Z");
                case 2:
                    if (m_height)
                        return QString("_distY_X");
                    else
                        return QString("_distX_Y");
                case 3:
                    if (m_height)
                        return QString("_distZ_D");
                    else
                        return QString("_distD_Z");
                default:
                    return "";
            }
        }

        int cell(VoxelScalar* voxel) const
        {
            if (m_cell < 0) {
                return (int)std::floor(m_position / delta(voxel));
            }
            else {
                return m_cell;
            }
        }

        int cellConsiderCompress(VoxelScalar* voxel) const
        {
            if (m_cell < 0) {
                return (int)std::floor(m_position / deltaConsiderCompress(voxel));
            }
            else {
                return (int)((float)m_cell * ratioCompress(voxel));
            }
        }

        QString name(VoxelScalar* voxel) const
        {
            return nameheader()
                 + QString::number((int)std::round(delta(voxel) * (float)cell(voxel) * 1e3f)).rightJustified(5, '0');
        }
    };

    QProgressDialog* progressDlg() { return m_pd; }

    static int valueDecimals(float value);

    void updateEnableTargetList();

    /// 親ダイアログを動かしたときにリサイズしてしまうのを防止
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override
    {
        if (isActiveWindow()) {
            if (minimumSize() != QSize(0, 0)) {
                setMinimumSize(QSize(0, 0));
            }
        }
        else {
            if (minimumSize() == QSize(0, 0)) {
                setMinimumSize(size());
            }
        }
        return QDialog::nativeEvent(eventType, message, result);
    }

    void hideFileExplorerDlg();

protected:
    void createTargetList();
    void addTargetList(const QStringList& path);
    bool addTargetList(const QString& path, int row);
    void setTargetListRelatedFile();

    void targetListCheck(bool check, bool all);
    void targetListDelete(bool all);
    void targetListArrayApply(bool all, bool show_setting_dlg);
    void targetListRelatedFileOpVox();
    void targetListRelatedFileVoxFdtd();

    void            optSurfaceXYUpdate();
    void            optSurfaceYZUpdate();
    void            optSurfaceXZUpdate();
    void            optSurfaceXYSetMinMax();
    void            optSurfaceYZSetMinMax();
    void            optSurfaceZXSetMinMax();
    void            optSurfaceXYClear();
    void            optSurfaceYZClear();
    void            optSurfaceZXClear();
    void            optSurfaceAll();
    void            optSurfaceXYAdd();
    void            optSurfaceYZAdd();
    void            optSurfaceZXAdd();
    std::set<float> strToSurfaceValue(QString str, int decimals);
    QString         summarizeValues(const std::set<float>& values, int decimals);
    QString         correctInputOptSurfaceValue(QString str, int decimals);
    void            updateOptSurfaceScale();
    void            changeOptSurfaceScale();

    void oneDirectionXY_XAdd();
    void oneDirectionXY_YAdd();
    void oneDirectionYZ_YAdd();
    void oneDirectionYZ_ZAdd();
    void oneDirectionXZ_XAdd();
    void oneDirectionXZ_ZAdd();
    void oneDirectionDZ_DAdd();
    void oneDirectionDZ_ZAdd();
    void oneDirectionXY_XUpdate();
    void oneDirectionXY_YUpdate();
    void oneDirectionYZ_YUpdate();
    void oneDirectionYZ_ZUpdate();
    void oneDirectionXZ_XUpdate();
    void oneDirectionXZ_ZUpdate();
    void oneDirectionDZ_DUpdate();
    void oneDirectionDZ_ZUpdate();
    void update1dDirectionScale();
    void change1dDirectionScale();

    void addValuesToLineEdit(QCheckBox* checkBox, QDoubleSpinBox* minSpinBox, QDoubleSpinBox* maxSpinBox,
                             QDoubleSpinBox* stepSpinBox, QLineEdit* lineEdit);

    std::vector<PlaneInfo> optTargetSurface();

    std::vector<std::vector<VoxOutputDlg::OneDirecitonInfo>> target1DDirection();

    void setDefault();
    void applyDefault(bool show_msg = false);
    void initDefault();

public slots:
    void setProgMessage(int value);
    void setProgText(const QString& text);
    void setProgEnd();

private:
    Vox3DForm*      m_3DForm;
    MyOpenGLWidget* m_gl_widget;

    Ui::VoxOutputDlg* ui;

    QProgressDialog* m_pd = nullptr;

    std::set<QString> m_target_all_files;

    QColor m_background_color;

    FileExplorerDialog* m_file_explorer_dlg = nullptr;
};

/// ProgressBar表示（メインスレッド）と処理を分けるようのスレッド
class VoxOutputThread : public QThread {
    Q_OBJECT
public:
    struct TargetNode {
        RefPtr<Node> m_node;
        bool         m_shading;
        bool         m_wireframe;
        bool         m_show;
        bool         m_suppress;
        float        m_transparent;
        Node*        m_proj_opt;
    };

    struct InputGroup {
        QString m_vox_file;
        QString m_fdtd_file;
    };

    struct Input {
        bool                                 m_change_colormap_specify = false;
        bool                                 m_change_colormap_minmax  = false;
        bool                                 m_colormap_valid          = false;
        bool                                 m_specify_range           = false;
        bool                                 m_current_show            = false;
        bool                                 m_show_fit                = false;
        bool                                 m_vox_current_shading     = true;
        bool                                 m_vox_current_wireframe   = true;
        bool                                 m_background_color_change = false;
        QColor                               m_background_color;
        bool                                 m_background_grad = false;
        float                                m_data_size_ratio = 0;
        int                                  m_pic_width       = 0;
        int                                  m_pic_height      = 0;
        std::vector<VoxOutputDlg::PlaneInfo> m_target_surface;

        QString m_ext;
        QString m_output_folder;

        std::vector<InputGroup> m_input_groups;
    };

    VoxOutputThread(VoxOutputDlg* dlg, Vox3DForm* form, MyOpenGLWidget* gl_view, Input& input_info)
        : m_dlg(dlg)
        , m_3DForm(form)
        , m_gl_widget(gl_view)
        , m_input(input_info)
    {
    }

    void setCancel() { m_cancel = true; }

    const auto& outputFolders() { return m_output_folders; }
    const auto& outputFolderFiles() { return m_output_folder_files; }

    void setFailed() { m_failed = true; }
    bool isFailed() { return m_failed; }

protected:
    void run() override;

    void collectTarget(Node* node);
    void outputPicture();

    void outputPictureOneGroup(const QString& label = "");

signals:
    void setProgMessage(int value);
    void setProgText(const QString& text);
    void setProgEnd();

protected:
    VoxOutputDlg*   m_dlg       = nullptr;
    Vox3DForm*      m_3DForm    = nullptr;
    MyOpenGLWidget* m_gl_widget = nullptr;

    Input m_input;

    RefPtr<Node>            m_save_root_node;    /// カレントでない場合ここに保持して復帰
    std::vector<TargetNode> m_voxel_list;
    std::vector<TargetNode> m_opt3d_list;
    std::vector<TargetNode> m_opt2d_list;
    std::vector<TargetNode> m_other_list;

    std::set<QString> m_set_basename;

    std::set<QString> m_output_folders;

    QMap<QString, QList<std::pair<QString, bool>>> m_output_folder_files;

    std::atomic<bool> m_cancel = false;

    bool m_failed = false;

    QString m_buffer;

    OutputFileThread* m_output_file_thread = nullptr;
};

#endif    // VOXOUTPUTDLG_H
