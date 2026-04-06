#include "Vox3DForm.h"
#include "ui_Vox3DForm.h"

#include "CommonSubFunc.h"

void Vox3DForm::func_01main_GL_makeGL3DfromVox(QVector<QOpenGLTriangle3D_vox>& triangles, QString in_voxfilepath)
{
    // ■ GL描画用形式への置き換え　通常図形=正方形平面(座標4点）→ GL描画用形式 = 3角形2個(座標6点)　反時計回り座標
    //
    // ■ 通常座標 (x, y, z) のまま、設定すると　openGLでは、Y軸と、Z軸が反対に表示されてしまうため
    //   GL座標 (x, -y, -z) として入れなおして計算する
    //

    // voxファイルから座標・面方向・マテリアル情報取得する
    qDebug() << "[DEBUG]voxRead_makeGLdata.cpp-func_01main_GL_make: start func_vox_get_voxGraffic "
                    + QDateTime::currentDateTime().toString("hh:mm:ss");
    func_vox_get_voxGraffic(in_voxfilepath);
    // ↑zDEBUG_vox... 実行時も、func_vox_get_voxGrafficは有効にしておく。そうしないと表示されないので。
    // zDEBUG_vox_get_voxGraffic_DEBUG01(in_voxfilepath); 元座標が　正方形=閉じた面　で表示OK 全部 (1,1,1) (2,2,2)
    // (10,10,10)などの場合。 zDEBUG_input_getPointOfMesh_by2point_v2(); //[DEBUG]用途
    // 長方形面の始点・終点座標から、直方体1個を表示するための値設定
    qDebug() << "[DEBUG]voxRead_makeGLdata.cpp-func_01main_GL_make: end func_vox_get_voxGraffic "
                    + QDateTime::currentDateTime().toString("hh:mm:ss");

    //    //-start- [DEBUG] 本番ではここではなく,
    //    adjoin関数で半透明情報とるようにコード修正　ここでループ回して情報取得すると、ループ処理で描画が遅くなる可能性があるので
    //    g_voxAcolorVec.clear();
    //    for(int i=0; i<g_voxMatnumVec.size(); i++){
    //        int matnum = g_voxMatnumVec.at(i);
    //        g_voxAcolorVec.append(g_hash_matToAcolor[matnum]);
    //    }
    //    //-end- [DEBUG]
    // 軸表示用
    input_getPointOfMesh_by2point_jiku(g_voxSurfaceStrList, g_voxXYZStartVec, g_voxXYZEndVec, g_voxMatnumVec,
                                       g_voxColorVec, g_voxAcolorVec);    // 軸情報付け足し
    // input_getPointOfMesh_by2point_jiku_worldZahyo(g_voxSurfaceStrList, g_voxXYZStartVec, g_voxXYZEndVec,
    // g_voxMatnumVec, g_voxColorVec, g_voxAcolorVec); //軸情報付け足し

    // voxファイルからの取得情報を、openGL描画形式のデータに置き換える
    // //openGLでの描画単位=1つの三角形ごとの情報作成する。
    qDebug() << "[DEBUG]voxRead_makeGLdata.cpp-func_01main_GL_make: start makeGLinfo "
                    + QDateTime::currentDateTime().toString("hh:mm:ss");
    triangles.clear();    // 空のtrianglesを関数に渡して、帰り値を取得する
    // func_GL_make_getPointOfMesh(triangles, g_voxXYZVec, g_voxSurfaceStrList, g_voxColorVec, g_voxMatnumVec);
    //-start
    // zDEBUG_input_getPointOfMesh_by2point_v2();
    func_GL_make_getPointOfMesh_by2point(triangles, g_voxXYZStartVec, g_voxXYZEndVec, g_voxSurfaceStrList,
                                         g_voxMatnumVec, g_voxColorVec, g_voxAcolorVec);    // 軸表示用図形追加
    //-end-
    qDebug() << "[DEBUG]voxRead_makeGLdata.cpp-func_01main_GL_make: end makeGLinfo "
                    + QDateTime::currentDateTime().toString("hh:mm:ss");

    qDebug() << "[DEBUG]voxRead_makeGLdata.cpp-func_01main_GL_make: end makeGLinfo all-face-count"
             << QString::number(g_voxXYZStartVec.count());
}

// void Vox3DForm::func_GL_make_getPointOfMesh(QVector<QOpenGLTriangle3D_vox> &triangles, QVector<QVector3D>
// in_voxXYZVec, QStringList in_voxSurfaceStrList, QVector<QVector3D> in_voxColors, QVector<int>in_voxMatnum)
// //voxファイルからの取得情報を、openGL描画形式のデータに置き換える
//{
//     //openGLでの描画単位=1つの三角形ごとの情報作成する。
//     //メモリ使用量削減するなら、この処理介さず、
//     qobj3dviewer.h　setTriangleで使う形式にしてしまった方がいいかもしれない。
//     //1三角形ごと個別→頂点情報(v.p1～p3)色情報　は。 2つの三角形で共通→色(colors)、vn(法線=面の向き surfaceStr)。
//     全三角形で共通→vt(テクスチャ情報) triangles.clear(); QVector<QVector3D> v, vn; QVector<QVector2D> vt;

//    //voxファイルからの取得情報を、openGL描画形式のデータに置き換える
//    for(int pnum=0; pnum < in_voxXYZVec.count(); pnum++){
//        //qDebug() << "[DEBUG]voxRead_makeGLdata.cpp-func_01main_GL_make_getPointOfMesh voxXYZvec=" <<
//        QString::number( in_voxXYZVec.at(pnum).x())<< "," <<   QString::number( in_voxXYZVec.at(pnum).y()) << "," <<
//        QString::number( in_voxXYZVec.at(pnum).z());

//        int in_i, in_j, in_k;
//        //in_i = 1;
//        //in_j = 1;
//        //in_k = 1;
//        in_i = in_voxXYZVec.at(pnum).x();
//        in_j = in_voxXYZVec.at(pnum).z(); //GL:Z軸　j=通常:Z軸
//        in_k = in_voxXYZVec.at(pnum).y() * (-1);  //GL:Y軸　k=通常:Y軸 * -1(反転 前がプラス, 後ろがマイナス）
//        //GLfloat nowOP_x, nowOP_y , nowOP_z;
//        float nowOP_x, nowOP_y , nowOP_z;
//        //float meshsizeGL_x, meshsizeGL_y, meshsizeGL_z;
//        //meshsizeGL_x = 1; //[DEBUG]決め打ち GL座標として
//        //meshsizeGL_y = 1;
//        //meshsizeGL_z = 1;
//        nowOP_x = in_i * meshsizeGL_x; //kuroda変更　-0.5 では表示が小さすぎになってしまうための対処
//        nowOP_y = in_j * meshsizeGL_y; //kuroda変更　-0.5 では表示が小さすぎになってしまうための対処
//        nowOP_z = in_k * meshsizeGL_z; //kuroda変更　-0.5 では表示が小さすぎになってしまうための対処

//        //前準備 openGL座標作成 1メッシュ立方体の頂点座標を定義する
//        QVector3D vertexA = QVector3D(nowOP_x, nowOP_y, nowOP_z);
//        QVector3D vertexB = QVector3D(nowOP_x + meshsizeGL_x, nowOP_y, nowOP_z);
//        QVector3D vertexC = QVector3D(nowOP_x + meshsizeGL_x, nowOP_y + meshsizeGL_y, nowOP_z);
//        QVector3D vertexD = QVector3D(nowOP_x , nowOP_y + meshsizeGL_y, nowOP_z);
//        QVector3D vertexE = QVector3D(nowOP_x, nowOP_y, nowOP_z - meshsizeGL_z);
//        QVector3D vertexF = QVector3D(nowOP_x + meshsizeGL_x, nowOP_y, nowOP_z - meshsizeGL_z);
//        QVector3D vertexG = QVector3D(nowOP_x + meshsizeGL_x, nowOP_y + meshsizeGL_y, nowOP_z - meshsizeGL_z);
//        QVector3D vertexH = QVector3D(nowOP_x , nowOP_y + meshsizeGL_y, nowOP_z - meshsizeGL_z);

//        //[DEBUG]
//        //    vertexA = QVector3D(0.0, 0.0, 0.0); //Front, Bottom, Left
//        //    vertexB = QVector3D(1, 0, 0);
//        //    vertexC = QVector3D(1, 1, 0);
//        //    vertexD = QVector3D(0, 1, 0);
//        //    vertexE = QVector3D(0, 0, -1); //Back,  Bottom, Left
//        //    vertexF = QVector3D(1, 0, -1);
//        //    vertexG = QVector3D(1, 1, -1);
//        //    vertexH = QVector3D(0, 1, -1);

//        //前準備(openGL座標作成)  使う対象全部 前後・左右・上下で共通　　後処理trianglesアレイに入れる順番だけ違う。
//        vt.append(QVector2D(0.0, 0.0));
//        vt.append(QVector2D(1, 0));
//        vt.append(QVector2D(1, 1));
//        vt.append(QVector2D(0, 1));

//        //前準備(openGL座標作成) objファイルのf情報(v, vn, vt の描画上での並び） 使う対象全部
//        面：前後・左右・上下で共通
//        //f 1/1/1 2/2/1 4/3/1
//        //f 4/3/1 3/4/1 1/1/1
//        //QVector<int> vList;
//        //QVector<int> vtList;
//        //vList << 1 << 2 << 4 << 4 << 3 << 1;
//        //vtList << 1 << 2 << 3 << 3 << 4 << 1;

//        //-start- openGL描画用情報作成
//        QString in_surfaceStr = "Front";

//        //前準備(openGL座標作成)
//        QVector<QVector3D> rectVec; //Front など 1平面（四角形の4頂点をopenGL描画用の並びにした座標リスト
//        3角形2つで構成される rectVec.clear();
//        //if(in_voxSurfaceStrList.at(pnum) == "Bottom"){ //通常座標=Bottom → GL座標=前
//        if(in_voxSurfaceStrList.at(pnum) == "Front"){
//            //GL図形 前
//            rectVec.append(vertexA); //1つ目の三角形
//            rectVec.append(vertexB);
//            rectVec.append(vertexC);
//            rectVec.append(vertexC); //2つ目の三角形
//            rectVec.append(vertexD);
//            rectVec.append(vertexA);
//        }
//        //if(in_voxSurfaceStrList.at(pnum) == "Top"){ //通常座標=Top → GL座標=後
//        if(in_voxSurfaceStrList.at(pnum) == "Back"){
//            //GL図形 後
//            rectVec.append(vertexF); //１つ目の三角形
//            rectVec.append(vertexE);
//            rectVec.append(vertexH);
//            rectVec.append(vertexH); //2つ目の三角形
//            rectVec.append(vertexG);
//            rectVec.append(vertexF);
//        }
//        if(in_voxSurfaceStrList.at(pnum) == "Right"){
//            //GL平面図形 右
//            rectVec.append(vertexB); //１つ目の三角形
//            rectVec.append(vertexF);
//            rectVec.append(vertexG);
//            rectVec.append(vertexG); //2つ目の三角形
//            rectVec.append(vertexC);
//            rectVec.append(vertexB);
//        }
//        if(in_voxSurfaceStrList.at(pnum) == "Left"){
//            //GL平面図形 左
//            rectVec.append(vertexE); //１つ目の三角形
//            rectVec.append(vertexA);
//            rectVec.append(vertexD);
//            rectVec.append(vertexD); //2つ目の三角形
//            rectVec.append(vertexH);
//            rectVec.append(vertexE);
//        }
//        //if(in_voxSurfaceStrList.at(pnum) == "Front"){ //通常座標=Front → GL座標=上
//        if(in_voxSurfaceStrList.at(pnum) == "Top"){
//            //GL平面図形 上
//            rectVec.append(vertexD); //１つ目の三角形
//            rectVec.append(vertexC);
//            rectVec.append(vertexG);
//            rectVec.append(vertexG); //2つ目の三角形
//            rectVec.append(vertexH);
//            rectVec.append(vertexD);
//        }
//        //if(in_voxSurfaceStrList.at(pnum) == "Back"){ //通常座標=Back → GL座標=下
//        if(in_voxSurfaceStrList.at(pnum) == "Bottom"){
//            //GL平面図形 下
//            rectVec.append(vertexE); //１つ目の三角形
//            rectVec.append(vertexF);
//            rectVec.append(vertexB);
//            rectVec.append(vertexB); //2つ目の三角形
//            rectVec.append(vertexA);
//            rectVec.append(vertexE);
//        }

//        //前準備(openGL座標作成) objファイルのvn情報　(法線=面の方向)
//        QVector3D vn_now;
//        vn_now = QVector3D(0, 0, 1);
//        if(in_voxSurfaceStrList.at(pnum) == "Front"){ vn_now = QVector3D(0, 0, 1);  }
//        if(in_voxSurfaceStrList.at(pnum) == "Back"){  vn_now = QVector3D(0, 0, -1); }
//        if(in_voxSurfaceStrList.at(pnum) == "Right"){ vn_now = QVector3D(1, 0, 0);  }
//        if(in_voxSurfaceStrList.at(pnum) == "Left"){  vn_now = QVector3D(-1, 0, 0); }
//        if(in_voxSurfaceStrList.at(pnum) == "Top"){   vn_now = QVector3D(0, 1, 0);  }
//        if(in_voxSurfaceStrList.at(pnum) == "Bottom"){vn_now = QVector3D(0, -1, 0); }

//        for(int shapeCnt=1; shapeCnt<=2; shapeCnt++ ){ //1平面 = 2つの三角形
//            //if(shapeCnt == 1){

//            //
//            QOpenGLTriangle3D_vox triangle;

//            //マテリアル番号 GL描画自体では使わないが、処理判定など別用途で使う
//            triangle.matnum = in_voxMatnum.at(pnum);

//            //色設定
//            //g_GLColors.append(in_voxColors.at(pnum));
//            triangle.color = in_voxColors.at(pnum);

//            // 1平面(四角形)-1つ目の三角形 頂点情報 (objファイルのvt)
//            QVector3D p1, p2, p3;
//            if(shapeCnt == 1){
//                p1 = rectVec.at(0);
//                p2 = rectVec.at(1);
//                p3 = rectVec.at(2);
//            } else if(shapeCnt == 2){
//                p1 = rectVec.at(3);
//                p2 = rectVec.at(4);
//                p3 = rectVec.at(5);
//            }
//            triangle.p1 = p1;
//            triangle.p2 = p2;
//            triangle.p3 = p3;

//            QVector2D p1UV, p2UV, p3UV;
//            //-start- NG QVector2D(0,0) がなぜか抜けてしまうため、vt.at(3)で落ちてしまう。。
//            //if(shapeCnt == 1){
//            //    p1UV = vt.at(0);
//            //    p2UV = vt.at(1);
//            //    p3UV = vt.at(2);
//            //} else if (shapeCnt == 2){
//            //    p1UV = vt.at(2);
//            //    p2UV = vt.at(3);
//            //    p3UV = vt.at(0);
//            //}
//            //-end- NG
//            if(shapeCnt == 1){
//                p1UV = QVector2D(0.0, 0.0);
//                p2UV = QVector2D(1, 0);
//                p3UV = QVector2D(1, 1);
//            } else if (shapeCnt == 2){
//                p1UV = QVector2D(1, 1);
//                p2UV = QVector2D(0, 1);
//                p3UV = QVector2D(0.0, 0.0);
//            }
//            triangle.p1UV = p1UV;
//            triangle.p2UV = p2UV;
//            triangle.p3UV = p3UV;

//            triangle.p1Normal = vn_now;
//            triangle.p2Normal = vn_now;
//            triangle.p3Normal = vn_now;

//            triangles.append(triangle);

//        }
//        //-end- for(int shapeCnt=1; shapeCnt<=2; shapeCnt++ )
//    }
//    //-end- for(int pnum=0; pnum < in_voxXYZVec.count(); pnum++)
//}

void Vox3DForm::zDEBUG_vox_get_voxGraffic_DEBUG01(
    QString in_voxfilepath)    // voxファイルから、座標とマテリアル情報を取得する　1点ごと
{
    // 取得する情報　Meshの位置(始点・終点）　面の方向(vn)　マテリアル番号からの色
    // 　将来的には半透明かどうか(acolorも）

    //[DEBUG]-start-
    g_voxXYZVec.clear();
    g_voxColorVec.clear();
    g_voxAcolorVec.clear();
    g_voxSurfaceStrList.clear();

    g_voxXYZVec.append(QVector3D(1, 1, 1));
    g_voxColorVec.append(QVector3D(1, 0, 0));    // 赤
    g_voxSurfaceStrList.append("Front");

    // g_voxXYZVec.append(QVector3D(1, 1, 2)); //XYZ=1,1,2 向き=Backだと表示がされない。。
    g_voxXYZVec.append(QVector3D(1, 1, 1));
    g_voxColorVec.append(QVector3D(0, 1, 0));    // 緑
    g_voxSurfaceStrList.append("Back");

    g_voxXYZVec.append(QVector3D(1, 1, 1));
    g_voxColorVec.append(QVector3D(1, 0, 1));    // マゼンタ
    g_voxSurfaceStrList.append("Right");

    g_voxXYZVec.append(QVector3D(1, 1, 1));
    g_voxColorVec.append(QVector3D(0, 0, 1));    // 青
    g_voxSurfaceStrList.append("Left");

    g_voxXYZVec.append(QVector3D(1, 1, 1));
    g_voxColorVec.append(QVector3D(1, 1, 0));    // 黄色
    g_voxSurfaceStrList.append("Top");

    g_voxXYZVec.append(QVector3D(1, 1, 1));
    g_voxColorVec.append(QVector3D(0, 1, 1));    // 水色
    g_voxSurfaceStrList.append("Bottom");

    return;
    //[DEBUG]-end-
}

