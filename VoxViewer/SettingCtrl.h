#ifndef SETTINGCTRL_H
#define SETTINGCTRL_H

#include <QObject>
#include <QSettings>

#include "VoxViewerGlobal.h"

class VOXVIEWER_EXPORT SettingCtrl {
public:
    SettingCtrl();
    ~SettingCtrl();

    static QString documentPath();

    static QString resultSettingPath();

    static QString resultColorMapPath();

    static QString settingIniPath();

    static QString logFilePath(int index);
    static int     logLevel();

    void    setInt(const QString& strKey, int iValue, bool bInit = false);
    void    setDouble(const QString& strKey, double dValue, bool bInit = false);
    void    setString(const QString& strKey, const QString& strValue, bool bInit = false);
    int     getInt(const QString& strKey, const int defaultValue = 0);
    double  getDouble(const QString& strKey, const double defaultValue = 0.0);
    QString getString(const QString& strKey, const QString& defaultValue = QString(), bool bRetDefaultIfEmpty = false);
    void    sync();

    void remove(const QString& strKey);

    static bool createDirectoryIfNotExists(const QString& dirPath);

    /// INI定義
    static inline const QString MOUSE                     = QStringLiteral("Mouse");
    static inline const QString CONFIG_MOUSE_ACT          = MOUSE + QStringLiteral("/act");
    static inline const QString CONFIG_MOUSE_ZAXIS_FIX    = MOUSE + QStringLiteral("/zaxis_fix");
    static inline const QString CONFIG_MOUSE_ZOOM_REVERSE = MOUSE + QStringLiteral("/zoom_reverse");

    static inline const QString LIGHT            = QStringLiteral("Light");
    static inline const QString CONFIG_LIGHT_DIR = LIGHT + QStringLiteral("/dir");

    static inline const QString VIEW                              = QStringLiteral("View");
    static inline const QString CONFIG_VIEW_BACKGROUND_COLOR      = VIEW + QStringLiteral("/background_color");
    static inline const QString CONFIG_VIEW_BACKGROUND_COLOR_GRAD = VIEW + QStringLiteral("/background_color_grad");
    static inline const QString CONFIG_VIEW_PROJECTION            = VIEW + QStringLiteral("/projection");

    static inline const QString CLIP               = QStringLiteral("Clip");
    static inline const QString CONFIG_CLIP_PRESET = CLIP + QStringLiteral("/preset");

    static inline const QString PICK                  = QStringLiteral("Pick");
    static inline const QString CONFIG_PICK_SNAP      = PICK + QStringLiteral("/snap");
    static inline const QString CONFIG_PICK_SNAP_TYPE = PICK + QStringLiteral("/snap_type");

    static inline const QString DIMENSION                    = QStringLiteral("Dimension");
    static inline const QString CONFIG_DIMENSION_CREATE_TYPE = DIMENSION + QStringLiteral("/create_type");
    static inline const QString CONFIG_DIMENSION_COLOR       = DIMENSION + QStringLiteral("/color");
    static inline const QString CONFIG_DIMENSION_SIZE        = DIMENSION + QStringLiteral("/size");
    static inline const QString CONFIG_DIMENSION_LINE_WIDTH  = DIMENSION + QStringLiteral("/line_width");
    static inline const QString CONFIG_DIMENSION_TEXT_DRAG   = DIMENSION + QStringLiteral("/text_drag");
    static inline const QString CONFIG_DIMENSION_DISP_UNIT   = DIMENSION + QStringLiteral("/disp_unit");
    static inline const QString CONFIG_DIMENSION_DISP_NAME   = DIMENSION + QStringLiteral("/disp_name");
    static inline const QString CONFIG_DIMENSION_EDGE_TYPE   = DIMENSION + QStringLiteral("/edge_type");

