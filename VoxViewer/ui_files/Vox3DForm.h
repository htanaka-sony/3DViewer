#ifndef VOX3DFORM_H
#define VOX3DFORM_H

#include <QWidget>

// #include "qobj3dreader.h" //openGL基礎表示のため
// #include "voxRead_makeGLdata.h" //voxファイル形式から,openGL描画形式のデータへの整形
#include "CommonSubClass.h"    //カスタマイズしたコンボボックス部品

#include <QCloseEvent>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QSettings>
#include <QStandardPaths>

//-start- Vox3DFormSub02readVoxMakeGL.cpp
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QObject>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QVector2D>
#include <QVector3D>
#include <QVector>

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QOpenGLWindow>

#include <gl/GL.h>
#include <math.h>
#include <QArrayData>
#include <QElapsedTimer>
#include <QMatrix4x4>
#include <QMouseEvent>
// #include <QMouseEventTransition>
#include <QOpenGLShader>
#include <QOpenGLVertexArrayObject>
#include <QScopedPointer>
#include <QVector3D>
#include <QtOpenGL/QtOpenGL>
// #include <mainwindow.h>
// #inlude  <ui_mainwindow.h>
// #include <vertex.h>
// #include <QTextCodec>
// For Japanese
#ifdef EYERIS_3D_VISUALIZATION
    #define TR(s) (s)
#else
    #define TR(s) (QString::fromLocal8Bit(s))
#endif
#include <QFile>        //kuroda
#include <QFileInfo>    //kuroda
#include <QHash>
#include <QMultiHash>
#include <tuple>    //kuroda 2021.06.xx-01 //2つ以上の帰り値を返すため　std::tuple, std::forward_as_tuple, std::tie で使用する
//-end- Vox3DFormSub02readVoxMakeGL.cpp

//-start- for-vox
#include <math.h>
#include <QPixmap>
#include <QProgressDialog>
#include <QtCore/QThread>
//-end- for-vox

#include "VoxViewerGlobal.h"

#include "Scene/SceneGraph.h"
using namespace Core;

struct QOpenGLTriangle3D_vox {
    QVector3D p1;
    QVector3D p2;
    QVector3D p3;
    QVector3D p1Normal;
    QVector3D p2Normal;
    QVector3D p3Normal;
    QVector2D p1UV;
    QVector2D p2UV;
    QVector2D p3UV;

    //-start- for-vox
    QVector3D color;
    float     acolor;
    int       matnum;
    //-end-
};

//-end- Vox3DFormSub02readVoxMakeGL.cpp

namespace Ui {
class Vox3DForm;
}

class ClippingCtrl;
class SettingGuiCtrl;
class DimensionCtrl;
class ResultCtrl;
class UtilityCtrl;
class MemoryMappedFile;

class VOXVIEWER_EXPORT Vox3DForm : public QWidget {
    Q_OBJECT

public:
    explicit Vox3DForm(QWidget* parent = nullptr);
    ~Vox3DForm();

    QOpenGLWidget* openGLWidget();

    bool eventFilter(QObject* obj, QEvent* event) override;
    void installEventFiltersForAllUiMembers(QObject* uiObject);

    QVector<QOpenGLTriangle3D_vox> g_vox_triangles;

