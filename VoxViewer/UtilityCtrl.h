#ifndef UTILITYCTRL_H
#define UTILITYCTRL_H

#include <QObject>

#include "Scene/Node.h"

class Vox3DForm;
class ResultOutputDlg;
class VoxOutputDlg;

namespace Ui {
class Vox3DForm;
}

using namespace Core;

class UtilityCtrl : public QObject {
    Q_OBJECT

public:
    UtilityCtrl(Vox3DForm* form, Ui::Vox3DForm* ui_);
    ~UtilityCtrl();

    void update();

    void resultOutput();
    void voxOutput();

protected:
    Vox3DForm* m_3DForm = nullptr;

    Ui::Vox3DForm* ui;

    ResultOutputDlg* m_result_output_dlg = nullptr;
    VoxOutputDlg*    m_vox_output_dlg    = nullptr;
};

#endif    // UTILITYCTRL_H
