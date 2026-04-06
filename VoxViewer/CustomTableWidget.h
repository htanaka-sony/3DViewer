#ifndef CUSTOMTABLEWIDGET_H
#define CUSTOMTABLEWIDGET_H

#include <QMouseEvent>
#include <QTableWidget>

#include <map>
#include <set>

/// QtのQTableWidgetの不具合対策
/// TableWidgetでテキスト編集中に、特に日本語入力のIMEが出る状態でフォーカス外れたりすると落ちる

class CustomTableWidget : public QTableWidget {
    Q_OBJECT
public:
    explicit CustomTableWidget(QWidget* parent = nullptr);

    CustomTableWidget(int rows, int columns, QWidget* parent = nullptr);

    bool edit(const QModelIndex& index, EditTrigger trigger, QEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;

    void keyPressEvent(QKeyEvent* event) override;

    void copy(bool all);

    void copy(const std::map<int, std::set<int>>& mapSelectIndexes);

    void setEnableCopy(bool enable) { m_enable_copy = enable; }

    void setEnablePaste(bool enable) { m_enable_paste = enable; }

    void setAddSpaceCopy(bool add_space) { m_add_space_copy = add_space; }

    /// コールバック関数型: void(const QStringList&)
    using DropCallback = std::function<void(const QStringList&)>;

    void setDropCallback(DropCallback cb) { dropCallback = cb; }

protected:
    void    selectIndexMap(std::map<int, std::set<int>>& mapSelectIndexes);
    QString tableString(const std::map<int, std::set<int>>& mapSelectIndexes);
    QString cellText(int row, int col);

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    Q_DECLARE_PRIVATE(QTableWidget)

    /// Qt不具合対策
    bool m_editing = false;

    /// 動作
    bool m_enable_copy    = true;
    bool m_add_space_copy = true;

    bool m_enable_paste = false;

    DropCallback dropCallback = nullptr;
};

#endif    // CUSTOMTABLEWIDGET_H
