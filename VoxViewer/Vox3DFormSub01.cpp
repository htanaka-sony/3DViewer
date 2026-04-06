#include "Vox3DForm.h"
#include "ui_Vox3DForm.h"

#include <QMenu>
#include "CommonSubFunc.h"

#include "../Render/RenderGlobal.h"
#include "ClippingCtrl.h"
#include "DimensionCtrl.h"
#include "MemoryMappedFile.h"
#include "ReadOp2.h"
#include "ReadOpt.h"
#include "ReadVoxel.h"
#include "ResultCtrl.h"
#include "SettingGuiCtrl.h"

#include <QDockWidget>
#include <QScrollArea>

#ifdef _WIN32
    #include <shlguid.h>
    #include <shobjidl.h>
    #include <windows.h>
#endif

//-start- for-vox 2021.10.01

void Vox3DForm::on_pushButton_open_files_clicked()
{
    QString filter = tr("All Files (*.vox *.opt *.op2 *.fdtd);;vox file (*.vox);;opt file (*.opt);;op2 file "
                        "(*.op2);;fdtd file (*.fdtd)");

    QStringList loadfileNames = QFileDialog::getOpenFileNames(this, tr("Select file"), "", filter);

    if (loadfileNames.isEmpty()) {
        return;
    }

    allLoad(loadfileNames, !m_setting_gui_ctrl->isOpenAllOp2Open());
}

void Vox3DForm::on_pushButton_open_folder_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this,                        // 親ウィジェット
                                                    tr("Select Folder"),         // ダイアログタイトル
                                                    "",                          // 初期ディレクトリ
                                                    QFileDialog::ShowDirsOnly    // ディレクトリのみ表示
    );
    if (!dir.isEmpty()) {
        allLoadForDD({dir});
    }
}

bool Vox3DForm::enableMultiLoad(const QString& path, QString& ext)
{
    ext = QFileInfo(path).suffix().toLower();
    if (ext == "vox") {
        return false;
    }
    else if (ext == "opt") {
        return false;
    }
    else if (ext == "op2") {
        return true;
    }
    else if (ext == "fdtd") {
        return false;
    }

    return false;
}

void Vox3DForm::allLoadTargetCheck(const QStringList& path_list, bool& has_valid_folder, bool& has_valid_file)
{
    for (const QString& filePath : path_list) {
        QFileInfo info(filePath);

        if (info.isDir()) {
            /// フォルダの場合
            QDirIterator it(filePath, QDir::Files, QDirIterator::NoIteratorFlags);
            while (it.hasNext()) {
                QString        subFilePath = it.next();
                const QString& ext         = QFileInfo(subFilePath).suffix().toLower();
                if (ext == "vox") {
                    has_valid_folder = true;
                    has_valid_file   = true;
                    return;
                }
                else if (ext == "opt") {
                    has_valid_folder = true;
                    has_valid_file   = true;
                    return;
                }
                else if (ext == "op2") {
                    has_valid_folder = true;
                    has_valid_file   = true;
                    return;
                }
                else if (ext == "fdtd") {
                    has_valid_folder = true;
                    has_valid_file   = true;
                    return;
                }
            }
        }
        else if (!has_valid_file) {
            const QString& ext = info.suffix().toLower();
            if (ext == "vox") {
                has_valid_file = true;
            }
            else if (ext == "opt") {
                has_valid_file = true;
            }
            else if (ext == "op2") {
                has_valid_file = true;
            }
            else if (ext == "fdtd") {
                has_valid_file = true;
            }
            if (has_valid_folder && has_valid_file) {
                return;
            }
        }
    }
}

void Vox3DForm::allLoadForDD(const QStringList& path_list)
{
    if (m_setting_gui_ctrl->isDragDropFolderOpen()) {
        bool has_valid_folder = false;
        bool has_valid_file   = false;
        allLoadTargetCheck(path_list, has_valid_folder, has_valid_file);
        if (has_valid_folder) {
            /// Reset
            ui->obj3dViewer->suppressRender(true);

            on_pushButton_optClear_clicked();
            m_result_ctrl->deleteResult2dList(true);
            on_pushButton_voxpathClear_clicked();
            on_vox_fdtd_clear_pushButton_clicked();

            ui->checkBox_result3d_display_in_view->setChecked(false);

            m_dimension_ctrl->reset(false);
            m_dimension_ctrl->clearAutoDimensionMaterialNameChange();
            m_clipping_ctrl->reset(true);
            m_clipping_ctrl->reset(false);    /// クリップ位置戻すためfalseで呼ぶ
            // m_result_ctrl->reset();
            ui->checkBox_project_opt->setChecked(false);
            on_checkBox_project_opt_stateChanged(0);
            func_tableMaterialDefaultSet_matnameToColorCombo();
            on_pushButton_viewAllOn_clicked();
            ui->checkBox_acolor->setChecked(true);
            ui->checkBox_acolor->setChecked(false);
            // ui->obj3dViewer->axoView();
            // ui->obj3dViewer->fitDisplay();
            ui->obj3dViewer->suppressRender(false);
        }
    }

    allLoad(path_list, !m_setting_gui_ctrl->isDragDropOp2Open());
}

void Vox3DForm::allLoad(const QStringList& path_list, bool op2_import_mode)
{
    ResultCtrl::fileOpenStartEnd result_file_open_flag(m_result_ctrl);    /// 暫定

    std::set<QString> loaded_ext;

    std::map<QString, QStringList> multi_load_files;

    for (QString filePath : path_list) {
        replaceShortCutPath(filePath);
        QFileInfo info(filePath);

        if (info.isDir()) {
            /// フォルダの場合
            QDirIterator it(filePath, QDir::Files, QDirIterator::NoIteratorFlags);
            while (it.hasNext()) {
                QString subFilePath = it.next();
                QString ext;
                if (!enableMultiLoad(subFilePath, ext)) {
                    if (!loaded_ext.insert(ext).second) {
                        continue;
                    }
                    if (!allLoad(subFilePath, ext, op2_import_mode)) {
                        return;
                    }
                }
                else {
                    multi_load_files[ext].emplace_back(subFilePath);
                }
            }
        }
        else {
            QString ext;
            if (!enableMultiLoad(filePath, ext)) {
                if (!loaded_ext.insert(ext).second) {
                    continue;
                }
                if (!allLoad(filePath, ext, op2_import_mode)) {
                    return;
                }
            }
            else {
                multi_load_files[ext].emplace_back(filePath);
            }
        }
    }

    for (auto& [ext, paths] : multi_load_files) {
        if (!allLoad(paths, ext, op2_import_mode)) {
            return;
        }
    }
}

bool Vox3DForm::allLoad(const QString& path, const QString& specified_ext, bool op2_import_mode)
{
    const QString& ext = specified_ext.isEmpty() ? QFileInfo(path).suffix().toLower() : specified_ext;

    if (ext == "vox") {
        return voxpathLoad(path);
    }
    else if (ext == "opt") {
        return optLoad(path);
    }
    else if (ext == "op2") {
        return op2Load({path}, op2_import_mode);
    }
    else if (ext == "fdtd") {
        vox_fdtd_load(path);
    }
    else {
    }

    return true;
}

bool Vox3DForm::allLoad(const QStringList& pathes, const QString& ext, bool op2_import_mode)
{
    if (pathes.isEmpty()) {
        return true;
    }

    if (ext == "op2") {
        return op2Load(pathes, op2_import_mode);
    }
    else {
    }

    return true;
}

void Vox3DForm::on_checkBox_project_opt_stateChanged(int arg1)
{
    // チェックボックスを全部ONにした後、再描画する。
    // スロットで1つのチェックボックス変更の都度再描画されてしまわないように、g_flag=0(再描画しない)にして、全チェックボックスON、テーブル最後のチェックボックスだけチェックON前にg_flag=1(再描画する))
    g_flag_slotMatColorCombo = 0;
    for (int row = 0; row < ui->tableWidget_material->rowCount(); row++) {
        if (row == ui->tableWidget_material->rowCount() - 1) {
            g_flag_slotMatColorCombo = 1;
        }
        int        matnum      = ui->tableWidget_material->item(row, 1)->text().toInt();
        QCheckBox* tmpCheckBox = ui->tableWidget_material->cellWidget(row, 6)->findChild<QCheckBox*>();
        bool       checkON     = false;
        if (ui->checkBox_project_opt->isChecked()) {
            checkON = true;
        }
        tmpCheckBox->setChecked(checkON);
    }

    updateEverything();    // openGL widget更新
}

void Vox3DForm::on_pushButton_open_vox_clicked()
{
    QString filter = tr("Vox / Fdtd File (*.vox *.fdtd);;vox file (*.vox);;fdtd file (*.fdtd)");

    QStringList loadfileNames = QFileDialog::getOpenFileNames(this, tr("Select file"), "", filter);

    if (loadfileNames.isEmpty()) {
        return;
    }

    /// Vox
    QString vox_file_path;
    for (auto& path : loadfileNames) {
        const QString& ext = QFileInfo(path).suffix().toLower();
        if (ext == "vox") {
            vox_file_path = path;
            break;
        }
    }

    QString fdtd_file_priority_name;
    if (!vox_file_path.isEmpty()) {
        fdtd_file_priority_name = QFileInfo(vox_file_path).completeBaseName();
    }

    QString fdtd_file_path;
    for (auto& path : loadfileNames) {
        const QString& ext = QFileInfo(path).suffix().toLower();
        if (ext == "fdtd") {
            if (fdtd_file_path.isEmpty()) {
                fdtd_file_path = path;
                if (fdtd_file_priority_name.isEmpty()) {
                    break;
                }
            }
            else if (!fdtd_file_priority_name.isEmpty()) {
                if (fdtd_file_priority_name.compare(QFileInfo(path).completeBaseName(), Qt::CaseInsensitive) == 0) {
                    fdtd_file_path = path;
                    break;
                }
            }
        }
    }

    if (!vox_file_path.isEmpty()) {
        voxpathLoad(vox_file_path);
    }
    if (!fdtd_file_path.isEmpty()) {
        vox_fdtd_load(fdtd_file_path);
    }
}

void Vox3DForm::on_pushButton_open_opt_clicked()
{
    on_pushButton_optLoad_clicked();
}

void Vox3DForm::on_pushButton_open_op2_clicked()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Select file"), "", tr("op2 file (*.op2)"));

    allLoad(fileNames, false);
}

void Vox3DForm::on_pushButton_import_op2_clicked()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Select file"), "", tr("op2 file (*.op2)"));

    allLoad(fileNames);
}

void Vox3DForm::on_pushButton_voxpathLoad_clicked()
{
    // ユーザーによるvoxファイル選択-------------------------------------------------------------------------
    QString loadfileName = "";
    QString tmpDirPath;

    // 既にloadされているファイルがある場合はそのファイルがあるフォルダをファイル選択ダイアログの起点フォルダとする
    tmpDirPath = "Desktop";
    if (ui->lineEdit_voxpath->text() != "") {
        tmpDirPath = QFileInfo(ui->lineEdit_voxpath->text()).absolutePath();
    }
    // ファイル読込み
    // loadfileName = QFileDialog::getOpenFileName(this, tr("Select file"), tmpDirPath, tr("text file (*.txt)"));
    loadfileName = QFileDialog::getOpenFileName(this, tr("Select file"), tmpDirPath, tr("vox file (*.vox)"));
    if (loadfileName.isEmpty() == 1) {
        return;
    }    // ファイル選択でキャンセルボタンが押されたら, そのまま終了。

    voxpathLoad(loadfileName);
}

void Vox3DForm::on_pushButton_voxpathApply_clicked()
{
    if (ui->lineEdit_voxpath->text() != "") {
        QString path = ui->lineEdit_voxpath->text();
        path.remove("\"");
        if (QFileInfo(path).exists()) {
            QString ext = QFileInfo(path).suffix().toLower();
            if (ext == "vox") {
                voxpathLoad(path);
            }
            else {
                QMessageBox::warning(this, windowTitle(), "Please input .vox file.");
            }
        }
        else {
            QMessageBox::warning(this, windowTitle(), "File not found.");
        }
    }
}

void Vox3DForm::on_pushButton_voxpathClear_clicked()
{
    ui->lineEdit_voxpath->setText("");
    ui->obj3dViewer->clearMaterialIdToNode(true);
    voxpathLoad("");
    m_dimension_ctrl->reset();
    m_dimension_ctrl->clearAutoDimensionMaterialNameChange();    /// 暫定 vox切り替わったら初期化
    m_clipping_ctrl->reset(true);    /// 暫定 - shapeBoundingBox前にクリッピング解除
    m_clipping_ctrl->reset(false);
    ui->obj3dViewer->update();
    m_result_ctrl->voxelFileCompressData().resize(0);
    m_result_ctrl->voxelFileCompressData().shrink_to_fit();
    m_result_ctrl->setCompressDataOriginalSize(0);
}

bool Vox3DForm::voxpathLoad(const QString& path)
{
    ui->lineEdit_voxpath->setText(path);

    // voxファイル読込み-------------------------------------------------------------------------
    // qDebug() << "[DEBUG]Vox3DFormSub01.cpp-on_pushButton_voxpathLoad_clicked: start-all "
    //                + QDateTime::currentDateTime().toString("hh:mm:ss");
    // ui->label_voxmessage->setText("Now-loading");

    QString tmpstr;

    // QString voxfilepath = "c:/tmp/tmp.vox";
    // QString voxfilepath = QFileDialog::getOpenFileName(this,"Open
    // vox",QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).at(0) ) ; //, "*obj");
    QString voxfilepath = ui->lineEdit_voxpath->text();

    // マテリアルテーブルへのセット
    // 色情報の初期化
    CommonSubFunc mCommonSubFunc;
    g_hash_matnameToCname =
        mCommonSubFunc.func_makehash_matnameToCname();    // 共通値参照ハッシュ　キー：マテリアル名　値:色名(greenなど)
    //
    g_hash_matToName.clear();      // マテリアル番号からマテリアル名取得用のハッシュ
    g_hash_matToViewOn.clear();    // マテリアル番号から、描画表示or非表示情報取得用のハッシュ
    g_hash_matToColorName.clear();    // マテリアル番号から、描画の色名取得用のハッシュ
    g_hash_matToColorVec.clear();     // マテリアル番号から、描画の色コード取得用のハッシュ
    g_hash_matToAcolor.clear();       // マテリアル番号から、描画の半透明情報取得用のハッシュ
    // qDebug() << "[DEBUG]Vox3DFormSub01.cpp-on_pushButton_voxpathLoad_clicked: start set-materialTable "
    //                 + QDateTime::currentDateTime().toString("hh:mm:ss");
    QStringList lineList = func_getMaterialfromVoxPath(ui->lineEdit_voxpath->text());
    func_tableMaterialDefaultSet(lineList);    // materialテーブルへの反映
    // qDebug() << "[DEBUG]Vox3DFormSub01.cpp-on_pushButton_voxpathLoad_clicked: end set-materialTable "
    //                 + QDateTime::currentDateTime().toString("hh:mm:ss");

    // openGL描画処理
    // qDebug() << "[DEBUG]Vox3DFormSub01.cpp-on_pushButton_voxpathLoad_clicked: start openGL-updateEverithing "
    //                + QDateTime::currentDateTime().toString("hh:mm:ss");
    updateEverything();    // GUI　タブメニューmainTabの設定内容を、openGL描画処理に渡す
    // qDebug() << "[DEBUG]Vox3DFormSub01.cpp-on_pushButton_voxpathLoad_clicked: end openGL-updateEverithing "
    //                 + QDateTime::currentDateTime().toString("hh:mm:ss");

    // QVector<QOpenGLTriangle3D_vox> triangles; //グローバル変数に置き換える
    QStringList comments;

#ifdef MEASURE_TIME
    QElapsedTimer elapsedTime;
    elapsedTime.start();
#endif

    if (QFile(voxfilepath).exists()) {
        QProgressDialog pd(this);
        pd.setModal(true);
        Qt::WindowFlags flags = pd.windowFlags();
        flags &= ~Qt::WindowCloseButtonHint;
        pd.setWindowFlags(flags);
        pd.open();
        pd.setFixedSize(pd.sizeHint());
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

        ReadVoxel readVoxel(this, &pd);
        ui->obj3dViewer->suppressRender(true);
        if (!readVoxel.read(voxfilepath, ui->obj3dViewer, this)) {
            ui->obj3dViewer->suppressRender(false);
            on_pushButton_voxpathClear_clicked();
            ui->lineEdit_voxpath->setText(path);    /// text残しておく？
            return false;
        }

        ui->checkBox_project_opt->setChecked(false);
        ui->checkBox_acolor->setChecked(false);

        if (ui->checkBox_result3d_show_only_in_voxel_material->isChecked()) {
            ui->checkBox_result3d_show_only_in_voxel_material->setChecked(false);
            ui->obj3dViewer->fitDisplay();
        }

        ui->obj3dViewer->suppressRender(false);
        // ui->label_voxmessage->setText("");    // 読み込み終了。
        func_tableMaterialDefaultSet_matnameToColorCombo();
        /// 暫定 - コンボボックスと色が不一致になるので
        {
            for (int row = 0; row < ui->tableWidget_material->rowCount(); row++) {
                auto tmpcombo = static_cast<CustomColorCombo*>(ui->tableWidget_material->cellWidget(row, 3));

                QString cname   = tmpcombo->currentText();            // 色名 Greenなど
                QString bgcolor = g_hash_cnameToStyleColor[cname];    // bgcolor="#008000" など
                /*
                QString fontcolor = "#000000";                          // 通常は文字色=黒
                if (g_list_whitefontBgColor.indexOf(cname) > -1) {
                    fontcolor = "#ffffff";    // 見やすさのため、背景色が濃い色の場合は文字色=白
                }
                // コンボボックス部品の背景色・文字色設定
                //[DEBUG]tmpcombo->setStyleSheet("QComboBox { background-color: #008000 ; color: #ff0000 ; }");
                ////例：背景色　緑　文字色：赤
                QString strStyle = QString("QComboBox { background-color: %1 ; color: %2 ; }").arg(bgcolor, fontcolor);
                tmpcombo->combobox()->setStyleSheet(strStyle);
                */
                tmpcombo->setColor(cname, bgcolor);
                int  matnum = ui->tableWidget_material->item(row, 1)->text().toInt();
                auto color  = tmpcombo->color();    // func_GL_defineColor_nameToRGBvec(cname);
                // qDebug() << "Color Change" << matnum;
                // qDebug() << "Color Change" << cname;
                // qDebug() << "Color Change" << color;
                ui->obj3dViewer->setColor(matnum, color, row == ui->tableWidget_material->rowCount() - 1);

                auto        tmpcombo2 = static_cast<QComboBox*>(ui->tableWidget_material->cellWidget(row, 5));
                const auto& drawMode  = tmpcombo2->currentText();
                if (drawMode == "Shade") {
                    ui->obj3dViewer->setDrawMode(matnum, true, false);
                }
                else if (drawMode == "Wire") {
                    ui->obj3dViewer->setDrawMode(matnum, false, true);
                }
                else if (drawMode == "S+W") {
                    ui->obj3dViewer->setDrawMode(matnum, true, true);
                }
                else {
                    ui->obj3dViewer->setDrawMode(matnum, false, false);
                }
            }
        }
        m_dimension_ctrl->reset();
        m_clipping_ctrl->reset(true);    /// 暫定 - shapeBoundingBox前にクリッピング解除
        m_clipping_ctrl->reset(false);
        m_result_ctrl->setVoxSize();

        m_dimension_ctrl->clearAutoDimensionMaterialNameChange();    /// 暫定 vox切り替わったら初期化

#ifdef MEASURE_TIME
        QEventLoop loop;
        connect(ui->obj3dViewer, &QOpenGLWidget::frameSwapped, &loop, &QEventLoop::quit);
        ui->obj3dViewer->update();    // 再描画をトリガー
        loop.exec();                  // 描画完了まで待機
        QMessageBox::information(this, "Vox File Read",
                                 QString("Elapsed Time: %1 s").arg(elapsedTime.elapsed() / 1000));
#endif
        return true;

        // voxRead_makeGLdata().func_01main_GL_make_getPointOfMesh(triangles, voxfilepath);
        // voxRead_makeGLdata().func_01main_GL_make_getPointOfMesh(g_vox_triangles, voxfilepath);
        // //グローバル変数に置き換える
        func_01main_GL_makeGL3DfromVox(
            g_vox_triangles, voxfilepath);    // グローバル変数に入れて、色切替え・表示切り替えできるようにする

        // 半透明ON/OFF設定 (全図形半透明 or 全図形通常表示)
        int flag_acolor = 0;
        if (ui->checkBox_acolor->isChecked()) {
            flag_acolor = 1;
        }
#ifdef ORIGINAL_OBJ3_VIEW    /// TODO: ビュー置き換え中によりコメントアウト
        ui->obj3dViewer->g_ui_acolorflag = flag_acolor;
#endif
        // voxファイルからの情報を渡して、openGL描画する
        qDebug() << "[DEBUG]Vox3DForm.cpp-on_pushButton_voxpathLoad_clicked(): start openGL-draw-setTriangle "
                        + QDateTime::currentDateTime().toString("hh:mm:ss");
        // ui->obj3dViewer->func_vox_setTriangles(triangles);
#ifdef ORIGINAL_OBJ3_VIEW    /// TODO: ビュー置き換え中によりコメントアウト
        ui->obj3dViewer->func_vox_setTriangles(
            g_vox_triangles);    // グローバル変数に入れて、色切替え・表示切り替えできるようにする
#else
        // ui->obj3dViewer->createObjects(g_vox_triangles);
#endif
        qDebug() << "[DEBUG]Vox3DForm.cpp-on_pushButton_voxpathLoad_clicked(): end openGL-draw-setTriangle "
                        + QDateTime::currentDateTime().toString("hh:mm:ss");

        // ui->label_voxmessage->setText("");    // 読み込み終了。
    }
    else {
        if (ui->checkBox_result3d_show_only_in_voxel_material->isChecked()) {
            ui->checkBox_result3d_show_only_in_voxel_material->setChecked(false);
        }
        // ui->label_voxmessage->setText("");
        //  QMessageBox::information(this, "notice", "file non-exsist " + voxfilepath);
    }

    return true;
    // openGL設定関連の設定をデフォルト状態にセットする　[tanslatetion]タブ -
    // [translate]などタイプごとの情報も設定される。
    // on_vox_pushButton_setDefaultViewMenu_clicked();

    // qDebug() << "[DEBUG]Vox3DFormSub01.cpp-on_pushButton_voxpathLoad_clicked: end-all "
    //                 + QDateTime::currentDateTime().toString("hh:mm:ss");
}

