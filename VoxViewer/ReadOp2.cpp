#include "ReadOp2.h"

#include "ResultCtrl.h"
#include "Vox3DForm.h"

#include "MyOpenGLWidget.h"

#include "Scene/Voxel.h"
#include "Scene/VoxelScalar.h"

#include <QDebug>
#include <QFile>
#include <QProgressDialog>
#include <QStringDecoder>
#include <QTextStream>

#include <execution>

ReadOp2::ReadOp2(QObject* parent, QProgressDialog* pd) : QObject(parent), m_pd(pd) {}

bool ReadOp2::read(const QStringList& paths, MyOpenGLWidget* gl_view, Vox3DForm* form)
{
    auto scene_graph = gl_view->sceneView()->sceneGraph();
    auto root_node   = scene_graph->rootNode();

    std::vector<RefPtr<Node>>           create_voxels;
    std::map<QString, WeakRefPtr<Node>> path_to_voxels;

    const auto& exists_2d_list = form->resultCtl()->path2dList();

    /// 最初にファイル名（斜め断面）を判定
    int  max_x_array  = 0;
    int  max_y_array  = 0;
    bool has_dsection = maxArrayXY(paths, max_x_array, max_y_array, !m_no_change_gui ? &exists_2d_list : nullptr);

    bool d_cancel = false;
    if (has_dsection) {
        if (!m_no_change_gui) {
            if (!form->resultCtl()->setDSectionParam(max_x_array, max_y_array, m_auto_dsection)) {
                d_cancel = true;
            }
        }
    }

    if (m_pd) {
        m_pd->setModal(true);
        Qt::WindowFlags flags = m_pd->windowFlags();
        flags &= ~Qt::WindowCloseButtonHint;
        m_pd->setWindowFlags(flags);
        m_pd->open();
        m_pd->setFixedSize(m_pd->sizeHint());
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

        /// 100%表記
        m_pd->setLabelText("Read Op2 Data");
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

    if (!m_op2_import_mode) {
        form->resultCtl()->deleteResult2dList(true);
    }

    /// ファイル読込み
    bool              read_file_end = false;
    ReadOp2FileThread read_file(&paths, gl_view, form, m_no_change_gui, d_cancel, m_auto_dsection, m_op2_array_list,
                                m_vox_box);
    connect(&read_file, &ReadOp2FileThread::setProgMessage, this, &ReadOp2::setProgMessage, Qt::QueuedConnection);
    connect(&read_file, &ReadOp2FileThread::finished, this, [&read_file_end] { read_file_end = true; });
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

    auto& nodes = read_file.createNode();
    for (auto& [lower_path, new_node] : nodes) {
        root_node->appendChild(new_node.ptr());
        create_voxels.emplace_back(new_node);
        path_to_voxels[lower_path] = new_node;
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

    if (!m_no_change_gui) {
        form->resultCtl()->addVoxel2d(create_voxels, path_to_voxels);

        setProgMessage(95);

        float e2_min_not_zero_all = read_file.e2MinNotZeroAll();
        float e2_max_all          = read_file.e2MaxAll();

        form->resultCtl()->setColorBand(e2_min_not_zero_all, e2_max_all, false, true);
        const auto& threshold_values = form->resultCtl()->threshold();
        const auto& color_values     = form->resultCtl()->colors();

        gl_view->set3DTextureColor(threshold_values, color_values);

        for (auto& voxel_scalar : create_voxels) {
            auto voxel_scalar_obj = voxel_scalar->object<VoxelScalar>();
            gl_view->createRenderEditableMesh(voxel_scalar.ptr());
            gl_view->create2DTexture(voxel_scalar_obj);
        }

        setProgMessage(100);

        gl_view->fitDisplay();
    }
    else if (m_auto_dsection && has_dsection && !m_op2_array_list) {
        for (auto& voxel_node : create_voxels) {
            form->resultCtl()->setDSectionMatrix(voxel_node.ptr(), -1, max_x_array, max_y_array,
                                                 m_vox_box.xMax() - m_vox_box.xMin(),
                                                 m_vox_box.yMax() - m_vox_box.yMin());
        }
    }

    if (m_cancel) {
        return false;
    }

    return true;
    // gl_view->setOpt3DNode(new_node.ptr());
}

bool ReadOp2::maxArrayXY(const QStringList& paths, int& max_x_array, int& max_y_array,
                         const std::map<QString, WeakRefPtr<Node>>* exists_2d_list)
{
    bool has_dsection = false;

    for (const auto& path : paths) {
        /// 読込み済みは除外
        QString lower_path = path.toLower();
        if (exists_2d_list && (exists_2d_list->find(lower_path) != exists_2d_list->end())) {
            continue;
        }

        QString     symbol, symbol2, number, number2;
        const auto& file_name = QFileInfo(path).completeBaseName();
        if (ResultCtrl::sectionName(file_name, symbol, number, symbol2, number2)) {
            if (symbol.compare("D", Qt::CaseInsensitive) == 0) {
                has_dsection = true;

                /// Array最大数とる
                int x_array = number2.toInt();
                if (x_array > max_x_array) {
                    max_x_array = x_array;
                }

                int y_array = 0;
                if (symbol2.size() == 1) {
                    char c = symbol2[0].toLatin1();
                    if (c >= 'A' && c <= 'Z') {
                        y_array = c - 'A' + 1;    // 'A'～'Z' → 0～25
                    }
                    else if (c >= 'a' && c <= 'z') {
                        y_array = c - 'a' + 1;    // 'a'～'z' → 0～25
                    }
                }
                if (y_array > max_y_array) {
                    max_y_array = y_array;
                }
            }
        }
    }

    return has_dsection;
}

void ReadOp2::setProgMessage(int value)
{
    if (m_pd && !m_cancel) {
        m_pd->setValue(value);
    }
}

ReadOp2FileThread::ReadOp2FileThread(const QStringList* file_path, MyOpenGLWidget* gl_view, Vox3DForm* form,
                                     bool no_change_gui, bool d_section_cancel, bool auto_dsection,
                                     const QList<QPair<int, int>>* op2_array_list, BoundingBox3f& bbox)
    : m_file_path(file_path)
    , m_gl_view(gl_view)
    , m_form(form)
    , m_no_change_gui(no_change_gui)
    , m_dsection_cancel(d_section_cancel)
    , m_auto_dsection(auto_dsection)
    , m_op2_array_list(op2_array_list)
    , m_vox_box(bbox)
{
}

void ReadOp2FileThread::run()
{
    const auto& exists_2d_list = m_form->resultCtl()->path2dList();

    int prog_min   = 0;
    int prog_max   = 90;
    int prog_step  = 10;
    int prog_range = (prog_max - prog_min) / prog_step + 1;    // 8段階
    int prog_count = m_file_path->size() / prog_range + 1;

    for (int ic = 0; ic < m_file_path->size(); ++ic) {
        if (m_cancel.load()) {
            return;
        }

        const auto& path = (*m_file_path)[ic];

        QString lower_path = path.toLower();
        if (!m_no_change_gui) {
            /// 読込み済みは除外
            if (exists_2d_list.find(lower_path) != exists_2d_list.end()) {
                if (ic % prog_count == 0) {
                    int progress_value = prog_min + ((ic / prog_count) * prog_step);
                    if (progress_value > prog_max) progress_value = prog_max;
                    emit setProgMessage(progress_value);
                }
                continue;
            }
        }

        /// ファイル名から座標を決める
        QString     symbol, symbol2, number, number2;
        const auto& file_name       = QFileInfo(path).completeBaseName();
        bool        valid_file_name = ResultCtrl::sectionName(file_name, symbol, number, symbol2, number2);
        if (valid_file_name && m_dsection_cancel) {
            if (symbol.compare("D", Qt::CaseInsensitive) == 0) {
                if (ic % prog_count == 0) {
                    int progress_value = prog_min + ((ic / prog_count) * prog_step);
                    if (progress_value > prog_max) progress_value = prog_max;
                    emit setProgMessage(progress_value);
                }
                continue;
            }
        }

        INFO() << "Read Op2: " << path;

        MemoryMappedFile mmfile(path.toStdWString());
        mmfile.openFile();
        if (!mmfile.isValid()) {
            if (ic % prog_count == 0) {
                int progress_value = prog_min + ((ic / prog_count) * prog_step);
                if (progress_value > prog_max) progress_value = prog_max;
                emit setProgMessage(progress_value);
            }
            continue;
        }
        // 2 Byte(short型):Nx(X方向メッシュ数)
        // 2 Byte(short型):Ny(Y方向メッシュ数)
        // 4 Byte(float型):メッシュサイズ[m]
        // 4 Byte(float型):波長λ[m]
        // 4 Byte(float型):入射角φ[degree]
        // 2 Byte(short型):励振面位置(Y軸座標)
        // 1 Byte(char型) :TMTE判定フラグ（0：TM、1:TE）

        short Nx        = mmfile.getShortAt(0);
        short Ny        = mmfile.getShortAt(2);
        float mesh_size = mmfile.getFloatAt(4);    // m
        float rd        = mmfile.getFloatAt(8);
        float ph        = mmfile.getFloatAt(12);
        short ypos      = mmfile.getFloatAt(16);
        char  tmte      = mmfile.getCharAt(18);

        DEBUG() << "Nx " << Nx;
        DEBUG() << "Ny " << Ny;
        DEBUG() << "File Size" << (size_t)4 * ((size_t)Nx * (size_t)Ny) + (size_t)19;

        float unit_length = mesh_size * 1.0e6;

        size_t zero_count      = 0;
        float  e2_min          = FLT_MAX;
        float  e2_min_not_zero = FLT_MAX;
        float  e2_max          = -FLT_MAX;

        /// 2次元データでデータ量も大したことないので作り変える
        float* data = new float[(size_t)Nx * (size_t)Ny];

        for (size_t iy = 0; iy < Ny; ++iy) {
            size_t file_index_y = (size_t)4 * ((size_t)iy * (size_t)Nx) + (size_t)19;
            size_t data_index_y = ((size_t)iy * (size_t)Nx);
            for (size_t ix = 0; ix < Nx; ++ix) {
                size_t file_index = file_index_y + 4 * (size_t)ix;
                size_t data_index = data_index_y + (size_t)ix;

                float value      = mmfile.getFloatAt(file_index);
                data[data_index] = value;

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

        mmfile.closeFile();

        DEBUG() << "Min " << e2_min;
        DEBUG() << "Min Not Zero " << e2_min_not_zero;
        DEBUG() << "Zero Count " << zero_count;
        DEBUG() << "Max " << e2_max;
        DEBUG() << "Mesh Size " << unit_length;

        auto voxel_scalar = VoxelScalar::createObject();
        voxel_scalar->init(0, 0, 0, unit_length, unit_length, unit_length, Nx, Ny, 1);
        voxel_scalar->set2DTexture(true);
        voxel_scalar->setCreateSectionLine(false);    /// 3D結果は輪郭作らない

        voxel_scalar->setScalarData(data, true);
        voxel_scalar->createShapeForTexture();

        voxel_scalar->setMinMax(e2_min_not_zero, e2_max);

        RefPtr<Node> new_node = Node::createNode();
        new_node->setObject(voxel_scalar.ptr());

        // create_voxels.emplace_back(new_node);
        // path_to_voxels[lower_path] = new_node;

        m_create_nodes.emplace_back(lower_path, new_node);

        /// セル中心に合わせる（セル境界は正しくない）
        float cell_center = unit_length * 0.5;

        std::wstring section_name;

        /// ファイル名から座標を決める
        if (valid_file_name) {
            float move_value = number.toFloat() * 1.0e-3f;    /// um

            Matrix4x4f matrix;
            if (symbol.compare("X", Qt::CaseInsensitive) == 0) {
                matrix.rotateDegree(90.0f, Point3f(1, 0, 0));
                matrix.rotateDegree(90.0f, Point3f(0, 1, 0));
                matrix.translate(Point3f(0, 0, move_value + cell_center));

                section_name = L"X";
            }
            else if (symbol.compare("Y", Qt::CaseInsensitive) == 0) {
                matrix.rotateDegree(90.0f, Point3f(1, 0, 0));
                matrix.translate(Point3f(0, 0, -move_value - cell_center));

                section_name = L"Y";
            }
            else if (symbol.compare("Z", Qt::CaseInsensitive) == 0) {
                matrix.translate(Point3f(0, 0, move_value + cell_center));

                section_name = L"Z";
            }
            else if (symbol.compare("D", Qt::CaseInsensitive) == 0) {
                /// 共通化のためD断面はここでmatrix設定はしない

                if (number.toInt() == 45) {
                    section_name = L"D45";
                }
                else if (number.toInt() == 135) {
                    section_name = L"D135";
                }
            }

            if (!matrix.isIdentity()) {
                new_node->setMatrix(matrix);
            }
        }

        new_node->setName(file_name);

        /// 属性で情報保持
        new_node->setUserAttributeFloat(L"Mesh size", mesh_size);
        new_node->setUserAttributeFloat(L"Wavelength", rd);
        new_node->setUserAttributeFloat(L"Incident angle phi", ph);
        new_node->setUserAttributeInt(L"Excitation plane position (Y-axis coordinate)", ypos);
        new_node->setUserAttributeString(L"TMTE Flag", tmte ? L"TE" : L"TM");
        new_node->setUserAttributeString(L"File Path", path.toStdWString());
        new_node->setUserAttributeString(L"Section", section_name);

        if (m_e2_min_not_zero_all > e2_min_not_zero) {
            m_e2_min_not_zero_all = e2_min_not_zero;
        }
        if (m_e2_max_all < e2_max) {
            m_e2_max_all = e2_max;
        }

        if (m_auto_dsection && m_op2_array_list) {
            if (m_op2_array_list->size() > ic) {
                const auto& array = m_op2_array_list->at(ic);
                if (array.first > 0 && array.second > 0) {
                    m_form->resultCtl()->setDSectionMatrix(new_node.ptr(), -1, array.first, array.second,
                                                           m_vox_box.xMax() - m_vox_box.xMin(),
                                                           m_vox_box.yMax() - m_vox_box.yMin());
                }
            }
        }

        if (ic % prog_count == 0) {
            int progress_value = prog_min + ((ic / prog_count) * prog_step);
            if (progress_value > prog_max) progress_value = prog_max;
            emit setProgMessage(progress_value);
        }
    }
}
