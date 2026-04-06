#ifndef RESULTCTRL_H
#define RESULTCTRL_H

#include <QLineEdit>
#include <QObject>
#include <QProgressDialog>
#include <QThread>
#include "Scene/Node.h"
#include "Scene/Voxel.h"
#include "Scene/VoxelScalar.h"

#include <atomic>
#include <vector>

class Vox3DForm;
class MyOpenGLWidget;
class ResultOutputDlg;

namespace Ui {
class Vox3DForm;
}

#define E2VIEWER_NAME "E2Viewer"

namespace tinycolormap {
enum class ColormapType;
}

using namespace Core;

class ResultCtrl : public QObject {
    Q_OBJECT

public:
    enum class ColorMapStyle {
        Custom = 0,    ///
        Separator,
        E2ViewerLike,
        /// tiny
        Parula,
        Heat,
        Jet,
        Turbo,
        Hot,
        Magma,
        Inferno,
        Plasma,
        Viridis,
        Cividis,
        Github,
        Cubehelix,
        HSV,
        /// White to / to Black
        WhiteToBlack,
        WhiteToRed,
        WhiteToGreen,
        WhiteToBlue,
        WhiteToYellow,
        WhiteToCyan,
        WhiteToMagenta,
        RedToBlack,
        GreenToBlack,
        BlueToBlack,
        YellowToBlack,
        CyanToBlack,
        MagentaToBlack,
    };

    struct ColormapInfo {
        QString                    m_name;
        QString                    m_tooltip;
        ColorMapStyle              m_style;
        tinycolormap::ColormapType m_tiny_type;
    };

    struct CustomColorInfo {
        std::vector<Point4f>       m_color_values;
        float                      m_range_min      = 0;
        float                      m_range_max      = 1;
        ColorMapStyle              m_original_style = ColorMapStyle::Custom;
        tinycolormap::ColormapType m_original_tiny_style =
            (tinycolormap::ColormapType)0;    /// m_original_styleで決まるがメモリ上保持

        std::optional<Point4f> m_min_color;
        std::optional<Point4f> m_max_color;
        std::optional<bool>    m_save_reverse;
        std::optional<bool>    m_save_gradient;
        std::optional<float>   m_save_range_min;
        std::optional<float>   m_save_range_max;
        std::optional<int>     m_save_steps;
    };

    struct RangeCalcData {
        Node* m_voxel_node;
        float m_min_value;
        float m_max_value;
        int   m_minmax[6];
        int   m_index;

        RefPtr<VoxelScalar> m_create_voxel;

        RangeCalcData(Node* node, float min_value, float max_value, int xmin, int xmax, int ymin, int ymax, int zmin,
                      int zmax, int index)
            : m_voxel_node(node)
            , m_min_value(min_value)
            , m_max_value(max_value)
            , m_index(index)
        {
            m_minmax[0] = xmin;
            m_minmax[1] = xmax;
            m_minmax[2] = ymin;
            m_minmax[3] = ymax;
            m_minmax[4] = zmin;
            m_minmax[5] = zmax;
        }
    };

    ResultCtrl(Vox3DForm* form, Ui::Vox3DForm* ui_);
    ~ResultCtrl();

    void reset();

    void showOpt(bool show);
    void showOp2(bool show);

    void setVoxel(RefPtr<Node> voxel_node);
    void setMinMaxLabel();
    bool setColorBand(float input_min = -1, float input_max = -1, bool reset_color = false, bool by_file_open = false);

    void calcThresholds(float min_value, float max_value, std::vector<float>& thresholds);

    void addVoxel2d(std::vector<RefPtr<Node>>& voxel_nodes, std::map<QString, WeakRefPtr<Node>>& path_to_node);

    bool setDSectionParam(int max_x_array = 1, int max_y_array = 1, bool auto_set = false);

    void setCustomColor(int index, const Point4f& color);

