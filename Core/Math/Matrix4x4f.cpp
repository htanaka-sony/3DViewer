#include "Matrix4x4f.h"
#include "CoreMath.h"

CORE_NAMESPACE_BEGIN

const Matrix4x4f Matrix4x4f::m_identity_matrix;

Point3f Matrix4x4f::preMult(const Point3f& v) const
{
    float d = (m_matrix[0][3] * v.x() + m_matrix[1][3] * v.y() + m_matrix[2][3] * v.z() + m_matrix[3][3]);
    if (d == 0.0) {
        d = 1.0f;
    }
    return Point3f((m_matrix[0][0] * v.x() + m_matrix[1][0] * v.y() + m_matrix[2][0] * v.z() + m_matrix[3][0]) / d,
                   (m_matrix[0][1] * v.x() + m_matrix[1][1] * v.y() + m_matrix[2][1] * v.z() + m_matrix[3][1]) / d,
                   (m_matrix[0][2] * v.x() + m_matrix[1][2] * v.y() + m_matrix[2][2] * v.z() + m_matrix[3][2]) / d);
}

Point3f Matrix4x4f::postMult(const Point3f& v) const
{
    float d = (m_matrix[3][0] * v.x() + m_matrix[3][1] * v.y() + m_matrix[3][2] * v.z() + m_matrix[3][3]);
    if (d == 0.0) {
        d = 1.0f;
    }
    return Point3f((m_matrix[0][0] * v.x() + m_matrix[0][1] * v.y() + m_matrix[0][2] * v.z() + m_matrix[0][3]) / d,
                   (m_matrix[1][0] * v.x() + m_matrix[1][1] * v.y() + m_matrix[1][2] * v.z() + m_matrix[1][3]) / d,
                   (m_matrix[2][0] * v.x() + m_matrix[2][1] * v.y() + m_matrix[2][2] * v.z() + m_matrix[2][3]) / d);
}

Point4f Matrix4x4f::preMult(const Point4f& v) const
{
    return Point4f((m_matrix[0][0] * v.x() + m_matrix[1][0] * v.y() + m_matrix[2][0] * v.z() + m_matrix[3][0] * v.w()),
                   (m_matrix[0][1] * v.x() + m_matrix[1][1] * v.y() + m_matrix[2][1] * v.z() + m_matrix[3][1] * v.w()),
                   (m_matrix[0][2] * v.x() + m_matrix[1][2] * v.y() + m_matrix[2][2] * v.z() + m_matrix[3][2] * v.w()),
                   (m_matrix[0][3] * v.x() + m_matrix[1][3] * v.y() + m_matrix[2][3] * v.z() + m_matrix[3][3] * v.w()));
}

Point4f Matrix4x4f::postMult(const Point4f& v) const
{
    return Point4f((m_matrix[0][0] * v.x() + m_matrix[0][1] * v.y() + m_matrix[0][2] * v.z() + m_matrix[0][3] * v.w()),
                   (m_matrix[1][0] * v.x() + m_matrix[1][1] * v.y() + m_matrix[1][2] * v.z() + m_matrix[1][3] * v.w()),
                   (m_matrix[2][0] * v.x() + m_matrix[2][1] * v.y() + m_matrix[2][2] * v.z() + m_matrix[2][3] * v.w()),
                   (m_matrix[3][0] * v.x() + m_matrix[3][1] * v.y() + m_matrix[3][2] * v.z() + m_matrix[3][3] * v.w()));
}

Point3f Matrix4x4f::operator*(const Point3f& v) const
{
    return preMult(v);
}

Point4f Matrix4x4f::operator*(const Point4f& v) const
{
    return preMult(v);
}

Matrix4x4f Matrix4x4f::preMult(const Matrix4x4f& other) const
{
    Matrix4x4f result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.m_matrix[i][j] = 0.0f;
            for (int k = 0; k < 4; ++k) {
                result.m_matrix[i][j] += m_matrix[k][j] * other.m_matrix[i][k];
            }
        }
    }
    return result;
}

Matrix4x4f Matrix4x4f::postMult(const Matrix4x4f& other) const
{
    Matrix4x4f result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.m_matrix[i][j] = 0.0f;
            for (int k = 0; k < 4; ++k) {
                result.m_matrix[i][j] += other.m_matrix[k][j] * m_matrix[i][k];
            }
        }
    }
    return result;
}

BoundingBox3f Matrix4x4f::preMult(const BoundingBox3f& box) const
{
    if (!box.valid()) {
        return BoundingBox3f();
    }
    BoundingBox3f new_box;
    for (int ic = 0; ic < 8; ++ic) {
        new_box.expandBy(preMult(box.corner(ic)));
    }
    return new_box;
}

