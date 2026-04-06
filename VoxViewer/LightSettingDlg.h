#ifndef LIGHTSETTINGDLG_H
#define LIGHTSETTINGDLG_H

#include <QDialog>
#include <QVector3D>

namespace Ui {
class LightSettingDlg;
}

class MyOpenGLWidget;

class LightSettingDlg : public QDialog {
    Q_OBJECT

public:
    explicit LightSettingDlg(QWidget* parent, MyOpenGLWidget* view);
    explicit LightSettingDlg(QWidget* parent, int light_mode);
    ~LightSettingDlg();

public:
    // モードを表す enum の定義
    enum LightMode {
        LeftUp,
        Left,
        LeftDown,
        Up,
        Center,
        Down,
        RightUp,
        Right,
        RightDown,
        LightModeCount    // モードの総数（配列サイズのために使用）
    };
    static QVector3D getLightDir(LightMode mode);

protected:
    // ライト方向の配列
    static constexpr QVector3D m_light_directions[LightModeCount] = {
        QVector3D(-1, 1, -1),     // LeftUp
        QVector3D(-1, 0, -1),     // Left
        QVector3D(-1, -1, -1),    // LeftDown
        QVector3D(0, 1, -1),      // Up
        QVector3D(0, 0, -1),      // Center
        QVector3D(0, -1, -1),     // Down
        QVector3D(1, 1, -1),      // RightUp
        QVector3D(1, 0, -1),      // Right
        QVector3D(1, -1, -1)      // RightDown
    };

public slots:
    void setLightDir(LightMode mode);

    LightMode getLightDir() const { return m_light_dir; }

private:
    Ui::LightSettingDlg* ui;

    MyOpenGLWidget* m_view;
    LightMode       m_light_dir;
};

#endif    // LIGHTSETTINGDLG_H
