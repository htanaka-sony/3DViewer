#ifndef FILE_EXPLORER_DIALOG_H
#define FILE_EXPLORER_DIALOG_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QMap>
#include <QStandardItemModel>
#include <QString>
#include <QTableView>
#include <utility>

class FileExplorerDialog : public QDialog {
    Q_OBJECT
public:
    explicit FileExplorerDialog(const QString& labelText, const QString& title = QString(), QWidget* parent = nullptr);

    // フォルダとファイル一覧＋成功フラグ（Pair形式）をセット
    void setFolderFileMap(const QMap<QString, QList<std::pair<QString, bool>>>& folderFileMap);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onFolderDoubleClicked(const QModelIndex& index);
    void onFileDoubleClicked(const QModelIndex& index);
    void onFolderChanged(const QModelIndex& current, const QModelIndex&);
    void showFileContextMenu(const QPoint& pos);
    void showFolderContextMenu(const QPoint& pos);

private:
    void openInExplorer(const QString& filePath);

    QStandardItemModel*                            folderModel;
    QStandardItemModel*                            fileModel;
    QTableView*                                    folderTable;
    QTableView*                                    fileTable;
    QMap<QString, QList<std::pair<QString, bool>>> folderFileMap;
};

#endif