    //-start- Vox3DFormSub02readVoxMakeGL.cpp
    void func_01main_GL_makeGL3DfromVox(QVector<QOpenGLTriangle3D_vox>& triangles, QString in_voxfilepath);
    // void func_GL_make_getPointOfMesh(QVector<QOpenGLTriangle3D_vox> &triangles, QVector<QVector3D> in_voxXYZVec,
    // QStringList in_voxSurfaceStrList, QVector<QVector3D> in_voxColors, QVector<int>in_voxMatnumVec);
    // //voxファイルからの取得情報を、openGL描画形式のデータに置き換える void
    // func_GL_make_getPointOfMesh_by2point(QVector<QOpenGLTriangle3D_vox> &triangles, QVector<QVector3D>
    // in_voxXYZStartVec, QVector<QVector3D> in_voxXYZEndVec, QStringList in_voxSurfaceStrList, QVector<QVector3D>
    // in_voxColors, QVector<int>in_voxMatnum); //voxファイルからの取得情報を、openGL描画形式のデータに置き換える
    void func_GL_make_getPointOfMesh_by2point(
        QVector<QOpenGLTriangle3D_vox>& triangles, QVector<QVector3D> in_voxXYZStartVec,
        QVector<QVector3D> in_voxXYZEndVec, QStringList in_voxSurfaceStrList, QVector<int> in_voxMatnum,
        QVector<QVector3D> in_voxColors,
        QVector<float> in_voxAcolors);    // voxファイルからの取得情報を、openGL描画形式のデータに置き換える
    void zDEBUG_input_getPointOfMesh_by2point_v2();    //[DEBUG]用途
                                                       // 長方形面の始点・終点座標から、直方体1個を表示するための値設定
    void func_vox_get_voxGraffic(QString in_voxfilepath);    // voxファイルから、座標とマテリアル情報を取得する　1点ごと
    void zDEBUG_vox_get_voxGraffic_DEBUG01(QString in_voxfilepath);    //[DEBUG] 例：　正方形1つ表示 正常表示される.
    void zDEBUG_vox_get_voxGraffic_DEBUG02(
        QString in_voxfilepath);    //[DEBUG] 例：　閉じてない面だけの3D形状　→　裏に色が付かない.　これから対処する　or
                                    // voxは全て閉じているはずなので問題にならないかもしれない。
    // ok//void input_getPointOfMesh_by2point_jiku(); //X,Y,Z軸を図形に付け足して表示する
    // 長方形面の始点・終点座標から、直方体1個を表示するための値設定
    void input_getPointOfMesh_by2point_jiku(QStringList& in_voxSurfaceStrList, QVector<QVector3D>& in_voxXYZStartVec,
                                            QVector<QVector3D>& in_voxXYZEndVec, QVector<int>& in_voxMatnumVec,
                                            QVector<QVector3D>& in_voxColorVec, QVector<float>& in_voxAcolorVec);
    void input_getPointOfMesh_by2point_rect(QVector3D in_startVec, QVector3D in_endVec, QVector3D in_colorVec,
                                            QStringList& in_voxSurfaceStrList, QVector<QVector3D>& in_voxXYZStartVec,
                                            QVector<QVector3D>& in_voxXYZEndVec, QVector<int>& in_voxMatnumVec,
                                            QVector<QVector3D>& in_voxColorVec, QVector<float>& in_voxAcolorVec);

    QVector<QVector3D> g_voxXYZVec;    // vox情報　GL座標情報(triangle)にも使用する
    QVector<QVector3D> g_voxXYZStartVec, g_voxXYZEndVec;
    QVector<QVector3D> g_voxColorVec;    // vox情報 色　GL座標情報(triangle)にも使用する
    QVector<float> g_voxAcolorVec;    // vox情報-GL表示用　半透明　GL座標情報(triangle)にも使用する
    QVector<int> g_voxMatnumVec;    // vox情報　マテリアル番号  // GL座標情報(triangle)への代入はなし
    QStringList g_voxSurfaceStrList;    // vox→3Dにした場合の、面の向き(Front, Back, Right, Left, Top, Bottom)  //
                                        // GL座標情報(triangle)への代入はなし
    QMultiHash<int, int>
        g_voxMatToPnumHash;    // マテリアル番号(matnum) と
                               // g_voxXYZvecインデックス番号(pnum)の紐づけ。　GUI操作色切替え・表示切替えのため。　pnum
                               // = positionNumber=インデックス番号の意味。
    int g_pnum;    // g_voxMatToPnumHash用途　マテリアル番号(matnum) と g_voxXYZvecインデックス番号(pnum)の紐づけ。
    // QVector<QVector3D> g_GLColors;
    QHash<int, QString> g_hash_matToName;    // マテリアル番号からマテリアル名取得用のハッシュ
    QHash<int, int> g_hash_matToViewOn;    // マテリアル番号から、描画表示or非表示情報取得用のハッシュ
    QHash<int, QString> g_hash_matToColorName;    // マテリアル番号から、描画の色名取得用のハッシュ
    QHash<int, QVector3D> g_hash_matToColorVec;    // マテリアル番号から、描画の色コード取得用のハッシュ
    QHash<int, float> g_hash_matToAcolor;    // マテリアル番号から、表示情報：半透明の取得用のハッシュ