void Vox3DForm::on_pushButton_optLoad_clicked()
{
    // ユーザーによるoptファイル選択-------------------------------------------------------------------------
    QString loadfileName = "";
    QString tmpDirPath;

    // 既にloadされているファイルがある場合はそのファイルがあるフォルダをファイル選択ダイアログの起点フォルダとする
    tmpDirPath = "Desktop";
    if (ui->lineEdit_optfile_path->text() != "") {
        tmpDirPath = QFileInfo(ui->lineEdit_optfile_path->text()).absolutePath();
    }
    // ファイル読込み
    // loadfileName = QFileDialog::getOpenFileName(this, tr("Select file"), tmpDirPath, tr("text file (*.txt)"));
    loadfileName = QFileDialog::getOpenFileName(this, tr("Select file"), tmpDirPath, tr("opt file (*.opt)"));
    if (loadfileName.isEmpty() == 1) {
        return;
    }    // ファイル選択でキャンセルボタンが押されたら, そのまま終了。

    optLoad(loadfileName);
}

void Vox3DForm::on_pushButton_optApply_clicked()
{
    if (ui->lineEdit_optfile_path->text() != "") {
        QString path = ui->lineEdit_optfile_path->text();
        path.remove("\"");
        if (QFileInfo(path).exists()) {
            QString ext = QFileInfo(path).suffix().toLower();
            if (ext == "opt") {
                optLoad(path);
            }
            else {
                QMessageBox::warning(this, windowTitle(), "Please input .opt file.");
            }
        }
        else {
            QMessageBox::warning(this, windowTitle(), "File not found.");
        }
    }
}

void Vox3DForm::on_pushButton_optClear_clicked()
{
    resultCtl()->setVoxel(nullptr);
    ui->obj3dViewer->removeOpt3DNode();
    ui->lineEdit_optfile_path->setText("");

    m_clipping_ctrl->reset(true);
    m_clipping_ctrl->reset(false);    /// クリップ位置戻すためfalseで呼ぶ
    ui->obj3dViewer->update();
}

bool Vox3DForm::optLoad(const QString& path)
{
    ui->lineEdit_optfile_path->setText(path);

    QProgressDialog pd(this);
    pd.setModal(true);
    Qt::WindowFlags flags = pd.windowFlags();
    flags &= ~Qt::WindowCloseButtonHint;
    pd.setWindowFlags(flags);
    pd.open();
    pd.setFixedSize(pd.sizeHint());
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

    ui->obj3dViewer->suppressRender(true);
    ReadOpt read_opt(this, &pd);
    if (!read_opt.read(path.toStdWString(), ui->obj3dViewer, this)) {
        ui->obj3dViewer->suppressRender(false);
        return false;
    }
    ui->obj3dViewer->suppressRender(false);

    func_tableMaterialDefaultSet_matnameToColorCombo();
    /// 暫定 - コンボボックスと色が不一致になるので
    {
        for (int row = 0; row < ui->tableWidget_material->rowCount(); row++) {
            auto tmpcombo = static_cast<CustomColorCombo*>(ui->tableWidget_material->cellWidget(row, 3));

            QString cname   = tmpcombo->currentText();            // 色名 Greenなど
            QString bgcolor = g_hash_cnameToStyleColor[cname];    // bgcolor="#008000" など
            /*
            QString fontcolor = "#000000";                          // 通常は文字色=黒
            if (g_list_whitefontBgColor.indexOf(cname) > -1) {
                fontcolor = "#ffffff";    // 見やすさのため、背景色が濃い色の場合は文字色=白
            }
            // コンボボックス部品の背景色・文字色設定
            //[DEBUG]tmpcombo->setStyleSheet("QComboBox { background-color: #008000 ; color: #ff0000 ; }");
            ////例：背景色　緑　文字色：赤
            QString strStyle = QString("QComboBox { background-color: %1 ; color: %2 ; }").arg(bgcolor, fontcolor);
            tmpcombo->combobox()->setStyleSheet(strStyle);
            */
            tmpcombo->setColor(cname, bgcolor);
            int  matnum = ui->tableWidget_material->item(row, 1)->text().toInt();
            auto color  = tmpcombo->color();    // func_GL_defineColor_nameToRGBvec(cname);
            // qDebug() << "Color Change" << matnum;
            // qDebug() << "Color Change" << cname;
            // qDebug() << "Color Change" << color;
            ui->obj3dViewer->setColor(matnum, color, row == ui->tableWidget_material->rowCount() - 1);
        }
    }
    // m_dimension_ctrl->reset();
    m_clipping_ctrl->reset(true);    /// 暫定 - shapeBoundingBox前にクリッピング解除
    m_clipping_ctrl->reset(false);
    m_result_ctrl->setVoxSize();

    /// 投影があるので呼ぶ
    func_tableMaterialToGL();

    /// 表示にチェック
    ui->checkBox_result3d_view->setChecked(true);

    return true;
}

bool Vox3DForm::op2Load(const QStringList& pathes, bool op2_import_mode)
{
    QProgressDialog pd(this);
    // pd.setModal(true);
    // Qt::WindowFlags flags = pd.windowFlags();
    // flags &= ~Qt::WindowCloseButtonHint;
    // pd.setWindowFlags(flags);
    // pd.open();
    // pd.setFixedSize(pd.sizeHint());
    // qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

    ReadOp2 read_op2(this, &pd);
    if (!op2_import_mode) {
        read_op2.setOpenMode();
    }
    ui->obj3dViewer->suppressRender(true);
    bool ret = read_op2.read(pathes, ui->obj3dViewer, this);
    ui->obj3dViewer->suppressRender(false);
    m_clipping_ctrl->reset(true);    /// 暫定 - shapeBoundingBox前にクリッピング解除
    m_clipping_ctrl->reset(false);

    return ret;
}

bool Vox3DForm::replaceShortCutPath(QString& path)
{
#ifdef _WIN32
    if (!path.endsWith(".lnk", Qt::CaseInsensitive)) {
        return false;
    }

    bool ret = false;

    HRESULT hr;
    CoInitialize(NULL);

    IShellLink* psl = nullptr;
    hr              = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    if (SUCCEEDED(hr)) {
        IPersistFile* ppf = nullptr;
        hr                = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
        if (SUCCEEDED(hr)) {
            // Convert QString to wchar_t*
            std::wstring wLnkPath = path.toStdWString();
            hr                    = ppf->Load(wLnkPath.c_str(), STGM_READ);
            if (SUCCEEDED(hr)) {
                WCHAR szTarget[MAX_PATH];
                hr = psl->GetPath(szTarget, MAX_PATH, nullptr, 0);
                if (SUCCEEDED(hr)) {
                    path = QString::fromWCharArray(szTarget);
                    ret  = true;
                }
            }
            ppf->Release();
        }
        psl->Release();
    }
    CoUninitialize();
    return ret;
#else
    return false;
#endif
}

void Vox3DForm::fdtdLoad(const QString& path, QHash<QString, QString>& hash_matname_to_cname)
{
    QFile InFile01(path);
    if (!InFile01.exists() || !InFile01.open(QIODevice::ReadOnly)) {
        return;
    }

    QTextStream in01(&InFile01);
    int         flag_color = 0;
    while (!in01.atEnd()) {
        const QString& linestr = in01.readLine();
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
            if (tmpList.size() > 2) {
                hash_matname_to_cname.insert(tmpList.at(0), tmpList.at(2));
            }
        }
    }
}

void Vox3DForm::currentVoxDrawMode(bool& shading, bool& wireframe)
{
    const auto& draw_default = ui->comboBox_voxel_draw_mode->currentText();
    if (draw_default == "Shading") {
        shading   = true;
        wireframe = false;
    }
    else if (draw_default == "Wireframe") {
        shading   = false;
        wireframe = true;
    }
    else if (draw_default == "Shading+Wireframe") {
        shading   = true;
        wireframe = true;
    }
    else {
        shading   = false;
        wireframe = false;
    }
}

void Vox3DForm::allShow(bool show)
{
    ui->obj3dViewer->suppressRender(true);

    voxAllShow(show);
    optAllShow(show);
    op2AllShow(show);
    dimensionAllShow(show);
    autoDimensionAllShow(show);

    ui->obj3dViewer->suppressRender(false);
}

void Vox3DForm::voxAllShow(bool show)
{
    if (show) {
        on_pushButton_viewAllOn_clicked();
    }
    else {
        on_pushButton_viewAllOff_clicked();
    }
}

void Vox3DForm::optAllShow(bool show)
{
    resultCtl()->showOpt(show);
}

void Vox3DForm::op2AllShow(bool show)
{
    resultCtl()->showOp2(show);
}

void Vox3DForm::dimensionAllShow(bool show)
{
    dimensionCtrl()->showDimension(show);
}

void Vox3DForm::autoDimensionAllShow(bool show)
{
    dimensionCtrl()->showAutoDimension(show);
}

void Vox3DForm::voxOnlyShow(bool show, bool show_self)
{
    ui->obj3dViewer->suppressRender(true);

    if (show_self) voxAllShow(show);
    optAllShow(!show);
    op2AllShow(!show);
    dimensionAllShow(!show);
    autoDimensionAllShow(!show);

    ui->obj3dViewer->suppressRender(false);
}

void Vox3DForm::optOnlyShow(bool show, bool show_self)
{
    ui->obj3dViewer->suppressRender(true);

    voxAllShow(!show);
    if (show_self) optAllShow(show);
    op2AllShow(!show);
    dimensionAllShow(!show);
    autoDimensionAllShow(!show);

    ui->obj3dViewer->suppressRender(false);
}

void Vox3DForm::op2OnlyShow(bool show, bool show_self)
{
    ui->obj3dViewer->suppressRender(true);

    voxAllShow(!show);
    optAllShow(!show);
    if (show_self) op2AllShow(show);
    dimensionAllShow(!show);
    autoDimensionAllShow(!show);

    ui->obj3dViewer->suppressRender(false);
}

void Vox3DForm::dimensionOnlyShow(bool show, bool show_self)
{
    ui->obj3dViewer->suppressRender(true);

    voxAllShow(!show);
    optAllShow(!show);
    op2AllShow(!show);
    if (show_self) dimensionAllShow(show);
    autoDimensionAllShow(!show);

    ui->obj3dViewer->suppressRender(false);
}

void Vox3DForm::autoDimensionOnlyShow(bool show, bool show_self)
{
    ui->obj3dViewer->suppressRender(true);

    voxAllShow(!show);
    optAllShow(!show);
    op2AllShow(!show);
    dimensionAllShow(!show);
    if (show_self) autoDimensionAllShow(show);

    ui->obj3dViewer->suppressRender(false);
}

void Vox3DForm::func_setGUIstartup()
{    // for-vox GUIデフォルト表示など
    this->setWindowTitle("3Dviewer");

    if (g_DEBUGMODE == 0) {
        // フォームを表示しない
        ui->label_rotateAngleY->setVisible(false);
        ui->lineEdit_rotateAngleY->setVisible(false);
        ui->label_rotateAngleZ->setVisible(false);
        ui->lineEdit_rotateAngleZ->setVisible(false);

        // フォームを表示しない　タブ
        // ui->lookAtTab->setVisible(); //setVisibleでは消えない。
        ui->mainTabs->removeTab(ui->mainTabs->indexOf(ui->perspectiveTab));
        ui->mainTabs->removeTab(
            ui->mainTabs->indexOf(ui->lookAtTab));    // 内部処理でGUIフォームの値変更しても落ちることなく動作している
        ui->mainTabs->removeTab(ui->mainTabs->indexOf(ui->transformTab));
        ui->mainTabs->removeTab(ui->mainTabs->indexOf(ui->textureTab));
        ui->mainTabs->removeTab(ui->mainTabs->indexOf(ui->diffuseLightTab));
        ui->mainTabs->removeTab(ui->mainTabs->indexOf(ui->VoxOptionTab));

        // フォームを表示しない　タブメニュー内
        ui->label_voxoption_01->setVisible(false);
    }

    ui->splitter_vox_01->setSizes(QList<int>() << 1000 << 300);

    // ui->label_voxmessage->setText("");

    ui->tableWidget_material->setColumnCount(7);
    QStringList labels;
    labels << "View"
           << "No"
           << "MaterialName"
           << "Color"
           << "Trns"
           << "Draw"
           << "opt";
    ui->tableWidget_material->setHorizontalHeaderLabels(labels);
    ui->tableWidget_material->QTableWidget::horizontalHeader()->setStretchLastSection(true);
    // ui->tableWidget_material->setEditTriggers(QAbstractItemView::NoEditTriggers); //Table上の直接編集禁止
    ui->tableWidget_material->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_material->setColumnWidth(0, 35);     //[0]列目(チェックボックス欄)の幅
    ui->tableWidget_material->setColumnWidth(1, 35);     //[1]列目(マテリアル番号欄)の幅
    ui->tableWidget_material->setColumnWidth(2, 180);    //[2]列目(マテリアル名)の幅
    ui->tableWidget_material->setColumnWidth(3, 140);    //[3]列目(色名)の幅
    ui->tableWidget_material->setColumnWidth(4, 35);     //[4]列目(チェックボックス欄)の幅
    ui->tableWidget_material->setColumnWidth(5, 60);     //[5]列目(表示モード)の幅
    ui->tableWidget_material->setColumnWidth(6, 35);     //[6]列目(チェックボックス欄)の幅
    // ui->tableWidget_material->hideColumn(1); //材質No 非表示にする ユーザーは見る必要もないため
    // ui->tableWidget_material->hideColumn(4);
    // //色透明　非表示にする　半透明切替が実装できたら表示する。それまでは非表示。
    ui->tableWidget_material->setMinimumWidth(520);
    g_flag_slotMatColorCombo =
        1;    // マテリアルコンボの値変更時にスロット処理させる:1 or
              // させない:0。（ユーザー操作の時だけスロット処理したい。自動設定のとき（自動=初期設定の時など）はスロット処理したくない）
    g_flag_slotMatView =
        1;    // マテリアルコンボの値変更時にスロット処理させる:1 or
              // させない:0。（ユーザー操作の時だけスロット処理したい。自動設定のとき（自動=初期設定の時など）はスロット処理したくない）

    // 簡易回転　コンボボックス設定
    QStringList voxRotateComboList, voxRotateVComboList;
    voxRotateComboList << "front"
                       << "left"
                       << "back"
                       << "right";
    voxRotateVComboList << "middle"
                        << "top"
                        << "bottom";
    ui->comboBox_voxRotate->addItems(voxRotateComboList);
    ui->comboBox_voxRotateV->addItems(voxRotateVComboList);

    QStringList voxLightPosComboList;
    voxLightPosComboList << "front"
                         << "left"
                         << "back"
                         << "right";
    ui->comboBox_voxLightPos->addItems(voxLightPosComboList);

    // 視点回転　ラベルにイメージ画像貼り付け
    // QPixmap tmppix("C:/kuroda/work/Qt/qt_work_3Dviewer/voxfile_viewer_MDIarea_H/images/cube_01.png");
    QPixmap tmppix(":/images/cube_01.png");
    // ui->label_voxRotateImg->setPixmap(tmppix.scaled(100,100));
    ui->label_voxRotateImg->setPixmap(tmppix.scaled(60, 60));

    // マウス動作切替え (回転・平行移動)
    ui->vox_comboBox_mouseAct->addItems({m_mouse_rotate, m_mouse_pan, m_mouse_nop});
    ui->vox_comboBox_mouseAct_default->addItems({m_mouse_rotate, m_mouse_pan, m_mouse_nop});    /// 設定タブ
    g_flag_notRedraw = 0;    // 通常=0。　マウス回転動作で、シグナル・スロット一時無効にする時 =1 にする。

    // GUI色定義　共通変数
    CommonSubFunc mCommonSubFunc;
    g_hash_matnameToCname =
        mCommonSubFunc.func_makehash_matnameToCname();    // 共通値参照ハッシュ　キー：マテリアル名　値:色名(greenなど)
    g_hash_cnameToQtColor =
        mCommonSubFunc.func_makehash_cnameToQtColor();    // 共通値参照ハッシュ　キー：色名(greenなど)　値:QColor型
                                                          // 例→QColor(0,128,0,255)
    g_hash_cnameToStyleColor =
        mCommonSubFunc
            .func_makehash_cnameToStyleColor();    // 共通値参照ハッシュ　キー：色名(greenなど)　値:String型　例→
                                                   // "#008000"
    g_hash_cnameToRGBColor =
        mCommonSubFunc
            .func_makehash_cnameToRGBColor();    // 共通値参照ハッシュ　キー：色名(greenなど)　値:QList型 例: {1, 0, 0}
    g_list_whitefontBgColor = mCommonSubFunc.func_makeList_whitefontBgColor();
}

