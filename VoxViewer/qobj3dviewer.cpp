#include "qobj3dviewer.h"
#include <QDebug>

QObj3dViewer::QObj3dViewer(QWidget* parent) : QOpenGLWidget(parent)
{
    texture = Q_NULLPTR;

    g_ui_acolorflag = 1;    // for-vox 1:半透明 0:通常表示
    g_ui_voxshiftflag =
        0;    // for-vox 1:図形平行移動モードON
              // マウスドラッグで平行移動　0:平行移動モードはOFF（OFFのときは、マウスドラッグで図形回転モード)
    g_flag_mouseRotate = 0;    // for-vox マウスドラッグ図形回転
    alpha = 0;    // alpha=25 //for-vox マウスドラッグ図形回転 GL座標:Y軸の角度 （=通常座標:Z軸の角度)
    beta = 0;    // beta=-25 //for-vox マウスドラッグ図形回転 GL座標:X軸の角度
}

QObj3dViewer::~QObj3dViewer()
{
    if (texture != Q_NULLPTR) delete texture;
}

// void QObj3dViewer::setTriangles(QVector<QOpenGLTriangle3D> &_triangles) //objファイル形式の時の読み込み用途
//{
//     triangles.clear();
//     normals.clear();

//    for(int i=0; i<_triangles.count(); i++)
//    {
//        triangles.append(_triangles.at(i).p1);
//        triangles.append(_triangles.at(i).p2);
//        triangles.append(_triangles.at(i).p3);

//        normals.append(_triangles.at(i).p1Normal);
//        normals.append(_triangles.at(i).p2Normal);
//        normals.append(_triangles.at(i).p3Normal);

//        textureUV.append(_triangles.at(i).p1UV);
//        textureUV.append(_triangles.at(i).p2UV);
//        textureUV.append(_triangles.at(i).p3UV);
//    }

//    update();
//}

void QObj3dViewer::initializeGL()
{
    initializeOpenGLFunctions();

    //-start- org DEBUGビルド用の記述 Q_ASSERTが必要
    //    Q_ASSERT(openglProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,
    //                                                   ":/vert-shader.vsh"));

    //    Q_ASSERT(openglProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,
    //                                                       ":/frag-shader.fsh"));

    //    Q_ASSERT(openglProgram.link());
    //    Q_ASSERT(openglProgram.bind());
    //-end- org

    //-start- for-vox Releaseビルド用の記述 Q_ASSERTは必ず除外　(そうしないと起動前に落ちる）
    openglProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vert-shader.vsh");
    openglProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/frag-shader.fsh");
    openglProgram.link();
    openglProgram.bind();
    //-end- for-vox

    perspective.aspectRatio = height() > 0.0f ? float(width()) / float(height()) : 1.0f;

    glClearColor(1.0, 1.0, 1.0, 1.0);
    glEnable(GL_DEPTH_TEST);
    // glEnable(GL_CULL_FACE); //kuroda
    // 片面のみ表示する設定（コメントアウなしで有効だと回転途中で見えなくなってしまう。）
    // 透明の時もこれで実際OKだった。　//チュートリアルによると有効にするべきかもしれないが、今回はそうしていない。→
    // (glDesableをオフが必須とのこと）http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-10-transparency/

    glEnable(GL_BLEND);                                   // kuroda 透明表示のため追加
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);    // kuroda 透明表示のため追加
}

