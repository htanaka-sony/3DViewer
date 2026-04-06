#include "FileExplorerDialog.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QProcess>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTableView>
#include <QUrl>
#include <QVBoxLayout>
#include <algorithm>
#include <utility>

FileExplorerDialog::FileExplorerDialog(const QString& labelText, const QString& title, QWidget* parent)
    : QDialog(parent)
{
    if (title.isEmpty()) {
        if (parent) setWindowTitle(parent->windowTitle());
    }
    else {
        setWindowTitle(title);
    }
    QVBoxLayout* layout = new QVBoxLayout(this);

    // ラベル
    auto* label = new QLabel(labelText, this);
    layout->addWidget(label);

    // --- フォルダテーブル（2列：OK/NG と フォルダパス） ---
    folderModel = new QStandardItemModel(this);
    folderModel->setColumnCount(2);
    folderModel->setHeaderData(0, Qt::Horizontal, "Status");
    folderModel->setHeaderData(1, Qt::Horizontal, "Folder Path");

    folderTable = new QTableView(this);
    folderTable->setModel(folderModel);
    folderTable->horizontalHeader()->setVisible(true);
    folderTable->verticalHeader()->setVisible(true);
    folderTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    folderTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    folderTable->setShowGrid(true);
    folderTable->verticalHeader()->setDefaultSectionSize(18);
    folderTable->horizontalHeader()->setStretchLastSection(true);
    folderTable->setWordWrap(false);
    folderTable->setTextElideMode(Qt::ElideLeft);
    folderTable->setSortingEnabled(true);

    // Status列だけ最小幅
    folderTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    int minStatusWidth_folder = folderTable->columnWidth(0);
    folderTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    folderTable->setColumnWidth(0, minStatusWidth_folder);

    // --- ファイルテーブル（2列：OK/NG と ファイル名） ---
    fileModel = new QStandardItemModel(this);
    fileModel->setColumnCount(2);
    fileModel->setHeaderData(0, Qt::Horizontal, "Status");
    fileModel->setHeaderData(1, Qt::Horizontal, "File Name");

    fileTable = new QTableView(this);
    fileTable->setModel(fileModel);
    fileTable->horizontalHeader()->setVisible(true);
    fileTable->verticalHeader()->setVisible(true);
    fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    fileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    fileTable->setShowGrid(true);
    fileTable->verticalHeader()->setDefaultSectionSize(18);
    fileTable->horizontalHeader()->setStretchLastSection(true);
    fileTable->setWordWrap(false);
    fileTable->setTextElideMode(Qt::ElideLeft);
    fileTable->setSortingEnabled(true);

    // Status列だけ最小幅
    fileTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    int minStatusWidth = fileTable->columnWidth(0);
    fileTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    fileTable->setColumnWidth(0, minStatusWidth);

    fileTable->installEventFilter(this);
    folderTable->installEventFilter(this);

    // --- QSplitterで2ペイン ---
    QSplitter* splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(folderTable);
    splitter->addWidget(fileTable);
    layout->addWidget(splitter);

    // シグナル・スロット
    connect(folderTable, &QTableView::doubleClicked, this, &FileExplorerDialog::onFolderDoubleClicked);
    connect(fileTable, &QTableView::doubleClicked, this, &FileExplorerDialog::onFileDoubleClicked);
    connect(folderTable->selectionModel(), &QItemSelectionModel::currentChanged, this,
            &FileExplorerDialog::onFolderChanged);

    fileTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(fileTable, &QTableView::customContextMenuRequested, this, &FileExplorerDialog::showFileContextMenu);

    folderTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(folderTable, &QTableView::customContextMenuRequested, this, &FileExplorerDialog::showFolderContextMenu);

    // Closeボタン
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttonBox, &QDialogButtonBox::clicked, this, &QDialog::reject);
    layout->addWidget(buttonBox);

    this->resize(500, 550);
    splitter->setSizes({200, 400});
}

