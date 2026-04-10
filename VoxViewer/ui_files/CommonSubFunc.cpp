#include "CommonSubFunc.h"
#include "ui_CommonSubFunc.h"

#include <QColor>
#include <QDebug>

CommonSubFunc::CommonSubFunc(QWidget* parent) : QWidget(parent), ui(new Ui::CommonSubFunc)
{
    ui->setupUi(this);
}

CommonSubFunc::~CommonSubFunc()
{
    delete ui;
}
// g_hash_officialColor キー:マテリアル名　   値：色名 green　など
QHash<QString, QString> CommonSubFunc::func_makehash_matnameToCname()    // ハッシュ作成して返り値とする
                                                                         // キー:マテリアル名　   値：色名 green　など
{
    static QHash<QString, QString> g_matname_to_cname_hash;
    if (g_matname_to_cname_hash.isEmpty()) {
        QHash<QString, QString> matnameToCnameHash;

        // 下記デフォルト値まで定義するもの。　実際は 別処理後、下記ファイル存在する時点で　matnameを読込直しする。
        // 　"C:/temp/FDTDsolver_work/FTP_OfficialMaterialList/List_OfficialMaterials.txt";

        matnameToCnameHash["Air"]                  = "Lightcyan";
        matnameToCnameHash["AlCu"]                 = "Navy";
        matnameToCnameHash["Al"]                   = "Navy";
        matnameToCnameHash["CF-9th-B_Woollam"]     = "Blue";
        matnameToCnameHash["CF-9th-G_Woollam"]     = "Green";
        matnameToCnameHash["CF-9th-R_Woollam"]     = "Red";
        matnameToCnameHash["CF-Cinema-B_SCK"]      = "Blue";
        matnameToCnameHash["CF-Cinema-G_SCK"]      = "Green";
        matnameToCnameHash["CF-Cinema-R_SCK"]      = "Red";
        matnameToCnameHash["CF-SIR_SCK"]           = "Pink";
        matnameToCnameHash["CF-SK9000C-Black_SCK"] = "Black";
        matnameToCnameHash["CT_Woollam"]           = "White";
        matnameToCnameHash["Cu"]                   = "Yellow";
        matnameToCnameHash["LDR_TEOS_SCK"]         = "Lightgray";
        matnameToCnameHash["LP-SiN_SCK"]           = "Magenta";
        matnameToCnameHash["LT-SiN_SCK"]           = "Purple";
        matnameToCnameHash["LTO_SCK"]              = "Gold";
        matnameToCnameHash["P-SiN_Woollam"]        = "Cyan";
        matnameToCnameHash["P-SiON_SCK"]           = "Magenta";
        matnameToCnameHash["Poly"]                 = "Greenyellow";
        matnameToCnameHash["SCF-AH_SCK"]           = "Lightcyan";
        matnameToCnameHash["SCF-AO_SCK"]           = "Pink";
        matnameToCnameHash["SCF-PT_SCK"]           = "White";
        matnameToCnameHash["SRO_SCK"]              = "Olive";
        matnameToCnameHash["STSR_Woollam"]         = "Cyan";
        matnameToCnameHash["SiCN_SCK"]             = "Cyan";
        matnameToCnameHash["SiO2_SCK"]             = "Gray";
        matnameToCnameHash["Si"]                   = "Black";
        matnameToCnameHash["TaO_ATG"]              = "Purple";
        matnameToCnameHash["Ta"]                   = "Olive";
        matnameToCnameHash["Ti"]                   = "Orange";
        matnameToCnameHash["UV-SiN_Woollam"]       = "Pink";
        matnameToCnameHash["W"]                    = "Brown";
        matnameToCnameHash["CF-10th-B_SCK"]        = "Blue";
        matnameToCnameHash["CF-10th-G_SCK"]        = "Green";
        matnameToCnameHash["CF-6th-B_SCK"]         = "Blue";
        matnameToCnameHash["CF-6th-G_SCK"]         = "Green";
        matnameToCnameHash["CF-6th-R_SCK"]         = "Red";
        matnameToCnameHash["CF-8kai-B_SCK"]        = "Blue";
        matnameToCnameHash["CF-8kai-G_SCK"]        = "Green";
        matnameToCnameHash["CF-8kai-R_SCK"]        = "Red";
        matnameToCnameHash["CF-Cyan_SCK"]          = "Lightcyan";
        matnameToCnameHash["CF-Real-B_SCK"]        = "Blue";
        matnameToCnameHash["CF-Real-G_SCK"]        = "Green";
        matnameToCnameHash["CF-Real-R_SCK"]        = "Red";
        matnameToCnameHash["M1C-PSIO_SCK"]         = "Olive";
        matnameToCnameHash["TiN_ATG"]              = "Purple";
        matnameToCnameHash["W_SCK"]                = "Brown";

        for (auto it = matnameToCnameHash.begin(); it != matnameToCnameHash.end(); ++it) {
            g_matname_to_cname_hash[it.key()]               = it.value();
            g_matname_to_cname_hash[it.key() + "[NoImp:0]"] = it.value();
        }
    }

    return (g_matname_to_cname_hash);
}