void QObj3dViewer::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    projectionMat.setToIdentity();
    projectionMat.perspective(perspective.verticalAngle, perspective.aspectRatio, perspective.nearPlane,
                              perspective.farPlane);

    viewMat.setToIdentity();
    viewMat.lookAt(lookAt.eye, lookAt.center, lookAt.up);

    modelMat.setToIdentity();
    modelMat.scale(modelScale);
    modelMat.rotate(modelRotation);    // 回転 org-OK objReader
    //-start-　本番用-OK マウスドラッグによる回転 voxbyM
    // if(g_flag_mouseRotate == 1 ){
    //    modelMat.rotate(alpha, 0, 1, 0); //通常座標:XY軸の角度 水平方向に回転)
    //    modelMat.rotate(beta, 1, 0, 0);  //通常座標:Z軸の角度　垂直方向に回転
    //    g_flag_mouseRotate = 0;
    //}
    //-end-　本番用-OK マウスドラッグによる回転 voxbyM
    modelMat.translate(modelTranslation);

    openglProgram.enableAttributeArray("vertexPosition");
    openglProgram.setAttributeArray("vertexPosition", triangles.constData());

    openglProgram.enableAttributeArray("vertexNormal");
    openglProgram.setAttributeArray("vertexNormal", normals.constData());

    openglProgram.enableAttributeArray("vertexUV_a");
    openglProgram.setAttributeArray("vertexUV_a", textureUV.constData());

    if (texture != Q_NULLPTR) {
        openglProgram.setUniformValue("renderTexture", true);
        openglProgram.setUniformValue("texture", 0);
        texture->bind();
    }
    else {
        openglProgram.setUniformValue("renderTexture", false);
    }

    openglProgram.setUniformValue("normalMatrix", modelMat.normalMatrix());    // world normal
    openglProgram.setUniformValue("modelViewMatrix", viewMat * modelMat);
    openglProgram.setUniformValue("projectionMatrix", projectionMat);
    openglProgram.setUniformValue("modelViewProjMatrix", projectionMat * viewMat * modelMat);

    openglProgram.setUniformValue("lightPosition", lightPosition);
    openglProgram.setUniformValue("lightKd", lightKd);
    openglProgram.setUniformValue("lightLd", lightLd);

    // qDebug() << "[DEBUG]qobj3dviewer.cpp-paintGL() g_GLcolors" << g_GLColors;
    openglProgram.setAttributeArray("color", g_GLColors.constData());
    openglProgram.enableAttributeArray("color");

    //-start- kuroda [DEBUG] テストデータ1平面　色設定用
    // QVector<QVector3D> colors;
    // for(int i=0; i<triangles.count(); i++){
    //    if(i < 1) {
    //        colors.append(QVector3D(1, 0, 0));
    //        colors.append(QVector3D(1, 0, 0));
    //        colors.append(QVector3D(1, 0, 0));
    //    } else {
    //        colors.append(QVector3D(0, 0, 1));
    //        colors.append(QVector3D(0, 0, 1));
    //        colors.append(QVector3D(0, 0, 1));
    //    }
    //}
    // openglProgram.setAttributeArray("color", colors.constData());
    // openglProgram.enableAttributeArray("color");
    //-end- kuroda [DEBUG] 1平面用

    //-start- kuroda [DEBUG] テストデータ1平面　半透明色設定用
    // QVector<GLfloat> acolors;
    // for(int i=0; i<triangles.count(); i++){
    //    acolors.append(0.3); //半透明
    //  if(i < 1) {
    //      acolors.append(0.3); //半透明
    //  } else {
    //      acolors.append(1.0); //通常色（透明にしない）
    //  }
    //}
    // openglProgram.enableAttributeArray("acolo");
    // openglProgram.setAttributeArray("acolor", acolors.constData());

    //-start- テスト各面ごとの半透明あり・なし
    openglProgram.setAttributeArray("acolorTEST01", g_GLAcolors.constData());
    openglProgram.enableAttributeArray("acolorTEST01");
    //-end-

    // ↑半透明設定　マテリアルごといまだできずのため　↓　全部半透明のための設定

    //-start- OK　フラグflag_acolorで、半透明あり、なしを決める
    // int flag_acolor;
    // flag_acolor = 0; //0:通常表示 1: 半透明
    // openglProgram.setUniformValue("flag_acolor", flag_acolor);
    //-end- OK　フラグflag_acolorで、半透明あり、なしを決める

    //    //-start- OK　acolorで、半透明あり、なしを決める
    float acolorOne;
    acolorOne = 0.3;    // 半透明
    if (g_ui_acolorflag == 1) {
        acolorOne = 0.3;    // 半透明
    }
    else {
        acolorOne = 1.0;    // 透明なし
    }
    openglProgram.setUniformValue("acolorOne", acolorOne);
    //-end- kuroda [DEBUG] 1平面用　半透明色設定用

    glDrawArrays(GL_TRIANGLES, 0, triangles.count());

    // qDebug() << "[DEBUG]qobj3dviewr.cpp-paintGL";
}

