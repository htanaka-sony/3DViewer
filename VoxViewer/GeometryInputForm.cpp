#include "GeometryInputForm.h"
#include "ui_GeometryInputForm.h"

#include "CommonSubClass.h"    //-start- 2017.12.12 combo_scroll_stop
#include "MainWindow3DViewer.h"        // extern共通変数取得のため
// #include "ui_MainWindow.h"
#include "CommonSubFunc.h"

#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsRectItem>
#include <QGridLayout>
#include <QImage>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPointF>
#include <QRect>
#include <QRectF>
// #include <QRegExp>
#include <QSizeGrip>
#include <QStringList>
#include <QTextStream>
#include <QTime>
#include <QTransform>
// #include <QtScript>    //2021.04.xx-01 本番時は必要　後でインストールする

#include <math.h>
#include <QDesktopServices>
#include <QMessageBox>

void GeometryInputForm::closeEvent(QCloseEvent* bar)
{    // ×ボタンが押されても、画面を閉じないようにする
    bar->ignore();
}

// GeometryInputForm * GeometryInputForm::win = nullptr; //2019.04.11 lineEditカスタム getUIfromOtherClass

//-start- org
// GeometryInputForm::GeometryInputForm(QWidget *parent)
//    : QMainWindow(parent)
//    , ui(new Ui::GeometryInputForm)
//-end- org
GeometryInputForm::GeometryInputForm(QWidget* parent) : QWidget(parent), ui(new Ui::GeometryInputForm)
{
    ui->setupUi(this);

    this->setWindowTitle("2Dviewer");

    // pGIwin = this; //2019.04.11 lineEditカスタム getUIfromOtherClass

    func_setBrushes();    // QPixmap描画色のカスタム設定

    ui->vox_pushButton_DEBUG->setVisible(false);    // DEBUG用ボタン非表示

    //-start- GeometrySubScene(MouseEvent)
    // org //XZscene = new QGraphicsScene(this);
    // org //YZscene = new QGraphicsScene(this);
    // org //XYscene = new QGraphicsScene(this);
    //
    XZscene = new GeometrySubScene(this);
    YZscene = new GeometrySubScene(this);
    XYscene = new GeometrySubScene(this);
    //-end- GeometrySubScene(MouseEvent)
    ui->XZgraphicsView->setScene(XZscene);
    ui->YZgraphicsView->setScene(YZscene);
    ui->XYgraphicsView->setScene(XYscene);

    QBrush darkgrayBrush(QColor(46, 46, 46, 255));
    ui->XZgraphicsView->setBackgroundBrush(darkgrayBrush);
    ui->YZgraphicsView->setBackgroundBrush(darkgrayBrush);
    ui->XYgraphicsView->setBackgroundBrush(darkgrayBrush);
    // ui->OutlinegraphicsView->setBackgroundBrush(darkgrayBrush);

    //-start- 2021.04.xx-01 vox-2Dviewer 斜め断面
    XZdiag_scene = new GeometrySubScene(this);
    // YZdiag_scene = new GeometrySubScene(this);
    ui->XZdiag_graphicsView->setScene(XZdiag_scene);
    // ui->YZdiag_graphicsView->setScene(YZdiag_scene);
    ui->XZdiag_graphicsView->setBackgroundBrush(darkgrayBrush);
    // ui->YZdiag_graphicsView->setBackgroundBrush(darkgrayBrush);
    //-end- 2021.04.xx-01 vox-2Dviewer 斜め断面

    ui->Xcoordinate_Slider->setMinimum(1);
    ui->Xcoordinate_Slider->setMaximum(4000);
    ui->Xcoordinate_spin_lineEdit->setText("1");
    // ui->Xcoordinate_spinBox->setMinimum(1);
    // ui->Xcoordinate_spinBox->setMaximum(4000);
    ui->Ycoordinate_spin_lineEdit->setText("1");
    //    ui->YZdiag_Xcoordinate_Slider->setMinimum(1); //2021.04.xx-01 vox2D-viewer 斜め断面
    //    ui->YZdiag_Xcoordinate_Slider->setMaximum(4000); //2021.04.xx-01 vox2D-viewer 斜め断面
    //    ui->YZdiag_Xcoordinate_spin_lineEdit->setText("1"); //2021.04.xx-01 vox2D-viewer 斜め断面

    ui->Ycoordinate_Slider->setMinimum(1);
    ui->Ycoordinate_Slider->setMaximum(4000);
    // ui->Ycoordinate_spinBox->setMinimum(1);
    // ui->Ycoordinate_spinBox->setMaximum(4000);
    ui->XZdiag_Ycoordinate_spin_lineEdit->setText("1");
    ui->XZdiag_Ycoordinate_Slider->setMinimum(1);          // 2021.04.xx-01 vox2D-viewer 斜め断面
    ui->XZdiag_Ycoordinate_Slider->setMaximum(4000);       // 2021.04.xx-01 vox2D-viewer 斜め断面
    ui->XZdiag_Ycoordinate_spin_lineEdit->setText("1");    // 2021.04.xx-01 vox2D-viewer 斜め断面

    ui->Zcoordinate_Slider->setMinimum(1);
    ui->Zcoordinate_Slider->setMaximum(8500);
    // ui->Zcoordinate_spinBox->setMinimum(1);
    // ui->Zcoordinate_spinBox->setMaximum(8500);
    ui->Zcoordinate_spin_lineEdit->setText("1");

    ui->Ycoordinate_2D_label->setHidden(true);    // 2D表示は隠す
    ui->Ycoordinate_2D_label->setText("Yposition2D");
    ui->Xcoordinate_2D_label->setHidden(true);
    ui->Xcoordinate_2D_label->setText("Xposition2D");
    ui->Ycoordinate_label->setVisible(true);    // 3D表示に戻す　2019.11.13 2D指定ポジションをViewerに反映
    ui->Ycoordinate_spin_lineEdit->setVisible(true);
    ui->Ycoordinate_spin_downButton->setVisible(true);
    ui->Ycoordinate_spin_upButton->setVisible(true);
    ui->Ycoordinate_Slider->setVisible(true);
    ui->Ypos_pixCenter_pushButton->setVisible(true);
    ui->Xcoordinate_label->setVisible(true);
    ui->Xcoordinate_spin_lineEdit->setVisible(true);
    ui->Xcoordinate_spin_downButton->setVisible(true);
    ui->Xcoordinate_spin_upButton->setVisible(true);
    ui->Xcoordinate_Slider->setVisible(true);
    ui->Xpos_pixCenter_pushButton->setVisible(true);

    // ui->XZ_comboBox_Length->hide();
    // ui->YZ_comboBox_Length->hide();
    // ui->XY_comboBox_Length->hide();
    ui->XYshowcenter_HV_comboBox->hide();

    QStringList rulerColorList;
    rulerColorList << "black"
                   << "gray"
                   << "white";
    ui->XZruler_color_comboBox->addItems(rulerColorList);
    ui->YZruler_color_comboBox->addItems(rulerColorList);
    ui->XYruler_color_comboBox->addItems(rulerColorList);
    ui->XZdiag_ruler_color_comboBox->addItems(rulerColorList);
    rulerXZ_X     = 9999;
    rulerXZ_Y     = 9999;
    rulerYZ_X     = 9999;
    rulerYZ_Y     = 9999;
    rulerXY_X     = 9999;
    rulerXY_Y     = 9999;
    rulerXZdiag_X = 9999;
    rulerXZdiag_Y = 9999;

    // 2018.08.09 未使用につきコメントアウト
    // ui->label_15->setText(""); //2018.07.31-001-3 BOXファイルのエラーメッセージ欄の初期化

    //-start- 2020.03.09 ショートカットキー説明のtoolTip追加
    ui->XZRedraw_pushButton->setToolTip("key R");
    ui->XZzoomin_pushButton->setToolTip("key Z");
    ui->XZzoomout_pushButton->setToolTip("key shift+Z");
    ui->XZzoomall_pushButton->setToolTip("key F");
    ui->XZ_comboBox->setToolTip("Zoombox key B");
    ui->XZlength_checkBox->setToolTip("key L");

    ui->YZRedraw_pushButton->setToolTip("key R");
    ui->YZzoomin_pushButton->setToolTip("key Z");
    ui->YZzoomout_pushButton->setToolTip("key shift+Z");
    ui->YZzoomall_pushButton->setToolTip("key F");
    ui->YZ_comboBox->setToolTip("Zoombox key B");
    ui->YZlength_checkBox->setToolTip("key L");

    ui->XYRedraw_pushButton->setToolTip("key R");
    ui->XYzoomin_pushButton->setToolTip("key Z");
    ui->XYzoomout_pushButton->setToolTip("key shift+Z");
    ui->XYzoomall_pushButton->setToolTip("key F");
    ui->XY_comboBox->setToolTip("Zoombox key B");
    ui->XYlength_checkBox->setToolTip("key L");
    //-end- 2020.03.09 ショートカットキー説明のtoolTip追加

    ui->XZruler_checkBox->setToolTip(QString::fromUtf8("※マウス右クリック"));
    ui->YZruler_checkBox->setToolTip(QString::fromUtf8("※マウス右クリック"));
    ui->XYruler_checkBox->setToolTip(QString::fromUtf8("※マウス右クリック"));

    ui->scrollArea_4->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->scrollArea_4->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);    // 2021.04.xx-01 vox2Dviewer
    this->window()->resize(600, 700);                                       // window全体の初期表示サイズ
    // ui->scrollArea_4->setMaximumHeight(2500); //2021.04.xx-01 vox2Dviewer スクロールバー表示のため
    // 　scrollAreaのMaxサイズは、その中に配置された部品群合計高さより小さくする
    // ui->XZgraphicsView->setMinimumHeight(2500); //2021.04.xx-01 vox2Dviewer
    // スクロールバー表示のため　部品のMinサイズを、scrollAreaのMaxサイズより大きくする

    //-start- 2021.04.xx-01 vox2Dviewer
    ui->tabWidget->setVisible(
        false);    // vox読み込み前の状態前で操作すると落ちることを防止するため初期で非表示。voxのloadボタン押下時点で初めて表示されるようにする。
    ui->tabWidget->setCurrentIndex(0);

    func_load_OfficialList();    // Eyeris起動時にgetしたOfficialListをロードする。

    ui->XZinfo_label->setText("");
    ui->YZinfo_label->setText("");
    ui->XYinfo_label->setText("");
    ui->XZdiag_info_label->setText("");

    //
    ui->XZshowcell_checkBox->setVisible(false);
    ui->XZshowcenter_checkBox->setVisible(false);
    //
    ui->YZshowcell_checkBox->setVisible(false);
    ui->YZshowcenter_checkBox->setVisible(false);
    //    ui->YZlength_checkBox->setVisible(false);
    //    ui->YZ_comboBox_Length->setVisible(false);
    //    ui->YZruler_checkBox->setVisible(false);
    //    ui->YZruler_color_comboBox->setVisible(false);
    //
    ui->XYshowcell_checkBox->setVisible(false);
    ui->XYshowcenter_checkBox->setVisible(false);
    //    ui->XYlength_checkBox->setVisible(false);
    //    ui->XY_comboBox_Length->setVisible(false);
    //    ui->XYruler_checkBox->setVisible(false);
    //    ui->XYruler_color_comboBox->setVisible(false);
    //
    ui->XZdiag_showcell_checkBox->setVisible(false);
    ui->XZdiag_showcenter_checkBox->setVisible(false);
    //    ui->XZdiag_length_checkBox->setVisible(false);
    //    ui->XZdiag_comboBox_Length->setVisible(false);
    //    ui->XZdiag_ruler_checkBox->setVisible(false);
    //    ui->XZdiag_ruler_color_comboBox->setVisible(false);
    //
    ui->Auto_Zposition_checkBox->setVisible(false);

    //-start- 2021.04.xx-01 vox2Dviewer
    g_flag_slotAutorun =
        1;    // マテリアルコンボの値変更時にスロット処理させる:1 or
              // させない:0。（ユーザー操作の時だけスロット処理したい。自動設定のとき（自動=初期設定の時など）はスロット処理したくない）
    g_flag_slotMatColorCombo =
        1;    // マテリアルコンボの値変更時にスロット処理させる:1 or
              // させない:0。（ユーザー操作の時だけスロット処理したい。自動設定のとき（自動=初期設定の時など）はスロット処理したくない）
    g_flag_slotMatView =
        1;    // マテリアルコンボの値変更時にスロット処理させる:1 or
              // させない:0。（ユーザー操作の時だけスロット処理したい。自動設定のとき（自動=初期設定の時など）はスロット処理したくない）

    // 斜め断面　ビュー切り替えコンボボックス
    QStringList XZdiag_comboBox_angleList;
    XZdiag_comboBox_angleList << "45"
                              << "-45";
    ui->XZdiag_comboBox_angle->addItems(XZdiag_comboBox_angleList);

    //-start- テーブルウィジェット初期設定
    ui->tableWidget->setColumnCount(5);
    QStringList labels;
    labels << "view"
           << "No"
           << "MaterialName"
           << "color"
           << "color_toumei";    // view, color_toumei使わない。　流用元のままにしてしまっている。。
    ui->tableWidget->setHorizontalHeaderLabels(labels);
    ui->tableWidget->QTableWidget::horizontalHeader()->setStretchLastSection(true);
    // ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers); //Table上の直接編集禁止
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setColumnWidth(0, 5);    //[0]列目(チェックボックス欄)の幅 50→5 実際使わない列なので小さくした。
    ui->tableWidget->setColumnWidth(1, 30);    //[1]列目(マテリアル番号)の幅 50→5 実際使わない列なので小さくした。
    ui->tableWidget->setColumnWidth(2, 250);    //[2]列目(マテリアル名)の幅
    ui->tableWidget->setColumnWidth(3, 50);     //[3]列目(色名)の幅
    ui->tableWidget->setColumnWidth(4, 5);    //[4]列目(チェックボックス欄)の幅　50→5 実際使わない列なので小さくした。
    ui->tableWidget->hideColumn(0);    // 材質表示・非表示チェックボックス 非表示にする Eyeris2Dviewerではない機能なので
    ui->tableWidget->hideColumn(1);    // 材質No 非表示にする ユーザーは見る必要もないため
    ui->tableWidget->hideColumn(4);    // 色透明　非表示にする　半透明切替が実装できたら表示する。それまでは非表示。
    // ui->tableWidget->setMinimumWidth(430);
    //-end- テーブルウィジェット初期設定

    // AutoLengthタブの設定
    ui->vox_tabWidget->setCurrentIndex(0);
    ui->vox_radioButton_autolength_directionV->setChecked(true);
    ui->vox_radioButton_autolength_pointMouse->setChecked(true);
    ui->vox_lineEdit_autolength_pointXY->setText("");
    ui->vox_textEdit_autolength_result->setText("");
    ui->vox_textEdit_autolength_result->setReadOnly(true);
    ui->label_autolength_point->setText("X");
    QStringList autoLengthColorList;
    g_flag_slotAutorun = 0;
    autoLengthColorList << "black"
                        << "gray"
                        << "white";
    ui->vox_comboBox_autolength_color->addItems(autoLengthColorList);
    QStringList autoLengthfontSizeList;
    autoLengthfontSizeList << "5"
                           << "10"
                           << "20"
                           << "30"
                           << "40"
                           << "50"
                           << "60"
                           << "70"
                           << "80"
                           << "90"
                           << "100";
    ui->vox_comboBox_autolength_fontsize->addItems(autoLengthfontSizeList);
    ui->vox_comboBox_autolength_fontsize->setCurrentText("100");
    g_flag_slotAutorun     = 1;
    g_autoXZ_X             = 99999;    // 自動測長　ユーザー指定座標値
    g_autoXZ_Y             = 99999;
    g_autoYZ_X             = 99999;
    g_autoYZ_Y             = 99999;
    g_autoXY_X             = 99999;
    g_autoXY_Y             = 99999;
    g_autoXZdiag_X         = 99999;
    g_autoXZdiag_Y         = 99999;
    g_autoXZ_direction     = "V";    // 自動測長　測長方向　縦=V or 横=H
    g_autoYZ_direction     = "V";
    g_autoXY_direction     = "V";
    g_autoXZdiag_direction = "V";

    //-end- 2021.04.xx-01 vox2Dviewer

    //-start- 画素中央ボタンの設定
    ui->XZdiag_Ypos_pixCenter_pushButton->setVisible(false);    // 機能未実装のため隠す
    Ycount_pixCenter        = 0;
    Xcount_pixCenter        = 0;
    XZdiag_Ycount_pixCenter = 0;
    //-end- 画素中央ボタンの設定

    g_flag_keyCTRL = 0;    // ショートカットキー AutoLength用途(ctrl+左マウスクリック）

    // スプリッタサイズ設定　画面に左右配置しているXZビュー, マテリアルテーブルの表示サイズ。
    ui->splitter->setSizes(QList<int>() << 900 << 500);

    // GUI色定義　共通変数
    CommonSubFunc mCommonSubFunc;
    g_hash_matnameToCname =
        mCommonSubFunc
            .func_makehash_matnameToCname();    // 共通値参照ハッシュ  キー:マテリアル名　   値：色名 green　など
    g_hash_cnameToQtColor =
        mCommonSubFunc.func_makehash_cnameToQtColor();    // 共通値参照ハッシュ キー:色名(greenなど)　値：Qcolor
                                                          // 例：QColor(0, 128, 0, 255)
    g_hash_cnameToStyleColor =
        mCommonSubFunc.func_makehash_cnameToStyleColor();    // 共通値参照ハッシュ  キー:色名(greenなど)　値：
                                                             // 例：String型 #008000
    g_list_whitefontBgColor << "Black"
                            << "Gray"
                            << "Brown"
                            << "Purple"
                            << "MediumPurple"
                            << "Green"
                            << "Olive"
                            << "Navy"
                            << "Blue";

    // シグナル・スロット
    connect(ui->Xcoordinate_Slider, SIGNAL(valueChanged(int)), this, SLOT(update_Xcoordinate_spinBox()));
    connect(ui->Xcoordinate_spin_lineEdit, SIGNAL(editingFinished()), this,
            SLOT(func_on_Xcoordinate_spin_lineEdit_editingFinished()));
    connect(ui->Ycoordinate_Slider, SIGNAL(valueChanged(int)), this, SLOT(update_Ycoordinate_spinBox()));
    connect(ui->Ycoordinate_spin_lineEdit, SIGNAL(editingFinished()), this,
            SLOT(func_on_Ycoordinate_spin_lineEdit_editingFinished()));    // SLOT(update_Ycoordinate_Slider()));
    connect(ui->Zcoordinate_Slider, SIGNAL(valueChanged(int)), this, SLOT(update_Zcoordinate_spinBox()));
    connect(ui->Zcoordinate_spin_lineEdit, SIGNAL(editingFinished()), this,
            SLOT(func_on_Zcoordinate_spin_lineEdit_editingFinished()));

    connect(ui->Xcoordinate_Slider, SIGNAL(valueChanged(int)), this, SLOT(viewer_update()));
    connect(ui->Ycoordinate_Slider, SIGNAL(valueChanged(int)), this, SLOT(viewer_update()));
    connect(ui->Zcoordinate_Slider, SIGNAL(valueChanged(int)), this, SLOT(viewer_update()));

    //-start- 2021.04.xx-01 vox-2Dviewer 斜め断面
    // connect(ui->YZdiag_Xcoordinate_Slider, SIGNAL(valueChanged(int)), this,
    // SLOT(update_YZdiag_Xcoordinate_spinBox()));
    // シグナルスロットで既に有効なので、コメントアウト//connect(ui->YZdiag_Xcoordinate_spin_lineEdit,
    // SIGNAL(editingFinished()), this, SLOT(on_YZdiag_Ycoordinate_spin_lineEdit_editingFinished()));
    connect(ui->XZdiag_Ycoordinate_Slider, SIGNAL(valueChanged(int)), this, SLOT(update_XZdiag_Ycoordinate_spinBox()));
    // シグナルスロットで既に有効なので、コメントアウト//connect(ui->XZdiag_Ycoordinate_spin_lineEdit,
    // SIGNAL(editingFinished()), this, SLOT(on_XZdiag_Ycoordinate_spin_lineEdit_editingFinished()));
    // connect(ui->YZdiag_Xcoordinate_Slider, SIGNAL(valueChanged(int)), this, SLOT(viewer_update()));
    connect(ui->XZdiag_Ycoordinate_Slider, SIGNAL(valueChanged(int)), this, SLOT(viewer_update()));
    //-end- 2021.04.xx-01 vox-2Dviewer 斜め断面

    //  connect(ui->Xcoordinate_spinBox, SIGNAL(valueChanged(int)), this, SLOT(update_Xcoordinate_Slider()));
    //  //スピンボックスの値が変更されたときもSliderを追従させることで,slider値変更でviewer_updateされる　(別のシグナルスロット処理にて）//2018.10.xx-02
    //  connect(ui->Ycoordinate_spin_lineEdit, SIGNAL(valueChanged(int)), this,
    //  SLOT(update_Ycoordinate_Slider()));//2018.10.xx-02 connect(ui->Zcoordinate_spinBox, SIGNAL(valueChanged(int)),
    //  this, SLOT(update_Zcoordinate_Slider()));//2018.10.xx-02
    connect(ui->Xcoordinate_spin_upButton, SIGNAL(clicked(bool)), this,
            SLOT(func_on_Xcoordinate_spin_upButton_clicked()));
    connect(ui->Xcoordinate_spin_downButton, SIGNAL(clicked(bool)), this,
            SLOT(func_on_Xcoordinate_spin_downButton_clicked()));
    connect(ui->Ycoordinate_spin_upButton, SIGNAL(clicked(bool)), this,
            SLOT(func_on_Ycoordinate_spin_upButton_clicked()));
    connect(ui->Ycoordinate_spin_downButton, SIGNAL(clicked(bool)), this,
            SLOT(func_on_Ycoordinate_spin_downButton_clicked()));
    connect(ui->Zcoordinate_spin_upButton, SIGNAL(clicked(bool)), this,
            SLOT(func_on_Zcoordinate_spin_upButton_clicked()));
    connect(ui->Zcoordinate_spin_downButton, SIGNAL(clicked(bool)), this,
            SLOT(func_on_Zcoordinate_spin_downButton_clicked()));

    // 2017.11.21　spinBox->viewer_update()処理追加 (変更前は 関数update_Xcoordinate_Slider()
    // から　関数update_Xcoordinate_Slider　を呼び出していたため、2回viewer_updateのされることのないよう処理なしだった。コード変更後：呼び出し無しにしたため、個別でviewer_updateが必要になった）
    connect(ui->Xcoordinate_spin_lineEdit, SIGNAL(editingFinished()), this, SLOT(viewer_update()));
    connect(ui->Ycoordinate_spin_lineEdit, SIGNAL(editingFinished()), this, SLOT(viewer_update()));
    connect(ui->Zcoordinate_spin_lineEdit, SIGNAL(editingFinished()), this, SLOT(viewer_update()));

    connect(ui->XZshowgrid_checkBox, SIGNAL(stateChanged(int)), this, SLOT(XZshowgrid_checkBox_stateChanged()));
    //    connect(ui->XZshowoutput_checkBox, SIGNAL(stateChanged(int)), this,
    //    SLOT(XZshowoutput_checkBox_stateChanged())); connect(ui->XZshowexcitationplane_checkBox,
    //    SIGNAL(stateChanged(int)), this, SLOT(XZshowexcitationplane_checkBox_stateChanged()));
    connect(ui->YZshowgrid_checkBox, SIGNAL(stateChanged(int)), this, SLOT(YZshowgrid_checkBox_stateChanged()));
    //    connect(ui->YZshowoutput_checkBox, SIGNAL(stateChanged(int)), this,
    //    SLOT(YZshowoutput_checkBox_stateChanged())); connect(ui->YZshowexcitationplane_checkBox,
    //    SIGNAL(stateChanged(int)), this, SLOT(YZshowexcitationplane_checkBox_stateChanged()));
    connect(ui->XYshowgrid_checkBox, SIGNAL(stateChanged(int)), this, SLOT(XYshowgrid_checkBox_stateChanged()));
    //    connect(ui->XYshowoutput_checkBox, SIGNAL(stateChanged(int)), this,
    //    SLOT(XYshowoutput_checkBox_stateChanged()));
    connect(ui->XZdiag_showgrid_checkBox, SIGNAL(stateChanged(int)), this,
            SLOT(XZdiag_showgrid_checkBox_stateChanged()));
    // connect(ui->YZdiag_showgrid_checkBox, SIGNAL(stateChanged(int)), this,
    // SLOT(YZdiag_showgrid_checkBox_stateChanged()));

    //    //
    //    //-start- 2018.06.xx-01 showOutputVolume Normal領域表示
    ////    connect(ui->XZshowOutputVolumeNormal_checkBox, SIGNAL(stateChanged(int)), this,
    /// SLOT(XZshowOutputVolumeNormal_checkBox_stateChanged())); /    connect(ui->YZshowOutputVolumeNormal_checkBox,
    /// SIGNAL(stateChanged(int)), this, SLOT(YZshowOutputVolumeNormal_checkBox_stateChanged())); /
    /// connect(ui->XYshowOutputVolumeNormal_checkBox, SIGNAL(stateChanged(int)), this,
    /// SLOT(XYshowOutputVolumeNormal_checkBox_stateChanged()));
    //    //-end- 2018.06.xx-01 showOutputVolume Normal領域表示
    //    //-start- 2018.06.xx-01 showOutputVolume Custom領域表示
    //    connect(ui->XZshowOutputVolumeCustom_checkBox, SIGNAL(stateChanged(int)), this,
    //    SLOT(XZshowOutputVolumeCustom_checkBox_stateChanged())); connect(ui->YZshowOutputVolumeCustom_checkBox,
    //    SIGNAL(stateChanged(int)), this, SLOT(YZshowOutputVolumeCustom_checkBox_stateChanged()));
    //    connect(ui->XYshowOutputVolumeCustom_checkBox, SIGNAL(stateChanged(int)), this,
    //    SLOT(XYshowOutputVolumeCustom_checkBox_stateChanged()));
    //    //-end- 2018.06.xx-01 showOutputVolume Normal領域表示

    //    //GraphicView画面のマウスクリック時の動作 signal=GeometrySubScene.cpp, slot=GeometryInputForm.cpp
    connect(XZscene, SIGNAL(sendCoord(float, float)), this, SLOT(receiveCoord(float, float)));
    connect(YZscene, SIGNAL(sendCoord(float, float)), this, SLOT(receiveCoord(float, float)));
    connect(XYscene, SIGNAL(sendCoord(float, float)), this, SLOT(receiveCoord(float, float)));
    connect(XZdiag_scene, SIGNAL(sendCoord(float, float)), this, SLOT(receiveCoord(float, float)));
    // connect(YZdiag_scene,SIGNAL(sendCoord(float, float)),this,SLOT(receiveCoord(float, float)));

    //    //GraphicView画面のマウスクリック時の動作 signal=GeometrySubScene.cpp, slot=GeometryInputForm.cpp
    connect(XZscene, SIGNAL(sendCoordMouseRight(float, float)), this, SLOT(receiveCoordMouseRight(float, float)));
    connect(YZscene, SIGNAL(sendCoordMouseRight(float, float)), this, SLOT(receiveCoordMouseRight(float, float)));
    connect(XYscene, SIGNAL(sendCoordMouseRight(float, float)), this, SLOT(receiveCoordMouseRight(float, float)));
    connect(XZdiag_scene, SIGNAL(sendCoordMouseRight(float, float)), this, SLOT(receiveCoordMouseRight(float, float)));
    // connect(YZdiag_scene,SIGNAL(sendCoordMouseRight(float, float)),this,SLOT(receiveCoordMouseRight(float, float)));

    //    //ショートカットキー時点の動作　2020.03.09
    connect(XZscene, SIGNAL(sendShortcutkey(QString)), this, SLOT(receiveShortcutKey(QString)));
    connect(YZscene, SIGNAL(sendShortcutkey(QString)), this, SLOT(receiveShortcutKey(QString)));
    connect(XYscene, SIGNAL(sendShortcutkey(QString)), this, SLOT(receiveShortcutKey(QString)));
    connect(XZdiag_scene, SIGNAL(sendShortcutkey(QString)), this, SLOT(receiveShortcutKey(QString)));
    // connect(YZdiag_scene,SIGNAL(sendShortcutkey(QString)),this,SLOT(receiveShortcutKey(QString)));

    //    //初期画面表示
    //    // FLAG_VIEWER_UPDATE = 1 の場合のみ、ビューアを更新する。(関数 viewer_update)。
    //    それ以外の場合はリアルタイム更新はしない(画面フリーズ防止のため）。 FLAG_VIEWER_UPDATE = 1; make_XZgeometry();

    // XY-YZ-XYviewerのタブが切り替わった際に画面更新する
    //    //connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(viewer_update()));
    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(func_tabWidget_currentChanged()));

    // 関数update_X/Y/Zcoordinate_spinBox()　 connectにより2回処理されてしまうことの防止
    COUNT_SPINBOX_RUN = 0;
}

GeometryInputForm::~GeometryInputForm()
{
    delete ui;
}

void GeometryInputForm::func_GUI_defaultSet()
{    // ファイルロードし直した時など、GUI, グローバル変数を初期状態にする
    // 画素中央ボタンの設定
    Ycount_pixCenter        = 0;
    Xcount_pixCenter        = 0;
    XZdiag_Ycount_pixCenter = 0;

    // 画素サイズ、セルアレイ数のクリア
    ui->cellX_lineEdit->setText("");
    ui->cellY_lineEdit->setText("");
    ui->arrayX_lineEdit->setText("");
    ui->arrayY_lineEdit->setText("");

    // メニュータブ
    ui->vox_tabWidget->setCurrentIndex(0);

    // AutoLengthタブのメニューをデフォルト状態にセットする
    ui->vox_comboBox_autolength_color->setCurrentText("black");
    ui->vox_checkBox_autolength_viewResult->setChecked(false);
    ui->vox_comboBox_autolength_fontsize->setCurrentText("100");
    ui->vox_radioButton_autolength_directionV->setChecked(true);
    ui->vox_radioButton_autolength_pointMouse->setChecked(true);
    ui->vox_lineEdit_autolength_pointXY->setText("");
    ui->vox_comboBox_autolength_pointCenter->clear();
    ui->vox_textEdit_autolength_result->setText("");
}

//-start- XZ, YZ, XYビュー共通処理
void GeometryInputForm::addrect(QString planestr, QString color, int pos1, int pos2, int w, int h,
                                QString InputTooltip)    // viewerの描画用
{
    QBrush blackBrush(QColor(0, 0, 0, 255));
    QBrush grayBrush(QColor(128, 128, 128, 255));
    QBrush darkgrayBrush(QColor(46, 46, 46, 255));
    QBrush lightgrayBrush(QColor(211, 211, 211, 255));
    QBrush whiteBrush(QColor(255, 255, 255, 255));
    QBrush brownBrush(QColor(165, 42, 42, 255));
    QBrush purpleBrush(QColor(128, 0, 128, 255));
    QBrush mediumpurpleBrush(QColor(147, 112, 219, 255));
    QBrush redBrush(QColor(255, 0, 0, 255));
    QBrush magentaBrush(QColor(255, 0, 255, 255));
    QBrush pinkBrush(QColor(255, 192, 203, 255));
    QBrush orangeBrush(QColor(225, 165, 0, 255));
    QBrush goldBrush(QColor(255, 215, 0, 255));
    QBrush yellowBrush(QColor(255, 255, 0, 255));
    QBrush greenBrush(QColor(0, 128, 0, 255));
    QBrush greenyellowBrush(QColor(173, 255, 47, 255));
    QBrush oliveBrush(QColor(128, 128, 0, 255));
    QBrush navyBrush(QColor(0, 0, 128, 255));
    QBrush blueBrush(QColor(0, 0, 255, 255));
    QBrush cyanBrush(QColor(0, 255, 255, 255));
    QBrush lightcyanBrush(QColor(224, 255, 255, 255));
    QPen   whitePen(QColor(255, 255, 255, 0));

    QGraphicsRectItem* tmpRect;
    QString            tmpMsg;
    tmpMsg = InputTooltip;

    if (planestr == "xzplane") {
        if (color == "Black") {
            //->addRect(pos1, pos2, w, h, whitePen, blackBrush);
            tmpRect = XZscene->addRect(pos1, pos2, w, h, whitePen, blackBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Gray") {
            // XZscene->addRect(pos1, pos2, w, h, whitePen, grayBrush);
            tmpRect = XZscene->addRect(pos1, pos2, w, h, whitePen, grayBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "DarkGray") {
            // XZscene->addRect(pos1, pos2, w, h, whitePen, darkgrayBrush);
            tmpRect = XZscene->addRect(pos1, pos2, w, h, whitePen, darkgrayBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Lightgray") {
            // XZscene->addRect(pos1, pos2, w, h, whitePen, lightgrayBrush);
            tmpRect = XZscene->addRect(pos1, pos2, w, h, whitePen, lightgrayBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "White") {
            // XZscene->addRect(pos1, pos2, w, h, whitePen, whiteBrush);
            tmpRect = XZscene->addRect(pos1, pos2, w, h, whitePen, whiteBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Brown") {
            // XZscene->addRect(pos1, pos2, w, h, whitePen, brownBrush);
            tmpRect = XZscene->addRect(pos1, pos2, w, h, whitePen, brownBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Purple") {
            // XZscene->addRect(pos1, pos2, w, h, whitePen, purpleBrush);
            tmpRect = XZscene->addRect(pos1, pos2, w, h, whitePen, purpleBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "MediumPurple") {
            // XZscene->addRect(pos1, pos2, w, h, whitePen, mediumpurpleBrush);
            tmpRect = XZscene->addRect(pos1, pos2, w, h, whitePen, mediumpurpleBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Red") {
            // XZscene->addRect(pos1, pos2, w, h, whitePen, redBrush);
            tmpRect = XZscene->addRect(pos1, pos2, w, h, whitePen, redBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Magenta") {
            // XZscene->addRect(pos1, pos2, w, h, whitePen, magentaBrush);
            tmpRect = XZscene->addRect(pos1, pos2, w, h, whitePen, magentaBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Pink") {
            // XZscene->addRect(pos1, pos2, w, h, whitePen, pinkBrush);
            tmpRect = XZscene->addRect(pos1, pos2, w, h, whitePen, pinkBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Orange") {
            // XZscene->addRect(pos1, pos2, w, h, whitePen, orangeBrush);
            tmpRect = XZscene->addRect(pos1, pos2, w, h, whitePen, orangeBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Gold") {
            // XZscene->addRect(pos1, pos2, w, h, whitePen, goldBrush);
            tmpRect = XZscene->addRect(pos1, pos2, w, h, whitePen, goldBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Yellow") {
            // XZscene->addRect(pos1, pos2, w, h, whitePen, yellowBrush);
            tmpRect = XZscene->addRect(pos1, pos2, w, h, whitePen, yellowBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Green") {
            // XZscene->addRect(pos1, pos2, w, h, whitePen, greenBrush);
            tmpRect = XZscene->addRect(pos1, pos2, w, h, whitePen, greenBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Greenyellow") {
            // XZscene->addRect(pos1, pos2, w, h, whitePen, greenyellowBrush);
            tmpRect = XZscene->addRect(pos1, pos2, w, h, whitePen, greenyellowBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Olive") {
            // XZscene->addRect(pos1, pos2, w, h, whitePen, oliveBrush);
            tmpRect = XZscene->addRect(pos1, pos2, w, h, whitePen, oliveBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Navy") {
            // XZscene->addRect(pos1, pos2, w, h, whitePen, navyBrush);
            tmpRect = XZscene->addRect(pos1, pos2, w, h, whitePen, navyBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Blue") {
            // XZscene->addRect(pos1, pos2, w, h, whitePen, blueBrush);
            tmpRect = XZscene->addRect(pos1, pos2, w, h, whitePen, blueBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Cyan") {
            // XZscene->addRect(pos1, pos2, w, h, whitePen, cyanBrush);
            tmpRect = XZscene->addRect(pos1, pos2, w, h, whitePen, cyanBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Lightcyan") {
            // XZscene->addRect(pos1, pos2, w, h, whitePen, lightcyanBrush);
            tmpRect = XZscene->addRect(pos1, pos2, w, h, whitePen, lightcyanBrush);
            tmpRect->setToolTip(tmpMsg);
        }
    }
    else if (planestr == "yzplane") {
        if (color == "Black") {
            tmpRect = YZscene->addRect(pos1, pos2, w, h, whitePen, blackBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Gray") {
            tmpRect = YZscene->addRect(pos1, pos2, w, h, whitePen, grayBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Lightgray") {
            tmpRect = YZscene->addRect(pos1, pos2, w, h, whitePen, lightgrayBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "White") {
            tmpRect = YZscene->addRect(pos1, pos2, w, h, whitePen, whiteBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Brown") {
            tmpRect = YZscene->addRect(pos1, pos2, w, h, whitePen, brownBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Purple") {
            tmpRect = YZscene->addRect(pos1, pos2, w, h, whitePen, purpleBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "MediumPurple") {
            tmpRect = YZscene->addRect(pos1, pos2, w, h, whitePen, mediumpurpleBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Red") {
            tmpRect = YZscene->addRect(pos1, pos2, w, h, whitePen, redBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Magenta") {
            tmpRect = YZscene->addRect(pos1, pos2, w, h, whitePen, magentaBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Pink") {
            tmpRect = YZscene->addRect(pos1, pos2, w, h, whitePen, pinkBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Orange") {
            tmpRect = YZscene->addRect(pos1, pos2, w, h, whitePen, orangeBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Gold") {
            tmpRect = YZscene->addRect(pos1, pos2, w, h, whitePen, goldBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Yellow") {
            tmpRect = YZscene->addRect(pos1, pos2, w, h, whitePen, yellowBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Green") {
            tmpRect = YZscene->addRect(pos1, pos2, w, h, whitePen, greenBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Greenyellow") {
            tmpRect = YZscene->addRect(pos1, pos2, w, h, whitePen, greenyellowBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Olive") {
            tmpRect = YZscene->addRect(pos1, pos2, w, h, whitePen, oliveBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Navy") {
            tmpRect = YZscene->addRect(pos1, pos2, w, h, whitePen, navyBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Blue") {
            tmpRect = YZscene->addRect(pos1, pos2, w, h, whitePen, blueBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Cyan") {
            tmpRect = YZscene->addRect(pos1, pos2, w, h, whitePen, cyanBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Lightcyan") {
            tmpRect = YZscene->addRect(pos1, pos2, w, h, whitePen, lightcyanBrush);
            tmpRect->setToolTip(tmpMsg);
        }
    }
    else if (planestr == "xyplane") {
        if (color == "Black") {
            tmpRect = XYscene->addRect(pos1, pos2, w, h, whitePen, blackBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Gray") {
            tmpRect = XYscene->addRect(pos1, pos2, w, h, whitePen, grayBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Lightgray") {
            tmpRect = XYscene->addRect(pos1, pos2, w, h, whitePen, lightgrayBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "White") {
            tmpRect = XYscene->addRect(pos1, pos2, w, h, whitePen, whiteBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Brown") {
            tmpRect = XYscene->addRect(pos1, pos2, w, h, whitePen, brownBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Purple") {
            tmpRect = XYscene->addRect(pos1, pos2, w, h, whitePen, purpleBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "MediumPurple") {
            tmpRect = XYscene->addRect(pos1, pos2, w, h, whitePen, mediumpurpleBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Red") {
            tmpRect = XYscene->addRect(pos1, pos2, w, h, whitePen, redBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Magenta") {
            tmpRect = XYscene->addRect(pos1, pos2, w, h, whitePen, magentaBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Pink") {
            tmpRect = XYscene->addRect(pos1, pos2, w, h, whitePen, pinkBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Orange") {
            tmpRect = XYscene->addRect(pos1, pos2, w, h, whitePen, orangeBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Gold") {
            tmpRect = XYscene->addRect(pos1, pos2, w, h, whitePen, goldBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Yellow") {
            tmpRect = XYscene->addRect(pos1, pos2, w, h, whitePen, yellowBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Green") {
            tmpRect = XYscene->addRect(pos1, pos2, w, h, whitePen, greenBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Greenyellow") {
            tmpRect = XYscene->addRect(pos1, pos2, w, h, whitePen, greenyellowBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Olive") {
            tmpRect = XYscene->addRect(pos1, pos2, w, h, whitePen, oliveBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Navy") {
            tmpRect = XYscene->addRect(pos1, pos2, w, h, whitePen, navyBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Blue") {
            tmpRect = XYscene->addRect(pos1, pos2, w, h, whitePen, blueBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Cyan") {
            tmpRect = XYscene->addRect(pos1, pos2, w, h, whitePen, cyanBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Lightcyan") {
            tmpRect = XYscene->addRect(pos1, pos2, w, h, whitePen, lightcyanBrush);
            tmpRect->setToolTip(tmpMsg);
        }
    }
    else if (planestr == "xzdiag_plane") {
        if (color == "Black") {
            //->addRect(pos1, pos2, w, h, whitePen, blackBrush);
            tmpRect = XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, blackBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Gray") {
            // XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, grayBrush);
            tmpRect = XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, grayBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "DarkGray") {
            // XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, darkgrayBrush);
            tmpRect = XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, darkgrayBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Lightgray") {
            // XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, lightgrayBrush);
            tmpRect = XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, lightgrayBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "White") {
            // XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, whiteBrush);
            tmpRect = XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, whiteBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Brown") {
            // XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, brownBrush);
            tmpRect = XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, brownBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Purple") {
            // XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, purpleBrush);
            tmpRect = XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, purpleBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "MediumPurple") {
            // XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, mediumpurpleBrush);
            tmpRect = XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, mediumpurpleBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Red") {
            // XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, redBrush);
            tmpRect = XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, redBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Magenta") {
            // XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, magentaBrush);
            tmpRect = XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, magentaBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Pink") {
            // XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, pinkBrush);
            tmpRect = XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, pinkBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Orange") {
            // XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, orangeBrush);
            tmpRect = XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, orangeBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Gold") {
            // XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, goldBrush);
            tmpRect = XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, goldBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Yellow") {
            // XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, yellowBrush);
            tmpRect = XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, yellowBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Green") {
            // XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, greenBrush);
            tmpRect = XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, greenBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Greenyellow") {
            // XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, greenyellowBrush);
            tmpRect = XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, greenyellowBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Olive") {
            // XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, oliveBrush);
            tmpRect = XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, oliveBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Navy") {
            // XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, navyBrush);
            tmpRect = XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, navyBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Blue") {
            // XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, blueBrush);
            tmpRect = XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, blueBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Cyan") {
            // XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, cyanBrush);
            tmpRect = XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, cyanBrush);
            tmpRect->setToolTip(tmpMsg);
        }
        else if (color == "Lightcyan") {
            // XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, lightcyanBrush);
            tmpRect = XZdiag_scene->addRect(pos1, pos2, w, h, whitePen, lightcyanBrush);
            tmpRect->setToolTip(tmpMsg);
        }
    }
}

void GeometryInputForm::swap(int* val1, int* val2)    // min/max値の大小関係が逆だった場合の入れ替え処理
{
    int tmpval;
    tmpval = *val1;
    *val1  = *val2;
    *val2  = tmpval;
}

// QString GeometryInputForm::evaluateStr(QString str) //数式文字列の計算（代表値のみの算出）
//{
//     QScriptEngine engine;
//     QScriptValue value;
//     QStringList formulalist;
//     QString replacedformula;
//     if(str.indexOf("$") >= 0){
//         formulalist = str.split("$");
//         for(int n = 0; n < formulalist.size()/2; n++){
//             if(acceptedVariablesNameList.indexOf(formulalist.at(2*n+1)) >= 0){
//                 int tmp_index = acceptedVariablesNameList.indexOf(formulalist.at(2*n+1));
//                 QString tmp_val = acceptedVariablesValueList.at(tmp_index);
//                 formulalist.replace(2*n+1, tmp_val);
//             }
//         }
//         for(int n = 0; n < formulalist.size(); n++){
//             replacedformula += formulalist.at(n);
//         }
//         //追加-start-
//         if(replacedformula.indexOf("$") >= 0){
//             QStringList replacedformulaList = replacedformula.split("$");
//             for(int n = 0; n < replacedformulaList.size() / 2; n++){
//                 if(acceptedVariablesNameList.indexOf(replacedformulaList.at(2*n+1)) >= 0){
//                     int tmp_index = acceptedVariablesNameList.indexOf(replacedformulaList.at(2*n+1));
//                     QString tmp_val = acceptedVariablesValueList.at(tmp_index);
//                     replacedformulaList.replace(2*n+1, tmp_val);
//                 }
//             }
//             QString rereplacedformula;
//             for(int n = 0; n < replacedformulaList.size(); n++){
//                 rereplacedformula += replacedformulaList.at(n);
//             }
//             value = engine.evaluate(rereplacedformula);
//         }else{
//             value = engine.evaluate(replacedformula);
//         }
//         //value = engine.evaluate(replacedformula);
//         //追加-end-
//     }else{
//         value = engine.evaluate(str);
//     }
//     return value.toString();
// }

void GeometryInputForm::func_setBrushes()
{
    Brushes[0]  = QColor(0, 0, 0, 255);          // Black
    Brushes[1]  = QColor(128, 128, 128, 255);    // Gray
    Brushes[2]  = QColor(46, 46, 46, 255);       // Darkgray
    Brushes[3]  = QColor(211, 211, 211, 255);    // Lightgray
    Brushes[4]  = QColor(255, 255, 255, 255);    // White
    Brushes[5]  = QColor(165, 42, 42, 255);      // Brawn
    Brushes[6]  = QColor(128, 0, 128, 255);      // Purple
    Brushes[7]  = QColor(147, 112, 219, 255);    // MediumPurple
    Brushes[8]  = QColor(255, 0, 0, 255);        // Red
    Brushes[9]  = QColor(255, 0, 255, 255);      // Magenta
    Brushes[10] = QColor(255, 192, 203, 255);    // Pink
    Brushes[11] = QColor(225, 165, 0, 255);      // Orange
    Brushes[12] = QColor(255, 215, 0, 255);      // Gold
    Brushes[13] = QColor(255, 255, 0, 255);      // Yellow
    Brushes[14] = QColor(0, 128, 0, 255);        // Green
    Brushes[15] = QColor(173, 255, 47, 255);     // Greenyellow
    Brushes[16] = QColor(128, 128, 0, 255);      // Olive
    Brushes[17] = QColor(0, 0, 128, 255);        // Navy
    Brushes[18] = QColor(0, 0, 255, 255);        // Blue
    Brushes[19] = QColor(0, 255, 255, 255);      // Cyan
    Brushes[20] = QColor(224, 255, 255, 255);    // Lightcyan
}

int GeometryInputForm::func_setBrushesNum(QString color)
{
    int colorNum = -1;

    if (color == "Black") {
        colorNum = 0;
    }
    if (color == "Gray") {
        colorNum = 1;
    }
    if (color == "DarkGray") {
        colorNum = 2;
    }
    if (color == "Lightgray") {
        colorNum = 3;
    }
    if (color == "White") {
        colorNum = 4;
    }
    if (color == "Brown") {
        colorNum = 5;
    }
    if (color == "Purple") {
        colorNum = 6;
    }
    if (color == "MediumPurple") {
        colorNum = 7;
    }
    if (color == "Red") {
        colorNum = 8;
    }
    if (color == "Magenta") {
        colorNum = 9;
    }
    if (color == "Pink") {
        colorNum = 10;
    }
    if (color == "Orange") {
        colorNum = 11;
    }
    if (color == "Gold") {
        colorNum = 12;
    }
    if (color == "Yellow") {
        colorNum = 13;
    }
    if (color == "Green") {
        colorNum = 14;
    }
    if (color == "Greenyellow") {
        colorNum = 15;
    }
    if (color == "Olive") {
        colorNum = 16;
    }
    if (color == "Navy") {
        colorNum = 17;
    }
    if (color == "Blue") {
        colorNum = 18;
    }
    if (color == "Cyan") {
        colorNum = 19;
    }
    if (color == "Lightcyan") {
        colorNum = 20;
    }

    return (colorNum);
}

void GeometryInputForm::viewer_update()    // viewerの更新
{
    // 今後削除予定のビューアー更新停止処理(ver.20171025コード更新時点で追加した処理だが、現在 FLAG_VIEWER_UPDATE
    // に置き替えたため） if(ui->XZNoRedraw_checkBox->isChecked()
    //|| ui->YZNoRedraw_checkBox->isChecked()
    //|| ui->XYNoRedraw_checkBox->isChecked() ) {
    //     return; // NoRedrawチェックボックスONの時は描画しない
    // }

    if (FLAG_VIEWER_UPDATE != 1) {
        // FLAG_VIEWER_UPDATE = 1 の場合のみ、ビューアを更新する。(関数 viewer_update)。
        // それ以外の場合はリアルタイム更新はしない(画面フリーズ防止のため）。
        return;
    }

    if (ui->tabWidget->currentIndex() == 0) {
        XZscene->clear();
        // 本番用//make_XZgeometry();
        func_vox_make_XZgeometry();    // 2021.06.xx-01 vox-2Dviewer

        //-end- commt-out //2021.06.xx-01  voxビューアーにするため変更
        // if(ui->XZshowOutputVolumeCustom_checkBox->isChecked()){
        //    XZshowOutputVolumeCustom_checkBox_isChecked();
        //}
        if (ui->XZshowgrid_checkBox->isChecked()) {
            XZshowgrid_checkBox_isChecked();
        }
        // if(ui->XZshowcell_checkBox->isChecked()){
        //     XZshowcell_checkBox_isChecked();
        // }
        // if(ui->XZshowcenter_checkBox->isChecked()){
        //     XZshowcenter_checkBox_isChecked();
        // }
        // if(ui->XZshowoutput_checkBox->isChecked()){
        //     XZshowoutput_checkBox_isChecked();
        // }
        // if(ui->XZshowexcitationplane_checkBox->isChecked()){
        //     XZshowexcitationplane_checkBox_isChecked();
        // }
        //-end- comment-out //2021.06.xx-01 voxビューアーにするため変更

        // if(ui->XZlength_checkBox->isChecked() || ui->XZlength_checkBox->text() == "Length_on"){
        if (lengthXZ_X.size() > 0 && lengthXZ_Y.size() > 0) {
            func_view_LengthAll();
        }
        if (ui->XZruler_checkBox->isChecked()) {
            func_view_ruler();
        }
        if (ui->XZautolength_checkBox->isChecked()) {
            func_view_AutoLength();
        }
    }
    else if (ui->tabWidget->currentIndex() == 1) {
        YZscene->clear();
        // Eyerisでの処理//make_YZgeometry();
        func_vox_make_YZgeometry();    // 2021.06.xx-01 vox-2Dviewer 3次元配列から表示

        //-start- comment-out //2021.06.xx-01 voxビューアーにするため変更
        // if(ui->YZshowOutputVolumeCustom_checkBox->isChecked()){
        //    YZshowOutputVolumeCustom_checkBox_isChecked();
        //}
        if (ui->YZshowgrid_checkBox->isChecked()) {
            YZshowgrid_checkBox_isChecked();
        }
        // if(ui->YZshowcell_checkBox->isChecked()){
        //     YZshowcell_checkBox_isChecked();
        // }
        // if(ui->YZshowcenter_checkBox->isChecked()){
        //     YZshowcenter_checkBox_isChecked();
        // }
        // if(ui->YZshowoutput_checkBox->isChecked()){
        //     YZshowoutput_checkBox_isChecked();
        // }
        // if(ui->YZshowexcitationplane_checkBox->isChecked()){
        //     YZshowexcitationplane_checkBox_isChecked();
        // }
        //-end- comment-out //2021.06.xx-01 voxビューアーにするため変更

        // if(ui->YZlength_checkBox->isChecked() || ui->YZlength_checkBox->text() == "Length_on"){
        if (lengthYZ_X.size() > 0 && lengthYZ_Y.size() > 0) {
            func_view_LengthAll();
        }
        if (ui->YZruler_checkBox->isChecked()) {
            func_view_ruler();
        }
        if (ui->YZautolength_checkBox->isChecked()) {
            func_view_AutoLength();
        }
    }
    else if (ui->tabWidget->currentIndex() == 2) {
        XYscene->clear();
        func_vox_make_XYgeometry();

        //-start- comment-out //2021.06.xx-01 voxビューアーにするため変更
        // if(ui->XYshowOutputVolumeCustom_checkBox->isChecked()){
        //    XYshowOutputVolumeCustom_checkBox_isChecked();
        //}
        if (ui->XYshowgrid_checkBox->isChecked()) {
            XYshowgrid_checkBox_isChecked();
        }
        // if(ui->XYshowoutput_checkBox->isChecked()){
        //     XYshowoutput_checkBox_isChecked();
        // }
        // if(ui->XYshowcell_checkBox->isChecked()){
        //     XYshowcell_checkBox_isChecked();
        // }
        // if(ui->XYshowcenter_checkBox->isChecked()){
        //     XYshowcenter_checkBox_isChecked();
        // }
        //-end- comment-out //2021.06.xx-01 voxビューアーにするため変更

        // if(ui->XYlength_checkBox->isChecked() || ui->XYlength_checkBox->text() == "Length_on"){
        if (lengthXY_X.size() > 0 && lengthXY_Y.size() > 0) {
            func_view_LengthAll();
        }
        if (ui->XYruler_checkBox->isChecked()) {
            func_view_ruler();
        }
        if (ui->XYautolength_checkBox->isChecked()) {
            func_view_AutoLength();
        }
    }
    else if (ui->tabWidget->currentIndex() == 3) {
        XZdiag_scene->clear();
        // 本番用//make_XZgeometry();
        func_vox_make_diagonal();    // 2021.06.xx-01 vox-2Dviewer

        if (ui->XZdiag_showgrid_checkBox->isChecked()) {
            XZdiag_showgrid_checkBox_isChecked();
        }

        // if(ui->XZlength_checkBox->isChecked() || ui->XZlength_checkBox->text() == "Length_on"){
        if (lengthXZdiag_X.size() > 0 && lengthXZdiag_Y.size() > 0) {
            func_view_LengthAll();
        }
        if (ui->XZdiag_ruler_checkBox->isChecked()) {
            qDebug() << "[DEBUG]viewer-update() XZdiag";
            func_view_ruler();
        }
        if (ui->XZdiag_autolength_checkBox->isChecked()) {
            func_view_AutoLength();
        }
    }
    if (FLAG_VIEWER_UPDATE == 1) {
        // 通常、常時リアルタイム更新なし。(FLAG_VIEWER_UPDATE = 1 の場合のみ、ビューアを更新する。)
        FLAG_VIEWER_UPDATE = 0;
    }
}

// GeometryInputForm *GeometryInputForm::getGIwinPtr() //2019.04.11 lineEditカスタム getUIfromOtherClass
//{
//     return pGIwin;
// }

// QString GeometryInputForm::getTextToOherWin(QString input_string) //2019.04.11 lineEditカスタム getUIfromOtherClass
// 他クラスからGeo画面のUI取得するため
//{
//     if(input_string == "mesh_lineEdit"){
//         return ui->meshsize_Value->text();
//     }
// }

void GeometryInputForm::func_load_OfficialList()    // Eyeris起動時にgetしたOfficialListをロードする。 //Eyeris
                                                    // MaterialForm.cppから流用
{
    QString InFilePath = "C:/temp/FDTDsolver_work/FTP_OfficialMaterialList/List_OfficialMaterials.txt";

    QStringList filelist;
    QFile       InFile01(InFilePath);
    if (!InFile01.open(QIODevice::ReadOnly)) {
        QString tmpStr = "can't open file:" + InFilePath;
        if (!InFile01.open(QIODevice::ReadOnly)) {
            QMessageBox::information(this, "can't open", tmpStr);
            return;
        }
    }

    // 　ファイル読み出し、テーブルへの反映
    QTextStream in(&InFile01);
    while (!in.atEnd()) {
        QString InFileLine = in.readLine(0);
        filelist << InFileLine;
    }

    //-start- 変更　vox2Dビューアー
    QStringList officialMatNameList;    // 変更前//officialMatNameList.clear();
    QStringList officialTypeList;
    QStringList officialVersionList;
    QStringList officialColorList;
    QStringList officialCategoryList;
    //-end- 変更　vox2Dビューアー

    for (int n = 0; n < filelist.size(); n++) {
        QString     tmpStr             = filelist.at(n);
        QStringList tmpList            = tmpStr.split(",");
        QString     filepathStr        = tmpList.at(0);
        QStringList filepathList       = filepathStr.split("/");
        QString     matName_and_Vesion = filepathList.at(1);

        QString officialType = filepathList.at(0);

        //.wnkファイル名側にVersionを追加したことによる処理
        QString Version = matName_and_Vesion.mid(matName_and_Vesion.length() - 3, 3);    // 末3文字取得　※バージョン
        // materialname = matName_and_Vesion.chop(4);
        matName_and_Vesion.chop(4);    // 末4文字(_***　※バージョン)削除
        QString materialname = matName_and_Vesion;

        if (officialMatNameList.indexOf(materialname) >= 0) {
            int         index          = officialMatNameList.indexOf(materialname);
            QString     tmpVersionStr  = officialVersionList.at(index);
            QStringList tmpVersionList = tmpVersionStr.split(",");
            if (!(tmpVersionList.indexOf(Version) >= 0)) {
                tmpVersionList << Version;
                tmpVersionList.sort();
                QString tmpStr;
                for (int m = 0; m < tmpVersionList.size() - 1; m++) {
                    tmpStr += tmpVersionList.at(m) + ",";
                }
                tmpStr += tmpVersionList.at(tmpVersionList.size() - 1);
                officialVersionList[index] = tmpStr;
            }
        }
        else {
            officialMatNameList << materialname;
            officialTypeList << officialType;
            officialVersionList << Version;
            officialColorList << tmpList.at(1);
            officialCategoryList << tmpList.at(2);
        }
    }

    // グローバル変数更新
    if (officialMatNameList.size() > 0) {
        g_hash_matnameToCname.clear();
    }
    for (int i = 0; i < officialMatNameList.size(); i++) {
        g_hash_matnameToCname[officialMatNameList.at(i)] = officialColorList.at(i);
    }

    acceptedMaterialList         = officialMatNameList;
    acceptedMaterialcolorList    = officialColorList;
    acceptedMaterialcategoryList = officialCategoryList;
    acceptedMaterialListforVox   = officialMatNameList;
}

// vox表示用に追加
void GeometryInputForm::func_vox_readHeader(QString in_voxpath)
{    // 2021.06.xx-01 voxファイルのヘッダー箇所の情報取得

    QString filePath1 = in_voxpath;

    g_orgMatNumStr = "";    // 例: "0,4,5,6,12,22,23,27,28";
    g_orgMatNameStr =
        "";    // 例:
               // "Air,CF-9th-B_Woollam,CF-9th-G_Woollam,CF-9th-R_Woollam,CT_Woollam,SCF-AO_SCK,SCF-PT_SCK,SiO2_SCK,Si";
    g_VoxMatIDList.clear();
    g_VoxMatNameList.clear();

    // voxfile1 を開く //参考:on_voxnAction_clicked()  make_XZgeometry() func_make_VoxFile() //viewerXZの描画
    int         nx1 = 0, ny1 = 0, nz1 = 0;
    int         cntHeaderLine   = -1;
    int         VoxUnitlength   = 20;
    QString     line_dataCount1 = "";
    QStringList VoxHeaderList1, VoxMatNameList1, VoxMatIDList1;
    QFile       voxinfile1(filePath1);
    if (!voxinfile1.open(QIODevice::ReadOnly)) {
        QMessageBox::information(this, "can't open", "can't open " + filePath1);
        return;
    }
    QTextStream in1(&voxinfile1);
    while (!in1.atEnd()) {
        QString VoxfileLine = in1.readLine(0);
        cntHeaderLine++;
        if (VoxfileLine.startsWith("Version")) {
            VoxHeaderList1 << VoxfileLine;
            continue;
        }
        else if (VoxfileLine.endsWith("precision")) {
            VoxHeaderList1 << VoxfileLine;
            continue;
        }
        else if (VoxfileLine.endsWith("unitlength")) {
            QStringList VoxfileLineList = VoxfileLine.split("\t");
            QString     tmpString       = VoxfileLineList.at(0);
            float       tmpFloat        = tmpString.toFloat();
            VoxUnitlength = int(tmpFloat * 1000);    //.voxファイルではum単位での記載　FDTDsolverのnm単位に合わせる
            VoxHeaderList1 << VoxfileLine;

            ui->meshsize_Value->setText(QString::number(VoxUnitlength));

            continue;
        }
        else if (VoxfileLine.startsWith("CellData")) {
            // CellData以後の行はヘッダー保持しない。
            continue;
        }
        else if (VoxfileLine.indexOf(" x ") > 0) {
            line_dataCount1             = VoxfileLine;
            QStringList VoxfileLineList = VoxfileLine.split(" x ");
            QString     nxstr           = VoxfileLineList.at(0);
            QString     nystr           = VoxfileLineList.at(1);
            QString     nzstr           = VoxfileLineList.at(2);
            nx1                         = nxstr.toInt();
            ny1                         = nystr.toInt();
            nz1                         = nzstr.toInt();

            g_nx = nx1;    // for-vox
            g_ny = ny1;
            g_nz = nz1;

            ui->domainX_Value->setText(QString::number(nx1 * VoxUnitlength));
            ui->domainY_Value->setText(QString::number(ny1 * VoxUnitlength));
            ui->domainZ_Value->setText(QString::number(nz1 * VoxUnitlength));

            ui->Ycoordinate_Slider->setMinimum(0);
            ui->Ycoordinate_Slider->setMaximum(ny1 * VoxUnitlength);
            ui->Xcoordinate_Slider->setMinimum(0);
            ui->Xcoordinate_Slider->setMaximum(nx1 * VoxUnitlength);
            ui->Zcoordinate_Slider->setMinimum(0);
            ui->Zcoordinate_Slider->setMaximum(nz1 * VoxUnitlength);
            ui->XZdiag_Ycoordinate_Slider->setMinimum(
                -nx1 * VoxUnitlength);    // 斜め断面の場合は、XのslicePosition( XZビューとは違い nyにしない）
            ui->XZdiag_Ycoordinate_Slider->setMaximum(nx1 * VoxUnitlength);

            // ui->dZ_defined_radioButton->setChecked(true);
            // ui->dZ_defined_lineEdit->setText(QString::number(nz1 * VoxUnitlength));

            break;    // ループ抜ける
        }
        else {
            VoxHeaderList1 << VoxfileLine;
            // マテリアル番号として保持する
            QStringList VoxfileLineList = VoxfileLine.split('\t');
            g_VoxMatIDList << VoxfileLineList.at(0);
            g_VoxMatNameList << VoxfileLineList.at(1);

            if (cntHeaderLine == 0) {
                g_orgMatNumStr  = VoxfileLineList.at(0);    // 例: "0,4,5,6,12,22,23,27,28";
                g_orgMatNameStr = VoxfileLineList.at(
                    1);    // 例:
                           // "Air,CF-9th-B_Woollam,CF-9th-G_Woollam,CF-9th-R_Woollam,CT_Woollam,SCF-AO_SCK,SCF-PT_SCK,SiO2_SCK,Si";
            }
            else {
                g_orgMatNumStr += "," + VoxfileLineList.at(0);    // 例: "0,4,5,6,12,22,23,27,28";
                g_orgMatNameStr +=
                    ","
                    + VoxfileLineList.at(
                        1);    // 例:
                               // "Air,CF-9th-B_Woollam,CF-9th-G_Woollam,CF-9th-R_Woollam,CT_Woollam,SCF-AO_SCK,SCF-PT_SCK,SiO2_SCK,Si";
            }
            continue;
        }
    }

    // QString tmpstr;
    // qDebug() << tmpstr.asprintf("[DEBUG]domainX=%d Y=%d Z=%d", nx1, ny1, nz1);
}

//-end- XZ, YZ, XYビュー共通処理

void GeometryInputForm::on_XZRedraw_pushButton_clicked()
{
    FLAG_VIEWER_UPDATE = 1;
    viewer_update();
}
//-end- XZビュー表示

//-start- YZビュー表示
void GeometryInputForm::on_YZRedraw_pushButton_clicked()
{
    FLAG_VIEWER_UPDATE = 1;
    viewer_update();
}

//-start- XYビュー表示

void GeometryInputForm::on_XYRedraw_pushButton_clicked()
{
    FLAG_VIEWER_UPDATE = 1;
    viewer_update();    // この関数で func_vox_make_XYgeometry()も呼び出される。
}

void GeometryInputForm::
    func_vox_make_XYgeometry()    // viewerXYの描画
                                  // //vox都度読み込みのまま、まだ保持した配列からの読み込みはできていない。。これから対応する予定
{
    XYscene->clear();
    int dX   = ui->domainX_Value->text().toInt();
    int dY   = ui->domainY_Value->text().toInt();
    int dZ   = ui->domainZ_Value->text().toInt();
    int Mesh = ui->meshsize_Value->text().toInt();
    //    int mesh = Mesh;
    int     Xdata      = dX / Mesh;
    int     Ydata      = dY / Mesh;
    int     Zdata      = dZ / Mesh;
    int     ZposSlider = ui->Zcoordinate_spin_lineEdit->text().toInt();
    int     Zpos       = ui->Zcoordinate_spin_lineEdit->text().toInt() - 1;
    int     Zposdata   = (int)((Zpos + (double)Mesh / 2 - 0.00001) / Mesh);
    QString tmp_tooltip;
    QString tmpString;
    float   tmpFloat;
    int     VoxUnitlength;

    // テーブルから、マテリアル番号から色を特定するハッシュの作成
    QHash<int, QString> matnumToColorHash;
    for (int row = 0; row < ui->tableWidget->rowCount(); row++) {
        int               matnum   = ui->tableWidget->item(row, 1)->text().toInt();
        CustomColorCombo* tmpcombo = static_cast<CustomColorCombo*>(ui->tableWidget->cellWidget(row, 3));
        QString           color    = tmpcombo->currentText();
        matnumToColorHash.insert(matnum, color);
    }

    // pixmapによる描画
    int     XWidthPixmap = dX;
    int     YWidthPixmap = dY;
    QPixmap pix(XWidthPixmap,
                YWidthPixmap);    // マイナスは指定できない
                                  // 最終のitem->offset　有り無しいずれにしてもitem1->setOffset(0, -YWidthPixmap );
    QPainter painter(&pix);

    QBrush lightcyanBrush(QColor(224, 255, 255, 255));
    QPen   whitePen(QColor(255, 255, 255, 0));

    int     i = 1;    // ←[DEBUG]//for(int i = 0; i < ui->tableWidget->rowCount(); i++){
    QString shape =
        ".vox (Normal)";    // QString shape = "BOX"; //←[DEBUG] //QString shape = ui->tableWidget->item(i, 0)->text();

    //    if(shape.startsWith("BOX")){  //------------------------------------------------------------BOX入力
    //        double xmin_d = 0; //ui->tableWidget->item(i, 4)->text().toDouble();
    //        double xmax_d = 2000; //ui->tableWidget->item(i, 6)->text().toDouble();
    //        //int xmin = (int)((xmin_d + (double)Mesh / 2 - 0.00001) / Mesh) * Mesh;
    //        //int xmax = (int)((xmax_d + (double)Mesh / 2 - 0.00001) / Mesh) * Mesh;

    //        int xmin, xmax;

    //        if(xmin_d >= 0){
    //            xmin = (int)((xmin_d + (double)Mesh / 2 - 0.00001) / (double)Mesh) * Mesh;
    //        }else if(xmin_d < 0){
    //            xmin = (int)((xmin_d - (double)Mesh / 2 + 0.00001) / (double)Mesh) * Mesh;
    //        }

    //        if(xmax_d >= 0){
    //            xmax = (int)((xmax_d + (double)Mesh / 2 - 0.00001) / (double)Mesh) * Mesh;
    //        }else if(xmax_d < 0){
    //            xmax = (int)((xmax_d - (double)Mesh / 2 + 0.00001) / (double)Mesh) * Mesh;
    //        }

    //        if(xmin > xmax){
    //            swap(&xmin, &xmax);
    //        }else if(xmin == xmax){
    //            //continue;
    //        }
    //        double ymin_d = 0; //ui->tableWidget->item(i, 8)->text().toDouble();
    //        double ymax_d = 2000; //ui->tableWidget->item(i, 10)->text().toDouble();
    //        //int ymin = (int)((ymin_d + (double)Mesh / 2 - 0.00001) / Mesh) * Mesh;
    //        //int ymax = (int)((ymax_d + (double)Mesh / 2 - 0.00001) / Mesh) * Mesh;

    //        int ymin, ymax;

    //        if(ymin_d >= 0){
    //            ymin = (int)((ymin_d + (double)Mesh / 2 - 0.00001) / (double)Mesh) * Mesh;
    //        }else if (ymin_d < 0){
    //            ymin = (int)((ymin_d - (double)Mesh / 2 + 0.00001) / (double)Mesh) * Mesh;
    //        }

    //        if(ymax_d >= 0){
    //            ymax = (int)((ymax_d + (double)Mesh / 2 - 0.00001) / (double)Mesh) * Mesh;
    //        }else if(ymax_d < 0){
    //            ymax = (int)((ymax_d - (double)Mesh / 2 + 0.00001) / (double)Mesh) * Mesh;
    //        }

    //        if(ymin > ymax){
    //            swap(&ymin, &ymax);
    //        }else if(ymin == ymax){
    //            //continue;
    //        }
    //        double zmin_d = 0; //ui->tableWidget->item(i, 12)->text().toDouble();
    //        double zmax_d = 8600; //ui->tableWidget->item(i, 14)->text().toDouble();
    //        //int zmin = (int)((zmin_d + (double)Mesh / 2 - 0.00001) / Mesh) * Mesh;
    //        //int zmax = (int)((zmax_d + (double)Mesh / 2 - 0.00001) / Mesh) * Mesh;

    //        int zmin, zmax;

    //        if(zmin_d >= 0){
    //            zmin = (int)((zmin_d + (double)Mesh / 2 - 0.00001) / (double)Mesh) * Mesh;
    //        }else if(zmin_d < 0){
    //            zmin = (int)((zmin_d - (double)Mesh / 2 + 0.00001) / (double)Mesh) * Mesh;
    //        }

    //        if(zmax_d >= 0){
    //            zmax = (int)((zmax_d + (double)Mesh / 2 - 0.00001) / (double)Mesh) * Mesh;
    //        }else if(zmax_d < 0){
    //            zmax = (int)((zmax_d - (double)Mesh / 2 + 0.00001) / (double)Mesh) * Mesh;
    //        }

    //        if(zmin > zmax){
    //            swap(&zmin, &zmax);
    //        }else if(zmin == zmax){
    //            //continue;
    //        }

    //        //QComboBox *tmp_combo = new QComboBox();
    //        //tmp_combo = static_cast<QComboBox*>(ui->tableWidget->cellWidget(i, 1));
    //        //QString tmp_mat = tmp_combo->currentText(); //更新前のComboBoxのselected itemを一時保存
    //        //QString matcolor;

    //        //for(int n = 0; n < acceptedMaterialList.size(); n++){
    //        //    if(QString::compare(tmp_mat, acceptedMaterialList.at(n)) == 0){
    //        //        matcolor = acceptedMaterialcolorList.at(n);
    //        //    }
    //        //}
    //        QString matcolor = "Red"; //DEBUG 2021.06.xx-01 thrash2D用途

    //        //2018.08.09 機能追加による書き換え
    //        int b = func_setBrushesNum(matcolor);
    //        int arrayX = 1;
    //        int arrayY = 1;
    //        int pitchX = 0;
    //        int pitchY = 0;
    //        //if(ui->tableWidget->item(i, 30)->text().toInt() == 1){
    //        //    arrayX = ui->tableWidget->item(i,31)->text().toInt();
    //        //    arrayY = ui->tableWidget->item(i,32)->text().toInt();
    //        //    pitchX = ui->tableWidget->item(i,34)->text().toInt();
    //        //    pitchY = ui->tableWidget->item(i,36)->text().toInt();
    //        //}
    //        int xmin_default = xmin;
    //        int xmax_default = xmax;
    //        int ymin_default = ymin;
    //        int ymax_default = ymax;

    //        //tmp_tooltip = QString::number(i+1) + " " + tmp_mat;

    //        //if(ui->tableWidget->item(i, 22)->text().toInt() == 0 && ui->tableWidget->item(i, 28)->text().toInt() ==
    //        0){ //Round・Taper無し for(int aY = 0; aY < arrayY; aY++){
    //            for(int aX = 0; aX < arrayX; aX++){
    //                xmin = xmin_default + aX * pitchX;
    //                xmax = xmax_default + aX * pitchX;
    //                ymin = ymin_default + aY * pitchY;
    //                ymax = ymax_default + aY * pitchY;
    //                if(Zposdata >= zmin / Mesh && Zposdata < zmax / Mesh && xmin < dX && ymin < dY){
    //                    int tmp_xposition = xmin;
    //                    int tmp_yposition = ymin;
    //                    int tmp_xWidth = xmax - xmin;
    //                    int tmp_yWidth = ymax - ymin;
    //                    QRect rect(tmp_xposition, tmp_yposition, tmp_xWidth, tmp_yWidth);
    //                    painter.fillRect(rect, Brushes[b]);
    //                }
    //            }
    //        }
    //        //}
    //        //-end- if(ui->tableWidget->item(i, 22)->text().toInt() == 0

    //    }
    //    //-end- if(shape.startsWith("BOX"))

    if (shape.startsWith(
            ".vox (Normal)")) {    //------------------------------------------------------------.vox (Normal)入力
        double xposition_d = 0;    // ui->tableWidget->item(i, 4)->text().toDouble();
        double yposition_d = 0;    // ui->tableWidget->item(i, 8)->text().toDouble();
        double zposition_d = 0;    // ui->tableWidget->item(i, 12)->text().toDouble();

        // int xposition = (int)((xposition_d + (double)Mesh / 2 - 0.00001) / Mesh) * Mesh;
        // int yposition = (int)((yposition_d + (double)Mesh / 2 - 0.00001) / Mesh) * Mesh;
        // int zposition = (int)((zposition_d + (double)Mesh / 2 - 0.00001) / Mesh) * Mesh;
        // 　ユーザーがMesh値を変えると図形縮小拡大してしまう点の修正
        // int xposition = xposition_d;
        // int yposition = yposition_d;
        // int zposition = zposition_d;

        int xposition, yposition, zposition;

        if (xposition_d >= 0) {
            xposition = (int)((xposition_d + (double)Mesh / 2 - 0.00001) / (double)Mesh) * Mesh;
        }
        else if (xposition_d < 0) {
            xposition = (int)((xposition_d - (double)Mesh / 2 + 0.00001) / (double)Mesh) * Mesh;
        }

        if (yposition_d >= 0) {
            yposition = (int)((yposition_d + (double)Mesh / 2 - 0.00001) / (double)Mesh) * Mesh;
        }
        else if (yposition_d < 0) {
            yposition = (int)((yposition_d - (double)Mesh / 2 + 0.00001) / (double)Mesh) * Mesh;
        }

        if (zposition_d >= 0) {
            zposition = (int)((zposition_d + (double)Mesh / 2 - 0.00001) / (double)Mesh) * Mesh;
        }
        else if (zposition_d < 0) {
            zposition = (int)((zposition_d - (double)Mesh / 2 + 0.00001) / (double)Mesh) * Mesh;
        }

        QString orgMatNumStr = g_orgMatNumStr;    // 例: "0,4,5,6,12,22,23,27,28";//ui->tableWidget->item(i,
                                                  // 16)->text();
        QString newMatNumStr =
            g_orgMatNumStr;    // 例: "0,4,5,6,12,22,23,27,28"; //DEBUG	//ui->tableWidget->item(i, 18)->text();
        QString newMatNameStr =
            g_orgMatNameStr;    // 例:
                                // "Air,CF-9th-B_Woollam,CF-9th-G_Woollam,CF-9th-R_Woollam,CT_Woollam,SCF-AO_SCK,SCF-PT_SCK,SiO2_SCK,Si";
                                // //DEBUG//ui->tableWidget->item(i, 19)->text();
        QStringList orgMatNumList  = orgMatNumStr.split(",");
        QStringList newMatNumList  = newMatNumStr.split(",");
        QStringList newMatNameList = newMatNameStr.split(",");
        QString     newMatName;

        QString RotationState = 0;    // ui->tableWidget->item(i, 21)->text();
        QString BoostState    = 0;    // ui->tableWidget->item(i, 42)->text();

        //-start- 2018.08.09 array機能追加による書き換え
        int arrayX = 1;
        int arrayY = 1;
        int pitchX = 0;
        int pitchY = 0;
        //-start- 2021.04.xx-01 vox2Dviewerのためコメントアウト
        // if(ui->tableWidget->item(i, 30)->text().toInt() == 1){
        //    arrayX = ui->tableWidget->item(i,31)->text().toInt();
        //    arrayY = ui->tableWidget->item(i,32)->text().toInt();
        //    pitchX = ui->tableWidget->item(i,34)->text().toInt();
        //    pitchY = ui->tableWidget->item(i,36)->text().toInt();
        //}
        //-end- 2021.04.xx-01 vox2Dviewerのためコメントアウト
        int xposition_default = xposition;
        int yposition_default = yposition;

        for (int aY = 0; aY < arrayY; aY++) {
            for (int aX = 0; aX < arrayX; aX++) {
                xposition = xposition_default + aX * pitchX;
                yposition = yposition_default + aY * pitchY;

                QFile voxinfile(
                    ui->vox_path_lineEdit->text());    // 例 "C:/kuroda/work/00_Data_vox/test_basic/test_basic.vox")
                if (!voxinfile.open(QIODevice::ReadOnly)) {
                    // QMessageBox::information(this, "can't open", "can't open");
                    return;
                }

                QTextStream in(&voxinfile);
                QString     VoxfileLine;
                QStringList VoxfileLineList;
                QString     nxstr, nystr, nzstr;
                int         nx, ny, nz;
                while (!in.atEnd()) {
                    VoxfileLine = in.readLine(0);
                    if (VoxfileLine.indexOf(" x ") > 0) {
                        VoxfileLineList = VoxfileLine.split(" x ");
                        nxstr           = VoxfileLineList.at(0);
                        nystr           = VoxfileLineList.at(1);
                        nzstr           = VoxfileLineList.at(2);
                        nx              = nxstr.toInt();
                        ny              = nystr.toInt();
                        nz              = nzstr.toInt();
                        break;
                    }
                    if (VoxfileLine.indexOf("unitlength") > 0) {
                        VoxfileLineList = VoxfileLine.split("\t");
                        tmpString       = VoxfileLineList.at(0);
                        tmpFloat        = tmpString.toFloat();
                        VoxUnitlength =
                            int(tmpFloat * 1000);    //.voxファイルではum単位での記載　FDTDsolverのnm単位に合わせる
                        // qDebug() << "[DEBUG]VoxfileLineList.at(0)" << tmpString <<"Mesh=" << QString::number(Mesh);
                    }
                }

                if (BoostState == "0" || BoostState == "") {
                    for (int z = 0; z < nz; z++) {
                        for (int y = 0; y < ny; y++) {
                            VoxfileLine = in.readLine(0);

                            if (z == ((ZposSlider - zposition) / Mesh)) {
                                VoxfileLineList = VoxfileLine.split(" ");
                                for (int l = 0; l < (VoxfileLineList.size() / 2); l++) {
                                    QString orgMatNum = VoxfileLineList.at(l * 2);
                                    QString newMatNum;
                                    for (int m = 0; m < orgMatNumList.size(); m++) {
                                        if (QString::compare(orgMatNumList.at(m), orgMatNum) == 0) {
                                            newMatNum  = newMatNumList.at(m);
                                            newMatName = newMatNameList.at(m);
                                        }
                                    }
                                    VoxfileLineList.replace(l * 2, newMatNum);
                                }
                                int countx = 0;
                                // 2018.10.04 add Rotation
                                if (RotationState == "0" || RotationState == "") {
                                    for (int x = 0; x < VoxfileLineList.size() / 2; x++) {
                                        QString tmpnumstr1 = VoxfileLineList.at(2 * x);
                                        int     tmpnum1    = tmpnumstr1.toInt();
                                        QString tmpnumstr2 = VoxfileLineList.at(2 * x + 1);
                                        int     tmpnum2    = tmpnumstr2.toInt();

                                        int tmp_xposition = xposition + (countx * Mesh);
                                        int tmp_yposition = yposition + (y * Mesh);
                                        int tmp_xWidth    = tmpnum2 * Mesh;
                                        int tmp_yWidth    = Mesh;

                                        // ΔX範囲をはみ出す場合の対処
                                        if ((tmp_xposition + tmp_xWidth) > dX) {
                                            tmp_xWidth = dX - tmp_xposition;
                                        }
                                        // ΔY範囲をはみ出す場合の対処
                                        if (tmp_yposition > dY) {
                                            tmp_yWidth = dY - tmp_yposition;
                                        }

                                        // X始点が0以下の場合の対処（ユーザーがマイナスオフセットしている場合)
                                        if (tmp_xposition < 0 && (tmp_xposition + tmp_xWidth > 0)) {
                                            tmp_xWidth    = tmp_xWidth - abs(tmp_xposition);
                                            tmp_xposition = 0;
                                        }

                                        // Y始点が0以下の場合の対処（ユーザーがマイナスオフセットしている場合)
                                        if (tmp_yposition < 0 && (tmp_yposition + tmp_yWidth > 0)) {
                                            tmp_yWidth    = tmp_yWidth - abs(tmp_yposition);
                                            tmp_yposition = 0;
                                        }

                                        if (tmp_xposition <= dX && tmp_yposition <= dY
                                            && (tmp_xposition + tmp_xWidth > 0) && (tmp_yposition + tmp_yWidth > 0)) {
                                            if (tmpnum1 == 0) {
                                                QRect rect(tmp_xposition, tmp_yposition, tmp_xWidth, tmp_yWidth);
                                                // QString  matcolor = acceptedMaterialcolorList.at(0); //2018.02.14
                                                // Airの表示色も変えられるよう処理変更 2021.06.xx-01 thrush2D用途
                                                // commentout//int b = func_setBrushesNum(matcolor); -start-
                                                // 2021.06.xx-01 thrush2D用途 int b = 1; //暫定初期値 Gray b =
                                                // tmpnumstr1.toInt() % 20;
                                                // //色定義関数　func_setBrushes()　で現在20色まであるため。 int matID =
                                                // tmpnumstr1.toInt(); if(g_VoxMatIDList.indexOf(QString::number(matID))
                                                // > -1){
                                                //     b = g_VoxMatIDList.indexOf(QString::number(matID))  % 20;
                                                //     //できるだけ全部違う色になるようにする if(b < 4){ b= b+ 15; }
                                                //     //[DEBUG]暫定　黒とか白は避けたい
                                                // }
                                                //-end- 2021.06.xx-01 thrush2D用途
                                                int b      = 1;    // 暫定初期値 Gray
                                                int matnum = tmpnumstr1.toInt();
                                                if (matnumToColorHash.contains(matnum)) {
                                                    QString colorname = matnumToColorHash.value(matnum);
                                                    b                 = func_setBrushesNum(colorname);
                                                }
                                                if (b != -1) {
                                                    painter.fillRect(rect, Brushes[b]);
                                                }
                                            }
                                            else if (tmpnum1 != 9999) {
                                                tmp_tooltip =
                                                    QString::number(i + 1)
                                                    + " .voxfile";    //.vox
                                                                      // ファイルの場合は形状が何列目のものかだけ表示する
                                                QRect rect(tmp_xposition, tmp_yposition, tmp_xWidth, tmp_yWidth);
                                                // 2021.06.xx-01 thrush2D用途 commentout//QString  matcolor =
                                                // acceptedMaterialcolorList.at(tmpnum1 - 1); int b =
                                                // func_setBrushesNum(matcolor); int b = 1; //暫定初期値 Gray b =
                                                // tmpnumstr1.toInt() % 20;
                                                // //色定義関数　func_setBrushes()　で現在20色まであるため。 int matID =
                                                // tmpnumstr1.toInt(); if(g_VoxMatIDList.indexOf(QString::number(matID))
                                                // > -1){
                                                //     b = g_VoxMatIDList.indexOf(QString::number(matID))  % 20;
                                                //     //できるだけ全部違う色になるようにする if(b < 4){ b= b+ 15; }
                                                //     //[DEBUG]暫定　黒とか白は避けたい
                                                // }
                                                ////-end- 2021.06.xx-01 thrush2D用途
                                                int b     = 1;    // 暫定初期値 Gray
                                                int matID = tmpnumstr1.toInt();
                                                if (matnumToColorHash.contains(matID)) {
                                                    QString colorname = matnumToColorHash.value(matID);
                                                    b                 = func_setBrushesNum(colorname);
                                                }
                                                if (b != -1) {
                                                    painter.fillRect(rect, Brushes[b]);
                                                }
                                            }
                                        }
                                        countx += tmpnum2;
                                    }
                                    //-end- for(int x = 0; x < VoxfileLineList.size() / 2; x++)
                                }
                                //-end- if(RotationState == "0" || RotationState == "")
                            }
                            //-end- if(z == ( (ZposSlider - zposition) / Mesh))
                        }
                        //-end- for(int y = 0; y < ny; y++)
                    }
                    //-end- for(int z = 0; z < nz; z++)
                }
                //-end- if(BoostState == "0" || BoostState == "")
                voxinfile.close();
            }
        }
    }
    //-end- if(shape.startsWith(".vox (Normal)")

    // pixmapによる描画
    item1 = new QGraphicsPixmapItem();
    // 上下反転
    QTransform matrix(1.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 1.0);
    item1->setTransform(matrix);
    item1->setPixmap(pix);
    XYscene->addItem(item1);

    pix_XYscene = pix;    // DEBUG 2018.06.xx-01 outputVolume機能　volume表示のため
}

//-end- XYビュー表示

void GeometryInputForm::on_XZdiag_Redraw_pushButton_clicked()
{
    FLAG_VIEWER_UPDATE = 1;
    viewer_update();    // この中で、func_vox_make_diagonal("XZ45")　も呼び出される。
}

void GeometryInputForm::update_Ycoordinate_spinBox()    // viewerXZのYpositionのスピンボックスとスライダーの連携
                                                        // Slider値変更時点で呼び出される
{
    // 2017.11.01以前の処理→//ui->Ycoordinate_spinBox->setValue(ui->Ycoordinate_Slider->value());

    // 補足：//ver20181010_002 ではスピンボックス↑↓ボタンでの即時update対応。
    // 補足：//spinBox,sliderはMesh刻みの数値のみで表示する（ユーザー希望仕様)については.uiファイルslider設定している。ただし、それだけでは実現しないため下記Mesh値にする処理も実行している

    //-start- spinBox,sliderはMesh刻みの数値のみで表示する（ユーザー希望仕様)
    ////関数update_Xcoordinate_Slider() connectにより呼び出されて2回処理されてしまうことを防止
    if (COUNT_SPINBOX_RUN > 0) {
        COUNT_SPINBOX_RUN = 0;
        return;
    }
    ////スライダー変更時、スピンボックスの値を連携させる
    int mesh_coordinate =
        ui->Ycoordinate_Slider->value() - (ui->Ycoordinate_Slider->value() % ui->meshsize_Value->text().toInt());
    if ((ui->Ycoordinate_Slider->value() - ui->meshsize_Value->text().toInt()) < 1) {
        mesh_coordinate = 1;
    }

    if (mesh_coordinate > (ui->domainY_Value->text().toInt() - ui->meshsize_Value->text().toInt())) {
        // （ユーザー希望仕様) spinBox,sliderの最大表示範囲は(ドメインmax-1Meshの距離) 例:ドメインmax=1000 Mesh=20
        // →　1　～ 980um まで表示
        mesh_coordinate = ui->domainY_Value->text().toInt() - ui->meshsize_Value->text().toInt();
        // QMessageBox::information(this, "info", "info: view max is domein - 1Mesh");
        // ui->XZinfo_label->setText("info: view max is domein - 1Mesh");
    }

    ui->Ycoordinate_spin_lineEdit->setText(QString::number(mesh_coordinate));
    //-end- spinBox,sliderはMesh刻みの数値のみで表示する（ユーザー希望仕様)
    FLAG_VIEWER_UPDATE = 1;
}

void GeometryInputForm::update_Zcoordinate_spinBox()    // viewerXYのZpositionのスピンボックスとスライダーの連携
                                                        // Slider値変更時点で呼び出される
{
    // 2017.11.01以前の処理→//ui->Zcoordinate_spinBox->setValue(ui->Zcoordinate_Slider->value());

    // 補足：//ver20181010_002 ではスピンボックス↑↓ボタンでの即時update対応。
    // 補足：//spinBox,sliderはMesh刻みの数値のみで表示する（ユーザー希望仕様)については.uiファイルslider設定している。ただし、それだけでは実現しないため下記Mesh値にする処理も実行している

    //-start- spinBox,sliderはMesh刻みの数値のみで表示する（ユーザー希望仕様)
    ////関数update_Zcoordinate_Slider() connectにより呼び出されて2回処理されてしまうことを防止
    if (COUNT_SPINBOX_RUN > 0) {
        COUNT_SPINBOX_RUN = 0;
        return;
    }
    ////スライダー変更時、スピンボックスの値を連携させる
    int mesh_coordinate =
        ui->Zcoordinate_Slider->value() - (ui->Zcoordinate_Slider->value() % ui->meshsize_Value->text().toInt());
    if ((ui->Zcoordinate_Slider->value() - ui->meshsize_Value->text().toInt()) < 1) {
        mesh_coordinate = 1;
    }

    if (mesh_coordinate > (ui->domainZ_Value->text().toInt() - ui->meshsize_Value->text().toInt())) {
        // （ユーザー希望仕様) spinBox,sliderの最大表示範囲は(ドメインmax-1Meshの距離) 例:ドメインmax=1000 Mesh=20
        // →　1　～ 980um まで表示
        mesh_coordinate = ui->domainZ_Value->text().toInt() - ui->meshsize_Value->text().toInt();
        // QMessageBox::information(this, "info", "info: view max is domein - 1Mesh");
        // ui->XYinfo_label->setText("info: view max is domein - 1Mesh");
    }

    ui->Zcoordinate_spin_lineEdit->setText(QString::number(mesh_coordinate));
    //-end- spinBox,sliderはMesh刻みの数値のみで表示する（ユーザー希望仕様)
    FLAG_VIEWER_UPDATE = 1;
}

void GeometryInputForm::update_Xcoordinate_spinBox()    // 2019.05.16
                                                        // viewerYZのXpositionのスピンボックスとスライダーの連携
                                                        // Slider値変更時点で呼び出される
{
    // 2017.11.01以前の処理→//ui->Xcoordinate_spinBox->setValue(ui->Xcoordinate_Slider->value());

    // 補足：//ver20181010_002 ではスピンボックス↑↓ボタンでの即時update対応。
    // 補足：//spinBox,sliderはMesh刻みの数値のみで表示する（ユーザー希望仕様)については.uiファイルslider設定している。ただし、それだけでは実現しないため下記Mesh値にする処理も実行している

    //-start- spinBox,sliderはMesh刻みの数値のみで表示する（ユーザー希望仕様)
    ////関数update_Xcoordinate_Slider() connectにより呼び出されて2回処理されてしまうことを防止
    if (COUNT_SPINBOX_RUN > 0) {
        COUNT_SPINBOX_RUN = 0;
        return;
    }
    ////スライダー変更時、スピンボックスの値を連携させる
    int mesh_coordinate =
        ui->Xcoordinate_Slider->value() - (ui->Xcoordinate_Slider->value() % ui->meshsize_Value->text().toInt());
    if ((ui->Xcoordinate_Slider->value() - ui->meshsize_Value->text().toInt()) < 1) {
        mesh_coordinate = 1;
    }

    if (mesh_coordinate > (ui->domainX_Value->text().toInt() - ui->meshsize_Value->text().toInt())) {
        // （ユーザー希望仕様) spinBox,sliderの最大表示範囲は(ドメインmax-1Meshの距離) 例:ドメインmax=1000 Mesh=20
        // →　1　～ 980um まで表示
        mesh_coordinate = ui->domainX_Value->text().toInt() - ui->meshsize_Value->text().toInt();
        // QMessageBox::information(this, "info", "info: view max is domein - 1Mesh");
        // ui->YZinfo_label->setText("info: view max is domein - 1Mesh");
    }

    ui->Xcoordinate_spin_lineEdit->setText(QString::number(mesh_coordinate));
    //-end- spinBox,sliderはMesh刻みの数値のみで表示する（ユーザー希望仕様)
    FLAG_VIEWER_UPDATE = 1;
}

void GeometryInputForm::
    update_XZdiag_Ycoordinate_spinBox()    // 2021.04.xx-01 vox-2Dviewer
                                           // 斜め断面　viewerXZのYpositionのスピンボックスとスライダーの連携
                                           // Slider値変更時点で呼び出される
{
    // 2017.11.01以前の処理→//ui->XZdiag_Ycoordinate_spinBox->setValue(ui->XZdiag_Ycoordinate_Slider->value());

    // 補足：//ver20181010_002 ではスピンボックス↑↓ボタンでの即時update対応。
    // 補足：//spinBox,sliderはMesh刻みの数値のみで表示する（ユーザー希望仕様)については.uiファイルslider設定している。ただし、それだけでは実現しないため下記Mesh値にする処理も実行している

    //-start- spinBox,sliderはMesh刻みの数値のみで表示する（ユーザー希望仕様)
    ////関数update_Xcoordinate_Slider() connectにより呼び出されて2回処理されてしまうことを防止
    if (COUNT_SPINBOX_RUN > 0) {
        COUNT_SPINBOX_RUN = 0;
        return;
    }
    ////スライダー変更時、スピンボックスの値を連携させる
    int mesh_coordinate = ui->XZdiag_Ycoordinate_Slider->value()
                        - (ui->XZdiag_Ycoordinate_Slider->value() % ui->meshsize_Value->text().toInt());
    // 2021.04.xx-01 斜め断面はマイナス値も取りうる
    // if( (ui->XZdiag_Ycoordinate_Slider->value() - ui->meshsize_Value->text().toInt()) < 1){
    //     mesh_coordinate = 1;
    // }

    if (mesh_coordinate
        > (ui->domainX_Value->text().toInt()
           - ui->meshsize_Value->text().toInt())) {    // 斜め断面の場合 slider最大値はdomainX (domainYではない）
        // （ユーザー希望仕様) spinBox,sliderの最大表示範囲は(ドメインmax-1Meshの距離) 例:ドメインmax=1000 Mesh=20
        // →　1　～ 980um まで表示
        mesh_coordinate = ui->domainX_Value->text().toInt() - ui->meshsize_Value->text().toInt();
        // QMessageBox::information(this, "info", "info: view max is domein - 1Mesh");
        // ui->XZdiag_info_label->setText("info: view max is domein - 1Mesh");
    }

    ui->XZdiag_Ycoordinate_spin_lineEdit->setText(QString::number(mesh_coordinate));
    //-end- spinBox,sliderはMesh刻みの数値のみで表示する（ユーザー希望仕様)
    FLAG_VIEWER_UPDATE = 1;
}

void GeometryInputForm::update_Xcoordinate_Slider()    // viewerYZのXpositionのスピンボックスとスライダーの連携
                                                       // スピンボックス値変更時に呼び出される
{
    // 補足：//ver20181010_002 スピンボックス↑↓ボタンでの即時update対応。
    // 補足：//spinBox,sliderはMesh刻みの数値のみで表示する（ユーザー希望仕様)については.ui側のフォーム設定でされる。

    //-start- spinBox,sliderはMesh刻みの数値のみで表示する（ユーザー希望仕様)
    COUNT_SPINBOX_RUN++;
    if (COUNT_SPINBOX_RUN > 1) {
        COUNT_SPINBOX_RUN = 0;
        return;
    }
    else {
        ui->Xcoordinate_Slider->setValue(ui->Xcoordinate_spin_lineEdit->text().toInt());
    }

    FLAG_VIEWER_UPDATE = 1;
}

void GeometryInputForm::update_Ycoordinate_Slider()    // viewerXZのYpositionのスピンボックスとスライダーの連携
                                                       // スピンボックス値変更時に呼び出される
{
    // 補足：//ver20181010_002 スピンボックス↑↓ボタンでの即時update対応。
    // 補足：//2019.05.14以前　spinBox,sliderはMesh刻みの数値のみで表示する（ユーザー希望仕様)については.ui側のフォーム設定でされる。
    // 補足：//2019.05.14以後 spinBox から
    // lineEditに変更したことによる仕様変更。事前にfunc_on_Ycoordinate_spin_lineEdit_editingFinished()　で処理済にしておく。

    //-start- spinBox,sliderはMesh刻みの数値のみで表示する（ユーザー希望仕様)
    COUNT_SPINBOX_RUN++;
    if (COUNT_SPINBOX_RUN > 1) {
        COUNT_SPINBOX_RUN = 0;
        return;
    }
    else {
        ui->Ycoordinate_Slider->setValue(ui->Ycoordinate_spin_lineEdit->text().toInt());
    }

    FLAG_VIEWER_UPDATE = 1;
}

void GeometryInputForm::update_Zcoordinate_Slider()    // viewerXYのZpositionのスピンボックスとスライダーの連携
                                                       // スピンボックス値変更時に呼び出される
{
    // 補足：//ver20181010_002 ではスピンボックス↑↓ボタンでの即時update対応。
    // 補足：//spinBox,sliderはMesh刻みの数値のみで表示する（ユーザー希望仕様)については.ui側のフォーム設定でされる。

    //-start- spinBox,sliderはMesh刻みの数値のみで表示する（ユーザー希望仕様)
    COUNT_SPINBOX_RUN++;
    if (COUNT_SPINBOX_RUN > 1) {
        COUNT_SPINBOX_RUN = 0;
        return;
    }
    else {
        ui->Zcoordinate_Slider->setValue(ui->Zcoordinate_spin_lineEdit->text().toInt());
    }

    FLAG_VIEWER_UPDATE = 1;
}

void GeometryInputForm::update_XZdiag_Ycoordinate_Slider()    // viewerXZのYpositionのスピンボックスとスライダーの連携
                                                              // スピンボックス値変更時に呼び出される
{
    // 補足：//ver20181010_002 スピンボックス↑↓ボタンでの即時update対応。
    // 補足：//2019.05.14以前　spinBox,sliderはMesh刻みの数値のみで表示する（ユーザー希望仕様)については.ui側のフォーム設定でされる。
    // 補足：//2019.05.14以後 spinBox から
    // lineEditに変更したことによる仕様変更。事前にfunc_on_XZdiag_Ycoordinate_spin_lineEdit_editingFinished()　で処理済にしておく。

    //-start- spinBox,sliderはMesh刻みの数値のみで表示する（ユーザー希望仕様)
    COUNT_SPINBOX_RUN++;
    if (COUNT_SPINBOX_RUN > 1) {
        COUNT_SPINBOX_RUN = 0;
        return;
    }
    else {
        ui->XZdiag_Ycoordinate_Slider->setValue(ui->XZdiag_Ycoordinate_spin_lineEdit->text().toInt());
    }

    FLAG_VIEWER_UPDATE = 1;
}

void GeometryInputForm::XZshowgrid_checkBox_stateChanged()    // viewerXZのshow
                                                              // gridのチェックボックスのstatusが変わった時の処理
{
    if (ui->XZshowgrid_checkBox->isChecked()) {
        XZshowgrid_checkBox_isChecked();
    }
    else {
        on_XZRedraw_pushButton_clicked();
    }
}

void GeometryInputForm::YZshowgrid_checkBox_stateChanged()    // viewerYZのshow
                                                              // gridのチェックボックスのstatusが変わった時の処理
{
    if (ui->YZshowgrid_checkBox->isChecked()) {
        YZshowgrid_checkBox_isChecked();
    }
    else {
        on_YZRedraw_pushButton_clicked();
    }
}

void GeometryInputForm::XYshowgrid_checkBox_stateChanged()    // viewerXYのshow
                                                              // gridのチェックボックスのstatusが変わった時の処理
{
    if (ui->XYshowgrid_checkBox->isChecked()) {
        XYshowgrid_checkBox_isChecked();
    }
    else {
        on_XYRedraw_pushButton_clicked();
    }
}

void GeometryInputForm::XZdiag_showgrid_checkBox_stateChanged()    // viewerXZのshow
                                                                   // gridのチェックボックスのstatusが変わった時の処理
{
    if (ui->XZdiag_showgrid_checkBox->isChecked()) {
        XZdiag_showgrid_checkBox_isChecked();
    }
    else {
        on_XZdiag_Redraw_pushButton_clicked();
    }
}

void GeometryInputForm::XZshowgrid_checkBox_isChecked()    // viewerXZのshow gridのチェックボックスのチェック状態の処理
{
    int dX    = ui->domainX_Value->text().toInt();
    int dZ    = ui->domainZ_Value->text().toInt();
    int Mesh  = ui->meshsize_Value->text().toInt();
    int mesh  = Mesh;
    int Xdata = dX / Mesh;
    int Zdata = dZ / Mesh;

    QPen whitePen(QColor(255, 255, 255, 255));

    for (int x = 0; x < Xdata + 1; x++) {
        XZscene->addLine(x * mesh, 0, x * mesh, -Zdata * mesh, whitePen);
    }
    for (int z = 0; z < Zdata + 1; z++) {
        XZscene->addLine(0, -z * mesh, Xdata * mesh, -z * mesh, whitePen);
    }
}

void GeometryInputForm::YZshowgrid_checkBox_isChecked()    // viewerYZのshow gridのチェックボックスのチェック状態の処理
{
    int dY    = ui->domainY_Value->text().toInt();
    int dZ    = ui->domainZ_Value->text().toInt();
    int Mesh  = ui->meshsize_Value->text().toInt();
    int mesh  = Mesh;
    int Ydata = dY / Mesh;
    int Zdata = dZ / Mesh;

    QPen whitePen(QColor(255, 255, 255, 255));

    for (int y = 0; y < Ydata + 1; y++) {
        YZscene->addLine(y * mesh, 0, y * mesh, -Zdata * mesh, whitePen);
    }
    for (int z = 0; z < Zdata + 1; z++) {
        YZscene->addLine(0, -z * mesh, Ydata * mesh, -z * mesh, whitePen);
    }
}

void GeometryInputForm::XYshowgrid_checkBox_isChecked()    // viewerXYのshow gridのチェックボックスのチェック状態の処理
{
    int dX    = ui->domainX_Value->text().toInt();
    int dY    = ui->domainY_Value->text().toInt();
    int Mesh  = ui->meshsize_Value->text().toInt();
    int mesh  = Mesh;
    int Xdata = dX / Mesh;
    int Ydata = dY / Mesh;

    QPen whitePen(QColor(255, 255, 255, 255));

    for (int x = 0; x < Xdata + 1; x++) {
        XYscene->addLine(x * mesh, 0, x * mesh, -Ydata * mesh, whitePen);
    }
    for (int y = 0; y < Ydata + 1; y++) {
        XYscene->addLine(0, -y * mesh, Xdata * mesh, -y * mesh, whitePen);
    }
}

void GeometryInputForm::XZdiag_showgrid_checkBox_isChecked()    // viewerXZのshow
                                                                // gridのチェックボックスのチェック状態の処理
{
    qDebug() << "[DEBUG]01 XZdiag_showgrid_checkBox_isChecked";
    int dX    = ui->domainX_Value->text().toInt();
    int dZ    = ui->domainZ_Value->text().toInt();
    int Mesh  = ui->meshsize_Value->text().toInt();
    int mesh  = Mesh;
    int Xdata = dX / Mesh;
    int Zdata = dZ / Mesh;

    QPen whitePen(QColor(255, 255, 255, 255));

    // 横幅のみ、ルート2を掛ける. (ZはMeshサイズのまま）
    for (int x = 0; x < Xdata + 1; x++) {    // 縦線
        // XZdiag_scene->addLine(x*mesh, 0, x*mesh, -Zdata*mesh, whitePen);
        XZdiag_scene->addLine(x * mesh * sqrt(2), 0, x * mesh * sqrt(2), -Zdata * mesh, whitePen);
    }
    for (int z = 0; z < Zdata + 1; z++) {    // 横線
        // XZdiag_scene->addLine(0, -z*mesh, Xdata*mesh, -z*mesh, whitePen);
        XZdiag_scene->addLine(0, -z * mesh, Xdata * mesh * sqrt(2), -z * mesh, whitePen);
    }
}

void GeometryInputForm::on_XZzoomin_pushButton_clicked()    // viewerXZのzoomin処理
{
    ui->XZgraphicsView->scale(1.2, 1.2);
}

void GeometryInputForm::on_YZzoomin_pushButton_clicked()    // viewerYZのzoomin処理
{
    ui->YZgraphicsView->scale(1.2, 1.2);
}

void GeometryInputForm::on_XYzoomin_pushButton_clicked()    // viewerXYのzoomin処理
{
    ui->XYgraphicsView->scale(1.2, 1.2);
}

void GeometryInputForm::on_XZdiag_zoomin_pushButton_clicked()    // viewerXZのzoomin処理
{
    ui->XZdiag_graphicsView->scale(1.2, 1.2);
}

void GeometryInputForm::on_XZzoomout_pushButton_clicked()    // viewerXZのzoomout処理
{
    ui->XZgraphicsView->scale(0.8, 0.8);
}

void GeometryInputForm::on_YZzoomout_pushButton_clicked()    // viewerYZのzoomout処理
{
    ui->YZgraphicsView->scale(0.8, 0.8);
}

void GeometryInputForm::on_XYzoomout_pushButton_clicked()    // viewerXYのzoomout処理
{
    ui->XYgraphicsView->scale(0.8, 0.8);
}

void GeometryInputForm::on_XZdiag_zoomout_pushButton_clicked()    // viewerXZのzoomout処理
{
    ui->XZdiag_graphicsView->scale(0.8, 0.8);
}

void GeometryInputForm::on_XZzoomall_pushButton_clicked()    // viewerXZのzoomall処理
{
    int dX = ui->domainX_Value->text().toInt();
    int dZ = ui->domainZ_Value->text().toInt();
    XZscene->setSceneRect(0, 0, dX, -dZ);
    ui->XZgraphicsView->fitInView(XZscene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void GeometryInputForm::on_YZzoomall_pushButton_clicked()    // viewerYZのzoomall処理
{
    int dY = ui->domainY_Value->text().toInt();
    int dZ = ui->domainZ_Value->text().toInt();
    YZscene->setSceneRect(0, 0, dY, -dZ);
    ui->YZgraphicsView->fitInView(YZscene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void GeometryInputForm::on_XYzoomall_pushButton_clicked()    // viewerXYのzoomall処理
{
    int dX = ui->domainX_Value->text().toInt();
    int dY = ui->domainY_Value->text().toInt();
    XYscene->setSceneRect(0, 0, dX, -dY);
    ui->XYgraphicsView->fitInView(XYscene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void GeometryInputForm::on_XZdiag_zoomall_pushButton_clicked()    // viewerXZdiag_のzoomall処理
{
    int dX = ui->domainX_Value->text().toInt();
    int dZ = ui->domainZ_Value->text().toInt();
    XZdiag_scene->setSceneRect(0, 0, dX, -dZ);
    ui->XZdiag_graphicsView->fitInView(XZdiag_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void GeometryInputForm::on_XZlength_checkBox_clicked(bool checked)
{
    if (checked == false) {
        ui->XZ_comboBox_Length->hide();
        lengthXZ_X.clear();
        lengthXZ_Y.clear();
        on_XZRedraw_pushButton_clicked();
    }
    else if (checked == true) {
        ui->XZ_comboBox_Length->show();
    }
    ui->XZlength_checkBox->setText(
        "Length");    // 2020.03.09 zoomBox・Length同時使用(バインドキー限定） //LengthCheckBox ON・OFF時点で
                      // ラベルはLengthにリセットされる (LengthがzoomBoxより優先　バインドキーLength_onP1より優先)
    FLAG_LENGTH = 0;
}

void GeometryInputForm::on_YZlength_checkBox_clicked(bool checked)
{
    if (checked == false) {
        ui->YZ_comboBox_Length->hide();
        lengthYZ_X.clear();
        lengthYZ_Y.clear();
        on_YZRedraw_pushButton_clicked();
    }
    else if (checked == true) {
        ui->YZ_comboBox_Length->show();
    }
    ui->YZlength_checkBox->setText(
        "Length");    // 2020.03.09 zoomBox・Length同時使用(バインドキー限定） //LengthCheckBox ON・OFF時点で
                      // ラベルはLengthにリセットされる (LengthがzoomBoxより優先　バインドキーLength_onP1より優先)
    FLAG_LENGTH = 0;
}

void GeometryInputForm::on_XYlength_checkBox_clicked(bool checked)
{
    if (checked == false) {
        ui->XY_comboBox_Length->hide();
        lengthXY_X.clear();
        lengthXY_Y.clear();
        on_XYRedraw_pushButton_clicked();
    }
    else if (checked == true) {
        ui->XY_comboBox_Length->show();
    }
    ui->XYlength_checkBox->setText(
        "Length");    // 2020.03.09 zoomBox・Length同時使用(バインドキー限定） //LengthCheckBox ON・OFF時点で
                      // ラベルはLengthにリセットされる (LengthがzoomBoxより優先　バインドキーLength_onP1より優先)
    FLAG_LENGTH = 0;
}

void GeometryInputForm::on_XZdiag_length_checkBox_clicked(bool checked)
{
    if (checked == false) {
        ui->XZdiag_comboBox_Length->hide();
        lengthXZdiag_X.clear();
        lengthXZdiag_Y.clear();
        on_XZdiag_Redraw_pushButton_clicked();
    }
    else if (checked == true) {
        ui->XZdiag_comboBox_Length->show();
    }
    ui->XZdiag_length_checkBox->setText(
        "Length");    // 2020.03.09 zoomBox・Length同時使用(バインドキー限定） //LengthCheckBox ON・OFF時点で
                      // ラベルはLengthにリセットされる (LengthがzoomBoxより優先　バインドキーLength_onP1より優先)
    FLAG_LENGTH = 0;
}

void GeometryInputForm::
    func_on_Xcoordinate_spin_lineEdit_editingFinished()    // viewerXZのYpositionのEditingFinished時の処理。　(EditingFinished
                                                           // , valueChanged両立　と
                                                           // sliderの連携のため個別関数として用意しスロットとして使う）
{
    QString tmpID = "spin_lineEdit";
    func_on_Xcoordinate_spin_valueInput(tmpID);
}

void GeometryInputForm::func_on_Xcoordinate_spin_upButton_clicked()    // 2019.05.16
                                                                       // viewerXZのspinBox代替▲ボタン押下時の処理
{
    QString tmpID = "spin_upButton";
    func_on_Xcoordinate_spin_valueInput(tmpID);
}

void GeometryInputForm::func_on_Xcoordinate_spin_downButton_clicked()    // 2019.05.16
                                                                         // viewerXZのspinBox代替▼ボタン押下時の処理
{
    QString tmpID = "spin_downButton";
    func_on_Xcoordinate_spin_valueInput(tmpID);
}

void GeometryInputForm::func_on_Xcoordinate_spin_valueInput(
    QString input_text)    // viewerXZのEditingFinished時の処理。spinBox代替▲▼ボタン押下時の処理
{
    int spinValue     = ui->Xcoordinate_spin_lineEdit->text().toInt();
    int spinValue_new = spinValue;    // デフォルト
    if (input_text == "spin_lineEdit") {
        spinValue_new =
            ui->Xcoordinate_spin_lineEdit->text().toInt();    // viewerXZのYpositionのEditingFinished時の処理。
    }
    if (input_text == "spin_upButton") {
        spinValue_new = ui->Xcoordinate_spin_lineEdit->text().toInt()
                      + ui->meshsize_Value->text().toInt();    // viewerXZのspinBox代替▲ボタン押下時の処理
    }
    if (input_text == "spin_downButton") {
        spinValue_new = ui->Xcoordinate_spin_lineEdit->text().toInt()
                      - ui->meshsize_Value->text().toInt();    // viewerXZのspinBox代替▼ボタン押下時の処理
    }
    // 入力値調整
    //     →メッシュ単位の値に直す
    //     →スピンボックスの値：0より下にならないようにする
    //     →スピンボックスの値：Domain値より上にならないようにする
    if (spinValue_new % ui->meshsize_Value->text().toInt() > 0) {
        spinValue_new =
            spinValue_new - (spinValue_new % ui->meshsize_Value->text().toInt());    // メッシュ単位の値に直す
    }
    if (spinValue_new < 1) {
        spinValue_new = 1;    // スピンボックスの値：0より下にならないようにする
    }
    QString domainMax =
        ui->domainX_Value->text();    // 2021.06.xx-01 vox表示専用ビューとするため変更//evaluateStr("$DomainX$");
    int domainMaxValue = domainMax.toInt();
    if (spinValue_new > domainMaxValue) {
        spinValue_new = domainMaxValue;    // スピンボックスの値：Domain値より上にならないようにする
    }
    if (spinValue_new != spinValue) {
        ui->Xcoordinate_spin_lineEdit->setText(QString::number(spinValue_new));
    }
    // スライダー更新する
    update_Xcoordinate_Slider();
}

void GeometryInputForm::
    func_on_Ycoordinate_spin_lineEdit_editingFinished()    // viewerXZのYpositionのEditingFinished時の処理。　(EditingFinished
                                                           // , valueChanged両立　と
                                                           // sliderの連携のため個別関数として用意しスロットとして使う）
{
    QString tmpID = "spin_lineEdit";
    func_on_Ycoordinate_spin_valueInput(tmpID);
}

void GeometryInputForm::func_on_Ycoordinate_spin_upButton_clicked()    // 2019.05.16
                                                                       // viewerXZのspinBox代替▲ボタン押下時の処理
{
    QString tmpID = "spin_upButton";
    func_on_Ycoordinate_spin_valueInput(tmpID);
}

void GeometryInputForm::func_on_Ycoordinate_spin_downButton_clicked()    // 2019.05.16
                                                                         // viewerXZのspinBox代替▼ボタン押下時の処理
{
    QString tmpID = "spin_downButton";
    func_on_Ycoordinate_spin_valueInput(tmpID);
}

void GeometryInputForm::func_on_Ycoordinate_spin_valueInput(
    QString input_text)    // viewerXZのEditingFinished時の処理。spinBox代替▲▼ボタン押下時の処理
{
    int spinValue     = ui->Ycoordinate_spin_lineEdit->text().toInt();
    int spinValue_new = spinValue;    // デフォルト
    if (input_text == "spin_lineEdit") {
        spinValue_new =
            ui->Ycoordinate_spin_lineEdit->text().toInt();    // viewerXZのYpositionのEditingFinished時の処理。
    }
    if (input_text == "spin_upButton") {
        spinValue_new = ui->Ycoordinate_spin_lineEdit->text().toInt()
                      + ui->meshsize_Value->text().toInt();    // viewerXZのspinBox代替▲ボタン押下時の処理
    }
    if (input_text == "spin_downButton") {
        spinValue_new = ui->Ycoordinate_spin_lineEdit->text().toInt()
                      - ui->meshsize_Value->text().toInt();    // viewerXZのspinBox代替▼ボタン押下時の処理
    }
    // 入力値調整
    //     →メッシュ単位の値に直す
    //     →スピンボックスの値：0より下にならないようにする
    //     →スピンボックスの値：Domain値より上にならないようにする
    if (spinValue_new % ui->meshsize_Value->text().toInt() > 0) {
        spinValue_new =
            spinValue_new - (spinValue_new % ui->meshsize_Value->text().toInt());    // メッシュ単位の値に直す
    }
    if (spinValue_new < 1) {
        spinValue_new = 1;    // スピンボックスの値：0より下にならないようにする
    }
    QString domainMax =
        ui->domainY_Value->text();    // 2021.06.xx-01 vox専用ビューとするため変更//evaluateStr("$DomainY$");
    int domainMaxValue = domainMax.toInt();
    if (spinValue_new > domainMaxValue) {
        spinValue_new = domainMaxValue;    // スピンボックスの値：Domain値より上にならないようにする
    }
    if (spinValue_new != spinValue) {
        ui->Ycoordinate_spin_lineEdit->setText(QString::number(spinValue_new));
    }

    // スライダー更新する
    update_Ycoordinate_Slider();
}

void GeometryInputForm::
    func_on_Zcoordinate_spin_lineEdit_editingFinished()    // viewerXZのYpositionのEditingFinished時の処理。　(EditingFinished
                                                           // , valueChanged両立　と
                                                           // sliderの連携のため個別関数として用意しスロットとして使う）
{
    QString tmpID = "spin_lineEdit";
    func_on_Zcoordinate_spin_valueInput(tmpID);
}

void GeometryInputForm::func_on_Zcoordinate_spin_upButton_clicked()    // 2019.05.16
                                                                       // viewerXZのspinBox代替▲ボタン押下時の処理
{
    QString tmpID = "spin_upButton";
    func_on_Zcoordinate_spin_valueInput(tmpID);
}

void GeometryInputForm::func_on_Zcoordinate_spin_downButton_clicked()    // 2019.05.16
                                                                         // viewerXZのspinBox代替▼ボタン押下時の処理
{
    QString tmpID = "spin_downButton";
    func_on_Zcoordinate_spin_valueInput(tmpID);
}

void GeometryInputForm::func_on_Zcoordinate_spin_valueInput(
    QString input_text)    // viewerXZのYpositionのEditingFinished時の処理。pinBox代替▲▼ボタン押下時の処理
{
    int spinValue     = ui->Zcoordinate_spin_lineEdit->text().toInt();
    int spinValue_new = spinValue;    // デフォルト
    if (input_text == "spin_lineEdit") {
        spinValue_new =
            ui->Zcoordinate_spin_lineEdit->text().toInt();    // viewerXZのYpositionのEditingFinished時の処理。
    }
    if (input_text == "spin_upButton") {
        spinValue_new = ui->Zcoordinate_spin_lineEdit->text().toInt()
                      + ui->meshsize_Value->text().toInt();    // viewerXZのspinBox代替▲ボタン押下時の処理
    }
    if (input_text == "spin_downButton") {
        spinValue_new = ui->Zcoordinate_spin_lineEdit->text().toInt()
                      - ui->meshsize_Value->text().toInt();    // viewerXZのspinBox代替▼ボタン押下時の処理
    }
    // 入力値調整
    //     →メッシュ単位の値に直す
    //     →スピンボックスの値：0より下にならないようにする
    //     →スピンボックスの値：Domain値より上にならないようにする
    if (spinValue_new % ui->meshsize_Value->text().toInt() > 0) {
        spinValue_new =
            spinValue_new - (spinValue_new % ui->meshsize_Value->text().toInt());    // メッシュ単位の値に直す
    }
    if (spinValue_new < 1) {
        spinValue_new = 1;    // スピンボックスの値：0より下にならないようにする
    }
    QString domainMax =
        ui->domainZ_Value->text();    // 2021.06.xx-01 voxビューアーにするため変更 //evaluateStr("$DomainZ$");
    int domainMaxValue = domainMax.toInt();
    if (spinValue_new > domainMaxValue) {
        spinValue_new = domainMaxValue;    // スピンボックスの値：Domain値より上にならないようにする
    }
    if (spinValue_new != spinValue) {
        ui->Zcoordinate_spin_lineEdit->setText(QString::number(spinValue_new));
    }

    // スライダー更新する
    update_Zcoordinate_Slider();
}

void GeometryInputForm::on_XZdiag_Ycoordinate_Slider_valueChanged(int value)
{
    //-start- XZ 斜め断面表示
    // int maxvalue = ui->domainX_Value->text().toInt() ;
    // if(maxvalue < ui->domainY_Value->text().toInt()) {
    //    maxvalue = ui->domainY_Value->text().toInt();
    //}
    ui->XZdiag_Ycoordinate_spin_lineEdit->setText(QString::number(value));
    // func_vox_make_diagonal("XZ");
    //-end- XZ 斜め断面表示
}

void GeometryInputForm::on_XZdiag_Ycoordinate_spin_lineEdit_editingFinished()
{
    // XZ 斜め断面表示
    //    FLAG_VIEWER_UPDATE = 1;
    //    viewer_update();

    //[DEBUG]XZdiag_scene->clear();
    //[DEBUG]func_vox_make_diagonal("XZ");

    QString tmpID = "spin_lineEdit";
    func_on_XZdiag_Ycoordinate_spin_valueInput(tmpID);
}

void GeometryInputForm::on_XZdiag_Ycoordinate_spin_upButton_clicked()    // viewerXZのspinBox代替▲ボタン押下時の処理
{
    QString tmpID = "spin_upButton";
    func_on_XZdiag_Ycoordinate_spin_valueInput(tmpID);
}

void GeometryInputForm::on_XZdiag_Ycoordinate_spin_downButton_clicked()    // viewerXZのspinBox代替▼ボタン押下時の処理
{
    QString tmpID = "spin_downButton";
    func_on_XZdiag_Ycoordinate_spin_valueInput(tmpID);
}

void GeometryInputForm::func_on_XZdiag_Ycoordinate_spin_valueInput(
    QString input_text)    // viewerXZのEditingFinished時の処理。spinBox代替▲▼ボタン押下時の処理
{
    int spinValue     = ui->XZdiag_Ycoordinate_spin_lineEdit->text().toInt();
    int spinValue_new = spinValue;    // デフォルト
    if (input_text == "spin_lineEdit") {
        spinValue_new =
            ui->XZdiag_Ycoordinate_spin_lineEdit->text().toInt();    // viewerXZのYpositionのEditingFinished時の処理。
    }
    if (input_text == "spin_upButton") {
        spinValue_new = ui->XZdiag_Ycoordinate_spin_lineEdit->text().toInt()
                      + ui->meshsize_Value->text().toInt();    // viewerXZのspinBox代替▲ボタン押下時の処理
    }
    if (input_text == "spin_downButton") {
        spinValue_new = ui->XZdiag_Ycoordinate_spin_lineEdit->text().toInt()
                      - ui->meshsize_Value->text().toInt();    // viewerXZのspinBox代替▼ボタン押下時の処理
    }
    // 入力値調整
    //     →メッシュ単位の値に直す
    //     →XZdiagではなし→スピンボックスの値：0より下にならないようにする
    //     →スピンボックスの値：Domain値より上にならないようにする
    if (spinValue_new % ui->meshsize_Value->text().toInt() > 0) {
        spinValue_new =
            spinValue_new - (spinValue_new % ui->meshsize_Value->text().toInt());    // メッシュ単位の値に直す
    }
    // XZdiagでは スピンボックスの取りうる範囲は -domainMax ～ + domainMax。　マイナスの場合もあり。
    // if(spinValue_new < 1 ){
    //     spinValue_new = 1;  //スピンボックスの値：0より下にならないようにする
    // }

    int domainMaxValue = ui->domainX_Value->text().toInt();
    // int domainMaxValue = ui->domainY_Value->text().toInt(); //2021.06.xx-01
    // vox専用ビューとするため変更//evaluateStr("$DomainY$"); if(domainMaxValue < ui->domainX_Value->text().toInt()) {
    // domainMaxValue = ui->domainX_Value->text().toInt(); }
    if (spinValue_new > domainMaxValue) {
        spinValue_new = domainMaxValue;    // スピンボックスの値：Domain値より上にならないようにする
    }
    if (spinValue_new != spinValue) {
        ui->XZdiag_Ycoordinate_spin_lineEdit->setText(QString::number(spinValue_new));
    }

    // スライダー更新する
    update_XZdiag_Ycoordinate_Slider();
}

void GeometryInputForm::func_view_ruler()
{
    QBrush rulerBrush[3];
    QPen   rulerPen[3];
    QColor rulerTextcolor[3];

    // 色が決まったら入れる
    rulerBrush[0]     = QColor(51, 51, 51, 255);
    rulerBrush[1]     = QColor(170, 170, 170, 255);
    rulerBrush[2]     = QColor(245, 245, 220, 255);
    rulerPen[0]       = QColor(51, 51, 51, 255);
    rulerPen[1]       = QColor(170, 170, 170, 255);
    rulerPen[2]       = QColor(245, 245, 220, 255);
    rulerTextcolor[0] = QColor(51, 51, 51, 255);
    rulerTextcolor[1] = QColor(170, 170, 170, 255);
    rulerTextcolor[2] = QColor(245, 245, 220, 255);

    int dX = ui->domainX_Value->text().toInt();
    int dY = ui->domainY_Value->text().toInt();
    int dZ = ui->domainZ_Value->text().toInt();

    QGraphicsEllipseItem* selectPoint;
    QGraphicsTextItem*    rulerText;

    // XZdplane用
    if (ui->tabWidget->currentIndex() == 0 && ui->XZruler_checkBox->isChecked() && rulerXZ_X != 9999
        && rulerXZ_Y != 9999) {
        int colnum = ui->XZruler_color_comboBox->currentIndex();
        rulerPen[colnum].setWidth(dZ / 2000);
        // 上
        XZscene->addLine(rulerXZ_X, -rulerXZ_Y, rulerXZ_X, -dZ, rulerPen[colnum]);
        for (int n = 1; n <= (dZ - rulerXZ_Y) / 100; n++) {
            if (n % 10 == 0) {
                XZscene->addLine(rulerXZ_X - 60, -(rulerXZ_Y + n * 100), rulerXZ_X + 60, -(rulerXZ_Y + n * 100),
                                 rulerPen[colnum]);
                rulerText = XZscene->addText(QString::number(n * 100));
                rulerText->setFont(QFont("Times", 100));
                rulerText->setDefaultTextColor(rulerTextcolor[colnum]);
                rulerText->setPos(rulerXZ_X + 60 + 40, -(rulerXZ_Y + n * 100 + 70));
            }
            else if (n % 5 == 0) {
                XZscene->addLine(rulerXZ_X - 40, -(rulerXZ_Y + n * 100), rulerXZ_X + 40, -(rulerXZ_Y + n * 100),
                                 rulerPen[colnum]);
            }
            else {
                XZscene->addLine(rulerXZ_X - 20, -(rulerXZ_Y + n * 100), rulerXZ_X + 20, -(rulerXZ_Y + n * 100),
                                 rulerPen[colnum]);
            }
        }

        // 下
        XZscene->addLine(rulerXZ_X, -rulerXZ_Y, rulerXZ_X, 0, rulerPen[colnum]);
        for (int n = 1; n <= rulerXZ_Y / 100; n++) {
            if (n % 10 == 0) {
                XZscene->addLine(rulerXZ_X - 60, -(rulerXZ_Y - n * 100), rulerXZ_X + 60, -(rulerXZ_Y - n * 100),
                                 rulerPen[colnum]);
                rulerText = XZscene->addText(QString::number(-(n * 100)));
                rulerText->setFont(QFont("Times", 100));
                rulerText->setDefaultTextColor(rulerTextcolor[colnum]);
                rulerText->setPos(rulerXZ_X + 60 + 40, -(rulerXZ_Y - n * 100 + 70));
            }
            else if (n % 5 == 0) {
                XZscene->addLine(rulerXZ_X - 40, -(rulerXZ_Y - n * 100), rulerXZ_X + 40, -(rulerXZ_Y - n * 100),
                                 rulerPen[colnum]);
            }
            else {
                XZscene->addLine(rulerXZ_X - 20, -(rulerXZ_Y - n * 100), rulerXZ_X + 20, -(rulerXZ_Y - n * 100),
                                 rulerPen[colnum]);
            }
        }

        // 左
        XZscene->addLine(rulerXZ_X, -rulerXZ_Y, 0, -rulerXZ_Y, rulerPen[colnum]);
        for (int n = 1; n <= rulerXZ_X / 100; n++) {
            if (n % 10 == 0) {
                XZscene->addLine(rulerXZ_X - (100 * n), -rulerXZ_Y - 60, rulerXZ_X - (100 * n), -rulerXZ_Y + 60,
                                 rulerPen[colnum]);
                rulerText = XZscene->addText(QString::number(-(n * 100)));
                rulerText->setFont(QFont("Times", 100));
                rulerText->setDefaultTextColor(rulerTextcolor[colnum]);
                rulerText->setPos(rulerXZ_X - (100 * n) - 200, -rulerXZ_Y + 70);
            }
            else if (n % 5 == 0) {
                XZscene->addLine(rulerXZ_X - (100 * n), -rulerXZ_Y - 40, rulerXZ_X - (100 * n), -rulerXZ_Y + 40,
                                 rulerPen[colnum]);
            }
            else {
                XZscene->addLine(rulerXZ_X - (100 * n), -rulerXZ_Y - 20, rulerXZ_X - (100 * n), -rulerXZ_Y + 20,
                                 rulerPen[colnum]);
            }
        }

        // 右
        XZscene->addLine(rulerXZ_X, -rulerXZ_Y, dX, -rulerXZ_Y, rulerPen[colnum]);
        for (int n = 1; n <= (dX - rulerXZ_X) / 100; n++) {
            if (n % 10 == 0) {
                XZscene->addLine(rulerXZ_X + (100 * n), -rulerXZ_Y - 60, rulerXZ_X + (100 * n), -rulerXZ_Y + 60,
                                 rulerPen[colnum]);
                rulerText = XZscene->addText(QString::number(n * 100));
                rulerText->setFont(QFont("Times", 100));
                rulerText->setDefaultTextColor(rulerTextcolor[colnum]);
                rulerText->setPos(rulerXZ_X + (100 * n) - 130, -rulerXZ_Y + 70);
            }
            else if (n % 5 == 0) {
                XZscene->addLine(rulerXZ_X + (100 * n), -rulerXZ_Y - 40, rulerXZ_X + (100 * n), -rulerXZ_Y + 40,
                                 rulerPen[colnum]);
            }
            else {
                XZscene->addLine(rulerXZ_X + (100 * n), -rulerXZ_Y - 20, rulerXZ_X + (100 * n), -rulerXZ_Y + 20,
                                 rulerPen[colnum]);
            }
        }

        // 選択した部分
        selectPoint =
            XZscene->addEllipse(rulerXZ_X - 20, -rulerXZ_Y - 20, 40, 40, rulerPen[colnum], rulerBrush[colnum]);
        selectPoint->setBrush(rulerBrush[colnum]);
    }

    // YZplane
    if (ui->tabWidget->currentIndex() == 1 && ui->YZruler_checkBox->isChecked() && rulerYZ_X != 9999
        && rulerYZ_Y != 9999) {
        int colnum = ui->YZruler_color_comboBox->currentIndex();
        rulerPen[colnum].setWidth(dZ / 2000);
        // 上
        YZscene->addLine(rulerYZ_X, -rulerYZ_Y, rulerYZ_X, -dZ, rulerPen[colnum]);
        for (int n = 1; n <= (dZ - rulerYZ_Y) / 100; n++) {
            if (n % 10 == 0) {
                YZscene->addLine(rulerYZ_X - 60, -(rulerYZ_Y + n * 100), rulerYZ_X + 60, -(rulerYZ_Y + n * 100),
                                 rulerPen[colnum]);
                rulerText = YZscene->addText(QString::number(n * 100));
                rulerText->setFont(QFont("Times", 100));
                rulerText->setDefaultTextColor(rulerTextcolor[colnum]);
                rulerText->setPos(rulerYZ_X + 60 + 40, -(rulerYZ_Y + n * 100 + 70));
            }
            else if (n % 5 == 0) {
                YZscene->addLine(rulerYZ_X - 40, -(rulerYZ_Y + n * 100), rulerYZ_X + 40, -(rulerYZ_Y + n * 100),
                                 rulerPen[colnum]);
            }
            else {
                YZscene->addLine(rulerYZ_X - 20, -(rulerYZ_Y + n * 100), rulerYZ_X + 20, -(rulerYZ_Y + n * 100),
                                 rulerPen[colnum]);
            }
        }

        // 下
        YZscene->addLine(rulerYZ_X, -rulerYZ_Y, rulerYZ_X, 0, rulerPen[colnum]);
        for (int n = 1; n <= rulerYZ_Y / 100; n++) {
            if (n % 10 == 0) {
                YZscene->addLine(rulerYZ_X - 60, -(rulerYZ_Y - n * 100), rulerYZ_X + 60, -(rulerYZ_Y - n * 100),
                                 rulerPen[colnum]);
                rulerText = YZscene->addText(QString::number(-(n * 100)));
                rulerText->setFont(QFont("Times", 100));
                rulerText->setDefaultTextColor(rulerTextcolor[colnum]);
                rulerText->setPos(rulerYZ_X + 60 + 40, -(rulerYZ_Y - n * 100 + 70));
            }
            else if (n % 5 == 0) {
                YZscene->addLine(rulerYZ_X - 40, -(rulerYZ_Y - n * 100), rulerYZ_X + 40, -(rulerYZ_Y - n * 100),
                                 rulerPen[colnum]);
            }
            else {
                YZscene->addLine(rulerYZ_X - 20, -(rulerYZ_Y - n * 100), rulerYZ_X + 20, -(rulerYZ_Y - n * 100),
                                 rulerPen[colnum]);
            }
        }

        // 左
        YZscene->addLine(rulerYZ_X, -rulerYZ_Y, 0, -rulerYZ_Y, rulerPen[colnum]);
        for (int n = 1; n <= rulerYZ_X / 100; n++) {
            if (n % 10 == 0) {
                YZscene->addLine(rulerYZ_X - (100 * n), -rulerYZ_Y - 60, rulerYZ_X - (100 * n), -rulerYZ_Y + 60,
                                 rulerPen[colnum]);
                rulerText = YZscene->addText(QString::number(-(n * 100)));
                rulerText->setFont(QFont("Times", 100));
                rulerText->setDefaultTextColor(rulerTextcolor[colnum]);
                rulerText->setPos(rulerYZ_X - (100 * n) - 200, -rulerYZ_Y + 70);
            }
            else if (n % 5 == 0) {
                YZscene->addLine(rulerYZ_X - (100 * n), -rulerYZ_Y - 40, rulerYZ_X - (100 * n), -rulerYZ_Y + 40,
                                 rulerPen[colnum]);
            }
            else {
                YZscene->addLine(rulerYZ_X - (100 * n), -rulerYZ_Y - 20, rulerYZ_X - (100 * n), -rulerYZ_Y + 20,
                                 rulerPen[colnum]);
            }
        }

        // 右
        YZscene->addLine(rulerYZ_X, -rulerYZ_Y, dY, -rulerYZ_Y, rulerPen[colnum]);
        for (int n = 1; n <= (dY - rulerYZ_X) / 100; n++) {
            if (n % 10 == 0) {
                YZscene->addLine(rulerYZ_X + (100 * n), -rulerYZ_Y - 60, rulerYZ_X + (100 * n), -rulerYZ_Y + 60,
                                 rulerPen[colnum]);
                rulerText = YZscene->addText(QString::number(n * 100));
                rulerText->setFont(QFont("Times", 100));
                rulerText->setDefaultTextColor(rulerTextcolor[colnum]);
                rulerText->setPos(rulerYZ_X + (100 * n) - 130, -rulerYZ_Y + 70);
            }
            else if (n % 5 == 0) {
                YZscene->addLine(rulerYZ_X + (100 * n), -rulerYZ_Y - 40, rulerYZ_X + (100 * n), -rulerYZ_Y + 40,
                                 rulerPen[colnum]);
            }
            else {
                YZscene->addLine(rulerYZ_X + (100 * n), -rulerYZ_Y - 20, rulerYZ_X + (100 * n), -rulerYZ_Y + 20,
                                 rulerPen[colnum]);
            }
        }

        // 選択した部分
        selectPoint =
            YZscene->addEllipse(rulerYZ_X - 20, -rulerYZ_Y - 20, 40, 40, rulerPen[colnum], rulerBrush[colnum]);
        selectPoint->setBrush(rulerBrush[colnum]);
    }

    // XYplane
    if (ui->tabWidget->currentIndex() == 2 && ui->XYruler_checkBox->isChecked() && rulerXY_X != 9999
        && rulerXY_Y != 9999) {
        int colnum = ui->XYruler_color_comboBox->currentIndex();
        rulerPen[colnum].setWidth(dY / 2000);
        // 上
        XYscene->addLine(rulerXY_X, -rulerXY_Y, rulerXY_X, -dY, rulerPen[colnum]);
        for (int n = 1; n <= (dY - rulerXY_Y) / 100; n++) {
            if (n % 10 == 0) {
                XYscene->addLine(rulerXY_X - 60, -(rulerXY_Y + n * 100), rulerXY_X + 60, -(rulerXY_Y + n * 100),
                                 rulerPen[colnum]);
                rulerText = XYscene->addText(QString::number(n * 100));
                rulerText->setFont(QFont("Times", 100));
                rulerText->setDefaultTextColor(rulerTextcolor[colnum]);
                rulerText->setPos(rulerXY_X + 60 + 40, -(rulerXY_Y + n * 100 + 70));
            }
            else if (n % 5 == 0) {
                XYscene->addLine(rulerXY_X - 40, -(rulerXY_Y + n * 100), rulerXY_X + 40, -(rulerXY_Y + n * 100),
                                 rulerPen[colnum]);
            }
            else {
                XYscene->addLine(rulerXY_X - 20, -(rulerXY_Y + n * 100), rulerXY_X + 20, -(rulerXY_Y + n * 100),
                                 rulerPen[colnum]);
            }
        }

        // 下
        XYscene->addLine(rulerXY_X, -rulerXY_Y, rulerXY_X, 0, rulerPen[colnum]);
        for (int n = 1; n <= rulerXY_Y / 100; n++) {
            if (n % 10 == 0) {
                XYscene->addLine(rulerXY_X - 60, -(rulerXY_Y - n * 100), rulerXY_X + 60, -(rulerXY_Y - n * 100),
                                 rulerPen[colnum]);
                rulerText = XYscene->addText(QString::number(-(n * 100)));
                rulerText->setFont(QFont("Times", 100));
                rulerText->setDefaultTextColor(rulerTextcolor[colnum]);
                rulerText->setPos(rulerXY_X + 60 + 40, -(rulerXY_Y - n * 100 + 70));
            }
            else if (n % 5 == 0) {
                XYscene->addLine(rulerXY_X - 40, -(rulerXY_Y - n * 100), rulerXY_X + 40, -(rulerXY_Y - n * 100),
                                 rulerPen[colnum]);
            }
            else {
                XYscene->addLine(rulerXY_X - 20, -(rulerXY_Y - n * 100), rulerXY_X + 20, -(rulerXY_Y - n * 100),
                                 rulerPen[colnum]);
            }
        }

        // 左
        XYscene->addLine(rulerXY_X, -rulerXY_Y, 0, -rulerXY_Y, rulerPen[colnum]);
        for (int n = 1; n <= rulerXY_X / 100; n++) {
            if (n % 10 == 0) {
                XYscene->addLine(rulerXY_X - (100 * n), -rulerXY_Y - 60, rulerXY_X - (100 * n), -rulerXY_Y + 60,
                                 rulerPen[colnum]);
                rulerText = XYscene->addText(QString::number(-(n * 100)));
                rulerText->setFont(QFont("Times", 100));
                rulerText->setDefaultTextColor(rulerTextcolor[colnum]);
                rulerText->setPos(rulerXY_X - (100 * n) - 200, -rulerXY_Y + 70);
            }
            else if (n % 5 == 0) {
                XYscene->addLine(rulerXY_X - (100 * n), -rulerXY_Y - 40, rulerXY_X - (100 * n), -rulerXY_Y + 40,
                                 rulerPen[colnum]);
            }
            else {
                XYscene->addLine(rulerXY_X - (100 * n), -rulerXY_Y - 20, rulerXY_X - (100 * n), -rulerXY_Y + 20,
                                 rulerPen[colnum]);
            }
        }

        // 右
        XYscene->addLine(rulerXY_X, -rulerXY_Y, dX, -rulerXY_Y, rulerPen[colnum]);
        for (int n = 1; n <= (dX - rulerXY_X) / 100; n++) {
            if (n % 10 == 0) {
                XYscene->addLine(rulerXY_X + (100 * n), -rulerXY_Y - 60, rulerXY_X + (100 * n), -rulerXY_Y + 60,
                                 rulerPen[colnum]);
                rulerText = XYscene->addText(QString::number(n * 100));
                rulerText->setFont(QFont("Times", 100));
                rulerText->setDefaultTextColor(rulerTextcolor[colnum]);
                rulerText->setPos(rulerXY_X + (100 * n) - 130, -rulerXY_Y + 70);
            }
            else if (n % 5 == 0) {
                XYscene->addLine(rulerXY_X + (100 * n), -rulerXY_Y - 40, rulerXY_X + (100 * n), -rulerXY_Y + 40,
                                 rulerPen[colnum]);
            }
            else {
                XYscene->addLine(rulerXY_X + (100 * n), -rulerXY_Y - 20, rulerXY_X + (100 * n), -rulerXY_Y + 20,
                                 rulerPen[colnum]);
            }
        }

        // 選択した部分
        selectPoint =
            XYscene->addEllipse(rulerXY_X - 20, -rulerXY_Y - 20, 40, 40, rulerPen[colnum], rulerBrush[colnum]);
        selectPoint->setBrush(rulerBrush[colnum]);
    }

    // XZdiag_plane用 //2021.04.xx-01 vox-2Dviwer 斜め断面
    if (ui->tabWidget->currentIndex() == 3 && ui->XZdiag_ruler_checkBox->isChecked() && rulerXZdiag_X != 9999
        && rulerXZdiag_Y != 9999) {
        int colnum = ui->XZdiag_ruler_color_comboBox->currentIndex();
        rulerPen[colnum].setWidth(dZ / 2000);
        // 上
        XZdiag_scene->addLine(rulerXZdiag_X, -rulerXZdiag_Y, rulerXZdiag_X, -dZ, rulerPen[colnum]);
        for (int n = 1; n <= (dZ - rulerXZdiag_Y) / 100; n++) {
            if (n % 10 == 0) {
                XZdiag_scene->addLine(rulerXZdiag_X - 60, -(rulerXZdiag_Y + n * 100), rulerXZdiag_X + 60,
                                      -(rulerXZdiag_Y + n * 100), rulerPen[colnum]);
                rulerText = XZdiag_scene->addText(QString::number(n * 100));
                rulerText->setFont(QFont("Times", 100));
                rulerText->setDefaultTextColor(rulerTextcolor[colnum]);
                rulerText->setPos(rulerXZdiag_X + 60 + 40, -(rulerXZdiag_Y + n * 100 + 70));
            }
            else if (n % 5 == 0) {
                XZdiag_scene->addLine(rulerXZdiag_X - 40, -(rulerXZdiag_Y + n * 100), rulerXZdiag_X + 40,
                                      -(rulerXZdiag_Y + n * 100), rulerPen[colnum]);
            }
            else {
                XZdiag_scene->addLine(rulerXZdiag_X - 20, -(rulerXZdiag_Y + n * 100), rulerXZdiag_X + 20,
                                      -(rulerXZdiag_Y + n * 100), rulerPen[colnum]);
            }
        }

        // 下
        XZdiag_scene->addLine(rulerXZdiag_X, -rulerXZdiag_Y, rulerXZdiag_X, 0, rulerPen[colnum]);
        for (int n = 1; n <= rulerXZdiag_Y / 100; n++) {
            if (n % 10 == 0) {
                XZdiag_scene->addLine(rulerXZdiag_X - 60, -(rulerXZdiag_Y - n * 100), rulerXZdiag_X + 60,
                                      -(rulerXZdiag_Y - n * 100), rulerPen[colnum]);
                rulerText = XZdiag_scene->addText(QString::number(-(n * 100)));
                rulerText->setFont(QFont("Times", 100));
                rulerText->setDefaultTextColor(rulerTextcolor[colnum]);
                rulerText->setPos(rulerXZdiag_X + 60 + 40, -(rulerXZdiag_Y - n * 100 + 70));
            }
            else if (n % 5 == 0) {
                XZdiag_scene->addLine(rulerXZdiag_X - 40, -(rulerXZdiag_Y - n * 100), rulerXZdiag_X + 40,
                                      -(rulerXZdiag_Y - n * 100), rulerPen[colnum]);
            }
            else {
                XZdiag_scene->addLine(rulerXZdiag_X - 20, -(rulerXZdiag_Y - n * 100), rulerXZdiag_X + 20,
                                      -(rulerXZdiag_Y - n * 100), rulerPen[colnum]);
            }
        }

        // 左
        XZdiag_scene->addLine(rulerXZdiag_X, -rulerXZdiag_Y, 0, -rulerXZdiag_Y, rulerPen[colnum]);
        for (int n = 1; n <= rulerXZdiag_X / 100; n++) {
            if (n % 10 == 0) {
                XZdiag_scene->addLine(rulerXZdiag_X - (100 * n), -rulerXZdiag_Y - 60, rulerXZdiag_X - (100 * n),
                                      -rulerXZdiag_Y + 60, rulerPen[colnum]);
                rulerText = XZdiag_scene->addText(QString::number(-(n * 100)));
                rulerText->setFont(QFont("Times", 100));
                rulerText->setDefaultTextColor(rulerTextcolor[colnum]);
                rulerText->setPos(rulerXZdiag_X - (100 * n) - 200, -rulerXZdiag_Y + 70);
            }
            else if (n % 5 == 0) {
                XZdiag_scene->addLine(rulerXZdiag_X - (100 * n), -rulerXZdiag_Y - 40, rulerXZdiag_X - (100 * n),
                                      -rulerXZdiag_Y + 40, rulerPen[colnum]);
            }
            else {
                XZdiag_scene->addLine(rulerXZdiag_X - (100 * n), -rulerXZdiag_Y - 20, rulerXZdiag_X - (100 * n),
                                      -rulerXZdiag_Y + 20, rulerPen[colnum]);
            }
        }

        // 右
        XZdiag_scene->addLine(rulerXZdiag_X, -rulerXZdiag_Y, dX, -rulerXZdiag_Y, rulerPen[colnum]);
        for (int n = 1; n <= (dX - rulerXZdiag_X) / 100; n++) {
            if (n % 10 == 0) {
                XZdiag_scene->addLine(rulerXZdiag_X + (100 * n), -rulerXZdiag_Y - 60, rulerXZdiag_X + (100 * n),
                                      -rulerXZdiag_Y + 60, rulerPen[colnum]);
                rulerText = XZdiag_scene->addText(QString::number(n * 100));
                rulerText->setFont(QFont("Times", 100));
                rulerText->setDefaultTextColor(rulerTextcolor[colnum]);
                rulerText->setPos(rulerXZdiag_X + (100 * n) - 130, -rulerXZdiag_Y + 70);
            }
            else if (n % 5 == 0) {
                XZdiag_scene->addLine(rulerXZdiag_X + (100 * n), -rulerXZdiag_Y - 40, rulerXZdiag_X + (100 * n),
                                      -rulerXZdiag_Y + 40, rulerPen[colnum]);
            }
            else {
                XZdiag_scene->addLine(rulerXZdiag_X + (100 * n), -rulerXZdiag_Y - 20, rulerXZdiag_X + (100 * n),
                                      -rulerXZdiag_Y + 20, rulerPen[colnum]);
            }
        }

        // 選択した部分
        selectPoint = XZdiag_scene->addEllipse(rulerXZdiag_X - 20, -rulerXZdiag_Y - 20, 40, 40, rulerPen[colnum],
                                               rulerBrush[colnum]);
        selectPoint->setBrush(rulerBrush[colnum]);
    }
}

void GeometryInputForm::func_view_Length(int x_meshAjust, int y_meshAjust)    // Length測長処理 XZ・YZ・XYビューの場合
{
    QGraphicsEllipseItem* Lengthpoint;
    QBrush                LengthBrush;
    QPen                  LengthPen;
    LengthBrush.setColor(QColor(255, 0, 0, 255));
    LengthPen.setColor(QColor(0, 0, 0, 255));

    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect(this);
    effect->setBlurRadius(3);
    effect->setOffset(0);
    effect->setColor(Qt::black);

    QGraphicsLineItem* Lengthline;

    QPen LengthlinePen;
    LengthlinePen.setColor(QColor(230, 230, 230, 255));
    LengthlinePen.setWidth(10);

    QGraphicsTextItem* lengthText;

    QString tmpStr;

    if (FLAG_LENGTH != 1) {
        // １点目クリック時点
        FLAG_LENGTH = 1;
        X1_CLICK    = x_meshAjust;
        Y1_CLICK    = y_meshAjust;

        //        if(ui->tabWidget->currentIndex() == 0  && ui->XZlength_checkBox->isChecked()
        //        || ui->tabWidget->currentIndex() == 0  && ui->XZlength_checkBox->text() == "Length_on"){ //2020.03.09
        //        ショートカットキーによる"Length_on"判定追加 //if(ui->tabWidget->currentIndex() == 0  &&
        //        ui->XZ_comboBox->currentText() == "Length"){
        //            on_XZRedraw_pushButton_clicked();
        if (ui->tabWidget->currentIndex() == 0) {
            Lengthpoint = XZscene->addEllipse(X1_CLICK - 15, -Y1_CLICK - 15, 30, 30, LengthPen, LengthBrush);
            Lengthpoint->setBrush(QColor(255, 255, 0, 255));
            Lengthpoint->setZValue(98);
            //        }else if(ui->tabWidget->currentIndex() == 1  && ui->YZlength_checkBox->isChecked()
            //              || ui->tabWidget->currentIndex() == 1  && ui->YZlength_checkBox->text() == "Length_on"){
            //              //2020.03.09 ショートカットキーによる"Length_on"判定追加 //if(ui->tabWidget->currentIndex()
            //              == 0  && ui->XZ_comboBox->currentText() == "Length"){
            //            on_YZRedraw_pushButton_clicked();
        }
        else if (ui->tabWidget->currentIndex() == 1) {
            Lengthpoint = YZscene->addEllipse(X1_CLICK - 15, -Y1_CLICK - 15, 30, 30, LengthPen, LengthBrush);
            Lengthpoint->setBrush(QColor(255, 255, 0, 255));
            Lengthpoint->setZValue(98);
            //        }else if(ui->tabWidget->currentIndex() == 2  && ui->XYlength_checkBox->isChecked()
            //              || ui->tabWidget->currentIndex() == 2  && ui->XYlength_checkBox->text() == "Length_on"){
            //              //2020.03.09 ショートカットキーによる"Length_on"判定追加 //if(ui->tabWidget->currentIndex()
            //              == 0  && ui->XZ_comboBox->currentText() == "Length"){
            //            on_XYRedraw_pushButton_clicked();
        }
        else if (ui->tabWidget->currentIndex() == 2) {
            Lengthpoint = XYscene->addEllipse(X1_CLICK - 15, -Y1_CLICK - 15, 30, 30, LengthPen, LengthBrush);
            Lengthpoint->setBrush(QColor(255, 255, 0, 255));
            Lengthpoint->setZValue(98);
        }
        else if (ui->tabWidget->currentIndex() == 3) {
            Lengthpoint = XZdiag_scene->addEllipse(X1_CLICK - 15, -Y1_CLICK - 15, 30, 30, LengthPen, LengthBrush);
            Lengthpoint->setBrush(QColor(255, 255, 0, 255));
            Lengthpoint->setZValue(98);
            //        }else if(ui->tabWidget->currentIndex() == 1  && ui->YZlength_checkBox->isChecked()
            //              || ui->tabWidget->currentIndex() == 1  && ui->YZlength_checkBox->text() == "Length_on"){
            //              //2020.03.09 ショートカットキーによる"Length_on"判定追加 //if(ui->tabWidget->currentIndex()
            //              == 0  && ui->XZ_comboBox->currentText() == "Length"){
            //            on_YZRedraw_pushButton_clicked();
        }
        //    } else {
    }
    else if (FLAG_LENGTH == 1) {
        // 　2点目クリック時点
        FLAG_LENGTH = 2;
        X2_CLICK    = x_meshAjust;
        Y2_CLICK    = y_meshAjust;

        int tmp_x1_click, tmp_x2_click, tmp_y1_click, tmp_y2_click, meshsize;
        meshsize     = ui->meshsize_Value->text().toInt();
        tmp_x1_click = X1_CLICK;
        tmp_x2_click = X2_CLICK;
        tmp_y1_click = Y1_CLICK;
        tmp_y2_click = Y2_CLICK;

        int x_width = abs(tmp_x2_click - tmp_x1_click);
        int y_width = abs(tmp_y2_click - tmp_y1_click);
        int length  = sqrt(x_width * x_width + y_width * y_width);

        // 2019.04.22 機能追加：Length_AnyAngle, Diagonal, Holizontal, Vertical
        // デフォルト=Any-Angleの場合
        int x_point = X2_CLICK - 15;
        int y_point = -Y2_CLICK - 15;
        int x1_line = X1_CLICK;
        int y1_line = -Y1_CLICK;
        int x2_line = X2_CLICK;
        int y2_line = -Y2_CLICK;
        int x_width_line, y_width_line, length_line;
        //        if( ui->tabWidget->currentIndex() == 0  && ui->XZlength_checkBox->isChecked()
        //                ||ui->tabWidget->currentIndex() == 1  && ui->YZlength_checkBox->isChecked()
        //                ||ui->tabWidget->currentIndex() == 2  && ui->XYlength_checkBox->isChecked()
        //                ||ui->tabWidget->currentIndex() == 0  && ui->XZlength_checkBox->text() == "Length_on"
        //                ||ui->tabWidget->currentIndex() == 1  && ui->YZlength_checkBox->text() == "Length_on"
        //                ||ui->tabWidget->currentIndex() == 2  && ui->XYlength_checkBox->text() == "Length_on"
        //                )
        //        {
        //            //2020.03.09 ショートカットキーによる "Length_on"判定追加
        //            if( ui->tabWidget->currentIndex() == 0  && ui->XZ_comboBox_Length->currentText() == "Diagonal"
        //                    ||ui->tabWidget->currentIndex() == 1  && ui->YZ_comboBox_Length->currentText() ==
        //                    "Diagonal"
        //                    ||ui->tabWidget->currentIndex() == 2  && ui->XY_comboBox_Length->currentText() ==
        //                    "Diagonal" )
        //            {
        //                //Length_Diagonalの場合: X1,Y1固定、
        //                X2,Y2：XかYで長い方を基準として変更・短い方を指定角度の位置にする if( x_width > y_width){
        //                    // X変更なし、Y位置を始点から45°の位置
        //                    int minusPlus = 1;
        //                    if(-Y1_CLICK > -Y2_CLICK){ minusPlus = -1;}
        //                    y_point = -Y1_CLICK + (x_width * minusPlus) -15 ;
        //                    y2_line = -Y1_CLICK + (x_width * minusPlus);
        //                } else {
        //                    // Y変更なし、X位置を始点から45°の位置
        //                    int minusPlus = 1;
        //                    if(X1_CLICK > X2_CLICK){ minusPlus = -1;}
        //                    x_point = X1_CLICK + (y_width * minusPlus) -15 ;
        //                    x2_line = X1_CLICK + (y_width * minusPlus);
        //                }
        //            } else if( ui->tabWidget->currentIndex() == 0  && ui->XZ_comboBox_Length->currentText() ==
        //            "Horizontal"
        //                       ||ui->tabWidget->currentIndex() == 1  && ui->YZ_comboBox_Length->currentText() ==
        //                       "Horizontal"
        //                       ||ui->tabWidget->currentIndex() == 2  && ui->XY_comboBox_Length->currentText() ==
        //                       "Horizontal" )
        //            {
        //                //Length_Horizontalの場合:
        //                y_point = -Y1_CLICK - 15;
        //                y2_line = -Y1_CLICK;
        //            } else if( ui->tabWidget->currentIndex() == 0  && ui->XZ_comboBox_Length->currentText() ==
        //            "Vertical"
        //                       ||ui->tabWidget->currentIndex() == 1  && ui->YZ_comboBox_Length->currentText() ==
        //                       "Vertical"
        //                       ||ui->tabWidget->currentIndex() == 2  && ui->XY_comboBox_Length->currentText() ==
        //                       "Vertical" )
        //            {
        //                //Length_Vertical の場合:
        //                x_point = X1_CLICK - 15;
        //                x2_line = X1_CLICK;
        //            }
        x_width_line = abs(x2_line - x1_line);
        y_width_line = abs(y2_line - y1_line);
        length_line  = sqrt(x_width_line * x_width_line + y_width_line * y_width_line);
        //        }

        //        if( (ui->tabWidget->currentIndex() == 0  && ui->XZlength_checkBox->isChecked())
        //        || (ui->tabWidget->currentIndex() == 0  && ui->XZlength_checkBox->text() == "Length_on") ){
        if (ui->tabWidget->currentIndex() == 0) {
            tmpStr = "width X:" + QString::number(x_width_line) + ", width Z:" + QString::number(y_width_line)
                   + ", Distance:" + QString::number(length_line);
            ui->XZinfo_label->setText(tmpStr);

            Lengthpoint = XZscene->addEllipse(x_point, y_point, 30, 30, LengthPen, LengthBrush);
            Lengthpoint->setBrush(QColor(255, 255, 0, 255));
            Lengthpoint->setZValue(98);

            Lengthline = XZscene->addLine(x1_line, y1_line, x2_line, y2_line, LengthlinePen);
            Lengthline->setZValue(97);

            lengthText = XZscene->addText(QString::number(length_line));
            lengthText->setFont(QFont("メイリオ", 150, QFont::DemiBold));
            lengthText->setDefaultTextColor(QColor(255, 255, 255, 255));
            lengthText->setGraphicsEffect(effect);
            if (X1_CLICK == X2_CLICK) {    // Verticalの場合
                lengthText->setPos((X2_CLICK + X1_CLICK) / 2 + 50, -(Y2_CLICK + Y1_CLICK) / 2 - 100);
            }
            else if (Y1_CLICK == Y2_CLICK) {    // Horizontalの場合
                lengthText->setPos((X2_CLICK + X1_CLICK) / 2 - 100, -(Y2_CLICK + Y1_CLICK) / 2 + 50);
            }
            else {                                                          // AnyAngle または　Dialgonal
                if ((Y2_CLICK - Y1_CLICK) / (X2_CLICK - X1_CLICK) > 0) {    // 傾きが＋なら
                    lengthText->setPos((X2_CLICK + X1_CLICK) / 2 + 50, -(Y2_CLICK + Y1_CLICK) / 2 - 50);
                }
                else {    // 傾きが-なら
                    lengthText->setPos((X2_CLICK + X1_CLICK) / 2, -(Y2_CLICK + Y1_CLICK) / 2 - 150);
                }
            }

            //         }else if( (ui->tabWidget->currentIndex() == 1  && ui->YZlength_checkBox->isChecked())
            //               || (ui->tabWidget->currentIndex() == 1  && ui->YZlength_checkBox->text() == "Length_on") ){
        }
        else if (ui->tabWidget->currentIndex() == 1) {
            tmpStr = "width Y:" + QString::number(x_width_line) + ", width Z:" + QString::number(y_width_line)
                   + ", Distance:" + QString::number(length_line);
            ui->YZinfo_label->setText(tmpStr);

            Lengthpoint = YZscene->addEllipse(x_point, y_point, 30, 30, LengthPen, LengthBrush);
            Lengthpoint->setBrush(QColor(255, 255, 0, 255));
            Lengthpoint->setZValue(98);

            Lengthline = YZscene->addLine(x1_line, y1_line, x2_line, y2_line, LengthlinePen);
            Lengthline->setZValue(97);

            lengthText = YZscene->addText(QString::number(length_line));
            lengthText->setFont(QFont("メイリオ", 150, QFont::DemiBold));
            lengthText->setDefaultTextColor(QColor(255, 255, 255, 255));
            lengthText->setGraphicsEffect(effect);
            if (X1_CLICK == X2_CLICK) {    // Verticalの場合
                lengthText->setPos((X2_CLICK + X1_CLICK) / 2 + 50, -(Y2_CLICK + Y1_CLICK) / 2 - 100);
            }
            else if (Y1_CLICK == Y2_CLICK) {    // Horizontalの場合
                lengthText->setPos((X2_CLICK + X1_CLICK) / 2 - 100, -(Y2_CLICK + Y1_CLICK) / 2 + 50);
            }
            else {                                                          // AnyAngle または　Dialgonal
                if ((Y2_CLICK - Y1_CLICK) / (X2_CLICK - X1_CLICK) > 0) {    // 傾きが＋なら
                    lengthText->setPos((X2_CLICK + X1_CLICK) / 2 + 50, -(Y2_CLICK + Y1_CLICK) / 2 - 50);
                }
                else {    // 傾きが-なら
                    lengthText->setPos((X2_CLICK + X1_CLICK) / 2, -(Y2_CLICK + Y1_CLICK) / 2 - 150);
                }
            }

            //        }else if( (ui->tabWidget->currentIndex() == 2  && ui->XYlength_checkBox->isChecked())
            //              || (ui->tabWidget->currentIndex() == 2  && ui->XYlength_checkBox->text() == "Length_on")){
        }
        else if (ui->tabWidget->currentIndex() == 2) {
            tmpStr = "width X:" + QString::number(x_width_line) + ", width Y:" + QString::number(y_width_line)
                   + ", Distance:" + QString::number(length_line);
            ui->XYinfo_label->setText(tmpStr);

            Lengthpoint = XYscene->addEllipse(x_point, y_point, 30, 30, LengthPen, LengthBrush);
            Lengthpoint->setBrush(QColor(255, 255, 0, 255));
            Lengthpoint->setZValue(98);

            Lengthline = XYscene->addLine(x1_line, y1_line, x2_line, y2_line, LengthlinePen);
            Lengthline->setZValue(97);

            lengthText = XYscene->addText(QString::number(length_line));
            lengthText->setFont(QFont("メイリオ", 150, QFont::DemiBold));
            lengthText->setDefaultTextColor(QColor(255, 255, 255, 255));
            lengthText->setGraphicsEffect(effect);
            if (X1_CLICK == X2_CLICK) {    // Verticalの場合
                lengthText->setPos((X2_CLICK + X1_CLICK) / 2 + 50, -(Y2_CLICK + Y1_CLICK) / 2 - 100);
            }
            else if (Y1_CLICK == Y2_CLICK) {    // Horizontalの場合
                lengthText->setPos((X2_CLICK + X1_CLICK) / 2 - 100, -(Y2_CLICK + Y1_CLICK) / 2 + 50);
            }
            else {                                                          // AnyAngle または　Dialgonal
                if ((Y2_CLICK - Y1_CLICK) / (X2_CLICK - X1_CLICK) > 0) {    // 傾きが＋なら
                    lengthText->setPos((X2_CLICK + X1_CLICK) / 2 + 50, -(Y2_CLICK + Y1_CLICK) / 2 - 50);
                }
                else {    // 傾きが-なら
                    lengthText->setPos((X2_CLICK + X1_CLICK) / 2, -(Y2_CLICK + Y1_CLICK) / 2 - 150);
                }
            }
        }
        else if (ui->tabWidget->currentIndex() == 3) {
            tmpStr = "width X:" + QString::number(x_width_line) + ", width Z:" + QString::number(y_width_line)
                   + ", Distance:" + QString::number(length_line);
            ui->XZinfo_label->setText(tmpStr);

            Lengthpoint = XZdiag_scene->addEllipse(x_point, y_point, 30, 30, LengthPen, LengthBrush);
            Lengthpoint->setBrush(QColor(255, 255, 0, 255));
            Lengthpoint->setZValue(98);

            Lengthline = XZdiag_scene->addLine(x1_line, y1_line, x2_line, y2_line, LengthlinePen);
            Lengthline->setZValue(97);

            lengthText = XZdiag_scene->addText(QString::number(length_line));
            lengthText->setFont(QFont("メイリオ", 150, QFont::DemiBold));
            lengthText->setDefaultTextColor(QColor(255, 255, 255, 255));
            lengthText->setGraphicsEffect(effect);
            if (X1_CLICK == X2_CLICK) {    // Verticalの場合
                lengthText->setPos((X2_CLICK + X1_CLICK) / 2 + 50, -(Y2_CLICK + Y1_CLICK) / 2 - 100);
            }
            else if (Y1_CLICK == Y2_CLICK) {    // Horizontalの場合
                lengthText->setPos((X2_CLICK + X1_CLICK) / 2 - 100, -(Y2_CLICK + Y1_CLICK) / 2 + 50);
            }
            else {                                                          // AnyAngle または　Dialgonal
                if ((Y2_CLICK - Y1_CLICK) / (X2_CLICK - X1_CLICK) > 0) {    // 傾きが＋なら
                    lengthText->setPos((X2_CLICK + X1_CLICK) / 2 + 50, -(Y2_CLICK + Y1_CLICK) / 2 - 50);
                }
                else {    // 傾きが-なら
                    lengthText->setPos((X2_CLICK + X1_CLICK) / 2, -(Y2_CLICK + Y1_CLICK) / 2 - 150);
                }
            }

            //         }else if( (ui->tabWidget->currentIndex() == 1  && ui->YZlength_checkBox->isChecked())
            //               || (ui->tabWidget->currentIndex() == 1  && ui->YZlength_checkBox->text() == "Length_on") ){
        }
    }

    //-end-  if(FLAG_LENGTH != 1 ){ ... } else { ... }
}

void GeometryInputForm::func_view_Length_float(float IN_X_f,
                                               float IN_Y_f)    // for-vox Length測長処理 斜め断面 XZdiagビューの場合
{
    float x_meshAjust = IN_X_f;    // 関数流用の都合上、変数を変えずに対応
    float y_meshAjust = IN_Y_f;

    QGraphicsEllipseItem* Lengthpoint;
    QBrush                LengthBrush;
    QPen                  LengthPen;
    LengthBrush.setColor(QColor(255, 0, 0, 255));
    LengthPen.setColor(QColor(0, 0, 0, 255));

    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect(this);
    effect->setBlurRadius(3);
    effect->setOffset(0);
    effect->setColor(Qt::black);

    QGraphicsLineItem* Lengthline;

    QPen LengthlinePen;
    LengthlinePen.setColor(QColor(230, 230, 230, 255));
    LengthlinePen.setWidth(10);

    QGraphicsTextItem* lengthText;

    QString tmpStr;

    if (FLAG_LENGTH != 1) {
        // １点目クリック時点
        FLAG_LENGTH = 1;
        X1_CLICK_f  = x_meshAjust;
        Y1_CLICK_f  = y_meshAjust;

        if (ui->tabWidget->currentIndex() == 3) {
            Lengthpoint = XZdiag_scene->addEllipse(X1_CLICK_f - 15, -Y1_CLICK_f - 15, 30, 30, LengthPen, LengthBrush);
            Lengthpoint->setBrush(QColor(255, 255, 0, 255));
            Lengthpoint->setZValue(98);
        }

        qDebug() << QString("[DEBUG]Geo.cpp-func_view_Length_float() X1_CLICK_f=%1 Y1_CLICK_f=%2")
                        .arg(QString::number(X1_CLICK_f), QString::number(Y1_CLICK_f));
    }
    else if (FLAG_LENGTH == 1) {
        // 　2点目クリック時点
        FLAG_LENGTH = 2;
        X2_CLICK_f  = x_meshAjust;
        Y2_CLICK_f  = y_meshAjust;

        qDebug() << QString("[DEBUG]Geo.cpp-func_view_Length_float() X2_CLICK_f=%1 Y2_CLICK_f=%2")
                        .arg(QString::number(X2_CLICK_f), QString::number(Y2_CLICK_f));

        float tmp_x1_click, tmp_x2_click, tmp_y1_click, tmp_y2_click, meshsize;
        meshsize     = ui->meshsize_Value->text().toInt();
        tmp_x1_click = X1_CLICK_f;
        tmp_x2_click = X2_CLICK_f;
        tmp_y1_click = Y1_CLICK_f;
        tmp_y2_click = Y2_CLICK_f;

        float x_width = abs(tmp_x2_click - tmp_x1_click);
        float y_width = abs(tmp_y2_click - tmp_y1_click);
        float length  = sqrt(x_width * x_width + y_width * y_width);

        // 2019.04.22 機能追加：Length_AnyAngle, Diagonal, Holizontal, Vertical
        // デフォルト=Any-Angleの場合
        float x_point = X2_CLICK_f - 15;
        float y_point = -Y2_CLICK_f - 15;
        float x1_line = X1_CLICK_f;
        float y1_line = -Y1_CLICK_f;
        float x2_line = X2_CLICK_f;
        float y2_line = -Y2_CLICK_f;
        float x_width_line, y_width_line, length_line;
        x_width_line = abs(x2_line - x1_line);
        y_width_line = abs(y2_line - y1_line);
        length_line  = sqrt(x_width_line * x_width_line + y_width_line * y_width_line);

        if (ui->tabWidget->currentIndex() == 3) {
            tmpStr = "width X:" + QString::number(x_width_line) + ", width Z:" + QString::number(y_width_line)
                   + ", Distance:" + QString::number(length_line);
            ui->XZinfo_label->setText(tmpStr);

            Lengthpoint = XZdiag_scene->addEllipse(x_point, y_point, 30, 30, LengthPen, LengthBrush);
            Lengthpoint->setBrush(QColor(255, 255, 0, 255));
            Lengthpoint->setZValue(98);

            Lengthline = XZdiag_scene->addLine(x1_line, y1_line, x2_line, y2_line, LengthlinePen);
            Lengthline->setZValue(97);

            lengthText = XZdiag_scene->addText(QString::number(length_line));
            lengthText->setFont(QFont("メイリオ", 150, QFont::DemiBold));
            lengthText->setDefaultTextColor(QColor(255, 255, 255, 255));
            lengthText->setGraphicsEffect(effect);
            if (X1_CLICK_f == X2_CLICK_f) {    // Verticalの場合
                lengthText->setPos((X2_CLICK_f + X1_CLICK_f) / 2 + 50, -(Y2_CLICK_f + Y1_CLICK_f) / 2 - 100);
            }
            else if (Y1_CLICK_f == Y2_CLICK_f) {    // Horizontalの場合
                lengthText->setPos((X2_CLICK_f + X1_CLICK_f) / 2 - 100, -(Y2_CLICK_f + Y1_CLICK_f) / 2 + 50);
            }
            else {                                                                  // AnyAngle または　Dialgonal
                if ((Y2_CLICK_f - Y1_CLICK_f) / (X2_CLICK_f - X1_CLICK_f) > 0) {    // 傾きが＋なら
                    lengthText->setPos((X2_CLICK_f + X1_CLICK_f) / 2 + 50, -(Y2_CLICK_f + Y1_CLICK_f) / 2 - 50);
                }
                else {    // 傾きが-なら
                    lengthText->setPos((X2_CLICK_f + X1_CLICK_f) / 2, -(Y2_CLICK_f + Y1_CLICK_f) / 2 - 150);
                }
            }

            //         }else if( (ui->tabWidget->currentIndex() == 1  && ui->YZlength_checkBox->isChecked())
            //               || (ui->tabWidget->currentIndex() == 1  && ui->YZlength_checkBox->text() == "Length_on") ){
        }
    }

    //-end-  if(FLAG_LENGTH != 1 ){ ... } else { ... }
}

void GeometryInputForm::func_view_LengthAll()
{
    FLAG_LENGTH = 0;

    if ((ui->tabWidget->currentIndex() == 0) && (lengthXZ_X.size() > 0) && (lengthXZ_X.size() == lengthXZ_Y.size())) {
        for (int n = 0; n < lengthXZ_X.size(); n++) {
            func_view_Length(lengthXZ_X.at(n), lengthXZ_Y.at(n));
        }
    }
    else if ((ui->tabWidget->currentIndex() == 1) && (lengthYZ_X.size() > 0)
             && (lengthYZ_X.size() == lengthYZ_Y.size())) {
        for (int n = 0; n < lengthYZ_X.size(); n++) {
            func_view_Length(lengthYZ_X.at(n), lengthYZ_Y.at(n));
        }
    }
    else if ((ui->tabWidget->currentIndex() == 2) && (lengthXY_X.size() > 0)
             && (lengthXY_X.size() == lengthXY_Y.size())) {
        for (int n = 0; n < lengthXY_X.size(); n++) {
            func_view_Length(lengthXY_X.at(n), lengthXY_Y.at(n));
        }
    }
    else if ((ui->tabWidget->currentIndex() == 3) && (lengthXZdiag_X.size() > 0)
             && (lengthXZdiag_X.size() == lengthXZdiag_Y.size())) {
        for (int n = 0; n < lengthXZdiag_X.size(); n++) {
            func_view_Length_float(lengthXZdiag_X.at(n), lengthXZdiag_Y.at(n));
        }
    }
}

// 自動測長処理 押下された1点から
// 垂直or水平にマテリアルごとに測長する。多数のポイントについて一括バッチ処理できるように関数化 + ui を使わない。
//  in_selectX, in_selectY, in_selectZ  測長開始点（GUIならviewをクリックした箇所)
//  in_view = xz, yz, xy, xydiag のいずれか
//  in_direction = H, V のいずれか　自動測長の方向　水平方向 or 垂直方向
void GeometryInputForm::func_view_AutoLength_batch(QStringList& res_matnameList, QList<float>& res_lengthfList,
                                                   QString in_voxfilePath, int in_mesh, float in_selectX,
                                                   float in_selectY, float in_selectZ, QString in_view,
                                                   QString in_direction)
{
    // 引数
    // in_voxfilepath　= voxファイルのパス
    // in_mesh        = voxファイルのメッシュ単位サイズ(20, 40, 5 など)
    // in_selectX, in_selectY, in_selectZ = 測長指定点 (GUIなら XZビューなどクリックされた座標)
    // in_view        = XZ, YZ, XY, XZdiag のいずれか
    // in_direction   = 測長方向　H, V のいずれか

    int mesh = in_mesh;

    //-start- 材質境界取得　共通 縦1列 or 横1列　でAutoLengthする場合
    QList<int> lineMatList, linePointList;
    int        matnum        = 0;
    int        matnum_before = 0;

    int end = g_nz;
    if (in_direction == "V") {
        if (in_view == "XZ" || in_view == "XY" || in_view == "XZdiag") {
            end = g_nz;
        }
        if (in_view == "XY") {
            end = g_ny;
        }
    }
    if (in_direction == "H") {
        if (in_view == "XZ") {
            end = g_nx;
        }
        if (in_view == "YZ") {
            end = g_ny;
        }
        if (in_view == "XY") {
            end = g_nx;
        }
        if (in_view == "XZdiag") {
            end = g_nxdiag;
        }
    }
    for (int p = 0; p < end; p++) {
        if (in_view == "XZ" || in_view == "YZ" || in_view == "XY") {
            int x = in_selectX / mesh;
            int y = in_selectY / mesh;
            int z = in_selectZ / mesh;
            if (in_direction == "V") {
                if (in_view == "XZ") {
                    z = p;
                }
                if (in_view == "YZ") {
                    z = p;
                }
                if (in_view == "XY") {
                    y = p;
                }
            }    // 縦1列AutoLengthする場合
            if (in_direction == "H") {
                if (in_view == "XZ") {
                    x = p;
                }
                if (in_view == "YZ") {
                    y = p;
                }
                if (in_view == "XY") {
                    x = p;
                }
            }    // 横1行AutoLengthする場合
            if (x < 0 || y < 0 || z < 0 || x >= g_nx || y >= g_ny || z >= g_nz) {
                continue;
            }    // 範囲外の場合、表示しない 処理スキップ
            matnum = g_voxDomain[x][y][z];
        }
        if (in_view == "XZdiag") {
            // 縦1列AutoLengthする場合
            int x = int((in_selectX / sqrt(2)) / mesh);
            int y = p;
            // 横1行AutoLengthする場合
            if (in_direction == "H") {
                x = p;
                y = int(in_selectZ / mesh);
            }
            if (x < 0 || y < 0 || x >= g_nxdiag || y >= g_nz) {
                continue;
            }    // 処理スキップ
            matnum = g_XZdiagNow2d[x][y];
        }

        // マテリアル始点を登録
        if (p == 0) {
            matnum_before = matnum;
            continue;
        }
        if (matnum != matnum_before) {
            // マテリアルがｚ方向1段下と違う場合  マテリアル始点を登録
            linePointList << p;
            lineMatList << matnum_before;
        }
        if (p == end - 1) {
            linePointList << p;
            lineMatList << matnum;
        }
        matnum_before = matnum;
    }

    //-end- 材質境界取得　共通 縦1列 or 横1列　でAutoLengthする場合

    //-start- DEBUG用表示
    QStringList restextList;
    int         cnt        = 0;
    int         cntDrawMat = 0;    // 測長結果の個数
    for (int i = 0; i < lineMatList.size(); i++) {
        if (lineMatList.at(i) != -1) {
            cntDrawMat++;
        }
    }
    //// 前準備:文字揃えるため
    int maxMatNameLength = 0;
    for (int i = 0; i < g_VoxMatNameList.size(); i++) {
        if (g_VoxMatNameList.at(i).length() > maxMatNameLength) {
            maxMatNameLength = g_VoxMatNameList.at(i).length();
        }
    }
    ////関数結果返り値作成  と　DEBUG用表示内容
    QStringList  tmp_matnameList;
    QList<float> tmp_lengthfList;
    for (int i = 0; i < linePointList.size(); i++) {
        // マテリアル番号が -1 の場合は描かない
        if (in_view == "XZdiag" && lineMatList.at(i) == -1) {
            continue;
        }
        cnt = cnt + 1;

        // GUIテキストフォームに結果表示するための文字列
        QString matname = "";
        //// マテリアル番号
        matnum = lineMatList.at(i);
        //// マテリアル名
        int tmpIndex = g_VoxMatIDList.indexOf(QString::number(matnum));
        if (tmpIndex > -1) {
            matname = g_VoxMatNameList.at(tmpIndex);
        }
        //// 測長文字列
        int     length  = 0;
        QString viewMsg = "";
        if (i == 0) {
            length = linePointList.at(i) * mesh;
        }
        else {
            length = (linePointList.at(i) - linePointList.at(i - 1)) * mesh;
        }
        if (i == linePointList.size() - 1) {    // 最後の材質は +1meshする
            length = length + mesh;
        }
        float length_f = float(length) * sqrt(2);    // 斜め断面　横測長の場合のみ

        // 関数結果返り値
        tmp_matnameList.append(matname);
        if (in_view == "XZdiag" && in_direction == "H") {
            tmp_lengthfList.append(length_f);
        }
        else {
            tmp_lengthfList.append(float(length));
        }

        //[DEBUG] DEBUG表示用途
        int tmpnum = cnt;    // 横測長　1始まり
        if (in_direction == "V") {
            tmpnum =
                cntDrawMat - cnt + 1;    // ビュー画面　上から 1, 2, 3...　となるように、逆順にする。　+1で、1始まり。
        }
        QString tmpStrNum = QString::number(tmpnum).leftJustified(6, ' ');    // 空白で文字列末尾を埋める
        QString tmpStrMat = matname.leftJustified(maxMatNameLength, ' ');
        QString resStr    = QString("%1 %2\t%3").arg(tmpStrNum, tmpStrMat, QString::number(length));
        if (in_view == "XZdiag" && in_direction == "H") {
            // 斜め断面で、測長　横方向の場合　length_f 小数点2桁まで表示する
            resStr = QString("%1 %2\t%3")
                         .arg(tmpStrNum, tmpStrMat,
                              QString::number(length_f, 'f', 2).rightJustified(12, ' '));    // 小数点2桁まで表示
        }
        restextList << resStr;
    }
    QStringList restextList2;
    res_matnameList.clear();
    res_lengthfList.clear();
    if (in_direction == "V") {
        // 縦測長の場合　結果表示　(上から1,2,3... ）のため、逆順にする
        for (int i = tmp_matnameList.size() - 1; i >= 0; i--) {
            res_matnameList << tmp_matnameList.at(i);
            res_lengthfList << tmp_lengthfList.at(i);
            restextList2 << restextList.at(i);
        }
    }
    if (in_direction == "H") {
        res_matnameList = tmp_matnameList;
        res_lengthfList = tmp_lengthfList;
        // 横測長の場合
        for (int i = 0; i < restextList.size(); i++) {
            restextList2 << restextList.at(i);
        }
    }
    //-end- [DEBUG] DEBUG表示用途
    // for(int i=0; i<restextList2.size(); i++){
    //    qDebug() << "[DEBUG]func_view_AutoLength_batch restextList2 i=" << QString::number(i) << " " <<
    //    restextList2.at(i);
    //}
    //
    // qDebug() << "[DEBUG]Geo.cpp func_view_AutoLength linePointList" << linePointList;
    // qDebug() << "[DEBUG]Geo.cpp func_view_AutoLength linematList" << lineMatList;
    //-end- [DEBUG] DEBUG表示用途
}

void GeometryInputForm::func_view_AutoLength()
{
    float   clickX_f  = 99999;
    float   clickY_f  = 99999;
    QString direction = "V";
    if (ui->vox_radioButton_autolength_directionH->isChecked()) {
        direction = "H";
    }
    int tabnum = ui->tabWidget->currentIndex();
    if (tabnum == 0) {    // XZビュー
        clickX_f  = g_autoXZ_X;
        clickY_f  = g_autoXZ_Y;
        direction = g_autoXZ_direction;
    }
    if (tabnum == 1) {    // YZビュー
        clickX_f  = g_autoYZ_X;
        clickY_f  = g_autoYZ_Y;
        direction = g_autoYZ_direction;
    }
    if (tabnum == 2) {    // XYビュー
        clickX_f  = g_autoXY_X;
        clickY_f  = g_autoXY_Y;
        direction = g_autoXY_direction;
    }
    if (tabnum == 3) {    // XZdiagビュー
        clickX_f  = g_autoXZdiag_X;
        clickY_f  = g_autoXZdiag_Y;
        direction = g_autoXZdiag_direction;
    }

    // 処理前判断　範囲外の場合、表示しない->処理終了
    int mesh = ui->meshsize_Value->text().toInt();
    // if(ui->vox_radioButton_autolength_directionV->isChecked()){
    if (direction == "V") {
        if (clickX_f < 0) {
            return;
        }
        if (tabnum == 0 && clickX_f > g_nx * mesh) {
            return;
        }    // XZビュー
        if (tabnum == 1 && clickX_f > g_ny * mesh) {
            return;
        }    // YZビュー
        if (tabnum == 2 && clickX_f > g_nx * mesh) {
            return;
        }    // XYビュー
        if (tabnum == 3 && clickX_f > g_nxdiag * mesh * sqrt(2)) {
            return;
        }    // XZdiagビュー
    }
    // if(ui->vox_radioButton_autolength_directionH->isChecked()){
    if (direction == "H") {
        if (clickY_f < 0) {
            return;
        }
        float max = float(g_nz);
        if (tabnum == 2) {
            max = g_ny;
        }
        if (clickY_f > max * mesh) {
            return;
        }
    }

    // ビューへの結果表示設定
    QBrush useBrush[3];
    QPen   usePen[3];
    QColor useTextcolor[3];

    // 色が決まったら入れる
    useBrush[0]     = QColor(51, 51, 51, 255);       // 黒
    useBrush[1]     = QColor(170, 170, 170, 255);    // 灰色
    useBrush[2]     = QColor(245, 245, 220, 255);    // 白
    usePen[0]       = QColor(51, 51, 51, 255);
    usePen[1]       = QColor(170, 170, 170, 255);
    usePen[2]       = QColor(245, 245, 220, 255);
    useTextcolor[0] = QColor(51, 51, 51, 255);
    useTextcolor[1] = QColor(170, 170, 170, 255);
    useTextcolor[2] = QColor(245, 245, 220, 255);

    // QColor(0, 0, 0, 255); 		//Black
    // QColor(255, 255, 255, 255);	//White
    // QColor(211, 211, 211, 255); 	//Lightgray
    // QColor(255, 0, 0, 255); 	//Red
    // QColor(255, 255, 0, 255); 	//Yellow

    QBrush draw1Brush;
    QPen   draw1Pen;
    QColor draw1Textcolor;

    int colnum = ui->vox_comboBox_autolength_color->currentIndex();

    draw1Brush     = useBrush[colnum];    // QColor(211, 211, 211, 255); 	//Lightgray
    draw1Pen       = usePen[colnum];
    draw1Textcolor = useTextcolor[colnum];

    draw1Pen.setWidth(10);

    QGraphicsTextItem* draw1Text;
    GeometrySubScene*  nowscene;

    // int nowpos = 0;
    if (ui->tabWidget->currentIndex() == 0) {
        nowscene = XZscene;    // 変数置き換え用
    }
    if (ui->tabWidget->currentIndex() == 1) {
        nowscene = YZscene;
    }
    if (ui->tabWidget->currentIndex() == 2) {
        nowscene = XYscene;
    }
    if (ui->tabWidget->currentIndex() == 3) {
        nowscene = XZdiag_scene;
    }

    //-start- func_view_AutoLength_関数呼び出しで　マテリアルごとに マテリアル名、長さを取得する
    QString voxfilePath = ui->vox_path_lineEdit->text();

    QString view = "XZ";
    if (ui->tabWidget->currentIndex() == 0) {
        view = "XZ";
    }
    if (ui->tabWidget->currentIndex() == 1) {
        view = "YZ";
    }
    if (ui->tabWidget->currentIndex() == 2) {
        view = "XY";
    }
    if (ui->tabWidget->currentIndex() == 3) {
        view = "XZdiag";
    }
    // 測長指定座標　XZの場合
    float selectX = clickX_f;    // ui->vox_lineEdit_autolength_pointXY->text().toInt(); //X1_CLICK;
    float selectY = ui->Ycoordinate_spin_lineEdit->text().toFloat();
    float selectZ = clickY_f;
    if (ui->tabWidget->currentIndex() == 1) {    // YZの場合
        selectX = ui->Xcoordinate_spin_lineEdit->text().toFloat();
        selectY = clickX_f;
        selectZ = clickY_f;
    }
    if (ui->tabWidget->currentIndex() == 2) {    // YZの場合
        selectX = clickX_f;
        selectY = clickY_f;
        selectZ = ui->Zcoordinate_spin_lineEdit->text().toFloat();
    }
    if (ui->tabWidget->currentIndex() == 3) {    // XZdiagの場合
        selectX = clickX_f;
        selectY = 0;    // XZdiagビューではg_XZdiagNow2d[x][y]を参照するため、使わない。
        selectZ = clickY_f;
    }

    // int mesh = ui->meshsize_Value->text().toInt();
    // QString direction = "H";
    // if(ui->vox_radioButton_autolength_directionV->isChecked()){ direction = "V"; }

    QStringList  res_matnameList;    // 関数結果の返り値用に空のリスト
    QList<float> res_lengthfList;    // 関数結果の返り値用に空のリスト
    func_view_AutoLength_batch(res_matnameList, res_lengthfList, voxfilePath, mesh, selectX, selectY, selectZ, view,
                               direction);

    float lengthAll = 0;
    for (int i = 0; i < res_lengthfList.size(); i++) {
        lengthAll += res_lengthfList.at(i);
    }
    // if(ui->vox_radioButton_autolength_directionV->isChecked() && ui->tabWidget->currentIndex()==3 && clickX_f >
    // lengthAll){ return; } //範囲外のため表示しない
    if (direction == "V" && ui->tabWidget->currentIndex() == 3 && clickX_f > lengthAll) {
        return;
    }    // 範囲外のため表示しない
    // 測長方向全体に線を引く
    int x1 = clickX_f;
    int y1 = 0;
    int x2 = clickX_f;
    int y2 = ui->domainZ_Value->text().toInt();
    // if(ui->vox_radioButton_autolength_directionV->isChecked()){
    if (direction == "V") {
        // 縦方向にAutoLengthする場合
        if (ui->tabWidget->currentIndex() == 2) {
            y2 = ui->domainY_Value->text().toInt();
        }    // XY平面の場合
    }
    // if(ui->vox_radioButton_autolength_directionH->isChecked()){
    if (direction == "H") {
        // 横方向にAutoLengthする場合
        x1 = 0;
        y1 = clickY_f;
        x2 = ui->domainX_Value->text().toInt();
        y2 = clickY_f;
        if (ui->tabWidget->currentIndex() == 0) {
            x2 = ui->domainX_Value->text().toInt();
        }
        if (ui->tabWidget->currentIndex() == 1) {
            x2 = ui->domainY_Value->text().toInt();
        }
        if (ui->tabWidget->currentIndex() == 2) {
            x2 = ui->domainX_Value->text().toInt();
        }
        // if(ui->tabWidget->currentIndex() == 3 ){ x2 = g_nxdiag * mesh; } //XZdiag平面の場合
        if (ui->tabWidget->currentIndex() == 3) {
            x2 = lengthAll;
        }    // XZdiag平面の場合
    }
    nowscene->addLine(x1, -y1, x2, -y2, draw1Pen);    // Qt座標は 上方向=マイナス

    // 水平AutoLength 横ライン(材質境界の印として), GUI画面 textEditへの記入
    // ui->vox_tabWidget->setCurrentIndex(1); //AutoLengthタブの表示
    ui->vox_textEdit_autolength_result->setText("");    // フォームクリア
    int maxMatNameLength = 0;
    // 文字揃えるため
    for (int i = 0; i < g_VoxMatNameList.size(); i++) {
        if (g_VoxMatNameList.at(i).length() > maxMatNameLength) {
            maxMatNameLength = g_VoxMatNameList.at(i).length();
        }
    }
    // 材質境界線の位置のため
    int fontsize = 100;
    fontsize     = ui->vox_comboBox_autolength_fontsize->currentText().toInt();
    QList<float> pointfList;        // 境界線の位置
    QList<float> textPointfList;    // 文字列の位置　2つの境界線の中央付近
    // if(ui->vox_radioButton_autolength_directionH->isChecked()){
    if (direction == "H") {
        // res_lengthfList 横の場合のlengthは、画面左 0->dX　の順で入っている
        float pointSum = 0;
        for (int i = 0; i < res_lengthfList.size(); i++) {
            pointfList << pointSum + res_lengthfList.at(i);

            float pointText = pointSum + res_lengthfList.at(i) / 2;
            pointText       = pointText - fontsize;    // 見た目調整。フォントサイズより
                                                 // lenght間隔が小さい場合、フォントが下がって見えてしまう対処として。
            textPointfList << pointText;

            pointSum += res_lengthfList.at(i);
        }
    }
    else {
        // res_lengthfList 縦の場合のlengthは、画面上から dZ->0　の順で入っている
        float        pointNow = 0;
        QList<float> tmp_pointfList;
        QList<float> tmp_textPointfList;
        // 逆順 ビュー下→上方向として元データ参照する
        for (int i = res_lengthfList.size() - 1; i >= 0; i--) {
            pointNow = pointNow + res_lengthfList.at(i);
            tmp_pointfList << pointNow;
            float pointText = pointNow - res_lengthfList.at(i) / 2 + fontsize;
            // if(res_lengthfList.at(i) < fontsize){ pointText =  pointText + fontsize;}//見た目調整。フォントサイズより
            // lenght間隔が小さい場合、フォントが下がって見えてしまう対処として。
            tmp_textPointfList << pointText;
        }
        // 逆順　上→下に戻す
        for (int i = tmp_pointfList.size() - 1; i >= 0; i--) {
            pointfList << tmp_pointfList.at(i);
            textPointfList << tmp_textPointfList.at(i);
        }
    }

    // ビューに結果文字列・測長線を表示する
    QStringList restextList;
    int         cnt        = 0;
    int         cntDrawMat = 0;
    for (int i = 0; i < res_matnameList.size(); i++) {
        if (res_matnameList.at(i) != "-1") {
            cntDrawMat++;
        }
    }
    for (int i = 0; i < res_lengthfList.size(); i++) {
        // マテリアル番号が -1 の場合は描かない
        if (ui->tabWidget->currentIndex() == 3 && res_matnameList.at(i) == "-1") {
            continue;
        }
        cnt = cnt + 1;

        // 目盛り線　材質境界ごと
        //  AutoLength　縦方向の場合
        int x1 = clickX_f - 30;
        int y1 = pointfList.at(i);
        int x2 = clickX_f + 30;
        int y2 = pointfList.at(i);
        // if(ui->vox_radioButton_autolength_directionH->isChecked()){
        if (direction == "H") {
            // AutoLength　横方向の場合
            x1 = pointfList.at(i);
            y1 = clickY_f - 30;
            x2 = pointfList.at(i);
            y2 = clickY_f + 30;
        }
        // AutoLength　縦方向の場合//nowscene->addLine( clickX_f - 10 , -(res_lengthfList.at(i)*mesh) , clickX_f + 10 ,
        // -(res_lengthfList.at(i)*mesh), draw1Pen);
        nowscene->addLine(x1, -y1, x2, -y2, draw1Pen);

        QString matname = "";
        // マテリアル番号
        // matnum = res_matnameList.at(i); //新規処理で一旦コメントアウト
        // マテリアル名
        // int tmpIndex = g_VoxMatIDList.indexOf(QString::number(matnum));
        // if(tmpIndex > -1){ matname = g_VoxMatNameList.at(tmpIndex); }
        matname = res_matnameList.at(i);
        // 測長文字列
        float length_f = res_lengthfList.at(i);
        int   length   = int(res_lengthfList.at(i));

        // 描画する文字列
        QString viewMsg = "";
        int     tmpnum  = cnt;                                              // 横測長　1始まり
        viewMsg         = QString(" <%1>").arg(QString::number(tmpnum));    // 画面結果表示　番号のみ
        // if(ui->vox_checkBox_autolength_viewResult->isChecked()){ //画面結果表示　番号　マテリアル名　測長値
        //     viewMsg = QString(" <%1> %2 %3").arg(QString::number(tmpnum), matname,QString::number(length));
        //
        //     if(ui->vox_radioButton_autolength_directionH->isChecked()){
        //         //[DEBUG] 横方向　隣の文字が重ならないように改行入れる
        //         viewMsg = QString("<%1>\n %2\n %3").arg(QString::number(tmpnum), matname, QString::number(length));
        //         if(ui->tabWidget->currentIndex() == 3 ){ //XZdiag 斜め断面の場合
        //             viewMsg = QString("<%1>\n %2\n %3").arg(QString::number(tmpnum), matname,
        //             QString::number(length_f));
        //         }
        //     }
        // }
        if (ui->vox_checkBox_autolength_viewResult->isChecked()
            || ui->vox_checkBox_autolength_viewResult_mat->isChecked()) {    // 画面結果表示　番号　マテリアル名　測長値
            // 縦方向
            viewMsg = QString(" <%1>").arg(QString::number(tmpnum));
            if (ui->vox_checkBox_autolength_viewResult_mat->isChecked()) {
                viewMsg = QString(" <%1> %2").arg(QString::number(tmpnum), matname);
            }
            if (ui->vox_checkBox_autolength_viewResult->isChecked()) {
                viewMsg = viewMsg + " " + QString::number(length);
            }

            // if(ui->vox_radioButton_autolength_directionH->isChecked()){
            if (direction == "H") {
                // 横方向　隣の文字が重ならないように改行入れる
                viewMsg = QString(" <%1>").arg(QString::number(tmpnum));
                if (ui->vox_checkBox_autolength_viewResult_mat->isChecked()) {
                    viewMsg = QString("<%1>\n %2").arg(QString::number(tmpnum), matname);
                }
                if (ui->vox_checkBox_autolength_viewResult->isChecked()) {
                    if (ui->tabWidget->currentIndex() == 3) {    // XZdiag 斜め断面の場合
                        viewMsg = viewMsg + "\n " + QString::number(length_f);
                    }
                    else {
                        viewMsg = viewMsg + "\n " + QString::number(length);
                    }
                }
            }
        }

        //
        // GUIテキストフォームに結果表示するための文字列保持
        QString tmpStrNum = QString::number(tmpnum).leftJustified(6, ' ');    // 空白で文字列末尾を埋める
        QString tmpStrMat = matname.leftJustified(maxMatNameLength, ' ');
        QString resStr    = QString("%1 %2\t%3").arg(tmpStrNum, tmpStrMat, QString::number(length));
        // if(ui->tabWidget->currentIndex() == 3 && ui->vox_radioButton_autolength_directionH->isChecked()){
        if (ui->tabWidget->currentIndex() == 3 && direction == "H") {
            // 斜め断面で、測長　横方向の場合　length_f 小数点2桁まで表示する
            resStr = QString("%1 %2\t%3")
                         .arg(tmpStrNum, tmpStrMat,
                              QString::number(length_f, 'f', 2).rightJustified(12, ' '));    // 小数点2桁まで表示
        }
        restextList << resStr;

        // ビューに結果テキスト表示
        int fontsize = 100;
        fontsize     = ui->vox_comboBox_autolength_fontsize->currentText().toInt();
        draw1Text    = nowscene->addText(viewMsg);
        draw1Text->setFont(QFont("Times", fontsize));
        draw1Text->setDefaultTextColor(draw1Textcolor);

        // 縦方向に測長する場合
        x1 = clickX_f;
        y1 = textPointfList.at(i);
        // if(ui->vox_radioButton_autolength_directionV->isChecked()){
        //     Y1 = textPointfList.at(i);
        // }
        // if(ui->vox_radioButton_autolength_directionH->isChecked()){
        if (direction == "H") {
            // 横方向に測長する場合
            x1 = textPointfList.at(i);
            y1 = clickY_f;
        }
        draw1Text->setPos(x1, -y1);    // Qt Graphic座標系のため　-Y1
    }
    // GUI 結果をテキストリストフォームに表示
    for (int i = 0; i < restextList.size(); i++) {
        ui->vox_textEdit_autolength_result->setText(ui->vox_textEdit_autolength_result->toPlainText()
                                                    + restextList.at(i) + "\n");
    }

    // qDebug() << "[DEBUG]Geo.cpp func_view_AutoLength res_lengthfList" << res_lengthfList;
    // qDebug() << "[DEBUG]Geo.cpp func_view_AutoLength res_matnameList" << res_matnameList;
}

void GeometryInputForm::func_vox_readTo3Darray()
{    // voxファイルの3次元配列への格納

    QFile voxinfile(ui->vox_path_lineEdit->text());    // 例 "C:/kuroda/work/00_Data_vox/test_basic/test_basic.vox")
                                                       // //QFile voxinfile(ui->tableWidget->item(i, 2)->text());
    if (!voxinfile.open(QIODevice::ReadOnly)) {
        // QMessageBox::information(this, "can't open", "can't open");
        return;
    }

    func_vox_readHeader(ui->vox_path_lineEdit->text());    // GUIフォーム欄　設定のため

    int         VoxUnitlength = 20;
    QTextStream in(&voxinfile);
    QString     VoxfileLine;
    QStringList VoxfileLineList;
    QString     nxstr, nystr, nzstr;
    int         nx, ny, nz;
    while (!in.atEnd()) {
        VoxfileLine = in.readLine(0);
        if (VoxfileLine.indexOf(" x ") > 0) {
            VoxfileLineList = VoxfileLine.split(" x ");
            nxstr           = VoxfileLineList.at(0);
            nystr           = VoxfileLineList.at(1);
            nzstr           = VoxfileLineList.at(2);
            nx              = nxstr.toInt();
            ny              = nystr.toInt();
            nz              = nzstr.toInt();
            break;
        }
        if (VoxfileLine.indexOf("unitlength") > 0) {
            VoxfileLineList   = VoxfileLine.split("\t");
            QString tmpString = VoxfileLineList.at(0);
            float   tmpFloat  = tmpString.toFloat();
            VoxUnitlength = int(tmpFloat * 1000);    //.voxファイルではum単位での記載　FDTDsolverのnm単位に合わせる
        }
    }

    // 初期化
    // int ***g_voxDomain = new int**[nx];
    g_voxDomain = new int**[nx];

    for (int x = 0; x < nx; x++) {
        g_voxDomain[x] = new int*[ny];
        for (int y = 0; y < ny; y++) {
            // g_voxDomain[x][y] = new int[nz];
            g_voxDomain[x][y] = new int[nz]();    // 2018.04.11配列初期化(データが無い場合など0を入れる）
        }
    }

    // 3次元配列に代入
    for (int z = 0; z < nz; z++) {
        for (int y = 0; y < ny; y++) {
            VoxfileLine = in.readLine(0);

            VoxfileLineList = VoxfileLine.split(" ");

            int countx = 0;
            for (int x = 0; x < VoxfileLineList.size() / 2; x++) {
                QString tmpnumstr1 = VoxfileLineList.at(2 * x);
                int     tmpnum1    = tmpnumstr1.toInt();
                QString tmpnumstr2 = VoxfileLineList.at(2 * x + 1);
                int     tmpnum2    = tmpnumstr2.toInt();
                for (int n = 0; n < tmpnum2; n++) {
                    g_voxDomain[countx + n][y][z] = tmpnum1;
                    // qDebug() << "[DEBUG]01 g_voxDomain[x][y][z]=" << g_voxDomain[x][y][z];
                }
                countx += tmpnum2;
            }
            //}
            //-end- for(int l = 0; l < (VoxfileLineList.size() / 2); l++)
        }
        //-end-1 for(int y = 0; y < ny; y++)
    }
    //-end- for(int z = 0; z < nz; z++)

    ////[DEBUG]内容表示
    // for(int  z=0; z < nz; z++){
    //     QString tmpstr;
    //     qDebug() << "\n[DEBUG]z=" << QString::number(z) << "bottom-left (0,0) top-right=(xmax, ymax)";
    //     for(int  y=ny-1; y >=0 ; y--){ //確認用に Y座標　左下(0,0) 右上(xmax, ymax)で表示する
    //         QString tmpstr="";
    //         for(int x=0; x< nx; x++){
    //             tmpstr = tmpstr + " " + QString::number(g_voxDomain[x][y][z]);
    //         }
    //         qDebug() << "[DEBUG] " << tmpstr;
    //     }
    // }

    // 終了 3次元配列
    // for(int x = 0; x <nx; x++){
    //     for(int y = 0; y < ny; y++){
    //         delete[] g_voxDomain[x][y];
    //     }
    //     delete[] g_voxDomain[x];
    // }
    // delete[] g_voxDomain;

    //---------------
    // 初期化 int **g_XZdiagNow2d; //現時点で表示されている斜め断面の形状データ（1面のみ)
    g_nxdiag      = int(ceil(nx * sqrt(2)));    // 斜め断面最大として配列準備 ceilで切り上げ
    g_XZdiagNow2d = new int*[g_nxdiag];
    for (int x = 0; x < g_nxdiag; x++) {
        g_XZdiagNow2d[x] =
            new int[nz]();    // 配列初期化(データが無い場合など0を入れる） //入れるデータはXZ, YZ断面　いずれか1面だけ
    }
    //[DEBUG]表示
    // for(int z = 0; z < nz; z++){
    //    for(int x = 0; x < nx; x++){
    //        qDebug() << QString("[DEBUG]Geo.cpp-func_vox_readTo3Darray() z=%1 x=%2
    //        voxDiagNow[x][z]=%3").arg(QString::number(z), QString::number(x), QString::number(g_XZdiagNow2d[x][z]));
    //    }
    //    qDebug() << "\n";
    //}
    //
    // 終了 2次元配列
    // for(int x = 0; x <nx; x++){
    //    delete[] g_XZdiagNow2d[x];
    //}
    // delete[] g_XZdiagNow2d;
}

void GeometryInputForm::func_vox_make_XZgeometry()    // viewerXZの描画 voxファイル 3D配列データをもとにする
{
    // loadボタンで別途読み込む(スライダーchangedなどでは読み込まない。 //func_vox_readTo3Darray(); //[DEBUG]

    // QMessageBox::information(this, "notice","[DEBUG] on_XZRedraw_XZ_pushButton_DEBUG1_clicked");
    int nx = ui->domainX_Value->text().toInt() / ui->meshsize_Value->text().toInt();
    int ny = ui->domainY_Value->text().toInt() / ui->meshsize_Value->text().toInt();
    int nz = ui->domainZ_Value->text().toInt() / ui->meshsize_Value->text().toInt();

    XZscene->clear();
    int dX   = (int)ui->domainX_Value->text().toDouble();     //[DEBUG]10000
    int dY   = (int)ui->domainY_Value->text().toDouble();     //[DEBUG]10000
    int dZ   = (int)ui->domainZ_Value->text().toDouble();     //[DEBUG]10000
    int Mesh = (int)ui->meshsize_Value->text().toDouble();    //[DEBUG]20
                                                              //    int mesh = Mesh;
    int     Xdata      = dX / Mesh;
    int     Ydata      = dY / Mesh;
    int     Zdata      = dZ / Mesh;
    int     YposSlider = ui->Ycoordinate_spin_lineEdit->text().toInt();
    int     Ypos       = ui->Ycoordinate_spin_lineEdit->text().toInt() - 1;
    int     Yposdata   = (int)((Ypos + (double)Mesh / 2 - 0.00001) / Mesh);    // 本番用
    QString tmp_tooltip;
    int     VoxUnitlength;
    QString tmpString;
    float   tmpFloat;

    // テーブルから、マテリアル番号から色を特定するハッシュの作成
    QHash<int, QString> matnumToColorHash;
    for (int row = 0; row < ui->tableWidget->rowCount(); row++) {
        int               matnum   = ui->tableWidget->item(row, 1)->text().toInt();
        CustomColorCombo* tmpcombo = static_cast<CustomColorCombo*>(ui->tableWidget->cellWidget(row, 3));
        QString           color    = tmpcombo->currentText();
        matnumToColorHash.insert(matnum, color);
    }

    // pixmapによる描画
    int     XWidthPixmap = dX;
    int     YWidthPixmap = dZ;
    QPixmap pix(XWidthPixmap,
                YWidthPixmap);    // マイナスは指定できない
                                  // 最終のitem->offset　有り無しいずれにしてもitem1->setOffset(0, -YWidthPixmap );
    QPainter painter(&pix);

    QBrush lightcyanBrush(QColor(224, 255, 255, 255));
    QPen   whitePen(QColor(255, 255, 255, 0));

    //-start- vox描画箇所の設定
    // int xposition_d = 0;
    // int yposition_d = 0;
    // int zposition_d = 0;
    // int xposition, yposition, zposition;
    // xposition = (int)((xposition_d + (double)Mesh / 2 - 0.00001) / (double)Mesh) * Mesh;
    // yposition = (int)((yposition_d + (double)Mesh / 2 - 0.00001) / (double)Mesh) * Mesh;
    // zposition = (int)((zposition_d + (double)Mesh / 2 - 0.00001) / (double)Mesh) * Mesh;

    if (Yposdata >= ny) {
        return;
    }
    for (int z = 0; z < nz; z++) {        // 高さ方向
        for (int x = 0; x < nx; x++) {    // 水平方向
            int x2cnt = 0;
            for (int x2 = x + 1; x2 < nx; x2++) {
                if (g_voxDomain[x2][Yposdata][z] == g_voxDomain[x2 - 1][Yposdata][z]) {
                    x2cnt++;
                }
                else {
                    break;
                }
            }
            // int tmp_xposition = xposition + (countx * Mesh); //現在の始点： オフセット開始座標X
            // ＋ここまでのvoxファイル内容
            int tmp_xposition = x * Mesh;    // xposition + x * Mesh;
            int tmp_yposition = z * Mesh;    // zposition + (z * Mesh);
            int tmp_xWidth    = Mesh * (x2cnt + 1);
            int tmp_yWidth    = Mesh;

            // ΔX範囲をはみ出す場合の対処
            if ((tmp_xposition + tmp_xWidth) > dX) {
                tmp_xWidth = dX - tmp_xposition;
            }
            // ΔZ範囲をはみ出す場合の対処
            if (tmp_yposition > dZ) {
                tmp_yWidth = dZ - tmp_yposition;
            }

            if (tmp_xposition <= dX && tmp_yposition <= dZ && (tmp_xposition + tmp_xWidth > 0)) {
                // pixmapによる描画
                QRect rect(tmp_xposition, tmp_yposition, tmp_xWidth, tmp_yWidth);

                QString tmpstr;
                // qDebug() << "[DEBUG]func_vox_make_XZgeometry() " << tmpstr.asprintf(" x=%d z=%d tmp_xposition=%d
                // tmp_xposition=%d tmp_xWidth=%d tmp_yWidth=%d\n", x, z, tmp_xposition, tmp_xposition, tmp_xWidth,
                // tmp_yWidth);

                // QString  matcolor = acceptedMaterialcolorList.at(0); //2018.02.14 Airの表示色も変えられるよう処理変更
                // int b = func_setBrushesNum(matcolor);
                int b = 1;    // 暫定初期値 Gray
                // Yposdata = ui->XZ_DEBUG_lineEdit1->text().toInt() / Mesh; //[DEBUG]
                // 本番時はスライダーの値を見るので削除 if( x >= 0 && x < nx && Yposdata >= 0 && Yposdata < ny && z >= 0
                // && z < nz ){
                //     b = g_voxDomain[x][Yposdata][z] % 20; //色定義関数　func_setBrushes()　で現在20色まであるため。
                //     int matID = g_voxDomain[x][Yposdata][z];
                //     if(g_VoxMatIDList.indexOf(QString::number(matID)) > -1){
                //         b = g_VoxMatIDList.indexOf(QString::number(matID))  % 20;
                //         //できるだけ全部違う色になるようにする if(b < 4){ b= b+ 15; }
                //         //[DEBUG]暫定　黒とか白は避けたい
                //     }
                // }
                // テーブルの色指定コンボから表示色を決定する。
                if (x >= 0 && x < nx && Yposdata >= 0 && Yposdata < ny && z >= 0 && z < nz) {
                    int matID = g_voxDomain[x][Yposdata][z];
                    if (matnumToColorHash.contains(matID)) {
                        QString colorname = matnumToColorHash.value(matID);
                        b                 = func_setBrushesNum(colorname);
                    }
                }
                // 四角形の描画
                if (b != -1) {
                    painter.fillRect(rect, Brushes[b]);
                    //[DEBUG]画面上にmatID表示→//painter.drawText(tmp_xposition, tmp_yposition + 10,
                    // QString::number(g_voxDomain[x][Yposdata][z])); //DEBUG
                }
            }
            //-end- if( tmp_xposition <= dX && tmp_yposition <= dZ && (tmp_xposition + tmp_xWidth > 0) )
            x = x + x2cnt;    // 連続値があった場合は、インデックススキップする。
        }
        //-end- for(int x = 0; x < nx; x++)
    }
    //-end- for(int z = 0; z < nz; z++
    //-end- vox描画箇所の設定

    // ビューへの描画
    item1 = new QGraphicsPixmapItem();
    // 上下反転
    QTransform matrix(1.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 1.0);
    item1->setTransform(matrix);
    item1->setPixmap(pix);
    XZscene->addItem(item1);

    pix_XZscene = pix;    // DEBUG 2018.06.xx-01 outputVolume機能　volume表示のため
}

void GeometryInputForm::func_vox_make_YZgeometry()    // viewerXZの描画 voxファイル 3D配列データをもとにする
{
    // loadボタンで別途読み込む(スライダーchangedなどでは読み込まない。 //func_vox_readTo3Darray(); //[DEBUG]

    // QMessageBox::information(this, "notice","[DEBUG] on_XZRedraw_XZ_pushButton_DEBUG1_clicked");
    int nx = ui->domainX_Value->text().toInt() / ui->meshsize_Value->text().toInt();
    int ny = ui->domainY_Value->text().toInt() / ui->meshsize_Value->text().toInt();
    int nz = ui->domainZ_Value->text().toInt() / ui->meshsize_Value->text().toInt();

    YZscene->clear();
    int dX   = (int)ui->domainX_Value->text().toDouble();     //[DEBUG]10000
    int dY   = (int)ui->domainY_Value->text().toDouble();     //[DEBUG]10000
    int dZ   = (int)ui->domainZ_Value->text().toDouble();     //[DEBUG]10000
    int Mesh = (int)ui->meshsize_Value->text().toDouble();    //[DEBUG]20
                                                              //    int mesh = Mesh;
    int     Xdata      = dX / Mesh;
    int     Ydata      = dY / Mesh;
    int     Zdata      = dZ / Mesh;
    int     XposSlider = ui->Xcoordinate_spin_lineEdit->text().toInt();
    int     Xpos       = ui->Xcoordinate_spin_lineEdit->text().toInt() - 1;
    int     Xposdata   = (int)((Xpos + (double)Mesh / 2 - 0.00001) / Mesh);    // 本番用
    QString tmp_tooltip;
    int     VoxUnitlength;
    QString tmpString;
    float   tmpFloat;

    // テーブルから、マテリアル番号から色を特定するハッシュの作成
    QHash<int, QString> matnumToColorHash;
    for (int row = 0; row < ui->tableWidget->rowCount(); row++) {
        int               matnum   = ui->tableWidget->item(row, 1)->text().toInt();
        CustomColorCombo* tmpcombo = static_cast<CustomColorCombo*>(ui->tableWidget->cellWidget(row, 3));
        QString           color    = tmpcombo->currentText();
        matnumToColorHash.insert(matnum, color);
    }

    // pixmapによる描画
    int     XWidthPixmap = dY;
    int     YWidthPixmap = dZ;
    QPixmap pix(XWidthPixmap,
                YWidthPixmap);    // マイナスは指定できない
                                  // 最終のitem->offset　有り無しいずれにしてもitem1->setOffset(0, -YWidthPixmap );
    QPainter painter(&pix);

    QBrush lightcyanBrush(QColor(224, 255, 255, 255));
    QPen   whitePen(QColor(255, 255, 255, 0));

    //-start- vox描画箇所の設定
    // int xposition_d = 0;
    // int yposition_d = 0;
    // int zposition_d = 0;
    // int xposition, yposition, zposition;
    // xposition = (int)((xposition_d + (double)Mesh / 2 - 0.00001) / (double)Mesh) * Mesh;
    // yposition = (int)((yposition_d + (double)Mesh / 2 - 0.00001) / (double)Mesh) * Mesh;
    // zposition = (int)((zposition_d + (double)Mesh / 2 - 0.00001) / (double)Mesh) * Mesh;

    if (Xposdata >= nx) {
        return;
    }
    for (int z = 0; z < nz; z++) {        // 高さ方向
        for (int x = 0; x < ny; x++) {    // 水平方向
            int x2cnt = 0;
            for (int x2 = x + 1; x2 < ny; x2++) {
                if (g_voxDomain[Xposdata][x2][z] == g_voxDomain[Xposdata][x2 - 1][z]) {
                    x2cnt++;
                }
                else {
                    break;
                }
            }
            // int tmp_xposition = xposition + (countx * Mesh); //現在の始点： オフセット開始座標X
            // ＋ここまでのvoxファイル内容
            int tmp_xposition = x * Mesh;    // xposition + x * Mesh;
            int tmp_yposition = z * Mesh;    // zposition + (z * Mesh);
            int tmp_xWidth    = Mesh * (x2cnt + 1);
            int tmp_yWidth    = Mesh;

            // ΔX範囲をはみ出す場合の対処
            if ((tmp_xposition + tmp_xWidth) > dY) {
                tmp_xWidth = dY - tmp_xposition;
            }
            // ΔZ範囲をはみ出す場合の対処
            if (tmp_yposition > dZ) {
                tmp_yWidth = dZ - tmp_yposition;
            }

            if (tmp_xposition <= dY && tmp_yposition <= dZ && (tmp_xposition + tmp_xWidth > 0)) {
                // pixmapによる描画
                QRect rect(tmp_xposition, tmp_yposition, tmp_xWidth, tmp_yWidth);
                int   matID = g_voxDomain[Xposdata][x][z];
                // QString tmpstr;
                // qDebug() << "[DEBUG]func_vox_make_YZgeometry() " << tmpstr.asprintf(" x=%d z=%d tmp_xposition=%d
                // tmp_xposition=%d tmp_xWidth=%d tmp_yWidth=%d\n", x, z, tmp_xposition, tmp_xposition, tmp_xWidth,
                // tmp_yWidth);

                // QString  matcolor = acceptedMaterialcolorList.at(0); //2018.02.14 Airの表示色も変えられるよう処理変更
                // int b = func_setBrushesNum(matcolor);
                // int b = 1; //暫定初期値 Gray
                // Xposdata = ui->XZ_DEBUG_lineEdit1->text().toInt() / Mesh; //[DEBUG]
                // 本番時はスライダーの値を見るので削除 if( x >= 0 && x < ny && Xposdata >= 0 && Xposdata < nx && z >= 0
                // && z < nz ){
                //     b = matID % 20; //色定義関数　func_setBrushes()　で現在20色まであるため。
                //     if(g_VoxMatIDList.indexOf(QString::number(matID)) > -1){
                //         int tmp_b = g_VoxMatIDList.indexOf(QString::number(matID))  % 20;
                //         //できるだけ全部違う色になるようにする if(tmp_b >= 0){ b = tmp_b; }
                //     }
                //     if(b < 4){ b=b+15; } //[DEBUG]暫定　黒とか白は避けたい
                // }
                int b = 1;    // 暫定初期値 Gray
                if (x >= 0 && x < ny && Xposdata >= 0 && Xposdata < nx && z >= 0 && z < nz) {
                    if (matnumToColorHash.contains(matID)) {
                        QString colorname = matnumToColorHash.value(matID);
                        b                 = func_setBrushesNum(colorname);
                    }
                }
                if (b != -1) {
                    painter.fillRect(rect, Brushes[b]);
                    //[DEBUG]画面上にmatID表示→//painter.drawText(tmp_xposition, tmp_yposition + 10,
                    // QString::number(matID)); //DEBUG
                }
            }
            //-end- if( tmp_xposition <= dX && tmp_yposition <= dZ && (tmp_xposition + tmp_xWidth > 0) )
            x = x + x2cnt;    // 連続値があった場合は、インデックススキップする。
        }
        //-end- for(int x = 0; x < nx; x++)
    }
    //-end- for(int z = 0; z < nz; z++
    //-end- vox描画箇所の設定

    // ビューへの描画
    item1 = new QGraphicsPixmapItem();
    // 上下反転
    QTransform matrix(1.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 1.0);
    item1->setTransform(matrix);
    item1->setPixmap(pix);
    YZscene->addItem(item1);

    pix_YZscene = pix;    // DEBUG 2018.06.xx-01 outputVolume機能　volume表示のため
}

void GeometryInputForm::func_vox_make_diagonal()
{    // 斜め断面表示 //現在未対応→描画の時は水平方向は 1メッシュ図形の水平方向幅　sqrt(meshsize)で描くことが必要
    // in_modeは,XZ45, XZm45のいずれか　(以前は"XZ", "YZ" のいずれかだったが変更。)
    QString in_mode = "XZ45";
    if (ui->tabWidget->currentIndex() == 3 && ui->XZdiag_comboBox_angle->currentText() == "-45") {
        in_mode = "XZm45";
    }

    //-start-  DEBUG
    // QStringList tmpList;
    // tmpList << "Red" << "Green" << "Blue" << "Yellow" << "Purple";
    // for(int row=1; row <= 5; row++){
    //    CustomColorCombo *tmpcombo = static_cast<CustomColorCombo*>(ui->tableWidget->cellWidget(row, 3));
    //    tmpcombo->setCurrentText(tmpList.at(row-1));
    //}
    //-end- DEBUG

    // voxファイルはloadボタンで別途読み込む(スライダーchangedなどでは読み込まない。//func_vox_readTo3Darray();
    // //[DEBUG]

    // QMessageBox::information(this, "notice","[DEBUG] on_XZRedraw_XZ_pushButton_DEBUG1_clicked");
    int nx = ui->domainX_Value->text().toInt() / ui->meshsize_Value->text().toInt();
    int ny = ui->domainY_Value->text().toInt() / ui->meshsize_Value->text().toInt();
    int nz = ui->domainZ_Value->text().toInt() / ui->meshsize_Value->text().toInt();

    if (in_mode == "XZ45" || in_mode == "XZm45") {
        XZdiag_scene->clear();
    }
    int dX   = (int)ui->domainX_Value->text().toDouble();     //[DEBUG]10000
    int dY   = (int)ui->domainY_Value->text().toDouble();     //[DEBUG]10000
    int dZ   = (int)ui->domainZ_Value->text().toDouble();     //[DEBUG]10000
    int Mesh = (int)ui->meshsize_Value->text().toDouble();    //[DEBUG]20
                                                              //    int mesh = Mesh;
    int Xdata = dX / Mesh;
    int Ydata = dY / Mesh;
    int Zdata = dZ / Mesh;
    // XZビューの場合
    // int YposSlider = ui->XZ_DEBUG_lineEdit1->text().toInt();
    // int Ypos = ui->XZ_DEBUG_lineEdit1->text().toInt() - 1;
    // int Yposdata = (int)((Ypos + (double)Mesh / 2 - 0.00001) / Mesh); //本番用
    // YZビューの場合
    // int XposSlider = ui->YZ_DEBUG_lineEdit1->text().toInt();
    // int Xpos = ui->YZ_DEBUG_lineEdit1->text().toInt() - 1;
    // int Xposdata = (int)((Xpos + (double)Mesh / 2 - 0.00001) / Mesh); //本番用

    QString tmp_tooltip;
    int     VoxUnitlength;
    QString tmpString;
    float   tmpFloat;

    // テーブルマテリアル番号から色を特定するハッシュの作成
    QHash<int, QString> matnumToColorHash;
    for (int row = 0; row < ui->tableWidget->rowCount(); row++) {
        int               matnum   = ui->tableWidget->item(row, 1)->text().toInt();
        CustomColorCombo* tmpcombo = static_cast<CustomColorCombo*>(ui->tableWidget->cellWidget(row, 3));
        QString           color    = tmpcombo->currentText();
        matnumToColorHash.insert(matnum, color);
    }

    // pixmapによる描画設定
    // int XWidthPixmap = dX;
    // int YWidthPixmap = dZ;
    // QPixmap pix(XWidthPixmap, YWidthPixmap); //マイナスは指定できない
    // 最終のitem->offset　有り無しいずれにしてもitem1->setOffset(0, -YWidthPixmap ); QPainter painter(&pix);

    QBrush lightcyanBrush(QColor(224, 255, 255, 255));
    QPen   whitePen(QColor(255, 255, 255, 0));

    // nx, ny 違う場合は大きい方でとって、ない部分はAirとして描く
    int viewMaxXY = nx;    // ビュー水平方向horizontal
    if (ny > nx) {
        viewMaxXY = ny;
    }
    // pixmapによる描画設定
    int     XWidthPixmap = int(ceil(viewMaxXY * Mesh * sqrt(2)));    // 通常、平面表示の時は =dX;
    int     YWidthPixmap = dZ;
    QPixmap pix(XWidthPixmap,
                YWidthPixmap);    // マイナスは指定できない
                                  // 最終のitem->offset　有り無しいずれにしてもitem1->setOffset(0, -YWidthPixmap );
    QPainter painter(&pix);
    //-end- pixmapによる描画設定

    QStringList DEBUG_nanameStrList;
    // int x_index_start = 0; //データ箇所(x,y,z)=(0,0,z)起点として45°断面
    // int x_index_start = 1 ; //(x,y,z)=(1,0,z)起点として45°断面
    int x_index_start = ui->XZdiag_Ycoordinate_spin_lineEdit->text().toInt()
                      / Mesh;    // XZビューの場合　本番時はスライダーの値 x方向の起点座標
    // int y_index_start = ui->YZdiag_Xcoordinate_spin_lineEdit->text().toInt() / Mesh;  //YZビューの場合

    // 初期化：g_XZdiagNow2d, g_YZdiagNow2d → 斜め断面ShapeInfoなどで使用するため、現在の1面表示データを保持しておく
    if (in_mode == "XZ45" || in_mode == "XZm45") {
        for (int tmpZ = 0; tmpZ < nz; tmpZ++)
            for (int tmpX = 0; tmpX < g_nxdiag; tmpX++) {    // 初期化　配列値全て-1(=描かない）にする
                g_XZdiagNow2d[tmpX][tmpZ] = -1;
            }
    }

    for (int z = 0; z < nz; z++) {    // 3次元配列としてのZ
        QString    tmpstr = "";
        QList<int> onelineMatList;

        if (in_mode == "XZ45") {    // XZ描画 横1行分を取得(onelineMatList)
            //
            //
            // ←ユーザー操作：スライダ 0 ～ -dx で斜め線を左にスライドさせていく(左上半分の描画)
            //            ________
            //    左上半分| 　    ／|
            //           |　　  ／  |  dy
            //           |　　／　　  |  ↑　　右下半分
            //           :斜め線を右上にスライドさせていく　（ユーザー操作：スライダー0　から dxへ移動の時） |／______|
            //           0
            //          ★0 スライダー値0 = 斜め線の左下。
            //            →ユーザー操作：スライダ 0 ～　dx で斜め線を右にスライドさせていく(右下半分の描画)
            //
            //            　　↑
            //         　　 ('_') 視点
            //
            //--------------
            // 描画データ設定　45°斜め　視点から左上半分。 x起点　0～max。 XZビューの場合
            // x_index_startはY:0～dyのどこを起点とするかを表す。
            if (x_index_start >= 0) {
                int y_index = -1;
                for (int x_index = x_index_start; x_index < viewMaxXY + x_index_start; x_index++) {
                    y_index++;    // 3次元配列で参照するｙ位置のデータを使う
                    if (x_index >= 0 && x_index < nx && y_index >= 0 && y_index < ny) {
                        onelineMatList << g_voxDomain[x_index][y_index][z];
                        tmpstr = tmpstr + " "
                               + QString::number(
                                     g_voxDomain[x_index][y_index][z]);    // ビューでは、データy=x+1の箇所を書き出す
                    }
                    else {
                        onelineMatList << -1;    // 描かない場所を -1 とする
                        tmpstr = tmpstr + " -1";
                    }
                }
            }
            // 描画データ設定　45°　視点から右下斜め半分
            if (x_index_start < 0) {
                for (int x_index = 0; x_index < viewMaxXY; x_index++) {
                    int y_index = x_index - x_index_start;    // 3次元配列で参照するｙ位置のデータを使う
                    if (x_index >= 0 && x_index < nx && y_index >= 0 && y_index < ny) {
                        onelineMatList << g_voxDomain[x_index][y_index][z];
                        tmpstr = tmpstr + " "
                               + QString::number(
                                     g_voxDomain[x_index][y_index][z]);    // ビューでは、データy=x+1の箇所を書き出す
                    }
                    else {
                        onelineMatList << -1;    // 描かない場所を -1 とする
                        tmpstr = tmpstr + " -1";
                    }
                }
            }
            DEBUG_nanameStrList << tmpstr;
        }

        if (in_mode == "XZm45") {    // XZ描画 横1行分を取得(onelineMatList)
            //
            //         　            　　　　　　　　→ユーザー操作：スライダ(x_index_start) 0 ～ dx
            //         で斜め線を右にスライドさせていく(右上半分の描画)
            //                      _______
            // 　　　　　　　　　　　　　　　　　　|＼ 　    |右上半分
            //                     |　　＼    |
            //                     |　   ＼  |
            //            　　左下半分|______＼|
            // 　　　　　　　　　　　　　　　　　           ★0 スライダー値0 = 斜め線の右下。
            //
            // ←ユーザー操作：スライダ 0 ～ -dx で斜め線を左にスライドさせていく(左下半分の描画)
            //
            //                         ↑
            //                        ('_') 視点
            //
            //--------------
            // 描画データ設定　-45°斜め右上半分 x起点　0～max
            if (x_index_start
                >= 0) {    // 斜めXZdiagでは　x_index_startは、X=0～dxのどこを起点とするかを表す。（GUIの名前はspinの名前にYが入っていて整合が取れていないが。。）(XZビューとXZdiagでは表す軸が別）
                qDebug() << "[DEBUG]01 Geo.cpp-func_vox_make_diagonal XZm45 plus x_index_start="
                         << QString::number(x_index_start);
                int y_index = nx + x_index_start;
                for (int x_index = 0; x_index < viewMaxXY; x_index++) {
                    y_index--;    // 3次元配列で参照するｙ位置のデータを使う
                    if (x_index >= 0 && x_index < nx && y_index >= 0 && y_index < ny) {
                        onelineMatList << g_voxDomain[x_index][y_index][z];
                        tmpstr = tmpstr + " "
                               + QString::number(
                                     g_voxDomain[x_index][y_index][z]);    // ビューでは、データy=x+1の箇所を書き出す
                    }
                    else {
                        onelineMatList << -1;    // 描かない場所を -1 とする
                        tmpstr = tmpstr + " -1";
                    }
                }
            }
            // 描画データ設定　-45°左下半分
            if (x_index_start < 0) {
                int y_index = nx - abs(x_index_start);
                for (int x_index = 0; x_index < nx; x_index++) {
                    y_index--;    // 3次元配列で参照するｙ位置のデータを使う
                    if (x_index >= 0 && x_index < nx && y_index >= 0 && y_index < ny) {
                        onelineMatList << g_voxDomain[x_index][y_index][z];
                        tmpstr = tmpstr + " "
                               + QString::number(
                                     g_voxDomain[x_index][y_index][z]);    // ビューでは、データy=x+1の箇所を書き出す
                    }
                    else {
                        onelineMatList << -1;    // 描かない場所を -1 とする
                        tmpstr = tmpstr + " -1";
                    }
                }
            }
            DEBUG_nanameStrList << tmpstr;
        }

        // qDebug() << "[DEBUG]Geo.cpp-func_vox_make_diagonal() onelineMatList" << onelineMatList;
        // 横1行分のデータを、ビューに描く複数の四角形としてpainterに登録
        for (int x = 0; x < onelineMatList.size(); x++) {
            int x2cnt = 0;
            for (int x2 = x + 1; x2 < onelineMatList.size(); x2++) {
                if (onelineMatList.at(x2) == onelineMatList.at(x2 - 1)) {
                    x2cnt++;
                }
                else {
                    break;
                }
            }

            // int tmp_xposition = x * Mesh; //ビューの描画位置
            float tmp_xposition = x * Mesh * sqrt(2);
            int   tmp_yposition = z * Mesh;
            // int tmp_xWidth = Mesh;
            // int tmp_xWidth = Mesh * (x2cnt + 1);
            float tmp_xWidth = Mesh * (x2cnt + 1) * sqrt(2);
            int   tmp_yWidth = Mesh;    // ルート2掛けるのは斜めX方向のみ
            int   matID      = onelineMatList.at(x);

            // ΔX範囲をはみ出す場合の対処
            if ((tmp_xposition + tmp_xWidth) > XWidthPixmap) {
                tmp_xWidth = XWidthPixmap - tmp_xposition;
            }
            // ΔZ範囲をはみ出す場合の対処
            if (tmp_yposition + tmp_yWidth > YWidthPixmap) {
                tmp_yWidth = YWidthPixmap - tmp_yposition;
            }

            // if( tmp_xposition <= XWidthPixmap && tmp_yposition <= YWidthPixmap  && (tmp_xposition + tmp_xWidth > 0)
            // ){
            if (tmp_xposition <= XWidthPixmap && tmp_yposition <= YWidthPixmap) {
                // pixmapによる描画
                // org//QRect rect(tmp_xposition, tmp_yposition, tmp_xWidth, tmp_yWidth); //QRectは整数しか扱えない。
                QRectF rect(tmp_xposition, tmp_yposition, tmp_xWidth, tmp_yWidth);    // 斜め断面のため QRectFに変更

                QString tmpstr;

                // QString  matcolor = acceptedMaterialcolorList.at(0); //2018.02.14 Airの表示色も変えられるよう処理変更
                // int b = func_setBrushesNum(matcolor);
                // int b = 1; //暫定初期値 Gray
                // int Yposdata = ui->XZ_DEBUG_lineEdit1->text().toInt() / Mesh; //[DEBUG]
                // 本番時はスライダーの値を見るので削除 if( x >= 0 && x < viewMaxXY &&  Yposdata >= 0 && Yposdata < ny
                // && tmp_yposition >= 0 && z < viewMaxXY ){ b = matID % 20;
                // //色定義関数　func_setBrushes()　で現在20色まであるため。
                // if(g_VoxMatIDList.indexOf(QString::number(matID)) > -1){
                //     b = g_VoxMatIDList.indexOf(QString::number(matID))  % 20;  //できるだけ全部違う色になるようにする
                // }
                // if(b < 4){ b= b+ 15; } //[DEBUG]暫定　黒とか白は避けたい
                // if(matID == 0){ b = 2; } //[DEBUG]暫定　Airの時はDarkgray (背景色と同じにする）
                //}
                // start- OK
                int b = 1;    // 暫定初期値 Gray
                // テーブルの色指定コンボから表示色を決定する
                //                    if(matnumToColorHash.contains(matID)){
                //                        QString colorname = matnumToColorHash.value(matID);
                //                        b = func_setBrushesNum(colorname);
                //                    }
                //                    if( b != -1 ){
                //                        painter.fillRect(rect, Brushes[b]);
                //                        //[DEBUG]画面上にmatID表示→//painter.drawText(tmp_xposition, tmp_yposition +
                //                        10, QString::number(matID)); //DEBUG
                //                        //qDebug() << "[DEBUG]02func_vox_make_geometry() " <<
                //                        tmpstr.asprintf("matID=%d x=%d z=%d tmp_xposition=%d tmp_yposition=%d
                //                        color:b=%d\n", matID, x, z, tmp_xposition, tmp_yposition , b);
                //                    }
                //-end- OK

                if (matID == -1) {    // 描かない箇所は背景色と同じにする
                    QBrush darkgrayBrush(QColor(46, 46, 46, 255));
                    painter.fillRect(rect, darkgrayBrush);
                }
                else {
                    int b = 1;    // 暫定初期値 Gray
                    // テーブルの色指定コンボから表示色を決定する
                    if (matnumToColorHash.contains(matID)) {
                        QString colorname = matnumToColorHash.value(matID);
                        b                 = func_setBrushesNum(colorname);
                    }
                    if (b != -1) {
                        painter.fillRect(rect, Brushes[b]);
                        //[DEBUG]画面上にmatID表示→//painter.drawText(tmp_xposition, tmp_yposition + 10,
                        // QString::number(matID)); //DEBUG qDebug() << "[DEBUG]02func_vox_make_geometry() " <<
                        // tmpstr.asprintf("matID=%d x=%d z=%d tmp_xposition=%d tmp_yposition=%d color:b=%d\n", matID,
                        // x, z, tmp_xposition, tmp_yposition , b);
                    }
                }
            }
            //-end- if( tmp_xposition <= XWidthPixmap && tmp_yposition <= YWidthPixmap )

            x = x + x2cnt;    // 連続値があった場合は、インデックススキップする。
        }
        //-end- for(int x=0; x<onelineMatList.size(); x++)

        for (int x = 0; x < onelineMatList.size(); x++) {
            if (in_mode == "XZ45" || in_mode == "XZm45") {
                g_XZdiagNow2d[x][z] = onelineMatList.at(x);
            }
        }
    }
    //-end- for(int z = 0; z < nz; z++)

    //-satrt- [DEBUG]斜め断面のデータ表示
    // qDebug() << "\n[DEBUG]Geo.cpp-func_vox_make_diagonal()  x-start=" << QString::number(x_index_start)  <<
    // "bottom-left (0,0) top-right=(xmax, ymax)\n"; for(int i = DEBUG_nanameStrList.size()-1; i >=0 ; i--){ qDebug() <<
    // DEBUG_nanameStrList.at(i) ;  } //[DEBUG]斜め断面のデータ表示 -end- [DEBUG]斜め断面のデータ表示

    //-end- 斜め断面　描画箇所設定

    // ビューへの描画反映　　上記までの処理painterに登録したデータを、ビューに描画する

    item1 = new QGraphicsPixmapItem();
    // 上下反転
    QTransform matrix(1.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 1.0);
    item1->setTransform(matrix);
    item1->setPixmap(pix);

    if (in_mode == "XZ45" || in_mode == "XZm45") {
        XZdiag_scene->addItem(item1);
        pix_XZdiag_scene = pix;
    }

    //[DEBUG]デバッグ表示
    //    if(in_mode == "XZ45"){
    //        for(int tmpZ=0; tmpZ < nz; tmpZ++){
    //            QString tmpstr="";
    //            for(int tmpX=0; tmpX<g_nxdiag; tmpX++){
    //                tmpstr  = tmpstr +  " " + QString::number(g_XZdiagNow2d[tmpX][tmpZ]);
    //            }
    //            qDebug() << tmpstr;
    //        }
    //    }
}

// -start- 2021.06.xx vox-2Dviewer
void GeometryInputForm::on_vox_load_pushButton_clicked()
{
    // voxファイル　ユーザー選択
    // 既にloadファイルされている場合、そのファイルがあるフォルダをファイル選択ダイアログの起点フォルダとする
    QString tmpDirPath = "c:/";
    if (ui->vox_path_lineEdit->text().isEmpty() == 0) {
        QString tmpdir = QFileInfo(QFile(ui->vox_path_lineEdit->text())).absolutePath();
        if (QDir(tmpdir).exists()) {
            tmpDirPath = QFileInfo(ui->vox_path_lineEdit->text()).absolutePath();
        }
    }
    // voxファイル　ユーザー選択
    QString filePath = "";
    // filePath = QFileDialog::getOpenFileName(this, tr("Select file"), "Desktop", "*");
    filePath = QFileDialog::getOpenFileName(this, tr("Select file"), tmpDirPath, tr("vox Files (*.vox)"));
    if (filePath.isEmpty()) {
        return;
    }    // ファイル選択ダイアログで、ユーザーがキャンセルボタンを押したときは、ここで処理終了する。

    ui->vox_path_lineEdit->setText(filePath);
    // GE_voxfilepath = filePath; //2022.04.27 今時点では 2Dviewer
    // 3Dviewerで異なるvoxfileも読むこと想定して、共通変数GE_には代入しない。

    if (!QFileInfo(QFile(filePath)).exists()) {
        QMessageBox::information(this, "notice", "Error: no-file.");
        return;
    }
    else {
        // voxファイルが存在する場合は読み込み、表示する。
        func_GUI_defaultSet();      // GUI、グローバル変数を初期状態にする
        func_vox_load(filePath);    // voxファイルを読込み　表示する
    }
}

void GeometryInputForm::func_vox_load(QString in_voxfilepath)
{
    ui->tabWidget->setVisible(true);

    if (!QFileInfo(QFile(in_voxfilepath)).exists()) {
        QMessageBox::information(this, "notice", "Error: no-file.");
        return;
    }
    else {
        // voxファイルが存在する場合は読み込み、表示する。
        // マテリアルテーブルへのセット
        g_flag_slotMatColorCombo =
            0;    // シグナルスロットの処理が1つごとに入ると処理が重くなってしまうため、ここでは止める。
        QStringList lineList = func_getMaterialfromVoxPath(in_voxfilepath);
        func_tableMaterialDefaultSet(lineList);    // materialテーブルへの反映
        g_flag_slotMatColorCombo =
            1;    // シグナルスロットの処理が通常通りされるように戻す。（ユーザー操作で色切り替えができるようにする）

        // func_vox_readHeader(in_voxfilepath); //voxファイルヘッダー読み込み domainX,Y,Z meshなど
        func_vox_readTo3Darray();    // voxファイルの3次元配列への格納・GUIフォーム domain,
                                     // Mesh設定。　ビューへの表示はまだしない。
        if (ui->tabWidget->currentIndex() == 0) {
            func_vox_make_XZgeometry();           // ビュー描画
            on_XZzoomall_pushButton_clicked();    // 画面zoolAllして図形全体が見えるようにする
        }
        if (ui->tabWidget->currentIndex() == 1) {
            func_vox_make_YZgeometry();
            on_YZzoomall_pushButton_clicked();
        }
        if (ui->tabWidget->currentIndex() == 2) {
            func_vox_make_XYgeometry();
            on_XYzoomall_pushButton_clicked();
        }
        if (ui->tabWidget->currentIndex() == 3) {
            func_vox_make_diagonal();
            on_XZdiag_zoomall_pushButton_clicked();
        }
        // 読込時はXZ面のみ描画するので, XZのみ。
    }
}

void GeometryInputForm::
    func_tabWidget_currentChanged()    // XY-YZ-XY タブが切り替わった時に呼び出される関数。ビューアー更新する。
{
    func_cellvalueChanged_setAutoLength();    // GUI入力値変更時の処理(セルサイズ、アレイ数)　自動測長(AutoLength)GUIをセットする

    FLAG_VIEWER_UPDATE = 1;
    viewer_update();
}

// マウスクリックされた場合の処理
void GeometryInputForm::receiveCoord(float X_IN_f, float Y_IN_f)
{
    int X_IN = int(X_IN_f);    // 整数化
    int Y_IN = int(Y_IN_f);

    // QString in_string = QString::number(X_IN) + ", " + QString::number(Y_IN);
    //  座標取得：以下処理にて共通で使用する　座標はメッシュ単位で表示する
    int tmp_x, tmp_y, meshsize;
    // meshsize = ui->meshsize_Value->text().toInt();
    meshsize = ui->meshsize_Value->text().toInt();
    //-start- 2018.09.21 メッシュ丸め変更
    // tmp_x = X_IN - (X_IN % meshsize);
    // tmp_y = Y_IN - (Y_IN % meshsize);
    if (X_IN >= 0) {
        tmp_x = (int)(((double)X_IN + (double)meshsize / 2 - 0.00001) / (double)meshsize) * meshsize;
    }
    else if (X_IN < 0) {
        tmp_x = (int)(((double)X_IN - (double)meshsize / 2 + 0.00001) / (double)meshsize) * meshsize;
    }
    if (Y_IN >= 0) {
        tmp_y = (int)(((double)Y_IN + (double)meshsize / 2 - 0.00001) / (double)meshsize) * meshsize;
    }
    else if (Y_IN < 0) {
        tmp_y = (int)(((double)Y_IN - (double)meshsize / 2 + 0.00001) / (double)meshsize) * meshsize;
    }
    //-end- 2018.09.21 メッシュ丸め変更

    //-start- 2018.01.28
    int x_meshAjust = tmp_x;
    int y_meshAjust = tmp_y;
    //-end- 2018.01.28

    // Nop処理　クリックされた座標を表示する
    if ((ui->tabWidget->currentIndex() == 0 && ui->XZ_comboBox->currentText() == "Nop")
        || (ui->tabWidget->currentIndex() == 1 && ui->YZ_comboBox->currentText() == "Nop")
        || (ui->tabWidget->currentIndex() == 2 && ui->XY_comboBox->currentText() == "Nop")
        || (ui->tabWidget->currentIndex() == 3 && ui->XZdiag_comboBox->currentText() == "Nop")) {
        QString in_string = "Coordinate: (" + QString::number(tmp_x) + ", " + QString::number(tmp_y) + ")";
        if (ui->tabWidget->currentIndex() == 0) {
            ui->XZinfo_label->setText(in_string);
        }
        if (ui->tabWidget->currentIndex() == 1) {
            ui->YZinfo_label->setText(in_string);
        }
        if (ui->tabWidget->currentIndex() == 2) {
            ui->XYinfo_label->setText(in_string);
        }
        if (ui->tabWidget->currentIndex() == 3) {
            ui->XZdiag_info_label->setText(in_string);
        }
    }

    //    // Polygonの”Click Vertex!!”中は処理をキャンセル
    //    QString shape = "";
    //    QModelIndexList selection = ui->tableWidget->selectionModel()->selectedRows();
    //    if(selection.size() != 0){
    //        shape = ui->tableWidget->item(ui->tableWidget->currentRow(), 0)->text();
    //    }

    //    QString obname = "";
    //    selection = ui->Object_tableWidget->selectionModel()->selectedRows();
    //    if(selection.size() != 0){
    //        obname = ui->Object_tableWidget->item(ui->Object_tableWidget->currentRow(), 1)->text();
    //    }

    //    QString bd_shape = "";
    //    selection = ui->bd_tableWidget->selectionModel()->selectedRows();
    //    if(selection.size() != 0){
    //        bd_shape = ui->bd_tableWidget->item(ui->bd_tableWidget->currentRow(), 0)->text();
    //    }

    if (((ui->tabWidget->currentIndex() == 0 && ui->XZ_comboBox->currentText() == "Zoom_box")
         || (ui->tabWidget->currentIndex() == 1 && ui->YZ_comboBox->currentText() == "Zoom_box")
         || (ui->tabWidget->currentIndex() == 2 && ui->XY_comboBox->currentText() == "Zoom_box")
         || (ui->tabWidget->currentIndex() == 3 && ui->XZdiag_comboBox->currentText() == "Zoom_box"))
        && g_flag_keyCTRL != 1) {
        //        if((shape.startsWith("Polygon") && (ui->VoP_AddwithMouse_pushButton->text() == " Click Vertex !! "))
        //        ||
        //           (obname.startsWith("POL") && (ui->ob_VoP_AddwithMouse_pushButton->text() == " Click Vertex !! "))
        //           || (bd_shape.startsWith("Polygon") && (ui->bd_VoP_AddwithMouse_pushButton->text() == " Click Vertex
        //           !! ")) || (obname.startsWith("PD") &&
        //           ui->PD_RDTI_Addline_Mouse_label->text().endsWith(TR("XYplane上で始点/終点をClickして下さい。"))) ){
        //            FLAG_ZOOM = 0;
        //        } else if ((ui->tabWidget->currentIndex() == 0  && ui->XZlength_checkBox->isChecked()) ||
        if ((ui->tabWidget->currentIndex() == 0 && ui->XZlength_checkBox->isChecked())
            || (ui->tabWidget->currentIndex() == 1 && ui->YZlength_checkBox->isChecked())
            || (ui->tabWidget->currentIndex() == 2 && ui->XYlength_checkBox->isChecked())
            || (ui->tabWidget->currentIndex() == 3 && ui->XZdiag_length_checkBox->isChecked())
            || (ui->tabWidget->currentIndex() == 0 && ui->XZlength_checkBox->text() == "Length_on")
            || (ui->tabWidget->currentIndex() == 1 && ui->YZlength_checkBox->text() == "Length_on")
            || (ui->tabWidget->currentIndex() == 2 && ui->XYlength_checkBox->text() == "Length_on")
            || (ui->tabWidget->currentIndex() == 3 && ui->XZdiag_length_checkBox->text() == "Length_on")) {
            // 2020.03.09 zoomBox・Length同時使用(バインドキー限定）
            // ①LengthチェックONの場合は、ZoomBoxしない。　②ラベルがLength_onの時はZoomBoxしない。　③ラベルがLength_onP1の時はZoomBoxする
            FLAG_ZOOM = 0;
        }
        else {
            // 実行：　ZoomBox処理
            func_view_Zoom_box(X_IN, Y_IN);
        }
    }
    else {
        FLAG_ZOOM = 0;
    }

    // Length(測長)処理
    // 2020.03.09 zoomBox・Length同時使用(バインドキー限定）
    // ユーザー操作：　バインドキー押しつつ1点目クリック　→ zoomBox　→　バインドキー押しつつ2点目クリック
    // LengthCheckBox ON・OFF時点で ラベルはLengthにリセットされる
    // (LengthがzoomBoxより優先　バインドキーLength_onP1より優先)
    // 　Lengthバインドキー押下している間だけ　ラベルはLength_on になる (Length 1, 2点目押下)
    // 　Lengthバインドキー押下して1点目クリック後　ラベルはLength_onP1　になる
    // (Lenghth_on:Zoomboxしない　Length_onP1:zoomboxする)
    if (((ui->tabWidget->currentIndex() == 0 && ui->XZlength_checkBox->isChecked())
         || (ui->tabWidget->currentIndex() == 1 && ui->YZlength_checkBox->isChecked())
         || (ui->tabWidget->currentIndex() == 2 && ui->XYlength_checkBox->isChecked())
         || (ui->tabWidget->currentIndex() == 3 && ui->XZdiag_length_checkBox->isChecked())
         || (ui->tabWidget->currentIndex() == 0 && ui->XZlength_checkBox->text() == "Length_on")
         || (ui->tabWidget->currentIndex() == 1 && ui->YZlength_checkBox->text() == "Length_on")
         || (ui->tabWidget->currentIndex() == 2 && ui->XYlength_checkBox->text() == "Length_on")
         || (ui->tabWidget->currentIndex() == 3 && ui->XZdiag_length_checkBox->text() == "Length_on"))
        && (g_flag_keyCTRL != 1)) {
        if ((ui->tabWidget->currentIndex() == 0 && ui->XZlength_checkBox->isChecked())
            || (ui->tabWidget->currentIndex() == 0 && ui->XZlength_checkBox->text() == "Length_on")) {
            lengthXZ_X << x_meshAjust;
            lengthXZ_Y << y_meshAjust;

            // 最後のPointが2点目なら、Diagonal/Vertival/Horizontalに応じて値を調整
            if (lengthXZ_X.size() % 2 == 0 && ui->XZ_comboBox_Length->currentText() != "AnyAngle") {
                if (ui->XZ_comboBox_Length->currentText() == "Diagonal") {
                    int x_width = abs(lengthXZ_X.at(lengthXZ_X.size() - 1) - lengthXZ_X.at(lengthXZ_X.size() - 2));
                    int y_width = abs(lengthXZ_Y.at(lengthXZ_Y.size() - 1) - lengthXZ_Y.at(lengthXZ_Y.size() - 2));
                    if (x_width > y_width) {
                        // X変更なし、Y位置を始点から45°の位置
                        int minusPlus = 1;
                        // if(-lengthXZ_Y[lengthXZ_Y.size() - 2] > -lengthXZ_Y[lengthXZ_Y.size() - 1]){ minusPlus = -1;}
                        // //符号が逆！
                        if (-lengthXZ_Y[lengthXZ_Y.size() - 2] < -lengthXZ_Y[lengthXZ_Y.size() - 1]) {
                            minusPlus = -1;
                        }
                        lengthXZ_Y[lengthXZ_Y.size() - 1] = lengthXZ_Y[lengthXZ_Y.size() - 2] + (x_width * minusPlus);
                    }
                    else {
                        // Y変更なし、X位置を始点から45°の位置
                        int minusPlus = 1;
                        if (lengthXZ_X[lengthXZ_X.size() - 2] > lengthXZ_X[lengthXZ_X.size() - 1]) {
                            minusPlus = -1;
                        }

                        lengthXZ_X[lengthXZ_X.size() - 1] = lengthXZ_X[lengthXZ_X.size() - 2] + (y_width * minusPlus);
                    }
                }
                else if (ui->XZ_comboBox_Length->currentText() == "Horizontal") {
                    lengthXZ_Y[lengthXZ_Y.size() - 1] = lengthXZ_Y[lengthXZ_Y.size() - 2];
                }
                else if (ui->XZ_comboBox_Length->currentText() == "Vertical") {
                    lengthXZ_X[lengthXZ_Y.size() - 1] = lengthXZ_X[lengthXZ_Y.size() - 2];
                }
            }

            on_XZRedraw_pushButton_clicked();
        }
        else if ((ui->tabWidget->currentIndex() == 1 && ui->YZlength_checkBox->isChecked())
                 || (ui->tabWidget->currentIndex() == 1 && ui->YZlength_checkBox->text() == "Length_on")) {
            lengthYZ_X << x_meshAjust;
            lengthYZ_Y << y_meshAjust;

            // 最後のPointが2点目なら、Diagonal/Vertival/Horizontalに応じて値を調整
            if (lengthYZ_X.size() % 2 == 0 && ui->YZ_comboBox_Length->currentText() != "AnyAngle") {
                if (ui->YZ_comboBox_Length->currentText() == "Diagonal") {
                    int x_width = abs(lengthYZ_X.at(lengthYZ_X.size() - 1) - lengthYZ_X.at(lengthYZ_X.size() - 2));
                    int y_width = abs(lengthYZ_Y.at(lengthYZ_Y.size() - 1) - lengthYZ_Y.at(lengthYZ_Y.size() - 2));
                    if (x_width > y_width) {
                        // X変更なし、Y位置を始点から45°の位置
                        int minusPlus = 1;
                        // if(-lengthYZ_Y[lengthYZ_Y.size() - 2] > -lengthYZ_Y[lengthYZ_Y.size() - 1]){ minusPlus = -1;}
                        if (-lengthYZ_Y[lengthYZ_Y.size() - 2] < -lengthYZ_Y[lengthYZ_Y.size() - 1]) {
                            minusPlus = -1;
                        }
                        lengthYZ_Y[lengthYZ_Y.size() - 1] = lengthYZ_Y[lengthYZ_Y.size() - 2] + (x_width * minusPlus);
                    }
                    else {
                        // Y変更なし、X位置を始点から45°の位置
                        int minusPlus = 1;
                        if (lengthYZ_X.at(lengthYZ_X.size() - 2) > lengthYZ_X.at(lengthYZ_X.size() - 1)) {
                            minusPlus = -1;
                        }
                        lengthYZ_X[lengthYZ_X.size() - 1] =
                            lengthYZ_X.at(lengthYZ_X.size() - 2) + (y_width * minusPlus);
                    }
                }
                else if (ui->YZ_comboBox_Length->currentText() == "Horizontal") {
                    lengthYZ_Y[lengthYZ_Y.size() - 1] = lengthYZ_Y[lengthYZ_Y.size() - 2];
                }
                else if (ui->YZ_comboBox_Length->currentText() == "Vertical") {
                    lengthYZ_X[lengthYZ_Y.size() - 1] = lengthYZ_X[lengthYZ_Y.size() - 2];
                }
            }

            on_YZRedraw_pushButton_clicked();
        }
        else if ((ui->tabWidget->currentIndex() == 2 && ui->XYlength_checkBox->isChecked())
                 || (ui->tabWidget->currentIndex() == 2 && ui->XYlength_checkBox->text() == "Length_on")) {
            lengthXY_X << x_meshAjust;
            lengthXY_Y << y_meshAjust;

            // 最後のPointが2点目なら、Diagonal/Vertival/Horizontalに応じて値を調整
            if (lengthXY_X.size() % 2 == 0 && ui->XY_comboBox_Length->currentText() != "AnyAngle") {
                if (ui->XY_comboBox_Length->currentText() == "Diagonal") {
                    int x_width = abs(lengthXY_X.at(lengthXY_X.size() - 1) - lengthXY_X.at(lengthXY_X.size() - 2));
                    int y_width = abs(lengthXY_Y.at(lengthXY_Y.size() - 1) - lengthXY_Y.at(lengthXY_Y.size() - 2));
                    if (x_width > y_width) {
                        // X変更なし、Y位置を始点から45°の位置
                        int minusPlus = 1;
                        // if(-lengthXY_Y[lengthXY_Y.size() - 2] > -lengthXY_Y[lengthXY_Y.size() - 1]){ minusPlus = -1;}
                        if (-lengthXY_Y[lengthXY_Y.size() - 2] < -lengthXY_Y[lengthXY_Y.size() - 1]) {
                            minusPlus = -1;
                        }
                        lengthXY_Y[lengthXY_Y.size() - 1] = lengthXY_Y[lengthXY_Y.size() - 2] + (x_width * minusPlus);
                    }
                    else {
                        // Y変更なし、X位置を始点から45°の位置
                        int minusPlus = 1;
                        if (lengthXY_X.at(lengthXY_X.size() - 2) > lengthXY_X.at(lengthXY_X.size() - 1)) {
                            minusPlus = -1;
                        }
                        lengthXY_X[lengthXY_X.size() - 1] =
                            lengthXY_X.at(lengthXY_X.size() - 2) + (y_width * minusPlus);
                    }
                }
                else if (ui->XY_comboBox_Length->currentText() == "Horizontal") {
                    lengthXY_Y[lengthXY_Y.size() - 1] = lengthXY_Y[lengthXY_Y.size() - 2];
                }
                else if (ui->XY_comboBox_Length->currentText() == "Vertical") {
                    lengthXY_X[lengthXY_Y.size() - 1] = lengthXY_X[lengthXY_Y.size() - 2];
                }
            }

            on_XYRedraw_pushButton_clicked();
        }
        else if ((ui->tabWidget->currentIndex() == 3 && ui->XZdiag_length_checkBox->isChecked())
                 || (ui->tabWidget->currentIndex() == 3 && ui->XZdiag_length_checkBox->text() == "Length_on")) {
            // lengthXZdiag_X << x_meshAjust;
            // lengthXZdiag_Y << y_meshAjust;
            lengthXZdiag_X << X_IN_f;
            lengthXZdiag_Y << Y_IN_f;

            // 最後のPointが2点目なら、Diagonal/Vertival/Horizontalに応じて値を調整
            if (lengthXZdiag_X.size() % 2 == 0 && ui->XZdiag_comboBox_Length->currentText() != "AnyAngle") {
                if (ui->XZdiag_comboBox_Length->currentText() == "Diagonal") {
                    float x_width = abs(lengthXZdiag_X.at(lengthXZdiag_X.size() - 1)
                                        - lengthXZdiag_X.at(lengthXZdiag_X.size() - 2));
                    float y_width = abs(lengthXZdiag_Y.at(lengthXZdiag_Y.size() - 1)
                                        - lengthXZdiag_Y.at(lengthXZdiag_Y.size() - 2));
                    if (x_width > y_width) {
                        // X変更なし、Y位置を始点から45°の位置
                        float minusPlus = 1;
                        // if(-lengthXZdiag_Y[lengthXZdiag_Y.size() - 2] > -lengthXZdiag_Y[lengthXZdiag_Y.size() - 1]){
                        // minusPlus = -1;} //符号が逆！
                        if (-lengthXZdiag_Y[lengthXZdiag_Y.size() - 2] < -lengthXZdiag_Y[lengthXZdiag_Y.size() - 1]) {
                            minusPlus = -1;
                        }
                        lengthXZdiag_Y[lengthXZdiag_Y.size() - 1] =
                            lengthXZdiag_Y[lengthXZdiag_Y.size() - 2] + (x_width * minusPlus);
                    }
                    else {
                        // Y変更なし、X位置を始点から45°の位置
                        float minusPlus = 1;
                        if (lengthXZdiag_X[lengthXZdiag_X.size() - 2] > lengthXZdiag_X[lengthXZdiag_X.size() - 1]) {
                            minusPlus = -1;
                        }

                        lengthXZdiag_X[lengthXZdiag_X.size() - 1] =
                            lengthXZdiag_X[lengthXZdiag_X.size() - 2] + (y_width * minusPlus);
                    }
                }
                else if (ui->XZdiag_comboBox_Length->currentText() == "Horizontal") {
                    lengthXZdiag_Y[lengthXZdiag_Y.size() - 1] = lengthXZdiag_Y[lengthXZdiag_Y.size() - 2];
                }
                else if (ui->XZdiag_comboBox_Length->currentText() == "Vertical") {
                    lengthXZdiag_X[lengthXZdiag_Y.size() - 1] = lengthXZdiag_X[lengthXZdiag_Y.size() - 2];
                }
            }

            on_XZdiag_Redraw_pushButton_clicked();
        }
    }
    else {
        if ((ui->tabWidget->currentIndex() == 0 && ui->XZlength_checkBox->text() == "Length_on")
            || (ui->tabWidget->currentIndex() == 1 && ui->YZlength_checkBox->text() == "Length_on")
            || (ui->tabWidget->currentIndex() == 2 && ui->XYlength_checkBox->text() == "Length_on")
            || (ui->tabWidget->currentIndex() == 3 && ui->XZdiag_length_checkBox->text() == "Length_on")) {
            FLAG_LENGTH = 0;
        }
    }
    // -end- Length(測長)処理

    // ShapeInfo処理 (クリックした座標が、Geometory構造一覧テーブルのどれにあたるかを表示する)
    if ((ui->tabWidget->currentIndex() == 0 && ui->XZ_comboBox->currentText() == "ShapeInfo")
        || (ui->tabWidget->currentIndex() == 1 && ui->YZ_comboBox->currentText() == "ShapeInfo")
        || (ui->tabWidget->currentIndex() == 2 && ui->XY_comboBox->currentText() == "ShapeInfo")
        || (ui->tabWidget->currentIndex() == 3 && ui->XZdiag_comboBox->currentText() == "ShapeInfo")) {
        if ((ui->tabWidget->currentIndex() == 0 && ui->XZlength_checkBox->isChecked())
            || (ui->tabWidget->currentIndex() == 1 && ui->YZlength_checkBox->isChecked())
            || (ui->tabWidget->currentIndex() == 2 && ui->XYlength_checkBox->isChecked())
            || (ui->tabWidget->currentIndex() == 3 && ui->XZdiag_length_checkBox->isChecked())
            || (ui->tabWidget->currentIndex() == 0 && ui->XZlength_checkBox->text() == "Length_on")
            || (ui->tabWidget->currentIndex() == 1 && ui->YZlength_checkBox->text() == "Length_on")
            || (ui->tabWidget->currentIndex() == 2 && ui->XYlength_checkBox->text() == "Length_on")
            || (ui->tabWidget->currentIndex() == 3 && ui->XZdiag_length_checkBox->text() == "Length_on")) {
            // LengthチェックONの場合、ShapeInfo処理は実行しない (Length処理優先するため）
        }
        else {
            // 実行：　ShapeInfo処理
            func_view_SelectedShapeInfo_vox(x_meshAjust, y_meshAjust);
        }
    }
    // -end- ShapeInfo処理

    // 2018.09.03追加
    //    if(shape.startsWith("Polygon") && (ui->VoP_AddwithMouse_pushButton->text() == " Click Vertex !! ")){
    //        func_VoP_addVertex_withMouse(x_meshAjust, y_meshAjust);
    //    }
    //    if(obname.startsWith("POL") && (ui->ob_VoP_AddwithMouse_pushButton->text() == " Click Vertex !! ")){
    //        func_ob_VoP_addVertex_withMouse(x_meshAjust, y_meshAjust);
    //    }
    //    if(bd_shape.startsWith("Polygon") && (ui->bd_VoP_AddwithMouse_pushButton->text() == " Click Vertex !! ")){
    //        func_bd_VoP_addVertex_withMouse(x_meshAjust, y_meshAjust);
    //    }
    //    if( ui->tabWidget->currentIndex() == 2 && obname.startsWith("PD") &&
    //    ui->PD_RDTI_Addline_Mouse_label->text().endsWith(TR("XYplane上で始点/終点をClickして下さい。")) ){
    //        func_PD_RDTI_Addline_MouseClick(x_meshAjust, y_meshAjust);
    //    }

    // クリックされたZ座標のXY画面へ移動する　2019.09.25
    if ((ui->tabWidget->currentIndex() == 0 && ui->XZ_comboBox->currentText() == "jumpXY")
        || (ui->tabWidget->currentIndex() == 1 && ui->YZ_comboBox->currentText() == "jumpXY")
        || (ui->tabWidget->currentIndex() == 3 && ui->XZdiag_comboBox->currentText() == "jumpXY")
        || (ui->tabWidget->currentIndex() == 4 && ui->XZdiag_comboBox->currentText() == "jumpXY")) {
        ui->Zcoordinate_spin_lineEdit->setText(QString::number(y_meshAjust));
        func_on_Zcoordinate_spin_lineEdit_editingFinished();
        on_XYRedraw_pushButton_clicked();
        ui->tabWidget->setCurrentIndex(2);    // XYビュー表示
        //
        ui->XZ_comboBox->setCurrentIndex(0);    // 初期状態に戻す
    }

    // 自動測長処理AutoLength (クリックした座標が、Geometory構造一覧テーブルのどれにあたるかを表示する)
    // ・Length, ruler処理優先のため、どちらかでもチェックONの場合、処理実行しない
    // ・処理フロー  関数呼び出しpushButton_clicked()→viewer_update()→func_view_autoLength
    // ・Ctrl+左クリックで動作する。※先にCtrl長押しして、同時に左クリックが必要、Ctrl認識まで時間必要なため。
    // if(ui->vox_radioButton_autolength_pointInput->isChecked() == false){
    // //座標指定(pointInput)、中央値指定(pointCenter)の場合は、ここでの座標指定不要
    if (ui->vox_radioButton_autolength_pointMouse->isChecked() == true && g_flag_keyCTRL == 1) {
        QString direction = "V";
        if (ui->vox_radioButton_autolength_directionV->isChecked()) {
            direction = "V";
        }
        if (ui->vox_radioButton_autolength_directionH->isChecked()) {
            direction = "H";
        }

        if (ui->tabWidget->currentIndex() == 0 && ui->XZautolength_checkBox->isChecked()) {
            g_autoXZ_X         = X_IN_f;
            g_autoXZ_Y         = Y_IN_f;
            g_autoXZ_direction = direction;    // 測長方向 "V" or "H"
            on_XZRedraw_pushButton_clicked();
        }
        if (ui->tabWidget->currentIndex() == 1 && ui->YZautolength_checkBox->isChecked()) {
            g_autoYZ_X         = X_IN_f;
            g_autoYZ_Y         = Y_IN_f;
            g_autoYZ_direction = direction;
            on_YZRedraw_pushButton_clicked();
        }
        if (ui->tabWidget->currentIndex() == 2 && ui->XYautolength_checkBox->isChecked()) {
            g_autoXY_X         = X_IN_f;
            g_autoXY_Y         = Y_IN_f;
            g_autoXY_direction = direction;
            on_XYRedraw_pushButton_clicked();
        }
        if (ui->tabWidget->currentIndex() == 3 && ui->XZdiag_autolength_checkBox->isChecked()) {
            g_autoXZdiag_X         = X_IN_f;
            g_autoXZdiag_Y         = Y_IN_f;
            g_autoXZdiag_direction = direction;
            on_XZdiag_Redraw_pushButton_clicked();
        }
    }
    // -end- AutoLength処理
}
// -end-  void GeometryInputForm::receiveCoord(int X_IN, int Y_IN)

// 2018.01.31 -start-
void GeometryInputForm::receiveCoordMouseRight(float X_IN_f, float Y_IN_f)
{
    int X_IN = int(X_IN_f);    // 整数化
    int Y_IN = int(Y_IN_f);

    // QString in_string = QString::number(X_IN) + ", " + QString::number(Y_IN);
    //  座標はメッシュ単位で表示する
    int tmp_x, tmp_y, meshsize;
    meshsize = ui->meshsize_Value->text().toInt();

    //-start- 2018.09.21 メッシュ丸め変更
    // tmp_x = X_IN - (X_IN % meshsize);
    // tmp_y = Y_IN - (Y_IN % meshsize);
    if (X_IN >= 0) {
        tmp_x = (int)(((double)X_IN + (double)meshsize / 2 - 0.00001) / (double)meshsize) * meshsize;
    }
    else if (X_IN < 0) {
        tmp_x = (int)(((double)X_IN - (double)meshsize / 2 + 0.00001) / (double)meshsize) * meshsize;
    }
    if (Y_IN >= 0) {
        tmp_y = (int)(((double)Y_IN + (double)meshsize / 2 - 0.00001) / (double)meshsize) * meshsize;
    }
    else if (Y_IN < 0) {
        tmp_y = (int)(((double)Y_IN - (double)meshsize / 2 + 0.00001) / (double)meshsize) * meshsize;
    }
    //-end- 2018.09.21 メッシュ丸め変更

    int     x_meshAjust = tmp_x;
    int     y_meshAjust = tmp_y;
    QString in_string   = "Coordinate MouseRight: (" + QString::number(tmp_x) + ", " + QString::number(tmp_y) + ")";

    // クリックされた座標を表示する
    if (ui->tabWidget->currentIndex() == 0) {
        ui->XZinfo_label->setText(in_string);
    }
    if (ui->tabWidget->currentIndex() == 1) {
        ui->YZinfo_label->setText(in_string);
    }
    if (ui->tabWidget->currentIndex() == 2) {
        ui->XYinfo_label->setText(in_string);
    }
    if (ui->tabWidget->currentIndex() == 3) {
        ui->XZdiag_info_label->setText(in_string);
    }

    // ShapeInfo処理 (クリックした座標が、Geometory構造一覧テーブルのどれにあたるかを表示する)
    // func_view_SelectedShapeInfo(X_IN, Y_IN);
    //-start- 2018.01.31
    func_view_SelectedShapeInfo_vox(x_meshAjust, y_meshAjust);
    // -end- ShapeInfo処理

    // ruler
    if (ui->tabWidget->currentIndex() == 0 && ui->XZruler_checkBox->isChecked()) {
        rulerXZ_X = x_meshAjust;
        rulerXZ_Y = y_meshAjust;
        on_XZRedraw_pushButton_clicked();
    }
    else if (ui->tabWidget->currentIndex() == 1 && ui->YZruler_checkBox->isChecked()) {
        rulerYZ_X = x_meshAjust;
        rulerYZ_Y = y_meshAjust;
        on_YZRedraw_pushButton_clicked();
    }
    else if (ui->tabWidget->currentIndex() == 2 && ui->XYruler_checkBox->isChecked()) {
        rulerXY_X = x_meshAjust;
        rulerXY_Y = y_meshAjust;
        on_XYRedraw_pushButton_clicked();
    }
    else if (ui->tabWidget->currentIndex() == 3 && ui->XZdiag_ruler_checkBox->isChecked()) {
        rulerXZdiag_X = x_meshAjust;
        rulerXZdiag_Y = y_meshAjust;
        on_XZdiag_Redraw_pushButton_clicked();
    }
}
// 2018.01.31 -end-

void GeometryInputForm::func_view_Zoom_box(int X_IN, int Y_IN)
{
    // Y_IN の座標はこの関数で -　から　+ に置き替えられて取得している

    QString in_string;

    if (FLAG_ZOOM != 1) {
        // １点目クリック時点
        // QMessageBox::information(this, "info", "Pre-Release.Can't Not Normal operation");
        FLAG_ZOOM = 1;
        X1_ZOOM   = X_IN;
        Y1_ZOOM   = Y_IN;
        qDebug() << "Flag_Zoom=" << FLAG_ZOOM << "X1_ZOOM=" << X1_ZOOM << "Y1_ZOOM" << Y1_ZOOM;
    }
    else {
        // 　2点目クリック時点
        FLAG_ZOOM = 2;
        X2_ZOOM   = X_IN;
        Y2_ZOOM   = Y_IN;
        qDebug() << "Flag_Zoom=" << FLAG_ZOOM << "X2_ZOOM=" << X2_ZOOM << "Y2_ZOOM" << Y2_ZOOM;

        int x_start, x_end, y_start, y_end;

        if (X1_ZOOM < X2_ZOOM) {
            x_start = X1_ZOOM;
            x_end   = X2_ZOOM;
        }
        else {
            x_start = X2_ZOOM;
            x_end   = X1_ZOOM;
        }
        if (Y1_ZOOM < Y2_ZOOM) {
            y_start = Y1_ZOOM;
            y_end   = Y2_ZOOM;
        }
        else {
            y_start = Y2_ZOOM;
            y_end   = Y1_ZOOM;
        }
        in_string = "area:(" + QString::number(x_start) + ", " + QString::number(y_start) + ")" + " - ("
                  + QString::number(x_end) + ", " + QString::number(y_end) + ")";
        // qDebug() <<"[DEBUG]select area" << in_string;

        int tmp_x_start = x_start;
        int tmp_y_start = y_start;
        int tmp_x_end   = x_end;
        int tmp_y_end   = y_end;
        if (x_start < 0) {
            tmp_x_start = 0;
        }
        if (y_start < 0) {
            tmp_y_start = 0;
        }
        if (x_end < 0) {
            tmp_x_end = 0;
        }
        if (y_end < 0) {
            tmp_y_end = 0;
        }
        in_string = "area:(" + QString::number(tmp_x_start) + ", " + QString::number(tmp_y_start) + ")" + " - ("
                  + QString::number(tmp_x_end) + ", " + QString::number(tmp_y_end) + ")";
        // qDebug() << "[DEBUG]select area hosei" << in_string;

        ui->XZinfo_label->setText(in_string);

        int w = abs(x_end - x_start);
        int h = abs(y_end - y_start);
        // 　w, h = 0 の場合のプログラム異常終了の防止
        if (w < 1) {
            w = 1;
        }
        if (h < 1) {
            h = 1;
        }

        int    x_center = abs(x_start) + w / 2;
        int    y_center = abs(y_start) + h / 2;
        double x_scale, y_scale, tmp_scale;
        int    x1_viewArea, y1_viewArea, x2_viewArea, y2_viewArea;
        int    w_viewArea, h_viewArea;
        int    dX = ui->domainX_Value->text().toInt();
        int    dY = ui->domainY_Value->text().toInt();
        int    dZ = ui->domainZ_Value->text().toInt();

        // qDebug() << "[DEBUG]area:" + in_string;
        // qDebug() << "x_start:" << x_start;
        // qDebug() << "y_start:" << y_start;
        // qDebug() << "x_end:" << x_end;
        // qDebug() << "y_end:" << y_end;
        // qDebug() << "x_center:" << x_center;
        // qDebug() << "y_center:" << y_center;
        // qDebug() << "w:" << w ;
        // qDebug() << "h:" << h ;
        // qDebug() << "dX:" << dX;

        // 以下 ZoomBox処理 XZscene, YZscene, XYscene全て処理内容は同じコードとしてXZgraphicsView の文字列が違うだけ）
        // ZoomBox処理 XZscene
        if (ui->tabWidget->currentIndex() == 0 && ui->XZ_comboBox->currentText() == "Zoom_box") {
            qDebug() << "[DEBUG]Geo.cpp-func_view_Zoom_box if tab=0";
            ui->XZinfo_label->setText(in_string);

            // zoomBox機能　指定された範囲表示
            ui->XZgraphicsView->centerOn(x_center, -(y_center));
            ui->XZgraphicsView->fitInView(QRect(x_start, y_start, x_end, y_end), Qt::KeepAspectRatio);
            ui->XZgraphicsView->centerOn(x_center, -(y_center));

            // fitＩｎだけではぴったりにならない場合があるため
            // scaleで調整　（狭い範囲が指定されたとき、領域が広すぎるなど）
            x1_viewArea = ui->XZgraphicsView->mapToScene(ui->XZgraphicsView->viewport()->geometry())
                              .boundingRect()
                              .bottomLeft()
                              .x();
            y1_viewArea = ui->XZgraphicsView->mapToScene(ui->XZgraphicsView->viewport()->geometry())
                              .boundingRect()
                              .bottomLeft()
                              .y();
            x2_viewArea = ui->XZgraphicsView->mapToScene(ui->XZgraphicsView->viewport()->geometry())
                              .boundingRect()
                              .topRight()
                              .x();
            y2_viewArea = ui->XZgraphicsView->mapToScene(ui->XZgraphicsView->viewport()->geometry())
                              .boundingRect()
                              .topRight()
                              .y();
            w_viewArea = ui->XZgraphicsView->mapToScene(ui->XZgraphicsView->viewport()->geometry())
                             .boundingRect()
                             .size()
                             .width();
            h_viewArea = ui->XZgraphicsView->mapToScene(ui->XZgraphicsView->viewport()->geometry())
                             .boundingRect()
                             .size()
                             .height();

            x_scale   = w_viewArea / w;
            y_scale   = h_viewArea / h;
            tmp_scale = x_scale;
            // qDebug()("Before-scale XZscene viewArea x1:y1 = %4.3f : %4.3f x2:y2 = %4.3f : %4.3f\n", x1_viewArea,
            // y1_viewArea, x2_viewArea, y2_viewArea); qDebug()("Before-scale XZscene viewArea w:%4.3f h:%4.3f",
            // w_viewArea, h_viewArea); qDebug()("x_scale:%4.3f y_scale:%4.3f tmp_scale:%4.3f\n", x_scale, y_scale,
            // tmp_scale);
            if (tmp_scale > 0) {
                // tmp_scale > 0　の場合のみscaleで調整　（ tmp_scale=0
                // やマイナスだと　画面ホワイトアウト現象が起こるためif文で回避している)
                ui->XZgraphicsView->centerOn(x_center, -(y_center));
                ui->XZgraphicsView->scale(tmp_scale, tmp_scale);
                ui->XZgraphicsView->centerOn(x_center, -(y_center));
            }
        }

        // ZoomBox処理 YZscene
        // qDebug() << "[DEBUG]YZ_comboBox:" + ui->YZ_comboBox->currentText();
        if (ui->tabWidget->currentIndex() == 1 && ui->YZ_comboBox->currentText() == "Zoom_box") {
            ui->YZinfo_label->setText(in_string);

            // zoomBox機能　指定された範囲表示
            ui->YZgraphicsView->centerOn(x_center, -(y_center));
            ui->YZgraphicsView->fitInView(QRect(x_start, y_start, x_end, y_end), Qt::KeepAspectRatio);
            ui->YZgraphicsView->centerOn(x_center, -(y_center));

            // fitＩｎだけではぴったりにならない場合があるため
            // scaleで調整　（狭い範囲が指定されたとき、領域が広すぎるなど）
            x1_viewArea = ui->YZgraphicsView->mapToScene(ui->YZgraphicsView->viewport()->geometry())
                              .boundingRect()
                              .bottomLeft()
                              .x();
            y1_viewArea = ui->YZgraphicsView->mapToScene(ui->YZgraphicsView->viewport()->geometry())
                              .boundingRect()
                              .bottomLeft()
                              .y();
            x2_viewArea = ui->YZgraphicsView->mapToScene(ui->YZgraphicsView->viewport()->geometry())
                              .boundingRect()
                              .topRight()
                              .x();
            y2_viewArea = ui->YZgraphicsView->mapToScene(ui->YZgraphicsView->viewport()->geometry())
                              .boundingRect()
                              .topRight()
                              .y();
            w_viewArea = ui->YZgraphicsView->mapToScene(ui->YZgraphicsView->viewport()->geometry())
                             .boundingRect()
                             .size()
                             .width();
            h_viewArea = ui->YZgraphicsView->mapToScene(ui->YZgraphicsView->viewport()->geometry())
                             .boundingRect()
                             .size()
                             .height();

            x_scale   = w_viewArea / w;
            y_scale   = h_viewArea / h;
            tmp_scale = x_scale;
            // qDebug()("Before-scale YZscene viewArea x1:y1 = %4.3f : %4.3f x2:y2 = %4.3f : %4.3f\n", x1_viewArea,
            // y1_viewArea, x2_viewArea, y2_viewArea); qDebug()("Before-scale YZscene viewArea w:%4.3f h:%4.3f",
            // w_viewArea, h_viewArea); qDebug()("x_scale:%4.3f y_scale:%4.3f tmp_scale:%4.3f\n", x_scale, y_scale,
            // tmp_scale);
            if (tmp_scale > 0) {
                // tmp_scale > 0　の場合のみscaleで調整　（ tmp_scale=0
                // やマイナスだと　画面ホワイトアウト現象が起こるためif文で回避している
                ui->YZgraphicsView->centerOn(x_center, -(y_center));
                ui->YZgraphicsView->scale(tmp_scale, tmp_scale);
                ui->YZgraphicsView->centerOn(x_center, -(y_center));
            }
        }

        // ZoomBox処理 XYscene
        // qDebug() << "[DEBUG]XZ_comboBox:" + ui->XZ_comboBox->currentText();
        if (ui->tabWidget->currentIndex() == 2 && ui->XY_comboBox->currentText() == "Zoom_box") {
            ui->XYinfo_label->setText(in_string);

            // zoomBox機能　指定された範囲表示
            ui->XYgraphicsView->centerOn(x_center, -(y_center));
            ui->XYgraphicsView->fitInView(QRect(x_start, y_start, x_end, y_end), Qt::KeepAspectRatio);
            ui->XYgraphicsView->centerOn(x_center, -(y_center));

            // fitＩｎだけではぴったりにならない場合があるため
            // scaleで調整　（狭い範囲が指定されたとき、領域が広すぎるなど）
            x1_viewArea = ui->XYgraphicsView->mapToScene(ui->XYgraphicsView->viewport()->geometry())
                              .boundingRect()
                              .bottomLeft()
                              .x();
            y1_viewArea = ui->XYgraphicsView->mapToScene(ui->XYgraphicsView->viewport()->geometry())
                              .boundingRect()
                              .bottomLeft()
                              .y();
            x2_viewArea = ui->XYgraphicsView->mapToScene(ui->XYgraphicsView->viewport()->geometry())
                              .boundingRect()
                              .topRight()
                              .x();
            y2_viewArea = ui->XYgraphicsView->mapToScene(ui->XYgraphicsView->viewport()->geometry())
                              .boundingRect()
                              .topRight()
                              .y();
            w_viewArea = ui->XYgraphicsView->mapToScene(ui->XYgraphicsView->viewport()->geometry())
                             .boundingRect()
                             .size()
                             .width();
            h_viewArea = ui->XYgraphicsView->mapToScene(ui->XYgraphicsView->viewport()->geometry())
                             .boundingRect()
                             .size()
                             .height();

            x_scale   = w_viewArea / w;
            y_scale   = h_viewArea / h;
            tmp_scale = x_scale;
            // qDebug()("Before-scale XYscene viewArea x1:y1 = %4.3f : %4.3f x2:y2 = %4.3f : %4.3f\n", x1_viewArea,
            // y1_viewArea, x2_viewArea, y2_viewArea); qDebug()("Before-scale XYscene viewArea w:%4.3f h:%4.3f",
            // w_viewArea, h_viewArea); qDebug()("x_scale:%4.3f y_scale:%4.3f tmp_scale:%4.3f\n", x_scale, y_scale,
            // tmp_scale);
            if (tmp_scale > 0) {
                // tmp_scale > 0　の場合のみscaleで調整　（ tmp_scale=0
                // やマイナスだと　画面ホワイトアウト現象が起こるためif文で回避している
                ui->XYgraphicsView->centerOn(x_center, -(y_center));
                ui->XYgraphicsView->scale(tmp_scale, tmp_scale);
                ui->XYgraphicsView->centerOn(x_center, -(y_center));
            }
        }

        // ZoomBox処理 XZdiag_scene
        if (ui->tabWidget->currentIndex() == 3 && ui->XZdiag_comboBox->currentText() == "Zoom_box") {
            ui->XZdiag_info_label->setText(in_string);

            // zoomBox機能　指定された範囲表示
            ui->XZdiag_graphicsView->centerOn(x_center, -(y_center));
            ui->XZdiag_graphicsView->fitInView(QRect(x_start, y_start, x_end, y_end), Qt::KeepAspectRatio);
            ui->XZdiag_graphicsView->centerOn(x_center, -(y_center));

            // fitＩｎだけではぴったりにならない場合があるため
            // scaleで調整　（狭い範囲が指定されたとき、領域が広すぎるなど）
            x1_viewArea = ui->XZdiag_graphicsView->mapToScene(ui->XZdiag_graphicsView->viewport()->geometry())
                              .boundingRect()
                              .bottomLeft()
                              .x();
            y1_viewArea = ui->XZdiag_graphicsView->mapToScene(ui->XZdiag_graphicsView->viewport()->geometry())
                              .boundingRect()
                              .bottomLeft()
                              .y();
            x2_viewArea = ui->XZdiag_graphicsView->mapToScene(ui->XZdiag_graphicsView->viewport()->geometry())
                              .boundingRect()
                              .topRight()
                              .x();
            y2_viewArea = ui->XZdiag_graphicsView->mapToScene(ui->XZdiag_graphicsView->viewport()->geometry())
                              .boundingRect()
                              .topRight()
                              .y();
            w_viewArea = ui->XZdiag_graphicsView->mapToScene(ui->XZdiag_graphicsView->viewport()->geometry())
                             .boundingRect()
                             .size()
                             .width();
            h_viewArea = ui->XZdiag_graphicsView->mapToScene(ui->XZdiag_graphicsView->viewport()->geometry())
                             .boundingRect()
                             .size()
                             .height();

            x_scale   = w_viewArea / w;
            y_scale   = h_viewArea / h;
            tmp_scale = x_scale;
            // qDebug()("Before-scale XZdiag_scene viewArea x1:y1 = %4.3f : %4.3f x2:y2 = %4.3f : %4.3f\n", x1_viewArea,
            // y1_viewArea, x2_viewArea, y2_viewArea); qDebug()("Before-scale XZdiag_scene viewArea w:%4.3f h:%4.3f",
            // w_viewArea, h_viewArea); qDebug()("x_scale:%4.3f y_scale:%4.3f tmp_scale:%4.3f\n", x_scale, y_scale,
            // tmp_scale);
            if (tmp_scale > 0) {
                // tmp_scale > 0　の場合のみscaleで調整　（ tmp_scale=0
                // やマイナスだと　画面ホワイトアウト現象が起こるためif文で回避している)
                ui->XZdiag_graphicsView->centerOn(x_center, -(y_center));
                ui->XZdiag_graphicsView->scale(tmp_scale, tmp_scale);
                ui->XZdiag_graphicsView->centerOn(x_center, -(y_center));
            }
        }
    }
    //-end- if(FLAG_ZOOM != 1 ){ ... } else { ... }
}

void GeometryInputForm::receiveShortcutKey(QString input_string)
{                                     // Graphicビューからのショートカットキー受信　2020.03.09
    if (input_string == "key_R") {    // ショートカットキー:R Redraw処理
        if (ui->tabWidget->currentIndex() == 0) {
            on_XZRedraw_pushButton_clicked();
        }
        if (ui->tabWidget->currentIndex() == 1) {
            on_YZRedraw_pushButton_clicked();
        }
        if (ui->tabWidget->currentIndex() == 2) {
            on_XYRedraw_pushButton_clicked();
        }
        if (ui->tabWidget->currentIndex() == 3) {
            on_XZdiag_Redraw_pushButton_clicked();
        }
    }

    if (input_string == "key_Z") {    // ショートカットキー:Z zoomIn処理
        if (ui->tabWidget->currentIndex() == 0) {
            on_XZzoomin_pushButton_clicked();
        }
        if (ui->tabWidget->currentIndex() == 1) {
            on_YZzoomin_pushButton_clicked();
        }
        if (ui->tabWidget->currentIndex() == 2) {
            on_XYzoomin_pushButton_clicked();
        }
        if (ui->tabWidget->currentIndex() == 3) {
            on_XZdiag_zoomin_pushButton_clicked();
        }
    }

    if (input_string == "shift_Z") {    // ショートカットキー:Z zoomOut処理
        if (ui->tabWidget->currentIndex() == 0) {
            on_XZzoomout_pushButton_clicked();
        }
        if (ui->tabWidget->currentIndex() == 1) {
            on_YZzoomout_pushButton_clicked();
        }
        if (ui->tabWidget->currentIndex() == 2) {
            on_XYzoomout_pushButton_clicked();
        }
        if (ui->tabWidget->currentIndex() == 0) {
            on_XZdiag_zoomout_pushButton_clicked();
        }
    }

    if (input_string == "key_F") {    // ショートカットキー:F ZoomAll=Fit処理
        if (ui->tabWidget->currentIndex() == 0) {
            on_XZzoomall_pushButton_clicked();
        }
        if (ui->tabWidget->currentIndex() == 1) {
            on_YZzoomall_pushButton_clicked();
        }
        if (ui->tabWidget->currentIndex() == 2) {
            on_XYzoomall_pushButton_clicked();
        }
        if (ui->tabWidget->currentIndex() == 3) {
            on_XZdiag_zoomall_pushButton_clicked();
        }
    }

    if (input_string == "key_B") {    // ショートカットキー:B ZoomBox処理
        if (ui->tabWidget->currentIndex() == 0) {
            ui->XZ_comboBox->setCurrentText("Zoom_box");
        }
        if (ui->tabWidget->currentIndex() == 1) {
            ui->YZ_comboBox->setCurrentText("Zoom_box");
        }
        if (ui->tabWidget->currentIndex() == 2) {
            ui->XY_comboBox->setCurrentText("Zoom_box");
        }
        if (ui->tabWidget->currentIndex() == 3) {
            ui->XZdiag_comboBox->setCurrentText("Zoom_box");
        }
    }

    // Length は キーLを押し続けている間に画面の2点（始点・終点）をクリックする
    if (input_string == "key_L") {    ////Length機能のための処理　shiftキーを押したら LengthチェックON/OFFを切り替える
        if (ui->tabWidget->currentIndex() == 0) {
            ui->XZlength_checkBox->setText("Length_on");
            // ui->XZ_comboBox_Length->setVisible(true);
        }
        if (ui->tabWidget->currentIndex() == 1) {
            ui->YZlength_checkBox->setText("Length_on");
            // ui->YZ_comboBox_Length->setVisible(true);
        }
        if (ui->tabWidget->currentIndex() == 2) {
            ui->XYlength_checkBox->setText("Length_on");
            // ui->XY_comboBox_Length->setVisible(true);
        }
        if (ui->tabWidget->currentIndex() == 3) {
            ui->XZdiag_length_checkBox->setText("Length_on");
            // ui->XZdiag_comboBox_Length->setVisible(true);
        }
    }

    if (input_string == "off_L") {    // Length機能のための処理　shift離したら、LengthチェックOFF
        if (ui->tabWidget->currentIndex() == 0) {
            //            if(FLAG_LENGTH == 1){
            //                ui->XZlength_checkBox->setText("Length_onP1");
            //                //Length_on:ZoomBoxしない。Length_off:ZoomBoxする
            //                ui->XZ_comboBox_Length->setVisible(true);
            //            } else {
            ui->XZlength_checkBox->setText("Length");    // もとに戻す
            // ui->XZ_comboBox_Length->setVisible(false);
            //            }
        }
        if (ui->tabWidget->currentIndex() == 1) {
            //            if(FLAG_LENGTH == 1){
            //                ui->YZlength_checkBox->setText("Length_onP1");
            //                //Length_on:ZoomBoxしない。Length_off:ZoomBoxする
            //                ui->YZ_comboBox_Length->setVisible(true);
            //            } else {
            ui->YZlength_checkBox->setText("Length");    // もとに戻す
            // ui->YZ_comboBox_Length->setVisible(false);
            //            }
        }
        if (ui->tabWidget->currentIndex() == 2) {
            //            if(FLAG_LENGTH == 1){
            //                ui->XYlength_checkBox->setText("Length_onP1");
            //                //Length_on:ZoomBoxしない。Length_off:ZoomBoxする
            //                ui->XY_comboBox_Length->setVisible(true);
            //            } else {
            ui->XYlength_checkBox->setText("Length");    // もとに戻す
            // ui->XY_comboBox_Length->setVisible(false);
            //            }
        }
        if (ui->tabWidget->currentIndex() == 3) {
            ui->XZdiag_length_checkBox->setText("Length");    // もとに戻す
        }
    }

    if (input_string == "key_Backspace") {    // Length機能のための処理
        if ((ui->tabWidget->currentIndex() == 0 && ui->XZlength_checkBox->isChecked())
            || (ui->tabWidget->currentIndex() == 0 && ui->XZlength_checkBox->text() == "Length_on")) {
            if ((lengthXZ_X.size() > 0) && (lengthXZ_X.size() == lengthXZ_Y.size())) {
                lengthXZ_X.removeLast();
                lengthXZ_Y.removeLast();

                on_XZRedraw_pushButton_clicked();
            }
        }
        else if ((ui->tabWidget->currentIndex() == 1 && ui->YZlength_checkBox->isChecked())
                 || (ui->tabWidget->currentIndex() == 1 && ui->YZlength_checkBox->text() == "Length_on")) {
            if ((lengthYZ_X.size() > 0) && (lengthYZ_X.size() == lengthYZ_Y.size())) {
                lengthYZ_X.removeLast();
                lengthYZ_Y.removeLast();

                on_YZRedraw_pushButton_clicked();
            }
        }
        else if ((ui->tabWidget->currentIndex() == 2 && ui->XYlength_checkBox->isChecked())
                 || (ui->tabWidget->currentIndex() == 2 && ui->XYlength_checkBox->text() == "Length_on")) {
            if ((lengthXY_X.size() > 0) && (lengthXY_X.size() == lengthXY_Y.size())) {
                lengthXY_X.removeLast();
                lengthXY_Y.removeLast();

                on_XYRedraw_pushButton_clicked();
            }
        }
        else if ((ui->tabWidget->currentIndex() == 3 && ui->XZdiag_length_checkBox->isChecked())
                 || (ui->tabWidget->currentIndex() == 3 && ui->XZdiag_length_checkBox->text() == "Length_on")) {
            if ((lengthXZ_X.size() > 0) && (lengthXZ_X.size() == lengthXZ_Y.size())) {
                lengthXZ_X.removeLast();
                lengthXZ_Y.removeLast();

                on_XZdiag_Redraw_pushButton_clicked();
            }
        }
    }

    if (input_string == "key_CTRL") {    // ショートカットキー AutoLength用途(ctrl+左マウスクリック）
        g_flag_keyCTRL = 1;
    }
    if (input_string == "off_CTRL") {    // ショートカットキー AutoLength用途(ctrl+左マウスクリック）
        g_flag_keyCTRL = 0;
    }
}

void GeometryInputForm::func_view_SelectedShapeInfo_vox(
    int Xinput,
    int Yinput)    // ShapeInfoモード　Geometory画面クリックされた図形の番号とマテリアル名表示　//2021.04.xx-01
                   // vox-2Dviewer用に変更
{
    // vox-2Dviewer用に内容変更する必要あり。
    // QString XZinfoString = "[DEBUG] click X=" + QString::number(Xinput) + "click Y=" +QString::number(Yinput);
    // ui->XZinfo_label->setText(XZinfoString);

    //-start- for-vox
    int     meshsize   = ui->meshsize_Value->text().toInt();
    QString infoString = "";
    int     x, y, z;
    x = 0;
    y = 0;
    z = 0;
    //-end- for-vox
    int Xclick, Yclick, Zclick;
    Xclick = 0;
    Yclick = 0;
    Zclick = 0;
    if (ui->tabWidget->currentIndex() == 0) {
        Xclick = Xinput;
        Yclick = ui->Ycoordinate_spin_lineEdit->text().toInt();
        Zclick = Yinput;
    }
    else if (ui->tabWidget->currentIndex() == 1) {
        Xclick = ui->Xcoordinate_spin_lineEdit->text().toInt();
        Yclick = Xinput;
        Zclick = Yinput;
    }
    else if (ui->tabWidget->currentIndex() == 2) {
        Xclick = Xinput;
        Yclick = Yinput;
        Zclick = ui->Zcoordinate_spin_lineEdit->text().toInt();
    }
    x = int(Xclick / meshsize);
    y = int(Yclick / meshsize);
    z = int(Zclick / meshsize);

    int     matnum     = 0;
    QString matname    = "Air";
    int     tabnum     = ui->tabWidget->currentIndex();
    int     flag_noMat = 1;
    if (tabnum == 0 || tabnum == 1 || tabnum == 2) {    // XZ, YZ, XYビュー
        if (x < 0 || y < 0 || z < 0 || Xclick >= ui->domainX_Value->text().toInt()
            || Yclick >= ui->domainY_Value->text().toInt() || Zclick >= ui->domainZ_Value->text().toInt()) {
            // 異常終了防止(matnum = g_voxDomain[x][y][z])。 図形描画範囲外の場合は マテリアル名は表示しない。
            flag_noMat = 1;
        }
        else {
            flag_noMat = 0;
            matnum     = g_voxDomain[x][y][z];
        }
    }
    if (tabnum == 3 || tabnum == 4) {
        x = int((ceil(Xinput / sqrt(2)) / meshsize));    //-1しているのは、インデックス番号0始まりのため。
        z = int(Yinput / meshsize);

        //-start- [DEBUG]
        // qDebug () << "\n[DEBUG]Geo.cpp-func_view_SelectedShapeInfo_vox()g_XZdiagNow2d=";
        // for(int z = g_nz - 1; z >= 0 ; z--){ //[DEBUG]確認のため上から書く
        //    QString tmpstr="";
        //    for(int x=0; x<g_nxdiag; x++){
        //        if(tabnum==3){tmpstr += " " + QString::number(g_XZdiagNow2d[x][z]);}
        //    }
        //    qDebug() << tmpstr;
        //}
        // qDebug() << QString("\n[DEBUG]Geo.cpp-func_view_SelectedShapeIfo_vox g_XZ/YZdiagNow2d index-x=%1
        // y=%2").arg(QString::number(x), QString::number(y)); -end- [DEBUG]

        if (tabnum == 3) {
            // qDebug() << QString("\n[DEBUG]Geo.cpp-func_view_SelectedShapeIfo_vox tabnum==3");
            if (x < 0 || z < 0 || x > g_nxdiag - 1 || z > g_nz - 1) {
                flag_noMat = 1;
            }
            else {
                flag_noMat = 0;
                matnum     = g_XZdiagNow2d[x][z];
            }
        }
    }

    int tmpIndex = g_VoxMatIDList.indexOf(QString::number(matnum));
    if (tmpIndex > -1) {
        matname = g_VoxMatNameList.at(tmpIndex);
    }
    if (flag_noMat == 1) {
        infoString = infoString + " material_NoExist.";
    }
    else {
        infoString = infoString + " matname: " + matname + " num: " + QString::number(matnum);
    }

    // フォームにメッセージ表示
    if (ui->tabWidget->currentIndex() == 0) {
        infoString = infoString + " clicked X=" + QString::number(Xinput) + " Z=" + QString::number(Yinput);
        ui->XZinfo_label->setText(infoString);
    }
    else if (ui->tabWidget->currentIndex() == 1) {
        infoString = infoString + " clicked Y=" + QString::number(Xinput) + " Z=" + QString::number(Yinput);
        ui->YZinfo_label->setText(infoString);
    }
    else if (ui->tabWidget->currentIndex() == 2) {
        infoString = infoString + " clicked X=" + QString::number(Xinput) + " Y=" + QString::number(Yinput);
        ui->XYinfo_label->setText(infoString);
    }
    else if (ui->tabWidget->currentIndex() == 3) {
        infoString = infoString + " clicked X=" + QString::number(Xinput) + " Z=" + QString::number(Yinput);
        ui->XZdiag_info_label->setText(infoString);
    }
}

void GeometryInputForm::on_XZruler_checkBox_stateChanged(int arg1)
{
    if (ui->XZruler_checkBox->isChecked()) {
        ui->XZruler_color_comboBox->show();
    }
    else {
        ui->XZruler_color_comboBox->hide();
        rulerXZ_X = 9999;
        rulerXZ_Y = 9999;
        on_XZRedraw_pushButton_clicked();
    }
}

void GeometryInputForm::on_YZruler_checkBox_stateChanged(int arg1)
{
    if (ui->YZruler_checkBox->isChecked()) {
        ui->YZruler_color_comboBox->show();
    }
    else {
        ui->YZruler_color_comboBox->hide();
        rulerYZ_X = 9999;
        rulerYZ_Y = 9999;
        on_YZRedraw_pushButton_clicked();
    }
}

void GeometryInputForm::on_XYruler_checkBox_stateChanged(int arg1)
{
    if (ui->XYruler_checkBox->isChecked()) {
        ui->XYruler_color_comboBox->show();
    }
    else {
        ui->XYruler_color_comboBox->hide();
        rulerXY_X = 9999;
        rulerXY_Y = 9999;
        on_XYRedraw_pushButton_clicked();
    }
}

void GeometryInputForm::on_XZdiag_ruler_checkBox_stateChanged(int arg1)
{
    if (ui->XZdiag_ruler_checkBox->isChecked()) {
        ui->XZdiag_ruler_color_comboBox->show();
    }
    else {
        ui->XZdiag_ruler_color_comboBox->hide();
        rulerXZdiag_X = 9999;
        rulerXZdiag_Y = 9999;
        on_XZdiag_Redraw_pushButton_clicked();
    }
}

void GeometryInputForm::on_XZruler_color_comboBox_currentIndexChanged(int index)
{
    if (ui->tabWidget->currentIndex() == 0 && ui->XZruler_checkBox->isChecked() && rulerXZ_X != 9999
        && rulerXZ_Y != 9999) {
        on_XZRedraw_pushButton_clicked();
    }
}

void GeometryInputForm::on_YZruler_color_comboBox_currentIndexChanged(int index)
{
    if (ui->tabWidget->currentIndex() == 1 && ui->YZruler_checkBox->isChecked() && rulerYZ_X != 9999
        && rulerYZ_Y != 9999) {
        on_YZRedraw_pushButton_clicked();
    }
}

void GeometryInputForm::on_XYruler_color_comboBox_currentIndexChanged(int index)
{
    if (ui->tabWidget->currentIndex() == 2 && ui->XYruler_checkBox->isChecked() && rulerXY_X != 9999
        && rulerXY_Y != 9999) {
        on_XYRedraw_pushButton_clicked();
    }
}

void GeometryInputForm::on_XZdiag_ruler_color_comboBox_currentIndexChanged(int index)
{
    if (ui->tabWidget->currentIndex() == 3 && ui->XZdiag_ruler_checkBox->isChecked() && rulerXZdiag_X != 9999
        && rulerXZdiag_Y != 9999) {
        on_XZdiag_Redraw_pushButton_clicked();
    }
}

void GeometryInputForm::on_XZ_comboBox_currentTextChanged(const QString& arg1)
{
    // qDebug() << "XZ_comboBox_currentTextChanged";

    // メニュー　Nop →　//別関数　receiveCoord<マウスクリックされた場合の処理> から起動されるためここでは処理記述なし
    // メニュー　Zoom_box　→　//別関数　receiveCoord<マウスクリックされた場合の処理>
    // から起動されるためここでは処理記述なし メニュー　Zoom_　→　//別関数　receiveCoord<マウスクリックされた場合の処理>
    // から起動されるためここでは処理記述なし

    // メニュー　ImgSaveAll 全範囲のスクリーンショット
    // if(ui->tabWidget->currentIndex() == 0  && ui->XZ_comboBox->currentText() == "ImgSaveAll")
    if (ui->tabWidget->currentIndex() == 0) {
        // メニュー　ImgSaveAll 全範囲のスクリーンショット
        if (ui->XZ_comboBox->currentText() == "ImgSaveAll") {
            func_ImgSaveAll();
            ui->XZ_comboBox->setCurrentIndex(0);    // Nopに戻す
        }

        // メニュー　ImgSaveAll 全範囲のスクリーンショット
        if (ui->XZ_comboBox->currentText() == "ImgSaveClipboard") {
            func_ImgSaveAll_Clipboard();
            ui->XZ_comboBox->setCurrentIndex(0);    // Nopに戻す
        }

        //        //メニュー　DrawSpeedUp 表示速度優先のため、描画を粗くする
        //        if(ui->XZ_comboBox->currentText() == "DrawSpeedUp") {
        //            func_DrawSpeedUp();
        //            ui->XZ_comboBox->setCurrentIndex(0); //Nopに戻す
        //        }

        //        //メニュー　forNewVersion
        //        プログラムのバージョンアップでフォームや、fdtdファイル書式が変更になった場合の一時対応用途
        //        if(ui->XZ_comboBox->currentText() == "forNewVersion") {
        //            func_forNewVersion();
        //            ui->XZ_comboBox->setCurrentIndex(0); //Nopに戻す
        //        }

        // メニュー　VoxCut .voxファイルを座標指定分割した新規.vox作成 2018.05.16
        //        if(ui->XZ_comboBox->currentText() == "VoxCut"){
        //            //func_DialogRun_VoxCut();
        //            func_VoxCut_allFlow();
        //            //Nopに戻すことはしない　2点目が取れなくなってしまうため
        //        }
    }
}

void GeometryInputForm::on_YZ_comboBox_currentTextChanged(const QString& arg1)
{
    // qDebug() << "YZ_comboBox_currentTextChanged";
    // メニュー　Nop →　//別関数　receiveCoord<マウスクリックされた場合の処理> から起動されるためここでは処理記述なし
    // メニュー　Zoom_box　→　//別関数　receiveCoord<マウスクリックされた場合の処理>
    // から起動されるためここでは処理記述なし メニュー　Zoom_　→　//別関数　receiveCoord<マウスクリックされた場合の処理>
    // から起動されるためここでは処理記述なし

    if (ui->tabWidget->currentIndex() == 1) {
        // メニュー　ImgSaveAll 全範囲のスクリーンショット
        if (ui->YZ_comboBox->currentText() == "ImgSaveAll") {
            func_ImgSaveAll();
            ui->YZ_comboBox->setCurrentIndex(0);    // Nopに戻す
        }

        // メニュー　ImgSaveAll 全範囲のスクリーンショット
        if (ui->YZ_comboBox->currentText() == "ImgSaveClipboard") {
            func_ImgSaveAll_Clipboard();
            ui->YZ_comboBox->setCurrentIndex(0);    // Nopに戻す
        }

        //        //メニュー　DrawSpeedUp 表示速度優先のため、描画を粗くする
        //        if(ui->YZ_comboBox->currentText() == "DrawSpeedUp") {
        //            func_DrawSpeedUp();
        //            ui->YZ_comboBox->setCurrentIndex(0); //Nopに戻す
        //        }

        //        //メニュー　forNewVersion
        //        プログラムのバージョンアップでフォームや、fdtdファイル書式が変更になった場合の一時対応用途
        //        if(ui->YZ_comboBox->currentText() == "forNewVersion") {
        //            func_forNewVersion();
        //            ui->YZ_comboBox->setCurrentIndex(0); //Nopに戻す
        //        }
    }
}

void GeometryInputForm::on_XY_comboBox_currentTextChanged(const QString& arg1)
{
    // qDebug() << "XY_comboBox_currentTextChanged";
    // メニュー　Nop →　//別関数　receiveCoord<マウスクリックされた場合の処理> から起動されるためここでは処理記述なし
    // メニュー　Zoom_box　→　//別関数　receiveCoord<マウスクリックされた場合の処理>
    // から起動されるためここでは処理記述なし メニュー　Zoom_　→　//別関数　receiveCoord<マウスクリックされた場合の処理>
    // から起動されるためここでは処理記述なし

    if (ui->tabWidget->currentIndex() == 2) {
        // メニュー　ImgSaveAll 全範囲のスクリーンショット
        if (ui->XY_comboBox->currentText() == "ImgSaveAll") {
            func_ImgSaveAll();
            ui->XY_comboBox->setCurrentIndex(0);    // Nopに戻す
        }

        // メニュー　ImgSaveAll 全範囲のスクリーンショット
        if (ui->XY_comboBox->currentText() == "ImgSaveClipboard") {
            func_ImgSaveAll_Clipboard();
            ui->XY_comboBox->setCurrentIndex(0);    // Nopに戻す
        }

        //        //メニュー　DrawSpeedUp 表示速度優先のため、描画を粗くする
        //        if(ui->XY_comboBox->currentText() == "DrawSpeedUp") {
        //            func_DrawSpeedUp();
        //            ui->XY_comboBox->setCurrentIndex(0); //Nopに戻す
        //        }

        //        //メニュー　forNewVersion
        //        プログラムのバージョンアップでフォームや、fdtdファイル書式が変更になった場合の一時対応用途
        //        if(ui->XY_comboBox->currentText() == "forNewVersion") {
        //            func_forNewVersion();
        //            ui->XY_comboBox->setCurrentIndex(0); //Nopに戻す
        //        }
    }
}

void GeometryInputForm::on_XZdiag_comboBox_currentTextChanged(const QString& arg1)
{
    // qDebug() << "XZdiag_comboBox_currentTextChanged";

    // メニュー　Nop →　//別関数　receiveCoord<マウスクリックされた場合の処理> から起動されるためここでは処理記述なし
    // メニュー　Zoom_box　→　//別関数　receiveCoord<マウスクリックされた場合の処理>
    // から起動されるためここでは処理記述なし メニュー　Zoom_　→　//別関数　receiveCoord<マウスクリックされた場合の処理>
    // から起動されるためここでは処理記述なし

    // メニュー　ImgSaveAll 全範囲のスクリーンショット
    // if(ui->tabWidget->currentIndex() == 0  && ui->XZdiag_comboBox->currentText() == "ImgSaveAll")
    if (ui->tabWidget->currentIndex() == 3) {
        // メニュー　ImgSaveAll 全範囲のスクリーンショット
        if (ui->XZdiag_comboBox->currentText() == "ImgSaveAll") {
            func_ImgSaveAll();
            ui->XZdiag_comboBox->setCurrentIndex(0);    // Nopに戻す
        }

        // メニュー　ImgSaveAll 全範囲のスクリーンショット
        if (ui->XZdiag_comboBox->currentText() == "ImgSaveClipboard") {
            func_ImgSaveAll_Clipboard();
            ui->XZdiag_comboBox->setCurrentIndex(0);    // Nopに戻す
        }
    }
}

void GeometryInputForm::func_ImgSaveAll()
{
    qDebug() << "[DEBUG]01 func_ImgSaveAll()";
    QString tmpString;

    // メッセージボックス表示　//スクリーンショットを取りますか？　YesNo でOKならば処理継続
    QMessageBox msgBOX;
    msgBOX.setText("Do you save image all area?");
    msgBOX.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    int ret = msgBOX.exec();
    switch (ret) {
        case QMessageBox::Yes:
            break;    // 処理継続
        case QMessageBox::No:
            return;    // 処理終了
        default:
            return;    // 処理終了
    }

    GeometrySubScene* tmpScene;
    int               tmpXDomain, tmpYDomain, tmpZDomain;
    tmpXDomain = ui->domainX_Value->text().toInt();
    tmpYDomain = ui->domainY_Value->text().toInt();
    tmpZDomain = ui->domainZ_Value->text().toInt();

    QString tmpFileFullPath, tmpFileName, tmpFileDir;
    // QString kouzou = ui->DisplayGeometry_comboBox->currentText(); //現在表示の構造を記録
    QString loadFilename = ui->vox_path_lineEdit->text();
    if (loadFilename.isEmpty() == 0) {
        tmpFileFullPath = loadFilename;
    }
    else {
        ////.fdtdファイルの保存が必要
        ////MainWindow.cpp の save_as関数呼び出し　connect定義は MainWindow.cppに記載
        // tmpString = TR(".fdtdファイルの保存が必要です。"); //"Please input setting filename.\n (for define data
        // folder)"; QMessageBox::information(this, "Notice", tmpString); emit sendtoMW_actionSave_as(); tmpFileFullPath
        // = saveasFilename; loadFilename = saveasFilename; ui->DisplayGeometry_comboBox->setCurrentText(kouzou);
        // //save後、1stになるので、表示されていた構造に戻す
        QMessageBox::information(this, "notice", "Error: please input voxfile path.");
        return;
    }
    // tmpString = "[DEBUG] tmpFileFullPath=" + tmpFileFullPath + "\n baseName=" + tmpFileName + "\n dir=" + tmpFileDir
    // ; QMessageBox::information(this, "Notice", tmpString);
    if (tmpFileFullPath.isEmpty()) {
        return;
    }

    tmpFileName = QFileInfo(tmpFileFullPath).baseName();
    tmpFileDir  = QFileInfo(tmpFileFullPath).absolutePath();

    // ファイル保存場所 フォルダがない場合は作成する
    QString saveFileDir, saveFileName;
    // saveFileDir = "c:/temp/FDTDsolver_work";
    // saveFileName = "image_FDTDsolver.png";
    // saveFileDir = tmpFileDir + "/FDTDwork_image_" + tmpFileName ;
    saveFileDir = tmpFileDir + "/FDTDwork_image";
    QDir dir(saveFileDir);
    if (!dir.exists()) {
        dir.mkpath(saveFileDir);
    }

    // ファイル名
    // saveFileName = "image_FDTDsolver.png";
    // 保存ファイル名　　①.fdtdの名前 + ②現在のプレーン..XZなど +  ③スライダーの位置..Zxxなど + ④日付(時間)
    QString planeStr, sliderStr;
    int     sliderValue;
    if (ui->tabWidget->currentIndex() == 0) {
        planeStr    = "XZ";
        sliderStr   = "Y";
        sliderValue = ui->Ycoordinate_Slider->value();
    }
    if (ui->tabWidget->currentIndex() == 1) {
        planeStr    = "YZ";
        sliderStr   = "X";
        sliderValue = ui->Xcoordinate_Slider->value();
    }
    if (ui->tabWidget->currentIndex() == 2) {
        planeStr    = "XY";
        sliderStr   = "Z";
        sliderValue = ui->Zcoordinate_Slider->value();
    }
    if (ui->tabWidget->currentIndex() == 3) {
        planeStr    = "XZdiag";
        sliderStr   = "Y";
        sliderValue = ui->XZdiag_Ycoordinate_Slider->value();
    }

    QDateTime dt         = QDateTime::currentDateTime();
    QString   TimeString = dt.toString("yyMMdd");

    // ファイル名
    saveFileName =
        tmpFileName + "_" + planeStr + "_" + sliderStr + QString::number(sliderValue) + "_" + TimeString + ".png";

    // 保存ファイル　フルパス
    QString saveFilePath = saveFileDir + "/" + saveFileName;

    if (!saveFilePath.isEmpty()) {
        int tmpWidth        = 1;
        int tmpHeight       = 1;
        int flag_autolength = 0;
        if (ui->tabWidget->currentIndex() == 0)    // XZscene の場合
        {
            tmpScene  = XZscene;
            tmpWidth  = tmpXDomain;
            tmpHeight = tmpZDomain;
            if (ui->XZautolength_checkBox->isChecked()) {
                flag_autolength = 1;
            }
        }
        if (ui->tabWidget->currentIndex() == 1)    // YZscene の場合
        {
            tmpScene  = YZscene;
            tmpWidth  = tmpYDomain;
            tmpHeight = tmpZDomain;
            if (ui->YZautolength_checkBox->isChecked()) {
                flag_autolength = 1;
            }
        }
        if (ui->tabWidget->currentIndex() == 2)    // XYscene の場合
        {
            tmpScene  = XYscene;
            tmpWidth  = tmpXDomain;
            tmpHeight = tmpYDomain;
            if (ui->XYautolength_checkBox->isChecked()) {
                flag_autolength = 1;
            }
        }
        if (ui->tabWidget->currentIndex() == 3)    // XZscene の場合
        {
            qDebug() << "[DEBUG]func_ImgSaveAll() XZdiag";
            tmpScene  = XZdiag_scene;
            tmpWidth  = tmpXDomain * sqrt(2);
            tmpHeight = tmpZDomain;
            if (ui->XZdiag_autolength_checkBox->isChecked()) {
                flag_autolength = 1;
            }
        }

        tmpScene->clearSelection();    // Selections would also render to the file
        // scene->setSceneRect(scene->itemsBoundingRect());                          // Re-shrink the scene to it's
        // bounding contents scene->setSceneRect(0, -100, 500, 500);
        tmpScene->setSceneRect(0, -tmpHeight, tmpWidth, tmpHeight);
        if (flag_autolength == 1) {
            // AutoLength結果ビュー表示が右側にはみ出す場合でも、画像取得するように tmpWidthのサイズを大きくする
            tmpScene->setSceneRect(0, -tmpHeight, tmpWidth * 2, tmpHeight);
        }
        QImage image(tmpScene->sceneRect().size().toSize(),
                     QImage::Format_ARGB32);    // Create the image with the exact size of the shrunk scene
        image.fill(Qt::transparent);            // Start all pixels transparent
        //        //画像サイズ縮小 ...大判などそのままだと画像のサイズが大きすぎて見づらいため。
        //        //ただしセルサイズ小1um以下の場合は縮小なしセルラーなど
        //        ...powerpoint資料作成時に小さすぎて画像ボケすることを避けたいため。 double scaleImgSize = 0.05;
        //        if(tmpXDomain <= 1000 || tmpYDomain <= 1000 ){
        //            scaleImgSize = 1;
        //        }
        //        //qDebug() << "[DEBUG] domainX_Value=" << ui->domainX_Value->text() << " domainY_Value=" <<
        //        ui->domainY_Value->text().toInt();
        //        //qDebug() << "[DEBUG] scaleImgSize=" << scaleImgSize;
        //        int tmpWidth2 = tmpScene->sceneRect().width() * scaleImgSize;// *0.05;
        //        int tmpHeight2 = tmpScene->sceneRect().height()  * scaleImgSize;// *0.05;

        // そのままsaveすると大きいため縮小
        double scaleImgSize = 0.1;
        int    tmpWidth2    = (int)((double)tmpScene->sceneRect().width() * scaleImgSize);
        int    tmpHeight2   = (int)((double)tmpScene->sceneRect().height() * scaleImgSize);
        image               = image.scaled(tmpWidth2, tmpHeight2, Qt::KeepAspectRatio, Qt::FastTransformation);

        QPainter painter(&image);
        tmpScene->render(&painter);

        // qDebug() << "savefile=" << saveFilePath << "tmpWidth=" << QString::number(tmpWidth)  << "tmpHeight=" <<
        // QString::number(tmpHeight);
        image.save(saveFilePath);
        tmpString = "save file: " + saveFilePath;
        QMessageBox::information(this, "Notice", tmpString);
    }
}

void GeometryInputForm::func_ImgSaveAll_Clipboard()    // 選択しているビューの全体画像をクリップボードにコピーする
                                                       // 2019.03.18-01
{
    //    QString planeStr;
    //    if(ui->tabWidget->currentIndex() == 0 ) {
    //        planeStr = "XZ";
    //    }
    //    if(ui->tabWidget->currentIndex() == 1 ) {
    //        planeStr = "YZ";
    //    }
    //    if(ui->tabWidget->currentIndex() == 2 ) {
    //        planeStr = "XY";
    //    }

    //    QClipboard *clip = QApplication::clipboard();
    // OK ただし貼り付け後、上下逆になってしまう。。//clip->setPixmap(item1->pixmap(), QClipboard::Clipboard);

    //    QPixmap pix_clip; //(XWidthPixmap, YWidthPixmap);
    //    pix_clip = item1->pixmap();
    //    QTransform matrix
    //    (
    //        1.0, 0.0,  0.0,
    //        0.0,  -1.0,  0.0,
    //        0.0,  0.0,  1.0
    //    );
    //    //pix_clip.transformed(matrix);
    //    clip->setPixmap(pix_clip.transformed(matrix), QClipboard::Clipboard);

    // item1->setTransform(matrix);

    // QString tmpMsg = "End. plane:" + planeStr +" ClipBoard Copy";
    // QMessageBox::information(this, "Notice", tmpMsg);

    // lengthやrulerを表示した状態(image save all と同じ状態)で保存させたいとの要望を受けて変更
    QClipboard*       clip = QApplication::clipboard();
    GeometrySubScene* tmpScene;
    int               tmpXDomain, tmpYDomain, tmpZDomain;
    tmpXDomain = ui->domainX_Value->text().toInt();
    tmpYDomain = ui->domainY_Value->text().toInt();
    tmpZDomain = ui->domainZ_Value->text().toInt();

    int tmpWidth        = 1;
    int tmpHeight       = 1;
    int flag_autolength = 0;
    if (ui->tabWidget->currentIndex() == 0)    // XZscene の場合
    {
        tmpScene  = XZscene;
        tmpWidth  = tmpXDomain;
        tmpHeight = tmpZDomain;
        if (ui->XZautolength_checkBox->isChecked()) {
            flag_autolength = 1;
        }
    }
    if (ui->tabWidget->currentIndex() == 1)    // YZscene の場合
    {
        tmpScene  = YZscene;
        tmpWidth  = tmpYDomain;
        tmpHeight = tmpZDomain;
        if (ui->YZautolength_checkBox->isChecked()) {
            flag_autolength = 1;
        }
    }
    if (ui->tabWidget->currentIndex() == 2)    // XYscene の場合
    {
        tmpScene  = XYscene;
        tmpWidth  = tmpXDomain;
        tmpHeight = tmpYDomain;
        if (ui->XYautolength_checkBox->isChecked()) {
            flag_autolength = 1;
        }
    }
    if (ui->tabWidget->currentIndex() == 3)    // XZscene の場合
    {
        qDebug() << "[DEBUG]func_ImgSaveAll_Clipboard XZdiag";
        tmpScene  = XZdiag_scene;
        tmpWidth  = tmpXDomain * sqrt(2);
        tmpHeight = tmpZDomain;
        if (ui->XZdiag_autolength_checkBox->isChecked()) {
            flag_autolength = 1;
        }
    }

    tmpScene->clearSelection();
    tmpScene->setSceneRect(0, -tmpHeight, tmpWidth, tmpHeight);
    if (flag_autolength == 1) {
        // AutoLength結果ビュー表示が右側にはみ出す場合でも、画像取得するように tmpWidthのサイズを大きくする
        tmpScene->setSceneRect(0, -tmpHeight, tmpWidth * 2, tmpHeight);
    }

    QImage image(tmpScene->sceneRect().size().toSize(),
                 QImage::Format_ARGB32);    // Create the image with the exact size of the shrunk scene
    image.fill(Qt::transparent);

    // そのままPasteすると大きいため縮小
    double scaleImgSize = 0.1;
    int    tmpWidth2    = (int)((double)tmpScene->sceneRect().width() * scaleImgSize);
    int    tmpHeight2   = (int)((double)tmpScene->sceneRect().height() * scaleImgSize);
    image               = image.scaled(tmpWidth2, tmpHeight2, Qt::KeepAspectRatio, Qt::FastTransformation);

    QPainter painter(&image);
    tmpScene->render(&painter);

    clip->setImage(image);
}

QStringList GeometryInputForm::func_getMaterialfromVoxPath(
    QString in_filepath)    // voxfile からマテリアル行のみ取得する。
{
    QStringList filelineList;
    QFile       file(in_filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        QString errStr = "file open error:" + file.errorString();
        qDebug() << errStr;
        // return(lines);
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString fileLine = in.readLine(0);
        if (fileLine.startsWith("CellData")) {
            break;
        }
        if (fileLine.startsWith("Version") || fileLine.endsWith("precision") || fileLine.endsWith("unitlength")) {
            continue;
        }
        filelineList << fileLine;
    }
    return (filelineList);
}

void GeometryInputForm::func_tableMaterialDefaultSet(
    QStringList in_materialLineList)    // materialテーブルのセット。マテリアルごとに表示・非表示, 色の変更などの用途。
{
    g_flag_slotMatColorCombo =
        0;    // マテリアル色コンボのスロット処理無効。自動設定の時にスロット処理されると実行エラーで落ちてしまうため。

    //[DEBUG]for(int i=0; i< in_materialLineList.size(); i++){  qDebug() <<
    //"[DEBUG]Vox3DForm.cpp-func_tableMaterialDefaultSet materialLineList" + in_materialLineList.at(i); }

    // 全行削除
    for (int row = ui->tableWidget->rowCount() - 1; row >= 0; row--) {
        ui->tableWidget->removeRow(0);
    }

    // 行追加
    //-start- [DEBUG]
    // QStringList srcList;
    // in_materialLineList << "11    mat11"; //タブ区切り　マテリアル番号とマテリアル名
    // in_materialLineList << "12    mat12";
    // in_materialLineList << "13    mat13";
    //-end- [DEBUG]
    QStringList srcList = in_materialLineList;
    for (int row = 0; row < srcList.size(); row++) {
        ui->tableWidget->insertRow(ui->tableWidget->rowCount());

        QStringList tmpList;
        tmpList = in_materialLineList.at(row).split("\t");

        //[0]列目　チェックボックス配置
        QCheckBox*   pItemCheck0 = new QCheckBox();
        QWidget*     pWidget0    = new QWidget();
        QHBoxLayout* pLayout0    = new QHBoxLayout();
        pItemCheck0->setCheckState(Qt::Checked);
        pLayout0->setAlignment(Qt::AlignCenter);
        pLayout0->addWidget(pItemCheck0);
        pWidget0->setLayout(pLayout0);
        ui->tableWidget->setCellWidget(ui->tableWidget->rowCount() - 1, 0, pWidget0);
        // チェックボックスON/OFF切り替え時のアクション
        //  int はチェック状態　引数はOFF=0 ON=2. 何行目がチェックされたか送りたいが、今の所方法が見つからない。→
        //  SLOT処理でtable全体を見て処理するしかない。（今　変わったものだけに対してのアクションはできない。）
        // connect(pItemCheck0, SIGNAL(stateChanged(int)), this, SLOT(func_tableMateriaRedrawSlot(int)));

        //[1]列目 マテリアル番号
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(""));    // 初期値
        if (tmpList.size() > 0) {
            ui->tableWidget->setItem(row, 1, new QTableWidgetItem(tmpList.at(0)));
        }

        //[2]列目 マテリアル名
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(""));    // 初期値
        if (tmpList.size() > 0) {
            ui->tableWidget->setItem(row, 2, new QTableWidgetItem(tmpList.at(1)));
        }

        //[3]列目　マテリアル色 コンボボックス配置
        // 本番時はコメントアウト解除 これから関数作成//func_tableMaterialColorAdd(row);
        // コンボボックス初期色設定
        CustomColorCombo* tmp_combo = new (CustomColorCombo);
        tmp_combo->setFocusPolicy(Qt::NoFocus);
        //        tmp_combo->addItems(QStringList() << "Black" <<"Gray" << "DarkGray" << "Lightgray" << "White" <<
        //        "Green" <<"Brown" << "Purple"
        //                             << "MediumPurple" <<"Red" << "Magenta" << "Pink" << "Orange" << "Gold" <<
        //                             "Yellow"
        //                             << "Green" <<"Greenyellow" << "Olive" << "Navy" << "Blue" << "Cyan" <<
        //                             "Lightcyan"
        //                             );
        QStringList colorList;
        colorList << "Black";    // 1
        colorList << "Gray";     // 2
        // colorList << "DarkGray";	// 3
        colorList << "Lightgray";       // 3
        colorList << "White";           // 4
        colorList << "Brown";           // 5
        colorList << "Purple";          // 6
        colorList << "MediumPurple";    // 7
        colorList << "Red";             // 8
        colorList << "Magenta";         // 9
        colorList << "Pink";            // 10
        colorList << "Orange";          // 11
        colorList << "Gold";            // 12
        colorList << "Yellow";          // 13
        colorList << "Green";           // 14
        colorList << "Greenyellow";     // 15
        colorList << "Olive";           // 16
        colorList << "Navy";            // 17
        colorList << "Blue";            // 18
        colorList << "Cyan";            // 19
        colorList << "Lightcyan";       // 20
        tmp_combo->addItems(colorList);
        // コンボボックス部品の背景色設定
        tmp_combo->setItemData(0, QColor(0, 0, 0, 255), Qt::BackgroundRole);    // 01 Black
        tmp_combo->setItemData(0, QColor(Qt::white), Qt::ForegroundRole);
        tmp_combo->setItemData(1, QColor(128, 128, 128, 255), Qt::BackgroundRole);    // 02  //Gray
        tmp_combo->setItemData(1, QColor(Qt::white), Qt::ForegroundRole);
        tmp_combo->setItemData(2, QColor(211, 211, 211, 255), Qt::BackgroundRole);    // 03 Lightgray
        tmp_combo->setItemData(3, QColor(255, 255, 255, 255), Qt::BackgroundRole);    // 04 White
        tmp_combo->setItemData(4, QColor(165, 42, 42, 255), Qt::BackgroundRole);      // 05 Brown
        tmp_combo->setItemData(4, QColor(Qt::white), Qt::ForegroundRole);
        tmp_combo->setItemData(5, QColor(128, 0, 128, 255), Qt::BackgroundRole);    // 06 Purple
        tmp_combo->setItemData(5, QColor(Qt::white), Qt::ForegroundRole);
        tmp_combo->setItemData(6, QColor(147, 112, 219, 255), Qt::BackgroundRole);    // 07 MediumPurple
        tmp_combo->setItemData(6, QColor(Qt::white), Qt::ForegroundRole);
        tmp_combo->setItemData(7, QColor(255, 0, 0, 255), Qt::BackgroundRole);        // 08 Red
        tmp_combo->setItemData(8, QColor(255, 0, 255, 255), Qt::BackgroundRole);      // 09 Magenta
        tmp_combo->setItemData(9, QColor(255, 192, 203, 255), Qt::BackgroundRole);    // 10 Pink
        tmp_combo->setItemData(10, QColor(225, 165, 0, 255), Qt::BackgroundRole);     // 11 Orange
        tmp_combo->setItemData(11, QColor(255, 215, 0, 255), Qt::BackgroundRole);     // 12 Gold
        tmp_combo->setItemData(12, QColor(255, 255, 0, 255), Qt::BackgroundRole);     // 13 Yellow
        tmp_combo->setItemData(13, QColor(0, 128, 0, 255), Qt::BackgroundRole);       // 14 Green
        tmp_combo->setItemData(13, QColor(Qt::white), Qt::ForegroundRole);
        tmp_combo->setItemData(14, QColor(173, 255, 47, 255), Qt::BackgroundRole);    // 15 Greenyellow
        tmp_combo->setItemData(15, QColor(128, 128, 0, 255), Qt::BackgroundRole);     // 16 Olive
        tmp_combo->setItemData(15, QColor(Qt::white), Qt::ForegroundRole);
        tmp_combo->setItemData(16, QColor(0, 0, 128, 255), Qt::BackgroundRole);    // 17 Navy
        tmp_combo->setItemData(16, QColor(Qt::white), Qt::ForegroundRole);
        tmp_combo->setItemData(17, QColor(0, 0, 255, 255), Qt::BackgroundRole);    // 18 Blue
        tmp_combo->setItemData(17, QColor(Qt::white), Qt::ForegroundRole);
        tmp_combo->setItemData(18, QColor(0, 255, 255, 255), Qt::BackgroundRole);      // 19 Cyan
        tmp_combo->setItemData(19, QColor(224, 255, 255, 255), Qt::BackgroundRole);    // 20 Lightcyan

        int tmpIndex = row % 10;
        tmp_combo->setCurrentIndex(tmpIndex);
        tmp_combo->setPalette(QColor(224, 255, 255, 255));    // setPaletteが効かない。。
        ui->tableWidget->setCellWidget(row, 3, tmp_combo);
        connect(tmp_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(func_tableMaterialColorSlotChanged(int)));

        //[4]列目　マテリアル色-半透明 チェックボックス配置
        QCheckBox*   pItemCheck1 = new QCheckBox();
        QWidget*     pWidget1    = new QWidget();
        QHBoxLayout* pLayout1    = new QHBoxLayout();
        pItemCheck1->setCheckState(Qt::Checked);
        pLayout1->setAlignment(Qt::AlignCenter);
        pLayout1->addWidget(pItemCheck1);
        pWidget1->setLayout(pLayout1);
        ui->tableWidget->setCellWidget(ui->tableWidget->rowCount() - 1, 4, pWidget1);
        // チェックボックスON/OFF切り替え時のアクション
        //  int はチェック状態　引数はOFF=0 ON=2. 何行目がチェックされたか送りたいが、今の所方法が見つからない。→
        //  SLOT処理でtable全体を見て処理するしかない。（今　変わったものだけに対してのアクションはできない。）
        // connect(pItemCheck0, SIGNAL(stateChanged(int)), this, SLOT(func_tableMaterialCheckAcolorSlotClicked(int)));

        // if(tmpList.at(0)== "0"){
        //     //マテリアル番号0 =
        //     3DviewerではAirは表示させないため、チェックOFFにする。チェックボックス操作も不可にする。
        //     pItemCheck0->setCheckState(Qt::Unchecked);
        //     pItemCheck0->setEnabled(false);
        //     //2,3 列目マテリアル名　背景色グレーにする。
        //     ui->tableWidget->item(row, 1)->setBackgroundColor(Qt::lightGray);
        //     ui->tableWidget->item(row, 2)->setBackgroundColor(Qt::lightGray);
        // }
    }

    func_tableMaterialDefaultSet_matnameToColorCombo();    // materialテーブルのセット。特定のマテリアルの場合、色設定コンボボックスを特定の色にセットする。

    g_flag_slotMatColorCombo =
        1;    // マテリアル色コンボのスロット処理有効に戻す。自動設定の時にスロット処理されると実行エラーで落ちてしまうため。
    // 本番時までに直す 異常終了落ちてしまう// func_tableMaterialColorSlotChanged();
    // //マテリアル色コンボボックス部品の背景色を、表示されている値と同じにする(Red, Blue）など
}

void GeometryInputForm::func_tableMaterialColorSlotChanged(int)    // マテリアル色コンボが変わった時のスロット処理
{
    // qDebug() << "[DEBUG]01 Geo.cpp-func_tableMaterialColorSlotChanged() ";
    if (g_flag_slotMatColorCombo == 0) {
        return;
    }    // 処理させないようフラグしてある場合は、以下処理なし。

    // マテリアル色コンボが変わったら、コンボボックス部品の背景色・文字色設定
    for (int row = 0; row < ui->tableWidget->rowCount(); row++) {
        CustomColorCombo* tmpcombo  = static_cast<CustomColorCombo*>(ui->tableWidget->cellWidget(row, 3));
        QString           cname     = tmpcombo->currentText();            // cname=Greeenなど
        QString           bgcolor   = g_hash_cnameToStyleColor[cname];    // bgcolor="#008000" など
        QString           fontcolor = "#000000";                          // 通常は文字色=黒
        if (g_list_whitefontBgColor.indexOf(cname) > -1) {
            fontcolor = "#ffffff";    // 見やすさのため、背景色が濃い色の場合は文字色=白
        }
        // コンボボックス部品の背景色・文字色設定
        //[DEBUG]tmpcombo->setStyleSheet("QComboBox { background-color: #008000 ; color: #ff0000 ; }"); //背景色　例：緑
        ////文字色　赤
        QString strStyle = QString("QComboBox { background-color: %1 ; color: %2 ; }").arg(bgcolor, fontcolor);
        tmpcombo->setStyleSheet(strStyle);
    }

    // qDebug() << "[DEBUG]02 Geo.cpp-func_tableMaterialColorSlotChanged() ";
    // マテリアル色コンボがかわったら再読み込みする。各関数の中で、マテリアル色コンボの色を参照しているので自動的に反映される。
    FLAG_VIEWER_UPDATE = 1;
    viewer_update();
    //    if(ui->tabWidget->currentIndex() == 0 ){
    //        FLAG_VIEWER_UPDATE = 1;
    //        viewer_update();
    //    }
    //    if(ui->tabWidget->currentIndex() == 1 ){ func_vox_make_YZgeometry(); }
    //    if(ui->tabWidget->currentIndex() == 2 ){ func_vox_make_XYgeometry(); }
    //    if(ui->tabWidget->currentIndex() == 3 ){ func_vox_make_diagonal();}
    //    if(ui->tabWidget->currentIndex() == 4 ){ func_vox_make_diagonal();}
}

void GeometryInputForm::
    func_tableMaterialDefaultSet_matnameToColorCombo()    // materialテーブルのセット。特定のマテリアルの場合、色設定コンボボックスを特定の色にセットする。
{
    QStringList matnameList, colorList;

    // ユーザーごとの.fdtdファイルが読み込める場合は、内容から色設定。
    QString fdtdpath = ui->vox_fdtd_path_lineEdit->text();
    int     openflag = 1;
    QFile   InFile01(fdtdpath);
    if (!InFile01.open(QIODevice::ReadOnly)) {
        openflag = 0;
    }
    QStringList             filelist;
    QHash<QString, QString> matnameToColorfdtdHash;
    if (openflag != 0) {
        QTextStream in01(&InFile01);
        int         flag_color = 0;
        while (!in01.atEnd()) {
            QString linestr = in01.readLine(0);
            if (linestr == "### Material List ###") {
                flag_color = 1;
                continue;
            }
            else if (flag_color == 1 && linestr.isEmpty()) {
                flag_color = 0;
                break;
            }
            else if (flag_color == 1) {
                QStringList tmpList = linestr.split("\t");
                matnameToColorfdtdHash.insert(tmpList.at(0), tmpList.at(2));
            }
        }
    }
    InFile01.close();
    //[DEBUG]//foreach(QString key, g_hash_matnameToCname.keys()){ qDebug() <<
    //"[DEBUG]Vox3DFormSubW.cpp-func_tableMaterialDefaultSet_matnameToColorCombo() key=" << key << " value=" <<
    // g_hash_matnameToCname.value(key); }

    for (int row = 0; row < ui->tableWidget->rowCount(); row++) {
        QString           matname  = ui->tableWidget->item(row, 2)->text();
        CustomColorCombo* tmpcombo = static_cast<CustomColorCombo*>(ui->tableWidget->cellWidget(row, 3));
        if (g_hash_matnameToCname.contains(matname)) {
            // officialマテリアルとして、色定義されている場合　コンボボックスの色に反映する
            tmpcombo->setCurrentText(g_hash_matnameToCname.value(matname));
        }
        if (matnameToColorfdtdHash.contains(matname)) {
            // fdtdマテリアルとして、色定義されている場合　コンボボックスの色に反映する
            tmpcombo->setCurrentText(matnameToColorfdtdHash.value(matname));
        }
        QString cname     = tmpcombo->currentText();            // 色名 Greenなど
        QString bgcolor   = g_hash_cnameToStyleColor[cname];    // bgcolor="#008000" など
        QString fontcolor = "#000000";                          // 通常は文字色=黒
        fontcolor         = "#000000";                          // 通常は文字色=黒
        if (g_list_whitefontBgColor.indexOf(cname) > -1) {
            fontcolor = "#ffffff";    // 見やすさのため、背景色が濃い色の場合は文字色=白
        }
        // コンボボックス部品の背景色・文字色設定
        //[DEBUG]tmpcombo->setStyleSheet("QComboBox { background-color: #008000 ; color: #ff0000 ; }"); //背景色　例：緑
        ////文字色　赤
        QString strStyle = QString("QComboBox { background-color: %1 ; color: %2 ; }").arg(bgcolor, fontcolor);
        tmpcombo->setStyleSheet(strStyle);
    }
}

void GeometryInputForm::func_slot_2D_acceptVoxpath(QString inStr)
{
    qDebug() << "[DEBUG]Geo.cpp-func_slot_2D_acceptVoxpath";
    ui->vox_path_lineEdit->setText("instr=" + inStr);
}

void GeometryInputForm::on_XZdiag_comboBox_angle_activated(const QString& arg1)
{
    if (ui->XZdiag_comboBox_angle->currentText() == "-45") {
        func_vox_make_diagonal();
    }
    else {
        func_vox_make_diagonal();
    }
}

void GeometryInputForm::on_XZautolength_checkBox_stateChanged(int arg1)
{
    if (ui->XZautolength_checkBox->isChecked()) {
        // チェックボックスONの時　AutoLengthタブを開く
        // ui->vox_tabWidget->setCurrentIndex(1);
        ui->vox_tabWidget->setCurrentIndex(ui->vox_tabWidget->indexOf(ui->vox_tab_AutoLength));
    }
    else {
        // チェックボックスOFFの時、自動測長結果を消すため画面更新
        FLAG_VIEWER_UPDATE = 1;
        viewer_update();
    }
}

void GeometryInputForm::on_YZautolength_checkBox_stateChanged(int arg1)
{
    if (ui->YZautolength_checkBox->isChecked()) {
        // チェックボックスONの時　AutoLengthタブを開く
        ui->vox_tabWidget->setCurrentIndex(ui->vox_tabWidget->indexOf(ui->vox_tab_AutoLength));
    }
    else {
        // チェックボックスOFFの時、自動測長結果を消すため画面更新
        on_YZRedraw_pushButton_clicked();
    }
}

void GeometryInputForm::on_XYautolength_checkBox_stateChanged(int arg1)
{
    if (ui->XYautolength_checkBox->isChecked()) {
        // チェックボックスONの時　AutoLengthタブを開く
        // ui->vox_tabWidget->setCurrentIndex(1);
        ui->vox_tabWidget->setCurrentIndex(ui->vox_tabWidget->indexOf(ui->vox_tab_AutoLength));
    }
    else {
        // チェックボックスOFFの時、自動測長結果を消すため画面更新
        on_XYRedraw_pushButton_clicked();
    }
}

void GeometryInputForm::on_XZdiag_autolength_checkBox_stateChanged(int arg1)
{
    if (ui->XZdiag_autolength_checkBox->isChecked()) {
        // チェックボックスONの時　AutoLengthタブを開く
        ui->vox_tabWidget->setCurrentIndex(ui->vox_tabWidget->indexOf(ui->vox_tab_AutoLength));
    }
    else {
        // チェックボックスOFFの時、自動測長結果を消すため画面更新
        on_XZdiag_Redraw_pushButton_clicked();
    }
}

void GeometryInputForm::on_vox_pushButton_autolength_pointRun_clicked()
{
    if (ui->vox_radioButton_autolength_pointInput->isChecked() == false) {
        return;
    }

    // 自動測長反映
    if (ui->vox_lineEdit_autolength_pointXY->text() == "") {
        QMessageBox::information(this, "error", "please input X, Y");
        return;
    }

    float   x1, y1;
    QString direction = "V";
    if (ui->vox_radioButton_autolength_directionV->isChecked()) {
        x1        = ui->vox_lineEdit_autolength_pointXY->text().toFloat();
        y1        = 0;
        direction = "V";
    }
    if (ui->vox_radioButton_autolength_directionH->isChecked()) {
        x1        = 0;
        y1        = ui->vox_lineEdit_autolength_pointXY->text().toFloat();
        direction = "H";
    }

    if (ui->tabWidget->currentIndex() == 0) {
        g_autoXZ_X         = x1;
        g_autoXZ_Y         = y1;
        g_autoXZ_direction = direction;    // 測長方向 "V" or "H"
        ui->XZautolength_checkBox->setChecked(true);
        on_XZRedraw_pushButton_clicked();
    }
    if (ui->tabWidget->currentIndex() == 1) {
        g_autoYZ_X         = x1;
        g_autoYZ_Y         = y1;
        g_autoYZ_direction = direction;
        ui->YZautolength_checkBox->setChecked(true);
        on_YZRedraw_pushButton_clicked();
    }
    if (ui->tabWidget->currentIndex() == 2) {
        g_autoXY_X         = x1;
        g_autoXY_Y         = y1;
        g_autoXY_direction = direction;
        ui->XYautolength_checkBox->setChecked(true);
        on_XYRedraw_pushButton_clicked();
    }
    if (ui->tabWidget->currentIndex() == 3) {
        g_autoXZdiag_X         = x1;
        g_autoXZdiag_Y         = y1;
        g_autoXZdiag_direction = direction;
        ui->XZdiag_autolength_checkBox->setChecked(true);
        on_XZdiag_Redraw_pushButton_clicked();
    }
}

void GeometryInputForm::on_vox_radioButton_autolength_directionV_clicked(bool checked)
{
    func_cellvalueChanged_setAutoLength();    // GUI入力値変更時の処理(セルサイズ、アレイ数)　自動測長(AutoLength)GUIをセットする
}

void GeometryInputForm::on_vox_radioButton_autolength_directionH_clicked(bool checked)
{
    func_cellvalueChanged_setAutoLength();    // GUI入力値変更時の処理(セルサイズ、アレイ数)　自動測長(AutoLength)GUIをセットする
}

void GeometryInputForm::on_vox_comboBox_autolength_color_currentIndexChanged(const QString& arg1)
{
    if (g_flag_slotAutorun == 0) {
        return;
    }    // スロット処理させる:1 or
         // させない:0。（ユーザー操作の時だけスロット処理したい。自動設定のとき（自動=初期設定の時など）はスロット処理したくない）
    // 画面更新
    if (ui->tabWidget->currentIndex() == 0) {
        on_XZRedraw_pushButton_clicked();
    }
    if (ui->tabWidget->currentIndex() == 1) {
        on_YZRedraw_pushButton_clicked();
    }
    if (ui->tabWidget->currentIndex() == 2) {
        on_XYRedraw_pushButton_clicked();
    }
    if (ui->tabWidget->currentIndex() == 3) {
        on_XZdiag_Redraw_pushButton_clicked();
    }
}

void GeometryInputForm::on_vox_checkBox_autolength_viewResult_clicked(bool checked)
{
    if (g_flag_slotAutorun == 0) {
        return;
    }    // スロット処理させる:1 or
         // させない:0。（ユーザー操作の時だけスロット処理したい。自動設定のとき（自動=初期設定の時など）はスロット処理したくない）
    // 画面更新
    if (ui->tabWidget->currentIndex() == 0) {
        on_XZRedraw_pushButton_clicked();
    }
    if (ui->tabWidget->currentIndex() == 1) {
        on_YZRedraw_pushButton_clicked();
    }
    if (ui->tabWidget->currentIndex() == 2) {
        on_XYRedraw_pushButton_clicked();
    }
    if (ui->tabWidget->currentIndex() == 3) {
        on_XZdiag_Redraw_pushButton_clicked();
    }
}

void GeometryInputForm::on_vox_comboBox_autolength_fontsize_currentTextChanged(const QString& arg1)
{
    if (g_flag_slotAutorun == 0) {
        return;
    }    // スロット処理させる:1 or
         // させない:0。（ユーザー操作の時だけスロット処理したい。自動設定のとき（自動=初期設定の時など）はスロット処理したくない）
    // 画面更新
    if (ui->tabWidget->currentIndex() == 0) {
        on_XZRedraw_pushButton_clicked();
    }
    if (ui->tabWidget->currentIndex() == 1) {
        on_YZRedraw_pushButton_clicked();
    }
    if (ui->tabWidget->currentIndex() == 2) {
        on_XYRedraw_pushButton_clicked();
    }
    if (ui->tabWidget->currentIndex() == 3) {
        on_XZdiag_Redraw_pushButton_clicked();
    }
}

void GeometryInputForm::on_vox_checkBox_autolength_viewResult_mat_clicked(bool checked)
{
    if (g_flag_slotAutorun == 0) {
        return;
    }    // スロット処理させる:1 or
         // させない:0。（ユーザー操作の時だけスロット処理したい。自動設定のとき（自動=初期設定の時など）はスロット処理したくない）
    // 画面更新
    if (ui->tabWidget->currentIndex() == 0) {
        on_XZRedraw_pushButton_clicked();
    }
    if (ui->tabWidget->currentIndex() == 1) {
        on_YZRedraw_pushButton_clicked();
    }
    if (ui->tabWidget->currentIndex() == 2) {
        on_XYRedraw_pushButton_clicked();
    }
    if (ui->tabWidget->currentIndex() == 3) {
        on_XZdiag_Redraw_pushButton_clicked();
    }
}

void GeometryInputForm::on_vox_pushButton_help_clicked()
{
    // ヘルプボタン押下　マニュアル表示
    qDebug() << "[DEBUG]on_pushButton_help_clicked()";
    QString workDirPath    = "C:/Temp/FDTDsolver_work/vox_2Dviewer/";
    QString exefileDirPath = QFileInfo(QCoreApplication::applicationFilePath()).absolutePath();
    QString orgFilepath    = exefileDirPath + "/imgHelp/manual_vox_2Dviewer.pdf";
    QString manualFilepath = workDirPath + "/manual_vox_2Dviewer.pdf";

    if (!QFile(manualFilepath).exists()) {
        QFile(manualFilepath).remove();
    }    // 古いファイルを削除
    QDir dir(workDirPath);
    if (!dir.exists()) {
        dir.mkpath(workDirPath);
    }    // コピー先フォルダがなければ作成する

    // マニュアルを作業フォルダにコピーする 　openUrlの時、パスに日本語が含まれていると実行できないため
    QString fromFile = orgFilepath;
    QString toFile   = manualFilepath;
    if (QFile(fromFile).exists()) {
        if (QFile(toFile).exists()) {
            QFile(toFile).remove();
        }
        QFile::copy(fromFile, toFile);    // マニュアルを作業フォルダにコピーする
    }
    else {
        QMessageBox::information(this, "notice", "no-file: " + fromFile);
        return;
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(manualFilepath));    // マニュアルを開く
}

void GeometryInputForm::on_Ypos_pixCenter_pushButton_clicked()
{
    // XZビュー 画素中央ボタン押下時の処理
    if (ui->cellY_lineEdit->text().isEmpty() || ui->cellY_lineEdit->text().indexOf(" ") > -1) {
        QMessageBox::information(this, "notice", "please input cellY.");
    }

    int CellY = ui->cellY_lineEdit->text().toInt();
    int Mesh  = ui->meshsize_Value->text().toInt();

    int centerY = (CellY / 2) / Mesh * Mesh + CellY * Ycount_pixCenter;
    ui->Ycoordinate_Slider->setValue(centerY);
    ui->Ycoordinate_spin_lineEdit->setText(QString::number(centerY));
    on_XZRedraw_pushButton_clicked();

    Ycount_pixCenter++;
    if (Ycount_pixCenter == ui->arrayY_lineEdit->text().toInt()) {
        Ycount_pixCenter = 0;
    }
}
void GeometryInputForm::on_Xpos_pixCenter_pushButton_clicked()
{
    // YZビュー 画素中央ボタン押下時の処理
    int CellX = ui->cellX_lineEdit->text().toInt();
    int Mesh  = ui->meshsize_Value->text().toInt();

    int centerX = (CellX / 2) / Mesh * Mesh + CellX * Xcount_pixCenter;
    ui->Xcoordinate_Slider->setValue(centerX);
    ui->Xcoordinate_spin_lineEdit->setText(QString::number(centerX));
    on_YZRedraw_pushButton_clicked();

    Xcount_pixCenter++;
    if (Xcount_pixCenter == ui->arrayX_lineEdit->text().toInt()) {
        Xcount_pixCenter = 0;
    }
}

void GeometryInputForm::on_vox_pushButton_autolength_pointCenterRun_clicked()    // 自動測長　画素中央での実行ボタン押下
{
    int tabnum = ui->tabWidget->currentIndex();

    // 実行前チェック
    if ((tabnum == 0 || tabnum == 1) && ui->vox_radioButton_autolength_directionH->isChecked()) {
        QMessageBox::information(this, "notice", TR("XZ,YZビュー：横方向測長で画素中央指定はできません。"));
        return;
    }
    if (ui->tabWidget->currentIndex() == 3) {    // XZdiagでは画素中央指定測長機能なし
        QMessageBox::information(this, "notice", TR("diagonalビュー:画素中央指定はできません。"));
        return;
    }

    // 自動測長 実行
    QString direction = "V";
    float   x1, y1;
    if (ui->vox_radioButton_autolength_directionV->isChecked()) {
        x1        = ui->vox_comboBox_autolength_pointCenter->currentText().toFloat();
        y1        = 0;
        direction = "V";
    }
    if (ui->vox_radioButton_autolength_directionH->isChecked()) {
        x1        = 0;
        y1        = ui->vox_comboBox_autolength_pointCenter->currentText().toFloat();
        direction = "H";
    }

    if (ui->tabWidget->currentIndex() == 0) {
        g_autoXZ_X         = x1;
        g_autoXZ_Y         = y1;
        g_autoXZ_direction = direction;    // 測長方向 "V" or "H"
        ui->XZautolength_checkBox->setChecked(true);
        on_XZRedraw_pushButton_clicked();
    }
    if (ui->tabWidget->currentIndex() == 1) {
        g_autoYZ_X         = x1;
        g_autoYZ_Y         = y1;
        g_autoYZ_direction = direction;
        ui->YZautolength_checkBox->setChecked(true);
        on_YZRedraw_pushButton_clicked();
    }
    if (ui->tabWidget->currentIndex() == 2) {
        g_autoXY_X         = x1;
        g_autoXY_Y         = y1;
        g_autoXY_direction = direction;
        ui->XYautolength_checkBox->setChecked(true);
        on_XYRedraw_pushButton_clicked();
    }
    // if(ui->tabWidget->currentIndex()==3){ //XZdiagでは画素中央指定測長機能なし
    //     ui->XZdiag_autolength_checkBox->setChecked(true);
    //     on_XZdiag_Redraw_pushButton_clicked();
    // }
}

void GeometryInputForm::on_cellX_lineEdit_editingFinished()
{
    qDebug() << "[DEBUG]GEo.cpp-on_cellX_lineEdit_editingFinished()";
    if (ui->tabWidget->currentIndex() == 0) {
        Ycount_pixCenter = 0;
    }    // XZビュー 画素中央ボタン押下時処理のリセット
    if (ui->tabWidget->currentIndex() == 1) {
        Xcount_pixCenter = 0;
    }    // YZビュー 画素中央ボタン押下時処理のリセット

    // GUI入力値変更時の処理(セルサイズ、アレイ数)　自動測長(AutoLength)GUIをセットする
    func_cellvalueChanged_setAutoLength();
}

void GeometryInputForm::on_cellY_lineEdit_editingFinished()
{
    qDebug() << "[DEBUG]GEo.cpp-on_cellX_lineEdit_editingFinished()";
    if (ui->tabWidget->currentIndex() == 0) {
        Ycount_pixCenter = 0;
    }    // XZビュー 画素中央ボタン押下時処理のリセット
    if (ui->tabWidget->currentIndex() == 1) {
        Xcount_pixCenter = 0;
    }    // YZビュー 画素中央ボタン押下時処理のリセット

    // GUI入力値変更時の処理(セルサイズ、アレイ数)　自動測長(AutoLength)GUIをセットする
    func_cellvalueChanged_setAutoLength();
}

void GeometryInputForm::on_arrayX_lineEdit_editingFinished()
{
    if (ui->tabWidget->currentIndex() == 0) {
        Ycount_pixCenter = 0;
    }    // XZビュー 画素中央ボタン押下時処理のリセット
    if (ui->tabWidget->currentIndex() == 1) {
        Xcount_pixCenter = 0;
    }    // YZビュー 画素中央ボタン押下時処理のリセット

    // GUI入力値変更時の処理(セルサイズ、アレイ数)　自動測長(AutoLength)GUIをセットする
    func_cellvalueChanged_setAutoLength();
}

void GeometryInputForm::on_arrayY_lineEdit_editingFinished()
{
    if (ui->tabWidget->currentIndex() == 0) {
        Ycount_pixCenter = 0;
    }    // XZビュー 画素中央ボタン押下時処理のリセット
    if (ui->tabWidget->currentIndex() == 1) {
        Xcount_pixCenter = 0;
    }    // YZビュー 画素中央ボタン押下時処理のリセット

    // GUI入力値変更時の処理(セルサイズ、アレイ数)　自動測長(AutoLength)GUIをセットする
    func_cellvalueChanged_setAutoLength();
}

void GeometryInputForm::func_cellvalueChanged_setAutoLength()
{    // GUI入力値変更時の処理(セルサイズ、アレイ数)　自動測長(AutoLength)GUIをセットする
    // GUIの設定
    int tabnum = ui->tabWidget->currentIndex();

    ui->vox_comboBox_autolength_pointCenter->clear();

    if (tabnum == 0) {
        ui->label_autolength_point->setText("X");
        if (ui->vox_radioButton_autolength_directionH->isChecked()) {
            ui->label_autolength_point->setText("Z");
        }
    }

    if (tabnum == 1) {
        ui->label_autolength_point->setText("Y");
        if (ui->vox_radioButton_autolength_directionH->isChecked()) {
            ui->label_autolength_point->setText("Z");
        }
    }

    if (tabnum == 2) {
        ui->label_autolength_point->setText("X");
        if (ui->vox_radioButton_autolength_directionH->isChecked()) {
            ui->label_autolength_point->setText("Y");
        }
    }

    if (tabnum == 3) {
        ui->label_autolength_point->setText("X");
        if (ui->vox_radioButton_autolength_directionH->isChecked()) {
            ui->label_autolength_point->setText("Z");
        }
    }

    //-----------------------------
    // 画面にAutoLength結果を表示する
    // GUI でcellsize, cellAry入力された時、AutoLengthの画素中央のコンボ値を変更する
    if (ui->cellX_lineEdit->text().isEmpty() || ui->cellX_lineEdit->text().indexOf(" ") > -1
        || ui->cellY_lineEdit->text().isEmpty() || ui->cellY_lineEdit->text().indexOf(" ") > -1
        || ui->arrayX_lineEdit->text().isEmpty() || ui->arrayX_lineEdit->text().indexOf(" ") > -1
        || ui->arrayY_lineEdit->text().isEmpty() || ui->arrayY_lineEdit->text().indexOf(" ") > -1) {
        // 処理スキップ 入力内容が空白か、空白含む場合は処理しない。
    }
    else {
        int CellX  = ui->cellX_lineEdit->text().toInt();
        int CellY  = ui->cellY_lineEdit->text().toInt();
        int arrayX = ui->arrayX_lineEdit->text().toInt();
        int arrayY = ui->arrayY_lineEdit->text().toInt();
        int Mesh   = ui->meshsize_Value->text().toInt();

        ui->vox_comboBox_autolength_pointCenter->clear();

        // 入力値：XZ、YZビューは値のみ。XYビューはX=xxx Y=xxx とする　※XZ・YZでの測長は縦方向のみ。横方向はできない。
        // XYは縦・横ともできる。
        if (tabnum == 0) {    // XZビューの場合 縦方向の測長のみ。center指定　横方向測長はできない。
            QStringList valueList;
            for (int i = 0; i < arrayX; i++) {
                int centerValue = (CellX / 2) / Mesh * Mesh + CellX * i;
                valueList << QString::number(centerValue);
            }
            ui->vox_comboBox_autolength_pointCenter->addItems(valueList);
        }

        if (tabnum == 1) {    // YZビューの場合 縦方向の測長のみ。center指定　横方向測長はできない。
            QStringList valueList;
            for (int i = 0; i < arrayY; i++) {
                int centerValue = (CellY / 2) / Mesh * Mesh + CellY * i;
                valueList << QString::number(centerValue);
            }
            ui->vox_comboBox_autolength_pointCenter->addItems(valueList);
        }

        // 入力値：XZ、YZビューは値のみ。XYビューはX=xxx Y=xxx
        // とする　※XZ・YZでの測長は縦方向のみ。横方向はできない　XYは縦・横ともできる。
        if (tabnum == 2) {    // XYビューの場合　縦・横とも測長できる。
            QStringList valueList;
            if (ui->vox_radioButton_autolength_directionV->isChecked()) {    // 縦方向測長
                for (int i = 0; i < arrayX; i++) {
                    int centerValue = (CellX / 2) / Mesh * Mesh + CellX * i;
                    valueList << QString::number(centerValue);
                }
            }
            if (ui->vox_radioButton_autolength_directionH->isChecked()) {    // 横方向測長
                for (int i = 0; i < arrayY; i++) {
                    int centerValue = (CellY / 2) / Mesh * Mesh + CellY * i;
                    valueList << QString::number(centerValue);
                }
            }
            ui->vox_comboBox_autolength_pointCenter->addItems(valueList);
        }

        if (tabnum == 3) {    // 斜め断面の場合
            ui->vox_comboBox_autolength_pointCenter->clear();
        }
    }
}

void GeometryInputForm::func_accepted_byMW_viewStart()
{                             // voxfile_viewer.exeがコマンド実行された場合の処理
    func_GUI_defaultSet();    // GUI、グローバル変数を初期状態にする

    // QString DEBUGstr = QString("[DEBUG]02test Geo.cpp-func_accepted_byMW_viewStart\n GE_cmdparamMode=%1\n
    // GE_voxfilepath=%2\n GE_fdtdfilepath=%3\n GE_cellsizeX=%4\n GE_cellsizeY=%5\n GE_cellarrayX=%6\n
    // GE_cellarrayY=%7").arg(GE_cmdparamMode, GE_voxfilepath, GE_fdtdfilepath, QString::number(GE_cellsizeX),
    // QString::number(GE_cellsizeY), QString::number(GE_cellarrayX),  QString::number(GE_cellarrayY));
    // QMessageBox::information(this, "notice", DEBUGstr);
    // qDebug() << DEBUGstr;
    ui->vox_path_lineEdit->setText(GE_voxfilepath);
    ui->vox_fdtd_path_lineEdit->setText(GE_fdtdfilepath);
    ui->cellX_lineEdit->setText(QString::number(GE_cellsizeX));
    ui->cellY_lineEdit->setText(QString::number(GE_cellsizeY));
    ui->arrayX_lineEdit->setText(QString::number(GE_cellarrayX));
    ui->arrayY_lineEdit->setText(QString::number(GE_cellarrayY));

    if (QFile(GE_voxfilepath).exists()) {
        func_vox_load(GE_voxfilepath);
    }
    else {
        QMessageBox::information(this, "notice", "file is no exsist.");
    }

    if (ui->tabWidget->currentIndex() == 0 && (!ui->arrayX_lineEdit->text().isEmpty())
        && (!ui->arrayX_lineEdit->text().isEmpty()) && (!ui->arrayX_lineEdit->text().isEmpty())
        && (!ui->arrayX_lineEdit->text().isEmpty())) {
        on_Ypos_pixCenter_pushButton_clicked();    // 画素中央ボタン押下
        func_cellvalueChanged_setAutoLength();    // GUI入力値変更時の処理(セルサイズ、アレイ数)　自動測長(AutoLength)GUIをセットする
    }

    // zoomAll 全体表示
    int dX = ui->domainX_Value->text().toInt();
    int dZ = ui->domainZ_Value->text().toInt();
    XZscene->setSceneRect(0, 0, dX, -dZ);
    ui->XZgraphicsView->fitInView(XZscene->itemsBoundingRect(), Qt::KeepAspectRatio);    // 通常本番用
    // ↓E2viwer同様に変更してみたけどNG
    // ui->XZgraphicsView->fitInView(XZscene->sceneRect(), Qt::KeepAspectRatio); //[DEBUG]お試し NG zoomInしすぎた状態
    // 大きくなりすぎる ui->XZgraphicsView->transform().m11(); ↑E2viwer同様に変更してみたけどNG

    // zoomAllだけでは、小さく表示されてしまうため。zoomInで調節。
    on_XZzoomall_pushButton_clicked();
    for (int i = 0; i < 15; i++) {
        on_XZzoomin_pushButton_clicked();
    }
}

void GeometryInputForm::on_vox_fdtd_load_pushButton_clicked()
{
    //.fdtdファイル読込み、入力欄にセットのみの処理。 .fdtd情報の反映はユーザーにRedrawボタン押下してもらう。
    // ファイル　ユーザー選択
    // 既にvoxファイルが、loadファイルされている場合、そのファイルがあるフォルダをファイル選択ダイアログの起点フォルダとする
    QString tmpDirPath = "c:/";
    if (ui->vox_path_lineEdit->text().isEmpty() == 0) {
        QString tmpdir = QFileInfo(QFile(ui->vox_path_lineEdit->text())).absolutePath();
        if (QDir(tmpdir).exists()) {
            tmpDirPath = QFileInfo(ui->vox_path_lineEdit->text()).absolutePath();
        }
    }
    // 既にfdtdファイルがloadファイルされている場合、そのファイルがあるフォルダをファイル選択ダイアログの起点フォルダとする
    if (ui->vox_fdtd_path_lineEdit->text().isEmpty() == 0) {
        QString tmpdir = QFileInfo(QFile(ui->vox_fdtd_path_lineEdit->text())).absolutePath();
        if (QDir(tmpdir).exists()) {
            tmpDirPath = QFileInfo(ui->vox_fdtd_path_lineEdit->text()).absolutePath();
        }
    }

    if (ui->vox_path_lineEdit->text().isEmpty() == 0) {
        QString tmpdir = QFileInfo(QFile(ui->vox_path_lineEdit->text())).absolutePath();
        if (QDir(tmpdir).exists()) {
            tmpDirPath = QFileInfo(ui->vox_path_lineEdit->text()).absolutePath();
        }
    }
    // ファイル　ユーザー選択
    QString filePath = "";
    // filePath = QFileDialog::getOpenFileName(this, tr("Select file"), "Desktop", "*");
    filePath = QFileDialog::getOpenFileName(this, tr("Select file"), tmpDirPath, tr("fdtd Files (*.fdtd)"));
    if (filePath.isEmpty()) {
        return;
    }    // ファイル選択ダイアログで、ユーザーがキャンセルボタンを押したときは、ここで処理終了する。
    if (!QFile(filePath).exists()) {
        return;
    }    // ファイル存在なしなら、この後の処理なし

    ui->vox_fdtd_path_lineEdit->setText(filePath);
    // GE_fdtdfilepath = filePath;  //2022.04.27 今時点では 2Dviewer
    // 3Dviewerで異なるvoxfileも読むこと想定して、共通変数GE_には代入しない。 GE_cellsizeX = 0; GE_cellsizeY = 0;
    // GE_cellarrayX = 0;
    // GE_cellarrayY = 0;
    QHash<QString, QString> hash_fdtdHeader;
    CommonSubFunc           mCommonSubFunc;
    hash_fdtdHeader = mCommonSubFunc.func_fdtd_readHeader(filePath);
    if (hash_fdtdHeader.contains("CellX")) {
        // GE_cellsizeX =  hash_fdtdHeader["CellX"].toInt(); //2022.04.27 今時点では 2Dviewer
        // 3Dviewerで異なるvoxfileも読むこと想定して、共通変数GE_には代入しない。
        ui->cellX_lineEdit->setText(hash_fdtdHeader["CellX"]);
    }
    if (hash_fdtdHeader.contains("CellY")) {
        ui->cellY_lineEdit->setText(hash_fdtdHeader["CellY"]);
    }
    if (hash_fdtdHeader.contains("ArrayX")) {
        ui->arrayX_lineEdit->setText(hash_fdtdHeader["ArrayX"]);
    }
    if (hash_fdtdHeader.contains("ArrayY")) {
        ui->arrayY_lineEdit->setText(hash_fdtdHeader["ArrayY"]);
    }

    // if(ui->tabWidget->currentIndex() == 0 && (! ui->arrayX_lineEdit->text().isEmpty()) && (!
    // ui->arrayX_lineEdit->text().isEmpty()) && (! ui->arrayX_lineEdit->text().isEmpty()) && (!
    // ui->arrayX_lineEdit->text().isEmpty())  ){
    //     on_Ypos_pixCenter_pushButton_clicked(); //画素中央ボタン押下
    //     func_cellvalueChanged_setAutoLength();
    //     //GUI入力値変更時の処理(セルサイズ、アレイ数)　自動測長(AutoLength)GUIをセットする
    // }

    // materialテーブルのセット。特定のマテリアルの場合、色設定コンボボックスを特定の色にセットする。
    func_tableMaterialDefaultSet_matnameToColorCombo();
}

// void GeometryInputForm::on_vox_pushButton_DEBUG_clicked()
//{
//
//      //exeから引数指定で起動された場合の処理
//     //sub_voxViewer.exe mode=2DviewerRun voxpath=C:/tmp/tmp_20220304/kuroda-qt-0225-01-sampe503_a0000.vox
//     fdtdpath=C:/tmp/tmp_20220304/z_DEBUG.fdtd cellsize=1400x1400 cellarray=2x2
//    GE_cmdparamMode = "2DviewerRun";
//    //GE_voxfilepath =
//    "C:/kuroda/work/00_Data_sim用ファイル/kuroda-qt-0225-01-sample503/a0000/kuroda-qt-0225-01-sample503_a0000.vox";
//    //GE_fdtdfilepath = "C:/kuroda/work/00_Data_sim用ファイル/kuroda-qt-0225-01-sample503/z_DEBUG.fdtd";
//    GE_voxfilepath = "C:/tmp/tmp_DEBUG_2DcmdRun/kuroda-qt-0225-01-sample503_a0000.vox";
//    GE_fdtdfilepath = "C:/tmp/tmp_DEBUG_2DcmdRun/z_DEBUG.fdtd";
//    GE_cellsizeX=1400;
//    GE_cellsizeY=1400;
//    GE_cellarrayX=2;
//    GE_cellarrayY=2;
//
//    //exeから引数指定で起動された場合の処理
//    func_accepted_byMW_viewStart();
// }

void GeometryInputForm::on_vox_pushButton_DEBUG_clicked()
{
    // テスト
    CommonSubFunc mCommonSubFunc;

    QHash<QString, QString> matnameToCnameHash = mCommonSubFunc.func_makehash_matnameToCname();
    qDebug() << "[DEBUG]Geo.cpp-on_vox_pushButton_DEBUG_clicked matnameToCnameHash[Air]=" << matnameToCnameHash["Air"];

    QHash<QString, QColor> cnameToQtColorHash = mCommonSubFunc.func_makehash_cnameToQtColor();
    // qDebug() << "[DEBUG]Geo.cpp-on_vox_pushButton_DEBUG_clicked cnameToQtColorHash[black]=" <<
    // cnameToStyleColorHash["Magenta"];

    QHash<QString, QString> cnameToStyleColorHash = mCommonSubFunc.func_makehash_cnameToStyleColor();
    qDebug() << "[DEBUG]Geo.cpp-on_vox_pushButton_DEBUG_clicked stylecolor[Green]=" << cnameToStyleColorHash["Green"];

    //[DEBUG] 効かない palletはコードでウィジェット部品定義する場合の初期設定時のみ有効かも
    // QPalette palette = ui->vox_pushButton_DEBUG->palette();
    // palette.setColor(QPalette::Window, "#ff0000");
    // palette.setColor(QPalette::WindowText, Qt::green);
    // ui->vox_pushButton_DEBUG->setAutoFillBackground(true);
    // ui->vox_pushButton_DEBUG->setPalette(palette);

    //[DEBUG] 既存ウィジェット部品の見た目変更はstylesheetだけかも
    // ui->vox_pushButton_DEBUG->setStyleSheet("background:#ff0000; color:green;");
    QString strColor1 = cnameToStyleColorHash[matnameToCnameHash["UV-SiN_Woollam"]];
    QString strColor2 = cnameToStyleColorHash["Red"];
    qDebug() << "[DEBUG]Geo.cpp-on_vox_pushButton_DEBUG_clicked strcolor=" << cnameToStyleColorHash["Green"];

    QString colortext = QString("background:%1; color:%2").arg(strColor1, strColor2);
    ui->vox_pushButton_DEBUG->setStyleSheet(colortext);
}