BoundingBox3f Matrix4x4f::operator*(const BoundingBox3f& box) const
{
    return preMult(box);
}

Planef Matrix4x4f::preMult(const Planef& plane) const
{
    /// 逆転置行列を使用して法線ベクトルを変換
    ///  ※拡大縮小がある場合はこれが必要
    float m[3][3];
    normalMatrix(m);
    Point3f normal = plane.normal();
    Point3f transformed_normal(m[0][0] * normal.x() + m[1][0] * normal.y() + m[2][0] * normal.z(),
                               m[0][1] * normal.x() + m[1][1] * normal.y() + m[2][1] * normal.z(),
                               m[0][2] * normal.x() + m[1][2] * normal.y() + m[2][2] * normal.z());
    // Point3f transformed_normal = preMult(plane.normal());
    Point3f point_on_plane    = -plane.normal() * plane.d();
    Point3f transformed_point = preMult(point_on_plane);

    return Planef(transformed_normal, transformed_point);
}

Planef Matrix4x4f::operator*(const Planef& plane) const
{
    return preMult(plane);
}

Planef Matrix4x4f::toPlane() const
{
    Point3f normal(m_matrix[2][0], m_matrix[2][1], m_matrix[2][2]);

    Point3f point_on_plane(m_matrix[3][0], m_matrix[3][1], m_matrix[3][2]);

    return Planef(normal, point_on_plane);
}

Matrix4x4f::Matrix4x4f(const Planef& plane)
{
    const Point4f& plane_eq = plane.plane();

    Point3f origin;

    if (fabs(plane_eq[0]) > fabs(plane_eq[1])) {
        if (fabs(plane_eq[0]) > fabs(plane_eq[2])) {
            /// 0
            origin = Point3f(-plane_eq[3] / plane_eq[0], 0, 0);
        }
        else if (fabs(plane_eq[2]) > 0.0) {
            /// 2
            origin = Point3f(0, 0, -plane_eq[3] / plane_eq[2]);
        }
        else {
            origin = Point3f(0, 0, 0);
        }
    }
    else if (fabs(plane_eq[1]) > fabs(plane_eq[2])) {
        /// 1
        origin = Point3f(0, -plane_eq[3] / plane_eq[1], 0);
    }
    else if (fabs(plane_eq[2]) > 0.0) {
        /// 2
        origin = Point3f(0, 0, -plane_eq[3] / plane_eq[2]);
    }
    else {
        origin = Point3f(0, 0, 0);
    }

    Point3f normal(plane_eq[0], plane_eq[1], plane_eq[2]);
    Point3f u = (std::abs(normal.x()) > std::abs(normal.y())) ? Point3f(-normal.z(), 0, normal.x()).normalized()
                                                              : Point3f(0, -normal.z(), normal.y()).normalized();
    Point3f v = (normal ^ u).normalized();

    set(u.x(), v.x(), normal.x(), origin.x(),    ///
        u.y(), v.y(), normal.y(), origin.y(),    ///
        u.z(), v.z(), normal.z(), origin.z(),    ///
        0, 0, 0, 1);                             ///
}

bool Matrix4x4f::isEqual(const Matrix4x4f& other, float absTol) const
{
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            float a = m_matrix[i][j];
            float b = other.m_matrix[i][j];
            if (std::fabs(a - b) > absTol) {
                return false;
            }
        }
    }

    return true;
}

void Matrix4x4f::ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
    if (left == right || bottom == top || nearPlane == farPlane) return;

    const float width     = right - left;
    const float invheight = top - bottom;
    const float clip      = farPlane - nearPlane;

    m_matrix[0][0] = 2.0f / width;
    m_matrix[1][0] = 0.0f;
    m_matrix[2][0] = 0.0f;
    m_matrix[3][0] = -(left + right) / width;
    m_matrix[0][1] = 0.0f;
    m_matrix[1][1] = 2.0f / invheight;
    m_matrix[2][1] = 0.0f;
    m_matrix[3][1] = -(top + bottom) / invheight;
    m_matrix[0][2] = 0.0f;
    m_matrix[1][2] = 0.0f;
    m_matrix[2][2] = -2.0f / clip;
    m_matrix[3][2] = -(nearPlane + farPlane) / clip;
    m_matrix[0][3] = 0.0f;
    m_matrix[1][3] = 0.0f;
    m_matrix[2][3] = 0.0f;
    m_matrix[3][3] = 1.0f;
}