void QObj3dViewer::resizeGL(int w, int h)
{
    perspective.aspectRatio = h > 0.0f ? float(w) / float(h) : 1.0f;
}

void QObj3dViewer::setRotation(float angle, float x, float y, float z)
{
    modelRotation = QQuaternion::fromAxisAndAngle(x, y, z, angle);
}

void QObj3dViewer::setTranslation(float x, float y, float z)
{
    modelTranslation = QVector3D(x, y, z);
}

void QObj3dViewer::setScale(float s)
{
    modelScale = s;
}

void QObj3dViewer::setPerspective(float verticalAngle, float nearPlane, float farPlane)
{
    perspective.verticalAngle = verticalAngle;
    perspective.nearPlane     = nearPlane;
    perspective.farPlane      = farPlane;
}

void QObj3dViewer::setLookAt(const QVector3D& eye, const QVector3D& center, const QVector3D& up)
{
    lookAt.eye    = eye;
    lookAt.center = center;
    lookAt.up     = up;
}

void QObj3dViewer::setLighting(const QVector3D& lightPos, const QVector3D& kd, const QVector3D& ld)
{
    lightPosition = lightPos;
    lightKd       = kd;
    lightLd       = ld;
}

void QObj3dViewer::setTextureFile(const QString& file)
{
    if (texture == Q_NULLPTR) delete texture;
    texture = new QOpenGLTexture(QImage(file).mirrored());
}

void QObj3dViewer::mouseDoubleClickEvent(QMouseEvent* event)
{
    // Q_UNUSED(event);
    // qDebug() << Q_FUNC_INFO << "Not implemented yet!";
    qDebug() << QString("QObj3dViewer.cpp-mouseDoubleClickEvent click x=%1, y=%2 GLall-width=%3 height=%4")
                    .arg(QString::number(event->x()), QString::number(event->y()), QString::number(width()),
                         QString::number(height()));
}

//-start- マウスドラッグで回転-OK-
// void QObj3dViewer::mouseMoveEvent(QMouseEvent *event)
//{
//    //-start- org
//    //Q_UNUSED(event);
//    //qDebug() << Q_FUNC_INFO << "Not implemented yet!";
//    //-end- org

//    int deltaX = event->x() - lastMousePosition.x();
//    int deltaY = event->y() - lastMousePosition.y();
//    if (event->buttons() & Qt::LeftButton) { //マウスドラッグ　(=左ボタン押しながらMouseMove)
//        alpha -= deltaX;
//        while (alpha < 0) {
//            alpha += 360;
//        }
//        while (alpha >= 360) {
//            alpha -= 360;
//        }

//        beta -= deltaY;
//        if (beta < -90) {
//            beta = -90;
//        }
//        if (beta > 90) {
//            beta = 90;
//        }
//        //qDebug() << QString("QObj3dViewer.cpp-mouseReleaseEvent after alpha=%1,
//        beta=%2").arg(QString::number(alpha),QString::number(beta) ); g_flag_mouseRotate = 1; update();
//    }

//}
//-end- マウスドラッグで回転-OK-

