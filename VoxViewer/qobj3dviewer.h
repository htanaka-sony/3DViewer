#ifndef QOBJ3DVIEWER_H
#define QOBJ3DVIEWER_H

#include <QMatrix4x4>
#include <QMouseEvent>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShader>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <QPoint>
#include <QQuaternion>

#include <QtMath>

// #include "qobj3dreader.h"
// #include "voxRead_makeGLdata.h"
#include "Vox3DForm.h"    //もとよりstruct triangles参照のため。　//for-vox ui参照などのため および

class QObj3dViewer : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit QObj3dViewer(QWidget* parent = 0);
    ~QObj3dViewer();

    // void setTriangles(QVector<QOpenGLTriangle3D> &_triangles); //org .objファイルからの情報設定
    void setTextureFile(const QString& file);

    //-start- for-vox
    void               func_vox_setTriangles(QVector<QOpenGLTriangle3D_vox>& _triangles);
    QVector<QVector3D> g_GLColors;
    QVector<QVector2D> g_GLAcolors;        // 半透明設定
    int                g_ui_acolorflag;    // GUIの半透明設定
    int g_ui_voxshiftflag;    // GUIのチェックボックス状態。マウス操作平行移動モード=チェックON＝1 , チェックOFF=0
    int g_flag_mouseRotate;    // GUIのチェックボックス状態。マウス操作回転モード=チェックON＝1 , チェックOFF=0
    void func_vox_setRotation(float angleY, float angleZ);
    //-end- for-vox

public slots:
    void setRotation(float angle, float x, float y, float z);
    void setTranslation(float x, float y, float z);
    void setScale(float s);
    void setPerspective(float verticalAngle, float nearPlane, float farPlane);
    void setLookAt(const QVector3D& eye, const QVector3D& center, const QVector3D& up);
    void setLighting(const QVector3D& lightPos, const QVector3D& kd, const QVector3D& ld);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);

    void mouseDoubleClickEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void wheelEvent(QWheelEvent* event);    // for-vox  マウスホイールでズームイン・ズームアウト

signals:
    void func_signal_mousewheelZoom(float send_modelScale);    // for-vox
    void func_signal_mouseRotate(float send_rotateAngleY, float send_rotateAngleZ);
    void func_signal_mouseDrag02(int send_mouseMoveX, int send_mouseMoveY);
    void func_signal_mousePress(float send_mouseX, float send_mouseY);
    void func_signal_mouseRelease(float send_mouseX, float send_mouseY);

public slots:

private:
    QVector<QVector3D> triangles;
    QVector<QVector3D> normals;
    QVector<QVector2D> textureUV;
    QOpenGLTexture*    texture;

    QOpenGLShaderProgram openglProgram;

    QQuaternion modelRotation;
    QVector3D   modelTranslation;
    float       modelScale;
    QMatrix4x4  modelMat, viewMat, projectionMat;

    struct {
        float verticalAngle;
        float aspectRatio;
        float nearPlane;
        float farPlane;
    } perspective;

    struct {
        QVector3D eye;
        QVector3D center;
        QVector3D up;
    } lookAt;

    QVector3D lightPosition;
    QVector3D lightKd;
    QVector3D lightLd;

    //-start- for-vox
    QPoint lastMousePos;         // org 使わなければ後で消す
    QPoint lastMousePosition;    // for-vox　opengl_myk-myglwidget.hから流用 マウス回転用
    double alpha;                // マウス回転用 GL座標:Y軸の角度 （=通常座標:Z軸の角度)
    double beta;                 // マウス回転用  X軸の回転角度
    // int g_flag_mouseRotate; //マウス回転用 publicに移動
    Vox3DForm* mVox3DForm;
    //-end- for-vox
};

#endif    // QOBJ3DVIEWER_H