    QVector3D func_GL_defineColor(int input_num);
    QVector3D func_GL_defineColor_nameToRGBvec(
        QString input_colorname);    // Redなどの名前から QVector(1,0,0) を返り値とする

    //-start-　流用　　opengl_myk miwidget.cpp　Vox3DFormSub02readVoxMakeGL.cpp
    // voxファイル読み込み用途
    // QStringList g_DrawMeshXYZList; //[DEBUG]用途　使わない。
    // dragFilePathList.clear();　//　使わない。
    QString g_voxfilePath;
    int***  g_aryVox;

    // メッシュサイズ
    float meshsizeVox;    // Voxファイルの unitlengthで定義されているメッシュサイズ
    float meshsizeVox_x;
    float meshsizeVox_y;
    float meshsizeVox_z;
    float meshsizeGL;    // openGLで表示させるために正規化したメッシュサイズ
    float meshsizeGL_x;
    float meshsizeGL_y;
    float meshsizeGL_z;

    // 総メッシュ数
    int          nx, ny, nz;
    unsigned int meshAll;
    // void func_vox_getMateNumOfMesh(QString in_voxfilepath); //voxファイルのヘッダー情報読み込み +
    // voxファイルを読み込んで、メッシュごとにマテリアルNo.を割り当てる
    void func_vox_getMateNumOfMesh_getVoxLine(
        QString in_voxfilepath);    // 3D配列に入れる //voxファイルのヘッダー情報読み込み +
                                    // voxファイルを読み込んで、メッシュごとにマテリアルNo.を割り当てる
    // void func_vox_GLadd_checkMateNumOfAdjoinMesh();
    // //openGL描画用の情報を入れる。　(流用元checkMateNumOfAdjoinMeshでは、通常座標としての記録のみだったが、処理内容変更。）
    void func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine();
    void func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine_Front();
    void func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine_Back();
    void func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine_Left();
    void func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine_Right();
    void func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine_Bottom();
    void func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine_Top();

    QVector<int> mateNoOfMesh;
    // メッシュごとのマテリアルNo
    unsigned int meshNo = 0;

    // メッシュNo. および面情報
    QMultiMap<int, unsigned int>  DrawMesh;
    QMultiHash<unsigned int, int> DrawSurface;

    // voxに登録されている全マテリアルのNoと名前
    QList<int>  mateNoList;
    QStringList mateNameList;
    //-end-

    int g_DEBUGMODE;

    int g_flag_notRedraw;    // 0=再描画しない 1=再描画する

    QHash<QString, QString> g_hash_matnameToCname;    // 共通値参照ハッシュ　キー：マテリアル名　値:色名(greenなど)
    QHash<QString, QColor>
        g_hash_cnameToQtColor;    // 共通値参照ハッシュ　キー：色名(greenなど)　値:QColor型 例→QColor(0,128,0,255)
    QHash<QString, QString>
        g_hash_cnameToStyleColor;    // 共通値参照ハッシュ　キー：色名(greenなど)　値:String型　例→ "#008000"
    QHash<QString, QList<float>>
        g_hash_cnameToRGBColor;    // 共通値参照ハッシュ　キー：色名(greenなど)　値:QList型 例: {1, 0, 0}
    QStringList g_list_whitefontBgColor;

    void func_comboBox_voxRotate_translateRotate();    // 簡易回転(水平方向): 物体自身の回転による動作
                                                       // [translate]タブ-[rotate]
    void func_comboBox_voxRotate_eyeChange();    //-start- 旧案1　簡易回転(水平方向): カメラ位置変更による動作
                                                 //[LookAt]タブ-[eye] //
                                                 // コーディングで動作確認のため、使うのみ。　ユーザー用は　別関数。
    void func_setDefaultViewMenu();    // 視点の初期化

