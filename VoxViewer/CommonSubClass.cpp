#include "CommonSubClass.h"
#include "MyOpenGLWidget.h"
#include "Vox3DForm.h"

CommonSubClass::CommonSubClass() {}

CustomColorCombo::CustomColorCombo(QWidget* parent, int matnum, MyOpenGLWidget* gl_widget) : QWidget(parent)
{
    m_gl_widget = gl_widget;
    m_matnum    = matnum;

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // コンボボックスを作成
    comboBox = new CustomCombo(this);
    comboBox->addItem("");
    comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    comboBox->setFocusPolicy(Qt::NoFocus);

    // カラーボタンを作成
    colorButton = new QPushButton("", this);
    connect(colorButton, &QPushButton::clicked, this, &CustomColorCombo::onColorButtonClicked);
    colorButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    colorButton->setFixedWidth(25);

    // レイアウトに追加
    layout->addWidget(comboBox);    // コンボボックスを追加
    layout->addWidget(colorButton);

    setColor("Dummy", QColor(255, 255, 255));
}

void CustomColorCombo::setColorButtonStyle(QColor color)
{
    colorButton->setStyleSheet(Vox3DForm::colorStyle(color));
}

bool CustomColorCombo::isColorCloserToWhite(const QColor& color)
{
    // RGB値を取得
    int r = color.red();
    int g = color.green();
    int b = color.blue();

    // 輝度を計算
    double brightness = 0.299 * r + 0.587 * g + 0.114 * b;

    // 閾値128を基準に判定
    return brightness > 128;
}

void CustomColorCombo::setColor(const QString& cname, QColor color)
{
    if (cname == "") {
        if (m_color_button_color.isValid()) {
            m_color = m_color_button_color;
        }
        // m_color_button_color = color;
        setColorButtonStyle(m_color);
        QString fontcolor = isColorCloserToWhite(m_color) ? "#000000" : "#ffffff";
        QString strStyle =
            QString("QComboBox { background-color: %1 ; color: %2 ; }").arg(m_color.name(QColor::HexArgb), fontcolor);
        comboBox->setStyleSheet(strStyle);
    }
    else {
        m_color = color;
        setColorButtonStyle(m_color);
        QString fontcolor = isColorCloserToWhite(m_color) ? "#000000" : "#ffffff";
        QString strStyle =
            QString("QComboBox { background-color: %1 ; color: %2 ; }").arg(m_color.name(QColor::HexArgb), fontcolor);
        comboBox->setStyleSheet(strStyle);
    }
}

void CustomColorCombo::onColorButtonClicked()
{
    QColor selected_color = QColorDialog::getColor(m_color, this, "カラーを選択");
    if (selected_color.isValid()) {
        m_color_button_color = selected_color;
        comboBox->setCurrentText("");
        setColor("Dummy", selected_color);

        if (m_gl_widget) {
            m_gl_widget->setColor(m_matnum, color(), true);
        }
    }
}