    const std::vector<float>&   threshold() const { return m_threshold_values; }
    const std::vector<Point4f>& colors() const { return m_color_values; }

    void setResult3dView(bool show);

    bool isHideOnProjection() const;

    void result3dRangeListContextMenu(const QPoint& mouse_pos);

    void updateResult();
    void suppressUpdateResult(bool suppress);

    std::vector<char>& voxelFileCompressData() { return m_voxel_file_compress_data; }
    void               setCompressDataOriginalSize(int size) { m_voxel_file_compress_data_original_size = size; }
    void               setVoxelFileCompressData(std::vector<char>& data) { m_voxel_file_compress_data = data; }
    int                compressDataOriginalSize() { return m_voxel_file_compress_data_original_size; }

    void setVoxSize();

    static bool sectionName(const QString& file_name, QString& symbol, QString& number, QString& symbol2,
                            QString& number2);

    const std::map<QString, WeakRefPtr<Node>>& path2dList() { return m_path_to_2dvoxel; }

    void setResultStyleDefaultCombobox();

    void select2dVoxel(Node* voxel_node);

    QString colormapMinLabel() const;
    QString colormapMaxLabel() const;

    void viewOnPlaneNoChangeGUI(Node* node);
    void viewOnPlaneNoChangeGUI(const Planef& plane);

    bool isShowInformationOnClick() const;
    int  showInformationOnClickMsec() const;

    Node*                                opt3D() { return m_voxel_3d.ptr(); }
    const std::vector<WeakRefPtr<Node>>& op2List() { return m_voxel_2d_list; }
    Point3f                              minDxDyDz();

    struct VoxNameData {
        QStringList m_vox_names;     /// _で分割
        QString     m_vox_path;      /// 元のパス
        QString     m_vox_folder;    /// フォルダ
    };

    struct FdtdNameData {
        QString m_path;      /// 元のパス
        QString m_folder;    /// フォルダ
    };

    static QString op2BaseName(const QString& op2_name, bool tm_te);
    static QString optBaseName(const QString& opt_name, bool tm_te);
    static bool    isRelatedVoxOp(const QString& vox_name, const QString& op2opt_name);
    static QString relatedVoxOp(const QString&                                     op2opt_name,
                                const std::map<QString, std::vector<VoxNameData>>& vox_name_data);
    static bool    isRelatedVoxFdtd(const QString& vox_name, const QString& fdtd_name);
    static QString relatedVoxFdtd(const QString& vox_name, const std::set<QString>& fdtd_names);
    static QString relatedVoxFdtd(const QString&                                      vox_name,
                                  const std::map<QString, std::vector<FdtdNameData>>& fdtd_name_data);

    bool setDSectionMatrix(Node* voxel_2d_scalar, int list_index = -1, int array_x = -1, int array_y = -1,
                           double vox_x = -1, double vox_y = -1);

    bool setShowOnlyVoxMaterialOnlyGUI(bool check);

    void createVoxelRangeList(std::vector<std::tuple<float, float, int>>& range_list,
                              std::vector<std::pair<float, float>>&       range_disp_list);

    bool createVoxelRangeShape(const std::vector<Node*>&                         voxel_scales,
                               const std::vector<std::tuple<float, float, int>>& range_list,
                               const std::vector<std::pair<float, float>>&       range_disp_list,
                               QWidget* pd_parent = nullptr, bool gui_no_change = false);

    auto voxelRangeVoxels() { return m_voxel_range_voxels; }
    void clearVoxelRangeVoxels() { m_voxel_range_voxels.clear(); }
    void setVoxelRangeVoxels(
        std::map<WeakRefPtr<Node>, std::map<float, std::map<float, RefPtr<Node>>>>& voxel_range_voxels)
    {
        m_voxel_range_voxels = voxel_range_voxels;
    }