void Vox3DForm::on_checkBox_acolor_stateChanged(int arg1)
{
    // for vox
    //  半透明ON/OFF設定 (全図形 半透明にする)
    int flag_acolor = 0;
    if (ui->checkBox_acolor->isChecked()) {
        flag_acolor = 1;
    }
#ifdef ORIGINAL_OBJ3_VIEW    /// TODO: ビュー置き換え中によりコメントアウト
    ui->obj3dViewer->g_ui_acolorflag = flag_acolor;
#endif

    // チェックボックスを全部ONにした後、再描画する。
    // スロットで1つのチェックボックス変更の都度再描画されてしまわないように、g_flag=0(再描画しない)にして、全チェックボックスON、テーブル最後のチェックボックスだけチェックON前にg_flag=1(再描画する))
    g_flag_slotMatColorCombo = 0;
    for (int row = 0; row < ui->tableWidget_material->rowCount(); row++) {
        if (row == ui->tableWidget_material->rowCount() - 1) {
            g_flag_slotMatColorCombo = 1;
        }
        int        matnum      = ui->tableWidget_material->item(row, 1)->text().toInt();
        QCheckBox* tmpCheckBox = ui->tableWidget_material->cellWidget(row, 4)->findChild<QCheckBox*>();
        bool       checkON     = false;
        float      acolor      = 0;    // 不透明(通常)
        if (ui->checkBox_acolor->isChecked()) {
            checkON = true;
            acolor  = 0.3;    // 半透明
        }
        tmpCheckBox->setChecked(checkON);
        g_hash_matToAcolor[matnum] = acolor;
    }

    updateEverything();    // openGL widget更新
}

void Vox3DForm::func_tableMaterialDefaultSet(
    QStringList in_materialLineList)    // materialテーブルのセット。マテリアルごとに表示・非表示, 色の変更などの用途。
{
    g_flag_slotMatColorCombo =
        0;    // マテリアル色コンボのスロット処理無効。自動設定の時にスロット処理されると実行エラーで落ちてしまうため。

    //[DEBUG]for(int i=0; i< in_materialLineList.size(); i++){  qDebug() <<
    //"[DEBUG]Vox3DForm.cpp-func_tableMaterialDefaultSet materialLineList" + in_materialLineList.at(i); }

    // 全行削除
    for (int row = ui->tableWidget_material->rowCount() - 1; row >= 0; row--) {
        ui->tableWidget_material->removeRow(0);
    }

    QStringList label_matColorList;
    label_matColorList << "Black"
                       << "Gray"
                       << "Lightgray"
                       << "Green"
                       << "Brown"
                       << "Purple"
                       << "MediumPurple"
                       << "Red"
                       << "Magenta"
                       << "Pink"
                       << "Orange"
                       << "Gold"
                       << "Yellow"
                       << "Green"
                       << "Greenyellow"
                       << "Olive"
                       << "Navy"
                       << "Blue"
                       << "Cyan"
                       << "Lightcyan";

    QStringList label_drawMode;
    label_drawMode << "Shade"
                   << "Wire"
                   << "S+W"
                   << "None";

    int def_drawmode = 0;

    const auto& draw_default = ui->comboBox_voxel_draw_mode->currentText();
    if (draw_default == "Shading") {
        def_drawmode = 0;
    }
    else if (draw_default == "Wireframe") {
        def_drawmode = 1;
    }
    else if (draw_default == "Shading+Wireframe") {
        def_drawmode = 2;
    }
    else {
        def_drawmode = 3;
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
        ui->tableWidget_material->insertRow(ui->tableWidget_material->rowCount());

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
        // ui->tableWidget_material->setCellWidget(ui->tableWidget_material->rowCount()-1, 0, pWidget0);
        ui->tableWidget_material->setCellWidget(row, 0, pWidget0);
        // チェックボックスON/OFF切り替え時のアクション
        //  int はチェック状態　引数はOFF=0 ON=2. 何行目がチェックされたか送りたいが、今の所方法が見つからない。→
        //  SLOT処理でtable全体を見て処理するしかない。（今　変わったものだけに対してのアクションはできない。）
        connect(pItemCheck0, SIGNAL(stateChanged(int)), this, SLOT(func_tableMateriaRedrawSlot(int)));

        //[1]列目 マテリアル番号
        int matnum = tmpList.at(0).toInt();
        ui->tableWidget_material->setItem(row, 1, new QTableWidgetItem(""));    // 初期値
        if (tmpList.size() > 0) {
            ui->tableWidget_material->setItem(row, 1, new QTableWidgetItem(QString::number(matnum)));
        }

        //[2]列目 マテリアル名
        ui->tableWidget_material->setItem(row, 2, new QTableWidgetItem(""));    // 初期値
        if (tmpList.size() > 0) {
            ui->tableWidget_material->setItem(row, 2, new QTableWidgetItem(tmpList.at(1)));
        }

        //[3]列目　マテリアル色 コンボボックス配置
        // 本番時はコメントアウト解除 これから関数作成//func_tableMaterialColorAdd(row);
        // コンボボックス初期色設定
        CustomColorCombo* tmp_combo =
            new CustomColorCombo(nullptr, ui->tableWidget_material->item(row, 1)->text().toInt(), ui->obj3dViewer);
        tmp_combo->setFocusPolicy(Qt::NoFocus);
        ui->tableWidget_material->setCellWidget(row, 3, tmp_combo);

        // tmp_combo->addItems(QStringList() << "Black" <<"Gray" << "DarkGray" << "Lightgray" << "Green" <<"Brown" <<
        // "Purple"
        //                      << "MediumPurple" <<"Red" << "Magenta" << "Pink" << "Orange" << "Gold" << "Yellow"
        //                      << "Green" <<"Greenyellow" << "Olive" << "Navy" << "Blue" << "Cyan" << "Lightcyan"
        //                      );
        // ↓　DarkGray削除
        tmp_combo->addItems(label_matColorList);
        int tmpIndex = row % 10 + 1;    /// 先頭に空白（任意色）を入れたので必ず＋１する
        tmp_combo->setCurrentIndex(tmpIndex);
        connect(tmp_combo->combobox(), SIGNAL(currentIndexChanged(int)), this,
                SLOT(func_tableMaterialColorSlotChanged(int)));
        QString matColorName = label_matColorList.at(tmpIndex - 1);    /// label_matColorListは+1していない

        //[4]列目　マテリアル色-半透明 チェックボックス配置
        QCheckBox*   pItemCheck1 = new QCheckBox();
        QWidget*     pWidget1    = new QWidget();
        QHBoxLayout* pLayout1    = new QHBoxLayout();
        pItemCheck1->setCheckState(Qt::Checked);
        pLayout1->setAlignment(Qt::AlignCenter);
        pLayout1->addWidget(pItemCheck1);
        pWidget1->setLayout(pLayout1);
        // ui->tableWidget_material->setCellWidget(ui->tableWidget_material->rowCount()-1, 4, pWidget1);
        ui->tableWidget_material->setCellWidget(row, 4, pWidget1);
        pItemCheck1->setCheckState(Qt::Unchecked);    // 初期:半透明OFF
        // チェックボックスON/OFF切り替え時のアクション
        //  int はチェック状態　引数はOFF=0 ON=2. 何行目がチェックされたか送りたいが、今の所方法が見つからない。→
        //  SLOT処理でtable全体を見て処理するしかない。（今　変わったものだけに対してのアクションはできない。）
        // connect(pItemCheck0, SIGNAL(stateChanged(int)), this, SLOT(func_tableMaterialCheckAcolorSlotClicked(int)));
        connect(pItemCheck1, SIGNAL(stateChanged(int)), this, SLOT(func_tableMateriaRedrawSlot(int)));

        //[5]列目 DrawMode
        CustomCombo* tmp_combo2 = new CustomCombo();
        tmp_combo2->setFocusPolicy(Qt::StrongFocus);
        tmp_combo2->addItems(label_drawMode);
        tmp_combo2->setCurrentIndex(def_drawmode);
        ui->tableWidget_material->setCellWidget(row, 5, tmp_combo2);
        connect(tmp_combo2, &QComboBox::currentIndexChanged, this,
                [this](int value) { func_tableMateriaRedrawSlot(value); });

        //[6]列目　マテリアル色-Opt チェックボックス配置
        QCheckBox*   pItemCheck2 = new QCheckBox();
        QWidget*     pWidget2    = new QWidget();
        QHBoxLayout* pLayout2    = new QHBoxLayout();
        pItemCheck2->setCheckState(Qt::Checked);
        pLayout2->setAlignment(Qt::AlignCenter);
        pLayout2->addWidget(pItemCheck2);
        pWidget2->setLayout(pLayout2);
        // ui->tableWidget_material->setCellWidget(ui->tableWidget_material->rowCount()-1, 4, pWidget1);
        ui->tableWidget_material->setCellWidget(row, 6, pWidget2);
        pItemCheck2->setCheckState(Qt::Unchecked);    // 初期:OFF
        connect(pItemCheck2, SIGNAL(stateChanged(int)), this, SLOT(func_tableMateriaRedrawSlot(int)));
        // if(tmpList.at(0)=="0"){
        //     //マテリアル番号:0 (Air) は表示させない。チェックボックスOFFにして、操作不可にする。
        //     pItemCheck0->setCheckState(Qt::Unchecked);
        //     pItemCheck0->setEnabled(false);
        //     //背景色グレーにする　2列目マテリアル番号, 3列目：マテリアル名
        //     ui->tableWidget_material->item(row, 1)->setBackground(Qt::lightGray);
        //     ui->tableWidget_material->item(row, 2)->setBackground(Qt::lightGray);
        // }

        g_hash_matToName[matnum] = tmpList.at(1);    // マテリアル番号からマテリアル名取得用のハッシュ
        g_hash_matToViewOn[matnum] = 1;    // マテリアル番号から、描画表示or非表示情報取得用のハッシュ
        g_hash_matToColorName[matnum] = matColorName;    // マテリアル番号から、描画の色名取得用のハッシュ
        g_hash_matToColorVec[matnum] =
            func_GL_defineColor_nameToRGBvec(matColorName);    // マテリアル番号から、描画の色コード取得用のハッシュ
        g_hash_matToAcolor[matnum] = 1.0;    // マテリアル番号から、表示情報：半透明の取得用のハッシュ
    }

    ui->tableWidget_material->setEditTriggers(QAbstractItemView::NoEditTriggers);

    func_tableMaterialDefaultSet_matnameToColorCombo();    // materialテーブルのセット。特定のマテリアルの場合、色設定コンボボックスを特定の色にセットする。

    g_flag_slotMatColorCombo =
        1;    // マテリアル色コンボのスロット処理有効に戻す。自動設定の時にスロット処理されると実行エラーで落ちてしまうため。
    // 本番時までに直す 異常終了落ちてしまう// func_tableMaterialColorSlotChanged();
    // //マテリアル色コンボボックス部品の背景色を、表示されている値と同じにする(Red, Blue）など
}

void Vox3DForm::func_tableMateriaRedrawSlot(int)    // ユーザー操作で、チェックボックス変更を, simNameテーブルに記録する
{
    if (g_flag_slotMatView == 0) {
        return;
    }    // 処理無効が指定されていたら、処理なし。
    if (g_flag_slotMatColorCombo == 0) {
        return;
    }    // 処理無効が指定されていたら、処理なし。

    // func_tableMaterialDefaultSet_matnameToColorCombo();    //
    // materialテーブルのセット。特定のマテリアルの場合、色設定コンボボックスを特定の色にセットする。

    // QStringList uiTableListMat = func_tableMaterial_gval();
    // QString filepath = ui->lineEdit_voxpath->text();
    func_tableMaterialToGL();    // for-vox
                                 // GL描画更新：ユーザーによるGUI操作された時点で、材質ごとの表示切替え・色切替え
}

void Vox3DForm::func_tableMaterialColorSlotChanged(
    int index)    // ユーザー操作で、マテリアル色選択コンボボックスが変わった場合の処理
{
    if (g_flag_slotMatColorCombo == 0) {
        return;
    }    // 処理無効が指定されていたら、処理なし。

    // func_tableMateriaRedrawSlot(1);    // GL画面をテーブル通りの表示・表示、色で更新

    /// 暫定対策
    /// 本来対象１アイテムだけでいい
    for (int row = 0; row < ui->tableWidget_material->rowCount(); row++) {
        auto tmpcombo = static_cast<CustomColorCombo*>(ui->tableWidget_material->cellWidget(row, 3));

        QString cname   = tmpcombo->currentText();            // 色名 Greenなど
        QString bgcolor = g_hash_cnameToStyleColor[cname];    // bgcolor="#008000" など
        /*
        QString fontcolor = "#000000";                          // 通常は文字色=黒
        if (g_list_whitefontBgColor.indexOf(cname) > -1) {
            fontcolor = "#ffffff";    // 見やすさのため、背景色が濃い色の場合は文字色=白
        }
        // コンボボックス部品の背景色・文字色設定
        //[DEBUG]tmpcombo->setStyleSheet("QComboBox { background-color: #008000 ; color: #ff0000 ; }");
        ////例：背景色　緑　文字色：赤
        QString strStyle = QString("QComboBox { background-color: %1 ; color: %2 ; }").arg(bgcolor, fontcolor);
        tmpcombo->combobox()->setStyleSheet(strStyle);
        */
        tmpcombo->setColor(cname, bgcolor);

        int  matnum = ui->tableWidget_material->item(row, 1)->text().toInt();
        auto color  = tmpcombo->color();    // func_GL_defineColor_nameToRGBvec(cname);
        // qDebug() << "Color Change" << matnum;
        // qDebug() << "Color Change" << cname;
        // qDebug() << "Color Change" << color;
        ui->obj3dViewer->setColor(matnum, color, row == ui->tableWidget_material->rowCount() - 1);
    }

    /// 暫定 コンボボックスにフォーカスが残って色がわかりづらいので
    ui->obj3dViewer->setFocus();

    return;

    // return;    //[DEBUG]本番時は削除する

    QStringList materiallist, materialcolorlist;
    for (int row = 0; row < ui->tableWidget_material->rowCount(); row++) {
        materiallist += ui->tableWidget_material->item(row, 2)->text();
        CustomColorCombo* tmpcombo     = static_cast<CustomColorCombo*>(ui->tableWidget_material->cellWidget(row, 3));
        QString           tmpcolorText = tmpcombo->currentText();
        materialcolorlist += tmpcolorText;

        // コンボボックスウィジェットに色付け
        if (tmpcolorText == "Black") {
            // combocolor->setItemData(0, QColor(0,0,0,255), Qt::BackgroundRole); //01 Black
            // combocolor->setItemData(0, QColor(Qt::white), Qt::TextColorRole);
            tmpcombo->setStyleSheet("QComboBox { background-color: #000000; color:#ffffff;}");    // 背景:黒　文字色：白
        }
        if (tmpcolorText == "Lightgray") {
            tmpcombo->setStyleSheet("QComboBox { background-color: #d3d3d3 }");    // 背景:灰色　文字色：デフォルト
        }
        // ↑正しくは全色設定するべき　現在途中まで。

        // QStringList uiTableListMat =
        //     func_tableMaterial_gval();    // openGL描画処理の別cppに値渡しするため、グローバル変数更新
        // QString filepath = ui->lineEdit_voxpath->text();
        //  ◆これから記述する予定 別cppへの値渡しと openGL描画処理呼び出し
    }

    // openGL
}

