#include "MainWindow3DViewer.h"
#include "ui_MainWindow3DViewer.h"

#include <QDockWidget>
#include "Vox3DForm.h"

#include "Vox3DForm.h"
// #include "Zsub2Form.h"
#include "GeometryInputForm.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QSplashScreen>
#include <QTextStream>
// #include <QtScript>

QString GE_cmdparamMode = "";
QString GE_voxfilepath  = "";
QString GE_fdtdfilepath = "";
int     GE_cellsizeX    = 1;
int     GE_cellsizeY    = 1;
int     GE_cellarrayX   = 1;
int     GE_cellarrayY   = 1;

MainWindow3DViewer::MainWindow3DViewer(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow3DViewer)
{
    /*
    ui->setupUi(this);

    GE_voxfilepath  = "";
    GE_fdtdfilepath = "";

    // コマンドラインから.exeファイル実行された場合、引数取得
    // 引数は下記いずれかが指定される場合に対応
    // 　引数例: cmd=2DviewerRun
    //  引数例: voxpath=c:/xxx/xxx.vox
    //  引数例: cellsize=1000x1000
    //  引数例: cellarray=2x2
    QString tmpstr = "";
    // 引数0個目はコマンド文字(～.exe)自体が取得される。　引数は i=1 以上。
    if (QCoreApplication::arguments().count() > 1) {
        for (int i = 1; i < QCoreApplication::arguments().count(); i++) {
            QString tmparg = QCoreApplication::arguments().at(i);
            G_inCmdargList << tmparg;    // 後で別関数で処理したい場合のため、値保持する。
            if (tmparg.startsWith("mode=")) {
                GE_cmdparamMode = tmparg.replace("mode=", "");
            }
            if (tmparg.startsWith("voxpath=")) {
                GE_voxfilepath = tmparg.replace("voxpath=", "");
            }
            if (tmparg.startsWith("fdtdpath=")) {
                GE_fdtdfilepath = tmparg.replace("fdtdpath=", "");
            }
            if (tmparg.startsWith("cellsize=")) {
                QString tmpstr = tmparg.replace("cellsize=", "");
                if (!tmpstr.isEmpty()) {
                    QStringList tmpList = tmpstr.split("x");
                    if (tmpList.count() >= 2) {
                        GE_cellsizeX = tmpList.at(0).toInt();
                        GE_cellsizeY = tmpList.at(1).toInt();
                    }
                }
            }
            if (tmparg.startsWith("cellarray=")) {
                QString tmpstr = tmparg.replace("cellarray=", "");
                if (!tmpstr.isEmpty()) {
                    QStringList tmpList = tmpstr.split("x");
                    if (tmpList.count() >= 2) {
                        GE_cellarrayX = tmpList.at(0).toInt();
                        GE_cellarrayY = tmpList.at(1).toInt();
                    }
                }
            }
        }
    }
    //-end- 引数

    //[DEBUG]QMessageBox::information(this, "notice", "[DEBUG]MainWin.cpp-constructor() arg=" + tmpstr);
    // QMessageBox::information(this, "notice", "[DEBUG]MainWin.cpp-constructor() \nGE_voxfilepath=" + GE_voxfilepath +
    // " \nGE_fdtdfilepath=" + GE_fdtdfilepath);

    this->setWindowTitle("voxfile_viewer");
    setWindowIcon(QIcon("://images/eyeris_icon.png"));    // アイコン設定(各ウィンドウバー上部)

    // 3Dviewer
    mVox3DForm         = new Vox3DForm(this);
    SubWindowVox3DForm = ui->mdiArea->addSubWindow(mVox3DForm);
    SubWindowVox3DForm->setGeometry(100, 20, 1500, 800);
    SubWindowVox3DForm->raise();    //(2022.01.11時点　2Dのみ表示するため、一旦コメントアウト）
    // SubWindowVox3DForm->close(); //2022.01.11時点　2Dのみ表示するため追加

    // 2Dviewer
    mGeometryInputForm         = new GeometryInputForm(this);
    SubWindowGeometryInputForm = ui->mdiArea->addSubWindow(mGeometryInputForm);
    SubWindowGeometryInputForm->setGeometry(0, 0, 1500, 800);
    SubWindowGeometryInputForm->raise();

    SubWindowVox3DForm
        ->raise();    // 3Dviewerを 2Dviewerより前面に表示する  //2022.01.11時点　2Dのみ表示するためコメントアウト

    // シグナルスロット
    connect(this, &MainWindow::sendto3D_sendVoxpath, mVox3DForm, &Vox3DForm::func_slot_3D_acceptVoxpath);
    connect(this, &MainWindow::sendto2D_sendVoxpath, mGeometryInputForm,
            &GeometryInputForm::func_slot_2D_acceptVoxpath);
    // connect( mSubform02, &Subform02::sendtoFM02_act01 , mSubform01, &Subform01::func_slot_FM01_act02 );

    // シグナルスロット voxfile_view.exeが、windowsコマンドから引数付きで実行された時の処理
    connect(this, &MainWindow::sendtoGI_viewStart, mGeometryInputForm,
            &GeometryInputForm::func_accepted_byMW_viewStart);    // voxfile_viewer.exe
                                                                  // がコマンドラインから引数付きで実行された時の処理

    QStringList voxFileComboList, voxChildWinComboList;
    voxChildWinComboList << "3Dviewer"
                         << "2Dviewer";
    ui->comboBox_voxChildWin->addItems(voxChildWinComboList);

    //-start- 2022.01.11時点　2Dのみ表示するため表示しない
    ui->menubar->actions().at(0)->setVisible(
        false);    // [0]番目のloadだけのつもりだったが、メニューごと表示されなくなる。。
    ui->menuWindow_childWin->menuAction()->setVisible(false);
    ui->lineEdit_voxpath->setVisible(false);
    ui->pushButton_voxpathLoad->setVisible(false);
    ui->comboBox_voxChildWin->setVisible(false);
    ui->MW_message_label->setVisible(false);
    ui->MW_lineEdit_DEBUG->setVisible(false);
    ui->MW_pushButton_DEBUG->setVisible(false);
    //-end- 2022.01.11時点　2Dのみ表示するため表示しない

    //-start- voxfile_viewer.exe がコマンドラインから引数付きで実行された時の処理
    //-start- DEBUG
    // GE_cmdparamMode="2DviewerRun";
    // GE_voxfilepath = "C:/kuroda/work/00_Data_vox/kuroda-qt-0225-01-sample503_a0000.vox";
    // GE_cellsizeX=1400;
    // GE_cellsizeY=1400;
    // GE_cellarrayX=2;
    // GE_cellarrayY=2;
    //-end- DEBUG
    // QString DEBUGstr = QString("[DEBUG]Main.cpp-constructor\n GE_voxfilepath=%1\n GE_cellsizeX=%2\n GE_cellsizeX=%3
    // GE_cellsizeX=%4 GE_cellarrayX=%5\n GE_cellarrayY=%6").arg(GE_voxfilepath, QString::number(GE_cellsizeX),
    // QString::number(GE_cellsizeY), QString::number(GE_cellarrayX),  QString::number(GE_cellarrayY));
    // QMessageBox::information(this, "notice", DEBUGstr);
    if (GE_cmdparamMode == "2DviewerRun"
        && GE_voxfilepath != "") {    // voxfile_viewer.exeがコマンド実行された場合。引数があれば、2Dviewerの描画開始。
        emit sendtoGI_viewStart();
    }
    //-end- voxfile_viewer.exe がコマンドラインから引数付きで実行された時の処理

    this->showMaximized();    // ウィンドウの最大化ボタン押すのと同じ状態になる。
    // SubWindowGeometryInputForm->showMaximized();
    // //2Dviewerのみリリースのため。3Dviewerリリースする時は、この行を削除する。
    */

    setWindowTitle(QApplication::applicationName() + " (build " + BUILD_DATE + ")");
    /*
    QMenuBar* menuBar    = new QMenuBar(this);
    QMenu*    fileMenu   = new QMenu("File", menuBar);
    QAction*  exitAction = new QAction("Close", fileMenu);

    fileMenu->addAction(exitAction);
    menuBar->addMenu(fileMenu);

    QObject::connect(exitAction, &QAction::triggered, this, []() { qApp->quit(); });

    setMenuBar(menuBar);
    */
}