    void setFileOpenStart()
    {
        m_set_opt_op2_open              = true;
        m_invalid_color_map_before_open = false;
        m_valid_color_map_min           = -1;
        m_valid_color_map_max           = -1;
    }
    void setFileOpenEnd()
    {
        m_set_opt_op2_open              = false;
        m_invalid_color_map_before_open = false;
        m_valid_color_map_min           = -1;
        m_valid_color_map_max           = -1;
    }
    class fileOpenStartEnd {
    public:
        fileOpenStartEnd(ResultCtrl* ctrl) : m_ctrl(ctrl)
        {
            if (m_ctrl) {
                m_ctrl->setFileOpenStart();
            }
        }
        ~fileOpenStartEnd()
        {
            if (m_ctrl) {
                m_ctrl->setFileOpenEnd();
            }
        }

    protected:
        ResultCtrl* m_ctrl = nullptr;
    };

    bool isEnableSpecifiedRange();

public slots:
    void setProgMessage(int value);
    void createRenderData(RangeCalcData* voxel);

    void deleteResult2dList(bool all);

protected:
    QString appendEditColorStyle(bool no_color_change = false);
    void appendCustomColorStyle(const QString& name, const CustomColorInfo& custom_color_info, bool set_current = true,
                                int insert_index = -1);
    void setE2ViewerColorStyle();

    void setThreshold(float min, float max, bool linear);
    void setColorMap();
    void setColorStyle();

    void loadColorStyle();
    void saveColorStyle();
    void deleteColorStyle();
    void setEnambleStyleSaveDelete();

    ColorMapStyle colorMapStyle(std::optional<tinycolormap::ColormapType>& tiny_type,
                                std::optional<QString>&                    style_name) const;

    int colorCount() const;
    int colorLabelCount() const;

    /// カラーマップタイプ
    Point4f e2ViewerColor(float t);
    Point4f tinyColor(float t, tinycolormap::ColormapType type, float min_range = 0.0f, float max_range = 1.0f);
    Point4f customColor(float t, const CustomColorInfo& custom_colorinfo);
    Point4f whiteToColor(float t, ColorMapStyle style);
    Point4f colorToBlack(float t, ColorMapStyle style);

    void createVoxelRangeShape();
    void clearVoxelRangeShape();

    void create3dResultRangeList();
    void add3dResultRangeList(int row = -1, const QString& min_value = QString(""),
                              const QString& max_value = QString(""));
    void insert3dResultRangeList();
    void delete3dResultRangeList(bool all);
    void resultRangeListShow(bool show, bool all);
    void setResultRangeListFromColormap();
    void clearResultRangeList();

    void create2dReultTab();
    void add2dResultList(Node* voxel_2d_scalar, bool d_section, int row);
    void result2dListContextMenu(const QPoint& mouse_pos);
    void result2dListShow(bool show, bool all, Node* except_node = nullptr, bool update_view = true);
    void applyOp2Array(bool all, bool show_setting_dlg);

    void applyOp2ArraySetting(Node* node, int index, bool update_view);

    void opt3dDetail();
    void op2Detail(Node* node);

    void setMinMax(Node* node);
    void viewOnPlane(Node* node, bool mode_2d);

protected:
    Vox3DForm* m_3DForm = nullptr;

    std::vector<float>   m_threshold_values;
    std::vector<Point4f> m_color_values;    /// 必ず　m_threshold_values.size() + 1

    /// 両端の色
    Point4f m_color_max = Point4f(1.0f, 1.0f, 1.0f, 1.0f);
    Point4f m_color_min = Point4f(0.0f, 0.0f, 0.0f, 1.0f);

    QList<QLineEdit*> m_threshold_labels;          /// 最大数確保。使用するのはm_threshold_values-2
    QList<float>      m_threshold_label_values;    /// ラベル上の数値

    /// E2Viewerのカラーマップ
    static const std::vector<Point4f> m_e2viewer_colormap;

    /// dummyの空データ
    static const CustomColorInfo m_custom_color_info_empty;