void Vox3DForm::zDEBUG_vox_get_voxGraffic_DEBUG02(
    QString in_voxfilepath)    // voxファイルから、座標とマテリアル情報を取得する　1点ごと
{
    // 取得する情報　Meshの位置(始点・終点）　面の方向(vn)　マテリアル番号からの色
    // 　将来的には半透明かどうか(acolorも） 裏側に色が付かない件、どうするかはこれから考える。。

    //[DEBUG]-start-
    g_voxXYZVec.clear();
    g_voxColorVec.clear();
    g_voxAcolorVec.clear();
    g_voxSurfaceStrList.clear();

    g_voxXYZVec.append(QVector3D(1, 1, 1));
    g_voxColorVec.append(QVector3D(1, 0, 0));    // 赤
    g_voxSurfaceStrList.append("Front");

    g_voxXYZVec.append(QVector3D(1, 1, 2));
    g_voxColorVec.append(QVector3D(0, 1, 0));    // 緑
    g_voxSurfaceStrList.append("Back");

    g_voxXYZVec.append(QVector3D(1, 1, 1));
    g_voxColorVec.append(QVector3D(1, 0, 1));    // マゼンタ
    g_voxSurfaceStrList.append("Right");

    g_voxXYZVec.append(QVector3D(1, 1, 2));
    g_voxColorVec.append(QVector3D(0, 0, 1));    // 青
    g_voxSurfaceStrList.append("Left");

    g_voxXYZVec.append(QVector3D(1, 1, 1));
    g_voxColorVec.append(QVector3D(1, 1, 0));    // 黄色
    g_voxSurfaceStrList.append("Top");

    g_voxXYZVec.append(QVector3D(1, 1, 2));
    g_voxColorVec.append(QVector3D(0, 1, 1));    // 水色
    g_voxSurfaceStrList.append("Bottom");

    return;
    //[DEBUG]-end-
}

// 流用元: QVector<QVector3D>  miWidget::func_get_voxGraffic(QString in_voxfilepath, QString mode)
// //[DEBU]kuroda　shaderで描くための座標・色設定情報を関数呼び出し先にリターンする
void Vox3DForm::func_vox_get_voxGraffic(
    QString in_voxfilepath)    // voxファイル読み込みして、座標とマテリアル情報を取得する　1Meshごと
{
    QString voxfilePath = in_voxfilepath;

    DrawMesh.clear();
    DrawSurface.clear();
    mateNoList.clear();
    mateNoOfMesh.clear();

    // 各メッシュの材質No. を取得

    qDebug() << "[DEBUG]voxRead_makeGLdata.cpp-func_vox_get_voxGraffic: readVox-getMateNumOfMesh start "
                    + QDateTime::currentDateTime().toString("hh:mm:ss");
    // func_vox_getMateNumOfMesh(in_voxfilepath);
    func_vox_getMateNumOfMesh_getVoxLine(in_voxfilepath);
    qDebug() << "[DEBUG]voxRead_makeGLdata.cpp-func_vox_get_voxGraffic: readVox-getMateNumOfMesh end "
                    + QDateTime::currentDateTime().toString("hh:mm:ss");

    QList<int> tmpQList;

    // 描画するメッシュの面情報を取得
    qDebug() << "[DEBUG]voxRead_makeGLdata.cpp-func_vox_get_voxGraffic: 2pointGet-AdjoinMesh start "
                    + QDateTime::currentDateTime().toString("hh:mm:ss");
    // func_vox_GLadd_checkMateNumOfAdjoinMesh();
    func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine();
    qDebug() << "[DEBUG]voxRead_makeGLdata.cpp-func_vox_get_voxGraffic: 2pointGet-AdjoinMesh end "
                    + QDateTime::currentDateTime().toString("hh:mm:ss");
}

void Vox3DForm::func_GL_make_getPointOfMesh_by2point(
    QVector<QOpenGLTriangle3D_vox>& triangles, QVector<QVector3D> in_voxXYZStartVec, QVector<QVector3D> in_voxXYZEndVec,
    QStringList in_voxSurfaceStrList, QVector<int> in_voxMatnum, QVector<QVector3D> in_voxColors,
    QVector<float> in_voxAcolors)    // voxファイルからの取得情報を、openGL描画形式のデータに置き換える
{
    // openGLでの描画単位=1つの三角形ごとの情報作成する。
    // メモリ使用量削減するなら、この処理介さず、
    // qobj3dviewer.h　setTriangleで使う形式にしてしまった方がいいかもしれない。
    // 1三角形ごと個別→頂点情報(v.p1～p3)色情報　は。 2つの三角形で共通→色(colors)、vn(法線=面の向き surfaceStr)。
    // 全三角形で共通→vt(テクスチャ情報)
    triangles.clear();
    QVector<QVector3D> v, vn;
    QVector<QVector2D> vt;

    // voxファイルからの取得情報を、openGL描画形式のデータに置き換える
    for (int pnum = 0; pnum < in_voxXYZStartVec.count(); pnum++) {
        // qDebug() << "[DEBUG]voxRead_makeGLdata.cpp-func_01main_GL_make_getPointOfMesh voxXYZvec=" << QString::number(
        // in_voxXYZVec.at(pnum).x())<< "," <<   QString::number( in_voxXYZVec.at(pnum).y()) << "," <<  QString::number(
        // in_voxXYZVec.at(pnum).z());

        int inS_i, inS_j, inS_k;
        int inE_i, inE_j, inE_k;
        // in_i = 1;
        // in_j = 1;
        // in_k = 1;
        inS_i = in_voxXYZStartVec.at(pnum).x();
        inS_j = in_voxXYZStartVec.at(pnum).z();    // GL:Z軸　j=通常:Z軸
        inS_k = in_voxXYZStartVec.at(pnum).y() * (-1);    // GL:Y軸　k=通常:Y軸 * -1(反転 前がプラス, 後ろがマイナス）
        inE_i = in_voxXYZEndVec.at(pnum).x();
        inE_j = in_voxXYZEndVec.at(pnum).z();    // GL:Z軸　j=通常:Z軸
        inE_k = in_voxXYZEndVec.at(pnum).y() * (-1);    // GL:Y軸　k=通常:Y軸 * -1(反転 前がプラス, 後ろがマイナス）
        // GLfloat nowOP_x, nowOP_y , nowOP_z;
        // float nowOP_x, nowOP_y , nowOP_z;
        // float meshsizeGL_x, meshsizeGL_y, meshsizeGL_z;
        // meshsizeGL_x = 1; //[DEBUG]決め打ち GL座標として
        // meshsizeGL_y = 1;
        // meshsizeGL_z = 1;
        //        nowOP_x = in_i * meshsizeGL_x; //kuroda変更　-0.5 では表示が小さすぎになってしまうための対処
        //        nowOP_y = in_j * meshsizeGL_y; //kuroda変更　-0.5 では表示が小さすぎになってしまうための対処
        //        nowOP_z = in_k * meshsizeGL_z; //kuroda変更　-0.5 では表示が小さすぎになってしまうための対処

        float startX, startY, startZ;
        float endX, endY, endZ;
        startX = inS_i * meshsizeGL_x;
        startY = inS_j * meshsizeGL_z;    // GL:Z軸　j=通常:Z軸
        startZ = inS_k * meshsizeGL_y;    // GL:Y軸　k=通常:Y軸
        endX   = inE_i * meshsizeGL_x;
        endY   = inE_j * meshsizeGL_z;    // GL:Z軸　j=通常:Z軸
        endZ   = inE_k * meshsizeGL_y;    // GL:Y軸　k=通常:Y軸

        //
        // D  __________ C
        //   |          |
        //   |          |
        // A  __________ B
        //
        QVector3D vertexA, vertexB, vertexC, vertexD;

        if (in_voxSurfaceStrList.at(pnum) == "Top" || in_voxSurfaceStrList.at(pnum) == "Bottom") {
            vertexA = QVector3D(startX, startY, startZ);    // TOP 0, 1, 0 - 1, 1 , 1
            vertexB = QVector3D(endX, startY, startZ);      // 1, 1, 1
            vertexC = QVector3D(endX, endY, endZ);          // 1, 1, 1
            vertexD = QVector3D(startX, endY, endZ);        // 0,1,0
        }
        else {
            // Front, Back, Right, Left
            vertexA = QVector3D(startX, startY, startZ);
            vertexB = QVector3D(endX, startY, endZ);
            vertexC = QVector3D(endX, endY, endZ);
            vertexD = QVector3D(startX, endY, startZ);
        }

        // 前準備 openGL座標作成 1メッシュ立方体の頂点座標を定義する
        //        QVector3D vertexA = QVector3D(nowOP_x, nowOP_y, nowOP_z);
        //        QVector3D vertexB = QVector3D(nowOP_x + meshsizeGL_x, nowOP_y, nowOP_z);
        //        QVector3D vertexC = QVector3D(nowOP_x + meshsizeGL_x, nowOP_y + meshsizeGL_y, nowOP_z);
        //        QVector3D vertexD = QVector3D(nowOP_x , nowOP_y + meshsizeGL_y, nowOP_z);
        //        QVector3D vertexE = QVector3D(nowOP_x, nowOP_y, nowOP_z - meshsizeGL_z);
        //        QVector3D vertexF = QVector3D(nowOP_x + meshsizeGL_x, nowOP_y, nowOP_z - meshsizeGL_z);
        //        QVector3D vertexG = QVector3D(nowOP_x + meshsizeGL_x, nowOP_y + meshsizeGL_y, nowOP_z - meshsizeGL_z);
        //        QVector3D vertexH = QVector3D(nowOP_x , nowOP_y + meshsizeGL_y, nowOP_z - meshsizeGL_z);

        //[DEBUG]
        //    vertexA = QVector3D(0.0, 0.0, 0.0); //Front, Bottom, Left
        //    vertexB = QVector3D(1, 0, 0);
        //    vertexC = QVector3D(1, 1, 0);
        //    vertexD = QVector3D(0, 1, 0);
        //    vertexE = QVector3D(0, 0, -1); //Back,  Bottom, Left
        //    vertexF = QVector3D(1, 0, -1);
        //    vertexG = QVector3D(1, 1, -1);
        //    vertexH = QVector3D(0, 1, -1);

        // 前準備(openGL座標作成)  使う対象全部 前後・左右・上下で共通　　後処理trianglesアレイに入れる順番だけ違う。
        vt.append(QVector2D(0.0, 0.0));
        vt.append(QVector2D(1, 0));
        vt.append(QVector2D(1, 1));
        vt.append(QVector2D(0, 1));

        // 前準備(openGL座標作成) objファイルのf情報(v, vn, vt の描画上での並び） 使う対象全部
        // 面：前後・左右・上下で共通 f 1/1/1 2/2/1 4/3/1 f 4/3/1 3/4/1 1/1/1 QVector<int> vList; QVector<int> vtList;
        // vList << 1 << 2 << 4 << 4 << 3 << 1;
        // vtList << 1 << 2 << 3 << 3 << 4 << 1;

        //-start- openGL描画用情報作成
        QString in_surfaceStr = "Front";

        // 前準備(openGL座標作成)
        QVector<QVector3D>
            rectVec;    // Front など 1平面（四角形の4頂点をopenGL描画用の並びにした座標リスト 3角形2つで構成される
        rectVec.clear();
        //-start- 始点・終点を与える場合
        rectVec.append(vertexA);    // 1つ目の三角形
        rectVec.append(vertexB);
        rectVec.append(vertexC);
        rectVec.append(vertexC);    // 2つ目の三角形
        rectVec.append(vertexD);
        rectVec.append(vertexA);
        //-end- 始点・終点を与える場合
        //        //if(in_voxSurfaceStrList.at(pnum) == "Bottom"){ //通常座標=Bottom → GL座標=前
        //        if(in_voxSurfaceStrList.at(pnum) == "Front"){
        //            //GL図形 前
        //            rectVec.append(vertexA); //1つ目の三角形
        //            rectVec.append(vertexB);
        //            rectVec.append(vertexC);
        //            rectVec.append(vertexC); //2つ目の三角形
        //            rectVec.append(vertexD);
        //            rectVec.append(vertexA);
        //        }
        //        //if(in_voxSurfaceStrList.at(pnum) == "Top"){ //通常座標=Top → GL座標=後
        //        if(in_voxSurfaceStrList.at(pnum) == "Back"){
        //            //GL図形 後
        //            rectVec.append(vertexF); //１つ目の三角形
        //            rectVec.append(vertexE);
        //            rectVec.append(vertexH);
        //            rectVec.append(vertexH); //2つ目の三角形
        //            rectVec.append(vertexG);
        //            rectVec.append(vertexF);
        //        }
        //        if(in_voxSurfaceStrList.at(pnum) == "Right"){
        //            //GL平面図形 右
        //            rectVec.append(vertexB); //１つ目の三角形
        //            rectVec.append(vertexF);
        //            rectVec.append(vertexG);
        //            rectVec.append(vertexG); //2つ目の三角形
        //            rectVec.append(vertexC);
        //            rectVec.append(vertexB);
        //        }
        //        if(in_voxSurfaceStrList.at(pnum) == "Left"){
        //            //GL平面図形 左
        //            rectVec.append(vertexE); //１つ目の三角形
        //            rectVec.append(vertexA);
        //            rectVec.append(vertexD);
        //            rectVec.append(vertexD); //2つ目の三角形
        //            rectVec.append(vertexH);
        //            rectVec.append(vertexE);
        //        }
        //        //if(in_voxSurfaceStrList.at(pnum) == "Front"){ //通常座標=Front → GL座標=上
        //        if(in_voxSurfaceStrList.at(pnum) == "Top"){
        //            //GL平面図形 上
        //            rectVec.append(vertexD); //１つ目の三角形
        //            rectVec.append(vertexC);
        //            rectVec.append(vertexG);
        //            rectVec.append(vertexG); //2つ目の三角形
        //            rectVec.append(vertexH);
        //            rectVec.append(vertexD);
        //        }
        //        //if(in_voxSurfaceStrList.at(pnum) == "Back"){ //通常座標=Back → GL座標=下
        //        if(in_voxSurfaceStrList.at(pnum) == "Bottom"){
        //            //GL平面図形 下
        //            rectVec.append(vertexE); //１つ目の三角形
        //            rectVec.append(vertexF);
        //            rectVec.append(vertexB);
        //            rectVec.append(vertexB); //2つ目の三角形
        //            rectVec.append(vertexA);
        //            rectVec.append(vertexE);
        //        }

        // 前準備(openGL座標作成) objファイルのvn情報　(法線=面の方向)
        QVector3D vn_now;
        vn_now = QVector3D(0, 0, 1);
        if (in_voxSurfaceStrList.at(pnum) == "Front") {
            vn_now = QVector3D(0, 0, 1);
        }
        if (in_voxSurfaceStrList.at(pnum) == "Back") {
            vn_now = QVector3D(0, 0, -1);
        }
        if (in_voxSurfaceStrList.at(pnum) == "Right") {
            vn_now = QVector3D(1, 0, 0);
        }
        if (in_voxSurfaceStrList.at(pnum) == "Left") {
            vn_now = QVector3D(-1, 0, 0);
        }
        if (in_voxSurfaceStrList.at(pnum) == "Top") {
            vn_now = QVector3D(0, 1, 0);
        }
        if (in_voxSurfaceStrList.at(pnum) == "Bottom") {
            vn_now = QVector3D(0, -1, 0);
        }

        for (int shapeCnt = 1; shapeCnt <= 2; shapeCnt++) {    // 1平面 = 2つの三角形
            // if(shapeCnt == 1){

            //
            QOpenGLTriangle3D_vox triangle;

            // マテリアル番号 GL描画自体では使わないが、処理判定など別用途で使う
            int matnum      = in_voxMatnum.at(pnum);
            triangle.matnum = matnum;

            // 色設定
            // g_GLColors.append(in_voxColors.at(pnum));
            triangle.color = in_voxColors.at(pnum);

            // 半透明設定
            // 本番OK//triangle.acolor = g_hash_matToAcolor[matnum]; //数値 0.0～1.0
            // (値小さい：透明、値大きい：不透明)　//グローバル変数はVox3DForm::func_tableMaterial_gval()　で定義済
            triangle.acolor = in_voxAcolors.at(pnum);
            //
            //-start- [DEBUG]テスト値を入れる　マテリアル番号20以上は半透明
            // int DEBUGint = in_voxMatnum.at(pnum) ;
            // float tmp_acolor = 1.0;
            // if(in_voxMatnum.at(pnum) > 20){
            //    tmp_acolor = 0.3; //半透明
            //} else {
            //    tmp_acolor = 1.0;
            //}
            // triangle.acolor = tmp_acolor; // 数値 0.0～1.0 (値小さい：透明、値大きい：不透明)
            //-end- [DEBUG]テスト値を入れる　マテリアル番号20以上は半透明

            // 1平面(四角形)-1つ目の三角形 頂点情報 (objファイルのvt)
            QVector3D p1, p2, p3;
            if (shapeCnt == 1) {
                p1 = rectVec.at(0);
                p2 = rectVec.at(1);
                p3 = rectVec.at(2);
            }
            else if (shapeCnt == 2) {
                p1 = rectVec.at(3);
                p2 = rectVec.at(4);
                p3 = rectVec.at(5);
            }
            triangle.p1 = p1;
            triangle.p2 = p2;
            triangle.p3 = p3;

            QVector2D p1UV, p2UV, p3UV;
            //-start- NG QVector2D(0,0) がなぜか抜けてしまうため、vt.at(3)で落ちてしまう。。
            // if(shapeCnt == 1){
            //    p1UV = vt.at(0);
            //    p2UV = vt.at(1);
            //    p3UV = vt.at(2);
            //} else if (shapeCnt == 2){
            //    p1UV = vt.at(2);
            //    p2UV = vt.at(3);
            //    p3UV = vt.at(0);
            //}
            //-end- NG
            if (shapeCnt == 1) {
                p1UV = QVector2D(0.0, 0.0);
                p2UV = QVector2D(1, 0);
                p3UV = QVector2D(1, 1);
            }
            else if (shapeCnt == 2) {
                p1UV = QVector2D(1, 1);
                p2UV = QVector2D(0, 1);
                p3UV = QVector2D(0.0, 0.0);
            }
            triangle.p1UV = p1UV;
            triangle.p2UV = p2UV;
            triangle.p3UV = p3UV;

            triangle.p1Normal = vn_now;
            triangle.p2Normal = vn_now;
            triangle.p3Normal = vn_now;

            triangles.append(triangle);
        }
        //-end- for(int shapeCnt=1; shapeCnt<=2; shapeCnt++ )
    }
    //-end- for(int pnum=0; pnum < in_voxXYZVec.count(); pnum++)
}