// 　マウスドラッグ処理(松岡さん引継ぎプログラムがベース)　処理案01
// (回転・平行移動モード単体動作はOK.平行移動モードにした時、回転状態がリセットされてしまう不具合あり。) void
// QObj3dViewer::mouseMoveEvent(QMouseEvent *event)
//{
//     //-start- org
//     //Q_UNUSED(event);
//     //qDebug() << Q_FUNC_INFO << "Not implemented yet!";
//     //-end- org
//     int deltaX = event->x() - lastMousePosition.x();
//     int deltaY = event->y() - lastMousePosition.y();
//     if (event->buttons() & Qt::LeftButton) { //マウスドラッグ　(=左ボタン押しながらMouseMove)
//         //通常は、マウスドラッグで図形回転モード
//         //マウスドラッグで図形平行移動モードONの場合(g_ui_voxshiftflag == 1)
//         マウスドラッグで平行移動。　0:平行移動モードはOFF（OFFのときは、マウスドラッグで図形回転モード)
//
//         if(g_ui_voxshiftflag == 0 ){
//             //通常 マウスドラッグで図形回転モード
//             alpha -= deltaX;
//             while (alpha < 0) {
//                 alpha += 360;
//             }
//             while (alpha >= 360) {
//                 alpha -= 360;
//             }
//
//             beta -= deltaY;
//             if (beta < -90) {
//                 beta = -90;
//             }
//             if (beta > 90) {
//                 beta = 90;
//             }
//             //-start- 回転処理:松岡さん引継ぎプログラムがベース
//             //qDebug() << QString("QObj3dViewer.cpp-mouseReleaseEvent after alpha=%1,
//             beta=%2").arg(QString::number(alpha),QString::number(beta) ); g_flag_mouseRotate = 1; update();
//             //-end- 回転処理:松岡さん引継ぎプログラムがベース
//
//             //emit func_signal_mouseRotate(alpha, beta); //黒田自作の回転処理
//         } else {
//             //平行移動モード
//             emit func_signal_mouseDrag02(deltaX, deltaY);
//         }
//     }
//     lastMousePosition = event->pos();
//     event->accept();
// }
//-end- mouseMoveEvent()

//-start- mouseMoveEvent() 処理案02　回転処理は黒田自作func_slot_mouseDrag02 (上下90度まで
// 左右回転で一部表示できない不具合あり)
void QObj3dViewer::mouseMoveEvent(QMouseEvent* event)
{
    //    Q_UNUSED(event);
    //    qDebug() << Q_FUNC_INFO << "Not implemented yet!";

    int deltaX = event->x() - lastMousePosition.x();
    int deltaY = event->y() - lastMousePosition.y();
    if (event->buttons() & Qt::LeftButton) {    // マウスドラッグ　(=左ボタン押しながらMouseMove)
        // 通常は、マウスドラッグで図形回転モード
        // マウスドラッグで図形平行移動モードONの場合
        // マウスドラッグで平行移動。　0:平行移動モードはOFF（OFFのときは、マウスドラッグで図形回転モード)
        emit func_signal_mouseDrag02(deltaX, deltaY);
    }
    lastMousePosition = event->pos();
    event->accept();
}
//-end- mouseMoveEvent()

void QObj3dViewer::mousePressEvent(QMouseEvent* event)
{
    //-org- start
    //    Q_UNUSED(event);
    //    qDebug() << Q_FUNC_INFO << "Not implemented yet!";
    //-org- end

    // for-vox change
    lastMousePosition = event->pos();

    emit func_signal_mousePress(event->x(), event->y());

    event->accept();

    // qDebug() << QString("[DEBUG]01 QObj3dViewer.cpp-mousePressEvent After lastMousePosition　x=%1
    // y=%2").arg(QString::number(lastMousePosition.x()), QString::number(lastMousePosition.y()));
}

void QObj3dViewer::mouseReleaseEvent(QMouseEvent* event)
{
    // Q_UNUSED(event);
    // qDebug() << Q_FUNC_INFO << "Not implemented yet!";

    float deltaX = lastMousePosition.x();
    float deltaY = lastMousePosition.y();
    emit  func_signal_mouseRelease(deltaX, deltaY);

    event->accept();
}

