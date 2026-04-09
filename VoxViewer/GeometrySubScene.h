#ifndef GEOMETRYSUBSCENE_H
#define GEOMETRYSUBSCENE_H

#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QTimer>

#ifdef EYERIS_3D_VISUALIZATION
#define TR(s) (s)
#else
#define TR(s) (QString::fromLocal8Bit(s))    // 2018.09.12 日本語表示対応(メッセージボックスなど）
#endif

class GeometrySubScene : public QGraphicsScene {
    Q_OBJECT

public:
    // GeometrySubScene();
    explicit GeometrySubScene(QObject* parent = 0);
    ~GeometrySubScene();

    // int mx;
    // int my;
    // for-vox 斜め断面のため floatに変更
    float mx;
    float my;

private:
    QPointF previousPoint;

signals:
    void sendCoord(float, float);              // for-vox 斜め断面(diagonal)のため変更
    void sendCoordMouseRight(float, float);    // for-vox 斜め断面(diagonal)のため変更
    void sendShortcutkey(QString);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event);    // クリックされた座標表示
    void keyPressEvent(QKeyEvent* event);                     // ショートカット機能 2020.03.xx-01
    void keyReleaseEvent(QKeyEvent* event);                   // ショートカット機能 2020.03.xx-01
};

#endif    // GEOMETRYSUBSCENE_H
