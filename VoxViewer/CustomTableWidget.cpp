#define _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING
#include "CustomTableWidget.h"
#include <private/qtablewidget_p.h>

#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QSpinBox>

#include "CommonSubClass.h"

#include "CoreDebug.h"

#include "Vox3DForm.h"

const QEditorInfo& editorForIndex(const QIndexEditorHash& indexEditorHash, const QModelIndex& index)
{
    static QEditorInfo nullInfo;

    // do not try to search to avoid slow implicit cast from QModelIndex to QPersistentModelIndex
    if (indexEditorHash.isEmpty()) return nullInfo;

    QIndexEditorHash::const_iterator it = indexEditorHash.find(index);
    if (it == indexEditorHash.end()) return nullInfo;

    return it.value();
}

CustomTableWidget::CustomTableWidget(QWidget* parent) : QTableWidget(parent) {}

CustomTableWidget::CustomTableWidget(int rows, int columns, QWidget* parent) : QTableWidget(rows, columns, parent) {}

bool CustomTableWidget::edit(const QModelIndex& index, EditTrigger trigger, QEvent* event)
{
    if (m_editing) {
        return false;
    }

    Q_D(QTableWidget);

    if (!d->isIndexValid(index)) return false;

    if (QWidget* w = (d->persistent.isEmpty() ? static_cast<QWidget*>(nullptr)
                                              : editorForIndex(d->indexEditorHash, index).widget.data())) {
        if (w->focusPolicy() == Qt::NoFocus) return false;
        if (!d->waitForIMCommit) {
            /// setFocusでさらにedit呼ばれて落ちる場合があるので対策
            m_editing = true;
            w->setFocus();
            m_editing = false;
        }
        else {
            updateMicroFocus();
        }
        return true;
    }

    return QTableWidget::edit(index, trigger, event);
}

void CustomTableWidget::mousePressEvent(QMouseEvent* event)
{
    /// 未選択状態から行以外クリックしたときに、前回の行内のアイテムのフォーカスが復活する
    /// それを消したいとき
    /// 　※現状特に害がないのでそのまま（寸法名編集が該当したが、ダブルクリックで編集にしたので特に問題ない。ダブルクリックなしで編集可にするなら入れた方がいい）
    /*
    QTableWidgetItem* item = itemAt(event->pos());
    if (!item) {
        // セル以外（空白やヘッダー）をクリックした場合
        if (currentItem()) {
            closePersistentEditor(currentItem());
        }
        clearFocus();
    }
    */
    // 通常の処理も呼ぶ
    QTableWidget::mousePressEvent(event);
}

void CustomTableWidget::keyPressEvent(QKeyEvent* event)
{
    if (m_enable_copy && event->matches(QKeySequence::Copy)) {
        /// 実装
        std::map<int, std::set<int>> mapSelectIndexes;
        selectIndexMap(mapSelectIndexes);
        if (mapSelectIndexes.empty()) return;

        QString    str       = tableString(mapSelectIndexes);
        const auto clipboard = QApplication::clipboard();
        clipboard->setText(str);

        return;
    }
    else if (m_enable_paste && event->matches(QKeySequence::Paste)) {
        /// 未実装
    }

    QTableWidget::keyPressEvent(event);
}

void CustomTableWidget::copy(bool all)
{
    if (all) {
        std::map<int, std::set<int>> mapSelectIndexes;

        for (int ic = 0; ic < rowCount(); ++ic) {
            auto& set_column = mapSelectIndexes[ic];
            for (int jc = 0; jc < columnCount(); ++jc) {
                set_column.insert(jc);
            }
        }

        QString    str       = tableString(mapSelectIndexes);
        const auto clipboard = QApplication::clipboard();
        clipboard->setText(str);
    }
    else {
        std::map<int, std::set<int>> mapSelectIndexes;
        selectIndexMap(mapSelectIndexes);
        if (mapSelectIndexes.empty()) return;

        QString    str       = tableString(mapSelectIndexes);
        const auto clipboard = QApplication::clipboard();
        clipboard->setText(str);
    }
}

void CustomTableWidget::copy(const std::map<int, std::set<int>>& mapSelectIndexes)
{
    QString    str       = tableString(mapSelectIndexes);
    const auto clipboard = QApplication::clipboard();
    clipboard->setText(str);
}

void CustomTableWidget::selectIndexMap(std::map<int, std::set<int>>& mapSelectIndexes)
{
    mapSelectIndexes.clear();

    if (QItemSelectionModel* selModel = selectionModel()) {
        const auto& selectIndexes = selModel->selectedIndexes();
        for (auto&& idx : selectIndexes) {
            const auto& nRow    = idx.row();
            const auto& nColumn = idx.column();

            mapSelectIndexes[nRow].insert(nColumn);
        }
    }

    /// 列ヘッダ選択
    /*
    if (horizontalHeader()) {
        QItemSelectionModel* headerSelModel = horizontalHeader()->selectionModel();
        if (headerSelModel) {
            const auto& headerIndexes = headerSelModel->selectedIndexes();
            for (const auto& idx : headerIndexes) {
                int nColumn = idx.column();
                mapSelectIndexes[-1].insert(nColumn);    /// row = -1 で列ヘッダを表現
            }
        }
    }*/
}

