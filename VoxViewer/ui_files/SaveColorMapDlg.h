#ifndef SAVECOLORMAPDLG_H
#define SAVECOLORMAPDLG_H

#include <QDialog>
#include <QVector3D>

namespace Ui {
class SaveColorMapDlg;
class Vox3DForm;
}    // namespace Ui

class SaveColorMapDlg : public QDialog {
    Q_OBJECT

public:
    explicit SaveColorMapDlg(QWidget* parent, Ui::Vox3DForm* ui, const QString& init_name);
    ~SaveColorMapDlg();

    QString filename();

    void checkSaveMinColor(bool checked);
    void checkSaveMaxColor(bool checked);
    void checkSaveSteps(bool checked);
    void checkSaveRange(bool checked);
    void checkSaveGradient(bool checked);
    void checkSaveReverse(bool checked);

    bool isSaveMinColor();
    bool isSaveMaxColor();
    bool isSaveSteps();
    bool isSaveRange();
    bool isSaveGradient();
    bool isSaveReverse();

protected:
    void accept() override;

private:
    Ui::SaveColorMapDlg* ui;
    Ui::Vox3DForm*       m_vox3d_ui;
};

#endif    // SAVECOLORMAPDLG_H