void FileExplorerDialog::setFolderFileMap(const QMap<QString, QList<std::pair<QString, bool>>>& folderFileMap)
{
    this->folderFileMap = folderFileMap;

    bool anyError = false;

    folderModel->removeRows(0, folderModel->rowCount());
    for (const QString& folder : folderFileMap.keys()) {
        const QList<std::pair<QString, bool>>& fileEntries = folderFileMap.value(folder);

        // そのフォルダ内にエラー
        bool hasError = std::any_of(fileEntries.begin(), fileEntries.end(),
                                    [](const std::pair<QString, bool>& entry) { return !entry.second; });
        if (hasError) anyError = true;

        QList<QStandardItem*> row;
        QStandardItem*        statusItem = new QStandardItem(hasError ? "Failed" : "");
        if (hasError) {
            statusItem->setBackground(QColor(255, 210, 210));
            statusItem->setForeground(Qt::black);
        }
        else {
            statusItem->setBackground(Qt::white);
            statusItem->setForeground(Qt::black);
        }
        row << statusItem;

        QStandardItem* folderItem = new QStandardItem(folder);
        if (hasError) {
            folderItem->setBackground(QColor(255, 210, 210));
            folderItem->setForeground(Qt::black);
        }
        else {
            folderItem->setBackground(Qt::white);
            folderItem->setForeground(Qt::black);
        }
        row << folderItem;

        folderModel->appendRow(row);
    }

    folderTable->scrollToTop();
    fileTable->scrollToTop();

    // 全体で1つでもエラーがあればStatus列ON、なければOFF
    folderTable->setColumnHidden(0, !anyError);
    fileTable->setColumnHidden(0, !anyError);

    folderTable->sortByColumn(1, Qt::AscendingOrder);

    if (folderModel->rowCount() > 0) {
        folderTable->selectRow(0);
        onFolderChanged(folderModel->index(0, 1), QModelIndex());
    }
    else {
        fileModel->removeRows(0, fileModel->rowCount());
    }
}

void FileExplorerDialog::onFolderDoubleClicked(const QModelIndex& index)
{
    QString folderPath = folderModel->item(index.row(), 1)->text();
    QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
}

void FileExplorerDialog::onFileDoubleClicked(const QModelIndex& index)
{
    if (index.row() < 0) return;
    int folderRow = folderTable->currentIndex().row();
    if (folderRow < 0) return;
    QString folderName = folderModel->item(folderRow, 1)->text();
    QString fileName   = fileModel->item(index.row(), 1)->text();    // File Name列
    QString filePath   = folderName + QDir::separator() + fileName;
    if (QFileInfo::exists(filePath)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    }
}

void FileExplorerDialog::onFolderChanged(const QModelIndex& current, const QModelIndex&)
{
    fileModel->removeRows(0, fileModel->rowCount());
    if (!current.isValid()) return;
    QString folderName = folderModel->item(current.row(), 1)->text();

    bool anyError = false;

    const QList<std::pair<QString, bool>>& fileEntries = folderFileMap.value(folderName);
    for (const auto& entry : fileEntries) {
        const QString&        path    = entry.first;
        bool                  success = entry.second;
        QList<QStandardItem*> row;
        // Status列
        QStandardItem* statusItem = new QStandardItem(success ? "" : "Failed");
        if (!success) {
            statusItem->setBackground(QColor(255, 210, 210));
            statusItem->setForeground(Qt::black);
            anyError = true;
        }
        else {
            statusItem->setBackground(Qt::white);
            statusItem->setForeground(Qt::black);
        }
        row << statusItem;

        // File Name列
        QStandardItem* nameItem = new QStandardItem(QFileInfo(path).fileName());
        if (!success) {
            nameItem->setBackground(QColor(255, 210, 210));
            nameItem->setForeground(Qt::black);
        }
        else {
            nameItem->setBackground(Qt::white);
            nameItem->setForeground(Qt::black);
        }
        row << nameItem;

        fileModel->appendRow(row);
    }
    // fileTable->setColumnHidden(0, !anyError);
    fileTable->sortByColumn(1, Qt::AscendingOrder);
}

