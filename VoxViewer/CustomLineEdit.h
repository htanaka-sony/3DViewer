#ifndef CUSTOMLINEEDIT_H
#define CUSTOMLINEEDIT_H

#include <QLineEdit>

#include <set>

class CustomLineEdit : public QLineEdit {
    Q_OBJECT
public:
    using QLineEdit::QLineEdit;    // 親のコンストラクタ継承

    void setDragDropFoloder() { m_drag_drop_folder = true; }
    void setDragDropFileExt(const std::set<QString>& valid_ext) { m_drag_drop_valid_exts = valid_ext; }

    /// コールバック関数型: void(const QStringList&)
    using DropCallback = std::function<void(const QString&)>;
    void setDropCallback(DropCallback cb) { dropCallback = cb; }

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    void setTextAndCallback(const QString& path);

    bool              m_drag_drop_folder = false;
    std::set<QString> m_drag_drop_valid_exts;

    DropCallback dropCallback = nullptr;
};

#endif    // CUSTOMLINEEDIT_H
