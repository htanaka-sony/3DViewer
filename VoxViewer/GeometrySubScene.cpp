#include "GeometrySubScene.h"

// GeometrySubScene::GeometrySubScene()
//{
//
// }

GeometrySubScene::GeometrySubScene(QObject* parent) : QGraphicsScene(parent) {}

GeometrySubScene::~GeometrySubScene() {}

void GeometrySubScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    // クリックされた座標に点(極小の円図形）を表示
    // addEllipse(event->scenePos().x() - 5,
    //           event->scenePos().y() - 5,
    //           10,
    //           10,
    //           QPen(Qt::NoPen),
    //           QBrush(Qt::red));

    // 　クリックされた座標表示
    previousPoint = event->scenePos();

    qDebug() << "DEBUG mousePressEvent x:" << QString::number(previousPoint.x())
             << " y:" << QString::number(previousPoint.y());

    mx = previousPoint.x();
    my = previousPoint.y()
       * -1;    // QGraphicView では、(0,0)より右は(+)だが、上領域通は(-) とされるため、表示修正のため(-1)を掛ける
    // QString tmpZahyo =  QString::number(previousPoint.x()) + ":" +  QString::number(previousPoint.y());
    QString tmpZahyo = QString::number(mx) + ", " + QString::number(my);

    if (event->button() == Qt::LeftButton) {    // 左マウスクリック
        emit sendCoord(mx, my);
    }
    if (event->button() == Qt::RightButton) {    // 右マウスクリック
        emit sendCoordMouseRight(mx, my);
        // emit sendCoordMouseRight_float(previousPoint.x(), previousPoint.y() * -1);
    }

    this->update();
}

void GeometrySubScene::keyPressEvent(QKeyEvent* event)    // 2020.03.09 ショートカットキー処理
{
    if (event->key() == Qt::Key_R) {
        emit sendShortcutkey("key_R");    // Redraw
    }

    if (event->key() == Qt::Key_Z) {
        emit sendShortcutkey("key_Z");    // ZoomIn
    }

    if (event->modifiers().testFlag(Qt::ShiftModifier) && event->key() == Qt::Key_Z) {
        emit sendShortcutkey("shift_Z");    // ZoomOut
    }

    if (event->key() == Qt::Key_F) {
        emit sendShortcutkey("key_F");    // ZoomAll = Fit
    }

    if (event->key() == Qt::Key_B) {
        emit sendShortcutkey("key_B");    // コンボボックスを Zoom_boxにセットする
    }

    //    if(event->key() == Qt::Key_X){
    //        emit sendShortcutkey("key_X"); //Length座標点クリック有効にする　※2019.03.09
    //        ショートカットキーLengthとチェックボックスLengthは動作違いあり。
    //        チェックボックスONの時はZoombox不可。ショートカットキーではZoomboxしながらLength使用可能。
    //    }

    if (event->key() == Qt::Key_L) {
        emit sendShortcutkey(
            "key_L");    // Length座標点クリック有効にする　※2019.03.09
                         // ショートカットキーLengthとチェックボックスLengthは動作違いあり。
                         // チェックボックスONの時はZoombox不可。ショートカットキーではZoomboxしながらLength使用可能。
    }

    if (event->key() == Qt::Key_Backspace) {
        emit sendShortcutkey("key_Backspace");    // Lengthの座標選択をList末から1つずつ削除
    }

    if (event->key() == Qt::Key_Control) {
        emit sendShortcutkey("key_CTRL");    // ショートカットキー AutoLength用途(ctrl+左マウスクリック）
    }

    QGraphicsScene::keyPressEvent(event);    // デフォルトの動作へ (その他キーのため）
}

void GeometrySubScene::keyReleaseEvent(QKeyEvent* event)    // 2020.03.09 ショートカットキー処理 キーを離した時
{
    //    if(event->key() == Qt::Key_X){
    //        emit sendShortcutkey("off_X"); //ショートカットキーLength用途
    //    }

    if (event->key() == Qt::Key_L) {
        emit sendShortcutkey("off_L");    // ショートカットキーLength用途
    }

    if (event->key() == Qt::Key_Control) {
        emit sendShortcutkey("off_CTRL");    // ショートカットキー AutoLength用途(ctrl+左マウスクリック）
    }

    QGraphicsScene::keyReleaseEvent(event);    // デフォルトの動作へ (その他キーのため）
}
