#include "LightSettingDlg.h"

#include "ui_LightSetting.h"

#include "MyOpenGLWidget.h"

LightSettingDlg::LightSettingDlg(QWidget* parent, MyOpenGLWidget* view)
    : QDialog(parent)
    , ui(new Ui::LightSettingDlg)
    , m_view(view)
{
    ui->setupUi(this);

    if (m_view->isEqaulLightDir(m_light_directions[LeftUp])) {
        ui->pushButton_LeftUp->setChecked(true);
        ui->pushButton_LeftUp->setFocus();
        m_light_dir = LeftUp;
    }
    else if (m_view->isEqaulLightDir(m_light_directions[Left])) {
        ui->pushButton_Left->setChecked(true);
        ui->pushButton_Left->setFocus();
        m_light_dir = Left;
    }
    else if (m_view->isEqaulLightDir(m_light_directions[LeftDown])) {
        ui->pushButton_LeftDown->setChecked(true);
        ui->pushButton_LeftDown->setFocus();
        m_light_dir = LeftDown;
    }
    else if (m_view->isEqaulLightDir(m_light_directions[Up])) {
        ui->pushButton_Up->setChecked(true);
        ui->pushButton_Up->setFocus();
        m_light_dir = Up;
    }
    else if (m_view->isEqaulLightDir(m_light_directions[Center])) {
        ui->pushButton_Center->setChecked(true);
        ui->pushButton_Center->setFocus();
        m_light_dir = Center;
    }
    else if (m_view->isEqaulLightDir(m_light_directions[Down])) {
        ui->pushButton_Down->setChecked(true);
        ui->pushButton_Down->setFocus();
        m_light_dir = Down;
    }
    else if (m_view->isEqaulLightDir(m_light_directions[RightUp])) {
        ui->pushButton_RightUp->setChecked(true);
        ui->pushButton_RightUp->setFocus();
        m_light_dir = RightUp;
    }
    else if (m_view->isEqaulLightDir(m_light_directions[Right])) {
        ui->pushButton_Right->setChecked(true);
        ui->pushButton_Right->setFocus();
        m_light_dir = Right;
    }
    else if (m_view->isEqaulLightDir(m_light_directions[RightDown])) {
        ui->pushButton_RightDown->setChecked(true);
        ui->pushButton_RightDown->setFocus();
        m_light_dir = RightDown;
    }

    connect(ui->pushButton_LeftUp, &QPushButton::toggled, this, [this]() { setLightDir(LeftUp); });
    connect(ui->pushButton_Left, &QPushButton::toggled, this, [this]() { setLightDir(Left); });
    connect(ui->pushButton_LeftDown, &QPushButton::toggled, this, [this]() { setLightDir(LeftDown); });
    connect(ui->pushButton_Up, &QPushButton::toggled, this, [this]() { setLightDir(Up); });
    connect(ui->pushButton_Center, &QPushButton::toggled, this, [this]() { setLightDir(Center); });
    connect(ui->pushButton_Down, &QPushButton::toggled, this, [this]() { setLightDir(Down); });
    connect(ui->pushButton_RightUp, &QPushButton::toggled, this, [this]() { setLightDir(RightUp); });
    connect(ui->pushButton_Right, &QPushButton::toggled, this, [this]() { setLightDir(Right); });
    connect(ui->pushButton_RightDown, &QPushButton::toggled, this, [this]() { setLightDir(RightDown); });
}