void Vox3DForm::
    zDEBUG_input_getPointOfMesh_by2point_v2()    //[DEBUG]用途
                                                 // 長方形面の始点・終点座標から、直方体1個を表示するための値設定
{
    g_voxXYZStartVec.clear();
    g_voxXYZEndVec.clear();
    g_voxColorVec.clear();
    g_voxAcolorVec.clear();
    g_voxSurfaceStrList.clear();
    g_voxMatnumVec.clear();

    g_voxXYZStartVec.append(QVector3D(0, 5, 0));
    g_voxXYZEndVec.append(QVector3D(2, 5, 7));
    g_voxColorVec.append(QVector3D(0, 0, 1));    // 青
    g_voxSurfaceStrList.append("Back");
    g_voxMatnumVec.append(1);

    g_voxXYZStartVec.append(QVector3D(0, 0, 0));
    g_voxXYZEndVec.append(QVector3D(2, 0, 7));
    g_voxColorVec.append(QVector3D(1, 0, 0));    // 赤
    g_voxSurfaceStrList.append("Front");
    g_voxMatnumVec.append(1);

    g_voxXYZStartVec.append(QVector3D(0, 0, 0));
    g_voxXYZEndVec.append(QVector3D(2, 5, 0));
    g_voxColorVec.append(QVector3D(0, 1, 1));    // シアン
    g_voxSurfaceStrList.append("Bottom");
    g_voxMatnumVec.append(1);

    g_voxXYZStartVec.append(QVector3D(2, 0, 0));
    g_voxXYZEndVec.append(QVector3D(2, 5, 7));
    g_voxColorVec.append(QVector3D(1, 1, 0));    // 黄色
    g_voxSurfaceStrList.append("Right");
    g_voxMatnumVec.append(1);

    g_voxXYZStartVec.append(QVector3D(0, 0, 7));
    g_voxXYZEndVec.append(QVector3D(2, 5, 7));
    g_voxColorVec.append(QVector3D(1, 0, 1));    // 紫
    g_voxSurfaceStrList.append("Top");
    g_voxMatnumVec.append(1);

    g_voxXYZStartVec.append(QVector3D(0, 0, 0));
    g_voxXYZEndVec.append(QVector3D(0, 5, 7));
    g_voxColorVec.append(QVector3D(0, 1, 0));    // 緑
    g_voxSurfaceStrList.append("Left");
    g_voxMatnumVec.append(1);
}

// voxファイルのヘッダー情報読み込み + voxファイルを読み込んで、メッシュごとにマテリアルNo.を割り当てる
// //旧処理：1mesh単位で面取得するので処理時間大 void Vox3DForm::func_vox_getMateNumOfMesh(QString in_voxfilepath)
//{
//      //voxfilePath = dragFilePathList.at(0);
//      QString voxfilePath = in_voxfilepath;
//      QStringList read_vox = readTextFileAll(voxfilePath);
//      //qDebug() << "read_vox.size" << read_vox.size();

//     int voxflag = 0;

//     //初期値
//     meshsizeGL = 1.0;

//     //進捗表示のバー
////     QProgressDialog pd(this);
////     pd.setRange(0, read_vox.size());
////     pd.setWindowModality(Qt::WindowModal);
////     pd.setLabelText(TR("Getting Material No. of Mesh"));
////     qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
////     if(pd.wasCanceled()){
////         exit(false);
////     }

//     for(int line_n=0; line_n < read_vox.size(); line_n++){ //-2; CellData(nx * ny * nz)の次行から読み始めた分の相殺
////       pd.setValue(line_n);
//         //if(read_vox.at(line_n).trimmed() == "CellData" ){ continue; }; //kuroda

//         //qDebug() << "miWidget::func_vox_getMateNumOfMesh() read_vox.at(line_n).trimmed()=" <<
//         read_vox.at(line_n).trimmed();

//         if(read_vox.at(line_n).trimmed()=="CellData"){
//             voxflag = 2;
//             line_n = line_n + 1;
//             //qDebug() << "miWidget::func_vox_getMateNumOfMesh() CellData voxflag=" << QString::number(voxflag);
//         }

//         QStringList temp01 = read_vox.at(line_n).split("\t");

//         if(voxflag != 2 && temp01.size()>1){ //これがないとプログラムがクラッシュして、立体画面が起動しない
//             //メッシュサイズ
//             if(temp01.at(1) == "unitlength"){
//                 meshsizeGL = temp01.at(0).toFloat();
//                 voxflag = 1;
//                 line_n = line_n+1;
//             }
//             //マテリアルNo & 名前
//             if(voxflag == 1){
//                 QStringList temp02 = read_vox.at(line_n).split("\t");
//                 mateNoList << temp02.at(0).toInt();
//                 mateNameList << temp02.at(1);
//             }
//         }

//         if(voxflag == 2){
//             nx = (int)read_vox.at(line_n).split(" ").at(0).toInt();
//             ny = (int)read_vox.at(line_n).split(" ").at(2).toInt();
//             nz = (int)read_vox.at(line_n).split(" ").at(4).toInt();
//             voxflag = 3;
//             line_n = line_n+1;
//         }

//         if(voxflag == 3 ){
//             QStringList temp03 = read_vox.at(line_n).split(" ");
//             //qDebug() << "miWidget::func_vox_getMateNumOfMesh() temp03=" << temp03;
//             //materialNoの個数を抽出->cnt
//             for(int i=0; i<temp03.size()/2; i++){
//                 int cnt = temp03.at(2*i+1).toInt();

//                 for(int j=0; j<cnt; j++){
//                     mateNoOfMesh << temp03.at(2*i).toInt();
//                 }
//             }
//         }
//     }
//     //-end- for(int line_n=0; line_n < read_vox.size(); line_n++)

//     //プログレスバーのMAXを100にするため
////     pd.setValue(read_vox.size());

//     //正規化
//     int maxn;

//     //xyzメッシュ数最大値取得
//     maxn = nx;
//     if(ny > maxn){maxn = ny;}
//     if(nz > maxn){maxn = nz;}

//     //正規化の掛け数抽出
//     meshsizeGL = 1.0 / maxn;

//     meshsizeGL_x = meshsizeGL;
//     meshsizeGL_y = meshsizeGL;
//     meshsizeGL_z = meshsizeGL;
//}

void Vox3DForm::func_vox_getMateNumOfMesh_getVoxLine(QString in_voxfilepath)
{
    qDebug() << "[DEBUG]01 Vox3DForm.cpp-func_vox_getMateNumOfMesh_getVoxLine";

    // voxfilePath = dragFilePathList.at(0);
    QString     voxfilePath = in_voxfilepath;
    QStringList read_vox    = readTextFileAll(voxfilePath);
    // qDebug() << "read_vox.size" << read_vox.size();

    int voxflag = 0;

    // 初期値
    meshsizeVox   = 0.02;    // 20um
    meshsizeVox_x = meshsizeVox;
    meshsizeVox_y = meshsizeVox;
    meshsizeVox_z = meshsizeVox;
    meshsizeGL    = 1.0;
    meshsizeGL_x  = meshsizeGL;
    meshsizeGL_y  = meshsizeGL;
    meshsizeGL_z  = meshsizeGL;

    // 3次元配列に入れるために使う
    int cntline = 0;
    int cntX    = 0;
    int cntY    = 0;
    int cntZ    = 0;

    // 進捗表示のバー
    QProgressDialog pd(this);
    pd.setRange(0, read_vox.size());
    pd.setWindowModality(Qt::WindowModal);
    pd.setLabelText(TR("Getting Material No. of Mesh"));
    qApp->processEvents(
        QEventLoop::
            ExcludeUserInputEvents);    // 重い処理をしている時、QtのGUI更新されない場合の対処(Loop=for文、while文内限定で有効??)
    if (pd.wasCanceled()) {
        exit(false);
    }

    for (int line_n = 0; line_n < read_vox.size();
         line_n++) {    //-2; CellData(nx * ny * nz)の次行から読み始めた分の相殺
        pd.setValue(line_n);
        // if(read_vox.at(line_n).trimmed() == "CellData" ){ continue; }; //kuroda

        // qDebug() << "miWidget::func_vox_getMateNumOfMesh() read_vox.at(line_n).trimmed()=" <<
        // read_vox.at(line_n).trimmed();

        if (read_vox.at(line_n).trimmed() == "CellData") {
            voxflag = 2;
            line_n  = line_n + 1;
            // qDebug() << "miWidget::func_vox_getMateNumOfMesh() CellData voxflag=" << QString::number(voxflag);
        }

        QStringList temp01 = read_vox.at(line_n).split("\t");

        if (voxflag != 2 && temp01.size() > 1) {    // これがないとプログラムがクラッシュして、立体画面が起動しない
            // メッシュサイズ
            if (temp01.at(1) == "unitlength") {
                meshsizeVox   = temp01.at(0).toFloat();
                meshsizeVox_x = meshsizeVox;
                meshsizeVox_y = meshsizeVox;
                meshsizeVox_z = meshsizeVox;
                meshsizeGL    = temp01.at(0).toFloat();
                voxflag       = 1;
                line_n        = line_n + 1;
            }
            // マテリアルNo & 名前
            if (voxflag == 1) {
                QStringList temp02 = read_vox.at(line_n).split("\t");
                mateNoList << temp02.at(0).toInt();
                mateNameList << temp02.at(1);
            }
        }

        if (voxflag == 2) {
            nx      = (int)read_vox.at(line_n).split(" ").at(0).toInt();
            ny      = (int)read_vox.at(line_n).split(" ").at(2).toInt();
            nz      = (int)read_vox.at(line_n).split(" ").at(4).toInt();
            voxflag = 3;

            //-start-  3次元配列の初期化 g_aryVox[z][y][x] として使う。(3次元配列でも処理速度遅くなかったので使う。)
            g_aryVox = new int**[nz];
            for (int z = 0; z < nz; z++) {
                g_aryVox[z] = new int*[ny];
                for (int y = 0; y < ny; y++) {
                    g_aryVox[z][y] = new int[nx]();    //()でデータがないときは0で全部初期化する
                }
            }

            line_n = line_n + 1;
        }

        if (voxflag == 3) {
            QStringList temp03 = read_vox.at(line_n).split(" ");

            // materialNoの個数を抽出->cnt
            cntX = 0;
            for (int i = 0; i < temp03.size() / 2; i++) {
                int cnt = temp03.at(2 * i + 1).toInt();

                for (int j = 0; j < cnt; j++) {
                    mateNoOfMesh << temp03.at(2 * i).toInt();
                    g_aryVox[cntZ][cntY][cntX] = temp03.at(2 * i).toInt();
                    cntX++;
                }
            }
            cntY = (cntline + 1) % ny;
            if (cntY == 0) {
                cntZ++;
            }
            cntline++;
        }
    }
    //-end- for(int line_n=0; line_n < read_vox.size(); line_n++)

    // プログレスバーのMAXを100にするため
    pd.setValue(read_vox.size());

    // 正規化
    int maxn;

    // xyzメッシュ数最大値取得
    maxn = nx;
    if (ny > maxn) {
        maxn = ny;
    }
    if (nz > maxn) {
        maxn = nz;
    }

    // 正規化の掛け数抽出
    meshsizeGL = 1.0 / maxn;

    meshsizeGL_x = meshsizeGL;
    meshsizeGL_y = meshsizeGL;
    meshsizeGL_z = meshsizeGL;
}

// void Vox3DForm::func_vox_GLadd_checkMateNumOfAdjoinMesh()
//{
//     g_voxXYZVec.clear();
//     g_voxColorVec.clear();
//     g_voxAcolorVec.clear();
//     g_voxMatnumVec.clear();
//     g_voxSurfaceStrList.clear();
//     g_voxMatToPnumHash.clear();

//    int i,j,k;
//    meshAll = nx*ny*nz;
//    unsigned int nowMeshNo = 0;
//    unsigned int chkMeshNo = 0;
//    int drawMeshFlag;

//    //-start- kuroda commentout
//    //QProgressDialog pd(this);
//    //pd.setRange(0, meshAll/1000);
//    //pd.setWindowModality(Qt::WindowModal);
//    //pd.setLabelText(TR("Selecting Drawing Mesh"));
//    //qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
//    //if(pd.wasCanceled()){
//    //    exit(false);
//    //}
//    //-end- kuroda commentout

//    QHash<int, QVector3D> matnumToColorvec = func_GL_defineColor_byTableMaterial();
//    //マテリアルテーブルからハッシュ作成する。　マテリアル番号と色番号 QVector3D(R, G, B) を紐づける
//    //[DEBUG]foreach(int key, matnumToColorvec.keys()){ qDebug() <<
//    "Vox3DForm::func_vox_GLadd_checkMateNumOfAdjoinMesh() matnumToColorvec key=" << QString::number(key) << "
//    value=matnumToColorvec" << matnumToColorvec.value(key) ;}

//    int pnum = 0;
//    for(k=0; k<nz; k++){
//        for(j=0; j<ny; j++){
//            for(i=0; i<nx; i++){
//                drawMeshFlag = 0;

//                if(mateNoOfMesh[nowMeshNo] == 0 ){
//                    nowMeshNo++;
//                    continue; //マテリアル番号=0 の場合はAir とみなして表示しない。以下の処理をスキップする
//                }
//                //マテリアル色
//                QVector3D tmp_colorvec = QVector3D(1, 0, 0); //赤
//                int matnum = mateNoOfMesh[nowMeshNo];
//                if(matnumToColorvec.contains(matnum)){
//                    tmp_colorvec = matnumToColorvec.value(matnum);
//                }

//                //QString strXYZ = QString::number(i) + "," + QString::number(j) + "," + QString::number(k) ;

//                //Bottom, Top... などの面方向は、GL描画するときの方向
//                //    通常座標 (x, y, z) のまま、設定すると　openGLでは、Y軸と、Z軸が反対に表示されてしまうが
//                // 　  別処理として関数func_01main_GL_make_getPointOfMeshでGL座標 (x, -y, -z)
//                として入れなおして,反対に表示されないようにしている
//                //
//                //通常座標　下　→　GL座標 Bottom
//                chkMeshNo = nowMeshNo - nx * ny;
//                if(k ==0 || (k !=0) && (mateNoOfMesh[nowMeshNo] != mateNoOfMesh[chkMeshNo])){
//                    g_voxSurfaceStrList.append("Bottom");

//                    //以下, 共通処理
//                    g_voxXYZVec.append(QVector3D(i, j, k)); //openGL座標なので (glX, glY, glZ) = 通常座標(x, z, -y)
//                    g_voxMatnumVec.append(mateNoOfMesh[nowMeshNo]); //マテリアル番号
//                    //g_voxColorVec.append(func_GL_defineColor(mateNoOfMesh[nowMeshNo])); //例　QVector3D(0, 1, 0)
//                    緑など設定 g_voxColorVec.append(tmp_colorvec); //マテリアル色
//                    //マテリアル番号(matnum) と g_voxXYZvecインデックス番号(pnum)の紐づけ
//                    g_voxMatToPnumHash.insert(mateNoOfMesh[nowMeshNo], pnum);//マテリアル番号(matnum) と
//                    g_voxXYZvecインデックス番号(pnum)の紐づけ。　GUI操作色切替え・表示切替えのため。 pnum++;
//                }

//                //通常座標　上　→　GL座標 Top
//                chkMeshNo = nowMeshNo + nx * ny;
//                if(k ==nz-1 || (k != nz-1) && (mateNoOfMesh[nowMeshNo] != mateNoOfMesh[chkMeshNo])){
//                    g_voxSurfaceStrList.append("Top");
//                    //以下, 共通処理
//                    g_voxXYZVec.append(QVector3D(i, j, k));
//                    g_voxMatnumVec.append(mateNoOfMesh[nowMeshNo]);
//                    //g_voxColorVec.append(func_GL_defineColor(mateNoOfMesh[nowMeshNo]));
//                    g_voxColorVec.append(tmp_colorvec); //マテリアル色
//                    g_voxMatToPnumHash.insert(mateNoOfMesh[nowMeshNo], pnum);//マテリアル番号(matnum) と
//                    g_voxXYZvecインデックス番号(pnum)の紐づけ。　GUI操作色切替え・表示切替えのため。 pnum++;
//                }

//                //通常座標　右　=　GL座標 right
//                chkMeshNo = nowMeshNo + 1;
//                if(i ==nx-1 || (i != nx-1) && (mateNoOfMesh[nowMeshNo] != mateNoOfMesh[chkMeshNo])){
//                    g_voxSurfaceStrList.append("Right");
//                    //以下, 共通処理
//                    g_voxXYZVec.append(QVector3D(i, j, k));
//                    g_voxMatnumVec.append(mateNoOfMesh[nowMeshNo]);
//                    //g_voxColorVec.append(func_GL_defineColor(mateNoOfMesh[nowMeshNo]));
//                    g_voxColorVec.append(tmp_colorvec); //マテリアル色
//                    g_voxMatToPnumHash.insert(mateNoOfMesh[nowMeshNo], pnum);//マテリアル番号(matnum) と
//                    g_voxXYZvecインデックス番号(pnum)の紐づけ。　GUI操作色切替え・表示切替えのため。 pnum++;
//                }

