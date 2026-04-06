#ifndef CSVUTIL_H_
#define CSVUTIL_H_

#include <QObject>
#include <QStringList>
#include "VoxViewerGlobal.h"

namespace CsvUtil {

// CSVファイル読込
bool VOXVIEWER_EXPORT readCsv(const QString& strCsvFilePath, QList<QStringList>& lCsvData);

// 1行をCSVセルに分割
void VOXVIEWER_EXPORT splitLineToCsvCells(const QString& strLine, QStringList& lCsvCells);

// 全文字列データ（改行込み）をCSVデータ（セルリストのリスト）に変換
void VOXVIEWER_EXPORT convertStringToCsvData(const QString& strData, QList<QStringList>& lCsvData);

// CSVファイル書込
bool VOXVIEWER_EXPORT writeCsv(const QString& strCsvFilePath, const QList<QStringList>& lCsvData);

// ダブルクオートチェックを無くしたゆるいversion
QList<QStringList> VOXVIEWER_EXPORT readCsvLoose(const QString& csvPath);

}    // namespace CsvUtil

#endif    // CSVUTIL_H_