MainWindow3DViewer::~MainWindow3DViewer()
{
    // delete ui;
}

Vox3DForm* MainWindow3DViewer::createVox3DForm(QMainWindow*& main_window)
{
    main_window    = new MainWindow3DViewer;
    auto vox3dform = new Vox3DForm(main_window);
    main_window->setCentralWidget(vox3dform);

    /// 表示しながらサイズ変更が起きるので対策
    main_window->setUpdatesEnabled(false);
    main_window->show();
    QApplication::processEvents();
    main_window->setUpdatesEnabled(true);

    return vox3dform;
}

void MainWindow3DViewer::deleteVox3DForm(Vox3DForm* vox3dform)
{
    delete vox3dform;
}

void MainWindow3DViewer::showArguments(Vox3DForm* vox3dform)
{
    /// 引数対応
    QString     vox_file, fdtd_file;
    QStringList file_list;
    const auto& arguments = QCoreApplication::arguments();
    if (arguments.count() > 1) {
        for (int ic = 1; ic < arguments.count(); ++ic) {
            const auto& tmparg = arguments[ic];
            if (QFileInfo(tmparg).exists()) {
                file_list << tmparg;
            }
            else if (tmparg.startsWith("voxpath=")) {
                vox_file = tmparg.mid(QString("voxpath=").length());
                vox_file.replace("\"", "");
            }
            else if (tmparg.startsWith("fdtdpath=")) {
                fdtd_file = tmparg.mid(QString("fdtdpath=").length());
                fdtd_file.replace("\"", "");
            }
        }
    }
    vox3dform->show();
    if (!fdtd_file.isEmpty()) {
        vox3dform->setFdtdFile(fdtd_file);
    }
    if (!vox_file.isEmpty()) {
        vox3dform->setVoxFile(vox_file);
    }
    if (!file_list.isEmpty()) {
        vox3dform->allLoad(file_list);
    }
}

