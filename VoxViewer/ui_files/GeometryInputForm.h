#ifndef GEOMETRYINPUTFORM_H
#define GEOMETRYINPUTFORM_H

#include <QClipboard>    //2019.03.18-01
#include <QCloseEvent>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QMainWindow>
#include <QMouseEvent>
#include <QWidget>

//-start- GeometrySubScene(MouseEvent)
#include <GeometrySubScene.h>
#include <QResizeEvent>
#include <QTimer>
//-end- GeometrySubScene(MouseEvent)

// #define TR(s) (QString::fromLocal8Bit(s)) //2018.09.12 日本語表示対応(メッセージボックスなど）
#ifndef TR
    #define TR(s) (s)    // 2018.09.12 日本語表示対応(メッセージボックスなど）
#endif

// QT_BEGIN_NAMESPACE
// namespace Ui { class GeometryInputForm; }
// QT_END_NAMESPACE
namespace Ui {
class GeometryInputForm;
}

// org//class GeometryInputForm : public QMainWindow
class GeometryInputForm : public QWidget {
    Q_OBJECT

    //-start- org2021.10.06 start-
    // public:
    //    GeometryInputForm(QWidget *parent = nullptr);
    //    ~GeometryInputForm();
    //-end- org 2021.10.06 start-
public:
    explicit GeometryInputForm(QWidget* parent = nullptr);
    ~GeometryInputForm();

    QPixmap pix_XZscene, pix_YZscene,
        pix_XYscene;    // 2018.06.xx-01 outputVolume表示用 //参考元から流用: GeometryInputForm.h
    QPixmap pix_XZdiag_scene;

    //-start- 2019.04.11 lineEditカスタム getUIfromOtherClass
    // static GeometryInputForm * getGIwinPtr();
    // QString getTextToOherWin(QString); //UI取得用
    //-end- 2019.04.11 lineEditカスタム getUIfromOtherClass

    // void acceptedMaterials(QStringList, QStringList, QStringList);
    QStringList acceptedMaterialList, acceptedMaterialcolorList, acceptedMaterialcategoryList,
        acceptedMaterialListforVox;    // 2018.10.04 追加

    // QString evaluateStr(QString str);

    void func_load_OfficialList();    // Eyeris起動時にgetしたOfficialListをロードする。

    //-start- voxfile情報 2021.06.xx-01 3Dvieweｒの2D断面表示処理用途
    QString g_orgMatNumStr;    // 例: "0,4,5,6,12,22,23,27,28";
    QString
        g_orgMatNameStr;    // 例:
                            // "Air,CF-9th-B_Woollam,CF-9th-G_Woollam,CF-9th-R_Woollam,CT_Woollam,SCF-AO_SCK,SCF-PT_SCK,SiO2_SCK,Si";
    int*** g_voxDomain;                              // 読み込んだ voxファイルの形状データ部分全体
    void func_vox_readHeader(QString in_voxpath);    // 2021.06.xx-01 voxファイルのヘッダー箇所の情報取得
    void        func_vox_readTo3Darray();            // voxファイルの3次元配列への格納
    QStringList g_VoxMatIDList, g_VoxMatNameList;

    int g_flag_slotMatColorCombo;    // マテリアルコンボの値変更時にスロット処理させる:1 or
                                     // させない:0。（ユーザー操作の時だけスロット処理したい。自動設定のとき（自動=初期設定の時など）はスロット処理したくない）
    int g_flag_slotMatView;          // マテリアルコンボの値変更時にスロット処理させる:1 or
                               // させない:0。（ユーザー操作の時だけスロット処理したい。自動設定のとき（自動=初期設定の時など）はスロット処理したくない）
    int g_flag_slotAutorun;    // スロット処理させる:1 or
                               // させない:0。（ユーザー操作の時だけスロット処理したい。自動設定のとき（自動=初期設定の時など）はスロット処理したくない）

