#include "SettingCtrl.h"

#include <QDir>
#include <QStandardPaths>

SettingCtrl::SettingCtrl()
{
    QDir doc_dir(documentPath());
    if (!doc_dir.exists()) {
        doc_dir.mkpath(documentPath());
    }
    m_setting = new QSettings(settingIniPath(), QSettings::IniFormat);
}

SettingCtrl::~SettingCtrl()
{
    delete m_setting;
}

QString SettingCtrl::documentPath()
{
    return QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).constFirst() + QDir::separator()
         + "EyerisSetting";
}

QString SettingCtrl::resultSettingPath()
{
    return documentPath() + QDir::separator() + "Result";
}

QString SettingCtrl::resultColorMapPath()
{
    return resultSettingPath() + QDir::separator() + "ColorMap";
}

QString SettingCtrl::settingIniPath()
{
    return documentPath() + QDir::separator() + QString("Eyeris3DViewer.ini");
}

QString SettingCtrl::logFilePath(int index)
{
    const auto& log_folder = documentPath() + QDir::separator() + QString("Log");

    QDir log_dir(log_folder);
    if (!log_dir.exists()) {
        log_dir.mkpath(log_folder);
    }

    if (index == 0)
        return log_folder + QDir::separator() + QString("Eyeris3DViewer.log");
    else
        return log_folder + QDir::separator() + QString("Eyeris3DViewer_%1.log").arg(index);
}

int SettingCtrl::logLevel()
{
    /// 直接ファイルを開く（起動の最初で一回呼ぶだけの想定）
    static int log_level = INT_MAX;
    if (log_level == INT_MAX) {
        QSettings settings(settingIniPath(), QSettings::IniFormat);
        log_level = settings.value("Log/log_level", 0).toInt();
        if (log_level == INT_MAX) {
            log_level = 0;
        }
    }
    return log_level;
}

void SettingCtrl::setInt(const QString& strKey, int iValue, bool bInit /*=false*/)
{
    setString(strKey, QString::number(iValue), bInit);
}

void SettingCtrl::setDouble(const QString& strKey, double dValue, bool bInit /*=false*/)
{
    setString(strKey, QString::number(dValue), bInit);
}

void SettingCtrl::setString(const QString& strKey, const QString& strValue, bool bInit /*=false*/)
{
    if (bInit) {
        if (m_setting->contains(strKey)) {
            return;
        }
    }

    /// setValueを呼ぶと変化がなくても保存が走るため、変化があるか（設定値の変更があるか）チェックする
    /// 　※value／setValueで共通処理が多く、高速化が必要ならQSettingsを派生させて改変するとかが必要だが、通常のINIの使い方であれば気にすることはないと思う
    /// キー自体がない／キーの値が空 を区別するため、通常存在しないdefault値を用いる
    static const QString DEFAULT_CHECK_VALUE = "FA4C3867B9D24966BA28CF395A1D5391";

    const QString& strOldValue = m_setting->value(strKey, DEFAULT_CHECK_VALUE).toString();
    if (strOldValue == DEFAULT_CHECK_VALUE || strOldValue != strValue) {
        m_setting->setValue(strKey, strValue);
    }
}

int SettingCtrl::getInt(const QString& strKey, const int defaultValue)
{
    const QString& strTemp = getString(strKey);
    if (strTemp.isEmpty()) {
        return defaultValue;
    }
    return strTemp.toInt();
}

double SettingCtrl::getDouble(const QString& strKey, const double defaultValue)
{
    const QString& strTemp = getString(strKey);
    if (strTemp.isEmpty()) {
        return defaultValue;
    }
    return strTemp.toDouble();
}

QString SettingCtrl::getString(const QString& strKey, const QString& defaultValue, bool bRetDefaultIfEmpty)
{
    /// 値が空の場合、空を返す
    if (!bRetDefaultIfEmpty || defaultValue.isEmpty()) {
        return m_setting->value(strKey, defaultValue).toString();
    }
    /// 値が空の場合、デフォルトを返す
    else {
        const QString& ret = m_setting->value(strKey, defaultValue).toString();
        if (ret.isEmpty()) {
            return defaultValue;
        }
        return ret;
    }
}

void SettingCtrl::sync()
{
    m_setting->sync();
}

void SettingCtrl::remove(const QString& strKey)
{
    m_setting->remove(strKey);
}

bool SettingCtrl::createDirectoryIfNotExists(const QString& dirPath)
{
    QDir dir;
    if (!dir.exists(dirPath)) {
        return dir.mkpath(dirPath);    /// 作成成功ならtrue、失敗ならfalse
    }
    return true;    /// 既に存在していればtrue
}