    void setVoxFile(const QString& path);
    void setFdtdFile(const QString& path);

    void contextMenuEvent(QContextMenuEvent* event) override;

    static QString colorStyle(QColor color);

    void setResultStyleDefaultCombobox();

    static bool isModalVisible(QWidget* widget);

    void closeProcess();

public slots:
    //-start- Vox3DFormSub02readVoxMakeGL.cpp
    int         fileWrteForWindows(QString WriteFilePath, QString WriteMode, QStringList msgList);
    QStringList readTextFileAll(QString fileName);
    //-end- Vox3DFormSub02readVoxMakeGL.cpp

    void func_slot_mousewheelZoom(float in_modelScale);    // for vox
    void func_slot_mouseRotate(
        float in_rotateAngleY,
        float in_rotateAngleZ);    // マウスドラッグで、図形回転。 GUI図形平行移動チェックボックスOFFの場合。
    void func_slot_mouseDrag02(
        int in_mouseMoveX,
        int in_mouseMoveY);    // マウスドラッグ時の動作。GUI図形平行移動チェックボックス　OFFの場合->図形回転,
                               // ONの場合->図形平行移動。
    void func_slot_mouseVoxRotate(
        int in_mouseMoveX,
        int in_mouseMoveY);    // マウスドラッグ時の動作。GUI図形平行移動チェックボックス　OFFの場合->図形回転。
    void func_slot_mouseVoxShift(
        int in_mouseMoveX,
        int in_mouseMoveY);    // マウスドラッグ時の動作。GUI図形平行移動チェックボックス　ONの場合->図形平行移動。
    void func_slot_mousePress(float in_x, float in_y);      // マウスボタン押したときの動作
    void func_slot_mouseRelease(float in_x, float in_y);    // マウスボタン離した時の動作

    void func_slot_3D_acceptVoxpath(QString inStr);

    void selectMaterialList(int material_id);

    bool enableMultiLoad(const QString& path, QString& ext);
    void allLoadTargetCheck(const QStringList& path_list, bool& has_valid_folder, bool& has_valid_file);
    void allLoadForDD(const QStringList& path_list);
    void allLoad(const QStringList& path_list, bool op2_import_mode = true);
    bool allLoad(const QString& path, const QString& specified_ext = "", bool op2_import_mode = true);
    bool allLoad(const QStringList& pathes, const QString& ext, bool op2_import_mode = true);
    bool voxpathLoad(const QString& path);
    bool optLoad(const QString& path);
    bool op2Load(const QStringList& path, bool op2_import_mode);

    static bool replaceShortCutPath(QString& path);

    static void fdtdLoad(const QString& path, QHash<QString, QString>& hash_matname_to_cname);

    void suppressUpdata() { g_flag_slotMatView = 0; }
    void unsuppressUpdata()
    {
        g_flag_slotMatView = 1;
        func_tableMaterialToGL();
    }

    void currentVoxDrawMode(bool& shading, bool& wireframe);

    void allShow(bool show);
    void voxAllShow(bool show);
    void optAllShow(bool show);
    void op2AllShow(bool show);
    void dimensionAllShow(bool show);
    void autoDimensionAllShow(bool show);
    void voxOnlyShow(bool show, bool show_self);
    void optOnlyShow(bool show, bool show_self);
    void op2OnlyShow(bool show, bool show_self);
    void dimensionOnlyShow(bool show, bool show_self);
    void autoDimensionOnlyShow(bool show, bool show_self);

private slots:
    void on_persVerticalAngleSpin_valueChanged(double arg1);

    void on_persNearSpin_valueChanged(double arg1);

    void on_persFarSpin_valueChanged(double arg1);

    void on_lookEyeXSpin_valueChanged(double arg1);

    void on_lookEyeYSpin_valueChanged(double arg1);

    void on_lookEyeZSpin_valueChanged(double arg1);

