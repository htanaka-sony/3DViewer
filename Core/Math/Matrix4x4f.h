#ifndef CORE_MATRIX4X4F_H
#define CORE_MATRIX4X4F_H

#include "CoreGlobal.h"

#include <float.h>
#include <cstring>
#include <vector>

#include "BoundingBox3f.h"
#include "Planef.h"
#include "Point3f.h"
#include "Point4f.h"

#ifdef USE_QT
    #include <QMatrix4x4>
    #include <QVector3D>
    #include <QVector4D>
#endif

CORE_NAMESPACE_BEGIN

class CORE_EXPORT Matrix4x4f {
#ifdef USE_QT
public:
    inline Matrix4x4f(const QMatrix4x4& other) { set(other.data()); }
    inline operator QMatrix4x4() const
    {
        QMatrix4x4 temp;
        memcpy(temp.data(), m_matrix, sizeof(m_matrix));
        return temp;
    }
    QMatrix4x4 operator*(const QMatrix4x4& v) const { return QMatrix4x4(preMult(Matrix4x4f(v))); }
    QVector3D  operator*(const QVector3D& v) const { return QVector3D(preMult(Point3f(v))); }
    QVector4D  operator*(const QVector4D& v) const { return QVector4D(preMult(Point4f(v))); }
#endif
public:
    inline Matrix4x4f()
    {
        memset(m_matrix, 0, sizeof(m_matrix));
        m_matrix[0][0] = m_matrix[1][1] = m_matrix[2][2] = m_matrix[3][3] = 1.0f;
    }

    inline Matrix4x4f(const Matrix4x4f& other) { memcpy(m_matrix, other.m_matrix, sizeof(m_matrix)); }
    inline Matrix4x4f(const float* a) { memcpy(m_matrix, a, sizeof(m_matrix)); }
    inline Matrix4x4f(float a00, float a01, float a02, float a03, float a10, float a11, float a12, float a13, float a20,
                      float a21, float a22, float a23, float a30, float a31, float a32, float a33)
    {
        set(a00, a01, a02, a03, a10, a11, a12, a13, a20, a21, a22, a23, a30, a31, a32, a33);
    }
    inline ~Matrix4x4f() {}

    Matrix4x4f(const Planef& plane);

    void setRow(int row, float a0, float a1, float a2, float a3)
    {
        m_matrix[row][0] = a0;
        m_matrix[row][1] = a1;
        m_matrix[row][2] = a2;
        m_matrix[row][3] = a3;
    }

    inline float*       ptr() { return (float*)m_matrix; }
    inline const float* ptr() const { return (const float*)m_matrix; }

    inline int  compare(const Matrix4x4f& other) const { return memcmp(m_matrix, other.m_matrix, sizeof(m_matrix)); }
    inline bool operator<(const Matrix4x4f& other) const { return compare(other) < 0; }
    inline bool operator==(const Matrix4x4f& other) const { return compare(other) == 0; }
    inline bool operator!=(const Matrix4x4f& other) const { return compare(other) != 0; }

    inline float&       operator()(int col, int row) { return m_matrix[col][row]; }
    inline const float& operator()(int col, int row) const { return m_matrix[col][row]; }
    inline float*       operator[](int col) { return m_matrix[col]; }
    inline const float* operator[](int col) const { return m_matrix[col]; }

    bool isEqual(const Matrix4x4f& other, float absTol = 1e-6f) const;

    inline bool isValid() const { return !isNan(); }
    inline bool isNan() const
    {
        return isnan(m_matrix[0][0]) || isnan(m_matrix[0][1]) || isnan(m_matrix[0][2]) || isnan(m_matrix[0][3])
            || isnan(m_matrix[1][0]) || isnan(m_matrix[1][1]) || isnan(m_matrix[1][2]) || isnan(m_matrix[1][3])
            || isnan(m_matrix[2][0]) || isnan(m_matrix[2][1]) || isnan(m_matrix[2][2]) || isnan(m_matrix[2][3])
            || isnan(m_matrix[3][0]) || isnan(m_matrix[3][1]) || isnan(m_matrix[3][2]) || isnan(m_matrix[3][3]);
    }

    inline Matrix4x4f& operator=(const Matrix4x4f& other)
    {
        if (&other == this) return *this;
        memcpy(m_matrix, other.m_matrix, sizeof(m_matrix));
        return *this;
    }

    inline void set(const Matrix4x4f& other) { memcpy(m_matrix, other.m_matrix, sizeof(m_matrix)); }

    inline void set(const float* ptr) { memcpy(m_matrix, ptr, sizeof(m_matrix)); }