//                //通常座標　左　=　GL座標 left
//                chkMeshNo = nowMeshNo - 1;
//                if(i ==0 || (i != 0) && (mateNoOfMesh[nowMeshNo] != mateNoOfMesh[chkMeshNo])){
//                    g_voxSurfaceStrList.append("Left");
//                    //以下, 共通処理
//                    g_voxXYZVec.append(QVector3D(i, j, k));
//                    g_voxMatnumVec.append(mateNoOfMesh[nowMeshNo]);
//                    //g_voxColorVec.append(func_GL_defineColor(mateNoOfMesh[nowMeshNo]));
//                    g_voxColorVec.append(tmp_colorvec); //マテリアル色
//                    g_voxMatToPnumHash.insert(mateNoOfMesh[nowMeshNo], pnum);//マテリアル番号(matnum) と
//                    g_voxXYZvecインデックス番号(pnum)の紐づけ。　GUI操作色切替え・表示切替えのため。 pnum++;
//                }

//                //通常座標　後　　→　　GL座標 Back
//                chkMeshNo = nowMeshNo + nx;
//                if(j == ny-1 || (j != ny-1) && (mateNoOfMesh[nowMeshNo] != mateNoOfMesh[chkMeshNo])){
//                    g_voxSurfaceStrList.append("Back");
//                    //以下, 共通処理
//                    g_voxXYZVec.append(QVector3D(i, j, k));
//                    g_voxMatnumVec.append(mateNoOfMesh[nowMeshNo]);
//                    //g_voxColorVec.append(func_GL_defineColor(mateNoOfMesh[nowMeshNo]));
//                    g_voxColorVec.append(tmp_colorvec); //マテリアル色
//                    g_voxMatToPnumHash.insert(mateNoOfMesh[nowMeshNo], pnum);//マテリアル番号(matnum) と
//                    g_voxXYZvecインデックス番号(pnum)の紐づけ。　GUI操作色切替え・表示切替えのため。 pnum++;
//                }

//                //通常座標　前　　→　GL座標 Front
//                chkMeshNo = nowMeshNo - nx;
//                if(j == 0 || (j != 0) && (mateNoOfMesh[nowMeshNo] != mateNoOfMesh[chkMeshNo])){
//                    g_voxSurfaceStrList.append("Front");
//                    //以下, 共通処理
//                    g_voxXYZVec.append(QVector3D(i, j, k));
//                    g_voxMatnumVec.append(mateNoOfMesh[nowMeshNo]);
//                    //g_voxColorVec.append(func_GL_defineColor(mateNoOfMesh[nowMeshNo]));
//                    g_voxColorVec.append(tmp_colorvec); //マテリアル色
//                    g_voxMatToPnumHash.insert(mateNoOfMesh[nowMeshNo], pnum);//マテリアル番号(matnum) と
//                    g_voxXYZvecインデックス番号(pnum)の紐づけ。　GUI操作色切替え・表示切替えのため。 pnum++;
//                }

//                nowMeshNo++;

//            }//-end- for i
//        }//-end- for j
//    }//-end- for k

//}

void Vox3DForm::func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine()
{
    //--------------------
    // 前準備
    //--------------------
    // qDebug() << "[DEBUG]01 vox3Dsub02.cpp-func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxline()";

    g_voxXYZVec.clear();
    g_voxXYZStartVec.clear();
    g_voxXYZEndVec.clear();
    g_voxColorVec.clear();
    g_voxAcolorVec.clear();
    g_voxMatnumVec.clear();
    g_voxSurfaceStrList.clear();
    g_voxMatToPnumHash.clear();
    g_pnum = 0;    // g_voxMatToPnumHash用途　マテリアル番号(matnum) と g_voxXYZvecインデックス番号(pnum)の紐づけ。

    // int i,j,k;
    meshAll = nx * ny * nz;
    // unsigned int nowMeshNo = 0;
    // unsigned int chkMeshNo = 0;
    // int drawMeshFlag;

    //-start- kuroda commentout
    // QProgressDialog pd(this);
    // pd.setRange(0, meshAll/1000);
    // pd.setWindowModality(Qt::WindowModal);
    // pd.setLabelText(TR("Selecting Drawing Mesh"));
    // qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    // if(pd.wasCanceled()){
    //    exit(false);
    //}
    //-end- kuroda commentout

    //    QHash<int, QVector3D> matnumToColorvecHash = func_GL_defineColor_byTableMaterial();
    //    //マテリアルテーブルからハッシュ作成する。　マテリアル番号と色番号 QVector3D(R, G, B) を紐づける
    //    //[DEBUG]foreach(int key, matnumToColorvec.keys()){ qDebug() <<
    //    "Vox3DForm::func_vox_GLadd_checkMateNumOfAdjoinMesh() matnumToColorvec key=" << QString::number(key) << "
    //    value=matnumToColorvec" << matnumToColorvec.value(key) ;}

    //-start- 3次元配列として処理する。 3次元配列でも遅くなかったので。
    // qDebug() << "[DEBUG]-start- func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine(): start copy-g_aryVox " +
    // QDateTime::currentDateTime().toString("hh:mm:ss"); int g_aryVox[nz][ny][ny]; int ***g_aryVox;
    // ↓　関数　func_vox_getMateNumOfMesh_getVoxLine　に移動
    //    g_aryVox= new int**[nz];
    //    for(int z=0; z<nz; z++){
    //        g_aryVox[z] = new int*[ny];
    //        for(int y=0; y<ny; y++){
    //            g_aryVox[z][y] = new int[nx](); //()でデータがないときは0で全部初期化する
    //        }
    //    }
    //    //qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine() g_aryVox[z, y, x]:";
    //    for(int k=0; k<nz; k++){
    //        for(int j=0; j<ny; j++){
    //            QString tmpstr = "";
    //            for(int i=0; i<nx; i++){
    //                g_aryVox[k][j][i] = mateNoOfMesh[k*ny*nx + j*nx + i ];
    //                tmpstr += " " + QString::number(g_aryVox[k][j][i]);
    //            }
    //            //qDebug() << tmpstr;
    //        }
    //    }
    // ↑　関数　func_vox_getMateNumOfMesh_getVoxLine　に移動
    // qDebug() << "[DEBUG]-end- func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine(): start copy-g_aryVox " +
    // QDateTime::currentDateTime().toString("hh:mm:ss"); -end- for-speed01

    //--------------------
    // 面情報取得　始点・終点を取得する
    //--------------------
    // 　描画順番に注意する必要あり。　Bottomが先、Frontを後にしないと半透明が効かない。
    // openGLの描画順番ルール
    //  * 1. 不透明と半透明なら、不透明を必ず先に描画する
    //  * 2. 半透明のものが複数あるなら、奥にあるものから描画する
    //
    // プログレスダイアログ　進捗表示のバー
    QProgressDialog pd(this);
    pd.setRange(0, 6);
    pd.setWindowModality(Qt::WindowModal);
    pd.setLabelText(TR("Getting Surface"));
    // qApp->processEvents(QEventLoop::ExcludeUserInputEvents);　//重い処理をしている時、QtのGUI更新されない場合の対処(Loop=for文、while文内限定で有効??)
    qApp->processEvents();    // 重い処理をしている時、QtのGUI更新されない場合の対処 Loop(=for文に限らないようにする)
                              // 有効かはまだ不明。。
    if (pd.wasCanceled()) {
        exit(false);
    }
    pd.setValue(1);

    func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine_Bottom();    // Bottom, Top
                                                                    // は処理内容なので、できれば関数まとめたい。。後ほど
    qApp->processEvents();    // 重い処理をしている時、QtのGUI更新されない場合の対処
    pd.setLabelText(TR("Getting Surface(1/6) Bottom"));
    pd.setValue(1);

    func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine_Top();
    qApp->processEvents();
    pd.setLabelText(TR("Getting Surface(2/6) Top"));
    pd.setValue(1);

    func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine_Left();
    qApp->processEvents();
    pd.setLabelText(TR("Getting Surface(3/6) Left"));
    pd.setValue(1);

    func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine_Right();
    qApp->processEvents();
    pd.setLabelText(TR("Getting Surface(4/6) Right"));
    pd.setValue(1);

    func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine_Back();
    qApp->processEvents();
    pd.setLabelText(TR("Getting Surface(5/6) Back"));
    pd.setValue(1);

    func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine_Front();
    qApp->processEvents();
    pd.setLabelText(TR("Getting Surface(6/6) Front"));
    pd.setValue(1);

    //------------------------------
    //[DEBUG]-start- 展開後のvox内容表示 本番時は必ず削除する　（処理が重くなって動かなくなるので)
    //    qDebug() << "[DEBUG]-start- Vox3DForm.cpp-func_01main_GL_makeGL3DfromVox() g_aryvox=";
    //    QString tmpstr="";
    //    for(int z=0; z<nz; z++){
    //        for(int y=0; y<ny; y++){
    //            for(int x=0; x<nx; x++){
    //                tmpstr += " " + QString::number(g_aryVox[z][y][x]);
    //            }
    //            qDebug() << tmpstr;
    //            tmpstr = "";
    //        }
    //        qDebug() << ""; //改行スペース空ける
    //    }
    //    qDebug() << "[DEBUG]-end- Vox3DForm.cpp-func_01main_GL_makeGL3DfromVox() g_aryvox end.";
    //[DEBUG]-end- 展開後のvox内容表示

    //------------------------------
    // メモリ解放 3D
    for (int z = 0; z < nz; z++) {
        for (int y = 0; y < ny; y++) {
            delete[] g_aryVox[z][y];    //()でデータがないときは0で全部初期化している
        }
        delete[] g_aryVox[z];
    }
    delete[] g_aryVox;
}

void Vox3DForm::func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine_Front()
{
    // 間違えて編集しないようにコメントアウト
    //     //qDebug() << "[DEBUG]01 vox3Dsub02.cpp-func_vox_GLadd_checkMateNumOfAdjoinMesh()";

    //    g_voxXYZVec.clear();
    //    g_voxXYZStartVec.clear();
    //    g_voxXYZEndVec.clear();
    //    g_voxColorVec.clear();
    //    g_voxAcolorVec.clear();
    //    g_voxMatnumVec.clear();
    //    g_voxSurfaceStrList.clear();
    //    g_voxMatToPnumHash.clear();

    //    int i,j,k;
    //    meshAll = nx*ny*nz;
    //    unsigned int nowMeshNo = 0;
    //    unsigned int chkMeshNo = 0;
    //    int drawMeshFlag;

    //    //-start- kuroda commentout
    //    //QProgressDialog pd(this);
    //    //pd.setRange(0, meshAll/1000);
    //    //pd.setWindowModality(Qt::WindowModal);
    //    //pd.setLabelText(TR("Selecting Drawing Mesh"));
    //    //qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    //    //if(pd.wasCanceled()){
    //    //    exit(false);
    //    //}
    //    //-end- kuroda commentout

    //    QHash<int, QVector3D> matnumToColorvecHash = func_GL_defineColor_byTableMaterial();
    //    //マテリアルテーブルからハッシュ作成する。　マテリアル番号と色番号 QVector3D(R, G, B) を紐づける
    //    //[DEBUG]foreach(int key, matnumToColorvec.keys()){ qDebug() <<
    //    "Vox3DForm::func_vox_GLadd_checkMateNumOfAdjoinMesh() matnumToColorvec key=" << QString::number(key) << "
    //    value=matnumToColorvec" << matnumToColorvec.value(key) ;}

    //    //-start- 3次元配列として処理する。 3次元配列でも遅くなかったので。
    //    //qDebug() << "[DEBUG]-start- func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine(): start copy-g_aryVox " +
    //    QDateTime::currentDateTime().toString("hh:mm:ss");
    //    //int g_aryVox[nz][ny][ny];
    //    int ***g_aryVox;
    //    g_aryVox= new int**[nz];
    //    for(int z=0; z<nz; z++){
    //        g_aryVox[z] = new int*[ny];
    //        for(int y=0; y<ny; y++){
    //            g_aryVox[z][y] = new int[nx](); //()でデータがないときは0で全部初期化する
    //        }
    //    }
    //    //qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine() g_aryVox[z, y, x]:";
    //    for(int k=0; k<nz; k++){
    //        for(int j=0; j<ny; j++){
    //            QString tmpstr = "";
    //            for(int i=0; i<nx; i++){
    //                g_aryVox[k][j][i] = mateNoOfMesh[k*ny*nx + j*nx + i ];
    //                tmpstr += " " + QString::number(g_aryVox[k][j][i]);
    //            }
    //            //qDebug() << tmpstr;
    //        }
    //    }
    //    //qDebug() << "[DEBUG]-end- func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine(): start copy-g_aryVox " +
    //    QDateTime::currentDateTime().toString("hh:mm:ss");
    //    //-end- for-speed01

    int i, j, k;
    meshAll                         = nx * ny * nz;
    unsigned int          nowMeshNo = 0;
    unsigned int          chkMeshNo = 0;
    int                   drawMeshFlag;
    QHash<int, QVector3D> matnumToColorvecHash =
        func_GL_defineColor_byTableMaterial();    // マテリアルテーブルからハッシュ作成する。　マテリアル番号と色番号
                                                  // QVector3D(R, G, B) を紐づける
    //[DEBUG]foreach(int key, matnumToColorvec.keys()){ qDebug() <<
    //"Vox3DForm::func_vox_GLadd_checkMateNumOfAdjoinMesh() matnumToColorvec key=" << QString::number(key) << "
    // value=matnumToColorvec" << matnumToColorvec.value(key) ;}

    QList<QVector3D> startList, endList;          // 全体のLineList( Bottom, Front, Back, Left Right );
    QList<int>       startMatList, endMatList;    // 全体のLineList( Bottom, Front, Back, Left Right );

    //-----------------------------
    //-start- Front
    QList<QVector3D> lineStartFrontList, lineEndFrontList;
    QList<int>       lineStartMatFrontList, lineEndMatFrontList;    // 面として連続するかの判定用
    QList<int>       lineflag1FrontList;    // 面として連続するかの判定用 QList<int> startFrontList;
    int**            aryXZfront = new int*[nz];
    for (int z = 0; z < nz; z++) {
        aryXZfront[z] = new int[nx]();    // 初期化　全部ゼロを入れる
    }

    QList<QVector3D> faceStartTopList, faceStartTopMatList;    // 面として連続するかの判定用
    QList<int>       frontSameStartList, frontSameEndList;
    for (int j = 0; j < ny; j++) {
        //-start- Front
        //-start- 始点終点取得の前処理 ①Y方向1つ前との差異判定 (描く、描かない）。　②Z方向　行単位で同一かの判定
        int        flag_sameOneline = 0;
        int        flag_same        = 0;    // 2行以上、同一か（始点・終点判定のため)
        QList<int> frontSameStartList, frontSameEndList;
        for (int z = 0; z < nz; z++) {
            flag_sameOneline = 1;    // 1つ下の列と同一か（始点・終点判定のため)
            for (int x = 0; x < nx; x++) {
                // 　Frontとの差異判定 (描く、描かない）
                aryXZfront[z][x] = g_aryVox[z][j][x];    // デフォルト
                if (j > 0) {
                    if (g_aryVox[z][j][x] == g_aryVox[z][j - 1][x]) {
                        aryXZfront[z][x] = -1;    // y方向1つ前が同じマテリアルの場合、描かないものとして 0 にする
                    }
                }
                //                if(j==0){
                //                    qDebug() << "[DEBUG]aryXZfront z=" << QString::number(z)  << " j=" <<
                //                    QString::number(j) << " x=" << QString::number(x)  << " aryXZfront[z][x]=" <<
                //                    QString::number(aryXZfront[z][x]) ;
                //                }

                // 1つ下の列と同一か（始点・終点判定のため)
                if (z > 0) {
                    if (aryXZfront[z][x] != aryXZfront[z - 1][x]) {
                        flag_sameOneline = 0;
                    }
                    if (x == nx - 1) {
                        // qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine()　j=" <<
                        // QString::number(j) << " z=" << QString::number(z) <<  " flag_same=" <<
                        // QString::number(flag_same) << " flag_sameOneline=" << QString::number(flag_sameOneline)  ;

                        if (flag_same == 0 && flag_sameOneline == 1) {
                            frontSameStartList << z - 1;
                            flag_same = 1;
                        }

                        if (flag_same == 1 && flag_sameOneline != 1) {
                            frontSameEndList << z - 1;
                            flag_same = 0;
                        }

                        if (flag_same == 1 && z == nz - 1) {
                            frontSameEndList << z;
                        }
                    }
                }
            }
            //-end- for(int x=0; x<nx; x++)
        }
        //-end- for(int z=0; z<nz; z++)

        //-start-
        //[DEBUG]デバッグ用表示　※本番時は必ず消す　残していると本番実際データだとループで　時間大・動作重くなるので
        // qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine() frontSameStartList=" <<
        // frontSameStartList; qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine()
        // frontSameEndList=" << frontSameEndList; if(j == 0){
        //    qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine() XZview j=" << QString::number(j)
        //    ; for(int z=nz-1; z>=0; z--){ //DEBUG見やすくするため　vox上面から
        //        QString tmpstr = "";
        //        for(int x=0; x<nx; x++){
        //            tmpstr += " " + QString::number(aryXZfront[z][x]);
        //        }
        //        qDebug() << tmpstr;
        //    }
        //}
        // qDebug() <<""; //改行
        //-end-
        //[DEBUG]デバッグ用表示　※本番時は必ず消す　残していると本番実際データだとループで　時間大・動作重くなるので

        // 1行単位の長方形　始点・終点判定
        for (int z = 0; z < nz; z++) {
            for (int x = 0; x < nx; x++) {
                // 終点高さ  if(frontSameStartList.indexOf(j) > 0 ){ row = frontSameEndList.indexOf(j); }

                //---始点判定：
                int tmp_flag = 0;
                if (x == 0) {
                    tmp_flag = 1;
                }    // 左端の場合
                if (x > 0) {
                    if (aryXZfront[z][x] != aryXZfront[z][x - 1]) {
                        tmp_flag = 1;
                    }    // 左の材質と違うとき
                }
                if (aryXZfront[z][x] == -1) {
                    tmp_flag = 0;
                }
                if (tmp_flag == 1) {
                    // lineStartFrontList << QVector3D(i, j, k);
                    // lineStartMatFrontList << g_aryVox[k][j][i];
                    // lineflag1FrontList << j; //面として連続するかの判定用
                    startList << QVector3D(x, j, z);
                    int matnum = g_aryVox[z][j][x];
                    startMatList << matnum;
                    g_voxMatToPnumHash.insert(
                        matnum,
                        g_pnum);    // マテリアル番号(matnum) と
                                    // g_voxXYZvecインデックス番号(pnum)の紐づけ。　GUI操作色切替え・表示切替えのため。
                    g_pnum++;
                    g_voxXYZStartVec.append(QVector3D(x, j, z));
                    //
                    QVector3D tmpColorVec = QVector3D(1, 0, 0);    // 赤
                    if (matnumToColorvecHash.contains(matnum)) {
                        tmpColorVec = matnumToColorvecHash[matnum];
                    }
                    else {
                        if (startList.count() % 2 == 0) {
                            tmpColorVec = QVector3D(0, 1, 0);
                        }    // 緑
                        if (startList.count() % 3 == 0) {
                            tmpColorVec = QVector3D(0, 0, 1);
                        }    // 青
                        if (startList.count() % 4 == 0) {
                            tmpColorVec = QVector3D(1, 1, 0);
                        }
                        if (startList.count() % 5 == 0) {
                            tmpColorVec = QVector3D(1, 0, 1);
                        }
                    }
                    g_voxColorVec.append(tmpColorVec);
                    g_voxSurfaceStrList.append("Front");
                    g_voxMatnumVec.append(matnum);
                    g_voxAcolorVec.append(g_hash_matToAcolor[matnum]);
                }
                //---終点判定：
                tmp_flag = 0;
                if (x == nx - 1) {
                    tmp_flag = 1;
                }    // 右端の場合
                if (x < nx - 1) {
                    if (aryXZfront[z][x] != aryXZfront[z][x + 1]) {
                        tmp_flag = 1;
                    }    // 右の材質と違うとき
                }
                if (aryXZfront[z][x] == -1) {
                    tmp_flag = 0;
                }
                if (tmp_flag == 1) {
                    // lineEndFrontList << QVector3D(i, j, k);
                    // lineEndMatFrontList << g_aryVox[k][j][i];
                    // lineflag1FrontList << j;
                    int tmpZ = z;
                    if (frontSameStartList.indexOf(z) > -1) {
                        tmpZ = frontSameEndList.at(frontSameStartList.indexOf(z));
                    }
                    endList << QVector3D(x, j, tmpZ);
                    endMatList << g_aryVox[tmpZ][j][x];

                    g_voxXYZEndVec.append(QVector3D(x + 1, j, tmpZ + 1));    // 図形にするときは、幅なので+1 する
                }
            }
            //-end- for(int x=0; x<nx; x++)

            // frontSameStartList　に含まれている場合は startListの行から EndListの行まで処理スキップする
            int tmpIndex = frontSameStartList.indexOf(z);
            if (tmpIndex > -1) {
                z = frontSameEndList.at(tmpIndex);
            }
        }
        //-end- for(int z=0; z<nz; z++)
        // if(j==0){
        //    qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() j=" << QString::number(j) << " startList=" <<
        //    startList; qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() j=" << QString::number(j) << "
        //    endList=" << endList;
        //}
    }
    //-end- for(int j=1; j<ny-1; j++)
    // qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() startList.count=" <<
    // QString::number(startList.count());

    //    //------------------------------
    //    //メモリ解放 3D
    //    for(int z=0; z<nz; z++){
    //        for(int y=0; y<ny; y++){
    //            delete[] g_aryVox[z][y] ; //()でデータがないときは0で全部初期化している
    //        }
    //        delete[] g_aryVox[z];
    //    }
    //    delete[] g_aryVox;

    // メモリ解放 2D
    for (int z = 0; z < nz; z++) {
        delete[] aryXZfront[z];
    }
    delete[] aryXZfront;

    // DEBUG 間違えて編集しないようにコメントアウト
}