void Matrix4x4f::perspective(float fovY, float aspect, float nearPlane, float farPlane)
{
    if (nearPlane == farPlane) return;

    const float tanHalfFovy = tan(fovY / 2.0f);
    const float clip        = farPlane - nearPlane;

    m_matrix[0][0] = 1.0f / (aspect * tanHalfFovy);
    m_matrix[1][0] = 0.0f;
    m_matrix[2][0] = 0.0f;
    m_matrix[3][0] = 0.0f;

    m_matrix[0][1] = 0.0f;
    m_matrix[1][1] = 1.0f / tanHalfFovy;
    m_matrix[2][1] = 0.0f;
    m_matrix[3][1] = 0.0f;

    m_matrix[0][2] = 0.0f;
    m_matrix[1][2] = 0.0f;
    m_matrix[2][2] = -(farPlane + nearPlane) / clip;
    m_matrix[3][2] = -(2.0f * farPlane * nearPlane) / clip;

    m_matrix[0][3] = 0.0f;
    m_matrix[1][3] = 0.0f;
    m_matrix[2][3] = -1.0f;
    m_matrix[3][3] = 0.0f;
}

void Matrix4x4f::lookAt(const Point3f& eye, const Point3f& center, const Point3f& up)
{
    Point3f forward = center - eye;
    if (!forward.normalize()) {
        return;
    }

    Point3f side = forward ^ up;
    if (!side.normalize()) {
        return;
    }

    Point3f upVector = side ^ forward;

    m_matrix[0][0] = side.x();
    m_matrix[1][0] = side.y();
    m_matrix[2][0] = side.z();
    m_matrix[3][0] = 0.0f;
    m_matrix[0][1] = upVector.x();
    m_matrix[1][1] = upVector.y();
    m_matrix[2][1] = upVector.z();
    m_matrix[3][1] = 0.0f;
    m_matrix[0][2] = -forward.x();
    m_matrix[1][2] = -forward.y();
    m_matrix[2][2] = -forward.z();
    m_matrix[3][2] = 0.0f;
    m_matrix[0][3] = 0.0f;
    m_matrix[1][3] = 0.0f;
    m_matrix[2][3] = 0.0f;
    m_matrix[3][3] = 1.0f;

    translate(-eye);
}

void Matrix4x4f::normalMatrix(float (*matrix)[3]) const
{
    Matrix4x4f inv = inverted();
    /// 上位3x3部分を転置
    for (int ic = 0; ic < 3; ++ic) {
        for (int jc = 0; jc < 3; ++jc) {
            matrix[ic][jc] = inv.m_matrix[jc][ic];
        }
    }
}

Matrix4x4f Matrix4x4f::rotateMatrix() const
{
    Matrix4x4f m;
    for (int ic = 0; ic < 3; ++ic) {
        /// 回転成分
        for (int jc = 0; jc < 3; ++jc) {
            m.m_matrix[ic][jc] = m_matrix[ic][jc];
        }

        /// 正規化
        float scale = sqrt(m.m_matrix[ic][0] * m.m_matrix[ic][0] + m.m_matrix[ic][1] * m.m_matrix[ic][1]
                           + m.m_matrix[ic][2] * m.m_matrix[ic][2]);
        if (scale > 0.0f) {
            for (int jc = 0; jc < 3; ++jc) {
                m.m_matrix[ic][jc] /= scale;
            }
        }
    }

    return m;
}

void Matrix4x4f::translate(const Point3f& vec)
{
    translate(vec.x(), vec.y(), vec.z());
}

void Matrix4x4f::translate(float x, float y, float z)
{
    m_matrix[3][0] += m_matrix[0][0] * x + m_matrix[1][0] * y + m_matrix[2][0] * z;
    m_matrix[3][1] += m_matrix[0][1] * x + m_matrix[1][1] * y + m_matrix[2][1] * z;
    m_matrix[3][2] += m_matrix[0][2] * x + m_matrix[1][2] * y + m_matrix[2][2] * z;
    m_matrix[3][3] += m_matrix[0][3] * x + m_matrix[1][3] * y + m_matrix[2][3] * z;
}

void Matrix4x4f::rotate(float angle, const Point3f& axis)
{
    rotate(angle, axis.x(), axis.y(), axis.z());
}

void Matrix4x4f::rotate(float angle, float x, float y, float z)
{
    if (angle == 0.0f) {
        return;
    }

    float cos = std::cos(angle);
    float sin = std::sin(angle);

    double len = std::sqrt(double(x) * double(x) + double(y) * double(y) + double(z) * double(z));
    if (len > 0.0) {
        x = float(double(x) / len);
        y = float(double(y) / len);
        z = float(double(z) / len);
    }
    float      icos = 1.0f - cos;
    Matrix4x4f rot;
    rot.m_matrix[0][0] = x * x * icos + cos;
    rot.m_matrix[1][0] = x * y * icos - z * sin;
    rot.m_matrix[2][0] = x * z * icos + y * sin;
    rot.m_matrix[3][0] = 0.0f;
    rot.m_matrix[0][1] = y * x * icos + z * sin;
    rot.m_matrix[1][1] = y * y * icos + cos;
    rot.m_matrix[2][1] = y * z * icos - x * sin;
    rot.m_matrix[3][1] = 0.0f;
    rot.m_matrix[0][2] = x * z * icos - y * sin;
    rot.m_matrix[1][2] = y * z * icos + x * sin;
    rot.m_matrix[2][2] = z * z * icos + cos;
    rot.m_matrix[3][2] = 0.0f;
    rot.m_matrix[0][3] = 0.0f;
    rot.m_matrix[1][3] = 0.0f;
    rot.m_matrix[2][3] = 0.0f;
    rot.m_matrix[3][3] = 1.0f;

    *this *= rot;
}