    static inline const QString AUTO_DIMENSION                    = QStringLiteral("AutoDimension");
    static inline const QString CONFIG_AUTO_DIMENSION_CREATE_TYPE = AUTO_DIMENSION + QStringLiteral("/create_type");
    static inline const QString CONFIG_AUTO_DIMENSION_COLOR       = AUTO_DIMENSION + QStringLiteral("/color");
    static inline const QString CONFIG_AUTO_DIMENSION_SIZE        = AUTO_DIMENSION + QStringLiteral("/size");
    static inline const QString CONFIG_AUTO_DIMENSION_LINE_WIDTH  = AUTO_DIMENSION + QStringLiteral("/line_width");
    static inline const QString CONFIG_AUTO_DIMENSION_TEXT_DRAG   = AUTO_DIMENSION + QStringLiteral("/text_drag");
    static inline const QString CONFIG_AUTO_DIMENSION_DISP_UNIT   = AUTO_DIMENSION + QStringLiteral("/disp_unit");
    static inline const QString CONFIG_AUTO_DIMENSION_DISP_MAT    = AUTO_DIMENSION + QStringLiteral("/disp_mat");
    static inline const QString CONFIG_AUTO_DIMENSION_DISP_VALUE  = AUTO_DIMENSION + QStringLiteral("/disp_value");
    static inline const QString CONFIG_AUTO_DIMENSION_EXTEND      = AUTO_DIMENSION + QStringLiteral("/extend");
    static inline const QString CONFIG_AUTO_DIMENSION_CONTINUOUS  = AUTO_DIMENSION + QStringLiteral("/continuous");
    static inline const QString CONFIG_AUTO_DIMENSION_ALONG_PLANE = AUTO_DIMENSION + QStringLiteral("/along_plane");
    static inline const QString CONFIG_AUTO_DIMENSION_SECTION     = AUTO_DIMENSION + QStringLiteral("/section");
    static inline const QString CONFIG_AUTO_DIMENSION_TEXT_ALIGN  = AUTO_DIMENSION + QStringLiteral("/text_align");
    static inline const QString CONFIG_AUTO_DIMENSION_EDGE_TYPE   = AUTO_DIMENSION + QStringLiteral("/edge_type");
    static inline const QString CONFIG_AUTO_DIMENSION_DISP_NAME   = AUTO_DIMENSION + QStringLiteral("/disp_name");

    static inline const QString VOXEL                             = QStringLiteral("Voxel");
    static inline const QString CONFIG_VOXEL_DRAW_MODE            = VOXEL + QStringLiteral("/draw_mode");
    static inline const QString CONFIG_VOXEL_WIRE_COLOR           = VOXEL + QStringLiteral("/wire_color");
    static inline const QString CONFIG_VOXEL_WIRE_COLOR_SHAPE     = VOXEL + QStringLiteral("/wire_color_shape");
    static inline const QString CONFIG_VOXEL_WIRE_WIDTH           = VOXEL + QStringLiteral("/wire_width");
    static inline const QString CONFIG_VOXEL_OPT_BASE_COLOR       = VOXEL + QStringLiteral("/opt_base_color");
    static inline const QString CONFIG_VOXEL_OPT_BASE_COLOR_SHAPE = VOXEL + QStringLiteral("/opt_base_color_shape");
    static inline const QString CONFIG_VOXEL_LIST_DOUBLE_CLICK    = VOXEL + QStringLiteral("/list_double_click");

    static inline const QString RESULT                               = QStringLiteral("Result");
    static inline const QString CONFIG_RESULT_COLOR_MODE             = RESULT + QStringLiteral("/color_mode");
    static inline const QString CONFIG_RESULT_STYLE                  = RESULT + QStringLiteral("/style");
    static inline const QString CONFIG_RESULT_AUTO_COLORBAR          = RESULT + QStringLiteral("/auto_colorbar");
    static inline const QString CONFIG_RESULT_DISPLAY_PRIORITY       = RESULT + QStringLiteral("/display_priority");
    static inline const QString CONFIG_RESULT_APPLY_COLOR_VALUE      = RESULT + QStringLiteral("/apply_color_value");
    static inline const QString CONFIG_RESULT_SHOW_INFO_CLICK        = RESULT + QStringLiteral("/show_info_click");
    static inline const QString CONFIG_RESULT_SHOW_INFO_CLICK_MSEC   = RESULT + QStringLiteral("/show_info_click_msec");
    static inline const QString CONFIG_RESULT_HIDE_OP2_ARRAY_SETTING = RESULT + QStringLiteral("/hide_array_setting");