void Vox3DForm::func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine_Back()
{
    //    //qDebug() << "[DEBUG]01 vox3Dsub02.cpp-func_vox_GLadd_checkMateNumOfAdjoinMesh()";

    //    g_voxXYZVec.clear();
    //    g_voxXYZStartVec.clear();
    //    g_voxXYZEndVec.clear();
    //    g_voxColorVec.clear();
    //    g_voxAcolorVec.clear();
    //    g_voxMatnumVec.clear();
    //    g_voxSurfaceStrList.clear();
    //    g_voxMatToPnumHash.clear();

    //    int i,j,k;
    //    meshAll = nx*ny*nz;
    //    unsigned int nowMeshNo = 0;
    //    unsigned int chkMeshNo = 0;
    //    int drawMeshFlag;

    //    //-start- kuroda commentout
    //    //QProgressDialog pd(this);
    //    //pd.setRange(0, meshAll/1000);
    //    //pd.setWindowModality(Qt::WindowModal);
    //    //pd.setLabelText(TR("Selecting Drawing Mesh"));
    //    //qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    //    //if(pd.wasCanceled()){
    //    //    exit(false);
    //    //}
    //    //-end- kuroda commentout

    //    QHash<int, QVector3D> matnumToColorvecHash = func_GL_defineColor_byTableMaterial();
    //    //マテリアルテーブルからハッシュ作成する。　マテリアル番号と色番号 QVector3D(R, G, B) を紐づける
    //    //[DEBUG]foreach(int key, matnumToColorvec.keys()){ qDebug() <<
    //    "Vox3DForm::func_vox_GLadd_checkMateNumOfAdjoinMesh() matnumToColorvec key=" << QString::number(key) << "
    //    value=matnumToColorvec" << matnumToColorvec.value(key) ;}

    //    //-start- 3次元配列として処理する。 3次元配列でも遅くなかったので。
    //    //qDebug() << "[DEBUG]-start- func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine(): start copy-g_aryVox " +
    //    QDateTime::currentDateTime().toString("hh:mm:ss");
    //    //int g_aryVox[nz][ny][ny];
    //    int ***g_aryVox;
    //    g_aryVox= new int**[nz];
    //    for(int z=0; z<nz; z++){
    //        g_aryVox[z] = new int*[ny];
    //        for(int y=0; y<ny; y++){
    //            g_aryVox[z][y] = new int[nx](); //()でデータがないときは0で全部初期化する
    //        }
    //    }
    //    //qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine() g_aryVox[z, y, x]:";
    //    for(int k=0; k<nz; k++){
    //        for(int j=0; j<ny; j++){
    //            QString tmpstr = "";
    //            for(int i=0; i<nx; i++){
    //                g_aryVox[k][j][i] = mateNoOfMesh[k*ny*nx + j*nx + i ];
    //                tmpstr += " " + QString::number(g_aryVox[k][j][i]);
    //            }
    //            //qDebug() << tmpstr;
    //        }
    //    }
    //    //qDebug() << "[DEBUG]-end- func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine(): start copy-g_aryVox " +
    //    QDateTime::currentDateTime().toString("hh:mm:ss");
    //    //-end- for-speed01

    int i, j, k;
    meshAll                         = nx * ny * nz;
    unsigned int          nowMeshNo = 0;
    unsigned int          chkMeshNo = 0;
    int                   drawMeshFlag;
    QHash<int, QVector3D> matnumToColorvecHash =
        func_GL_defineColor_byTableMaterial();    // マテリアルテーブルからハッシュ作成する。　マテリアル番号と色番号
                                                  // QVector3D(R, G, B) を紐づける
    //[DEBUG]foreach(int key, matnumToColorvec.keys()){ qDebug() <<
    //"Vox3DForm::func_vox_GLadd_checkMateNumOfAdjoinMesh() matnumToColorvec key=" << QString::number(key) << "
    // value=matnumToColorvec" << matnumToColorvec.value(key) ;}

    QList<QVector3D> startList, endList;          // 全体のLineList( Bottom, Back, Back, Left Right );
    QList<int>       startMatList, endMatList;    // 全体のLineList( Bottom, Back, Back, Left Right );

    //-----------------------------
    //-start- Back
    QList<QVector3D> lineStartBackList, lineEndBackList;
    QList<int>       lineStartMatBackList, lineEndMatBackList;    // 面として連続するかの判定用
    QList<int>       lineflag1BackList;    // 面として連続するかの判定用 QList<int> startBackList;
    int**            aryXZback = new int*[nz];
    for (int z = 0; z < nz; z++) {
        aryXZback[z] = new int[nx]();    // 初期化　全部ゼロを入れる
    }

    QList<QVector3D> faceStartTopList, faceStartTopMatList;    // 面として連続するかの判定用
    QList<int>       backSameStartList, backSameEndList;
    for (int j = 0; j < ny; j++) {
        //-start- Back
        //-start- 始点終点取得の前処理 ①Y方向1つ前との差異判定 (描く、描かない）。　②Z方向　行単位で同一かの判定
        int        flag_sameOneline = 0;
        int        flag_same        = 0;    // 2行以上、同一か（始点・終点判定のため)
        QList<int> backSameStartList, backSameEndList;
        for (int z = 0; z < nz; z++) {
            flag_sameOneline = 1;    // 1つ下の列と同一か（始点・終点判定のため)
            for (int x = 0; x < nx; x++) {
                // 　Backとの差異判定 (描く、描かない）
                aryXZback[z][x] = g_aryVox[z][j][x];    // デフォルト
                //-start- Back-changed Frontの場合
                // if(j>0){
                //    if(g_aryVox[z][j][x] == g_aryVox[z][j-1][x]){
                //        aryXZfront[z][x] = -1; //y方向1つ前が同じマテリアルの場合、描かないものとして 0 にする
                //    }
                //}
                //-end- Back-changed Frontの場合
                if (j < ny - 1) {
                    if (g_aryVox[z][j][x] == g_aryVox[z][j + 1][x]) {
                        aryXZback[z][x] = -1;    // y方向1つ前が同じマテリアルの場合、描かないものとして 0 にする
                    }
                }
                //                if(j==0){
                //                    qDebug() << "[DEBUG]aryXZback z=" << QString::number(z)  << " j=" <<
                //                    QString::number(j) << " x=" << QString::number(x)  << " aryXZback[z][x]=" <<
                //                    QString::number(aryXZback[z][x]) ;
                //                }

                // 1つ下の列と同一か（始点・終点判定のため)
                if (z > 0) {
                    if (aryXZback[z][x] != aryXZback[z - 1][x]) {
                        flag_sameOneline = 0;
                    }
                    if (x == nx - 1) {
                        // qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine()　j=" <<
                        // QString::number(j) << " z=" << QString::number(z) <<  " flag_same=" <<
                        // QString::number(flag_same) << " flag_sameOneline=" << QString::number(flag_sameOneline)  ;

                        if (flag_same == 0 && flag_sameOneline == 1) {
                            backSameStartList << z - 1;
                            flag_same = 1;
                        }

                        if (flag_same == 1 && flag_sameOneline != 1) {
                            backSameEndList << z - 1;
                            flag_same = 0;
                        }

                        if (flag_same == 1 && z == nz - 1) {
                            backSameEndList << z;
                        }
                    }
                }
            }
            //-end- for(int x=0; x<nx; x++)
        }
        //-end- for(int z=0; z<nz; z++)

        //-start-
        //[DEBUG]デバッグ用表示　※本番時は必ず消す　残していると本番実際データだとループで　時間大・動作重くなるので
        // qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine() backSameStartList=" <<
        // backSameStartList; qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine() backSameEndList="
        // << backSameEndList; if(j == 0){
        //    qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine() XZview j=" << QString::number(j)
        //    ; for(int z=nz-1; z>=0; z--){ //DEBUG見やすくするため　vox上面から
        //        QString tmpstr = "";
        //        for(int x=0; x<nx; x++){
        //            tmpstr += " " + QString::number(aryXZback[z][x]);
        //        }
        //        qDebug() << tmpstr;
        //    }
        //}
        // qDebug() <<""; //改行
        //-end-
        //[DEBUG]デバッグ用表示　※本番時は必ず消す　残していると本番実際データだとループで　時間大・動作重くなるので

        // 1行単位の長方形　始点・終点判定
        for (int z = 0; z < nz; z++) {
            for (int x = 0; x < nx; x++) {
                // 終点高さ  if(backSameStartList.indexOf(j) > 0 ){ row = backSameEndList.indexOf(j); }

                //---始点判定：
                int tmp_flag = 0;
                if (x == 0) {
                    tmp_flag = 1;
                }    // 左端の場合
                if (x > 0) {
                    if (aryXZback[z][x] != aryXZback[z][x - 1]) {
                        tmp_flag = 1;
                    }    // 左の材質と違うとき
                }
                if (aryXZback[z][x] == -1) {
                    tmp_flag = 0;
                }
                if (tmp_flag == 1) {
                    // lineStartBackList << QVector3D(i, j, k);
                    // lineStartMatBackList << g_aryVox[k][j][i];
                    // lineflag1BackList << j; //面として連続するかの判定用
                    startList << QVector3D(x, j, z);
                    int matnum = g_aryVox[z][j][x];
                    startMatList << matnum;
                    g_voxMatToPnumHash.insert(
                        matnum,
                        g_pnum);    // マテリアル番号(matnum) と
                                    // g_voxXYZvecインデックス番号(pnum)の紐づけ。　GUI操作色切替え・表示切替えのため。
                    g_pnum++;
                    g_voxXYZStartVec.append(QVector3D(x, j + 1, z));    // Backは1Mesh後ろに描画する //Back-changed
                    //
                    QVector3D tmpColorVec = QVector3D(1, 0, 0);    // 赤
                    if (matnumToColorvecHash.contains(matnum)) {
                        tmpColorVec = matnumToColorvecHash[matnum];
                    }
                    else {
                        if (startList.count() % 2 == 0) {
                            tmpColorVec = QVector3D(0, 1, 0);
                        }    // 緑
                        if (startList.count() % 3 == 0) {
                            tmpColorVec = QVector3D(0, 0, 1);
                        }    // 青
                        if (startList.count() % 4 == 0) {
                            tmpColorVec = QVector3D(1, 1, 0);
                        }
                        if (startList.count() % 5 == 0) {
                            tmpColorVec = QVector3D(1, 0, 1);
                        }
                    }
                    g_voxColorVec.append(tmpColorVec);
                    g_voxSurfaceStrList.append("Back");
                    g_voxMatnumVec.append(matnum);
                    g_voxAcolorVec.append(g_hash_matToAcolor[matnum]);
                }
                //---終点判定：
                tmp_flag = 0;
                if (x == nx - 1) {
                    tmp_flag = 1;
                }    // 右端の場合
                if (x < nx - 1) {
                    if (aryXZback[z][x] != aryXZback[z][x + 1]) {
                        tmp_flag = 1;
                    }    // 右の材質と違うとき
                }
                if (aryXZback[z][x] == -1) {
                    tmp_flag = 0;
                }
                if (tmp_flag == 1) {
                    // lineEndBackList << QVector3D(i, j, k);
                    // lineEndMatBackList << g_aryVox[k][j][i];
                    // lineflag1BackList << j;
                    int tmpZ = z;
                    if (backSameStartList.indexOf(z) > -1) {
                        tmpZ = backSameEndList.at(backSameStartList.indexOf(z));
                    }
                    endList << QVector3D(x, j, tmpZ);
                    endMatList << g_aryVox[tmpZ][j][x];

                    g_voxXYZEndVec.append(
                        QVector3D(x + 1, j + 1, tmpZ + 1));    // x,z:図形にするときは、幅なので+1 する
                                                               // //j:Backの場合　1Mesh後ろにする。　Back-changed
                }
            }
            //-end- for(int x=0; x<nx; x++)

            // backSameStartList　に含まれている場合は startListの行から EndListの行まで処理スキップする
            int tmpIndex = backSameStartList.indexOf(z);
            if (tmpIndex > -1) {
                z = backSameEndList.at(tmpIndex);
            }
        }
        //-end- for(int z=0; z<nz; z++)
        // if(j==0){
        //    qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() j=" << QString::number(j) << " startList=" <<
        //    startList; qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() j=" << QString::number(j) << "
        //    endList=" << endList;
        //}
    }
    //-end- for(int j=1; j<ny-1; j++)
    // qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() startList.count=" <<
    // QString::number(startList.count());

    //    //------------------------------
    //    //メモリ解放 3D
    //    for(int z=0; z<nz; z++){
    //        for(int y=0; y<ny; y++){
    //            delete[] g_aryVox[z][y] ; //()でデータがないときは0で全部初期化している
    //        }
    //        delete[] g_aryVox[z];
    //    }
    //    delete[] g_aryVox;

    // メモリ解放 2D
    for (int z = 0; z < nz; z++) {
        delete[] aryXZback[z];
    }
    delete[] aryXZback;
    // DEBUG 間違えて編集しないようにコメントアウト
}