// QStringList Vox3DForm::func_gval_table_material() //GL描画処理用の別cpp への値渡しのため、グローバル変数を更新する。
QStringList Vox3DForm::func_tableMaterial_gval()    // GL描画処理用の別cpp への値渡しのため、グローバル変数を更新する。
{
    QStringList lineList;
    g_hash_matToName.clear();

    for (int row = 0; row < ui->tableWidget_material->rowCount(); row++) {
        int matnum = ui->tableWidget_material->item(row, 1)->text().toInt();
        g_hash_matToName[matnum] =
            ui->tableWidget_material->item(row, 2)->text();    // マテリアル番号からマテリアル名取得用のハッシュ

        QString oneline = "";
        for (int col = 0; col < ui->tableWidget_material->horizontalHeader()->count(); col++) {
            QString colstr = "";
            if (col != 0 && col != 3 && col != 4 && col != 5 && col != 6) {
                colstr = ui->tableWidget_material->item(row, col)->text();
            }
            if (col == 0 || col == 4 || col == 5 || col == 6) {
                // テーブル チェックボックス状態
                if (col == 0) {
                    QWidget*   pWidget  = ui->tableWidget_material->cellWidget(row, 0);
                    QCheckBox* checkbox = pWidget->findChild<QCheckBox*>();
                    int        check    = 0;
                    if (checkbox->isChecked()) {
                        check = 1;
                    }
                    colstr = QString::number(check);    // 1列目
                    g_hash_matToViewOn[matnum] = check;    // マテリアル番号から、描画表示or非表示情報取得用のハッシュ
                }
                if (col == 4) {
                    // マテリアル番号から、描画の半透明情報取得用のハッシュに値設定する
                    g_hash_matToAcolor[matnum] = 1.0;
                    QWidget*   pWidget         = ui->tableWidget_material->cellWidget(row, 4);
                    QCheckBox* checkbox        = pWidget->findChild<QCheckBox*>();
                    int        check           = 0;
                    if (checkbox->isChecked()) {
                        check                      = 1;
                        g_hash_matToAcolor[matnum] = 0.3;
                    }
                    colstr = QString::number(check);    // 4列目
                }
                if (col == 5) {
                    QComboBox*  combobox = (QComboBox*)ui->tableWidget_material->cellWidget(row, 5);
                    const auto& text     = combobox->currentText();
                    colstr               = text;
                }
                if (col == 6) {
                    QWidget*   pWidget  = ui->tableWidget_material->cellWidget(row, 6);
                    QCheckBox* checkbox = pWidget->findChild<QCheckBox*>();
                    int        check    = 0;
                    if (checkbox->isChecked()) {
                        check = 1;
                    }
                    colstr = QString::number(check);    // 5列目
                }
            }
            if (col == 3) {
                // テーブル comboBox状態
                CustomColorCombo* tmpcombo =
                    static_cast<CustomColorCombo*>(ui->tableWidget_material->cellWidget(row, 3));
                QString colorName = tmpcombo->currentText();
                colstr            = colorName;
                g_hash_matToColorName[matnum] = colorName;    // マテリアル番号から、描画の色名取得用のハッシュ
                g_hash_matToColorVec[matnum] = func_GL_defineColor_nameToRGBvec(
                    colorName);    // マテリアル番号から、描画の色コード取得用のハッシュ
            }
            oneline += colstr + ",";
        }
        oneline.chop(1);    // 最後の "," は削除する
        lineList << oneline;
    }

    // QMessageBox::information(this, "notice", msg);
    // qDebug() << "[DEBUG]func_tableComboMy01_changed() " + linelist

    return (lineList);
}

QStringList Vox3DForm::func_getMaterialfromVoxPath(QString in_filepath)    // voxfile からマテリアル行のみ取得する。
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

void Vox3DForm::func_tableMaterialColorAdd(int in_row)    // table_material の指定行in_row に色付きComboBOX1個を登録する
{
    CustomColorCombo* combocolor =
        new CustomColorCombo(this, ui->tableWidget_material->item(in_row, 1)->text().toInt(), ui->obj3dViewer);
    combocolor->setFocusPolicy(Qt::NoFocus);

    combocolor->addItems(QStringList() << "Black"
                                       << "Gray"
                                       << "Lightgray"
                                       << "Green"
                                       << "Brown"
                                       << "Purple"
                                       << "MidiumPurple"
                                       << "Red"
                                       << "Magenta"
                                       << "Pink"
                                       << "Orange"
                                       << "Gold "
                                       << "Yellow"
                                       << "Green"
                                       << "Greenyellow"
                                       << "Olive"
                                       << "Navy"
                                       << "Blue"
                                       << "Cyan"
                                       << "Lightcyan");

    // 1つのComboBoxの選択肢の見た目設定　背景色・文字色
    combocolor->setItemData(0, QColor(0, 0, 0, 255), Qt::BackgroundRole);          // 00 Black 背景色
    combocolor->setItemData(0, QColor(Qt::white), Qt::ForegroundRole);             // 00 Black 文字色
    combocolor->setItemData(1, QColor(128, 128, 128, 255), Qt::BackgroundRole);    // 01 Gray
    combocolor->setItemData(1, QColor(Qt::white), Qt::ForegroundRole);
    combocolor->setItemData(2, QColor(211, 211, 211, 255), Qt::BackgroundRole);    // 02 Lightgray
    // combocolor->setItemData(3, QColor(255, 255, 255,255), Qt::BackgroundRole); //03 White
    combocolor->setItemData(
        3, QColor(0, 255, 0, 255),
        Qt::BackgroundRole);    // 03 Green //暫定置き換え White->Green  //openGL背景白と重ならないようにするため
    combocolor->setItemData(3, QColor(Qt::white), Qt::ForegroundRole);

    combocolor->setItemData(4, QColor(165, 42, 42, 255), Qt::BackgroundRole);    // 04 Brown
    combocolor->setItemData(4, QColor(Qt::white), Qt::ForegroundRole);
    combocolor->setItemData(5, QColor(128, 0, 128, 255), Qt::BackgroundRole);    // 05 Purple
    combocolor->setItemData(5, QColor(Qt::white), Qt::ForegroundRole);
    combocolor->setItemData(6, QColor(147, 112, 219, 255), Qt::BackgroundRole);    // 06 MediumPurple
    combocolor->setItemData(6, QColor(Qt::white), Qt::ForegroundRole);
    combocolor->setItemData(7, QColor(255, 0, 0, 255), Qt::BackgroundRole);        // 07 Red
    combocolor->setItemData(8, QColor(255, 0, 255, 255), Qt::BackgroundRole);      // 08 Magenta
    combocolor->setItemData(9, QColor(255, 192, 203, 255), Qt::BackgroundRole);    // 09 Pink
    combocolor->setItemData(10, QColor(255, 165, 0, 255), Qt::BackgroundRole);     // 10 Orange
    combocolor->setItemData(11, QColor(255, 215, 0, 255), Qt::BackgroundRole);     // 11 Gold
    combocolor->setItemData(12, QColor(255, 255, 0, 255), Qt::BackgroundRole);     // 12 Yellow
    combocolor->setItemData(13, QColor(0, 255, 0, 255), Qt::BackgroundRole);       // 13 Green
    combocolor->setItemData(13, QColor(Qt::white), Qt::ForegroundRole);
    combocolor->setItemData(14, QColor(173, 255, 47, 255), Qt::BackgroundRole);    // 14 Greenyellow
    combocolor->setItemData(14, QColor(128, 128, 0, 255), Qt::BackgroundRole);     // 15 Olive
    // ↑サンプルとして途中まで。。
    connect(
        combocolor->combobox(), &QComboBox::currentIndexChanged, this,
        &Vox3DForm::func_tableMaterialColorSlotChanged);    // materialテーブルのセット。マテリアルごとに表示・非表示,
    // 色の変更などの用途。
    ui->tableWidget_material->setCellWidget(in_row, 3, combocolor);
}

// void Vox3DForm::on_DEBUG01_pushButton_clicked()    // for-vox
//{
//     qDebug() << "[DEBUG]Vox3DFormSub01.cpp - on_DEBUG01_pushButton_clicked";
// }

// void Vox3DForm::on_pushButton_matTableToGL_clicked()
//{
//     func_tableMaterialToGL();    // for-vox
//                                  // GL描画更新：ユーザーによるGUI操作された時点で、材質ごとの表示切替え・色切替え
// }

// void Vox3DForm::func_tableMaterialToGL() //for-vox
// GL描画更新：ユーザーによるGUI操作された時点で、材質ごとの表示切替え・色切替え
//{

//    qDebug() << "[DEBUG]Vox3DForm.cpp-func_tableMaterialToGL() start-time=" +
//    QDateTime::currentDateTime().toString("hh:mm:ss");;

//    //foreach(int key, g_voxMatToPnumHash.uniqueKeys()){  qDebug()  << "[DEBUG]Vox3DForm.cpp-func_tableMaterialToGL()
//    g_voxMatToPnumHash key=" << QString::number(key) << " values=" << g_voxMatToPnumHash.values(key); }

//    //GUI マテリアルテーブルから情報取得
//    QStringList uiMatTableList = func_tableMaterial_gval();
//    //openGL描画処理の別cppに値渡しするため、グローバル変数更新 QList<int> nonDrawMatList, drawMatList;
//    QVector<QVector3D> colorRGBvec;
//    QHash<int, QVector3D> colorNameToRGBhash;
//    QList<int> tableMatnumList;
//    for(int i=0; i < uiMatTableList.count(); i++){
//        QStringList tmpStrList =  uiMatTableList.at(i).trimmed().split(",");
//        int flag_chk = tmpStrList.at(0).toInt();
//        int matnum = tmpStrList.at(1).toInt();
//        QString colorname = tmpStrList.at(3);

//        if(flag_chk == 0 ){ nonDrawMatList << matnum; }
//        if(flag_chk == 1 ){ drawMatList << matnum; }
//        tableMatnumList << matnum;

//        QVector3D colorRGB = func_GL_defineColor_nameToRGBvec(colorname);
//        colorNameToRGBhash[matnum] = colorRGB;
//    }

//    QString tmpStr = "";

//    //qDebug() << "[DEBUG]Vox3DForm.cpp-func_tableMaterialToGL() nonDrawMatList=" << nonDrawMatList;
//    //qDebug() << "[DEBUG]Vox3DForm.cpp-func_tableMaterialToGL() tableMatnumList=" << tableMatnumList;
//    //foreach(int key, colorNameToRGBhash.uniquekeys()){
//    //    qDebug() << "[DEBUG]Vox3DForm.cpp-func_tableMaterialToGL() key" << QString::number(key) << " RGBcolor:x=" <<
//    QString::number(colorNameToRGBhash[key].x()) << " y=" << QString::number(colorNameToRGBhash[key].y()) << " z=" <<
//    QString::number(colorNameToRGBhash[key].z());
//    //}

//    //表示切替　GUIテーブル:チェックONだけ表示する、OFF表示なし / 色・コンボボックス指定の色にする
//    QVector<QVector3D> tmp_voxXYZVec;
//    QStringList tmp_voxSurfaceStrList;
//    QVector<QVector3D> tmp_voxColorVec;
//    QVector<int>tmp_voxMatnumVec;
//    for(int pnum=0; pnum < g_voxMatnumVec.size() ; pnum++){
//        int matnum = g_voxMatnumVec.at(pnum);
//        if(drawMatList.indexOf(matnum) > -1){
//            tmp_voxXYZVec.append(g_voxXYZVec.at(pnum));
//            tmp_voxSurfaceStrList.append(g_voxSurfaceStrList.at(pnum));
//            tmp_voxMatnumVec.append(g_voxMatnumVec.at(pnum));
//            tmp_voxColorVec.append(colorNameToRGBhash[matnum]); //色・コンボボックス指定の色にする
//        }
//    }

//    //----------------------------------------------------
//    //voxファイルからの取得情報を、openGL描画形式のデータに置き換える
//    //openGLでの描画単位=1つの三角形ごとの情報作成する。 QVector<QOpenGLTriangle3D_vox> tmp_triangles; qDebug() <<
//    "[DEBUG]voxRead_makeGLdata.cpp-func_01main_GL_make: start makeGLinfo " +
//    QDateTime::currentDateTime().toString("hh:mm:ss"); func_GL_make_getPointOfMesh(tmp_triangles, tmp_voxXYZVec,
//    tmp_voxSurfaceStrList, tmp_voxColorVec, tmp_voxMatnumVec); qDebug() <<
//    "[DEBUG]voxRead_makeGLdata.cpp-func_01main_GL_make: end makeGLinfo " +
//    QDateTime::currentDateTime().toString("hh:mm:ss");

//    // 半透明ON/OFF設定 (全図形半透明 or 全図形通常表示)
//    int flag_acolor = 0;
//    if(ui->checkBox_acolor->isChecked()){ flag_acolor = 1; }
//    ui->obj3dViewer->g_ui_acolorflag = flag_acolor;

//    //voxファイルからの情報を渡して、openGL描画する
//    qDebug() << "[DEBUG]Vox3DForm.cpp-on_DEBUG01_pushButton_clicked(): start openGL-draw-setTriangle " +
//    QDateTime::currentDateTime().toString("hh:mm:ss");
//    //ui->obj3dViewer->func_vox_setTriangles(triangles);
//    ui->obj3dViewer->func_vox_setTriangles(tmp_triangles);//グローバル変数に入れて、色切替え・表示切り替えできるようにする
//    qDebug() << "[DEBUG]Vox3DForm.cpp-on_DEBUG01_pushButton_clicked(): end openGL-draw-setTriangle " +
//    QDateTime::currentDateTime().toString("hh:mm:ss");

//}

void Vox3DForm::
    func_tableMaterialToGL()    // for-vox GL描画更新：ユーザーによるGUI操作された時点で、材質ごとの表示切替え・色切替え
{
    // qDebug() << "[DEBUG]Vox3DForm.cpp-func_tableMaterialToGL() start-time="
    //                 + QDateTime::currentDateTime().toString("hh:mm:ss");
    //;

    // foreach(int key, g_voxMatToPnumHash.uniqueKeys()){  qDebug()  << "[DEBUG]Vox3DForm.cpp-func_tableMaterialToGL()
    // g_voxMatToPnumHash key=" << QString::number(key) << " values=" << g_voxMatToPnumHash.values(key); }

    // GUI マテリアルテーブルから情報取得
    QStringList uiMatTableList =
        func_tableMaterial_gval();    // openGL描画処理の別cppに値渡しするため、グローバル変数更新
    QList<int>                           nonDrawMatList, drawMatList, toumeiList, optList;
    std::map<int, std::pair<bool, bool>> drawModeList;
    QVector<QVector3D>                   colorRGBvec;
    QHash<int, QVector3D>                colorNameToRGBhash;
    QList<int>                           tableMatnumList;
    for (int i = 0; i < uiMatTableList.count(); i++) {
        QStringList tmpStrList = uiMatTableList.at(i).trimmed().split(",");
        int         flag_chk   = tmpStrList.at(0).toInt();
        int         matnum     = tmpStrList.at(1).toInt();
        QString     colorname  = tmpStrList.at(3);
        int         toumei     = tmpStrList.at(4).toInt();
        QString     drawMode   = tmpStrList.at(5);
        int         opt        = tmpStrList.at(6).toInt();

        if (flag_chk == 0) {
            nonDrawMatList << matnum;
        }
        if (flag_chk == 1) {
            drawMatList << matnum;
        }
        if (toumei == 1) {
            toumeiList << matnum;
        }
        if (opt == 1) {
            optList << matnum;
        }
        tableMatnumList << matnum;

        auto& draw_flag = drawModeList[matnum];
        if (drawMode == "Shade") {
            draw_flag.first  = true;
            draw_flag.second = false;
        }
        else if (drawMode == "Wire") {
            draw_flag.first  = false;
            draw_flag.second = true;
        }
        else if (drawMode == "S+W") {
            draw_flag.first  = true;
            draw_flag.second = true;
        }

        QVector3D colorRGB         = func_GL_defineColor_nameToRGBvec(colorname);
        colorNameToRGBhash[matnum] = colorRGB;
    }
#ifdef ORIGINAL_OBJ3_VIEW    /// TODO: ビュー置き換え中によりコメントアウト
#else
    bool change_show = ui->obj3dViewer->noshowMaterial(nonDrawMatList, false);
    ui->obj3dViewer->transparentMateial(toumeiList, false);
    ui->obj3dViewer->drawMode(drawModeList, false);
    ui->obj3dViewer->optMateial(optList, true);
    m_clipping_ctrl->updateClipping();
    if (change_show) {
        m_result_ctrl->updateResult();
    }
    return;
#endif

    QString tmpStr = "";

    // qDebug() << "[DEBUG]Vox3DForm.cpp-func_tableMaterialToGL() nonDrawMatList=" << nonDrawMatList;
    // qDebug() << "[DEBUG]Vox3DForm.cpp-func_tableMaterialToGL() tableMatnumList=" << tableMatnumList;
    // foreach(int key, colorNameToRGBhash.uniquekeys()){
    //     qDebug() << "[DEBUG]Vox3DForm.cpp-func_tableMaterialToGL() key" << QString::number(key) << " RGBcolor:x=" <<
    //     QString::number(colorNameToRGBhash[key].x()) << " y=" << QString::number(colorNameToRGBhash[key].y()) << "
    //     z=" << QString::number(colorNameToRGBhash[key].z());
    // }

    // 表示切替　GUIテーブル:チェックONだけ表示する、OFF表示なし / 色・コンボボックス指定の色にする
    // QVector<QVector3D> tmp_voxXYZVec;
    QVector<QVector3D> tmp_voxXYZStartVec, tmp_voxXYZEndVec;
    QStringList        tmp_voxSurfaceStrList;
    QVector<QVector3D> tmp_voxColorVec;
    // QVector<QVector2D> tmp_voxAcolorVec;
    QVector<float> tmp_voxAcolorVec;
    QVector<int>   tmp_voxMatnumVec;
    for (int pnum = 0; pnum < g_voxMatnumVec.size(); pnum++) {
        int matnum = g_voxMatnumVec.at(pnum);
        if (drawMatList.indexOf(matnum) > -1) {
            // tmp_voxXYZVec.append(g_voxXYZVec.at(pnum));
            tmp_voxXYZStartVec.append(g_voxXYZStartVec.at(pnum));
            tmp_voxXYZEndVec.append(g_voxXYZEndVec.at(pnum));
            tmp_voxSurfaceStrList.append(g_voxSurfaceStrList.at(pnum));
            tmp_voxMatnumVec.append(g_voxMatnumVec.at(pnum));
            tmp_voxColorVec.append(colorNameToRGBhash[matnum]);    // 色・コンボボックス指定の色にする
            tmp_voxAcolorVec.append(g_hash_matToAcolor[matnum]);
            // float tmp_acolor = 1.0;
            // if(matnum > 20){
            //     tmp_acolor = 0.3;
            // }
            // tmp_voxAcolorVec.append(QVector2D(tmp_acolor,tmp_acolor)); //色・コンボボックス指定の色にする
        }
    }

    input_getPointOfMesh_by2point_jiku(tmp_voxSurfaceStrList, tmp_voxXYZStartVec, tmp_voxXYZEndVec, tmp_voxMatnumVec,
                                       tmp_voxColorVec, tmp_voxAcolorVec);    // 軸表示用図形追加
    // input_getPointOfMesh_by2point_jiku_worldZahyo(tmp_voxSurfaceStrList, tmp_voxXYZStartVec, tmp_voxXYZEndVec,
    // tmp_voxMatnumVec, tmp_voxColorVec, tmp_voxAcolorVec); //軸表示用図形追加

    //----------------------------------------------------
    // voxファイルからの取得情報を、openGL描画形式のデータに置き換える
    // //openGLでの描画単位=1つの三角形ごとの情報作成する。
    QVector<QOpenGLTriangle3D_vox> tmp_triangles;
    // qDebug() << "[DEBUG]Vox3DForm.cpp-func_tableMaterialToGL():  start makeGLinfo "
    //                 + QDateTime::currentDateTime().toString("hh:mm:ss");
    //  func_GL_make_getPointOfMesh(tmp_triangles, tmp_voxXYZVec, tmp_voxSurfaceStrList, tmp_voxColorVec,
    //  tmp_voxMatnumVec);
    func_GL_make_getPointOfMesh_by2point(tmp_triangles, tmp_voxXYZStartVec, tmp_voxXYZEndVec, tmp_voxSurfaceStrList,
                                         tmp_voxMatnumVec, tmp_voxColorVec, tmp_voxAcolorVec);
    // qDebug() << "[DEBUG]voxRead_makeGLdata.cpp-func_01main_GL_make: end makeGLinfo "
    //                 + QDateTime::currentDateTime().toString("hh:mm:ss");

    // 半透明ON/OFF設定 (全図形半透明 or 全図形不透明)
    int flag_acolor = 0;
    if (ui->checkBox_acolor->isChecked()) {
        flag_acolor = 1;
    }
#ifdef ORIGINAL_OBJ3_VIEW    /// TODO: ビュー置き換え中によりコメントアウト
    ui->obj3dViewer->g_ui_acolorflag = flag_acolor;
#endif
    // 半透明ON/OFF設定 (マテリアルごとGUI半透明切り替えチェックボックスの通り　半透明 or 不透明)

    // voxファイルからの情報を渡して、openGL描画する
    // qDebug() << "[DEBUG]Vox3DForm.cpp-func_tableMaterialToGL(): start openGL-draw-setTriangle "
    //                + QDateTime::currentDateTime().toString("hh:mm:ss");
    // ui->obj3dViewer->func_vox_setTriangles(triangles);
#ifdef ORIGINAL_OBJ3_VIEW    /// TODO: ビュー置き換え中によりコメントアウト
    ui->obj3dViewer->func_vox_setTriangles(
        tmp_triangles);    // グローバル変数に入れて、色切替え・表示切り替えできるようにする
#endif
    // qDebug() << "[DEBUG]Vox3DForm.cpp-on_func_tableMaterialToGL(): end openGL-draw-setTriangle "
    //                 + QDateTime::currentDateTime().toString("hh:mm:ss");
}

