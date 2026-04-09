#include "ReadVoxel.h"

#include "ResultCtrl.h"
#include "Vox3DForm.h"

#include "MyOpenGLWidget.h"

#include "Scene/Voxel.h"
#include "Scene/VoxelScalar.h"

#include <QDebug>
#include <QFile>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QStringDecoder>
#include <QTextStream>

#include <algorithm>
#include <execution>

#include "lz4.h"

/// Qtの実装で遅いので対策
void readAll(QTextStream& textStream, QString& decode_data)
{
    auto dv = textStream.device();
    if (!dv) {
        return;
    }
    QStringDecoder decoder(textStream.encoding());
    const auto&    read_data = dv->readAll();
    decode_data              = decoder.decode(read_data);
}

bool ReadVoxel::read(const QString& path, MyOpenGLWidget* gl_view, Vox3DForm* form)
{
    INFO() << "Read Voxel: " << path;

    auto scene_graph = gl_view->sceneView()->sceneGraph();
    auto root_node   = scene_graph->rootNode();

    /// プログレスバー表示中に固まらないように処理とメインスレッドを分ける
    /// ただしGUIアクセス部分があるので、それを回避してわける
    /// GUIアクセス部分はメインスレッドで行う

    std::vector<std::pair<int, RefPtr<Node>>> material_id_node_list;

    int prog_step             = 5;                 /// ProgressBarの刻み
    int file_to_voxel_percent = 10 * prog_step;    /// ReadVoxelReadFileThreadで進むパーセント位置

    if (m_pd) {
        /// 100%表記
        m_pd->setLabelText("Read Voxel Data");
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

    /// 関数分けるべき
    {
        std::map<int, Core::RefPtr<Core::Voxel>> material_id_voxel_map;
        std::map<int, QString>                   material_id_name_map;

        {
            QStringList* lines = nullptr;
            {
                QFile file(path);
                if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    return true;
                }

                lines = new QStringList;

                if (!m_no_change_gui) {
                    /// これがあるのでメインスレッドでとりあえず行う
                    /// QFile渡すとメモリ開放が遅れるので渡さない
                    gl_view->clearMaterialIdToNode(true);
                }

                auto& compress_data = form->resultCtl()->voxelFileCompressData();
                compress_data.resize(0);
                form->resultCtl()->setCompressDataOriginalSize(0);

                QTextStream in(&file);
                QString     content;
                readAll(in, content);
                file.close();

                *lines = content.split('\n', Qt::SkipEmptyParts);
            }

            /// ファイル読込み
            bool                read_file_end               = false;
            auto&               compress_data               = form->resultCtl()->voxelFileCompressData();
            int                 compress_data_original_size = 0;
            ReadVoxelFileThread read_file(lines, &material_id_voxel_map, &material_id_name_map, &compress_data,
                                          &compress_data_original_size, m_pd != nullptr, file_to_voxel_percent,
                                          prog_step);
            connect(&read_file, &ReadVoxelFileThread::setProgMessage, this, &ReadVoxel::setProgMessage,
                    Qt::QueuedConnection);
            connect(&read_file, &ReadVoxelFileThread::finished, this, [&read_file_end] { read_file_end = true; });
            read_file.start();

            /// 計算中 - GUI更新
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

            form->resultCtl()->setCompressDataOriginalSize(compress_data_original_size);
        }

        if (m_cancel) {
            auto& compress_data = form->resultCtl()->voxelFileCompressData();
            compress_data.resize(0);
            form->resultCtl()->setCompressDataOriginalSize(0);
            return false;
        }

        if (material_id_voxel_map.empty()) {
            return true;
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

        int id_count = 0;

        std::wstring file_path = path.toStdWString();

        for (auto& [id, voxel] : material_id_voxel_map) {
            /// データなし
            if (!voxel->hasBuffer()) {
                continue;
            }

            auto new_node = root_node->addChild();
            new_node->setObject(voxel.ptr());

            const auto& material_name = material_id_name_map[id];

            new_node->setName(material_name.toStdWString());

            /// 暫定 - すべて同じになるが、全体情報を保持
            new_node->setUserAttributeString(L"File Path", file_path);

            int tmpIndex = id_count % 10 + 1;    /// 先頭に空白（任意色）を入れたので必ず＋１する
            QString matColorName = label_matColorList.at(tmpIndex - 1);    /// label_matColorListは+1していない
            auto    color        = form->func_GL_defineColor_nameToRGBvec(matColorName);
            if (form->g_hash_matnameToCname.contains(material_name)) {
                color = form->func_GL_defineColor_nameToRGBvec(form->g_hash_matnameToCname.value(material_name));
            }
            if (!m_fdtd_mname_to_cname.empty()) {
                QString fdtd_name = m_fdtd_mname_to_cname.value(material_name);
                if (!fdtd_name.isEmpty()) {
                    color = form->func_GL_defineColor_nameToRGBvec(m_fdtd_mname_to_cname.value(material_name));
                }
            }
            voxel->setColor(Point4f(color[0], color[1], color[2], 1.0f));
            ++id_count;

            material_id_node_list.emplace_back(id, new_node);
        }

        if (m_pd) {
            m_pd->setValue(file_to_voxel_percent);
        }
    }

    if (m_cancel) {
        auto& compress_data = form->resultCtl()->voxelFileCompressData();
        compress_data.resize(0);
        form->resultCtl()->setCompressDataOriginalSize(0);

        std::set<Node*> nodes;
        for (auto& [id, node] : material_id_node_list) {
            nodes.insert(node.ptr());
        }
        root_node->removeChild(nodes);
        return false;
    }

    /// 表示データ作成
    bool                      create_disp_end = false;
    CreateVoxelDispDataThread create_disp(&material_id_node_list, m_pd != nullptr, file_to_voxel_percent, prog_step);
    connect(&create_disp, &CreateVoxelDispDataThread::setProgMessage, this, &ReadVoxel::setProgMessage,
            Qt::QueuedConnection);
    connect(&create_disp, &CreateVoxelDispDataThread::finished, this, [&create_disp_end] { create_disp_end = true; });
    create_disp.start();

    /// 計算中 - GUI更新
    while (1) {
        qApp->processEvents(Vox3DForm::isModalVisible(m_pd) ? QEventLoop::AllEvents
                                                            : QEventLoop::ExcludeUserInputEvents);
        if (create_disp_end) {
            break;
        }

        if (m_cancel) {
            create_disp.setCancel();
        }
    }

    if (m_cancel) {
        auto& compress_data = form->resultCtl()->voxelFileCompressData();
        compress_data.resize(0);
        form->resultCtl()->setCompressDataOriginalSize(0);

        std::set<Node*> nodes;
        for (auto& [id, node] : material_id_node_list) {
            nodes.insert(node.ptr());
        }
        root_node->removeChild(nodes);
        return false;
    }

    for (auto& [id, node] : material_id_node_list) {
        gl_view->createRenderEditableMesh(node.ptr());
    }

    if (m_pd) {
        m_pd->setValue(100);
    }

    if (!m_no_change_gui) {
        gl_view->setMaterialIdToNode(material_id_node_list);
        gl_view->fitDisplay();
    }

    return true;
}

void ReadVoxel::readFdtd(const QString& path)
{
    Vox3DForm::fdtdLoad(path, m_fdtd_mname_to_cname);
}

void ReadVoxel::setProgMessage(int value)
{
    if (m_pd && !m_cancel) {
        m_pd->setValue(value);
    }
}

void ReadVoxelFileThread::run()
{
    auto& lines                 = *m_lines;
    auto& material_id_voxel_map = *m_material_id_voxel_map;
    auto& material_id_name_map  = *m_material_id_name_map;

    int prog_step = m_prog_step;    /// ProgressBarの刻み
    int file_to_voxel_percent_0 =
        m_prog_max_value / 2;    /// ファイル読んでVoxelDataにするまでのProgressBarのパーセント

    int file_to_voxel_percent_1 = m_prog_max_value;

    double step      = lines.size() > 0 ? (double)(file_to_voxel_percent_0) / (double)lines.size() : 1;
    int    pre_count = 0;

    /// ヘッダ
    QString version;              /// 未使用
    double  precision   = 1.0;    /// 未使用
    double  unit_length = 1.0;    /// セルサイズ
    int     number_x    = 0;
    int     number_y    = 0;
    int     number_z    = 0;

    static QRegularExpression reg("\\s+");

    int line_count = 0;
    for (; line_count < lines.size(); ++line_count) {
        const auto&        line   = lines[line_count];
        const QStringList& fields = line.split(reg, Qt::SkipEmptyParts);

        if (fields.size() <= 0) {
            continue;
        }
        if (fields[0] == "CellData") {
            if (line_count < lines.size() - 1) {
                ++line_count;
                const QStringList& fields2 =
                    lines[line_count].split(reg, Qt::SkipEmptyParts);    /// 12 x 13 x 14 のフォーマット
                if (fields2.size() >= 5) {
                    number_x = fields2[0].toInt();
                    number_y = fields2[2].toInt();
                    number_z = fields2[4].toInt();
                }
                ++line_count;
            }
            break;
        }

        if (fields.size() >= 2) {
            switch (line_count) {
                case 0: {
                    if (fields[0] == "Version") {
                        version = fields[1];
                    }
                    break;
                }
                case 1: {
                    if (fields[1] == "precision") {
                        precision = fields[0].toDouble();
                    }
                    break;
                }
                case 2: {
                    if (fields[1] == "unitlength") {
                        unit_length = fields[0].toDouble();
                    }
                    break;
                }
                default: {
                    material_id_name_map.insert(std::pair<int, QString>(fields[0].toInt(), fields[1]));
                } break;
            }
        }
    }

    if (m_cancel.load()) {
        delete m_lines;
        return;
    }
    if (m_use_progress_bar) {
        int cur_count = (int)(((double)line_count * step));
        if (cur_count > file_to_voxel_percent_0) cur_count = file_to_voxel_percent_0;
        if (cur_count / prog_step != pre_count / prog_step) {
            emit this->setProgMessage(cur_count / m_prog_step * m_prog_step);
            pre_count = cur_count;
        }
    }

    std::vector<int> material_id_xs;

    for (; line_count < lines.size(); ++line_count) {
        const auto& line  = lines[line_count];
        int         start = 0;
        while (start < line.length()) {
            /// 一つ目
            int end = line.indexOf(' ', start);
            if (end == -1) {
                end = line.length();
            }
            if (start >= end) {
                break;
            }

            QStringView material(line.data() + start, line.data() + end);
            start = end + 1;

            /// 二つ目
            end = line.indexOf(' ', start);
            if (end == -1) {
                end = line.length();
            }
            if (start >= end) {
                break;
            }

            QStringView xcount(line.data() + start, line.data() + end);
            start = end + 1;

            const int material_id = material.toInt();
            const int x_count     = xcount.toInt();

            material_id_xs.emplace_back(material_id);
            material_id_xs.emplace_back(x_count);
        }

        if (m_use_progress_bar) {
            int cur_count = (int)(((double)line_count * step));
            if (cur_count > file_to_voxel_percent_0) cur_count = file_to_voxel_percent_0;
            if (cur_count / prog_step != pre_count / prog_step) {
                if (m_cancel.load()) {
                    delete m_lines;
                    return;
                }
                emit this->setProgMessage(cur_count / m_prog_step * m_prog_step);
                pre_count = cur_count;
            }
        }
    }
    delete m_lines;
    material_id_xs.shrink_to_fit();

    /// データなし
    if (number_x == 0 || number_y == 0 || number_z == 0) {
        return;
    }

    if (m_cancel.load()) {
        return;
    }

    /// 初期化
    for (auto& [id, name] : material_id_name_map) {
        auto voxel = Voxel::createObject();
        // voxel->init(0, 0, 0, unit_length, unit_length, unit_length, number_x, number_y, number_z);
        /// TEST
        // voxel->setVboUse(false);
        material_id_voxel_map[id] = voxel;
    }

    std::map<int, std::vector<std::map<std::pair<int, int>, std::vector<int>>>> map_ranges_value_zxy;

    /// データ読込み
    int current_x = 0;
    int current_y = 0;
    int current_z = 0;

    auto material_id_xs_size = material_id_xs.size();
    for (int ic = 0; ic < material_id_xs_size; ic += 2) {
        auto material_id = material_id_xs[ic];
        auto x_count     = material_id_xs[ic + 1];

        // Voxel* voxel_data = nullptr;
        // auto   itr_find   = material_id_voxel_map.find(material_id);
        // if (itr_find != material_id_voxel_map.end()) {
        //     voxel_data = itr_find->second.ptr();
        // }
        auto& ranges_value_zxy = map_ranges_value_zxy[material_id];

        if (m_cancel.load()) {
            return;
        }
        if (ranges_value_zxy.size() == 0) {
            ranges_value_zxy.resize(number_z);
        }

        /// 範囲指定して設定
        /// X最大(または超える)の場合（※超える場合考慮してwhile)
        while (current_x + x_count >= number_x) {
            // if (voxel_data) {
            //     voxel_data->setXCells(current_x, number_x - 1, current_y, current_z, true);
            // }
            auto& range_value_xy = ranges_value_zxy[current_z];
            range_value_xy[std::pair<int, int>(current_x, number_x - 1)].emplace_back(current_y);

            x_count -= (number_x - current_x);
            current_x = 0;
            ++current_y;
            if (current_y == number_y) {
                current_y = 0;
                ++current_z;
            }
        }

        if (x_count > 0) {
            // if (voxel_data) {
            //     voxel_data->setXCells(current_x, current_x + x_count - 1, current_y, current_z, true);
            // }
            auto& range_value_xy = ranges_value_zxy[current_z];
            range_value_xy[std::pair<int, int>(current_x, current_x + x_count - 1)].emplace_back(current_y);

            current_x += x_count;
        }
    }

    MutexWrapper mutex;
    int          material_count = 0;

    std::for_each(std::execution::par, map_ranges_value_zxy.begin(), map_ranges_value_zxy.end(), [&](auto& entry) {
        std::vector<std::map<std::pair<int, int>, std::vector<int>>>& ranges_value_zxy = entry.second;

        Voxel*     voxel_data = nullptr;
        const auto itr_find   = material_id_voxel_map.find(entry.first);
        if (itr_find != material_id_voxel_map.end()) {
            voxel_data = itr_find->second.ptr();
        }
        if (voxel_data) {
            std::set<int> x_values;
            std::set<int> y_values;
            std::set<int> z_values;

            for (int iz = 0; iz < ranges_value_zxy.size(); ++iz) {
                auto& range_value_xy = ranges_value_zxy[iz];
                if (range_value_xy.size() > 0) {
                    z_values.insert(iz - 1);
                    z_values.insert(iz);
                    z_values.insert(iz + 1);

                    for (auto& value_xy : range_value_xy) {
                        int x0 = value_xy.first.first;
                        int x1 = value_xy.first.second;

                        x_values.insert(x0 - 1);
                        x_values.insert(x0);
                        x_values.insert(x0 + 1);    /// 不要と思うが念のため
                        x_values.insert(x1 - 1);    /// 不要と思うが念のため
                        x_values.insert(x1);
                        x_values.insert(x1 + 1);

                        for (auto y : value_xy.second) {
                            y_values.insert(y - 1);
                            y_values.insert(y);
                            y_values.insert(y + 1);
                        }
                    }
                }
            }

            if (x_values.size()) {
                int x_min = *x_values.begin();
                if (x_min < 0) x_min = 0;
                int x_max = *x_values.rbegin();
                if (x_max > number_x - 1) x_max = number_x - 1;

                int y_min = *y_values.begin();
                if (y_min < 0) y_min = 0;
                int y_max = *y_values.rbegin();
                if (y_max > number_y - 1) y_max = number_y - 1;

                int z_min = *z_values.begin();
                if (z_min < 0) z_min = 0;
                int z_max = *z_values.rbegin();
                if (z_max > number_z - 1) z_max = number_z - 1;

                int               x_count = 0;
                std::vector<int>  old_new_index_x(x_max - x_min + 1, 0);
                std::vector<int>* new_old_index_x = new std::vector<int>;
                new_old_index_x->reserve(x_values.size() + 1);
                for (auto x : x_values) {
                    if (x >= x_min && x <= x_max) {
                        old_new_index_x[x - x_min] = x_count;
                        new_old_index_x->emplace_back(x);
                        x_count++;
                    }
                }
                new_old_index_x->emplace_back(x_max + 1);

                int               y_count = 0;
                std::vector<int>  old_new_index_y(y_max - y_min + 1, 0);
                std::vector<int>* new_old_index_y = new std::vector<int>;
                new_old_index_y->reserve(y_values.size() + 1);
                for (auto y : y_values) {
                    if (y >= y_min && y <= y_max) {
                        old_new_index_y[y - y_min] = y_count;
                        new_old_index_y->emplace_back(y);
                        y_count++;
                    }
                }
                new_old_index_y->emplace_back(y_max + 1);

                int               z_count = 0;
                std::vector<int>  old_new_index_z(z_max - z_min + 1, 0);
                std::vector<int>* new_old_index_z = new std::vector<int>;
                new_old_index_z->reserve(z_values.size() + 1);
                for (auto z : z_values) {
                    if (z >= z_min && z <= z_max) {
                        old_new_index_z[z - z_min] = z_count;
                        new_old_index_z->emplace_back(z);
                        z_count++;
                    }
                }
                new_old_index_z->emplace_back(z_max + 1);

                voxel_data->init(0, 0, 0, unit_length, unit_length, unit_length, x_count, y_count, z_count);

                /// init後に設定
                auto read_perform               = voxel_data->createReadPerformInfo();
                read_perform->m_new_old_index_x = new_old_index_x;
                read_perform->m_new_old_index_y = new_old_index_y;
                read_perform->m_new_old_index_z = new_old_index_z;
                read_perform->m_number.set(number_x, number_y, number_z);

                std::vector<int> z_range(z_max - z_min + 1);
                std::iota(z_range.begin(), z_range.end(), z_min);

                std::for_each(std::execution::par, z_range.begin(), z_range.end(), [&](int z) {
                    auto& ranges_value = ranges_value_zxy[z];

                    for (const auto& range : ranges_value) {
                        auto [start_x, end_x] = range.first;
                        const auto& y_values  = range.second;
                        for (int ic = 0; ic < (int)y_values.size(); ++ic) {
                            voxel_data->setXCells(old_new_index_x[start_x - x_min], old_new_index_x[end_x - x_min],
                                                  old_new_index_y[y_values[ic] - y_min], old_new_index_z[z - z_min],
                                                  true);
                        }
                    }
                });
            }
        }
        if (m_use_progress_bar) {
            mutex.lock();
            ++material_count;
            int cur_count = (int)(((double)material_count / (double)map_ranges_value_zxy.size()
                                   * (double)(file_to_voxel_percent_1 - file_to_voxel_percent_0))
                                  + file_to_voxel_percent_0);
            if (cur_count > file_to_voxel_percent_1) cur_count = file_to_voxel_percent_1;
            if (cur_count / prog_step != pre_count / prog_step) {
                emit this->setProgMessage(cur_count / m_prog_step * m_prog_step);
                pre_count = cur_count;
            }
            mutex.unlock();
        }
    });

    int         src_size = material_id_xs.size() * sizeof(int);
    const char* src_ptr  = reinterpret_cast<const char*>(material_id_xs.data());

    /// 最大圧縮サイズ
    int max_dst_size = LZ4_compressBound(static_cast<int>(src_size));

    /// 1/5サイズで圧縮を試みる
    int try_size = max_dst_size / 5;
    m_compress_data->resize(try_size);

    int compressed_size = LZ4_compress_default(src_ptr, m_compress_data->data(), static_cast<int>(src_size), try_size);

    /// 失敗したら最大サイズで再度圧縮
    if (compressed_size <= 0) {
        m_compress_data->resize(max_dst_size);
        compressed_size =
            LZ4_compress_default(src_ptr, m_compress_data->data(), static_cast<int>(src_size), max_dst_size);
    }

    if (compressed_size > 0) {
        /// 圧縮後サイズにリサイズ
        m_compress_data->resize(compressed_size);
        m_compress_data->shrink_to_fit();
        *m_compress_data_original_size = material_id_xs.size();
    }
    else {
        m_compress_data->resize(0);
        m_compress_data->shrink_to_fit();
        *m_compress_data_original_size = 0;
    }

    DEBUG() << "Voxel Material Data Src " << src_size;
    DEBUG() << "Voxel Material Data Compress " << compressed_size;
}

void CreateVoxelDispDataThread::run()
{
    double step      = m_material_id_node_list->size() > 0
                         ? (100.0 - (double)m_prog_init_value) / (double)m_material_id_node_list->size()
                         : 1;
    int    count     = 0;
    int    pre_count = 0;

    MutexWrapper mutex;

    std::for_each(std::execution::par, m_material_id_node_list->begin(), m_material_id_node_list->end(),
                  [&](std::pair<int, RefPtr<Node>>& entry) {
                      if (m_cancel.load()) {
                          return;
                      }
                      Voxel* voxel = (Voxel*)entry.second->object();
                      voxel->createDisplayData();
                      voxel->deleteVoxelData();    /// 保持する必要がないので削除
                      // voxel->markSegmentsGroupDirty();

                      if (m_use_progress_bar) {
                          mutex.lock();
                          int cur_count = (int)(((double)count++ * step) + m_prog_init_value);
                          if (cur_count > 100) cur_count = 100;
                          if (cur_count / m_prog_step != pre_count / m_prog_step) {
                              emit this->setProgMessage(cur_count / m_prog_step * m_prog_step);
                              pre_count = cur_count;
                          }
                          mutex.unlock();
                      }
                  });
}