LightSettingDlg::LightSettingDlg(QWidget* parent, int light_mode)
    : QDialog(parent)
    , ui(new Ui::LightSettingDlg)
    , m_view(nullptr)
{
    ui->setupUi(this);

    if (light_mode == LeftUp) {
        ui->pushButton_LeftUp->setChecked(true);
        ui->pushButton_LeftUp->setFocus();
    }
    else if (light_mode == Left) {
        ui->pushButton_Left->setChecked(true);
        ui->pushButton_Left->setFocus();
    }
    else if (light_mode == LeftDown) {
        ui->pushButton_LeftDown->setChecked(true);
        ui->pushButton_LeftDown->setFocus();
    }
    else if (light_mode == Up) {
        ui->pushButton_Up->setChecked(true);
        ui->pushButton_Up->setFocus();
    }
    else if (light_mode == Center) {
        ui->pushButton_Center->setChecked(true);
        ui->pushButton_Center->setFocus();
    }
    else if (light_mode == Down) {
        ui->pushButton_Down->setChecked(true);
        ui->pushButton_Down->setFocus();
    }
    else if (light_mode == RightUp) {
        ui->pushButton_RightUp->setChecked(true);
        ui->pushButton_RightUp->setFocus();
    }
    else if (light_mode == Right) {
        ui->pushButton_Right->setChecked(true);
        ui->pushButton_Right->setFocus();
    }
    else if (light_mode == RightDown) {
        ui->pushButton_RightDown->setChecked(true);
        ui->pushButton_RightDown->setFocus();
    }
    m_light_dir = (LightMode)light_mode;

    connect(ui->pushButton_LeftUp, &QPushButton::toggled, this, [this]() { setLightDir(LeftUp); });
    connect(ui->pushButton_Left, &QPushButton::toggled, this, [this]() { setLightDir(Left); });
    connect(ui->pushButton_LeftDown, &QPushButton::toggled, this, [this]() { setLightDir(LeftDown); });
    connect(ui->pushButton_Up, &QPushButton::toggled, this, [this]() { setLightDir(Up); });
    connect(ui->pushButton_Center, &QPushButton::toggled, this, [this]() { setLightDir(Center); });
    connect(ui->pushButton_Down, &QPushButton::toggled, this, [this]() { setLightDir(Down); });
    connect(ui->pushButton_RightUp, &QPushButton::toggled, this, [this]() { setLightDir(RightUp); });
    connect(ui->pushButton_Right, &QPushButton::toggled, this, [this]() { setLightDir(Right); });
    connect(ui->pushButton_RightDown, &QPushButton::toggled, this, [this]() { setLightDir(RightDown); });
}

LightSettingDlg::~LightSettingDlg()
{
    delete ui;
}

QVector3D LightSettingDlg::getLightDir(LightMode mode)
{
    return m_light_directions[mode];
}

void LightSettingDlg::setLightDir(LightMode mode)
{
    static int local_block = 0;
    if (local_block) {
        return;
    }

    QPushButton* target = nullptr;

    switch (mode) {
        case LeftUp:
            target = ui->pushButton_LeftUp;
            if (m_view) m_view->setLightDir(m_light_directions[LeftUp]);
            break;
        case 1:
            target = ui->pushButton_Left;
            if (m_view) m_view->setLightDir(m_light_directions[Left]);
            break;
        case 2:
            target = ui->pushButton_LeftDown;
            if (m_view) m_view->setLightDir(m_light_directions[LeftDown]);
            break;
        case 3:
            target = ui->pushButton_Up;
            if (m_view) m_view->setLightDir(m_light_directions[Up]);
            break;
        case 4:
            target = ui->pushButton_Center;
            if (m_view) m_view->setLightDir(m_light_directions[Center]);
            break;
        case 5:
            target = ui->pushButton_Down;
            if (m_view) m_view->setLightDir(m_light_directions[Down]);
            break;
        case 6:
            target = ui->pushButton_RightUp;
            if (m_view) m_view->setLightDir(m_light_directions[RightUp]);
            break;
        case 7:
            target = ui->pushButton_Right;
            if (m_view) m_view->setLightDir(m_light_directions[Right]);
            break;
        case 8:
            target = ui->pushButton_RightDown;
            if (m_view) m_view->setLightDir(m_light_directions[RightDown]);
            break;
    }
    m_light_dir = mode;

    if (!target) {
        return;
    }

    local_block = 1;
    ui->pushButton_LeftUp->setChecked(false);
    ui->pushButton_Left->setChecked(false);
    ui->pushButton_LeftDown->setChecked(false);
    ui->pushButton_Up->setChecked(false);
    ui->pushButton_Center->setChecked(false);
    ui->pushButton_Down->setChecked(false);
    ui->pushButton_RightUp->setChecked(false);
    ui->pushButton_Right->setChecked(false);
    ui->pushButton_RightDown->setChecked(false);
    target->setChecked(true);
    local_block = 0;

    if (m_view) m_view->update();
}
