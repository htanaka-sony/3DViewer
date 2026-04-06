#include "CsvUtil.h"

#include <QFile>
#include <QTextStream>

bool CsvUtil::readCsv(const QString& strCsvFilePath, QList<QStringList>& lCsvData)
{
    lCsvData.clear();

    QFile csvFile(strCsvFilePath);
    if (!csvFile.open(QIODevice::ReadOnly)) {
        return false;
    }

    QTextStream textStream(&csvFile);

    QStringList temp;
    while (!textStream.atEnd()) {
        const auto line = textStream.readLine();
        splitLineToCsvCells(line, temp);
        lCsvData << temp;
    }

    return true;
}

void CsvUtil::splitLineToCsvCells(const QString& strLine, QStringList& lCsvCells)
{
    lCsvCells.clear();

    enum State { Normal, Quote } state = Normal;
    QString value;

    for (int ic = 0; ic < strLine.size(); ++ic) {
        const QChar current = strLine.at(ic);
        if (state == Normal) {
            // Comma
            if (current == ',') {
                lCsvCells.append(value.trimmed());
                value.clear();
            }
            // Double-quote
            else if (current == '"') {
                state = Quote;
                value += current;
            }
            // Other character
            else
                value += current;
        }
        // In-quote state
        else if (state == Quote) {
            // Another double-quote
            if (current == '"') {
                if (ic < strLine.size()) {
                    if (ic + 1 < strLine.size() && strLine.at(ic + 1) == '"') {
                        value += '"';
                        ++ic;
                    }
                    else {
                        state = Normal;
                        value += '"';
                    }
                }
            }
            // Other
            else {
                value += current;
            }
        }
    }

    if (!value.isEmpty()) {
        lCsvCells.append(value.trimmed());
    }

    for (int ic = 0; ic < lCsvCells.size(); ++ic) {
        if (lCsvCells[ic].length() >= 1 && lCsvCells[ic].at(0) == '"') {
            lCsvCells[ic] = lCsvCells[ic].mid(1);
            if (lCsvCells[ic].length() >= 1 && lCsvCells[ic].right(1) == '"') {
                lCsvCells[ic] = lCsvCells[ic].left(lCsvCells[ic].length() - 1);
            }
        }
    }
}

void CsvUtil::convertStringToCsvData(const QString& strData, QList<QStringList>& lCsvData)
{
    lCsvData.clear();

    QString     strSteram = strData;
    QTextStream stream(&strSteram);

    QStringList temp;
    while (!stream.atEnd()) {
        const auto line = stream.readLine();
        splitLineToCsvCells(line, temp);
        lCsvData << temp;
    }
}

bool CsvUtil::writeCsv(const QString& strCsvFilePath, const QList<QStringList>& lCsvData)
{
    QFile csvFile(strCsvFilePath);
    if (!csvFile.open(QIODevice::WriteOnly)) {
        return false;
    }

    QTextStream textStream(&csvFile);

    // MaxCells
    int nMaxCells = 0;
    for (auto ic = 0; ic < lCsvData.size(); ++ic) {
        if (nMaxCells < lCsvData[ic].size()) {
            nMaxCells = lCsvData[ic].size();
        }
    }

    // Data
    for (auto ic = 0; ic < lCsvData.size(); ++ic) {
        auto& strLine = lCsvData[ic];
        for (auto jc = 0; jc < nMaxCells; ++jc) {
            QString str;

            if (jc < strLine.size()) {
                str = strLine[jc];
            }

            textStream << str;

            if (jc < nMaxCells - 1) {
                textStream << ",";
            }
        }

        if (ic < lCsvData.size() - 1) {
            textStream << "\n";
        }
    }

    return true;
}

/**
 * @brief CsvUtil::readCsvLoose
 * @param csvPath
 * @return
 */
QList<QStringList> CsvUtil::readCsvLoose(const QString& csvPath)
{
    QFile csvFile(csvPath);
    if (!csvFile.open(QIODevice::ReadOnly)) {
        return {};
    }

    QTextStream textStream(&csvFile);

    QList<QStringList> csvData;
    while (!textStream.atEnd()) {
        const auto line = textStream.readLine();
        csvData << line.split(",");
    }
    return csvData;
}