    static inline const QString PERFORMANCE                     = QStringLiteral("Performance");
    static inline const QString CONFIG_PERFORMANCE_CPU_PRIORITY = PERFORMANCE + QStringLiteral("/cpu_priority");
    static inline const QString CONFIG_PERFORMANCE_OPT_LIMIT    = PERFORMANCE + QStringLiteral("/opt_limit");
    static inline const QString CONFIG_PERFORMANCE_OPT_SIZE     = PERFORMANCE + QStringLiteral("/opt_size");
    static inline const QString CONFIG_PERFORMANCE_OPT_UNIT     = PERFORMANCE + QStringLiteral("/opt_unit");
    static inline const QString CONFIG_PERFORMANCE_OPT_METHOD   = PERFORMANCE + QStringLiteral("/opt_method");
    static inline const QString CONFIG_PERFORMANCE_OPT_WARN     = PERFORMANCE + QStringLiteral("/opt_warn");

    static inline const QString FILE_LOAD                     = QStringLiteral("FileLoad");
    static inline const QString CONFIG_FILE_LOAD_OP2_OPEN_ALL = FILE_LOAD + QStringLiteral("/op2_open_all");
    static inline const QString CONFIG_FILE_LOAD_OP2_DROP     = FILE_LOAD + QStringLiteral("/op2_drop");
    static inline const QString CONFIG_FILE_LOAD_FOLDER_OPEN  = FILE_LOAD + QStringLiteral("/folder_open");

    static inline const QString RESULT_OUTPUT_IMAGE               = QStringLiteral("ResultOutputImage");
    static inline const QString CONFIG_RESULT_OUTPUT_IMAGE_FORMAT = RESULT_OUTPUT_IMAGE + QStringLiteral("/format");
    static inline const QString CONFIG_RESULT_OUTPUT_IMAGE_SIZE_TYPE =
        RESULT_OUTPUT_IMAGE + QStringLiteral("/size_type");
    static inline const QString CONFIG_RESULT_OUTPUT_IMAGE_SIZE_RATIO =
        RESULT_OUTPUT_IMAGE + QStringLiteral("/size_ratio");
    static inline const QString CONFIG_RESULT_OUTPUT_IMAGE_SIZE_W = RESULT_OUTPUT_IMAGE + QStringLiteral("/size_w");
    static inline const QString CONFIG_RESULT_OUTPUT_IMAGE_SIZE_H = RESULT_OUTPUT_IMAGE + QStringLiteral("/size_h");
    static inline const QString CONFIG_RESULT_OUTPUT_IMAGE_COLORVAL_TYPE =
        RESULT_OUTPUT_IMAGE + QStringLiteral("/colorval_type");
    static inline const QString CONFIG_RESULT_OUTPUT_IMAGE_COLORVAL_MIN =
        RESULT_OUTPUT_IMAGE + QStringLiteral("/colorval_min");
    static inline const QString CONFIG_RESULT_OUTPUT_IMAGE_COLORVAL_MAX =
        RESULT_OUTPUT_IMAGE + QStringLiteral("/colorval_max");
    static inline const QString CONFIG_RESULT_OUTPUT_IMAGE_COLORBAR = RESULT_OUTPUT_IMAGE + QStringLiteral("/colorbar");
    static inline const QString CONFIG_RESULT_OUTPUT_IMAGE_SPECIFY_RANGE =
        RESULT_OUTPUT_IMAGE + QStringLiteral("/specify_range");
    static inline const QString CONFIG_RESULT_OUTPUT_IMAGE_DISP_VOX = RESULT_OUTPUT_IMAGE + QStringLiteral("/disp_vox");
    static inline const QString CONFIG_RESULT_OUTPUT_IMAGE_BACKGROUND_CHANGE =
        RESULT_OUTPUT_IMAGE + QStringLiteral("/background_change");
    static inline const QString CONFIG_RESULT_OUTPUT_IMAGE_BACKGROUND_COLOR =
        RESULT_OUTPUT_IMAGE + QStringLiteral("/background_color");
    static inline const QString CONFIG_RESULT_OUTPUT_IMAGE_BACKGROUND_GRAD =
        RESULT_OUTPUT_IMAGE + QStringLiteral("/background_color_grad");