void Matrix4x4f::rotateDegree(float degree, const Point3f& axis)
{
    rotate(degreeToRadian(degree), axis);
}

void Matrix4x4f::rotateDegree(float degree, float x, float y, float z)
{
    rotate(degreeToRadian(degree), x, y, z);
}

void Matrix4x4f::scale(float x, float y, float z)
{
    m_matrix[0][0] *= x;
    m_matrix[0][1] *= x;
    m_matrix[0][2] *= x;
    m_matrix[0][3] *= x;
    m_matrix[1][0] *= y;
    m_matrix[1][1] *= y;
    m_matrix[1][2] *= y;
    m_matrix[1][3] *= y;
    m_matrix[2][0] *= z;
    m_matrix[2][1] *= z;
    m_matrix[2][2] *= z;
    m_matrix[2][3] *= z;
}

void Matrix4x4f::scale(float factor)
{
    m_matrix[0][0] *= factor;
    m_matrix[0][1] *= factor;
    m_matrix[0][2] *= factor;
    m_matrix[0][3] *= factor;
    m_matrix[1][0] *= factor;
    m_matrix[1][1] *= factor;
    m_matrix[1][2] *= factor;
    m_matrix[1][3] *= factor;
    m_matrix[2][0] *= factor;
    m_matrix[2][1] *= factor;
    m_matrix[2][2] *= factor;
    m_matrix[2][3] *= factor;
}

Matrix4x4f Matrix4x4f::inverted(bool* inverted) const
{
    Matrix4x4f temp(*this);
    auto       ret = temp.setToInverse();
    if (inverted) *inverted = ret;
    return temp;
}

bool Matrix4x4f::setToInverse()
{
    int indxc[4], indxr[4], ipiv[4];
    int icol = 0;
    int irow = 0;

    /// 初期化
    for (int i = 0; i < 4; ++i) {
        ipiv[i] = 0;
    }

    double matrixd[4][4];
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            matrixd[i][j] = m_matrix[i][j];
        }
    }

    auto& matrix = matrixd;

    /// ガウス・ジョルダン法の適用
    for (int i = 0; i < 4; ++i) {
        /// ピボット選択
        float big = 0.0f;
        for (int j = 0; j < 4; ++j) {
            if (ipiv[j] != 1) {
                for (int k = 0; k < 4; ++k) {
                    if (ipiv[k] == 0) {
                        if (std::abs(matrix[k][j]) > big) {
                            big  = std::abs(matrix[k][j]);
                            irow = j;
                            icol = k;
                        }
                    }
                    else if (ipiv[k] > 1) {
                        return false;    /// 特異行列
                    }
                }
            }
        }

        ++ipiv[icol];

        /// 行の交換
        if (irow != icol) {
            for (int j = 0; j < 4; ++j) {
                std::swap(matrix[j][irow], matrix[j][icol]);
            }
        }

        indxr[i] = irow;
        indxc[i] = icol;

        /// ピボット要素が0の場合、特異行列
        if (matrix[icol][icol] == 0) {
            return false;
        }

        /// ピボット行を正規化
        double pivinv      = 1.0 / matrix[icol][icol];
        matrix[icol][icol] = 1.0;
        for (int j = 0; j < 4; ++j) {
            matrix[j][icol] *= pivinv;
        }

        /// 他の行を操作
        for (int j = 0; j < 4; ++j) {
            if (j != icol) {
                double dum      = matrix[icol][j];
                matrix[icol][j] = 0;
                for (int k = 0; k < 4; ++k) {
                    matrix[k][j] -= matrix[k][icol] * dum;
                }
            }
        }
    }

    /// 列の順序を元に戻す
    for (int i = 3; i >= 0; --i) {
        if (indxr[i] != indxc[i]) {
            for (int j = 0; j < 4; ++j) {
                std::swap(matrix[indxr[i]][j], matrix[indxc[i]][j]);
            }
        }
    }

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            m_matrix[i][j] = matrixd[i][j];
        }
    }

    return true;
}

CORE_NAMESPACE_END
