#ifndef OPTPERFORMANCESETTING_H
#define OPTPERFORMANCESETTING_H

#include <QDialog>

namespace Ui {
class OptPerformanceSetting;
}
class SettingGuiCtrl;

class OptPerformanceSetting : public QDialog {
    Q_OBJECT

public:
    explicit OptPerformanceSetting(QWidget* parent, SettingGuiCtrl* ctrl);
    ~OptPerformanceSetting();

private:
    Ui::OptPerformanceSetting* ui;
    SettingGuiCtrl*            m_ctrl;
};

#endif    // OPTPERFORMANCESETTING_H
