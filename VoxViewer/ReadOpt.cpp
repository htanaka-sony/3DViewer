#include "ReadOpt.h"

#include "ResultCtrl.h"
#include "SettingGuiCtrl.h"
#include "Vox3DForm.h"

#include "MyOpenGLWidget.h"
#include "OptPerformanceSetting.h"

#include "Scene/Voxel.h"
#include "Scene/VoxelScalar.h"

#include <QDebug>
#include <QFile>
#include <QLabel>
#include <QProgressDialog>
#include <QStringDecoder>
#include <QTextStream>

#include <algorithm>
#include <execution>

ReadOpt::ReadOpt(QObject* parent, QProgressDialog* pd) : QObject(parent), m_pd(pd) {}

bool ReadOpt::read(const std::wstring& file_path, MyOpenGLWidget* gl_view, Vox3DForm* form)
{
    INFO() << "Read Opt: " << file_path;

    if (m_pd) {
        /// 100%表記
        m_pd->setLabelText("Read Opt Data");
        m_pd->setRange(0, 100);
        m_pd->setValue(0);
        m_pd->setModal(true);
        connect(m_pd, &QProgressDialog::canceled, [&]() {
            if (m_pd) {
                m_cancel = true;
                m_pd->setLabelText("Canceling...");
                /// 以降キャンセルできないのでキャンセルボタンをグレーアウト
                QList<QPushButton*> buttons = m_pd->findChildren<QPushButton*>();
                for (auto btn : buttons) {
                    if (!btn->text().isEmpty()) {
                        btn->setEnabled(false);    // グレーアウト
                    }
                }
                /// キャンセルで消えてしまうので再表示
                m_pd->show();
                qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
            }
        });
    }

    if (!m_no_change_gui) {
        form->resultCtl()->setVoxel(nullptr);
        gl_view->removeOpt3DNode();
    }

    bool   memory_limit   = form->settingGuiCtrl()->readOptMemoryLimit();
    double memory_value   = form->settingGuiCtrl()->readOptMemorySize();
    int    memory_unit    = form->settingGuiCtrl()->readOptMemoryUnit();
    int    memoory_method = form->settingGuiCtrl()->readOptMemoryMethod();

    size_t memory_size = 0;
    if (memory_unit == 0) {    /// MB
        memory_size = (size_t)(memory_value * 1024.0 * 1024.0);
    }
    else {
        memory_size = (size_t)(memory_value * 1024.0 * 1024.0 * 1024.0);
    }

    /// ファイル読込み
    bool              read_file_end = false;
    ReadOptFileThread read_file(file_path, gl_view, memory_limit, memory_size, memoory_method);
    connect(&read_file, &ReadOptFileThread::setProgMessage, this, &ReadOpt::setProgMessage, Qt::QueuedConnection);
    connect(&read_file, &ReadOptFileThread::finished, this, [&read_file_end] { read_file_end = true; });
    read_file.start();

    while (1) {
        qApp->processEvents(Vox3DForm::isModalVisible(m_pd) ? QEventLoop::AllEvents
                                                            : QEventLoop::ExcludeUserInputEvents);
        if (read_file_end) {
            break;
        }
        if (m_cancel) {
            read_file.setCancel();
        }
    }

    if (m_cancel) {
        return false;
    }
    auto new_node = read_file.createNode();
    if (!new_node) {
        return false;
    }

    if (m_pd) {
        /// 以降キャンセルできないのでキャンセルボタンをグレーアウト
        QList<QPushButton*> buttons = m_pd->findChildren<QPushButton*>();
        for (auto btn : buttons) {
            if (!btn->text().isEmpty()) {
                btn->setEnabled(false);    // グレーアウト
                qApp->processEvents(Vox3DForm::isModalVisible(m_pd) ? QEventLoop::AllEvents
                                                                    : QEventLoop::ExcludeUserInputEvents);
            }
        }
    }

    auto voxel_scalar = new_node->object<VoxelScalar>();

    /// ファイル名保持
    QString file_name = QFileInfo(QString::fromStdWString(file_path)).completeBaseName();
    new_node->setUserAttributeString(L"FileName", file_name.toStdWString());

    if (!m_no_change_gui) {
        form->resultCtl()->setVoxel(new_node);
        form->resultCtl()->setColorBand(voxel_scalar->minValue(), voxel_scalar->maxValue(), false, true);
    }
    const auto& threshold_values = form->resultCtl()->threshold();
    const auto& color_values     = form->resultCtl()->colors();

    /// 範囲内描画とかその他で行列制御よりシェーダー制御の方がやりやすいので変える
    // Matrix4x4f scale_matrix;
    // scale_matrix.scale(Nx * unit_length, Ny * unit_length, Nz * unit_length);
    //  scale_matrix.swapAxes(0, 2); /// シェーダーにやらせる
    // new_node->setMatrix(scale_matrix);

    bool create_success;
    if (!m_no_change_gui) {
        gl_view->set3DTextureColor(threshold_values, color_values);
        create_success = gl_view->create3DTexture(voxel_scalar);
        // voxel_scalar->setVboUse(false);
        gl_view->createRenderData(voxel_scalar);
        gl_view->setOpt3DNode(new_node.ptr());
    }

    if (m_pd) {
        m_pd->setValue(100);
    }

    if (!m_no_change_gui) {
        gl_view->fitDisplay();
        gl_view->update();
    }
    else {
        return true;
    }

    if (!create_success) {
        if (form->settingGuiCtrl()->readOptMemoryWarn()) {
            QDialog dialog(form);
            dialog.setWindowTitle(form->windowTitle());
            QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);

            // 警告アイコン＋テキスト
            QHBoxLayout* iconLayout = new QHBoxLayout();
            QLabel*      iconLabel  = new QLabel;
            iconLabel->setPixmap(QMessageBox::standardIcon(QMessageBox::Warning));
            iconLayout->addWidget(iconLabel);

            // 隙間を追加
            iconLayout->addSpacing(10);

            QLabel* label = new QLabel("メモリ不足の可能性があり正しく表示できません。\n\n"
                                       "設定 - 動作設定(パフォーマンス・その他) - 「Opt パフォーマンス設定」\n"
                                       "で制御することで表示できます。\n"
                                       "※ 設定後再度ファイルを読み込んでください。\n");
            iconLayout->addWidget(label);
            mainLayout->addLayout(iconLayout);

            // ボタン配置
            QHBoxLayout* buttonLayout  = new QHBoxLayout();
            QPushButton* settingButton = new QPushButton("設定を開く");
            QPushButton* okButton      = new QPushButton("OK");
            buttonLayout->addWidget(settingButton, 0, Qt::AlignLeft);
            buttonLayout->addStretch();
            buttonLayout->addWidget(okButton, 0, Qt::AlignRight);
            mainLayout->addLayout(buttonLayout);

            dialog.setLayout(mainLayout);

            QObject::connect(settingButton, &QPushButton::clicked, [&]() {
                dialog.accept();
                /// 設定ダイアログ表示
                OptPerformanceSetting dlg(form, form->settingGuiCtrl());
                dlg.exec();
            });
            QObject::connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);

            dialog.exec();
        }
    }

    return true;
}

