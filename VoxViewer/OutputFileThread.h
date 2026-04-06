#ifndef OUTPUTFILETHREAD_H
#define OUTPUTFILETHREAD_H

#include <QFile>
#include <QImage>
#include <QTextStream>
#include <QThread>

#include <mutex>

class OutputFileThread : public QThread {
    Q_OBJECT
public:
    class FileData {
    public:
        FileData(const QString& path) : m_file_path(path) {}
        virtual ~FileData() {}
        virtual void output() = 0;

        bool           isSuccess() { return m_success; }
        const QString& path() { return m_file_path; }

    protected:
        QString m_file_path;
        bool    m_success = false;
    };

    class ImageFileData : public FileData {
    public:
        ImageFileData(const QImage& image, const QString& path) : m_image(image), FileData(path) {}
        ~ImageFileData() {}
        void output() override
        {
            if (m_image.save(m_file_path)) {
                m_success = true;
            }
            m_image = QImage();
        }

    private:
        QImage m_image;
    };

    class TextFileData : public FileData {
    public:
        TextFileData(const QString& str, const QString& path) : m_str(str), FileData(path) {}
        ~TextFileData() {}
        void output() override
        {
            QFile out_file(m_file_path);
            if (out_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream ts(&out_file);
                ts << m_str;
                out_file.close();
                m_success = true;
            }
            m_str.clear();
            m_str = QString();
        }

    private:
        QString m_str;
    };

public:
    OutputFileThread();
    ~OutputFileThread();

    void stop();
    void cancel();
    int  count();
    int  size();

    void addFileData(QImage& image, const QString& path);
    void addFileData(const QString& str, const QString& path);

    std::vector<FileData*>& fileDataList() { return m_file_list; }

protected:
    void run();
    void process_one(FileData* file_data);

private:
    std::vector<FileData*> m_file_list;

    std::atomic<bool>       m_stop;
    std::atomic<bool>       m_cancel;
    std::mutex              m_mutex;    /// Wrapperは使わない(m_cvで使えないため）
    std::condition_variable m_cv;
    std::atomic<int>        m_count;
};
#endif    // OUTPUTFILETHREAD_H