QHash<QString, QColor>
CommonSubFunc::func_makehash_cnameToQtColor()    // ハッシュ作成して返り値とする  キー:色名(greenなど)　値：Qcolor
                                                 // 例：QColor(0, 128, 0, 255)
{
    QHash<QString, QColor> cnameToQtColorHash;

    cnameToQtColorHash["Black"]        = QColor(0, 0, 0, 255);
    cnameToQtColorHash["Gray"]         = QColor(128, 128, 128, 255);
    cnameToQtColorHash["Lightgray"]    = QColor(211, 211, 211, 255);
    cnameToQtColorHash["White"]        = QColor(255, 255, 255, 255);
    cnameToQtColorHash["Brown"]        = QColor(165, 42, 42, 255);
    cnameToQtColorHash["Purple"]       = QColor(128, 0, 128, 255);
    cnameToQtColorHash["MediumPurple"] = QColor(147, 112, 219, 255);
    cnameToQtColorHash["Red"]          = QColor(255, 0, 0, 255);
    cnameToQtColorHash["Magenta"]      = QColor(255, 0, 255, 255);
    cnameToQtColorHash["Pink"]         = QColor(255, 192, 203, 255);
    cnameToQtColorHash["Orange"]       = QColor(225, 165, 0, 255);
    cnameToQtColorHash["Gold"]         = QColor(255, 215, 0, 255);
    cnameToQtColorHash["Yellow"]       = QColor(255, 255, 0, 255);
    cnameToQtColorHash["Green"]        = QColor(0, 128, 0, 255);
    cnameToQtColorHash["Greenyellow"]  = QColor(173, 255, 47, 255);
    cnameToQtColorHash["Olive"]        = QColor(128, 128, 0, 255);
    cnameToQtColorHash["Navy"]         = QColor(0, 0, 128, 255);
    cnameToQtColorHash["Blue"]         = QColor(0, 0, 255, 255);
    cnameToQtColorHash["Cyan"]         = QColor(0, 255, 255, 255);
    cnameToQtColorHash["Lightcyan"]    = QColor(224, 255, 255, 255);

    return (cnameToQtColorHash);
}

QHash<QString, QString>
CommonSubFunc::func_makehash_cnameToStyleColor()    // ハッシュ作成して返り値とする キー:色名(greenなど)　値:QString型：
                                                    // 例：#008000
{
    QHash<QString, QString> cnameToStyleColorHash;

    cnameToStyleColorHash["Black"]        = "#000000";
    cnameToStyleColorHash["Gray"]         = "#808080";
    cnameToStyleColorHash["Lightgray"]    = "#d3d3d3";
    cnameToStyleColorHash["White"]        = "#ffffff";
    cnameToStyleColorHash["Brown"]        = "#a52a2a";
    cnameToStyleColorHash["Purple"]       = "#800080";
    cnameToStyleColorHash["MediumPurple"] = "#9370db";
    cnameToStyleColorHash["Red"]          = "#ff0000";
    cnameToStyleColorHash["Magenta"]      = "#ff00ff";
    cnameToStyleColorHash["Pink"]         = "#ffc0cb";
    cnameToStyleColorHash["Orange"]       = "#ffa500";
    cnameToStyleColorHash["Gold"]         = "#ffd700";
    cnameToStyleColorHash["Yellow"]       = "#ffff00";
    cnameToStyleColorHash["Green"]        = "#008000";
    cnameToStyleColorHash["Greenyellow"]  = "#adff2f";
    cnameToStyleColorHash["Olive"]        = "#808000";
    cnameToStyleColorHash["Navy"]         = "#000080";
    cnameToStyleColorHash["Blue"]         = "#0000ff";
    cnameToStyleColorHash["Cyan"]         = "#00ffff";
    cnameToStyleColorHash["Lightcyan"]    = "#e0ffff";

    return (cnameToStyleColorHash);
}