    static inline const QString VOX_OUTPUT_IMAGE                   = QStringLiteral("VoxOutputImage");
    static inline const QString CONFIG_VOX_OUTPUT_IMAGE_FORMAT     = VOX_OUTPUT_IMAGE + QStringLiteral("/format");
    static inline const QString CONFIG_VOX_OUTPUT_IMAGE_SIZE_TYPE  = VOX_OUTPUT_IMAGE + QStringLiteral("/size_type");
    static inline const QString CONFIG_VOX_OUTPUT_IMAGE_SIZE_RATIO = VOX_OUTPUT_IMAGE + QStringLiteral("/size_ratio");
    static inline const QString CONFIG_VOX_OUTPUT_IMAGE_SIZE_W     = VOX_OUTPUT_IMAGE + QStringLiteral("/size_w");
    static inline const QString CONFIG_VOX_OUTPUT_IMAGE_SIZE_H     = VOX_OUTPUT_IMAGE + QStringLiteral("/size_h");
    static inline const QString CONFIG_VOX_OUTPUT_IMAGE_DISP_VOX   = VOX_OUTPUT_IMAGE + QStringLiteral("/disp_vox");
    static inline const QString CONFIG_VOX_OUTPUT_IMAGE_FIT_DISP_VOX =
        VOX_OUTPUT_IMAGE + QStringLiteral("/fit_disp_vox");
    static inline const QString CONFIG_VOX_OUTPUT_IMAGE_BACKGROUND_CHANGE =
        VOX_OUTPUT_IMAGE + QStringLiteral("/background_change");
    static inline const QString CONFIG_VOX_OUTPUT_IMAGE_BACKGROUND_COLOR =
        VOX_OUTPUT_IMAGE + QStringLiteral("/background_color");
    static inline const QString CONFIG_VOX_OUTPUT_IMAGE_BACKGROUND_GRAD =
        VOX_OUTPUT_IMAGE + QStringLiteral("/background_color_grad");

protected:
    QSettings* m_setting;
};

/// 汎用のラッパークラス
class VOXVIEWER_EXPORT SettingsWrapper {
public:
    enum class Mode { UseQSettings, UseQMap };

    SettingsWrapper(Mode mode, const QString& filepath = QString()) : m_mode(mode)
    {
        if (m_mode == Mode::UseQSettings) {
            m_settings = new QSettings(filepath, QSettings::IniFormat);
        }
    }

    ~SettingsWrapper() { delete m_settings; }

    void setValue(const QString& key, const QVariant& value)
    {
        if (m_mode == Mode::UseQSettings) {
            m_settings->setValue(key, value);
        }
        else {
            m_map[key] = value;
        }
    }

    QVariant value(const QString& key, const QVariant& defaultValue = QVariant()) const
    {
        if (m_mode == Mode::UseQSettings) {
            return m_settings->value(key, defaultValue);
        }
        else {
            return m_map.value(key, defaultValue);
        }
    }

    void remove(const QString& key)
    {
        if (m_mode == Mode::UseQSettings) {
            m_settings->remove(key);
        }
        else {
            m_map.remove(key);
        }
    }

private:
    Mode                    m_mode;
    QSettings*              m_settings = nullptr;
    QMap<QString, QVariant> m_map;
};

#endif    // SETTINGCTRL_H
