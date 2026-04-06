#ifndef NONBORDERGROUGBOX_H
#define NONBORDERGROUGBOX_H

#include <QGroupBox>

class NonBorderGroupBox : public QGroupBox {
public:
    NonBorderGroupBox(QWidget* parent) : QGroupBox(parent) {}

protected:
    void paintEvent(QPaintEvent*) override {}
};

#endif
