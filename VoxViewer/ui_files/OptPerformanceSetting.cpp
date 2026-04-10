#include "OptPerformanceSetting.h"
#include "SettingGuiCtrl.h"
#include "ui_OptPerformanceSetting.h"

OptPerformanceSetting::OptPerformanceSetting(QWidget* parent, SettingGuiCtrl* ctrl)
    : QDialog(parent)
    , m_ctrl(ctrl)
    , ui(new Ui::OptPerformanceSetting)
{
    ui->setupUi(this);

    ui->groupBox_limit_memory->setChecked(m_ctrl->readOptMemoryLimit());
    ui->doubleSpinBox_limit_memory_value->setValue(m_ctrl->readOptMemorySize());
    ui->comboBox_limit_memory_unit->setCurrentIndex(m_ctrl->readOptMemoryUnit());
    ui->comboBox_limit_memory_method->setCurrentIndex(m_ctrl->readOptMemoryMethod());
    ui->checkBox_warn->setChecked(m_ctrl->readOptMemoryWarn());

    connect(this, &QDialog::accepted, this, [this]() {
        m_ctrl->saveOptMemoryLimit(ui->groupBox_limit_memory->isChecked());
        m_ctrl->saveOptMemorySize(ui->doubleSpinBox_limit_memory_value->value());
        m_ctrl->saveOptMemoryUnit(ui->comboBox_limit_memory_unit->currentIndex());
        m_ctrl->saveOptMemoryMethod(ui->comboBox_limit_memory_method->currentIndex());
        m_ctrl->saveOptMemoryWarn(ui->checkBox_warn->isChecked());
    });

    connect(ui->pushButton_default, &QPushButton::clicked, this, [this]() {
        ui->groupBox_limit_memory->setChecked(m_ctrl->optMemoryLimitDefault());
        ui->doubleSpinBox_limit_memory_value->setValue(m_ctrl->optMemorySizeDefault());
        ui->comboBox_limit_memory_unit->setCurrentIndex(m_ctrl->optMemoryUnitDefault());
        ui->comboBox_limit_memory_method->setCurrentIndex(m_ctrl->optMemoryMethodDefault());
        ui->checkBox_warn->setChecked(m_ctrl->optMemoryWarnDefault());
    });
}

OptPerformanceSetting::~OptPerformanceSetting()
{
    delete ui;
}
