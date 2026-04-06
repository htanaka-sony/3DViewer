#ifndef RESULTSTYLEDEFAULTCOMBBOX_H
#define RESULTSTYLEDEFAULTCOMBBOX_H

#include <QComboBox>
#include "CommonSubClass.h"
#include "Vox3DForm.h"

class ResultStyleDefaultCombBox : public CustomCombo {
    Q_OBJECT
public:
    explicit ResultStyleDefaultCombBox(QWidget* parent = nullptr) : CustomCombo(parent) {}

    void setVox3dForm(Vox3DForm* form) { m_form = form; }

protected:
    // プルダウンが開く直前に呼ばれる
    void showPopup() override
    {
        if (m_form) {
            m_form->setResultStyleDefaultCombobox();
        }
        QTimer::singleShot(0, this, [this]() {
            QComboBox::showPopup();    // 必ず親クラスも呼ぶ
        });
    }

    Vox3DForm* m_form = nullptr;
};

#endif    // RESULTSTYLEDEFAULTCOMBBOX_H