void Vox3DForm::on_pushButton_viewAllOn_clicked()
{
    g_flag_slotMatView = 0;    // スロット処理無効にする　（都度変更されて、GL反映だと時間がかかりすぎるため）

    for (int row = 0; row < ui->tableWidget_material->rowCount(); row++) {
        QWidget*   pWidget  = ui->tableWidget_material->cellWidget(row, 0);
        QCheckBox* checkbox = pWidget->findChild<QCheckBox*>();
        checkbox->setChecked(true);
    }

    g_flag_slotMatView = 1;    // スロット処理有効に戻す

    func_tableMateriaRedrawSlot(1);    // GL画面をテーブル通りの表示・表示、色で更新
}

void Vox3DForm::on_pushButton_viewAllOff_clicked()
{
    g_flag_slotMatView = 0;    // スロット処理無効にする　（都度変更されて、GL反映だと時間がかかりすぎるため）

    for (int row = 0; row < ui->tableWidget_material->rowCount(); row++) {
        QWidget*   pWidget  = ui->tableWidget_material->cellWidget(row, 0);
        QCheckBox* checkbox = pWidget->findChild<QCheckBox*>();
        checkbox->setChecked(false);
    }

    g_flag_slotMatView = 1;    // スロット処理有効に戻す

    func_tableMateriaRedrawSlot(1);    // GL画面をテーブル通りの表示・表示、色で更新
}

void Vox3DForm::on_lineEdit_rotateAngleY_editingFinished()
{
#ifdef ORIGINAL_OBJ3_VIEW    /// TODO: ビュー置き換え中によりコメントアウト
    ui->obj3dViewer->func_vox_setRotation(ui->lineEdit_rotateAngleY->text().toFloat(),
                                          ui->lineEdit_rotateAngleZ->text().toFloat());

    ui->obj3dViewer->update();    // have to call this manually (for better performance)
#endif
}

void Vox3DForm::on_lineEdit_rotateAngleZ_editingFinished()
{
#ifdef ORIGINAL_OBJ3_VIEW    /// TODO: ビュー置き換え中によりコメントアウト
    ui->obj3dViewer->func_vox_setRotation(ui->lineEdit_rotateAngleY->text().toFloat(),
                                          ui->lineEdit_rotateAngleZ->text().toFloat());

    ui->obj3dViewer->update();    // have to call this manually (for better performance)
#endif
}

// void Vox3DForm::on_vox_pushButton_setDefaultViewMenu_clicked()
//{
//     // タブ VoxOption
//     ui->comboBox_voxRotate->setCurrentText("front");
//     func_setDefaultViewMenu();
// }

// 視点の初期化
void Vox3DForm::func_setDefaultViewMenu()
{
    // タブ perspective
    ui->persVerticalAngleSpin->setValue(30.00);
    // ui->persNearSpin->setValue(0.10);
    ui->persFarSpin->setValue(100.00);

    // タブ Look At
    ui->lookEyeXSpin->setValue(-5.00);
    ui->lookEyeYSpin->setValue(2.00);
    ui->lookEyeZSpin->setValue(5.00);

    ui->lookCenterXSpin->setValue(0.00);
    ui->lookCenterYSpin->setValue(0.00);
    ui->lookCenterZSpin->setValue(0.00);

    ui->lookUpXSpin->setValue(0.00);
    ui->lookUpYSpin->setValue(1.00);
    ui->lookUpZSpin->setValue(0.00);

    // タブ Transration
    // ui->scaleSpin->setValue(1.00);
    //
    // 構造物の大きさを、全体が画面にぴったり収まるくらいで表示するように調整
    ui->scaleSpin->setValue(
        2.5);    // どのタイプでも 2 でOK。( 3でもOK。少し大き目)　-1～1範囲で正規化して表示してあるから。

    // ui->translateXSpin->setValue(0.00);
    // ui->translateYSpin->setValue(0.00);
    // ui->translateZSpin->setValue(0.00);
    //
    // 回転動作を構造物中心で行うため　(ワールド座標で、物体を置く位置をずらす)
    ui->translateXSpin->setValue(-(nx * meshsizeGL_x) / 2);
    ui->translateYSpin->setValue(-(nz * meshsizeGL_z) / 2);
    ui->translateZSpin->setValue(ny * meshsizeGL_y / 2);    // gl座標Yは手前(+) 奥(-)。　手前側+にずらす。

    ui->rotateAngleSlider->setValue(0.00);
    ui->rotateXSpin->setValue(0.00);
    ui->rotateYSpin->setValue(0.00);
    ui->rotateZSpin->setValue(0.00);

    // タブ Texture
    // ui->textureFileEdit->setText("");

    // タブ Diffuse
    ui->lightPosXSpin->setValue(-25.00);
    ui->lightPosYSpin->setValue(25.00);
    ui->lightPosZSpin->setValue(25.00);

    ui->lightKd1Spin->setValue(0.00);
    ui->lightKd2Spin->setValue(1.00);
    ui->lightKd3Spin->setValue(0.00);

    ui->lightLd1Spin->setValue(1.00);
    ui->lightLd2Spin->setValue(1.00);
    ui->lightLd3Spin->setValue(1.00);

    ui->obj3dViewer->update();
}

void Vox3DForm::func_slot_mousewheelZoom(float in_modelScale)
{
    ui->scaleSpin->setValue(double(
        in_modelScale));    // 値セットするだけで、GUI変更によるシグナルスロット　on_scaleSpin_valueChanged　でopenGL画面が更新される
}

// void Vox3DForm::on_checkBox_voxshift_stateChanged(int arg1)
//{
//     ui->obj3dViewer->g_ui_voxshiftflag = 0;
//     if(ui->checkBox_voxshift->isChecked()){
//         ui->obj3dViewer->g_ui_voxshiftflag = 1;
//     }
// }

void Vox3DForm::func_slot_mousePress(float in_x, float in_y)
{
    // マウスボタンが押された時の動作
    // 現状使わなくなったので処理なし
    Q_UNUSED(in_x);
    Q_UNUSED(in_y);
}

void Vox3DForm::func_slot_mouseRelease(float in_x, float in_y)
{
    // マウスボタンが離された時の動作
    // 現状使わなくなったので処理なし
    Q_UNUSED(in_x);
    Q_UNUSED(in_y);
}

void Vox3DForm::func_slot_mouseDrag02(int in_mouseMoveX, int in_mouseMoveY)
{    // マウスドラッグ時の動作。
    // qDebug() << QString("[DEBUG]Vox3DForm.cpp-func_slot_mouseDrag02 %d %d").arg( in_mouseMoveX, in_mouseMoveY );
    //    if(ui->vox_radioButton_mouseRotate->isChecked()){
    //        func_slot_mouseVoxRotate(in_mouseMoveX, in_mouseMoveY);
    //    }
    //    if(ui->vox_radioButton_mouseShift->isChecked()){
    //        func_slot_mouseVoxShift(in_mouseMoveX, in_mouseMoveY);
    //    }

    if (ui->vox_comboBox_mouseAct->currentText() == "Rotate") {
        func_slot_mouseVoxRotate(in_mouseMoveX, in_mouseMoveY);
    }
    if (ui->vox_comboBox_mouseAct->currentText() == "Shift") {
        func_slot_mouseVoxShift(in_mouseMoveX, in_mouseMoveY);
    }
}

void Vox3DForm::func_slot_mouseRotate(float in_rotateAngleY, float in_rotateAngleZ)
{
    // 旧案1 通常は、マウスドラッグで図形回転モード (GUIで図形平行移動モード=チェックOFFの場合)
    //    qDebug() << "[DEBUG]01 Vox3DFormSub01.cpp-func_slot_mouseRotate.cpp";
    //    ui->lineEdit_rotateAngleY->setText(QString::number(in_rotateAngleY));
    //    ui->lineEdit_rotateAngleZ->setText(QString::number(in_rotateAngleZ));
    //    ui->obj3dViewer->update();
}

//-start- 旧案1 [bkup] マウス操作での物体回転　カメラ位置([Lookat]タブ-[Eye])
// void Vox3DForm::func_slot_mouseVoxRotate(int in_mouseMoveX, int in_mouseMoveY){
//    //カメラ位置を変更することで回転する　//平行移動後、動作NGあり(回転軸中心ずれ)　平行移動:[translate]タブでの操作でも、
//    平行移動[Lookat]タブ-[Center]でもNG. double alpha, beta; int deltaX = in_mouseMoveX; int deltaY = in_mouseMoveY;
//    alpha = 0; //後で修正する　eyeX・Y・Zの位置から計算する　XY面の角度を計算する
//    //beta  = 0; //後で修正する　eyeX・Y・Zの位置から計算する　YZ（XZかも?)面の角度を計算する
//    //alpha -= double(deltaX);
//    alpha = double(in_mouseMoveX);
//    while (alpha < 0) {
//        alpha += 360;
//    }
//    while (alpha >= 360) {
//        alpha -= 360;
//    }
//
////    beta -= double(deltaY);
////    if (beta < -90) {
////        beta = -90;
////    }
////    if (beta > 90) {
////        beta = 90;
////    }
//
//    double alphaV = 0;
//    alphaV -= double(deltaY);
//    while (alphaV < 0) {
//        alphaV += 360;
//    }
//    while (alphaV >= 360) {
//        alphaV -= 360;
//    }
//    double alphaRad = alpha * M_PI / 180;
//    double alphaVRad = alphaV * M_PI / 180;
//
//    //qDebug() << "[DEBUG]Vox3DFormSub01.cpp-func_slot_mouseDrag02 mouseMove alpha=" << QString::number(alpha) << "
//    beta=" << QString::number(beta) ;
//    //qDebug() << "\n[DEBUG]Vox3DFormSub01.cpp-func_slot_mouseDrag02 mouseMove mouseMoveX=" <<
//    QString::number(in_mouseMoveX) << " mouseMoveY=" << QString::number(in_mouseMoveY) ;
//    //qDebug() << "[DEBUG]Vox3DFormSub01.cpp-func_slot_mouseDrag02 alpha　deg=" << QString::number(alpha) << " rad="
//    << QString::number(alphaRad) << " alphaV deg=" << QString::number(alphaV)  << " rad=" <<
//    QString::number(alphaVRad) ;
//
//    //通常は、マウスドラッグで図形回転モード(カメラ視点を変える。追従して光源方向も変える。）
//    //水平方向回転 右にドラッグ→時計回りに回転　左にドラッグ→反時計回りに回転。
//    //問題あり、これから直す。270°以上になった時に、271°でなく0°（？）にスキップしてしまう。。
//    double oldX = ui->lookEyeXSpin->value();
//    double oldY = ui->lookEyeZSpin->value(); //フォームのEyeZSpin 水平方向(Y)　(GL座標のネーミングになっている?）
//    double circleLen = 5; //カメラ位置 直径決め打ち
//    double oldtan =oldY/oldX;
//    double maxtan = 89 * 180/M_PI; //tan(90°)は無限大値になってしまい判定に使えないので 89°で計算)
//    if(oldtan > maxtan){
//        oldtan = maxtan;
//    }
//    double old_angleRad = atan(oldtan); // 単位：ラジアン
//    -90°～90°の範囲になり、第1-2象限、第4-3象限で区別がつかない。 double old_angleDeg = old_angleRad * 180/M_PI;
//    if(oldX >= 0 && oldY >= 0 ){ //第1象限
//        old_angleDeg = old_angleRad * 180/M_PI; //そのまま使う
//    }
//    if(oldX < 0  && oldY >= 0 ){ //第2象限
//        old_angleDeg = 180 -  abs(old_angleDeg);
//    }
//    if(oldX < 0  && oldY < 0 ){ //第3象限
//        old_angleDeg = abs(old_angleDeg) + 180;
//    }
//    if(oldX >0 && oldY < 0){ //第4象限
//        old_angleDeg = 360 - abs(old_angleDeg);
//    }
//    int new_angleDeg = old_angleDeg + alpha;
//    while (new_angleDeg < 0) {
//        new_angleDeg += 360;
//    }
//    while (new_angleDeg >= 360) {
//        new_angleDeg -= 360;
//    }
//    double new_angleRad = new_angleDeg  * M_PI/180;
//    double newX = cos(new_angleRad) * circleLen ; //[DEBUG]直径5決め打ち
//    double newY = sin(new_angleRad) * circleLen ; //[DEBUG]直径5決め打ち
//    int in_mouseMoveX_DEBUG = in_mouseMoveX;
//
//    //qDebug() << "[DEBUG]Vox3DFormSub01.cpp-func_slot_mouseDrag02 angleRad before-rad " <<
//    QString::number(old_angleRad) <<  " rad=" << QString::number(angleRad) << " degree=" << QString::number(angleRad *
//    180/M_PI) << " oldX=" << QString::number(oldX) << " oldY=" << QString::number(oldY);
//    ui->lookEyeXSpin->setValue(newX);
//    ui->lookEyeZSpin->setValue(newY);
//
//    //垂直方向回転　上にドラッグ→時計回りに回転　下にドラッグ→反時計回りに回転。
//    double oldZ =ui->lookEyeYSpin->value(); //フォームのEyeYSpin 上下方向　(GL座標のネーミングになっている?）
//    //qDebug() << "[DEBUG]Vox3DFormSub01.cpp-func_slot_mouseDrag02 before-GUI oldZ=" << QString::number(oldZ) << "
//    alphaVRad=" << QString::number(alphaVRad) ; double zplus = abs( circleLen * sin(alphaVRad) ); double newZ = oldZ +
//    zplus ; if(in_mouseMoveY > 0  && oldZ + zplus < circleLen){
//        newZ = oldZ + zplus ;
//    }
//    if(in_mouseMoveY < 0  && oldZ - zplus > -circleLen){
//        newZ = oldZ - zplus ;
//    }
//    ui->lookEyeYSpin->setValue(newZ);
//
//    //光源もカメラの位置に追従
//    double lightPosX = abs(ui->lightPosXSpin->value());
//    double lightPosY = abs(ui->lightPosYSpin->value());
//    double lightPosZ = abs(ui->lightPosZSpin->value());
//    if(ui->lookEyeXSpin->value() < 0) { lightPosX = -1 * lightPosX ; } //openGL座標のネーミング　lightPosZ=Y方向
//    if(ui->lookEyeYSpin->value() < 0) { lightPosY = -1 * lightPosY ; } //openGL座標のネーミング　Z方向
//    if(ui->lookEyeZSpin->value() < 0) { lightPosZ = -1 * lightPosZ ; } //openGL座標のネーミング　lookEyeZSpin=Y方向
//    ui->lightPosXSpin->setValue(lightPosX);
//    ui->lightPosYSpin->setValue(lightPosY);
//    ui->lightPosZSpin->setValue(lightPosZ);
//
//}
//-end- 旧案1 [bkup] マウス操作での物体回転　カメラ位置([Lookat]タブ-[Eye])