void ReadOpt::setProgMessage(int value)
{
    if (m_pd && !m_cancel) {
        m_pd->setValue(value);
    }
}

void ReadOptFileThread::run()
{
    FILE*   fp  = NULL;
    errno_t err = _wfopen_s(&fp, m_file_path.c_str(), L"rb");
    if (err != 0 || fp == NULL) {
        return;
    }

    auto scene_graph = m_gl_view->sceneView()->sceneGraph();
    auto root_node   = scene_graph->rootNode();

    /// ヘッダ読込
    unsigned char buf[28];
    size_t        read_size = fread(buf, 1, sizeof(buf), fp);
    if (read_size < sizeof(buf)) {
        fclose(fp);
        return;
    }

    /// 2 Byte(short型):Nx(X方向メッシュ数)
    /// 2 Byte(short型):Ny(Y方向メッシュ数)
    /// 2 Byte(short型):Nz(Z方向メッシュ数)
    /// 4 Byte(float型):メッシュサイズ[m]
    /// 4 Byte(float型):波長λ[m]
    /// 4 Byte(float型):入射角Θ[degree]
    /// 4 Byte(float型):入射角φ[degree]
    /// 4 Byte(float型):入射角Ψ[degree]
    /// 2 Byte(short型):励振面位置(Z軸座標)

    size_t Nx        = *(short*)(buf + 0);    /// size_tで扱う
    size_t Ny        = *(short*)(buf + 2);
    size_t Nz        = *(short*)(buf + 4);
    float  mesh_size = *(float*)(buf + 6);
    float  rd        = *(float*)(buf + 10);
    float  t         = *(float*)(buf + 14);
    float  ph        = *(float*)(buf + 18);
    float  py        = *(float*)(buf + 22);
    short  z_        = *(short*)(buf + 26);

    size_t NxAdj = Nx;
    size_t NyAdj = Ny;
    size_t NzAdj = Nz;

    if (m_memory_limit) {
        size_t max_file_size = m_memory_size;

        /// サイズオーバーのとき
        int div_x = 2, div_y = 2, div_z = 2;
        while ((4 * NxAdj * NyAdj * NzAdj) > max_file_size) {
            if (NxAdj >= NyAdj && NxAdj >= NzAdj && NxAdj > 1) {
                NxAdj = (NxAdj + div_x - 1) / div_x;
                if (NxAdj < 1) NxAdj = 1;
                div_x++;
            }
            else if (NyAdj >= NxAdj && NyAdj >= NzAdj && NyAdj > 1) {
                NyAdj = (NyAdj + div_y - 1) / div_y;
                if (NyAdj < 1) NyAdj = 1;
                div_y++;
            }
            else if (NzAdj > 1) {
                NzAdj = (NzAdj + div_z - 1) / div_z;
                if (NzAdj < 1) NzAdj = 1;
                div_z++;
            }
            else {
                break;
            }
        }
    }

    DEBUG() << "Nx " << Nx;
    DEBUG() << "Ny " << Ny;
    DEBUG() << "Nz " << Nz;
    DEBUG() << "Nx Adj" << NxAdj;
    DEBUG() << "Ny Adj" << NyAdj;
    DEBUG() << "Nz Adj" << NzAdj;
    DEBUG() << "File Size" << 4 * Nx * Ny * Nz + 28;
    DEBUG() << "Memory Size" << 4 * NxAdj * NyAdj * NzAdj;

    emit setProgMessage(10);

    int prog_min   = 10;
    int prog_max   = 80;
    int prog_step  = 10;
    int prog_range = (prog_max - prog_min) / prog_step + 1;    // 8段階
    int prog_count = Nx / prog_range + 1;

    size_t zero_count      = 0;
    float  e2_min          = FLT_MAX;
    float  e2_min_not_zero = FLT_MAX;
    float  e2_max          = -FLT_MAX;

    float* e2_data     = new float[NxAdj * NyAdj * NzAdj];
    size_t NyNz        = Ny * Nz;
    size_t slice_size  = 10;
    float* work_buffer = new float[slice_size * Ny * Nz];
    size_t work_index  = 0;
    size_t p_index     = 0;

    bool data_compress = false;

    /// 圧縮なし
    if (Nx == NxAdj && Ny == NyAdj && Nz == NzAdj) {
        for (size_t ix = 0; ix < Nx; ++ix) {
            if (ix % slice_size == 0) {
                read_size  = fread(work_buffer, 4, slice_size * NyNz, fp);
                work_index = 0;
            }

            for (size_t index = 0; index < NyNz; ++index) {
                if (work_index >= read_size) {
                    DEBUG() << "File Error";
                    break;
                }
                float value = e2_data[p_index++] = work_buffer[work_index++];
                if (value == 0.0f) {
                    if (e2_min > value) e2_min = value;
                    zero_count++;
                }
                else {
                    if (e2_min_not_zero > value) e2_min_not_zero = value;
                }
                if (e2_max < value) e2_max = value;
            }

            if (ix % prog_count == 0) {
                if (m_cancel) {
                    return;
                }
                int progress_value = prog_min + ((ix / prog_count) * prog_step);
                if (progress_value > prog_max) progress_value = prog_max;
                emit setProgMessage(progress_value);
            }
        }
    }
    /// データ圧縮
    else {
        data_compress = true;

        size_t NxNyNzAdj = NxAdj * NyAdj * NzAdj;
        size_t NyNzAdj   = NyAdj * NzAdj;

        /// 冗長だがとりあえず。。。
        unsigned short* average_count_buffer = nullptr;

        if (m_memory_method == 0) {
            memset(e2_data, 0, NxNyNzAdj * 4);
            DEBUG() << "Max merge";
        }
        if (m_memory_method == 1) {
            std::fill(e2_data, e2_data + NxNyNzAdj, FLT_MAX);
            DEBUG() << "Min merge";
        }
        else if (m_memory_method == 2) {
            memset(e2_data, 0, NxNyNzAdj * 4);
            average_count_buffer = new unsigned short[NxNyNzAdj];
            memset(average_count_buffer, 0, NxNyNzAdj * 2);
            DEBUG() << "Average merge";
        }

        for (size_t ix = 0; ix < Nx; ++ix) {
            if (ix % slice_size == 0) {
                read_size  = fread(work_buffer, 4, slice_size * NyNz, fp);
                work_index = 0;
            }
            size_t ixAdj = ix * NxAdj / Nx;
            if (ixAdj >= NxAdj) ixAdj = NxAdj - 1;
            size_t adj_index_x = ixAdj * NyNzAdj;

            for (size_t iy = 0; iy < Ny; ++iy) {
                size_t iyAdj = iy * NyAdj / Ny;
                if (iyAdj >= NyAdj) iyAdj = NyAdj - 1;
                size_t adj_index_xy = adj_index_x + iyAdj * NzAdj;
                for (size_t iz = 0; iz < Nz; ++iz) {
                    if (work_index >= read_size) {
                        DEBUG() << "File Error";
                        break;
                    }

                    size_t izAdj = iz * NzAdj / Nz;
                    if (izAdj >= NzAdj) izAdj = NzAdj - 1;
                    size_t adj_index = adj_index_xy + izAdj;

                    float value = work_buffer[work_index++];

                    /// 最大
                    if (m_memory_method == 0) {
                        if (e2_data[adj_index] < value) {
                            e2_data[adj_index] = value;
                        }
                    }
                    /// 最小
                    else if (m_memory_method == 1) {
                        if (e2_data[adj_index] > value) {
                            e2_data[adj_index] = value;
                        }
                    }
                    /// 平均
                    else if (m_memory_method == 2) {
                        e2_data[adj_index] += value;
                        ++average_count_buffer[adj_index];
                    }
                    else {
                        /// とりあえず最大
                        if (e2_data[adj_index] < value) {
                            e2_data[adj_index] = value;
                        }
                    }

                    if (value == 0.0f) {
                        if (e2_min > value) e2_min = value;
                        zero_count++;
                    }
                    else {
                        if (e2_min_not_zero > value) e2_min_not_zero = value;
                    }
                    if (e2_max < value) e2_max = value;
                }
            }

            if (ix % prog_count == 0) {
                if (m_cancel) {
                    delete[] e2_data;
                    delete[] work_buffer;
                    fclose(fp);
                    return;
                }
                int progress_value = prog_min + ((ix / prog_count) * prog_step);
                if (progress_value > prog_max) progress_value = prog_max;
                emit setProgMessage(progress_value);
            }
        }

        if (m_memory_method == 1) {
            for (size_t ic = 0; ic < NxNyNzAdj; ++ic) {
                if (e2_data[ic] == FLT_MAX) {
                    e2_data[ic] = 0.0f;
                }
            }
        }
        else if (m_memory_method == 2) {
            for (size_t ic = 0; ic < NxNyNzAdj; ++ic) {
                if (average_count_buffer[ic] > 0) {
                    e2_data[ic] /= (float)average_count_buffer[ic];
                    if (e2_data[ic] > e2_max) {
                        DEBUG() << e2_data[ic];
                        e2_max = e2_data[ic];
                    }
                }
                else {
                    e2_data[ic] = 0.0f;
                }
            }
            delete[] average_count_buffer;
        }
    }

    delete[] work_buffer;
    fclose(fp);

    if (m_cancel) {
        return;
    }

    DEBUG() << "Min " << e2_min;
    DEBUG() << "Min Not Zero " << e2_min_not_zero;
    DEBUG() << "Zero Count " << zero_count;
    DEBUG() << "Max " << e2_max;

    float unit_length   = mesh_size * 1.0e6;
    float unit_length_x = unit_length * ((float)Nx / (float)NxAdj);
    float unit_length_y = unit_length * ((float)Ny / (float)NyAdj);
    float unit_length_z = unit_length * ((float)Nz / (float)NzAdj);

    auto voxel_scalar = VoxelScalar::createObject();
    voxel_scalar->init(0, 0, 0, unit_length_x, unit_length_y, unit_length_z, NxAdj, NyAdj, NzAdj);
    voxel_scalar->setCreateSectionLine(false);    /// 3D結果は輪郭作らない

    voxel_scalar->setScalarData(e2_data, true);
    voxel_scalar->createShapeForTexture();

    voxel_scalar->setMinMax(e2_min_not_zero, e2_max);

    if (data_compress) {
        voxel_scalar->setOriginalData(Point3i(Nx, Ny, Nz), Point3d(unit_length, unit_length, unit_length));
    }

    m_create_node = root_node->addChild();
    m_create_node->setObject(voxel_scalar.ptr());

    /// 属性で情報保持
    m_create_node->setUserAttributeFloat(L"Mesh size", mesh_size);
    m_create_node->setUserAttributeFloat(L"Wavelength", rd);
    m_create_node->setUserAttributeFloat(L"Incident angle theta", t);
    m_create_node->setUserAttributeFloat(L"Incident angle phi", ph);
    m_create_node->setUserAttributeFloat(L"Incident angle psi", py);
    m_create_node->setUserAttributeInt(L"Excitation plane position (Z-axis coordinate)", z_);
    m_create_node->setUserAttributeString(L"File Path", m_file_path);

    emit setProgMessage(90);
}