    QStringList func_getMaterialfromVoxPath(QString in_filepath);    // voxfile からマテリアル行のみ取得する。
    void func_tableMaterialDefaultSet(
        QStringList in_materialLineList);    // materialテーブルのセット。マテリアルごとに表示・非表示,
                                             // 色の変更などの用途。
    void
    func_tableMaterialDefaultSet_matnameToColorCombo();    // materialテーブルのセット。特定のマテリアルの場合、色設定コンボボックスを特定の色にセットする。
    int** g_XZdiagNow2d;    // 現時点で表示されている斜め断面の形状データ（1面のみ）　ShapeInfoなどで参照するため。
    int g_nx, g_ny, g_nz;
    int g_nxdiag, g_nydiag;

    void func_GUI_defaultSet();    // ファイルロードし直した時など、GUI, グローバル変数を初期状態にする
    //-end- voxfile情報 2021.06.xx-01 3Dvieweｒの2D断面表示処理用途

    int Ycount_pixCenter, Xcount_pixCenter,
        XZdiag_Ycount_pixCenter;    // 画素中央ボタンでの、セルアレイの何個目を見るかのカウント用途

    void func_vox_load(QString in_voxfilepath);    // voxfile読込表示する

    int g_flag_keyCTRL;    // ショートカットキー AutoLength用途(ctrl+左マウスクリック）

    QHash<QString, QColor>
    func_hashQtcolor();    // ハッシュ作成して渡す    ハッシュキー色名(greenなど)　ハッシュ値：Qcolor 例：QColor(0, 128,
                           // 0, 255)

    QHash<QString, QString> g_hash_matnameToCname;    // 共通値参照ハッシュ  キー:マテリアル名　   値：色名 green　など
    QHash<QString, QColor>
        g_hash_cnameToQtColor;    // 共通値参照ハッシュ キー:色名(greenなど)　値：Qcolor 例：QColor(0, 128, 0, 255)
    QHash<QString, QString>
        g_hash_cnameToStyleColor;    // 共通値参照ハッシュ  キー:色名(greenなど)　値： 例：String型 #008000
    QStringList g_list_whitefontBgColor;

private:
    Ui::GeometryInputForm* ui;

    void closeEvent(QCloseEvent* bar);

    //-start- 参考元から流用: GeometryInputForm.h
    // static GeometryInputForm * pGIwin; //2019.04.11 lineEditカスタム getUIfromOtherClass

    GeometrySubScene* XZscene;
    GeometrySubScene* YZscene;
    GeometrySubScene* XYscene;

    GeometrySubScene* XZdiag_scene;    // 2021.04.xx-01 vox-2Dviewer 斜め断面

    int   FLAG_ZOOM, FLAG_LENGTH;
    int   X1_ZOOM, Y1_ZOOM, X2_ZOOM, Y2_ZOOM;
    int   X1_CLICK, Y1_CLICK, X2_CLICK, Y2_CLICK;
    float X1_CLICK_f, Y1_CLICK_f, X2_CLICK_f, Y2_CLICK_f;    // for-vox 斜め断面 XZdiagのLength処理

    // 2018.05.18 VoxCut
    int FLAG_VOXCUT, X1_VOXCUT, X2_VOXCUT, Y1_VOXCUT, Y2_VOXCUT;

    // FLAG_VIEWER_UPDATE = 1 の場合のみ、ビューアを更新する。(関数 viewer_update)。
    // それ以外の場合はリアルタイム更新はしない(画面フリーズ防止のため）。
    int FLAG_VIEWER_UPDATE;

    // 関数update_Xcoordinate_Slider() connectにより2回処理されてしまうことを防止　(Y,Z関数でも共通使用）
    int COUNT_SPINBOX_RUN;

    void func_view_Zoom_box(int, int);
    void func_view_Length(int, int);
    void func_view_Length_float(float IN_X_f, float IN_Y_f);    // for-vox Length測長処理 斜め断面 XZdiagビューの場合
    void func_view_SelectedObjectRect(QString);                 // ※オブジェクト用
    void func_view_SelectedShapeInfo_vox(int, int);    // 2021.04.xx-01 vox-2Dviewer用途として内容変更
    void func_view_ruler();                            // ruler描画
    int rulerXZ_X, rulerXZ_Y, rulerYZ_X, rulerYZ_Y, rulerXY_X, rulerXY_Y, rulerXZdiag_X, rulerXZdiag_Y;    // 初期は9999