bool MainWindow3DViewer::m_mouse_moving = false;
void MainWindow3DViewer::moveEvent(QMoveEvent* event)
{
    m_mouse_moving = true;
    /*
    /// Qtのバグか使い方の問題かドッキング外したウィジットが勝手にサイズ変更される（縮小される）ので対策
    /// ここではサイズ縮小不可にして、アクティブにしたら再度変更可能にする
    Vox3DForm* form = (Vox3DForm*)centralWidget();
    for (auto& dock : form->m_dock_widgets) {
        if (dock->isFloating()) {
            dock->setMinimumSize(dock->size());
        }
    }*/

    QMainWindow::moveEvent(event);
}

void MainWindow3DViewer::closeEvent(QCloseEvent* event)
{
    hide();

    Vox3DForm* form = qobject_cast<Vox3DForm*>(centralWidget());
    if (form) {
        form->closeProcess();
    }

    event->accept();
}

// void MainWindow::on_actionExit_triggered()
//{    // プログラム終了して、GUIを閉じる。
//     exit(0);
// }

void MainWindow3DViewer::on_actionLoad_triggered()
{    // プログラム終了して、GUIを閉じる。
    qDebug() << "[DEBUG]01 MainWin.cpp-on_actionLoad_file_triggered";
}

void MainWindow3DViewer::on_action3Dviewer_triggered()
{    // 3Dviewerを前面に表示する
    SubWindowVox3DForm->setVisible(true);
    SubWindowVox3DForm->raise();
    SubWindowVox3DForm->setWindowState(Qt::WindowActive);
}

void MainWindow3DViewer::on_action2Dviewer_triggered()
{    // 3Dviewerを前面に表示する
    qDebug() << "[DEBUG]01 on_action2Dviewer_triggered";
    SubWindowGeometryInputForm->setVisible(true);
    SubWindowGeometryInputForm->raise();
    SubWindowGeometryInputForm->setWindowState(Qt::WindowActive);
}

