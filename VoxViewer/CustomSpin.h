#ifndef CUSTOMSPIN_H
#define CUSTOMSPIN_H

#include <QSpinBox>

class CustomSpin : public QSpinBox {
    Q_OBJECT
    // カーソルがコンボボックスを通っただけで、値がかわってしまうのが
    // QComboBoxの仕様だが、それでは、ユーザーが意図しない場合でも、QComboBoxの値が値がかわってしまう。
    // ユーザー要望として、そうならないよう新しくクラス定義する　CustomCombo　に格上げしたコンボボックスでは
    // ユーザーにより、コンボボックスをクリックした時だけ値が変わる。

public:
    explicit CustomSpin(QWidget* parent = 0) : QSpinBox(parent) {}

    void wheelEvent(QWheelEvent* e) { e->ignore(); }
};

#endif    // CUSTOMSPIN_H
