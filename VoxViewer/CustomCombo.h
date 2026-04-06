#ifndef CUSTOMCOMBO_H
#define CUSTOMCOMBO_H

#include <QComboBox>

class CustomCombo : public QComboBox {
    Q_OBJECT
    // カーソルがコンボボックスを通っただけで、値がかわってしまうのが
    // QComboBoxの仕様だが、それでは、ユーザーが意図しない場合でも、QComboBoxの値が値がかわってしまう。
    // ユーザー要望として、そうならないよう新しくクラス定義する　CustomCombo　に格上げしたコンボボックスでは
    // ユーザーにより、コンボボックスをクリックした時だけ値が変わる。

public:
    explicit CustomCombo(QWidget* parent = 0) : QComboBox(parent) {}

    void wheelEvent(QWheelEvent* e)
    {
        // qDebug<< "scroll";
        // if(hasFocus()){
        //     QComboBox::wheelEvent(e);
        // } else {
        //     e->ignore();
        // }
        e->ignore();
    }
};

#endif    // CUSTOMCOMBO_H