//-start- for-vox マウスホイールでズームイン・ズームアウト
void QObj3dViewer::wheelEvent(QWheelEvent* event)
{
    //-start- バックアップ動作OK
    //    int delta = event->delta(); //マウスホイールが、どちらの方向に、どれだけの量動かされたかを取得。

    //    if (event->orientation() == Qt::Vertical) {
    //        if (delta < 0) {
    //            modelScale = modelScale * 1.1;
    //            //GUIフォーム Transformタブ-scale も更新すべきだが、未対応。これから。
    //        } else if (delta > 0) {
    //            modelScale = modelScale * 0.9;
    //            //GUIフォーム Transformタブ-scale も更新すべきだが、未対応。これから。
    //        }

    //        //qDebug() << QString("[DEBUG]01 QObj3dViewer.cpp-wheelEvent After
    //        modelScale=%1").arg(QString::number(modelScale)); update();
    //      }
    //      event->accept();
    //-end- バックアップ動作OK

    //-start- テスト
    int delta = event->angleDelta().y();    // マウスホイールが、どちらの方向に、どれだけの量動かされたかを取得。
    float modelScale_new = modelScale;
    if (delta != 0) {
        if (delta < 0) {    // マウス奥方向へホイール→縮小
            modelScale_new = modelScale * 0.9;
            // GUIフォーム Transformタブ-scale も更新すべきだが、未対応。これから。
        }
        else if (delta > 0) {    // マウス方向へホイール→拡大
            modelScale_new = modelScale * 1.1;
            // GUIフォーム Transformタブ-scale も更新すべきだが、未対応。これから。
        }

        emit func_signal_mousewheelZoom(modelScale_new);

        // qDebug() << QString("[DEBUG]01 QObj3dViewer.cpp-wheelEvent After
        // modelScale=%1").arg(QString::number(modelScale)); update();
    }
    event->accept();
    //-end- テスト
}
//-start- for-vox

void QObj3dViewer::func_vox_setTriangles(QVector<QOpenGLTriangle3D_vox>& _triangles)
{
    triangles.clear();
    normals.clear();

    textureUV.clear();      // for-vox
    g_GLColors.clear();     // for-vox
    g_GLAcolors.clear();    // for-vox

    for (int i = 0; i < _triangles.count(); i++) {
        triangles.append(_triangles.at(i).p1);
        triangles.append(_triangles.at(i).p2);
        triangles.append(_triangles.at(i).p3);

        normals.append(_triangles.at(i).p1Normal);
        normals.append(_triangles.at(i).p2Normal);
        normals.append(_triangles.at(i).p3Normal);

        textureUV.append(_triangles.at(i).p1UV);
        textureUV.append(_triangles.at(i).p2UV);
        textureUV.append(_triangles.at(i).p3UV);

        g_GLColors.append(_triangles.at(i).color);    // 色は3頂点とも同色を入れる
        g_GLColors.append(_triangles.at(i).color);
        g_GLColors.append(_triangles.at(i).color);

        g_GLAcolors.append(QVector2D(
            _triangles.at(i).acolor,
            _triangles.at(i).acolor));    // 透明設定(値：0.0～1.0)は3頂点とも同色を入れる //本来は　1個だけ渡せば良いが
                                          // openglProgram.setAttributeArrayを使うため vector2にする
        g_GLAcolors.append(QVector2D(_triangles.at(i).acolor, _triangles.at(i).acolor));
        g_GLAcolors.append(QVector2D(_triangles.at(i).acolor, _triangles.at(i).acolor));
    }

    update();
}

void QObj3dViewer::func_vox_setRotation(float angleY, float angleZ)
{
    g_flag_mouseRotate = 1;
    alpha              = angleY;
    beta               = angleZ;
}