void Vox3DForm::func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine_Left()
{
    // 間違えて編集しないようにコメントアウト

    int i, j, k;
    meshAll                = nx * ny * nz;
    unsigned int nowMeshNo = 0;
    unsigned int chkMeshNo = 0;
    int          drawMeshFlag;

    QHash<int, QVector3D> matnumToColorvecHash =
        func_GL_defineColor_byTableMaterial();    // マテリアルテーブルからハッシュ作成する。　マテリアル番号と色番号
                                                  // QVector3D(R, G, B) を紐づける
    //[DEBUG]foreach(int key, matnumToColorvec.keys()){ qDebug() <<
    //"Vox3DForm::func_vox_GLadd_checkMateNumOfAdjoinMesh() matnumToColorvec key=" << QString::number(key) << "
    // value=matnumToColorvec" << matnumToColorvec.value(key) ;}

    QList<QVector3D> startList, endList;          // 全体のLineList( Bottom, Left, Back, Left Right );
    QList<int>       startMatList, endMatList;    // 全体のLineList( Bottom, Left, Back, Left Right );

    //-----------------------------
    //-start- Left
    QList<QVector3D> lineStartLeftList, lineEndLeftList;
    QList<int>       lineStartMatLeftList, lineEndMatLeftList;    // 面として連続するかの判定用
    QList<int>       lineflag1LeftList;    // 面として連続するかの判定用 QList<int> startLeftList;
    int**            aryYZleft = new int*[nz];
    for (int z = 0; z < nz; z++) {
        aryYZleft[z] = new int[ny]();    // 初期化　全部ゼロを入れる
    }

    QList<QVector3D> faceStartTopList, faceStartTopMatList;    // 面として連続するかの判定用
    QList<int>       leftSameStartList, leftSameEndList;
    // for(int j=0; j<ny; j++){
    for (int i = 0; i < nx; i++) {
        //-start- Left
        //-start- 始点終点取得の前処理 ①Y方向1つ前との差異判定 (描く、描かない）。　②Z方向　行単位で同一かの判定
        int        flag_sameOneline = 0;
        int        flag_same        = 0;    // 2行以上、同一か（始点・終点判定のため)
        QList<int> leftSameStartList, leftSameEndList;
        for (int z = 0; z < nz; z++) {
            flag_sameOneline = 1;    // 1つ下の列と同一か（始点・終点判定のため)
            // for(int x=0; x<nx; x++){
            for (int y = 0; y < ny; y++) {
                // 　Leftとの差異判定 (描く、描かない）
                aryYZleft[z][y] = g_aryVox[z][y][i];    // デフォルト
                if (i > 0) {
                    if (g_aryVox[z][y][i] == g_aryVox[z][y][i - 1]) {
                        aryYZleft[z][y] = -1;    // y方向1つ前が同じマテリアルの場合、描かないものとして 0 にする
                    }
                }
                //                if(j==0){
                //                    qDebug() << "[DEBUG]aryYZleft z=" << QString::number(z)  << " y=" <<
                //                    QString::number(y)  << " aryYZleft[z][y]=" << " i=" << QString::number(i) <<
                //                    QString::number(aryYZleft[z][y]) ;
                //                }

                // 1つ下の列と同一か（始点・終点判定のため)
                if (z > 0) {
                    if (aryYZleft[z][y] != aryYZleft[z - 1][y]) {
                        flag_sameOneline = 0;
                    }
                    if (y == ny - 1) {
                        // qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine()　i=" <<
                        // QString::number(i) << " z=" << QString::number(z) <<  " flag_same=" <<
                        // QString::number(flag_same) << " flag_sameOneline=" << QString::number(flag_sameOneline)  ;

                        if (flag_same == 0 && flag_sameOneline == 1) {
                            leftSameStartList << z - 1;
                            flag_same = 1;
                        }

                        if (flag_same == 1 && flag_sameOneline != 1) {
                            leftSameEndList << z - 1;
                            flag_same = 0;
                        }

                        if (flag_same == 1 && z == nz - 1) {
                            leftSameEndList << z;
                        }
                    }
                }
            }
            //-end- for(int x=0; x<nx; x++)
        }
        //-end- for(int z=0; z<nz; z++)

        //-start-
        //[DEBUG]デバッグ用表示　※本番時は必ず消す　残していると本番実際データだとループで　時間大・動作重くなるので
        // qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine() leftSameStartList=" <<
        // leftSameStartList; qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine() leftSameEndList="
        // << leftSameEndList; if(j == 0){
        //    qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine() XZview j=" << QString::number(j)
        //    ; for(int z=nz-1; z>=0; z--){ //DEBUG見やすくするため　vox上面から
        //        QString tmpstr = "";
        //        for(int y=ny-1; y<=0; y--){ //DEBUG見やすくするため　vox前面から
        //            tmpstr += " " + QString::number(aryYZleft[z][y]);
        //        }
        //        qDebug() << tmpstr;
        //    }
        //}
        // qDebug() <<""; //改行
        //-end-
        //[DEBUG]デバッグ用表示　※本番時は必ず消す　残していると本番実際データだとループで　時間大・動作重くなるので

        // 1行単位の長方形　始点・終点判定
        for (int z = 0; z < nz; z++) {
            // for(int x=0; x<nx; x++){
            for (int y = 0; y < ny; y++) {
                // 終点高さ  if(leftSameStartList.indexOf(j) > 0 ){ row = leftSameEndList.indexOf(j); }

                //---始点判定：
                int tmp_flag = 0;
                if (y == 0) {
                    tmp_flag = 1;
                }    // 前端の場合
                if (y > 0) {
                    if (aryYZleft[z][y] != aryYZleft[z][y - 1]) {
                        tmp_flag = 1;
                    }    // 左の材質と違うとき
                }
                if (aryYZleft[z][y] == -1) {
                    tmp_flag = 0;
                }
                if (tmp_flag == 1) {
                    // lineStartLeftList << QVector3D(i, j, k);
                    // lineStartMatLeftList << g_aryVox[k][j][i];
                    // lineflag1LeftList << j; //面として連続するかの判定用
                    startList << QVector3D(i, y, z);
                    int matnum = g_aryVox[z][y][i];
                    startMatList << matnum;
                    g_voxMatToPnumHash.insert(
                        matnum,
                        g_pnum);    // マテリアル番号(matnum) と
                                    // g_voxXYZvecインデックス番号(pnum)の紐づけ。　GUI操作色切替え・表示切替えのため。
                    g_pnum++;
                    g_voxXYZStartVec.append(QVector3D(i, y, z));
                    //
                    QVector3D tmpColorVec = QVector3D(1, 0, 0);    // 赤
                    if (matnumToColorvecHash.contains(matnum)) {
                        tmpColorVec = matnumToColorvecHash[matnum];
                    }
                    else {
                        if (startList.count() % 2 == 0) {
                            tmpColorVec = QVector3D(0, 1, 0);
                        }    // 緑
                        if (startList.count() % 3 == 0) {
                            tmpColorVec = QVector3D(0, 0, 1);
                        }    // 青
                        if (startList.count() % 4 == 0) {
                            tmpColorVec = QVector3D(1, 1, 0);
                        }
                        if (startList.count() % 5 == 0) {
                            tmpColorVec = QVector3D(1, 0, 1);
                        }
                    }
                    g_voxColorVec.append(tmpColorVec);
                    g_voxSurfaceStrList.append("Left");
                    g_voxMatnumVec.append(matnum);
                    g_voxAcolorVec.append(g_hash_matToAcolor[matnum]);
                }
                //---終点判定：
                tmp_flag = 0;
                if (y == ny - 1) {
                    tmp_flag = 1;
                }    // 右端の場合
                if (y < ny - 1) {
                    if (aryYZleft[z][y] != aryYZleft[z][y + 1]) {
                        tmp_flag = 1;
                    }    // 右の材質と違うとき
                }
                if (aryYZleft[z][y] == -1) {
                    tmp_flag = 0;
                }
                if (tmp_flag == 1) {
                    // lineEndLeftList << QVector3D(i, j, k);
                    // lineEndMatLeftList << g_aryVox[k][j][i];
                    // lineflag1LeftList << j;
                    int tmpZ = z;
                    if (leftSameStartList.indexOf(z) > -1) {
                        tmpZ = leftSameEndList.at(leftSameStartList.indexOf(z));
                    }
                    endList << QVector3D(i, y, tmpZ);
                    endMatList << g_aryVox[tmpZ][y][i];

                    g_voxXYZEndVec.append(QVector3D(i, y + 1, tmpZ + 1));    // 図形にするときは、幅なので+1 する
                }
            }
            //-end- for(int x=0; x<nx; x++)

            // leftSameStartList　に含まれている場合は startListの行から EndListの行まで処理スキップする
            int tmpIndex = leftSameStartList.indexOf(z);
            if (tmpIndex > -1) {
                z = leftSameEndList.at(tmpIndex);
            }
        }
        //-end- for(int z=0; z<nz; z++)
        // if(j==0){
        //    qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() i=" << QString::number(i) << " startList=" <<
        //    startList; qDebug() << "\n[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() i=" << QString::number(i) << "
        //    endList=" << endList;
        //
        //    qDebug() << "\n[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() i=" << QString::number(i) << "
        //    g_voxXYZStartVec=" << g_voxXYZStartVec; qDebug() << "\n[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() i="
        //    << QString::number(i) << " g_voxXYZEndVec=" << g_voxXYZEndVec;
        //}
    }
    //-end- for(int i=1; i<nx-1; i++)
    // qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() startList.count=" <<
    // QString::number(startList.count());

    //    //------------------------------
    //    //メモリ解放 3D
    //    for(int z=0; z<nz; z++){
    //        for(int y=0; y<ny; y++){
    //            delete[] g_aryVox[z][y] ; //()でデータがないときは0で全部初期化している
    //        }
    //        delete[] g_aryVox[z];
    //    }
    //    delete[] g_aryVox;

    // メモリ解放 2D
    for (int z = 0; z < nz; z++) {
        delete[] aryYZleft[z];
    }
    delete[] aryYZleft;

    // 間違えて編集しないようにコメントアウト
}

void Vox3DForm::func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine_Right()
{
    // 間違えて編集しないようにコメントアウト
    int i, j, k;
    meshAll                = nx * ny * nz;
    unsigned int nowMeshNo = 0;
    unsigned int chkMeshNo = 0;
    int          drawMeshFlag;

    QHash<int, QVector3D> matnumToColorvecHash =
        func_GL_defineColor_byTableMaterial();    // マテリアルテーブルからハッシュ作成する。　マテリアル番号と色番号
                                                  // QVector3D(R, G, B) を紐づける
    //[DEBUG]foreach(int key, matnumToColorvec.keys()){ qDebug() <<
    //"Vox3DForm::func_vox_GLadd_checkMateNumOfAdjoinMesh() matnumToColorvec key=" << QString::number(key) << "
    // value=matnumToColorvec" << matnumToColorvec.value(key) ;}

    QList<QVector3D> startList, endList;          // 全体のLineList( Bottom, Left, Back, Left Right );
    QList<int>       startMatList, endMatList;    // 全体のLineList( Bottom, Left, Back, Left Right );

    //-----------------------------
    //-start- Left
    QList<QVector3D> lineStartLeftList, lineEndLeftList;
    QList<int>       lineStartMatLeftList, lineEndMatLeftList;    // 面として連続するかの判定用
    QList<int>       lineflag1LeftList;    // 面として連続するかの判定用 QList<int> startLeftList;
    int**            aryYZleft = new int*[nz];
    for (int z = 0; z < nz; z++) {
        aryYZleft[z] = new int[ny]();    // 初期化　全部ゼロを入れる
    }

    QList<QVector3D> faceStartTopList, faceStartTopMatList;    // 面として連続するかの判定用
    QList<int>       leftSameStartList, leftSameEndList;
    // for(int j=0; j<ny; j++){
    for (int i = 0; i < nx; i++) {
        //-start- Left
        //-start- 始点終点取得の前処理 ①Y方向1つ前との差異判定 (描く、描かない）。　②Z方向　行単位で同一かの判定
        int        flag_sameOneline = 0;
        int        flag_same        = 0;    // 2行以上、同一か（始点・終点判定のため)
        QList<int> leftSameStartList, leftSameEndList;
        for (int z = 0; z < nz; z++) {
            flag_sameOneline = 1;    // 1つ下の列と同一か（始点・終点判定のため)
            // for(int x=0; x<nx; x++){
            for (int y = 0; y < ny; y++) {
                // 　Leftとの差異判定 (描く、描かない）
                aryYZleft[z][y] = g_aryVox[z][y][i];    // デフォルト
                //-start- Right-changed Frontの場合
                // if(i>0){
                //    if(g_aryVox[z][y][i] == g_aryVox[z][y][i-1]){
                //        aryYZleft[z][y] = -1; //y方向1つ前が同じマテリアルの場合、描かないものとして 0 にする
                //    }
                //}
                if (i < nx - 1) {
                    if (g_aryVox[z][y][i] == g_aryVox[z][y][i + 1]) {
                        aryYZleft[z][y] = -1;    // y方向1つ前が同じマテリアルの場合、描かないものとして 0 にする
                    }
                }
                //-end- Right-changed Frontの場合
                //                if(j==0){
                //                    qDebug() << "[DEBUG]aryYZleft z=" << QString::number(z)  << " y=" <<
                //                    QString::number(y)  << " aryYZleft[z][y]=" << " i=" << QString::number(i) <<
                //                    QString::number(aryYZleft[z][y]) ;
                //                }

                // 1つ下の列と同一か（始点・終点判定のため)
                if (z > 0) {
                    if (aryYZleft[z][y] != aryYZleft[z - 1][y]) {
                        flag_sameOneline = 0;
                    }
                    if (y == ny - 1) {
                        // qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine()　i=" <<
                        // QString::number(i) << " z=" << QString::number(z) <<  " flag_same=" <<
                        // QString::number(flag_same) << " flag_sameOneline=" << QString::number(flag_sameOneline)  ;

                        if (flag_same == 0 && flag_sameOneline == 1) {
                            leftSameStartList << z - 1;
                            flag_same = 1;
                        }

                        if (flag_same == 1 && flag_sameOneline != 1) {
                            leftSameEndList << z - 1;
                            flag_same = 0;
                        }

                        if (flag_same == 1 && z == nz - 1) {
                            leftSameEndList << z;
                        }
                    }
                }
            }
            //-end- for(int x=0; x<nx; x++)
        }
        //-end- for(int z=0; z<nz; z++)

        //-start-
        //[DEBUG]デバッグ用表示　※本番時は必ず消す　残していると本番実際データだとループで　時間大・動作重くなるので
        // qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine() leftSameStartList=" <<
        // leftSameStartList; qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine() leftSameEndList="
        // << leftSameEndList; if(j == 0){
        //    qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine() XZview j=" << QString::number(j)
        //    ; for(int z=nz-1; z>=0; z--){ //DEBUG見やすくするため　vox上面から
        //        QString tmpstr = "";
        //        for(int y=ny-1; y<=0; y--){ //DEBUG見やすくするため　vox前面から
        //            tmpstr += " " + QString::number(aryYZleft[z][y]);
        //        }
        //        qDebug() << tmpstr;
        //    }
        //}
        // qDebug() <<""; //改行
        //-end-
        //[DEBUG]デバッグ用表示　※本番時は必ず消す　残していると本番実際データだとループで　時間大・動作重くなるので

        // 1行単位の長方形　始点・終点判定
        for (int z = 0; z < nz; z++) {
            // for(int x=0; x<nx; x++){
            for (int y = 0; y < ny; y++) {
                // 終点高さ  if(leftSameStartList.indexOf(j) > 0 ){ row = leftSameEndList.indexOf(j); }

                //---始点判定：
                int tmp_flag = 0;
                if (y == 0) {
                    tmp_flag = 1;
                }    // 前端の場合
                if (y > 0) {
                    if (aryYZleft[z][y] != aryYZleft[z][y - 1]) {
                        tmp_flag = 1;
                    }    // 左の材質と違うとき
                }
                if (aryYZleft[z][y] == -1) {
                    tmp_flag = 0;
                }
                if (tmp_flag == 1) {
                    // lineStartLeftList << QVector3D(i, j, k);
                    // lineStartMatLeftList << g_aryVox[k][j][i];
                    // lineflag1LeftList << j; //面として連続するかの判定用
                    startList << QVector3D(i, y, z);
                    int matnum = g_aryVox[z][y][i];
                    startMatList << matnum;
                    g_voxMatToPnumHash.insert(
                        matnum,
                        g_pnum);    // マテリアル番号(matnum) と
                                    // g_voxXYZvecインデックス番号(pnum)の紐づけ。　GUI操作色切替え・表示切替えのため。
                    g_pnum++;
                    //-start- Right-changed ★Leftの場合から変更
                    // g_voxXYZStartVec.append(QVector3D(i, y, z));
                    g_voxXYZStartVec.append(QVector3D(i + 1, y, z));    // Rightは1Mesh右に描画する //Right-changed
                    //-end- Right-changed ★Leftの場合から変更
                    //
                    QVector3D tmpColorVec = QVector3D(1, 0, 0);    // 赤
                    if (matnumToColorvecHash.contains(matnum)) {
                        tmpColorVec = matnumToColorvecHash[matnum];
                    }
                    else {
                        if (startList.count() % 2 == 0) {
                            tmpColorVec = QVector3D(0, 1, 0);
                        }    // 緑
                        if (startList.count() % 3 == 0) {
                            tmpColorVec = QVector3D(0, 0, 1);
                        }    // 青
                        if (startList.count() % 4 == 0) {
                            tmpColorVec = QVector3D(1, 1, 0);
                        }
                        if (startList.count() % 5 == 0) {
                            tmpColorVec = QVector3D(1, 0, 1);
                        }
                    }
                    g_voxColorVec.append(tmpColorVec);
                    g_voxSurfaceStrList.append("Right");    // ★Leftの場合から変更
                    g_voxMatnumVec.append(matnum);
                    g_voxAcolorVec.append(g_hash_matToAcolor[matnum]);
                }
                //---終点判定：
                tmp_flag = 0;
                if (y == ny - 1) {
                    tmp_flag = 1;
                }    // 右端の場合
                if (y < ny - 1) {
                    if (aryYZleft[z][y] != aryYZleft[z][y + 1]) {
                        tmp_flag = 1;
                    }    // 右の材質と違うとき
                }
                if (aryYZleft[z][y] == -1) {
                    tmp_flag = 0;
                }
                if (tmp_flag == 1) {
                    // lineEndLeftList << QVector3D(i, j, k);
                    // lineEndMatLeftList << g_aryVox[k][j][i];
                    // lineflag1LeftList << j;
                    int tmpZ = z;
                    if (leftSameStartList.indexOf(z) > -1) {
                        tmpZ = leftSameEndList.at(leftSameStartList.indexOf(z));
                    }
                    endList << QVector3D(i, y, tmpZ);
                    endMatList << g_aryVox[tmpZ][y][i];

                    //-start- Right-changed ★Leftの場合から変更
                    // g_voxXYZEndVec.append(QVector3D(i, y+1, tmpZ+1)); //図形にするときは、幅なので+1 する
                    g_voxXYZEndVec.append(QVector3D(
                        i + 1, y + 1,
                        tmpZ + 1));    // 図形にするときは、幅なので+1 する  //Rightは1Mesh右に描画する //Right-changed
                    //-end- Right-changed ★Leftの場合から変更
                }
            }
            //-end- for(int x=0; x<nx; x++)

            // leftSameStartList　に含まれている場合は startListの行から EndListの行まで処理スキップする
            int tmpIndex = leftSameStartList.indexOf(z);
            if (tmpIndex > -1) {
                z = leftSameEndList.at(tmpIndex);
            }
        }
        //-end- for(int z=0; z<nz; z++)
        //-start- [DEBUG] DEBUG表示用　本番時は必ず消す　（重くなるので)
        // if(i==0){
        // qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine_Right() i=" << QString::number(i) << " startList="
        // << startList; qDebug() << "\n[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLineRight() i=" << QString::number(i) <<
        // " endList=" << endList;
        //
        // qDebug() << "\n[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine_Right() i=" << QString::number(i) << "
        // g_voxXYZStartVec=" << g_voxXYZStartVec; qDebug() << "\n[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine_Right()
        // i=" << QString::number(i) << " g_voxXYZEndVec=" << g_voxXYZEndVec; qDebug() << "";
        //}
        //-end- [DEBUG] DEBUG表示用　本番時は必ず消す　（重くなるので)
    }
    //-end- for(int i=1; i<nx-1; i++)

    //-start- [DEBUG] DEBUG表示用　本番時は必ず消す　（重くなるので)
    // qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() startList.count=" <<
    // QString::number(startList.count()); qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() startList.count="
    // << QString::number(startList.count()); qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() startList=" <<
    // startList << "\n"; qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() endList=" << endList << "\n";
    //-end- [DEBUG] DEBUG表示用　本番時は必ず消す　（重くなるので)

    //    //------------------------------
    //    //メモリ解放 3D
    //    for(int z=0; z<nz; z++){
    //        for(int y=0; y<ny; y++){
    //            delete[] g_aryVox[z][y] ; //()でデータがないときは0で全部初期化している
    //        }
    //        delete[] g_aryVox[z];
    //    }
    //    delete[] g_aryVox;

    // メモリ解放 2D
    for (int z = 0; z < nz; z++) {
        delete[] aryYZleft[z];
    }
    delete[] aryYZleft;

    // 間違えて編集しないようにコメントアウト
}