void FileExplorerDialog::showFileContextMenu(const QPoint& pos)
{
    QModelIndex index = fileTable->indexAt(pos);
    if (index.isValid()) {
        QMenu    contextMenu;
        QAction* openInExplorerAction = contextMenu.addAction("Open in Explorer");
        connect(openInExplorerAction, &QAction::triggered, this, [this, index]() {
            if (index.row() < 0) return;
            int folderRow = folderTable->currentIndex().row();
            if (folderRow < 0) return;
            QString folderName = folderModel->item(folderRow, 1)->text();
            QString fileName   = fileModel->item(index.row(), 1)->text();
            QString filePath   = folderName + QDir::separator() + fileName;
            if (QFileInfo::exists(filePath)) {
                openInExplorer(filePath);
            }
        });
        QAction* openAction = contextMenu.addAction("Open in App");
        connect(openAction, &QAction::triggered, this, [this, index]() { onFileDoubleClicked(index); });
        contextMenu.exec(fileTable->viewport()->mapToGlobal(pos));
    }
}

void FileExplorerDialog::showFolderContextMenu(const QPoint& pos)
{
    QModelIndex index = folderTable->indexAt(pos);
    if (index.isValid()) {
        QMenu    contextMenu;
        QAction* openAction = contextMenu.addAction("Open");
        connect(openAction, &QAction::triggered, this, [this, index]() { onFolderDoubleClicked(index); });
        contextMenu.exec(folderTable->viewport()->mapToGlobal(pos));
    }
}

void FileExplorerDialog::openInExplorer(const QString& filePath)
{
#ifdef Q_OS_WIN
    QString     nativePath = QDir::toNativeSeparators(filePath);
    QStringList args;
    args << "/select," << nativePath;
    QProcess::startDetached("explorer.exe", args);
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filePath).absolutePath()));
#endif
}

bool FileExplorerDialog::eventFilter(QObject* obj, QEvent* event)
{
    QTableView*         targetTable = nullptr;
    QStandardItemModel* targetModel = nullptr;

    if (obj == fileTable) {
        targetTable = fileTable;
        targetModel = fileModel;
    }
    else if (obj == folderTable) {
        targetTable = folderTable;
        targetModel = folderModel;
    }
    else {
        return QDialog::eventFilter(obj, event);
    }

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->matches(QKeySequence::Copy)) {
            QString clipboardText;

            auto selModel    = targetTable->selectionModel();
            auto selBehavior = targetTable->selectionBehavior();

            if (selBehavior == QAbstractItemView::SelectRows) {
                // 行選択モードの場合
                QModelIndexList selRows = selModel->selectedRows();
                std::sort(selRows.begin(), selRows.end(),
                          [](const QModelIndex& a, const QModelIndex& b) { return a.row() < b.row(); });

                QStringList lines;
                for (const QModelIndex& idx : selRows) {
                    QStringList columns;
                    for (int col = 0; col < targetModel->columnCount(); ++col) {
                        if (targetTable->isColumnHidden(col)) continue;    // ← 非表示列スキップ
                        columns << targetModel->item(idx.row(), col)->text();
                    }
                    lines << columns.join('\t');
                }
                clipboardText = lines.join('\n');
            }
            else {
                // セル選択モードの場合
                QModelIndexList selCells = selModel->selectedIndexes();
                // 行・列順で並べ替え（Excel互換）
                std::sort(selCells.begin(), selCells.end(), [](const QModelIndex& a, const QModelIndex& b) {
                    if (a.row() == b.row()) return a.column() < b.column();
                    return a.row() < b.row();
                });
                int         prevRow = -1;
                QStringList lines;
                QStringList columns;
                for (const QModelIndex& idx : selCells) {
                    if (targetTable->isColumnHidden(idx.column())) continue;    // ← 非表示セルスキップ
                    if (prevRow != -1 && idx.row() != prevRow) {
                        lines << columns.join('\t');
                        columns.clear();
                    }
                    columns << targetModel->item(idx.row(), idx.column())->text();
                    prevRow = idx.row();
                }
                if (!columns.isEmpty()) lines << columns.join('\t');
                clipboardText = lines.join('\n');
            }

            QApplication::clipboard()->setText(clipboardText);
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}
