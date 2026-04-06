#ifndef COMMONSUBCLASS_H
#define COMMONSUBCLASS_H

#include <QApplication>
#include <QColorDialog>
#include <QEvent>
#include <QHBoxLayout>
#include <QPushButton>
#include <QWheelEvent>
#include <QWidget>

#include "CustomCombo.h"
#include "CustomDoubleSpin.h"
#include "CustomSpin.h"

class MyOpenGLWidget;

class CommonSubClass {
public:
    CommonSubClass();
};

class CustomColorCombo : public QWidget {
    Q_OBJECT

public:
    CustomColorCombo(QWidget* parent = nullptr, int matnum = 0, MyOpenGLWidget* gl_widget = nullptr);

    QComboBox* combobox() { return comboBox; }

    void setCurrentIndex(int index) { comboBox->setCurrentIndex(index); }
    void setCurrentText(const QString& text) { comboBox->setCurrentText(text); }

    int     currentIndex() const { return comboBox->currentIndex(); }
    QString currentText() const { return comboBox->currentText(); }
    void    setItemText(int index, const QString& text) { comboBox->setItemText(index, text); }
    void    setItemData(int index, const QVariant& value, int role = Qt::UserRole)
    {
        comboBox->setItemData(index, value, role);
    }
    inline void addItem(const QString& text, const QVariant& userData = QVariant())
    {
        comboBox->addItem(text, userData);
    }
    inline void addItem(const QIcon& icon, const QString& text, const QVariant& userData = QVariant())
    {
        comboBox->addItem(icon, text, userData);
    }
    inline void addItems(const QStringList& texts) { comboBox->addItems(texts); }
    inline void removeItems() { comboBox->clear(); }

    inline QStringList items() const
    {
        QStringList texts;
        for (int ic = 0; ic < comboBox->count(); ++ic) {
            texts << comboBox->itemText(ic);
        }
        return texts;
    }

    void setColorButtonStyle(QColor color);

    bool isColorCloserToWhite(const QColor& color);

    void setColor(const QString& cname, QColor color);

    QVector3D color() const { return QVector3D(m_color.redF(), m_color.greenF(), m_color.blueF()); }
    QColor    qColor() const { return m_color; }

    void wheelEvent(QWheelEvent* e) { e->ignore(); }

private slots:
    void onColorButtonClicked();

private:
    CustomCombo* comboBox;
    QPushButton* colorButton;
    QColor       m_color;
    QColor       m_color_button_color;

    MyOpenGLWidget* m_gl_widget;
    int             m_matnum;
};

class CustomComboDialog : public QDialog {
    Q_OBJECT

public:
    CustomComboDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // CustomComboを1つだけ配置
        customCombo = new CustomColorCombo(this);
        customCombo->setFocusPolicy(Qt::NoFocus);
        mainLayout->addWidget(customCombo);

        // OK・キャンセルボタン
        QHBoxLayout* buttonLayout = new QHBoxLayout;
        QPushButton* okButton     = new QPushButton("OK", this);
        QPushButton* cancelButton = new QPushButton("キャンセル", this);

        buttonLayout->addStretch();
        buttonLayout->addWidget(okButton);
        buttonLayout->addWidget(cancelButton);

        mainLayout->addLayout(buttonLayout);

        // シグナル接続
        connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    }

    CustomColorCombo* getCustomCombo() const { return customCombo; }

private:
    CustomColorCombo* customCombo;
};

#endif    // COMMONSUBCLASS_H