    inline void set(float a00, float a01, float a02, float a03, float a10, float a11, float a12, float a13, float a20,
                    float a21, float a22, float a23, float a30, float a31, float a32, float a33)
    {
        m_matrix[0][0] = a00;
        m_matrix[0][1] = a10;
        m_matrix[0][2] = a20;
        m_matrix[0][3] = a30;
        m_matrix[1][0] = a01;
        m_matrix[1][1] = a11;
        m_matrix[1][2] = a21;
        m_matrix[1][3] = a31;
        m_matrix[2][0] = a02;
        m_matrix[2][1] = a12;
        m_matrix[2][2] = a22;
        m_matrix[2][3] = a32;
        m_matrix[3][0] = a03;
        m_matrix[3][1] = a13;
        m_matrix[3][2] = a23;
        m_matrix[3][3] = a33;
    }

    // 静的メンバー変数として単位行列を保持
    static const Matrix4x4f m_identity_matrix;

    static const Matrix4x4f& identityMatrix() { return m_identity_matrix; }

    // 新しいメソッドを追加
    inline bool isIdentity() const { return memcmp(m_matrix, m_identity_matrix.m_matrix, sizeof(m_matrix)) == 0; }

    inline void setToIdentity()
    {
        memset(m_matrix, 0, sizeof(m_matrix));
        m_matrix[0][0] = m_matrix[1][1] = m_matrix[2][2] = m_matrix[3][3] = 1.0f;
    }

    bool       setToInverse();
    Matrix4x4f inverted(bool* inverse = nullptr) const;

    void translate(const Point3f& vec);
    void translate(float x, float y, float z);
    void rotate(float angle, const Point3f& axis);
    void rotate(float angle, float x, float y, float z);
    void rotateDegree(float degree, const Point3f& axis);
    void rotateDegree(float degree, float x, float y, float z);
    void scale(float x, float y, float z);
    void scale(float factor);

    Point3f translate() const { return Point3f(m_matrix[3][0], m_matrix[3][1], m_matrix[3][2]); }

    void ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane);
    void lookAt(const Point3f& eye, const Point3f& center, const Point3f& up);
    void perspective(float fovY, float aspect, float nearPlane, float farPlane);

    /// 法線変換行列
    void normalMatrix(float (*matrix)[3]) const;

    /// 単純回転成分
    Matrix4x4f rotateMatrix() const;

    Point3f    preMult(const Point3f& v) const;
    Point3f    postMult(const Point3f& v) const;
    Point3f    operator*(const Point3f& v) const;
    Point4f    preMult(const Point4f& v) const;
    Point4f    postMult(const Point4f& v) const;
    Point4f    operator*(const Point4f& v) const;
    Matrix4x4f preMult(const Matrix4x4f&) const;
    Matrix4x4f postMult(const Matrix4x4f&) const;

    BoundingBox3f preMult(const BoundingBox3f& box) const;
    BoundingBox3f operator*(const BoundingBox3f& box) const;

    void operator*=(const Matrix4x4f& other) { *this = preMult(other); }

    Matrix4x4f operator*(const Matrix4x4f& other) const { return preMult(other); }

    Planef preMult(const Planef& plane) const;
    Planef operator*(const Planef& plane) const;

    Planef toPlane() const;

    void swapAxes(int axis1, int axis2)
    {
        if ((axis1 < 0) || (axis1 > 3) || (axis2 < 0) || (axis2 > 3) || (axis1 == axis2)) {
            return;
        }

        // 行を入れ替え
        for (int i = 0; i < 3; ++i) {
            std::swap(m_matrix[axis1][i], m_matrix[axis2][i]);
        }
    }

    inline void pickMatrix(float x, float y, float width, float height, int viewport[4])
    {
        setToIdentity();
        m_matrix[0][0] = viewport[2] / width;
        m_matrix[1][1] = viewport[3] / height;
        m_matrix[3][0] = (viewport[2] + 2.0f * (viewport[0] - x)) / width;
        m_matrix[3][1] = (viewport[3] + 2.0f * (viewport[1] - y)) / height;
    }

protected:
    float m_matrix[4][4];    /// 列優先形式で格納 [列][行]
};

typedef std::vector<Matrix4x4f> VecMatrix4x4f;

inline Point3f operator*(const Point3f& v, const Matrix4x4f& m)
{
    return m.postMult(v);
}

inline Point4f operator*(const Point4f& v, const Matrix4x4f& m)
{
    return m.postMult(v);
}

#ifdef USE_QT
inline QMatrix4x4 operator*(const QMatrix4x4& m2, const Matrix4x4f& m)
{
    return m.postMult(m2);
}
#endif

CORE_NAMESPACE_END

#endif    // CORE_MATRIX4X4F_H