QString CustomTableWidget::tableString(const std::map<int, std::set<int>>& mapSelectIndexes)
{
    QList<QList<QString>> all_texts;

    for (auto&& [row, col_set] : mapSelectIndexes) {
        all_texts.emplaceBack(QList<QString>());

        auto& line = all_texts[all_texts.size() - 1];

        for (auto&& col : col_set) {
            QString str = cellText(row, col);

            str.remove('"');
            str = str.trimmed();

            line.emplaceBack(str);
        }
    }

    QList<int> space_adjust;
    int        max_col = 0;
    for (int ic = 0; ic < all_texts.size(); ++ic) {
        auto& line = all_texts[ic];
        if (line.size() > max_col) {
            max_col = line.size();
        }
    }

    if (m_add_space_copy) {
        for (int jc = 0; jc < max_col; ++jc) {    /// 列
            int max_length = 0;

            for (int ic = 0; ic < all_texts.size(); ++ic) {
                auto& line = all_texts[ic];
                if (line.size() > jc) {
                    if (max_length < line[jc].length()) {
                        max_length = line[jc].length();
                    }
                }
            }

            space_adjust.emplaceBack(max_length + 1);
        }
    }

    QString str_all_data;
    for (int ic = 0; ic < all_texts.size(); ++ic) {
        auto& line = all_texts[ic];
        for (int jc = 0; jc < line.size(); ++jc) {
            str_all_data += line[jc];

            if (jc < max_col - 1) {
                if (space_adjust.size() > jc) {
                    int count = space_adjust[jc] - line[jc].length();
                    str_all_data += QString(" ").repeated(count);
                }
                str_all_data += '\t';
            }
        }

        if (ic < all_texts.size() - 1) {
            str_all_data += '\n';
        }
    }

    return str_all_data;
}

QString CustomTableWidget::cellText(int row, int col)
{
    QString str;
    if (row == -1) {
        /// 列ヘッダ
        QTableWidgetItem* headerItem = horizontalHeaderItem(col);
        if (headerItem) {
            str = headerItem->text();
        }
    }
    else {
        QWidget* cell_widget = cellWidget(row, col);
        if (cell_widget) {
            bool cell_str = false;
            if (auto lineEdit = qobject_cast<QLineEdit*>(cell_widget)) {
                str      = lineEdit->text();
                cell_str = true;
            }
            else if (auto comboBox = qobject_cast<QComboBox*>(cell_widget)) {
                str      = comboBox->currentText();
                cell_str = true;
            }
            else if (auto comboBox = qobject_cast<CustomCombo*>(cell_widget)) {
                str      = comboBox->currentText();
                cell_str = true;
            }
            else if (auto comboBox = qobject_cast<CustomColorCombo*>(cell_widget)) {
                str      = comboBox->currentText();
                cell_str = true;
            }
            else if (auto checkBox = qobject_cast<QCheckBox*>(cell_widget)) {
                str      = checkBox->isChecked() ? "1" : "0";
                cell_str = true;
            }
            else if (auto spin_box = qobject_cast<QSpinBox*>(cell_widget)) {
                str      = QString::number(spin_box->value());
                cell_str = true;
            }
            else if (auto double_spin_box = qobject_cast<QDoubleSpinBox*>(cell_widget)) {
                str      = QString::number(double_spin_box->value());
                cell_str = true;
            }
            else {
                QCheckBox* check_box = cell_widget->findChild<QCheckBox*>();
                if (check_box) {
                    str      = check_box->isChecked() ? "1" : "0";
                    cell_str = true;
                }
            }

            if (!cell_str) {
                QTableWidgetItem* pItem = item(row, col);
                if (pItem) {
                    str = pItem->text();
                }
            }
        }
        else {
            QTableWidgetItem* pItem = item(row, col);
            if (pItem) {
                str = pItem->text();
            }
        }
    }

    return str;
}

void CustomTableWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
    else {
        event->ignore();
    }
}
void CustomTableWidget::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
    else {
        event->ignore();
    }
}

void CustomTableWidget::dropEvent(QDropEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        QStringList pathList;
        QList<QUrl> urlList = event->mimeData()->urls();
        for (const QUrl& url : urlList) {
            QString filePath = url.toLocalFile();
            Vox3DForm::replaceShortCutPath(filePath);
            if (!filePath.isEmpty()) pathList << filePath;
        }
        // コールバックに渡す
        if (dropCallback) {
            dropCallback(pathList);
        }
        event->acceptProposedAction();
    }
    else {
        event->ignore();
    }
}
