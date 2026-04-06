#include "OutputFileThread.h"

OutputFileThread::OutputFileThread() : m_stop(false), m_cancel(false), m_count(0) {}

OutputFileThread::~OutputFileThread()
{
    for (auto& file : m_file_list) {
        delete file;
    }
}

void OutputFileThread::stop()
{
    m_stop.store(true);
    m_cv.notify_all();
}

void OutputFileThread::cancel()
{
    m_cancel.store(true);
    m_cv.notify_all();
}

int OutputFileThread::count()
{
    return m_count.load();
}

int OutputFileThread::size()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_file_list.size();
}

void OutputFileThread::addFileData(QImage& image, const QString& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_file_list.emplace_back(new ImageFileData(image, path));
    m_cv.notify_one();
}

void OutputFileThread::addFileData(const QString& str, const QString& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_file_list.emplace_back(new TextFileData(str, path));
    m_cv.notify_one();
}

void OutputFileThread::run()
{
    unsigned int core_count   = std::thread::hardware_concurrency();
    unsigned int MAX_PARALLEL = std::clamp(core_count, 2u, 24u);

    std::vector<std::future<void>> running;
    size_t                         processed_cursor = 0;

    while (!m_stop.load()) {
        FileData* item;

        /// 新規アイテム取得
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock,
                      [this, &processed_cursor] { return m_stop.load() || processed_cursor < m_file_list.size(); });
            if (m_stop.load()) break;
            if (processed_cursor >= m_file_list.size()) continue;
            item = m_file_list[processed_cursor];
            ++processed_cursor;
        }

        /// running future掃除
        while (running.size() >= MAX_PARALLEL) {
            for (size_t i = 0; i < running.size();) {
                using namespace std::chrono_literals;
                if (running[i].wait_for(0ms) == std::future_status::ready) {
                    running[i].get();
                    running.erase(running.begin() + i);
                }
                else {
                    ++i;
                }
            }
            if (running.size() >= MAX_PARALLEL) std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        /// 並列タスク投入
        running.emplace_back(std::async(std::launch::async, [=]() {
            process_one(item);
            m_count.fetch_add(1);
        }));
    }

    /// stop後、残り（未処理）が最後まで消化される必要あり
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (processed_cursor < m_file_list.size()) {
            // if (m_cancel.load()) {/// 後出力するだけなのでキャンセルしない（失敗扱いになるので）
            //     break;
            // }

            auto item = m_file_list[processed_cursor];
            ++processed_cursor;
            /// 並列数チェックは再利用
            while (running.size() >= MAX_PARALLEL) {
                for (size_t i = 0; i < running.size();) {
                    using namespace std::chrono_literals;
                    if (running[i].wait_for(0ms) == std::future_status::ready) {
                        running[i].get();
                        running.erase(running.begin() + i);
                    }
                    else {
                        ++i;
                    }
                }
                if (running.size() >= MAX_PARALLEL) std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            /// タスク投下
            running.emplace_back(std::async(std::launch::async, [=]() {
                process_one(item);
                m_count.fetch_add(1);
            }));
        }
    }

    /// 残りのタスクも終了待ち
    for (auto& f : running) {
        f.get();
    }
}

void OutputFileThread::process_one(FileData* file_data)
{
    file_data->output();
}