    /// 標準のColopMapリスト
    QList<ColormapInfo> m_colormap_default_list;

    /// Customのカラーマップ
    std::map<QString, CustomColorInfo> m_custom_colormap;

    /// VoxelScalar
    WeakRefPtr<Node> m_voxel_3d;

    /// 2D解析結果
    std::vector<WeakRefPtr<Node>>       m_voxel_2d_list;
    std::map<QString, WeakRefPtr<Node>> m_path_to_2dvoxel;

    std::wstring m_last_view_on_plane_path;
    Planef       m_last_view_on_plane;

    /// 範囲指定用
    std::map<WeakRefPtr<Node>, std::map<float, std::map<float, RefPtr<Node>>>> m_voxel_range_voxels;

    /// 形状情報 -
    /// 形状内に限定するのに使う（※全体ファイル情報を圧縮して持つ。個別のVoxelで圧縮して持つより、圧倒的に省メモリなのでこうする）
    std::vector<char> m_voxel_file_compress_data;
    int               m_voxel_file_compress_data_original_size;

    Ui::Vox3DForm* ui;

    bool m_suppress_set_color     = false;
    bool m_suppress_update_result = false;

    /// 全体のMin/Max(2D結果も読み込むので)
    float m_result_all_min = FLT_MAX;
    float m_result_all_max = -FLT_MAX;

    QProgressDialog* m_pd = nullptr;

    /// 暫定 ファイルオープン時のカラーマップ設定
    bool  m_set_opt_op2_open              = false;
    bool  m_invalid_color_map_before_open = false;
    float m_valid_color_map_min           = -1;
    float m_valid_color_map_max           = -1;
};

/// ProgressBar表示（メインスレッド）と処理を分けるようのスレッド
class CreateRangeVoxelScalar : public QThread {
    Q_OBJECT
public:
    CreateRangeVoxelScalar(std::vector<char>* voxel_compress_data, int compress_data_original_size,
                           MyOpenGLWidget* widget, std::vector<std::tuple<Node*, float, float>>& voxel_ranges_list,
                           std::vector<ResultCtrl::RangeCalcData>& targets)
        : m_voxel_compress_data(voxel_compress_data)
        , m_compress_data_original_size(compress_data_original_size)
        , m_gl_widget(widget)
        , m_input(&voxel_ranges_list)
        , m_targets(&targets)
    {
    }

    void setCancel() { m_cancel = true; }

protected:
    void run() override;

    bool is_in_merged_range(const std::vector<std::pair<float, float>>& merged_range, float x);
    void processVoxelRegion(int ix_min, int ix_max, int iy_min, int iy_max, int iz_min, int iz_max, int Nx, int Ny,
                            int Nz, const float* data, const std::vector<std::pair<float, float>>& merged_range,
                            Voxel* voxel_scalar_obj);
    void processVoxelRegion(int ix_min, int ix_max, int iy_min, int iy_max, int iz_min, int iz_max, int Nx, int Ny,
                            int Nz, const float* data, float min_value, float max_value, Voxel* voxel_scalar_obj);
    void processVoxelRegion(int ix_min, int ix_max, int iy_min, int iy_max, size_t Nx, size_t Ny, const float* data,
                            float min_value, float max_value, Voxel* voxel_scalar_obj);

signals:
    void setProgMessage(int value);
    void createRenderData(ResultCtrl::RangeCalcData* voxel);

protected:
    std::vector<char>*                            m_voxel_compress_data         = nullptr;
    int                                           m_compress_data_original_size = 0;
    MyOpenGLWidget*                               m_gl_widget                   = nullptr;
    std::vector<std::tuple<Node*, float, float>>* m_input                       = nullptr;
    std::vector<ResultCtrl::RangeCalcData>*       m_targets                     = nullptr;

    std::atomic<bool> m_cancel = false;
};

#endif    // RESULTCTRL_H