void MainWindow3DViewer::on_MW_pushButton_DEBUG_clicked()
{
    // emit sendto3D_sendVoxpath(ui->MW_lineEdit_DEBUG->text());
    // emit sendto2D_sendVoxpath(ui->MW_lineEdit_DEBUG->text());

    //[DEBUG]コマンドラインから.exeが実行された場合、コマンドラインでの引数をセット
    // for(int i=0; i<G_inCmdargList.size(); i++){
    //    QString  tmparg = G_inCmdargList.at(i);
    //    if(i==0){ GE_voxfilepath = tmparg; }
    //    if(i==1){ GE_cellsizeX = tmparg.toInt(); }
    //    if(i==2){ GE_cellarrayX = tmparg.toInt(); }
    //
    //    if(tmparg.startsWith("voxpath=")){ GE_voxfilepath = tmparg.replace("voxpath=", ""); }
    //    if(tmparg.startsWith("cellsize=")){
    //        QString tmpstr =  tmparg.replace("cellsize=", "");
    //        if(! tmpstr.isEmpty()){ GE_cellsize = tmpstr.toInt(); }
    //    }
    //    if(tmparg.startsWith("cellarray=")){
    //        QString tmpstr =  tmparg.replace("cellarray=", "");
    //        if(! tmpstr.isEmpty()){ GE_cellarray = tmpstr.toInt(); }
    //    }
    //}
}

void MainWindow3DViewer::on_pushButton_voxpathLoad_clicked()
{
    // ユーザーによるvoxファイル選択-------------------------------------------------------------------------
    QString     loadfileName = "";
    QStringList tmpStringList;
    QString     loadfileNameBefore;
    QString     beforefilename, beforefilepath;
    QString     tmpDirPath;

    // tmpDirPath = "Desktop";
    // loadfileName = QFileDialog::getOpenFileName(this, tr("Select file"), tmpDirPath, tr("text file (*.txt)"));
    // if(loadfileName.isEmpty() == 1 ){return;} // ファイル選択でキャンセルボタンが押されたら, そのまま終了。

    // 既にloadされているファイルがある場合はそのファイルがあるフォルダをファイル選択ダイアログの起点フォルダとする
    tmpDirPath = "Desktop";
    if (ui->lineEdit_voxpath->text() != "") {
        tmpDirPath = QFileInfo(ui->lineEdit_voxpath->text()).absolutePath();
    }
    // loadfileName = QFileDialog::getOpenFileName(this, tr("Select file"), tmpDirPath, tr("text file (*.txt)"));
    loadfileName = QFileDialog::getOpenFileName(this, tr("Select file"), tmpDirPath);
    if (loadfileName.isEmpty() == 1) {
        return;
    }    // ファイル選択でキャンセルボタンが押されたら, そのまま終了。
    ui->lineEdit_voxpath->setText(loadfileName);
    ui->lineEdit_voxpath->setToolTip(loadfileName);

    if (ui->comboBox_voxChildWin->currentText() == "3Dviewer") {
        emit sendto3D_sendVoxpath(ui->lineEdit_voxpath->text());
    }
    if (ui->comboBox_voxChildWin->currentText() == "2Dviewer") {
        emit sendto2D_sendVoxpath(ui->lineEdit_voxpath->text());
    }
}

void MainWindow3DViewer::on_comboBox_voxChildWin_currentIndexChanged(const QString& arg1)
{
    if (ui->comboBox_voxChildWin->currentText() == "3Dviewr") {    // 3Dviewerを前面に表示する
        SubWindowVox3DForm->setVisible(true);
        SubWindowVox3DForm->raise();
        SubWindowVox3DForm->setWindowState(Qt::WindowActive);
    }

    if (ui->comboBox_voxChildWin->currentText() == "2Dviewr") {    // 3Dviewerを前面に表示する
        qDebug() << "[DEBUG]01 on_action2Dviewer_triggered";
        SubWindowGeometryInputForm->setVisible(true);
        SubWindowGeometryInputForm->raise();
        SubWindowGeometryInputForm->setWindowState(Qt::WindowActive);
    }
}