    void on_lookCenterXSpin_valueChanged(double arg1);

    void on_lookCenterYSpin_valueChanged(double arg1);

    void on_lookCenterZSpin_valueChanged(double arg1);

    void on_lookUpXSpin_valueChanged(double arg1);

    void on_lookUpYSpin_valueChanged(double arg1);

    void on_lookUpZSpin_valueChanged(double arg1);

    void on_scaleSpin_valueChanged(double arg1);

    void on_translateXSpin_valueChanged(double arg1);

    void on_translateYSpin_valueChanged(double arg1);

    void on_translateZSpin_valueChanged(double arg1);

    void on_rotateAngleSpin_valueChanged(double arg1);

    void on_rotateXSpin_valueChanged(double arg1);

    void on_rotateYSpin_valueChanged(double arg1);

    void on_rotateZSpin_valueChanged(double arg1);

    void on_lightPosXSpin_valueChanged(double arg1);

    void on_lightPosYSpin_valueChanged(double arg1);

    void on_lightPosZSpin_valueChanged(double arg1);

    // void on_actionAbout_Qt_triggered();

    // void on_action_Open_triggered();

    // void on_actionExit_triggered();

    void on_persVerticalAngleSlider_valueChanged(int value);

    void on_persNearSlider_valueChanged(int value);

    void on_persFarSlider_valueChanged(int value);

    void on_lightKd1Spin_valueChanged(double arg1);

    void on_lightKd2Spin_valueChanged(double arg1);

    void on_lightKd3Spin_valueChanged(double arg1);

    void on_lightLd1Spin_valueChanged(double arg1);

    void on_lightLd2Spin_valueChanged(double arg1);

    void on_lightLd3Spin_valueChanged(double arg1);

    void on_textureFileEdit_returnPressed();

    void on_browseTextureBtn_pressed();
    // void on_pushButton_matTableToGL_clicked();

    void on_rotateAngleSlider_valueChanged(int value);
    // void on_vox_pushButton_setDefaultViewMenu_clicked();

    void on_lineEdit_rotateAngleZ_editingFinished();
    void on_comboBox_voxLightPos_currentTextChanged(const QString& arg1);

    //-start- for-vox
    // void on_DEBUG01_pushButton_clicked();
    void on_checkBox_acolor_stateChanged(int arg1);
    void on_pushButton_open_files_clicked();
    void on_pushButton_open_folder_clicked();
    void on_checkBox_project_opt_stateChanged(int arg1);
    void on_pushButton_open_vox_clicked();
    void on_pushButton_open_opt_clicked();
    void on_pushButton_open_op2_clicked();
    void on_pushButton_import_op2_clicked();
    void on_pushButton_voxpathLoad_clicked();
    void on_pushButton_voxpathApply_clicked();
    void on_pushButton_voxpathClear_clicked();
    void on_pushButton_optLoad_clicked();
    void on_pushButton_optApply_clicked();
    void on_pushButton_optClear_clicked();
    void func_tableMateriaRedrawSlot(int);    // ユーザー操作で、チェックボックス変更を, simNameテーブルに記録する
    void func_tableMaterialColorSlotChanged(int);    // ユーザー操作で、マテリアル色選コンボボックスが変わった場合の処理
    void on_lineEdit_rotateAngleY_editingFinished();
    void on_pushButton_viewAllOn_clicked();
    void on_pushButton_viewAllOff_clicked();
    // void on_checkBox_voxshift_stateChanged(int arg1);
    // void on_comboBox_voxRotate_currentIndexChanged(const QString& arg1);
    // void on_comboBox_voxRotateV_currentIndexChanged(const QString& arg1);
    void on_vox_comboBox_mouseAct_currentTextChanged(const QString& arg1);
    void vox_fdtd_load(const QString& path);
    void on_vox_fdtd_load_pushButton_clicked();
    void on_vox_fdtd_apply_pushButton_clicked();
    void on_vox_fdtd_clear_pushButton_clicked();
    void on_pushButton_about_3dviewer_clicked();
    //-end- for-vox

