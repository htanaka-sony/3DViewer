#ifndef COMMONSUBFUNC_H
#define COMMONSUBFUNC_H

#include <QFile>
#include <QHash>
#include <QMessageBox>
#include <QWidget>

namespace Ui {
class CommonSubFunc;
}

class CommonSubFunc : public QWidget {
    Q_OBJECT

public:
    explicit CommonSubFunc(QWidget* parent = nullptr);
    ~CommonSubFunc();

    // 色表示
    QHash<QString, QString>
    func_makehash_matnameToCname();    // ハッシュ作成して返り値とする  キー:マテリアル名　   値：色名 green　など
    QHash<QString, QColor>
    func_makehash_cnameToQtColor();    // ハッシュ作成して返り値とする  キー:色名(greenなど)　値：Qcolor 例：QColor(0,
                                       // 128, 0, 255)
    QHash<QString, QString> func_makehash_cnameToStyleColor();    // ハッシュ作成して返り値とする
                                                                  // キー:色名(greenなど)　値： 例：String型 #008000
    QStringList func_makeList_whitefontBgColor();    // リスト作成して返り値とする
                                                     // GUI表示での背景色リストの内、文字色(fontcolor)を白にするもの。
                                                     // 背景濃い色だと、通常フォント色黒で見えづらいので
    QHash<QString, QList<float>>
    func_makehash_cnameToRGBColor();    // ハッシュ作成して返り値とする　キー：色名(Greenなど)　値：QString型
                                        // (例 #008000)
    // 2022.04.27-01 fdtdファイルのヘッダー箇所の情報取得
    QHash<QString, QString> func_fdtd_readHeader(QString in_fdtdpath);

private:
    Ui::CommonSubFunc* ui;
};

#endif    // COMMONSUBFUNC_H