    // Length 複数表示版
    void         func_view_LengthAll();
    QList<int>   lengthXZ_X, lengthXZ_Y, lengthYZ_X, lengthYZ_Y, lengthXY_X, lengthXY_Y;
    QList<float> lengthXZdiag_X, lengthXZdiag_Y;

    void                 swap(int* val1, int* val2);
    QBrush               Brushes[50];
    QGraphicsPixmapItem* item1;
    void                 func_setBrushes();
    int                  func_setBrushesNum(QString);

    void func_ImgSaveAll();
    void func_ImgSaveAll_Clipboard();

    // 自動測長処理=Autolength用途
    float g_autoXZ_X, g_autoXZ_Y, g_autoYZ_X, g_autoYZ_Y, g_autoXY_X, g_autoXY_Y, g_autoXZdiag_X,
        g_autoXZdiag_Y;    // 初期は9999
    QString g_autoXZ_direction, g_autoYZ_direction, g_autoXY_direction,
        g_autoXZdiag_direction;    // 縦方向測長=V or 横方向測長=H
    // 自動測長処理 押下された1点から 垂直or水平にマテリアルごとに測長表示する
    void func_view_AutoLength();
    // 自動測長処理 押下された1点から
    // 垂直or水平にマテリアルごとに測長する。多数のポイントについて一括バッチ処理できるように関数化 + ui を使わない。
    void func_view_AutoLength_batch(QStringList& res_matnameList, QList<float>& res_lengthfList, QString in_voxfilePath,
                                    int in_mesh, float in_selectX, float in_selectY, float in_selectZ, QString in_view,
                                    QString in_direction);
    // 自動測長処理 GUI入力値変更時の処理(セルサイズ、アレイ数)　自動測長のGUIをセットする
    void func_cellvalueChanged_setAutoLength();

public slots:
    void receiveCoord(float, float);    // for-vox 斜め断面のため,floatに変更 // void receiveCoord(int, int);
                                        // //クリックされた座標表示　マウス左クリックの場合
    void receiveCoordMouseRight(float,
                                float);    // for-vox 斜め断面のため,floatに変更 //void receiveCoordMouseRight(int,
                                           // int); //クリックされた座標表示　マウス右クリックの場合
    void receiveShortcutKey(QString);    // ショートカットキー 2020.03.09

    void func_slot_2D_acceptVoxpath(QString inStr);    // シグナルスロット
    void func_accepted_byMW_viewStart();               // MainWindowからのシグナルスロット。

private slots:
    //    int gcd(int a, int b);
    //    int lcm(int a, int b);
    // void make_XZgeometry(); //2021.06.xx thrash2D用途で関数置き換えのため、不使用にする　→置き換え後
    // func_vox_make_XZgeometry() void make_YZgeometry(); //2021.06.xx
    // thrash2D用途で関数置き換えのため、不使用にする　→置き換え後 func_vox_make_YZgeometry()
    void func_vox_make_XYgeometry();
    void viewer_update();

    //-start- 2021.06.xx-01　vox-2Dviewer
    void func_vox_make_XZgeometry();    // viewerXZの描画 voxファイル 3D配列データをもとにする
                                        // //XZについては、速度比較して、もと or 3次元配列版どちらを使うか検討する
    void func_vox_make_YZgeometry();    // viewerXZの描画 voxファイル 3D配列データをもとにする
    void func_vox_make_diagonal();      // 斜め断面表示 引数は "XZ" or "YZ"
    //-start- 2021.06.xx-01 vox-2Dviewer

    // void addrect(QString planestr, QString color, int pos1, int pos2, int w, int h);
    void addrect(QString planestr, QString color, int pos1, int pos2, int w, int h, QString InputTooltip);

    void update_Xcoordinate_spinBox();
    void update_Xcoordinate_Slider();
    void update_Ycoordinate_spinBox();
    void update_Ycoordinate_Slider();
    void update_Zcoordinate_spinBox();
    void update_Zcoordinate_Slider();
    //-start- 2021.04.xx-01 vox-2Dviewer 斜め断面
    void update_XZdiag_Ycoordinate_spinBox();
    void update_XZdiag_Ycoordinate_Slider();
    //-end-  2021.04.xx-01 vox-2Dviewer 斜め断面