void Vox3DForm::func_slot_mouseVoxRotate(int in_mouseMoveX, int in_mouseMoveY)
{
    //============================================================================
    // 案2 マウス操作での物体回転　([traslation]タブ-[rotate] = 物体自身のローカル座標で設定する)
    //============================================================================
    // in_mouseMoveX, Yとも実値=-5～5 程度のみが入ってくる
    // (動き始めてから止まるまでの距離でなく、動かしている間中都度・微小移動での値が入って来るため)
    float old_angleDeg = ui->rotateAngleSpin->value();
    float old_rotateX  = ui->rotateXSpin->value();
    float old_rotateY  = ui->rotateYSpin->value();
    float old_rotateZ  = ui->rotateZSpin->value();

    // 回転角度
    float new_angleDeg =
        old_angleDeg + in_mouseMoveX
        + in_mouseMoveY;    // %360
                            // して、360°越えないようにするか後で修正したほうがいいかも。あと0°以下になった場合どうするかの修正もしたほうが
    ui->rotateAngleSpin->setValue(int(new_angleDeg) % 360);    // 360～360°になるように

    // 回転させる　左右、上下の向き(回転させる軸の向きを指定する)
    //(spinX,Y,Z)=(1, 0, 0) など　Xに値が入っている時、X軸中心に回転する
    //(spinX,Y,Z)=(0, 1, 0) など　Yに値が入っている時、Z軸中心に回転する (openGL座標なのでspinY=通常座標Z)
    //(spinX,Y,Z)=(0, 0, 1) など　Zに値が入っている時、Y軸中心に回転する (openGL座標なのでspinZ=通常座標Y)
    float new_rotateX = 0;
    float new_rotateY = 0;
    float new_rotateZ = 0;

    // NG//ある程度OKだが。途中からXspin,Yspinの大小差が大きくなりすぎると左右or上下回転できなくなる
    // mousePressでドラッグ開始時点にリセットの必要あり float DEBUG_newX = old_rotateX + in_mouseMoveY; float DEBUG_newY
    // = old_rotateY + in_mouseMoveX; ui->rotateXSpin->setValue(old_rotateX + in_mouseMoveY); //上下回転
    // ui->rotateYSpin->setValue(old_rotateY + in_mouseMoveX); //左右回転
    // OK//-end-

    // 前回までの操作での角度を保つ + Xspin/Yspinの大小差がある場合の対処
    // これまでの操作で　Xspin、Yspin
    // 極端に値が違う場合、マウス操作回転で左右or上下方向に動かないので、比率は保ちつつ、値を同じ程度にそろえる
    float oldX = ui->rotateXSpin->value();
    float oldY = ui->rotateYSpin->value();
    float newX = oldX;
    float newY = oldY;
    int   pmX  = 1;    // もとの値がプラス or マイナス どちらか取得 (1 or -1, 0の場合は1)
    if (newX < 0) {
        pmX = -1;
    }
    int pmY = 1;
    if (newY < 0) {
        pmY = -1;
    }

    if (abs(newX) > abs(newY)) {
        // newX/newX == 100 として割合にする
        // (基準=1にするとマウス操作値のほうが大きすぎて動作NG。マウス操作値より大きくするため 基準10にする)
        if (oldX == 0) {
            newX = 0;
        }
        else {
            newX = pmX * 100;
        }
        // newY = newY/newX
        if (abs(oldY / oldX) < 0.01) {    // GUI最小値0.01これ以下だと0に丸め込まれてしまう
            newY = pmY * 0.01;
        }
        else {
            newY = pmY * abs(oldY) / abs(oldX) * 100;
        }
    }
    else if (abs(newX) < abs(newY)) {
        // newX/newX == 1 として割合にする
        if (oldY == 0) {
            newY = 0;
        }
        else {
            newY = pmY * 100;
        }
        // newY = newY/newX
        if (abs(oldX / oldY) < 0.01) {    // GUI最小値0.01これ以下だと0に丸め込まれてしまう
            newX = pmX * 0.01;
        }
        else {
            newX = pmX * abs(oldX) / abs(oldY) * 100;
        }
    }

    newX = newX + in_mouseMoveY;    // X軸基準に,上下回転なので、マウス上下方向の値を足す
    newY = newY + in_mouseMoveX;    // 左右回転

    ui->rotateXSpin->setValue(newX);    // 上下回転
    ui->rotateYSpin->setValue(newY);    // 左右回転 通常座標系Z軸を軸として回転
    // ui->rotateZSpin->setValue(0);  //Z=通常座標系Y軸 回転動作で利用してないので何もしない
}

//-start- 旧案1: マウスで物体平行移動する translationタブ [translation]項目　←動作するが、違和感ありNG
// void Vox3DForm::func_slot_mouseVoxShift(int in_mouseMoveX, int in_mouseMoveY){
//    //旧案1 lookAtタブ　[Eye]項目で現在の視点角度を取得、 translationタブ
//    [translation]項目設定して平行移動する　←動作するが、違和感ありNG
//    //マウスドラッグで図形平行移動モードONの場合
//    マウスドラッグで平行移動。　0:平行移動モードはOFF（OFFのときは、マウスドラッグで図形回転モード)
//    //試し中　視点位置E eye と　注視点を変更 （もとの角度のまま両方　平行移動させる）

//    //　glXZ平面 = 通常 XY平面  glZ手前マイナス、奥がプラス
//    //                          | glZ(-)
//    //                          |
//    //      2象限(定位置Left)     |    1象限(定位置back)
//    //                          |
//    // glX(-)                   |                       glX(+)
//    // ----------------------------------------------------------
//    //                          |
//    //                          |
//    //      3象限(定位置Front)    |    4象限(定位置Right)
//    //                          |
//    //                          | glZ(+)

//    //double mouseMoveH = double(in_mouseMoveX);
//    double mouseMoveH = (double(in_mouseMoveX) / ui->obj3dViewer->width()) * 3 ; //width()=表示範囲全体
//    //*3は移動幅調整。値は適当。 double anglerad = atan(abs(ui->lookEyeXSpin->value()) /
//    abs(ui->lookEyeZSpin->value())); double transGLX = mouseMoveH * cos(abs(anglerad)); double transGLZ = mouseMoveH *
//    cos(abs(M_PI/2 - abs(anglerad)));

//    //注視線に直角の線上を進む。進む方向に対応するように、マイナスを掛ける。
//    //マウス移動H方向(左右) 視点位置が glXZ平面　第3象限の場合 (定位置Front) 動作Ok-45°以外も
//    //if(ui->lookEyeXSpin->value() < 0 && ui->lookEyeZSpin->value() > 0){
//        //注視線に直角の線の進行方向は、第4象限 glX方向+ , glY方向＋　なので処理なし
//    //}

//    //H方向 左右 視点位置がXY平面　第1象限の場合 (定位置Back） 動作Ok-45°以外も
//    if(ui->lookEyeXSpin->value() > 0 && ui->lookEyeZSpin->value() < 0){
//        //注視線に直角の線の進行方向は、第2象限 glX方向- , glY方向-
//        transGLX = -1 * transGLX ;
//        transGLZ = -1 * transGLZ ;
//    }

//    //H方向 左右 視点位置がXY平面　第2象限の場合 動作Ok-45°以外も (定位置Left)
//    if(ui->lookEyeXSpin->value() < 0 && ui->lookEyeZSpin->value() < 0){
//        transGLX = -1 * transGLX ;
//    }

//    //H方向 左右 視点位置がXY平面　第4象限の場合 動作test (定位置Right)
//    if(ui->lookEyeXSpin->value() > 0 && ui->lookEyeZSpin->value() > 0){
//        //注視線に直角の線の進行方向は、第1象限 glX方向+ , glY方向-
//        transGLZ = -1 * transGLZ ;
//    }

//    //qDebug() << "[DEBUG]Vox3DForm.cpp-func_slot_mouseVoxShsift moveMouse mouseMoveH=" <<
//    QString::number(mouseMoveH);
//    //qDebug() << "[DEBUG]Vox3DForm.cpp-func_slot_mouseVoxShift moveMouse anglerad=" << QString::number(anglerad) << "
//    angle-degree=" << QString::number(anglerad * 180/M_PI);
//    //qDebug() << "[DEBUG]Vox3DForm.cpp-func_slot_mouseVoxShift transGLX=" << QString::number(transGLX) << "
//    transGLZ=" << QString::number(transGLZ); ui->translateXSpin->setValue(ui->translateXSpin->value() + transGLX);
//    ui->translateZSpin->setValue(ui->translateZSpin->value() + transGLZ);

//    //V方向 上下　translate
//    double mouseMoveV = (double(in_mouseMoveY) / ui->obj3dViewer->height()) * -1 ; //heght()=表示範囲全体
//    //mouseMoveYは下移動するときマイナス値、上移動するときプラス値？
//    //double mouseMoveV = (double(in_mouseMoveY)) * -1;
//    ui->translateYSpin->setValue(ui->translateYSpin->value() + mouseMoveV);

//}
//-end- 旧案1: マウスで物体平行移動する translationタブ [translation]項目　←動作するが、違和感ありNG

//-start- マウスで物体平行移動する lookAtタブ[center]
void Vox3DForm::func_slot_mouseVoxShift(int in_mouseMoveX, int in_mouseMoveY)
{
    // 案 lookAtタブ　[Eye]項目で現在の視点角度を取得、 lookAtタブ
    // [center]項目設定して平行移動する　centerで画面中心に置く、物体座標を指定する

    float windowH = ui->obj3dViewer->width();    // 表示範囲全体。　変数に代入しないと、なぜか0として扱われてしまう。。
    float  windowV    = ui->obj3dViewer->height();    //
    double mouseMoveH = float(in_mouseMoveX);
    double mouseMoveV = float(in_mouseMoveY);

    // H方向 左右
    int   direction = 1;    // 移動方向 デフォルト+
    float eyeX      = ui->lookEyeXSpin->value();
    float eyeY      = ui->lookEyeZSpin->value();
    float centerX   = ui->lookCenterXSpin->value();
    float centerY   = ui->lookCenterZSpin->value();
    // float slope = eyeY/eyeX; //1次式の傾き

    double eyetan = -eyeY / eyeX;
    double maxtan = 89 * 180 / M_PI;    // tan(90°)は無限大値になってしまい判定に使えないので 89°で計算)
    double eye_angleRad =
        atan(eyetan);    // 単位：ラジアン -90°～90°の範囲になり、第1-2象限、第4-3象限で区別がつかない。
    double eye_angleDeg = eye_angleRad * 180 / M_PI;
    if (eyetan > maxtan) {
        eyetan = maxtan;
    }
    if (eyetan < -maxtan) {
        eyetan = -maxtan;
    }
    // 　glXZ平面 = 通常 XY平面  glZ手前マイナス、奥がプラス
    //                           | glZ(-)
    //                           |
    //       2象限(定位置Left)     |    1象限(定位置Back)
    //                           |
    //  glX(-)                   |                       glX(+)
    //  ----------------------------------------------------------
    //                           |
    //                           |
    //       3象限(定位置Front)    |    4象限(定位置Right)
    //                           |
    //                           | glZ(+)
    if (eyeX >= 0 && eyeY <= 0) {                    // 第1象限 (BACK)
        eye_angleDeg = eye_angleRad * 180 / M_PI;    // そのまま使う
        direction    = 1;
    }
    if (eyeX < 0 && eyeY <= 0) {    // 第2象限 (LEFT)
        eye_angleDeg = 180 - abs(eye_angleDeg);
        direction    = -1;
        // if(mouseMoveH > 0 ){ direction = -1; }
        // if(mouseMoveH <= 0 ){ direction = 1; }
    }
    if (eyeX < 0 && eyeY > 0) {    // 第3象限 (FRONT)
        eye_angleDeg = abs(eye_angleDeg) + 180;
        direction    = 1;
    }
    if (eyeX > 0 && eyeY > 0) {    // 第4象限 (RIGHT)
        eye_angleDeg = 360 - abs(eye_angleDeg);
        direction    = -1;
    }
    // int eye_angleDeg = eye_angleDeg + alpha;
    // while (eye_angleDeg < 0) {
    //     eye_angleDeg += 360;
    // }
    // while (eye_angleDeg >= 360) {
    //     eye_angleDeg -= 360;
    // }
    // eye_angleRad = eye_angleDeg  * M_PI/180;

    eye_angleRad      = eye_angleDeg * M_PI / 180;
    float add_centerX = mouseMoveH * cos(eye_angleRad);
    float add_centerY = mouseMoveH * sin(eye_angleRad);

    // float offset_move1 = 1;
    // if(mouseMoveH == 0){ offset_move1 = 0; }
    // if(mouseMoveV != 0){
    //     offset_move1 = mouseMoveH / mouseMoveV;
    // }
    float offset_move1 =
        3;    // 調整値。値は適当。　操作性改善のため。マウスの動きに対して、図形を動かす距離が小さすぎるので対策。

    ui->lookCenterXSpin->setValue(
        centerX
        + (add_centerX / windowH) * offset_move1
              * direction);    // windowH しないと移動量大きすぎになってしまう。 //offset_moveは調整値　適当。。　あとで
                               // ui->obj3dViewer->width()考慮して適正な値を決める予定。。
    ui->lookCenterZSpin->setValue(centerY + (add_centerY / windowH) * offset_move1 * direction);

    //-----
    // V方向 上下 //平行移動動作OK
    ui->lookCenterYSpin->setValue(ui->lookCenterYSpin->value() + (mouseMoveV / windowH));
    // ↑　Yspinは GL座標Yspin.
    // ↑　/windowH しないと移動量大きすぎになってしまう。　本来hight =
    // windowVとすべきだが、動作がしっくりこないのでwidthにしている。(円弧状に動く角度が急峻にならないようにwidthにしている)
}
//-end- マウスで物体平行移動する lookAtタブ[center]

//-start- 旧案1　簡易回転(水平方向): カメラ位置変更による動作 [LookAt]タブ-[eye]
// コーディングで動作確認のため、使うのみ。　本番用は　別関数。
// void Vox3DForm::on_comboBox_voxRotate_currentIndexChanged(const QString& arg1)
//{
//    func_comboBox_voxRotate_translateRotate();    // 簡易回転(水平方向): 物体自身の回転による動作
//                                                  // [translate]タブ-[rotate]
//
//    // func_comboBox_voxRotate_eyeChange(); //旧案1　簡易回転(水平方向): カメラ位置変更による動作 [LookAt]タブ-[eye]
//    //
//    // コーディングで動作確認のため、使うのみ。　ユーザー用は　別関数。
//}

//-start- 旧案1　簡易回転(水平方向): カメラ位置変更による動作 [LookAt]タブ-[eye]
// コーディングで動作確認のため、使うのみ。　ユーザー用は　別関数。
void Vox3DForm::func_comboBox_voxRotate_eyeChange()
{
    QString rotateStr = ui->comboBox_voxRotate->currentText();

    // 視点の初期化
    func_setDefaultViewMenu();

    // カメラ位置　通常座標X,Y 方向設定　(GUI GL座標 eyeX=通常座標X方向. eyeZ=通常座標(-)Y方向:手前+,奥-)
    QString selectname = ui->comboBox_voxRotate->currentText();
    QString tmpImg     = "cube_01.png";
    double  eyeX       = -5;
    double  eyeZ       = 5;    // GL座標 eyeZ = 通常座標(-)Y方向:手前+,奥-
    double  lightPosX  = -25;
    double  lightPosZ  = 25;
    QPixmap tmpPix;
    if (selectname == "front") {
        eyeX      = -5;
        eyeZ      = 5;
        lightPosX = -25;
        lightPosZ = 25;
        tmpImg    = "cube_01.png";
    }
    if (selectname == "left") {
        eyeX      = -5;
        eyeZ      = -5;
        lightPosX = -25;
        lightPosZ = -25;
        tmpImg    = "cube_02.png";
    }
    if (selectname == "back") {
        eyeX      = 5;
        eyeZ      = -5;
        lightPosX = 25;
        lightPosZ = -25;
        tmpImg    = "cube_03.png";
    }
    if (selectname == "right") {
        eyeX      = 5;
        eyeZ      = 5;
        lightPosX = 25;
        lightPosZ = 25;
        tmpImg    = "cube_04.png";
    }
    //[DEBUG]//QPixmap tmpPix(":/images/cube_01.png");
    tmpPix = QPixmap(":/images/" + tmpImg);
    // ui->label_voxRotateImg->setPixmap(tmpPix.scaled(100,100));
    ui->label_voxRotateImg->setPixmap(tmpPix.scaled(60, 60));

    // カメラ位置　通常座標Z 方向設定 GL座標 eyeY = 通常座標Z方向
    selectname       = ui->comboBox_voxRotateV->currentText();
    double eyeY      = 2;    // GL座標 eyeY = 通常座標Z方向
    double lightPosY = 25;
    if (selectname == "middle") {
        eyeY = 2;
    }
    if (selectname == "top") {
        eyeY = 5;
    }
    if (selectname == "bottom") {
        eyeY      = -5;
        lightPosY = -25;
    }

    ui->lookEyeXSpin->setValue(eyeX);
    ui->lookEyeYSpin->setValue(eyeY);
    ui->lookEyeZSpin->setValue(eyeZ);

    ui->lightPosXSpin->setValue(lightPosX);
    ui->lightPosYSpin->setValue(lightPosY);
    ui->lightPosZSpin->setValue(lightPosZ);
}
//-end- 旧案1　簡易回転: カメラ位置変更による動作

