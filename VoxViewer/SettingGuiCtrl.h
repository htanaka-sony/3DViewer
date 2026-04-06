#ifndef SETTINGGUICTRL_H
#define SETTINGGUICTRL_H

#include <QColor>
#include <QObject>
#include <QSettings>

#include "SettingCtrl.h"

QT_BEGIN_NAMESPACE
class QMainWindow;
QT_END_NAMESPACE

class Vox3DForm;

namespace Ui {
class Vox3DForm;
}

class SettingGuiCtrl : public QObject {
    Q_OBJECT
public:
    SettingGuiCtrl(Vox3DForm* parent, Ui::Vox3DForm* ui_);
    ~SettingGuiCtrl();

    void applyDefault();
    void resetDefault(bool init);

    void resetDefaultOtherSettings(bool init);

    void readMouseAct();
    void saveMouseAct();

    void readMouseZAxisFix();
    void saveMouseZAxisFix();

    void readMouseZoomReverse();
    void saveMouseZoomReverse();

    void readLightDir();
    void saveLightDir();

    void readClipAnyPreset();
    void saveClipAnyPreset();

    void readProjection();
    void saveProjection();

    void readBackGroundColor();
    void saveBackGroundColor();

    void readBackGroundColorGradient();
    void saveBackGroundColorGradient();

    void readPickSnap();
    void savePickSnap();

    void readPickSnapType();
    void savePickSnapType();

    void readDimensionCreateType();
    void saveDimensionCreateType();

    void readDimensionColor();
    void saveDimensionColor();

    void readDimensionSize();
    void saveDimensionSize();

    void readDimensionLineWidth();
    void saveDimensionLineWidth();

    void readDimensionEdgeType();
    void saveDimensionEdgeType();

    void readDimensionDispName();
    void saveDimensionDispName();

    void readDimensionTextDrag();
    void saveDimensionTextDrag();

    void readDimensionDispUnit();
    void saveDimensionDispUnit();

    void readAutoDimensionCreateType();
    void saveAutoDimensionCreateType();

    void readAutoDimensionColor();
    void saveAutoDimensionColor();

    void readAutoDimensionSize();
    void saveAutoDimensionSize();

    void readAutoDimensionLineWidth();
    void saveAutoDimensionLineWidth();

    void readAutoDimensionEdgeType();
    void saveAutoDimensionEdgeType();

    void readAutoDimensionDispName();
    void saveAutoDimensionDispName();

    void readAutoDimensionTextDrag();
    void saveAutoDimensionTextDrag();

    void readAutoDimensionDispUnit();
    void saveAutoDimensionDispUnit();

    void readAutoDimensionDispMat();
    void saveAutoDimensionDispMat();

    void readAutoDimensionDispValue();
    void saveAutoDimensionDispValue();

    void readAutoDimensionExtend();
    void saveAutoDimensionExtend();

    void readAutoDimensionContinuous();
    void saveAutoDimensionContinuous();

    void readAutoDimensionAlongPlane();
    void saveAutoDimensionAlongPlane();

    void readAutoDimensionSection();
    void saveAutoDimensionSection();

    void readAutoDimensionTextAlign();
    void saveAutoDimensionTextAlign();

    void readVoxelDrawMode();
    void saveVoxelDrawMode();

    void readVoxelListDoubleClick();
    void saveVoxelListDoubleClick();

    void readVoxelWireColor();
    void saveVoxelWireColor();

    void readVoxelWireColorShape();
    void saveVoxelWireColorShape();

    void readVoxelWireWidth();
    void saveVoxelWireWidth();

    void readVoxelOptBaseColor();
    void saveVoxelOptBaseColor();

    void readVoxelOptBaseColorShape();
    void saveVoxelOptBaseColorShape();

    void readResultColormapMode();
    void saveResultColormapMode();

    void readResultStyle();
    void saveResultStyle();

    void readResultAutoColorbar();
    void saveResultAutoColorbar();

    void readResultDisplayPriority();
    void saveResultDisplayPriority();

    void readResultApplyColorMinMax();
    void saveResultApplyColorMinMax();

    void readResultShowInformationOnClick();
    void saveResultShowInformationOnClick();

    void readResultShowInformationOnClickMSec();
    void saveResultShowInformationOnClickMSec();

    void readCPUPriority();
    void saveCPUPriority();
    void applyCPUPriority();

