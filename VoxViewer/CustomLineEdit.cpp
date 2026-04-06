#include "CustomLineEdit.h"
#include <QDir>
#include <QDirIterator>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QUrl>

#include "Vox3DForm.h"

void CustomLineEdit::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
    else if (event->mimeData()->hasText()) {
        event->acceptProposedAction();    // テキストも許可
    }
    else {
        event->ignore();
    }
}

void CustomLineEdit::dropEvent(QDropEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        auto urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            /// 拡張子指定なし / フォルダでない
            if (m_drag_drop_valid_exts.empty() && !m_drag_drop_folder) {
                QString filePath = urls.first().toLocalFile();    // 一つ目のファイルパス
                Vox3DForm::replaceShortCutPath(filePath);
                setTextAndCallback(filePath);
            }
            else {
                QString second_file;
                for (auto& url : urls) {
                    auto path = url.toLocalFile();
                    Vox3DForm::replaceShortCutPath(path);

                    QFileInfo file_info(path);

                    /// 拡張子指定ある場合
                    if (!m_drag_drop_valid_exts.empty()) {
                        if (m_drag_drop_valid_exts.count(file_info.suffix().toLower())) {
                            setTextAndCallback(path);
                            return;
                        }
                        if (second_file.isEmpty() && file_info.isDir()) {
                            QDirIterator it(path, QDir::Files, QDirIterator::NoIteratorFlags);
                            while (it.hasNext()) {
                                QString subFilePath = it.next();
                                if (m_drag_drop_valid_exts.count(QFileInfo(subFilePath).suffix().toLower())) {
                                    second_file = subFilePath;
                                    break;
                                }
                            }
                        }
                    }

                    /// フォルダの場合
                    if (m_drag_drop_folder) {
                        if (file_info.isDir()) {
                            setTextAndCallback(path);
                            return;
                        }
                        else if (second_file.isEmpty()) {
                            /// returnがあればそっち優先
                            second_file = file_info.absoluteDir().absolutePath();
                        }
                    }
                }

                if (!second_file.isEmpty()) {
                    setTextAndCallback(second_file);
                }
            }
        }
        event->acceptProposedAction();
    }
    else if (event->mimeData()->hasText()) {
        setTextAndCallback(event->mimeData()->text());
        event->acceptProposedAction();
    }
    else {
        event->ignore();
    }
}

void CustomLineEdit::setTextAndCallback(const QString& path)
{
    setText(path);
    if (dropCallback) {
        dropCallback(path);
    }
}