void Vox3DForm::func_comboBox_voxRotate_translateRotate()
{
    // 簡易回転(水平方向): 物体自身の回転による動作 [translate]タブ-[rotate]

    // 視点の初期化
    ui->lookEyeXSpin->setValue(-5);    // カメラ位置(glX,glY,glZ)=(-5,2,5) 第3象限=225°(45°)
    ui->lookEyeYSpin->setValue(2);
    ui->lookEyeZSpin->setValue(5);
    ui->lookCenterXSpin->setValue(0);    // カメラ注視点(0,0,0)
    ui->lookCenterYSpin->setValue(0);
    ui->lookCenterZSpin->setValue(0);
    ui->lookUpXSpin->setValue(0);    // カメラ向き角度(0, 0, 0)
    ui->lookUpYSpin->setValue(1);
    ui->lookUpZSpin->setValue(0);
    ui->lightPosXSpin->setValue(-25);    // 光源ライト位置　第3象限=225°(45°)　高さだけlookEyeと違う
    ui->lightPosYSpin->setValue(25);
    ui->lightPosZSpin->setValue(25);

    // ユーザー指定(コンボボックス)の位置にする。
    QString selectnameH = ui->comboBox_voxRotate->currentText();
    QString selectnameV = ui->comboBox_voxRotateV->currentText();
    QString tmpImg      = "cube_01.png";
    QPixmap tmpPix;

    //    /*
    //     //X軸設定後、Y軸とかはできない。。一気に設定するしかない
    //    //位置設定 水平方向
    //    g_flag_notRedraw = 1; //処理途中にスロットで再描画されてしまわないようにする
    //    ui->rotateXSpin->setValue(0);
    //    ui->rotateYSpin->setValue(1);
    //    ui->rotateZSpin->setValue(0);
    //    g_flag_notRedraw = 0;
    //    ui->rotateAngleSpin->setValue(0);
    //    if(selectnameH == "front"){
    //        ui->rotateAngleSpin->setValue(0); //eye設定そのまま　第3象限=225°(45°)
    //        tmpImg = "cube_01.png";
    //    }
    //    if(selectnameH == "left"){
    //        ui->rotateAngleSpin->setValue(90); //eye設定そのまま　第3象限=225°(45°)
    //        tmpImg = "cube_02.png";
    //    }
    //    if(selectnameH == "back"){
    //        ui->rotateAngleSpin->setValue(180);
    //        tmpImg = "cube_03.png";
    //    }
    //    if(selectnameH == "right"){
    //        ui->rotateAngleSpin->setValue(270);
    //        tmpImg = "cube_04.png";
    //    }

    //    //位置設定 垂直方向
    //    g_flag_notRedraw = 1; //処理途中にスロットで再描画されてしまわないようにする
    //    ui->rotateXSpin->setValue(1);
    //    ui->rotateYSpin->setValue(0);
    //    ui->rotateZSpin->setValue(0);
    //    g_flag_notRedraw = 0;
    //    ui->rotateAngleSpin->setValue(0);
    //    if(selectnameV == "middle"){

    //        ui->rotateAngleSpin->setValue(0);

    //         //ui->lightPosYSpin->setValue(-25); //照明位置
    //    }
    //    if(selectnameV == "top"){
    //        ui->rotateAngleSpin->setValue(30);
    //    }
    //    if(selectnameV == "bottom"){
    //        ui->rotateAngleSpin->setValue(-30);
    //    }

    //    //照明位置変更 bottomの場合のみ
    //    if(selectnameV == "bottom"){
    //        ui->lightPosYSpin->setValue(-25);
    //    }
    //*/

    ui->rotateAngleSpin->setValue(0);
    float angle = 0;
    float spinX = 0;
    float spinY = 0;
    float spinZ = 0;
    if (selectnameH == "front") {    // OK
        tmpImg = "cube_01.png";
        if (selectnameV == "middle") {
            g_flag_notRedraw = 1;    // 処理途中にスロットで再描画されてしまわないようにする
            ui->rotateXSpin->setValue(0);
            ui->rotateYSpin->setValue(1);
            ui->rotateZSpin->setValue(0);
            g_flag_notRedraw = 0;
            ui->rotateAngleSpin->setValue(0);
        }
        if (selectnameV == "top") {    // OK
            g_flag_notRedraw = 1;      // 処理途中にスロットで再描画されてしまわないようにする
            ui->rotateXSpin->setValue(1);
            ui->rotateYSpin->setValue(0);
            ui->rotateZSpin->setValue(1);
            g_flag_notRedraw = 0;
            ui->rotateAngleSpin->setValue(30);
        }
        if (selectnameV == "bottom") {
            ui->rotateXSpin->setValue(1);
            ui->rotateYSpin->setValue(0);
            ui->rotateZSpin->setValue(1);
            g_flag_notRedraw = 0;
            ui->rotateAngleSpin->setValue(-30);
        }
    }
    if (selectnameH == "left") {
        tmpImg = "cube_02.png";
        if (selectnameV == "middle") {
            g_flag_notRedraw = 1;    // 処理途中にスロットで再描画されてしまわないようにする
            ui->rotateXSpin->setValue(0);
            ui->rotateYSpin->setValue(1);
            ui->rotateZSpin->setValue(0);
            g_flag_notRedraw = 0;
            ui->rotateAngleSpin->setValue(90);
        }
        if (selectnameV == "top") {
            g_flag_notRedraw = 1;    // 処理途中にスロットで再描画されてしまわないようにする
            ui->rotateXSpin->setValue(0.1);    // top,bottom共通
            ui->rotateYSpin->setValue(-5);     // top,bottom共通
            ui->rotateZSpin->setValue(-1);
            g_flag_notRedraw = 0;
            ui->rotateAngleSpin->setValue(-90);    // 270 でもOK //top,bottom共通
        }
        if (selectnameV == "bottom") {
            g_flag_notRedraw = 1;    // 処理途中にスロットで再描画されてしまわないようにする
            ui->rotateXSpin->setValue(0.1);
            ui->rotateYSpin->setValue(-5);
            ui->rotateZSpin->setValue(2);
            g_flag_notRedraw = 0;
            ui->rotateAngleSpin->setValue(-90);    // 270 でもOK //top,bottom共通
        }
    }
    if (selectnameH == "back") {
        tmpImg = "cube_03.png";
        if (selectnameV == "middle") {
            g_flag_notRedraw = 1;    // 処理途中にスロットで再描画されてしまわないようにする
            ui->rotateXSpin->setValue(0);
            ui->rotateYSpin->setValue(1);
            ui->rotateZSpin->setValue(0);
            g_flag_notRedraw = 0;
            ui->rotateAngleSpin->setValue(180);
        }
        if (selectnameV == "top") {
            g_flag_notRedraw = 1;    // 処理途中にスロットで再描画されてしまわないようにする
            ui->rotateXSpin->setValue(-1);    // top,bottom共通
            ui->rotateYSpin->setValue(10);
            ui->rotateZSpin->setValue(1);    // top,bottom共通
            g_flag_notRedraw = 0;
            ui->rotateAngleSpin->setValue(180);    // top,bottom共通
        }
        if (selectnameV == "bottom") {
            g_flag_notRedraw = 1;    // 処理途中にスロットで再描画されてしまわないようにする
            ui->rotateXSpin->setValue(-1);    // top,bottom共通
            ui->rotateYSpin->setValue(-5);
            ui->rotateZSpin->setValue(1);    // top,bottom共通
            g_flag_notRedraw = 0;
            ui->rotateAngleSpin->setValue(180);    // top,bottom共通
        }
    }
    if (selectnameH == "right") {    // OK
        tmpImg = "cube_04.png";
        if (selectnameV == "middle") {
            g_flag_notRedraw = 1;    // 処理途中にスロットで再描画されてしまわないようにする
            ui->rotateXSpin->setValue(0);
            ui->rotateYSpin->setValue(1);
            ui->rotateZSpin->setValue(0);
            g_flag_notRedraw = 0;
            ui->rotateAngleSpin->setValue(270);
        }
        if (selectnameV == "top") {
            g_flag_notRedraw = 1;    // 処理途中にスロットで再描画されてしまわないようにする
            ui->rotateXSpin->setValue(1);
            ui->rotateYSpin->setValue(-2);    // top,bottom共通
            ui->rotateZSpin->setValue(0);     // top,bottom共通
            g_flag_notRedraw = 0;
            ui->rotateAngleSpin->setValue(-270);    // top,bottom共通
        }
        if (selectnameV == "bottom") {
            g_flag_notRedraw = 1;    // 処理途中にスロットで再描画されてしまわないようにする
            ui->rotateXSpin->setValue(-1);
            ui->rotateYSpin->setValue(-2);    // top,bottom共通
            ui->rotateZSpin->setValue(0);     // top,bottom共通
            g_flag_notRedraw = 0;
            ui->rotateAngleSpin->setValue(-270);    // top,bottom共通
        }
    }
}

// void Vox3DForm::on_comboBox_voxRotateV_currentIndexChanged(const QString& arg1)
//{
//     on_comboBox_voxRotate_currentIndexChanged(0);    // 引数は影響しないはず
// }

void Vox3DForm::on_comboBox_voxLightPos_currentTextChanged(const QString& arg1)
{
    // カメラ位置　通常座標X,Y 方向設定　(GUI GL座標 eyeX=通常座標X方向. eyeZ=通常座標(-)Y方向:手前+,奥-)
    QString selectname = ui->comboBox_voxLightPos->currentText();
    QString tmpImg     = "cube_01.png";
    double  lightPosX  = -25;
    double  lightPosZ  = 25;
    if (selectname == "front") {
        lightPosX = -25;
        lightPosZ = 25;
    }
    if (selectname == "left") {
        lightPosX = -25;
        lightPosZ = -25;
    }
    if (selectname == "back") {
        lightPosX = 25;
        lightPosZ = -25;
    }
    if (selectname == "right") {
        lightPosX = 25;
        lightPosZ = 25;
    }
    // カメラ位置　通常座標Z 方向設定 GL座標 eyeY = 通常座標Z方向
    selectname = ui->comboBox_voxRotateV->currentText();

    ui->lightPosXSpin->setValue(lightPosX);
    // ui->lightPosYSpin->setValue(lightPosY);
    ui->lightPosZSpin->setValue(lightPosZ);
}

void Vox3DForm::func_slot_3D_acceptVoxpath(QString inStr)
{
    // qDebug() << "[DEBUG]Vox3DSub02.cpp-func_slot_3D_acceptVoxpath";
    ui->lineEdit_voxpath->setText(inStr);
    on_pushButton_voxpathLoad_clicked();
}

void Vox3DForm::on_vox_comboBox_mouseAct_currentTextChanged(const QString& arg1)
{
#ifdef ORIGINAL_OBJ3_VIEW    /// TODO: ビュー置き換え中によりコメントアウト
    if (arg1 == "Rotate") {
        ui->obj3dViewer->g_flag_mouseRotate = 1;
        ui->obj3dViewer->g_ui_voxshiftflag  = 0;
    }

    if (arg1 == "Shift") {
        ui->obj3dViewer->g_flag_mouseRotate = 0;
        ui->obj3dViewer->g_ui_voxshiftflag  = 1;
    }
#endif
}

