#include "MainWindow3DViewer.h"
#include "SettingCtrl.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QtLogging>

#include <QLockFile>
#include <QMutex>

#define USE_QT 1
#include "../Core/CoreDebug.h"

/// 暫定: INIファイルの設定で切り替えれるようにする
#define WITH_INSTALLER 1

void MessageHandler(const QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    /// ログ出力レベルで出力内容を制御する
    static const int logLevel = SettingCtrl::logLevel();
    if (logLevel == 0) {    /// 出力しない
        return;
    }
    else if (logLevel == 1) {    /// Debug以外を出力する
        if (type == QtMsgType::QtDebugMsg) {
            return;
        }
    }
    else if (logLevel < 0) {    /// マイナスがつく場合標準出力のみ
        if (logLevel == -1) {
            if (type == QtMsgType::QtDebugMsg) {
                return;
            }
        }
        const auto& logStr = qFormatLogMessage(type, context, msg);
        QTextStream(stdout) << logStr << Qt::endl;
        return;
    }

    {
        static QMutex mutex;
        QMutexLocker  locker(&mutex);    /// スレッド排他

        static QFile                      outFile;
        static std::shared_ptr<QLockFile> tryLock;
        static bool                       first_open = true;
        if (first_open) {
            /// 複数起動考慮してファイルオープン時は排他制御
            const auto& tmp_path = QDir::tempPath();

            int index = 0;
            while (true) {
                const auto& file_path      = SettingCtrl::logFilePath(index);
                const auto& lock_file_path = tmp_path + QString("/Eyeris3DViewerLog_%1.lock").arg(index);
                QTextStream(stdout) << lock_file_path << Qt::endl;
                tryLock = std::make_shared<QLockFile>(lock_file_path);
                if (tryLock->tryLock()) {    /// プロセス間排他
                    outFile.setFileName(file_path);
                    if (outFile.open(QIODevice::WriteOnly)) {
                        break;    /// 開けたらループ終了
                    }
                    tryLock->unlock();
                    tryLock.reset();
                }
                else {
                    tryLock.reset();
                }
                /// 開けなかった場合はファイル名を変更
                ++index;
                /// indexが大きくなりすぎたらエラー回避
                if (index > 100) break;
            }
            first_open = false;
        }
        const auto& logStr = qFormatLogMessage(type, context, msg);
        if (outFile.isOpen()) {
            QTextStream ts(&outFile);
            ts << logStr << Qt::endl;
        }
        QTextStream(stdout) << logStr << Qt::endl;
    }

    if (type == QtMsgType::QtFatalMsg) abort();
}

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

#ifdef WITH_INSTALLER
    qSetMessagePattern("%{time yyyy/MM/dd hh:mm:ss} : [%{type}] %{message}");
    qInstallMessageHandler(MessageHandler);
#else
    qSetMessagePattern(
        "[%{time yyyy/MM/dd hh:mm:ss.zzz}] [pid=%{pid} tid=%{threadid}] [%{type}] [%{category}] %{message}");
#endif

    INFO() << "3D Viewer Start";

    /// 暫定: 元のGUI
    QMainWindow* main_window;
    auto         vox3dform = MainWindow3DViewer::createVox3DForm(main_window);
    ((QWidget*)main_window)->setWindowIcon(QIcon("://images/3DViewer.ico"));
    MainWindow3DViewer::showArguments(vox3dform);
    auto ret = a.exec();
    MainWindow3DViewer::deleteVox3DForm(vox3dform);

    INFO() << "3D Viewer End";

    return ret;
}