    // 2019.05.16  スピンボックスを lineEditに変更したことによる関数追加
    void
    func_on_Xcoordinate_spin_lineEdit_editingFinished();    // viewerYZのYpositionのEditingFinished時の処理。　(EditingFinished
                                                            // , valueChanged両立　と
                                                            // sliderの連携のため個別関数として用意しスロットとして使う）
    void func_on_Xcoordinate_spin_upButton_clicked();      // viewerYZのspinBox代替▲ボタン押下時の処理
    void func_on_Xcoordinate_spin_downButton_clicked();    // viewerYZのspinBox代替▼ボタン押下時の処理
    void func_on_Xcoordinate_spin_valueInput(
        QString input_text);    // viewerYZのYpositionのEditingFinished時の処理。pinBox代替▲▼ボタン押下時の処理
    //-----
    void
    func_on_Ycoordinate_spin_lineEdit_editingFinished();    // viewerXZのYpositionのEditingFinished時の処理。　(EditingFinished
                                                            // , valueChanged両立　と
                                                            // sliderの連携のため個別関数として用意しスロットとして使う）
    void func_on_Ycoordinate_spin_upButton_clicked();      // viewerXZのspinBox代替▲ボタン押下時の処理
    void func_on_Ycoordinate_spin_downButton_clicked();    // viewerXZのspinBox代替▼ボタン押下時の処理
    void func_on_Ycoordinate_spin_valueInput(
        QString input_text);    // viewerXZのYpositionのEditingFinished時の処理。pinBox代替▲▼ボタン押下時の処理
    //-----
    void
    func_on_Zcoordinate_spin_lineEdit_editingFinished();    // viewerXYのYpositionのEditingFinished時の処理。　(EditingFinished
                                                            // , valueChanged両立　と
                                                            // sliderの連携のため個別関数として用意しスロットとして使う）
    void func_on_Zcoordinate_spin_upButton_clicked();      // viewerXYのspinBox代替▲ボタン押下時の処理
    void func_on_Zcoordinate_spin_downButton_clicked();    // viewerXYのspinBox代替▼ボタン押下時の処理
    void func_on_Zcoordinate_spin_valueInput(
        QString input_text);    // viewerXYのYpositionのEditingFinished時の処理。pinBox代替▲▼ボタン押下時の処理

    void XZshowgrid_checkBox_stateChanged();
    void YZshowgrid_checkBox_stateChanged();
    void XYshowgrid_checkBox_stateChanged();
    void XZdiag_showgrid_checkBox_stateChanged();    // 2021.04.xx-01 vox-2Dviewr
    void XZshowgrid_checkBox_isChecked();
    void YZshowgrid_checkBox_isChecked();
    void XYshowgrid_checkBox_isChecked();
    void XZdiag_showgrid_checkBox_isChecked();    // 2021.04.xx-01 vox-2Dviewr

    //    void XZshowoutput_checkBox_stateChanged();
    //    void YZshowoutput_checkBox_stateChanged();
    //    void XYshowoutput_checkBox_stateChanged();
    //    void XZshowoutput_checkBox_isChecked();
    //    void YZshowoutput_checkBox_isChecked();
    //    void XYshowoutput_checkBox_isChecked();
    //    void XZshowexcitationplane_checkBox_stateChanged();
    //    void YZshowexcitationplane_checkBox_stateChanged();
    //    void XZshowexcitationplane_checkBox_isChecked();
    //    void YZshowexcitationplane_checkBox_isChecked();

    // Redrawボタンが押されたら(常時はviewer画面更新しない。FLAG_VIEWER_UPDATE=1 とした場合のみ画面更新する）
    void on_XZRedraw_pushButton_clicked();
    void on_YZRedraw_pushButton_clicked();
    void on_XYRedraw_pushButton_clicked();

    void on_XZzoomin_pushButton_clicked();
    void on_YZzoomin_pushButton_clicked();
    void on_XYzoomin_pushButton_clicked();
    void on_XZdiag_zoomin_pushButton_clicked();    // 2021.04.xx-01 vox-2Dviewer 斜め断面
    void on_XZzoomout_pushButton_clicked();
    void on_YZzoomout_pushButton_clicked();
    void on_XYzoomout_pushButton_clicked();
    void on_XZdiag_zoomout_pushButton_clicked();    // 2021.04.xx-01 vox-2Dviewer 斜め断面
    void on_XZzoomall_pushButton_clicked();         // publicに移動
    void on_YZzoomall_pushButton_clicked();
    void on_XYzoomall_pushButton_clicked();
    void on_XZdiag_zoomall_pushButton_clicked();    // 2021.04.xx-01 vox-2Dviewer 斜め断面