void Vox3DForm::func_tableMaterialDefaultSet_matnameToColorCombo()
{    // materialテーブルのセット。特定のマテリアルの場合、色設定コンボボックスを特定の色にセットする。
    QStringList matnameList, colorList;

    // ユーザーごとの.fdtdファイルが読み込める場合は、内容から色設定。
    QString fdtdpath = ui->vox_fdtd_path_lineEdit->text();
    int     openflag = 1;
    QFile   InFile01(fdtdpath);
    if (!InFile01.exists() || !InFile01.open(QIODevice::ReadOnly)) {
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
    //"[DEBUG]Vox3DFormSubH.cpp-func_tableMaterialDefaultSet_matnameToColorCombo key=" << key << " value=" <<
    // g_hash_matnameToCname.value(key);

    for (int row = 0; row < ui->tableWidget_material->rowCount(); row++) {
        QString           matname  = ui->tableWidget_material->item(row, 2)->text();
        CustomColorCombo* tmpcombo = static_cast<CustomColorCombo*>(ui->tableWidget_material->cellWidget(row, 3));

        if (matnameToColorfdtdHash.contains(matname)) {
            // officialマテリアルとして、色定義されている場合　コンボボックスの色に反映する。
            tmpcombo->setCurrentText(matnameToColorfdtdHash.value(matname));
        }
        else if (g_hash_matnameToCname.contains(matname)) {
            // officialマテリアルとして、色定義されている場合　コンボボックスの色に反映する。
            tmpcombo->setCurrentText(g_hash_matnameToCname.value(matname));
        }
        else {
            int tmpIndex = row % 10 + 1;    /// 先頭に空白（任意色）を入れたので必ず＋１する
            tmpcombo->setCurrentIndex(tmpIndex);
        }
        QString cname   = tmpcombo->currentText();            // 色名 Greenなど
        QString bgcolor = g_hash_cnameToStyleColor[cname];    // bgcolor="#008000" など
        /*
        QString fontcolor = "#000000";                          // 通常は文字色=黒
        if (g_list_whitefontBgColor.indexOf(cname) > -1) {
            fontcolor = "#ffffff";    // 見やすさのため、背景色が濃い色の場合は文字色=白
        }
        // コンボボックス部品の背景色・文字色設定
        //[DEBUG]tmpcombo->setStyleSheet("QComboBox { background-color: #008000 ; color: #ff0000 ; }");
        ////例：背景色　緑　文字色：赤
        QString strStyle = QString("QComboBox { background-color: %1 ; color: %2 ; }").arg(bgcolor, fontcolor);
        tmpcombo->combobox()->setStyleSheet(strStyle);
        */
        tmpcombo->setColor(cname, bgcolor);
    }
}

void Vox3DForm::on_vox_fdtd_load_pushButton_clicked()
{
    //.fdtdファイル読み込み、入力欄にセットのみの処理。　.fdtd情報の反映はユーザーにRedrawボタン押下してもらう。
    // ファイルユーザー選択前処理　既存入力があれば、そのディレクトリを起点にする
    QString tmpDirPath = "c:/";
    if (ui->vox_fdtd_path_lineEdit->text().isEmpty() == 0) {
        QString tmpdir = QFileInfo(QFile(ui->vox_fdtd_path_lineEdit->text())).absolutePath();
        if (QDir(tmpdir).exists()) {
            tmpDirPath = ui->vox_fdtd_path_lineEdit->text();
        }
    }
    // ファイルユーザー選択
    QString filePath = "";
    // filePath = QFileDialog::getOpenFileName(this, tr("Select file"), "Desktop", "*");
    filePath = QFileDialog::getOpenFileName(this, tr("Select file"), tmpDirPath, tr("fdtd Fileds (*.fdtd)"));
    if (filePath.isEmpty()) {
        return;
    }
    vox_fdtd_load(filePath);
}

void Vox3DForm::vox_fdtd_load(const QString& path)
{
    ui->vox_fdtd_path_lineEdit->setText(path);

    // materialテーブルのセット
    func_tableMaterialDefaultSet_matnameToColorCombo();
}

void Vox3DForm::on_vox_fdtd_apply_pushButton_clicked()
{
    if (ui->tableWidget_material->rowCount() == 0) {
        return;
    }

    /// ユーザーごとの.fdtdファイルが読み込める場合
    {
        QString fdtdpath = ui->vox_fdtd_path_lineEdit->text();
        if (fdtdpath.isEmpty()) {
            if (QMessageBox::question(this, windowTitle(), "fdtd fileが設定されていません。デフォルト色に戻しますか？")
                != QMessageBox::Yes) {
                return;
            }
        }
        else {
            QFile InFile01(fdtdpath);
            if (!InFile01.open(QIODevice::ReadOnly)) {
                if (QMessageBox::question(this, windowTitle(), "fdtd fileが読めません。デフォルト色に戻しますか？")
                    != QMessageBox::Yes) {
                    return;
                }
            }
        }
    }

    /// materialテーブルのセット
    func_tableMaterialDefaultSet_matnameToColorCombo();
}

void Vox3DForm::on_vox_fdtd_clear_pushButton_clicked()
{
    ui->vox_fdtd_path_lineEdit->clear();
    /// materialテーブルのセット
    func_tableMaterialDefaultSet_matnameToColorCombo();
}

void Vox3DForm::input_getPointOfMesh_by2point_jiku(
    QStringList& in_voxSurfaceStrList, QVector<QVector3D>& in_voxXYZStartVec, QVector<QVector3D>& in_voxXYZEndVec,
    QVector<int>& in_voxMatnumVec, QVector<QVector3D>& in_voxColorVec,
    QVector<float>& in_voxAcolorVec)    // X,Y,Z軸を図形に付け足して表示する
                                        // 長方形面の始点・終点座標から、直方体1個を表示するための値設定
{
    // 引数は値返却に使うためのポインタ　実値をこの関数内で参照するものではない

    // 追加なので　in_voxXYZStartVec.clear()　はしない
    // in_voxXYZEndVec.clear();
    // in_voxColorVec.clear();
    // in_voxAcolorVec.clear();
    // in_voxSurfaceStrList.clear();
    // in_voxMatnumVec.clear();

    float xlen = nx * 2;
    float ylen = ny;
    float zlen = nz;

    for (int i = 0; i < 3; i++) {
#ifdef ORIGINAL_OBJ3_VIEW    /// TODO: ビュー置き換え中によりコメントアウト
        int matnum = 1;      // 後で直す
#else
        int matnum = -999999;    // 一旦別物扱いでTriaで扱うが、左下座標系表示したので不要か？
#endif
        QVector3D colorVec = QVector3D(1, 0, 0);    // 赤
        float     acolor   = 1;                     // 通常不透明=1 半透明=0.3

        if (i == 0) {    // X軸
            xlen     = nx * 2;
            ylen     = 1;
            zlen     = 1;
            colorVec = QVector3D(1, 0, 0);    // 赤
        }
        if (i == 1) {    // Y軸
            zlen     = 1;
            ylen     = ny * 2;
            xlen     = 1;
            colorVec = QVector3D(0, 1, 0);    // 緑
        }
        if (i == 2) {    // Z軸
            xlen     = 1;
            ylen     = 1;
            zlen     = nz * 2;
            colorVec = QVector3D(0, 0, 1);    // 青
        }

        in_voxSurfaceStrList.append("Front");
        in_voxXYZStartVec.append(QVector3D(0, 0, 0));
        in_voxXYZEndVec.append(QVector3D(xlen, 0, zlen));
        in_voxMatnumVec.append(matnum);
        in_voxColorVec.append(colorVec);
        in_voxAcolorVec.append(acolor);

        in_voxSurfaceStrList.append("Back");
        in_voxXYZStartVec.append(QVector3D(0, ylen, 0));
        in_voxXYZEndVec.append(QVector3D(xlen, ylen, zlen));
        in_voxMatnumVec.append(matnum);
        in_voxColorVec.append(colorVec);
        in_voxAcolorVec.append(acolor);

        in_voxSurfaceStrList.append("Bottom");
        in_voxXYZStartVec.append(QVector3D(0, 0, 0));
        in_voxXYZEndVec.append(QVector3D(xlen, ylen, 0));
        in_voxMatnumVec.append(matnum);
        in_voxColorVec.append(colorVec);
        in_voxAcolorVec.append(acolor);

        in_voxSurfaceStrList.append("Top");
        in_voxXYZStartVec.append(QVector3D(0, 0, zlen));
        in_voxXYZEndVec.append(QVector3D(xlen, ylen, zlen));
        in_voxMatnumVec.append(matnum);
        in_voxColorVec.append(colorVec);
        in_voxAcolorVec.append(acolor);

        in_voxSurfaceStrList.append("Left");
        in_voxXYZStartVec.append(QVector3D(0, 0, 0));
        in_voxXYZEndVec.append(QVector3D(0, ylen, zlen));
        in_voxMatnumVec.append(matnum);
        in_voxColorVec.append(colorVec);
        in_voxAcolorVec.append(acolor);

        in_voxSurfaceStrList.append("Right");
        in_voxXYZStartVec.append(QVector3D(xlen, 0, 0));
        in_voxXYZEndVec.append(QVector3D(xlen, ylen, zlen));
        in_voxMatnumVec.append(matnum);
        in_voxColorVec.append(colorVec);
        in_voxAcolorVec.append(acolor);
    }
}

void Vox3DForm::input_getPointOfMesh_by2point_rect(
    QVector3D in_startVec, QVector3D in_endVec, QVector3D in_colorVec, QStringList& in_voxSurfaceStrList,
    QVector<QVector3D>& in_voxXYZStartVec, QVector<QVector3D>& in_voxXYZEndVec, QVector<int>& in_voxMatnumVec,
    QVector<QVector3D>& in_voxColorVec,
    QVector<float>&     in_voxAcolorVec)    // X,Y,Z軸を図形に付け足して表示する
                                        // 長方形面の始点・終点座標から、直方体1個を表示するための値設定
{
    // 3D直方体の左前下(start),右後上(end)の２点を引数として受け取り、直方体の6面の面座標を追加する

    // &in_voxSurfaceStrList 以後の引数は値返却に使うためのポインタ　実値をこの関数内で参照するものではない

    // 引数の例
    //  in_startVec = (x1, y1, z1)
    //  in_endVec   = (x2, y2, z2)
    QVector3D colorVec = QVector3D(1, 0, 0);    // 赤
    // colorVec = QVector3D(1, 0, 1); //マゼンタ
    // colorVec = QVector3D(0, 1, 0); //緑
    // colorVec = QVector3D(1, 1, 0); //黄色
    // colorVec = QVector3D(0, 0, 1); //青
    // colorVec = QVector3D(1, 0, 1); //紫

    int in_x1 = int(in_startVec.x());
    int in_y1 = int(in_startVec.y());
    int in_z1 = int(in_startVec.z());
    int in_x2 = int(in_endVec.x());
    int in_y2 = int(in_endVec.y());
    int in_z2 = int(in_endVec.z());

    int x1 = in_x1;
    int y1 = in_y1;
    int z1 = in_z1;
    int x2 = in_x2;
    int y2 = in_y2;
    int z2 = in_z2;

    QStringList surfaceList;
    surfaceList << "Front"
                << "Front"
                << "Back"
                << "Right"
                << "Left"
                << "Bottom"
                << "Top";
    for (int i = 0; i < 6; i++) {
        in_voxSurfaceStrList.append(surfaceList.at(i));

        if (i == 0) {    // Front
            x1       = in_x1;
            y1       = in_y1;
            z1       = in_z1;
            x2       = in_x2;
            y2       = in_y1;
            z2       = in_z2;
            colorVec = QVector3D(1, 0, 0);    // 赤
        }
        if (i == 1) {    // Back
            x1       = in_x1;
            y1       = in_y2;
            z1       = in_z1;
            x2       = in_x2;
            y2       = in_y2;
            z2       = in_z2;
            colorVec = QVector3D(1, 0, 1);    // マゼンタ
        }
        if (i == 2) {    // Left
            x1       = in_x1;
            y1       = in_y1;
            z1       = in_z1;
            x2       = in_x1;
            y2       = in_y2;
            z2       = in_z2;
            colorVec = QVector3D(0, 1, 0);    // 緑
        }
        if (i == 3) {    // Right
            x1       = in_x2;
            y1       = in_y1;
            z1       = in_z1;
            x2       = in_x2;
            y2       = in_y2;
            z2       = in_z2;
            colorVec = QVector3D(1, 1, 0);    // 黄色
        }
        if (i == 4) {    // Bottom
            x1       = in_x1;
            y1       = in_y1;
            z1       = in_z1;
            x2       = in_x2;
            y2       = in_y2;
            z2       = in_z1;
            colorVec = QVector3D(0, 0, 1);    // 青
        }
        if (i == 5) {    // Top
            x1       = in_x1;
            y1       = in_y1;
            z1       = in_z2;
            x2       = in_x2;
            y2       = in_y2;
            z2       = in_z2;
            colorVec = QVector3D(1, 0, 1);    // 紫
        }

        int matnum = 9999;    // 適当
        int acolor = 1;       // 透明なし
        in_voxXYZStartVec.append(QVector3D(x1, y1, z1));
        in_voxXYZEndVec.append(QVector3D(x2, y2, z2));
        in_voxMatnumVec.append(matnum);
        in_voxColorVec.append(colorVec);
        in_voxAcolorVec.append(acolor);
    }
}

void Vox3DForm::selectMaterialList(int material_id)
{
    for (int row = 0; row < ui->tableWidget_material->rowCount(); row++) {
        int matnum = ui->tableWidget_material->item(row, 1)->text().toInt();
        if (material_id == matnum) {
            auto save_selection_mode = ui->tableWidget_material->selectionMode();
            ui->tableWidget_material->setSelectionMode(QAbstractItemView::SingleSelection);
            ui->tableWidget_material->selectRow(row);
            ui->tableWidget_material->setSelectionMode(save_selection_mode);
            return;
        }
    }
}

void Vox3DForm::setVoxFile(const QString& path)
{
    voxpathLoad(path);
}

void Vox3DForm::setFdtdFile(const QString& path)
{
    ui->vox_fdtd_path_lineEdit->setText(path);

    // materialテーブルのセット
    func_tableMaterialDefaultSet_matnameToColorCombo();
}

void Vox3DForm::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu contextMenu(this);

    QMenu sub_menu_tab("タブ表示", this);
    contextMenu.addMenu(&sub_menu_tab);
    for (auto dock_widget : m_dock_widgets) {
        auto action = new QAction(dock_widget->windowTitle(), this);
        sub_menu_tab.addAction(action);
        connect(action, &QAction::triggered, this, [this, dock_widget]() { dock_widget->show(); });
    }
    sub_menu_tab.addSeparator();
    auto action_tab_all_on = new QAction("All On", this);
    sub_menu_tab.addAction(action_tab_all_on);
    connect(action_tab_all_on, &QAction::triggered, this, [this]() {
        for (auto& dock : m_dock_widgets) {
            dock->show();
        }
    });
    auto action_tab_all_off = new QAction("All Off", this);
    sub_menu_tab.addAction(action_tab_all_off);
    connect(action_tab_all_off, &QAction::triggered, this, [this]() {
        for (auto& dock : m_dock_widgets) {
            dock->hide();
        }
    });

    contextMenu.addSeparator();

    QMenu sub_menu_layout("起動時の画面レイアウト", this);
    contextMenu.addMenu(&sub_menu_layout);
    auto action_layout_save = new QAction("保存", this);
    sub_menu_layout.addAction(action_layout_save);
    auto action_layout_load = new QAction("読込", this);
    sub_menu_layout.addAction(action_layout_load);
    sub_menu_layout.addSeparator();
    auto action_layout_init = new QAction("保存したレイアウトを削除(初期化)", this);
    sub_menu_layout.addAction(action_layout_init);

    connect(action_layout_save, &QAction::triggered, this, [this]() {
        if (m_setting_gui_ctrl) {
            m_setting_gui_ctrl->saveMainWindowState();
        }
    });
    connect(action_layout_load, &QAction::triggered, this, [this]() {
        if (m_setting_gui_ctrl) {
            m_setting_gui_ctrl->readMainWindowState();
        }
    });
    connect(action_layout_init, &QAction::triggered, this, [this]() {
        if (m_setting_gui_ctrl) {
            m_setting_gui_ctrl->resetMainWindowState();
            /*
            if (QMessageBox::question(
                    this, windowTitle(),
                    "起動時の画面レイアウト設定を初期化しました。\n現在の画面レイアウトも初期化しますか？")
                != QMessageBox::Yes) {
                return;
            }
            m_setting_gui_ctrl->readMainWindowState();
            */
        }
    });

    QPoint globalMousePos = QCursor::pos();
    contextMenu.exec(event->globalPos());
}

QString Vox3DForm::colorStyle(QColor color)
{
    return QString("QPushButton {"
                   "   border: 1px solid gray;"    // ボタンの枠線
                   "   border-radius: 1px;"        // ボタンの角を丸くする（半径10px）
                   "   padding: 1px;"              // ボタン内の余白
                   "   background-color: %1;"      // ボタンの背景色
                   "   color: black;"              // ボタンのテキスト色
                   "}")
        .arg(color.name(QColor::HexArgb));
}

void Vox3DForm::setResultStyleDefaultCombobox()
{
    m_result_ctrl->setResultStyleDefaultCombobox();
}

bool Vox3DForm::isModalVisible(QWidget* widget)
{
    if (!widget) {
        return false;
    }
    if (widget->isVisible() && widget->isModal()) {
        return true;
    }
    return false;
}

void Vox3DForm::on_pushButton_about_3dviewer_clicked()
{
    QString txt;

    txt +=
        QString("Version %1.%2.%3\n").arg(PROJECT_VERSION_MAJOR).arg(PROJECT_VERSION_MINOR).arg(PROJECT_VERSION_PATCH);
    txt += QString("Build Date %1\n").arg(BUILD_DATE);

    txt.replace(QString("\n"), QString("<br>"));
    txt += "<br>";
    txt += tr("This program uses...<br>");
    txt += tr("- Qt toolkit library."
              "<br>&nbsp;&nbsp;<a href='https://www.qt.io/developers/'>https://www.qt.io/developers</a><br>");
    txt += tr("- earcut.<br>&nbsp;&nbsp;<a "
              "href='https://github.com/mapbox/earcut.hpp'>https://github.com/mapbox/earcut.hpp</a><br>");
    txt += tr("- tinycolormap.<br>&nbsp;&nbsp;<a "
              "href='https://github.com/yuki-koyama/tinycolormap'>https://github.com/yuki-koyama/tinycolormap</a><br>");
    txt += tr("- lz4.<br>&nbsp;&nbsp;<a "
              "href='https://lz4.org'>https://lz4.org</a><br>");
    txt += tr("For details, see the LICENSE folder in the application folder.");

    QMessageBox msgBox((QMainWindow*)parentWidget());
    msgBox.setWindowTitle(QApplication::applicationName());
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    msgBox.setFont(font);
    msgBox.setText(txt);
    msgBox.exec();
}

void Vox3DForm::voxMaterialContextMenu(const QPoint& pos)
{
    // 右クリックメニューを作成
    QMenu contextMenu;

    // メニュー項目を追加
    QAction* action_show = contextMenu.addAction("表示");
    QAction* action_hide = contextMenu.addAction("非表示");
    contextMenu.addSeparator();
    QAction* action_color = contextMenu.addAction("色変更");
    QMenu    sub_trans("半透明");
    QAction* action_trans_on  = sub_trans.addAction("ON");
    QAction* action_trans_off = sub_trans.addAction("OFF");
    contextMenu.addMenu(&sub_trans);
    QMenu    sub_draw("Draw Mode");
    QAction* action_draw_s  = sub_draw.addAction("Shading");
    QAction* action_draw_w  = sub_draw.addAction("Wireframe");
    QAction* action_draw_sw = sub_draw.addAction("Shading+Wireframe");
    QAction* action_draw_n  = sub_draw.addAction("None");
    contextMenu.addMenu(&sub_draw);
    contextMenu.addSeparator();
    QMenu    sub_opt("Opt投影");
    QAction* action_opt_on  = sub_opt.addAction("ON");
    QAction* action_opt_off = sub_opt.addAction("OFF");
    contextMenu.addMenu(&sub_opt);

    // メニューを表示して選択されたアクションを取得
    QAction* selectedAction = contextMenu.exec(ui->tableWidget_material->viewport()->mapToGlobal(pos));

    if (selectedAction == action_show) {
        voxMaterialShow(true);
    }
    else if (selectedAction == action_hide) {
        voxMaterialShow(false);
    }
    else if (selectedAction == action_color) {
        voxMaterialColor();
    }
    else if (selectedAction == action_trans_on) {
        voxMaterialTrans(true);
    }
    else if (selectedAction == action_trans_off) {
        voxMaterialTrans(false);
    }
    else if (selectedAction == action_draw_s) {
        voxMaterialDrawMode(0, false);
    }
    else if (selectedAction == action_draw_w) {
        voxMaterialDrawMode(1, false);
    }
    else if (selectedAction == action_draw_sw) {
        voxMaterialDrawMode(2, false);
    }
    else if (selectedAction == action_draw_n) {
        voxMaterialDrawMode(3, false);
    }
    else if (selectedAction == action_opt_on) {
        voxMaterialOpt(true);
    }
    else if (selectedAction == action_opt_off) {
        voxMaterialOpt(false);
    }
}

void Vox3DForm::voxMaterialShow(bool show)
{
    bool change = false;

    QItemSelectionModel* selection_model = ui->tableWidget_material->selectionModel();
    for (int row = 0; row < ui->tableWidget_material->rowCount(); row++) {
        if (!selection_model->isRowSelected(row)) {
            continue;
        }

        QWidget*   pWidget  = ui->tableWidget_material->cellWidget(row, 0);
        QCheckBox* checkbox = pWidget->findChild<QCheckBox*>();
        if (show != checkbox->isChecked()) {
            g_flag_slotMatView = 0;
            checkbox->setChecked(show);
            change = true;
        }
    }

    if (change) {
        g_flag_slotMatView = 1;
        func_tableMateriaRedrawSlot(1);
    }
}

void Vox3DForm::voxMaterialColor()
{
    bool        init = true;
    QColor      init_color;
    int         init_index;
    QStringList labels;

    QList<CustomColorCombo*> target_button;

    QItemSelectionModel* selection_model = ui->tableWidget_material->selectionModel();
    for (int row = 0; row < ui->tableWidget_material->rowCount(); row++) {
        if (!selection_model->isRowSelected(row)) {
            continue;
        }
        auto tmpcombo = static_cast<CustomColorCombo*>(ui->tableWidget_material->cellWidget(row, 3));
        target_button.emplace_back(tmpcombo);

        if (init) {
            init_color = tmpcombo->qColor();
            init_index = tmpcombo->currentIndex();
            init       = false;
            labels     = tmpcombo->items();
        }
    }
    if (init) {
        return;
    }

    CustomComboDialog dlg;
    auto              combo_box = dlg.getCustomCombo();
    combo_box->removeItems();
    combo_box->addItems(labels);
    combo_box->setCurrentIndex(init_index);
    combo_box->setColor("Dummy", init_color);

    connect(combo_box->combobox(), &QComboBox::currentIndexChanged, this, [this, combo_box]() {
        QString cname   = combo_box->currentText();
        QString bgcolor = g_hash_cnameToStyleColor[cname];    // bgcolor="#008000" など
        combo_box->setColor(cname, bgcolor);
    });

    if (dlg.exec() == QDialog::Accepted) {
        g_flag_slotMatView = 0;

        for (auto target : target_button) {
            target->blockSignals(true);
            target->setCurrentIndex(combo_box->currentIndex());
            target->setColor("Dummy", combo_box->qColor());
            target->blockSignals(false);
        }

        g_flag_slotMatView = 1;
        func_tableMateriaRedrawSlot(1);
    }
}

void Vox3DForm::voxMaterialTrans(bool on)
{
    bool change = false;

    QItemSelectionModel* selection_model = ui->tableWidget_material->selectionModel();
    for (int row = 0; row < ui->tableWidget_material->rowCount(); row++) {
        if (!selection_model->isRowSelected(row)) {
            continue;
        }

        QWidget*   pWidget  = ui->tableWidget_material->cellWidget(row, 4);
        QCheckBox* checkbox = pWidget->findChild<QCheckBox*>();
        if (on != checkbox->isChecked()) {
            g_flag_slotMatView = 0;
            checkbox->setChecked(on);
            change = true;
        }
    }

    if (change) {
        g_flag_slotMatView = 1;
        func_tableMateriaRedrawSlot(1);
    }
}

void Vox3DForm::voxMaterialDrawMode(int mode, bool all)
{
    bool change = false;

    QItemSelectionModel* selection_model = ui->tableWidget_material->selectionModel();
    for (int row = 0; row < ui->tableWidget_material->rowCount(); row++) {
        if (!all) {
            if (!selection_model->isRowSelected(row)) {
                continue;
            }
        }

        QComboBox* combobox = (QComboBox*)ui->tableWidget_material->cellWidget(row, 5);
        if (combobox->currentIndex() != mode) {
            g_flag_slotMatView = 0;
            combobox->setCurrentIndex(mode);
            change = true;
        }
    }

    if (change) {
        g_flag_slotMatView = 1;
        func_tableMateriaRedrawSlot(1);
    }
}

void Vox3DForm::voxMaterialOpt(bool on)
{
    bool change = false;

    QItemSelectionModel* selection_model = ui->tableWidget_material->selectionModel();
    for (int row = 0; row < ui->tableWidget_material->rowCount(); row++) {
        if (!selection_model->isRowSelected(row)) {
            continue;
        }

        QWidget*   pWidget  = ui->tableWidget_material->cellWidget(row, 6);
        QCheckBox* checkbox = pWidget->findChild<QCheckBox*>();
        if (on != checkbox->isChecked()) {
            g_flag_slotMatView = 0;
            checkbox->setChecked(on);
            change = true;
        }
    }

    if (change) {
        g_flag_slotMatView = 1;
        func_tableMateriaRedrawSlot(1);
    }
}