    void voxMaterialContextMenu(const QPoint& pos);
    void voxMaterialShow(bool show);
    void voxMaterialColor();
    void voxMaterialTrans(bool on);
    void voxMaterialDrawMode(int mode, bool all);
    void voxMaterialOpt(bool on);

private:
    Ui::Vox3DForm* ui;

    // void closeEvent(QCloseEvent *bar);

    void updateEverything();

    //-start- for-vox
    void func_setGUIstartup();    // スタート時のGUI設定
    void func_tableMaterialDefaultSet(
        QStringList materialLineList);    // materialテーブルのセット。マテリアルごとに表示・非表示,
                                          // 色の変更などの用途。
    void
    func_tableMaterialDefaultSet_matnameToColorCombo();    // materialテーブルのセット。特定のマテリアルの場合、色設定コンボボックスを特定の色にセットする。
    QStringList func_getMaterialfromVoxPath(QString in_filepath);    // voxfile からマテリアル行のみ取得する。

    // 使わない//int g_rowChecked_material(); //テーブル上のチェックボックスのsignal&slotアクション時に使う変数
    QStringList func_tableMaterial_gval();    // QStringList func_gval_table_material();  //GL描画処理用の別cpp
                                              // への値渡しのため、グローバル変数を更新する。
    void func_tableMaterialColorAdd(int row);    // マテリアルテーブル　色選択コンボボックスの設定。
    int g_flag_slotMatColorCombo;    // マテリアルコンボの値変更時にスロット処理させる:1 or
                                     // させない:0。（ユーザー操作の時だけスロット処理したい。自動設定のとき（自動=初期設定の時など）はスロット処理したくない）
    int g_flag_slotMatView;          // 表示・非表示切替え　スロット処理させる:1 or
                               // させない:0。（ユーザー操作の時だけスロット処理したい。自動設定のとき（自動=初期設定の時など）はスロット処理したくない）
    void func_tableMaterialToGL();    // for-vox
                                      // GL描画更新：ユーザーによるGUI操作された時点で、材質ごとの表示切替え・色切替え
    QHash<int, QVector3D>
    func_GL_defineColor_byTableMaterial();    // マテリアルテーブルからハッシュ作成する。　マテリアル番号と色番号
                                              // QVector3D(R, G, B) を紐づける

public:
    SceneGraph*     sceneGraph() { return m_scene_graph.ptr(); }
    ClippingCtrl*   clippingCtrl() { return m_clipping_ctrl; }
    DimensionCtrl*  dimensionCtrl() { return m_dimension_ctrl; }
    ResultCtrl*     resultCtl() { return m_result_ctrl; }
    UtilityCtrl*    utilityCtrl() { return m_utility_ctrl; }
    SettingGuiCtrl* settingGuiCtrl() { return m_setting_gui_ctrl; }

public:
    QList<QDockWidget*>& dockWidgets() { return m_dock_widgets; }
    QSet<QDockWidget*>   tabActiveWidgets();

    /// TEST
    QDockWidget* clippingDockWidget();
    QDockWidget* dimensionDockWidget();

public:
    static inline const QString m_mouse_rotate           = QString("Rotate");
    static inline const QString m_mouse_pan              = QString("Pan");
    static inline const QString m_mouse_nop              = QString("None");
    static inline const QString m_projection_ortho       = QString("平行投影");
    static inline const QString m_projection_perspective = QString("透視投影");

private:
    //// データ構造管理
    RefPtr<SceneGraph> m_scene_graph;

    /// クリッピング
    ClippingCtrl* m_clipping_ctrl = nullptr;

    /// 寸法作成
    DimensionCtrl* m_dimension_ctrl = nullptr;

    /// 結果
    ResultCtrl* m_result_ctrl = nullptr;

    /// Utility
    UtilityCtrl* m_utility_ctrl = nullptr;

    /// 設定管理
    SettingGuiCtrl* m_setting_gui_ctrl = nullptr;

public:
    QList<QDockWidget*> m_dock_widgets;
};

#endif    // VOX3DFORM_H
