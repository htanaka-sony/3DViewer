#ifndef MAINWINDOW3DVIEWER_H
#define MAINWINDOW3DVIEWER_H

#include <QMainWindow>
#include <QPointer>

#include <QMdiSubWindow>

// for FTP Download
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
// #include <QRegExp>
#include <QtCore/QThread>
// #define TR(s) (QString::fromLocal8bit(s)) //2018.09.12 日本語表示

#include <QWidget>

#include "VoxViewerGlobal.h"

// 他画面との変数共有のため extern使用　①MainWindow.h でclassより前に宣言(例： extern QString GE_param1;)
// ②Mainwindow.cpp でclassより前に値設定 例：(QString QString GE_param1="ABC";) ③　OtherWindow.cppに記述追加　#include
// MainWindow.h。→　OtherWin.cppから　GE_param1変数を参照できるようになる。（MainWindow.cppでの値変更がリアルタイムに反映される）
extern QString GE_cmdparamMode;
extern QString GE_voxfilepath;
extern QString GE_fdtdfilepath;
extern int     GE_cellsizeX;
extern int     GE_cellsizeY;
extern int     GE_cellarrayX;
extern int     GE_cellarrayY;

// QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow3DViewer;
}
// QT_END_NAMESPACE

class Vox3DForm;
// class Zsub2Form;
class GeometryInputForm;

class VOXVIEWER_EXPORT MainWindow3DViewer : public QMainWindow {
    Q_OBJECT

public:
    MainWindow3DViewer(QWidget* parent = nullptr);
    ~MainWindow3DViewer();

    QStringList G_inCmdargList;    // .exeファイルがコマンドラインから実行された時の引数取得 voxfilepath,　cellsize,
                                   // cellary など

    static Vox3DForm* createVox3DForm(QMainWindow*& main_window);
    static void       deleteVox3DForm(Vox3DForm* vox3dform);

    static void showArguments(Vox3DForm* vox3dform);

    static bool  m_mouse_moving;
    virtual void moveEvent(QMoveEvent* event);

    virtual void closeEvent(QCloseEvent* event);

signals:
    void sendto3D_sendVoxpath(QString sendStr);
    void sendto2D_sendVoxpath(QString sendStr);
    void sendtoGI_viewStart();

private:
    Ui::MainWindow3DViewer* ui;

    // QPointer<Zsub2Form> mZsub2Form;
    // QMdiSubWindow *SubWindowZsub2Form;
    QPointer<GeometryInputForm> mGeometryInputForm;            // Signal&slot用途
    QMdiSubWindow*              SubWindowGeometryInputForm;    // MDIarea用途

    QPointer<Vox3DForm> mVox3DForm;
    QMdiSubWindow*      SubWindowVox3DForm;    // MDIarea用途

private slots:
    // void on_actionExit_triggered();    // プログラム終了して、GUIを閉じる。
    void on_actionLoad_triggered();
    void on_action3Dviewer_triggered();
    void on_action2Dviewer_triggered();
    void on_MW_pushButton_DEBUG_clicked();
    void on_comboBox_voxChildWin_currentIndexChanged(const QString& arg1);
    void on_pushButton_voxpathLoad_clicked();
};
#endif    // MAINWINDOW3DVIEWER_H