    bool   optMemoryLimitDefault();
    bool   readOptMemoryLimit();
    void   saveOptMemoryLimit(bool limit);
    double optMemorySizeDefault();
    double readOptMemorySize();
    void   saveOptMemorySize(double size);
    int    optMemoryUnitDefault();
    int    readOptMemoryUnit();
    void   saveOptMemoryUnit(int unit);
    int    optMemoryMethodDefault();
    int    readOptMemoryMethod();
    void   saveOptMemoryMethod(int method);
    bool   optMemoryWarnDefault();
    bool   readOptMemoryWarn();
    void   saveOptMemoryWarn(bool warn);

    void readOpenAllOp2Open();
    void saveOpenAllOp2Open();
    bool isOpenAllOp2Open();

    void readDragDropOp2Open();
    void saveDragDropOp2Open();
    bool isDragDropOp2Open();

    void readDragDropFolderOpen();
    void saveDragDropFolderOpen();
    bool isDragDropFolderOpen();

    void readOp2HideArraySetting();
    void saveOp2HideArraySetting();

    void saveMainWindowInitState();
    void saveMainWindowState();
    void readMainWindowState();
    void resetMainWindowState();

    void    saveResultOutputImageFormat(int format);
    int     readResultOutputImageFormat();
    void    saveResultOutputImageSizeType(int type);
    int     readResultOutputImageSizeType();
    void    saveResultOutputImageSizeRatio(float ratio);
    float   readResultOutputImageSizeRatio();
    void    saveResultOutputImageSizeW(int w);
    int     readResultOutputImageSizeW();
    void    saveResultOutputImageSizeH(int h);
    int     readResultOutputImageSizeH();
    void    saveResultOutputImageColorvalType(int type);
    int     readResultOutputImageColorvalType();
    void    saveResultOutputImageColorvalMin(const QString& min_value);
    QString readResultOutputImageColorvalMin();
    void    saveResultOutputImageColorvalMax(const QString& max_value);
    QString readResultOutputImageColorvalMax();
    void    saveResultOutputImageColorbar(bool check);
    bool    readResultOutputImageColorbar();
    void    saveResultOutputImageSpecifyRange(bool check);
    bool    readResultOutputImageSpecifyRange();
    void    saveResultOutputImageDispVox(bool check);
    bool    readResultOutputImageDispVox();
    void    saveResultOutputImageBackgroundChange(bool check);
    bool    readResultOutputImageBackgroundChange();
    void    saveResultOutputImageBackgroundColor(QColor color);
    QColor  readResultOutputImageBackgroundColor();
    void    saveResultOutputImageBackgroundGrad(bool check);
    bool    readResultOutputImageBackgroundGrad();

    void initResultOutputImage();

    void   saveVoxOutputImageFormat(int format);
    int    readVoxOutputImageFormat();
    void   saveVoxOutputImageSizeType(int type);
    int    readVoxOutputImageSizeType();
    void   saveVoxOutputImageSizeRatio(float ratio);
    float  readVoxOutputImageSizeRatio();
    void   saveVoxOutputImageSizeW(int w);
    int    readVoxOutputImageSizeW();
    void   saveVoxOutputImageSizeH(int h);
    int    readVoxOutputImageSizeH();
    void   saveVoxOutputImageDispVox(bool check);
    bool   readVoxOutputImageDispVox();
    void   saveVoxOutputImageFitDispVox(bool check);
    bool   readVoxOutputImageFitDispVox();
    void   saveVoxOutputImageBackgroundChange(bool check);
    bool   readVoxOutputImageBackgroundChange();
    void   saveVoxOutputImageBackgroundColor(QColor color);
    QColor readVoxOutputImageBackgroundColor();
    void   saveVoxOutputImageBackgroundGrad(bool check);
    bool   readVoxOutputImageBackgroundGrad();

    void initVoxOutputImage();

protected:
    void saveMainWindowState(QMainWindow* main_window, SettingsWrapper& settings);
    void readMainWindowState(QMainWindow* main_window, SettingsWrapper& settings);

protected:
    Vox3DForm*     m_parent;
    Ui::Vox3DForm* ui;

    /// uiで足りない設定値
    int    m_light_dir;
    QColor m_backgound_color;
    QColor m_dimension_color;
    QColor m_auto_dimension_color;
    QColor m_voxel_wire_color;
    QColor m_voxel_opt_base_color;

    SettingCtrl m_setting_ctrl;

    QScopedPointer<SettingsWrapper> m_init_geometry_setting;
};
#endif    // SETTINGGUICTRL_H