QStringList
CommonSubFunc::func_makeList_whitefontBgColor()    // リスト作成して返り値とする
                                                   // GUI表示での背景色リストの内、文字色(fontcolor)を白にするもの。
                                                   // 背景濃い色だと、通常フォント色黒で見えづらいので
{
    QStringList whitefontBgColorList;
    whitefontBgColorList << "Black"
                         << "Gray"
                         << "Brown"
                         << "Purple"
                         << "MediumPurple"
                         << "Green"
                         << "Olive"
                         << "Navy"
                         << "Blue";
    return (whitefontBgColorList);
}

QHash<QString, QList<float>>
CommonSubFunc::func_makehash_cnameToRGBColor()    // ハッシュ作成して返り値とする　キー：色名(Greenなど)　値：QString型
                                                  // (例 #008000)
{
    // 値はQVector3Dにしたいが文法上ハッシュには入れられないようなので、QListを値にする。
    // func_makehash_cnameToRGBColor()の呼び出しもとに返してからQList→QVector3Dに入れなおして使うようにする。
    QHash<QString, QList<float>> cnameToRGBColorHash;

    cnameToRGBColorHash["Black"]     = {0, 0, 0};
    cnameToRGBColorHash["Gray"]      = {0.5, 0.5, 0.5};
    cnameToRGBColorHash["DarkGray"]  = {0.18, 0.18, 0.18};
    cnameToRGBColorHash["Lightgray"] = {0.82, 0.82, 0.82};
    cnameToRGBColorHash["White"]     = {1, 1, 1};

    cnameToRGBColorHash["Brown"]        = {0.64, 0.16, 0.16};
    cnameToRGBColorHash["Purple"]       = {0.5, 0, 0.5};
    cnameToRGBColorHash["MediumPurple"] = {0.57, 0.43, 0.85};
    cnameToRGBColorHash["Red"]          = {1, 0, 0};
    cnameToRGBColorHash["Magenta"]      = {1, 0, 1};
    cnameToRGBColorHash["Pink"]         = {1, 0.75, 0.8};
    cnameToRGBColorHash["Orange"]       = {0.88, 0.65, 0};
    cnameToRGBColorHash["Gold"]         = {1, 0.84, 0};
    cnameToRGBColorHash["Yellow"]       = {1, 1, 0};

    cnameToRGBColorHash["Green"]       = {0, 0.5, 0};
    cnameToRGBColorHash["Greenyellow"] = {0.67, 1, 0.18};
    cnameToRGBColorHash["Olive"]       = {0.5, 0.5, 0};
    cnameToRGBColorHash["Navy"]        = {0, 0, 0.5};
    cnameToRGBColorHash["Blue"]        = {0, 0, 1};
    cnameToRGBColorHash["Cyan"]        = {0, 1, 1};
    cnameToRGBColorHash["Lightcyan"]   = {0.87, 1, 1};

    return (cnameToRGBColorHash);
}

// 2022.04.27-01 fdtdファイルのヘッダー箇所の情報取得
QHash<QString, QString> CommonSubFunc::func_fdtd_readHeader(QString in_fdtdpath)
{
    QHash<QString, QString> return_hash;
    QString                 filePath1 = in_fdtdpath;

    QFile infile1(filePath1);
    if (!infile1.open(QIODevice::ReadOnly)) {
        QMessageBox::information(this, "can't open", "can't open " + filePath1);
        return (return_hash);
    }
    QTextStream in1(&infile1);
    while (!in1.atEnd()) {
        QString fileLine = in1.readLine(0);
        if (fileLine.startsWith("Type = ")) {
            return_hash["Type"] = fileLine.replace("Type = ", "");
            continue;
        }
        else if (fileLine.startsWith("CellX = ")) {
            return_hash["CellX"] = fileLine.replace("CellX = ", "");
            continue;
        }
        else if (fileLine.startsWith("CellY = ")) {
            return_hash["CellY"] = fileLine.replace("CellY = ", "");
            continue;
        }
        else if (fileLine.startsWith("ArrayX = ")) {
            return_hash["ArrayX"] = fileLine.replace("ArrayX = ", "");
            continue;
        }
        else if (fileLine.startsWith("ArrayY = ")) {
            return_hash["ArrayY"] = fileLine.replace("ArrayY = ", "");
            break;
        }
    }

    return (return_hash);
}