    void on_XZlength_checkBox_clicked(bool checked);    // 測長
    void on_YZlength_checkBox_clicked(bool checked);
    void on_XYlength_checkBox_clicked(bool checked);
    void on_XZdiag_length_checkBox_clicked(bool checked);
    //-start- 2021.06.xx-01 vox-2Dviewer
    void on_vox_load_pushButton_clicked();
    void on_vox_fdtd_load_pushButton_clicked();

    void on_XZdiag_Ycoordinate_Slider_valueChanged(int value);
    void on_XZdiag_Ycoordinate_spin_lineEdit_editingFinished();
    void on_XZdiag_Ycoordinate_spin_upButton_clicked();
    void on_XZdiag_Ycoordinate_spin_downButton_clicked();
    void func_on_XZdiag_Ycoordinate_spin_valueInput(
        QString input_text);    // viewerXZのEditingFinished時の処理。spinBox代替▲▼ボタン押下時の処理

    void on_XZdiag_Redraw_pushButton_clicked();

    void func_tabWidget_currentChanged();    // XY-YZ-XY タブが切り替わった時に呼び出される関数。ビューアー更新する。

    void on_XZruler_checkBox_stateChanged(int arg1);
    void on_YZruler_checkBox_stateChanged(int arg1);
    void on_XYruler_checkBox_stateChanged(int arg1);
    void on_XZdiag_ruler_checkBox_stateChanged(int arg1);
    void on_XZruler_color_comboBox_currentIndexChanged(int index);
    void on_YZruler_color_comboBox_currentIndexChanged(int index);
    void on_XYruler_color_comboBox_currentIndexChanged(int index);
    void on_XZdiag_ruler_color_comboBox_currentIndexChanged(int index);

    void on_XZ_comboBox_currentTextChanged(const QString& arg1);
    void on_YZ_comboBox_currentTextChanged(const QString& arg1);
    void on_XY_comboBox_currentTextChanged(const QString& arg1);
    void on_XZdiag_comboBox_currentTextChanged(const QString& arg1);

    void func_tableMaterialColorSlotChanged(
        int);    // materialテーブルのセット。特定のマテリアルの場合、色設定コンボボックスを特定の色にセットする。

    void on_XZdiag_comboBox_angle_activated(const QString& arg1);

    void on_XZautolength_checkBox_stateChanged(int arg1);
    void on_YZautolength_checkBox_stateChanged(int arg1);
    void on_XYautolength_checkBox_stateChanged(int arg1);
    void on_XZdiag_autolength_checkBox_stateChanged(int arg1);
    void on_vox_pushButton_autolength_pointRun_clicked();
    void on_vox_radioButton_autolength_directionV_clicked(bool checked);
    void on_vox_radioButton_autolength_directionH_clicked(bool checked);
    void on_vox_comboBox_autolength_color_currentIndexChanged(const QString& arg1);
    void on_vox_checkBox_autolength_viewResult_clicked(bool checked);
    void on_vox_comboBox_autolength_fontsize_currentTextChanged(const QString& arg1);
    void on_vox_pushButton_help_clicked();
    void on_Ypos_pixCenter_pushButton_clicked();    // XZビュー　画素中央ボタン押下時の処理
    void on_Xpos_pixCenter_pushButton_clicked();    // YZビュー　画素中央ボタン押下時の処理

    // 自動測長 測長ポイント　画素中央モードの場合の処理
    void on_vox_pushButton_autolength_pointCenterRun_clicked();
    void on_cellX_lineEdit_editingFinished();
    void on_arrayX_lineEdit_editingFinished();
    void on_cellY_lineEdit_editingFinished();
    void on_arrayY_lineEdit_editingFinished();
    void on_vox_checkBox_autolength_viewResult_mat_clicked(bool checked);
    //

    void on_vox_pushButton_DEBUG_clicked();
};
#endif    // GEOMETRYINPUTFORM_H