void Vox3DForm::func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine_Bottom()
{
    int i, j, k;
    meshAll                         = nx * ny * nz;
    unsigned int          nowMeshNo = 0;
    unsigned int          chkMeshNo = 0;
    int                   drawMeshFlag;
    QHash<int, QVector3D> matnumToColorvecHash =
        func_GL_defineColor_byTableMaterial();    // マテリアルテーブルからハッシュ作成する。　マテリアル番号と色番号
                                                  // QVector3D(R, G, B) を紐づける
    //[DEBUG]foreach(int key, matnumToColorvec.keys()){ qDebug() <<
    //"Vox3DForm::func_vox_GLadd_checkMateNumOfAdjoinMesh() matnumToColorvec key=" << QString::number(key) << "
    // value=matnumToColorvec" << matnumToColorvec.value(key) ;}

    QList<QVector3D> startList, endList;          // 全体のLineList( Bottom, Bottom, Back, Left Right );
    QList<int>       startMatList, endMatList;    // 全体のLineList( Bottom, Bottom, Back, Left Right );

    //-----------------------------
    //-start- Bottom
    QList<QVector3D> lineStartBottomList, lineEndBottomList;
    QList<int>       lineStartMatBottomList, lineEndMatBottomList;    // 面として連続するかの判定用
    QList<int>       lineflag1BottomList;    // 面として連続するかの判定用 QList<int> startBottomList;
    // int **aryXYbottom = new int*[nz];
    // for(int z=0; z< nz; z++){
    //     aryXYbottom[y] = new int[nx](); //初期化　全部ゼロを入れる
    // }
    int** aryXYbottom = new int*[ny];
    for (int y = 0; y < ny; y++) {
        aryXYbottom[y] = new int[nx]();    // 初期化　全部ゼロを入れる
    }

    QList<QVector3D> faceStartTopList, faceStartTopMatList;    // 面として連続するかの判定用
    QList<int>       bottomSameStartList, bottomSameEndList;
    // for(int j=0; j<ny; j++){
    for (int k = 0; k < nz; k++) {
        //-start- Bottom
        //-start- 始点終点取得の前処理 ①Y方向1つ前との差異判定 (描く、描かない）。　②Z方向　行単位で同一かの判定
        int        flag_sameOneline = 0;
        int        flag_same        = 0;    // 2行以上、同一か（始点・終点判定のため)
        QList<int> bottomSameStartList, bottomSameEndList;
        // for(int z=0; z<nz; z++){
        for (int y = 0; y < ny; y++) {
            flag_sameOneline = 1;    // 1つ下の列と同一か（始点・終点判定のため)
            for (int x = 0; x < nx; x++) {
                // 　Bottomとの差異判定 (描く、描かない）
                aryXYbottom[y][x] = g_aryVox[k][y][x];    // デフォルト
                if (k > 0) {
                    if (g_aryVox[k][y][x] == g_aryVox[k - 1][y][x]) {
                        aryXYbottom[y][x] = -1;    // y方向1つ前が同じマテリアルの場合、描かないものとして 0 にする
                    }
                }
                //                if(j==0){
                //                    qDebug() << "[DEBUG]aryXYbottom z=" << QString::number(z)  << " j=" <<
                //                    QString::number(j) << " x=" << QString::number(x)  << " aryXYbottom[y][x]=" <<
                //                    QString::number(aryXYbottom[y][x]) ;
                //                }

                // Y=1つ前の列と同一か（始点・終点判定のため)
                if (y > 0) {
                    if (aryXYbottom[y][x] != aryXYbottom[y - 1][x]) {
                        flag_sameOneline = 0;
                    }
                    if (x == nx - 1) {
                        // qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine()　j=" <<
                        // QString::number(j) << " z=" << QString::number(z) <<  " flag_same=" <<
                        // QString::number(flag_same) << " flag_sameOneline=" << QString::number(flag_sameOneline)  ;

                        if (flag_same == 0 && flag_sameOneline == 1) {
                            bottomSameStartList << y - 1;
                            flag_same = 1;
                        }

                        if (flag_same == 1 && flag_sameOneline != 1) {
                            bottomSameEndList << y - 1;
                            flag_same = 0;
                        }

                        if (flag_same == 1 && y == ny - 1) {
                            bottomSameEndList << y;
                        }
                    }
                }
            }
            //-end- for(int x=0; x<nx; x++)
        }
        //-end- for(int z=0; z<nz; z++)

        //-start-
        //[DEBUG]デバッグ用表示　※本番時は必ず消す　残していると本番実際データだとループで　時間大・動作重くなるので
        // qDebug() << "\n[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine() bottomSameStartList=" <<
        // bottomSameStartList; qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine()
        // bottomSameEndList=" << bottomSameEndList; if(k == 0){
        //    qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine() XYview z=k=" <<
        //    QString::number(k) ; for(int y=ny-1; y>=0; y--){ //DEBUG見やすくするため　vox上面から
        //        QString tmpstr = "";
        //        for(int x=0; x<nx; x++){
        //            tmpstr += " " + QString::number(aryXYbottom[y][x]);
        //        }
        //        qDebug() << tmpstr;
        //    }
        //}
        // qDebug() <<""; //改行
        //-end-
        //[DEBUG]デバッグ用表示　※本番時は必ず消す　残していると本番実際データだとループで　時間大・動作重くなるので

        // 1行単位の長方形　始点・終点判定
        // for(int z=0; z<nz; z++){
        for (int y = 0; y < ny; y++) {
            for (int x = 0; x < nx; x++) {
                // 終点高さ  if(bottomSameStartList.indexOf(j) > 0 ){ row = bottomSameEndList.indexOf(j); }

                //---始点判定：
                int tmp_flag = 0;
                if (x == 0) {
                    tmp_flag = 1;
                }    // 左端の場合
                if (x > 0) {
                    if (aryXYbottom[y][x] != aryXYbottom[y][x - 1]) {
                        tmp_flag = 1;
                    }    // 左の材質と違うとき
                }
                if (aryXYbottom[y][x] == -1) {
                    tmp_flag = 0;
                }
                if (tmp_flag == 1) {
                    // lineStartBottomList << QVector3D(i, j, k);
                    // lineStartMatBottomList << g_aryVox[k][j][i];
                    // lineflag1BottomList << j; //面として連続するかの判定用
                    startList << QVector3D(x, y, k);
                    int matnum = g_aryVox[k][y][x];
                    startMatList << matnum;
                    g_voxMatToPnumHash.insert(
                        matnum,
                        g_pnum);    // マテリアル番号(matnum) と
                                    // g_voxXYZvecインデックス番号(pnum)の紐づけ。　GUI操作色切替え・表示切替えのため。
                    g_pnum++;
                    g_voxXYZStartVec.append(QVector3D(x, y, k));
                    //
                    QVector3D tmpColorVec = QVector3D(1, 0, 0);    // 赤
                    if (matnumToColorvecHash.contains(matnum)) {
                        tmpColorVec = matnumToColorvecHash[matnum];
                    }
                    else {
                        if (startList.count() % 2 == 0) {
                            tmpColorVec = QVector3D(0, 1, 0);
                        }    // 緑
                        if (startList.count() % 3 == 0) {
                            tmpColorVec = QVector3D(0, 0, 1);
                        }    // 青
                        if (startList.count() % 4 == 0) {
                            tmpColorVec = QVector3D(1, 1, 0);
                        }
                        if (startList.count() % 5 == 0) {
                            tmpColorVec = QVector3D(1, 0, 1);
                        }
                    }
                    g_voxColorVec.append(tmpColorVec);
                    g_voxSurfaceStrList.append("Bottom");
                    g_voxMatnumVec.append(matnum);
                    g_voxAcolorVec.append(g_hash_matToAcolor[matnum]);
                }
                //---終点判定：
                tmp_flag = 0;
                if (x == nx - 1) {
                    tmp_flag = 1;
                }    // 右端の場合
                if (x < nx - 1) {
                    if (aryXYbottom[y][x] != aryXYbottom[y][x + 1]) {
                        tmp_flag = 1;
                    }    // 右の材質と違うとき
                }
                if (aryXYbottom[y][x] == -1) {
                    tmp_flag = 0;
                }
                if (tmp_flag == 1) {
                    // lineEndBottomList << QVector3D(i, j, k);
                    // lineEndMatBottomList << g_aryVox[k][j][i];
                    // lineflag1BottomList << j;
                    int tmpY = y;
                    if (bottomSameStartList.indexOf(y) > -1) {
                        tmpY = bottomSameEndList.at(bottomSameStartList.indexOf(y));
                    }
                    endList << QVector3D(x, tmpY, k);
                    endMatList << g_aryVox[k][tmpY][x];

                    g_voxXYZEndVec.append(QVector3D(x + 1, tmpY + 1, k));    // 図形にするときは、幅なので+1 する
                }
            }
            //-end- for(int x=0; x<nx; x++)

            // bottomSameStartList　に含まれている場合は startListの行から EndListの行まで処理スキップする
            int tmpIndex = bottomSameStartList.indexOf(y);
            if (tmpIndex > -1) {
                y = bottomSameEndList.at(tmpIndex);
            }
        }
        //-end- for(int z=0; z<nz; z++)
        // if(k==0){
        //    qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() k=" << QString::number(k) << " startList=" <<
        //    startList; qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() k=" << QString::number(k) << "
        //    endList=" << endList;
        //}
    }
    //-end- for(int j=1; j<ny-1; j++)
    // qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() startList.count=" <<
    // QString::number(startList.count());

    // qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() g_voxXYZStartVec=" << g_voxXYZStartVec;
    // qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() g_voxXYZEndVec=" << g_voxXYZEndVec;
    // qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() g_voxXYZColorVec=" << g_voxColorVec;
    // qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() g_voxSurfaceStrList=" << g_voxSurfaceStrList;
    // qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() g_voxMatnumVec=" << g_voxMatnumVec;

    //    //------------------------------
    //    //メモリ解放 3D
    //    for(int z=0; z<nz; z++){
    //        for(int y=0; y<ny; y++){
    //            delete[] g_aryVox[k][y] ; //()でデータがないときは0で全部初期化している
    //        }
    //        delete[] g_aryVox[k];
    //    }
    //    delete[] g_aryVox;

    // メモリ解放 2D
    for (int y = 0; y < ny; y++) {
        delete[] aryXYbottom[y];
    }
    delete[] aryXYbottom;
    // DEBUG 間違えて編集しないようにコメントアウト
}

