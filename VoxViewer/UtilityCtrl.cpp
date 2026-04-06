#include "UtilityCtrl.h"
#include "ResultOutputDlg.h"
#include "Vox3DForm.h"
#include "VoxOutputDlg.h"
#include "ui_Vox3DForm.h"

UtilityCtrl::UtilityCtrl(Vox3DForm* form, Ui::Vox3DForm* ui_) : QObject(form), ui(ui_), m_3DForm(form)
{
    connect(ui->pushButton_result3d_output_data, &QPushButton::clicked, this, [this]() { resultOutput(); });
    connect(ui->pushButton_vox_output_data, &QPushButton::clicked, this, [this]() { voxOutput(); });
}

UtilityCtrl::~UtilityCtrl()
{
    delete m_result_output_dlg;
    delete m_vox_output_dlg;
}

void UtilityCtrl::resultOutput()
{
    if (!m_result_output_dlg) {
        m_result_output_dlg = new ResultOutputDlg(m_3DForm, ui->obj3dViewer);
    }
    m_result_output_dlg->show();
    m_result_output_dlg->raise();
}

void UtilityCtrl::voxOutput()
{
    if (!m_vox_output_dlg) {
        m_vox_output_dlg = new VoxOutputDlg(m_3DForm, ui->obj3dViewer);
    }
    m_vox_output_dlg->show();
    m_vox_output_dlg->raise();
}

void UtilityCtrl::update()
{
    if (m_result_output_dlg) {
        m_result_output_dlg->updateDlg();
    }
    if (m_vox_output_dlg) {
        m_vox_output_dlg->updateDlg();
    }
}
