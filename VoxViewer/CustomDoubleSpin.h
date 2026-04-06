#ifndef CUSTOMDOUBLESPIN_H
#define CUSTOMDOUBLESPIN_H

#include <QDoubleSpinBox>

class CustomDoubleSpin : public QDoubleSpinBox {
    Q_OBJECT
    // カーソルがコンボボックスを通っただけで、値がかわってしまうのが
    // QComboBoxの仕様だが、それでは、ユーザーが意図しない場合でも、QComboBoxの値が値がかわってしまう。
    // ユーザー要望として、そうならないよう新しくクラス定義する　CustomCombo　に格上げしたコンボボックスでは
    // ユーザーにより、コンボボックスをクリックした時だけ値が変わる。

public:
    explicit CustomDoubleSpin(QWidget* parent = 0) : QDoubleSpinBox(parent) {}

    void wheelEvent(QWheelEvent* e) { e->ignore(); }
};
#endif    // CUSTOMDOUBLESPIN_H