void Vox3DForm::func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine_Top()
{
    int i, j, k;
    meshAll                         = nx * ny * nz;
    unsigned int          nowMeshNo = 0;
    unsigned int          chkMeshNo = 0;
    int                   drawMeshFlag;
    QHash<int, QVector3D> matnumToColorvecHash =
        func_GL_defineColor_byTableMaterial();    // マテリアルテーブルからハッシュ作成する。　マテリアル番号と色番号
                                                  // QVector3D(R, G, B) を紐づける
    //[DEBUG]foreach(int key, matnumToColorvec.keys()){ qDebug() <<
    //"Vox3DForm::func_vox_GLadd_checkMateNumOfAdjoinMesh() matnumToColorvec key=" << QString::number(key) << "
    // value=matnumToColorvec" << matnumToColorvec.value(key) ;}

    QList<QVector3D> startList, endList;          // 全体のLineList( Bottom, Bottom, Back, Left Right );
    QList<int>       startMatList, endMatList;    // 全体のLineList( Bottom, Bottom, Back, Left Right );

    //-----------------------------
    //-start- Bottom
    QList<QVector3D> lineStartBottomList, lineEndBottomList;
    QList<int>       lineStartMatBottomList, lineEndMatBottomList;    // 面として連続するかの判定用
    QList<int>       lineflag1BottomList;    // 面として連続するかの判定用 QList<int> startBottomList;
    // int **aryXYbottom = new int*[nz];
    // for(int z=0; z< nz; z++){
    //     aryXYbottom[y] = new int[nx](); //初期化　全部ゼロを入れる
    // }
    int** aryXYbottom = new int*[ny];
    for (int y = 0; y < ny; y++) {
        aryXYbottom[y] = new int[nx]();    // 初期化　全部ゼロを入れる
    }

    QList<QVector3D> faceStartTopList, faceStartTopMatList;    // 面として連続するかの判定用
    QList<int>       bottomSameStartList, bottomSameEndList;
    // for(int j=0; j<ny; j++){
    for (int k = 0; k < nz; k++) {
        //-start- Bottom
        //-start- 始点終点取得の前処理 ①Y方向1つ前との差異判定 (描く、描かない）。　②Z方向　行単位で同一かの判定
        int        flag_sameOneline = 0;
        int        flag_same        = 0;    // 2行以上、同一か（始点・終点判定のため)
        QList<int> bottomSameStartList, bottomSameEndList;
        // for(int z=0; z<nz; z++){
        for (int y = 0; y < ny; y++) {
            flag_sameOneline = 1;    // 1つ下の列と同一か（始点・終点判定のため)
            for (int x = 0; x < nx; x++) {
                // 　Bottomとの差異判定 (描く、描かない）
                aryXYbottom[y][x] = g_aryVox[k][y][x];    // デフォルト
                //-start- Top-changed Bottomからの変更点
                // if(k>0){
                //    if(g_aryVox[k][y][x] == g_aryVox[k-1][y][x]){
                //        aryXYbottom[y][x] = -1; //y方向1つ前が同じマテリアルの場合、描かないものとして 0 にする
                //    }
                //}
                if (k < nz - 1) {
                    if (g_aryVox[k][y][x] == g_aryVox[k + 1][y][x]) {
                        aryXYbottom[y][x] = -1;    // z方向1つ上が同じマテリアルの場合、描かないものとして 0 にする
                    }
                }
                //-end- Top-changed Bottomからの変更点
                //                if(j==0){
                //                    qDebug() << "[DEBUG]aryXYbottom z=" << QString::number(z)  << " j=" <<
                //                    QString::number(j) << " x=" << QString::number(x)  << " aryXYbottom[y][x]=" <<
                //                    QString::number(aryXYbottom[y][x]) ;
                //                }

                // Y=1つ前の列と同一か（始点・終点判定のため)
                if (y > 0) {
                    if (aryXYbottom[y][x] != aryXYbottom[y - 1][x]) {
                        flag_sameOneline = 0;
                    }
                    if (x == nx - 1) {
                        // qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine()　j=" <<
                        // QString::number(j) << " z=" << QString::number(z) <<  " flag_same=" <<
                        // QString::number(flag_same) << " flag_sameOneline=" << QString::number(flag_sameOneline)  ;

                        if (flag_same == 0 && flag_sameOneline == 1) {
                            bottomSameStartList << y - 1;
                            flag_same = 1;
                        }

                        if (flag_same == 1 && flag_sameOneline != 1) {
                            bottomSameEndList << y - 1;
                            flag_same = 0;
                        }

                        if (flag_same == 1 && y == ny - 1) {
                            bottomSameEndList << y;
                        }
                    }
                }
            }
            //-end- for(int x=0; x<nx; x++)
        }
        //-end- for(int z=0; z<nz; z++)

        //-start-
        //[DEBUG]デバッグ用表示　※本番時は必ず消す　残していると本番実際データだとループで　時間大・動作重くなるので
        // qDebug() << "\n[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine() bottomSameStartList=" <<
        // bottomSameStartList; qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine()
        // bottomSameEndList=" << bottomSameEndList; if(k == 0){
        //    qDebug() << "[DEBUG]func_vox_GLadd_checkMateNumOfAdjoinMesh_getVoxLine() XYview z=k=" <<
        //    QString::number(k) ; for(int y=ny-1; y>=0; y--){ //DEBUG見やすくするため　vox上面から
        //        QString tmpstr = "";
        //        for(int x=0; x<nx; x++){
        //            tmpstr += " " + QString::number(aryXYbottom[y][x]);
        //        }
        //        qDebug() << tmpstr;
        //    }
        //}
        // qDebug() <<""; //改行
        //-end-
        //[DEBUG]デバッグ用表示　※本番時は必ず消す　残していると本番実際データだとループで　時間大・動作重くなるので

        // 1行単位の長方形　始点・終点判定
        // for(int z=0; z<nz; z++){
        for (int y = 0; y < ny; y++) {
            for (int x = 0; x < nx; x++) {
                // 終点高さ  if(bottomSameStartList.indexOf(j) > 0 ){ row = bottomSameEndList.indexOf(j); }

                //---始点判定：
                int tmp_flag = 0;
                if (x == 0) {
                    tmp_flag = 1;
                }    // 左端の場合
                if (x > 0) {
                    if (aryXYbottom[y][x] != aryXYbottom[y][x - 1]) {
                        tmp_flag = 1;
                    }    // 左の材質と違うとき
                }
                if (aryXYbottom[y][x] == -1) {
                    tmp_flag = 0;
                }
                if (tmp_flag == 1) {
                    // lineStartBottomList << QVector3D(i, j, k);
                    // lineStartMatBottomList << g_aryVox[k][j][i];
                    // lineflag1BottomList << j; //面として連続するかの判定用
                    startList << QVector3D(x, y, k);
                    int matnum = g_aryVox[k][y][x];
                    startMatList << matnum;
                    g_voxMatToPnumHash.insert(
                        matnum,
                        g_pnum);    // マテリアル番号(matnum) と
                                    // g_voxXYZvecインデックス番号(pnum)の紐づけ。　GUI操作色切替え・表示切替えのため。
                    g_pnum++;
                    //-start- Top-changed Bottomからの変更点
                    // g_voxXYZStartVec.append(QVector3D(x, y, k));
                    g_voxXYZStartVec.append(
                        QVector3D(x, y, k + 1));    // k+1 Topは1Meshz方向:上に描画する //Top-changed Bottomからの変更点
                    //-end- Top-changed Bottomからの変更点
                    //
                    QVector3D tmpColorVec = QVector3D(1, 0, 0);    // 赤
                    if (matnumToColorvecHash.contains(matnum)) {
                        tmpColorVec = matnumToColorvecHash[matnum];
                    }
                    else {
                        if (startList.count() % 2 == 0) {
                            tmpColorVec = QVector3D(0, 1, 0);
                        }    // 緑
                        if (startList.count() % 3 == 0) {
                            tmpColorVec = QVector3D(0, 0, 1);
                        }    // 青
                        if (startList.count() % 4 == 0) {
                            tmpColorVec = QVector3D(1, 1, 0);
                        }
                        if (startList.count() % 5 == 0) {
                            tmpColorVec = QVector3D(1, 0, 1);
                        }
                    }
                    g_voxColorVec.append(tmpColorVec);
                    //-start- Top-changed Bottomからの変更点
                    // g_voxSurfaceStrList.append("Bottom");
                    g_voxSurfaceStrList.append("Top");
                    //-end- Top-changed Bottomからの変更点
                    g_voxMatnumVec.append(matnum);
                    g_voxAcolorVec.append(g_hash_matToAcolor[matnum]);
                }
                //---終点判定：
                tmp_flag = 0;
                if (x == nx - 1) {
                    tmp_flag = 1;
                }    // 右端の場合
                if (x < nx - 1) {
                    if (aryXYbottom[y][x] != aryXYbottom[y][x + 1]) {
                        tmp_flag = 1;
                    }    // 右の材質と違うとき
                }
                if (aryXYbottom[y][x] == -1) {
                    tmp_flag = 0;
                }
                if (tmp_flag == 1) {
                    // lineEndBottomList << QVector3D(i, j, k);
                    // lineEndMatBottomList << g_aryVox[k][j][i];
                    // lineflag1BottomList << j;
                    int tmpY = y;
                    if (bottomSameStartList.indexOf(y) > -1) {
                        tmpY = bottomSameEndList.at(bottomSameStartList.indexOf(y));
                    }
                    endList << QVector3D(x, tmpY, k);
                    endMatList << g_aryVox[k][tmpY][x];
                    //-start- Top-changed Bottomからの変更点
                    // g_voxXYZEndVec.append(QVector3D(x+1, tmpY+1, k)); //図形にするときは、幅なので+1 する
                    g_voxXYZEndVec.append(QVector3D(
                        x + 1, tmpY + 1, k + 1));    // 図形にするときは、幅なので+1 する  //k+1
                                                     // Topは1Meshz方向:上に描画する //Top-changed Bottomからの変更点
                    //-end- Top-changed Bottomからの変更点
                }
            }
            //-end- for(int x=0; x<nx; x++)

            // bottomSameStartList　に含まれている場合は startListの行から EndListの行まで処理スキップする
            int tmpIndex = bottomSameStartList.indexOf(y);
            if (tmpIndex > -1) {
                y = bottomSameEndList.at(tmpIndex);
            }
        }
        //-end- for(int z=0; z<nz; z++)
        // if(k==0){
        //    qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() k=" << QString::number(k) << " startList=" <<
        //    startList; qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() k=" << QString::number(k) << "
        //    endList=" << endList;
        //}
    }
    //-end- for(int j=1; j<ny-1; j++)
    // qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() startList.count=" <<
    // QString::number(startList.count());

    // qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() g_voxXYZStartVec=" << g_voxXYZStartVec;
    // qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() g_voxXYZEndVec=" << g_voxXYZEndVec;
    // qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() g_voxXYZColorVec=" << g_voxColorVec;
    // qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() g_voxSurfaceStrList=" << g_voxSurfaceStrList;
    // qDebug() << "[DEBUG]Vox3DSub02.cpp-AdojoinMesh_getVoxLine() g_voxMatnumVec=" << g_voxMatnumVec;

    //    //------------------------------
    //    //メモリ解放 3D
    //    for(int z=0; z<nz; z++){
    //        for(int y=0; y<ny; y++){
    //            delete[] g_aryVox[k][y] ; //()でデータがないときは0で全部初期化している
    //        }
    //        delete[] g_aryVox[k];
    //    }
    //    delete[] g_aryVox;

    // メモリ解放 2D
    for (int y = 0; y < ny; y++) {
        delete[] aryXYbottom[y];
    }
    delete[] aryXYbottom;
    // DEBUG 間違えて編集しないようにコメントアウト
}

QStringList Vox3DForm::readTextFileAll(QString fileName)
{
    QStringList lines;
    QFile       file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QString errStr = "file open error:" + file.errorString();
        qDebug() << errStr;
        // return(lines);
    }
    QTextStream in(&file);
    QString     inStr = in.readAll();
    file.close();

    // voxファイルのLF改行に対応
    // QStringList lines = inStr.split("\r\n");
    lines = inStr.split("\r\n");    // kuroda add
    lines = inStr.split("\n");
    return lines;
}

// ファイル書き込み(windowsで使うファイル用)
int Vox3DForm::fileWrteForWindows(QString WriteFilePath, QString WriteMode, QStringList msgList)
{
    QFile file(WriteFilePath);
    if (WriteMode == "Append") {
        if (!file.open(QIODevice::Append | QIODevice::Text)) {
            // QMessageBox::information(this, tr("Unable to openfile"), file.errorString());
            // //Qtフォームクラスでないため、QMessageBoxが使えない。。代わりにエラーの場合は -1 を返り値に渡す
            return (-1);
        }
    }
    else {
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            // QMessageBox::information(this, tr("Unable to openfile"), file.errorString());
            // //Qtフォームクラスでないため、QMessageBoxが使えない。。代わりにエラーの場合は -1 を返り値に渡す
            return (-1);
        }
    }

    QTextStream out(&file);
    for (int n = 0; n < msgList.size(); n++) {
        out << msgList.at(n) << Qt::endl;
    }
    file.close();

    return 0;
}

QVector3D Vox3DForm::func_GL_defineColor(int input_colornum)
{
    QVector3D resultColor  = QVector3D(1, 0, 0);
    int       tmpint_color = input_colornum % 10;

    if (tmpint_color == 0) {
        resultColor = QVector3D(1, 0, 0);
    }    // 赤
    if (tmpint_color == 1) {
        resultColor = QVector3D(0, 1, 0);
    }    // 緑
    if (tmpint_color == 2) {
        resultColor = QVector3D(0, 0, 1);
    }    // 青

    if (tmpint_color == 3) {
        resultColor = QVector3D(1, 1, 0);
    }    // 黄色
    if (tmpint_color == 4) {
        resultColor = QVector3D(1, 0, 1);
    }    // マゼンタ
    if (tmpint_color == 5) {
        resultColor = QVector3D(0, 1, 1);
    }    // シアン(水色)

    if (tmpint_color == 6) {
        resultColor = QVector3D(0.75, 0.8, 0);
    }    // オレンジ
    if (tmpint_color == 7) {
        resultColor = QVector3D(0.67, 1, 0.18);
    }    // 黄緑
    if (tmpint_color == 8) {
        resultColor = QVector3D(0.5, 0, 0.5);
    }    // 紫

    if (tmpint_color == 8) {
        resultColor = QVector3D(1, 0.75, 0.8);
    }    // ピンク

    if (tmpint_color == 6) {
        resultColor = QVector3D(0.5, 0, 0);
    }    // 濃い赤
    if (tmpint_color == 7) {
        resultColor = QVector3D(0, 0.5, 0);
    }    // 濃い緑
    if (tmpint_color == 8) {
        resultColor = QVector3D(0, 0, 0.5);
    }    // 濃い青

    //-------------------------
    // 参考
    // if(tmpint_color == 0){ resultColor = QVector3D(0, 0, 0); } //Black QColor(0, 0, 0, 255)
    // if(tmpint_color == 1){ resultColor = QVector3D(0.5, 0.5, 0.5); } //Glay QColor(128, 128, 128, 255)
    // if(tmpint_color == 2){ resultColor = QVector3D(0.82, 0.82, 0.82); } //LightGlay QColor(211, 211, 211, 255)
    // if(tmpint_color == 3){ resultColor = QVector3D(1, 1, 1); } //White QColor(255, 255, 255, 255)

    // if(tmpint_color == 4){ resultColor = QVector3D(0.64, 0.16, 0.16); } //Brown QColor(165, 42, 42, 255)
    // if(tmpint_color == 5){ resultColor = QVector3D(0.5, 0, 0.5); } //Purple QColor(128, 0, 128, 255)
    // if(tmpint_color == 6){ resultColor = QVector3D(0.57, 0.43, 0.85); } //Medium Purple QColor(147, 112, 219, 255)
    // if(tmpint_color == 7){ resultColor = QVector3D(1, 0, 0); } //Red QColor(255, 0, 0, 255)
    // if(tmpint_color == 8){ resultColor = QVector3D(1, 0, 1); } //Magenta QColor(255, 0, 255, 255)
    // if(tmpint_color == 9){ resultColor = QVector3D(1, 0.75, 0.8); } //pink QColor(255, 192, 203, 255)
    // if(tmpint_color == 10){ resultColor = QVector3D(0.75, 0.8, 0); } //Orange QColor(225, 165, 0, 255)
    // if(tmpint_color == 11){ resultColor = QVector3D(1, 0.84, 0); } //Gold QColor(255, 215, 0, 255)
    // if(tmpint_color == 12){ resultColor = QVector3D(1, 1, 0); } //Yellow QColor(255, 255, 0, 255)

    // if(tmpint_color == 13){ resultColor = QVector3D(0, 0.5, 0); } //Green QColor(0, 128, 0, 255)
    // if(tmpint_color == 14){ resultColor = QVector3D(0.67, 1, 0.18); } //Greenyellow QColor(173, 255, 47, 255)
    // if(tmpint_color == 15){ resultColor = QVector3D(0.5, 0.5, 0); } //0live QColor(128, 128, 0, 255)
    // if(tmpint_color == 16){ resultColor = QVector3D(0, 0, 0.5); } //Navy QColor(0, 0, 128, 255)
    // if(tmpint_color == 17){ resultColor = QVector3D(0, 0, 1); } //Blue QColorr(0, 0, 255, 255)
    // if(tmpint_color == 18){ resultColor = QVector3D(0, 1, 1); } //cyan QColor(0, 255, 255, 255)
    // if(tmpint_color == 19){ resultColor = QVector3D(0.87, 1, 1); } //Lightcyan QColor(224, 255, 255, 255)
    //-------------------------

    return resultColor;
}

QVector3D Vox3DForm::func_GL_defineColor_nameToRGBvec(QString input_colorname)
{
    QVector3D resultColor = QVector3D(1, 0, 0);    // デフォルト

    if (input_colorname == "Black") {
        resultColor = QVector3D(0, 0, 0);
    }    // Black QColor(0, 0, 0, 255)
    if (input_colorname == "Gray") {
        resultColor = QVector3D(0.5, 0.5, 0.5);
    }    // Glay QColor(128, 128, 128, 255)
    if (input_colorname == "DarkGray") {
        resultColor = QVector3D(0.18, 0.18, 0.18);
    }    // DarkGlay QColor(46, 46, 46, 255)
    if (input_colorname == "Lightgray") {
        resultColor = QVector3D(0.82, 0.82, 0.82);
    }    // LightGlay QColor(211, 211, 211, 255)
    if (input_colorname == "White") {
        resultColor = QVector3D(1, 1, 1);
    }    // White QColor(255, 255, 255, 255)

    if (input_colorname == "Brown") {
        resultColor = QVector3D(0.64, 0.16, 0.16);
    }    // Brown QColor(165, 42, 42, 255)
    if (input_colorname == "Purple") {
        resultColor = QVector3D(0.5, 0, 0.5);
    }    // Purple QColor(128, 0, 128, 255)
    if (input_colorname == "MediumPurple") {
        resultColor = QVector3D(0.57, 0.43, 0.85);
    }    // Medium Purple QColor(147, 112, 219, 255)
    if (input_colorname == "Red") {
        resultColor = QVector3D(1, 0, 0);
    }    // Red QColor(255, 0, 0, 255)
    if (input_colorname == "Magenta") {
        resultColor = QVector3D(1, 0, 1);
    }    // Magenta QColor(255, 0, 255, 255)
    if (input_colorname == "Pink") {
        resultColor = QVector3D(1, 0.75, 0.8);
    }    // pink QColor(255, 192, 203, 255)
    if (input_colorname == "Orange") {
        resultColor = QVector3D(0.88, 0.65, 0);
    }    // Orange QColor(225, 165, 0, 255)
    if (input_colorname == "Gold") {
        resultColor = QVector3D(1, 0.84, 0);
    }    // Gold QColor(255, 215, 0, 255)
    if (input_colorname == "Yellow") {
        resultColor = QVector3D(1, 1, 0);
    }    // Yellow QColor(255, 255, 0, 255)

    if (input_colorname == "Green") {
        resultColor = QVector3D(0, 0.5, 0);
    }    // Green QColor(0, 128, 0, 255)
    if (input_colorname == "Greenyellow") {
        resultColor = QVector3D(0.67, 1, 0.18);
    }    // Greenyellow QColor(173, 255, 47, 255)
    if (input_colorname == "Olive") {
        resultColor = QVector3D(0.5, 0.5, 0);
    }    // 0live QColor(128, 128, 0, 255)
    if (input_colorname == "Navy") {
        resultColor = QVector3D(0, 0, 0.5);
    }    // Navy QColor(0, 0, 128, 255)
    if (input_colorname == "Blue") {
        resultColor = QVector3D(0, 0, 1);
    }    // Blue QColorr(0, 0, 255, 255)
    if (input_colorname == "Cyan") {
        resultColor = QVector3D(0, 1, 1);
    }    // Cyan QColor(0, 255, 255, 255)
    if (input_colorname == "Lightcyan") {
        resultColor = QVector3D(0.87, 1, 1);
    }    // Lightcyan QColor(224, 255, 255, 255)

    return resultColor;
}

QHash<int, QVector3D> Vox3DForm::func_GL_defineColor_byTableMaterial()
{    // マテリアルテーブルからハッシュ作成する。　マテリアル番号と色番号 QVector3D(R, G, B) を紐づける
    // matnumを引数として、テーブルから同行のマテリアル色名をとる。
    QHash<int, QVector3D> ret_matnumToColorvec;
    for (int row = 0; row < ui->tableWidget_material->rowCount(); row++) {
        int matnum = ui->tableWidget_material->item(row, 1)->text().toInt();

        CustomColorCombo* tmpcombo =
            new CustomColorCombo(nullptr, ui->tableWidget_material->item(row, 1)->text().toInt(), ui->obj3dViewer);
        tmpcombo          = static_cast<CustomColorCombo*>(ui->tableWidget_material->cellWidget(row, 3));
        QString colorText = tmpcombo->currentText();

        QVector3D colorVec = func_GL_defineColor_nameToRGBvec(colorText);

        ret_matnumToColorvec.insert(matnum, colorVec);
    }

    return ret_matnumToColorvec;
}
