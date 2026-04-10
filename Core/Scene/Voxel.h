#ifndef CORE_VOXEL_H
#define CORE_VOXEL_H

#include "Math/Point3d.h"
#include "Math/Point3i.h"

#include "MutexWrapper.h"
#include "Scene/Node.h"
#include "Scene/RenderMesh.h"
#include "Scene/Shape.h"

#include <functional>

CORE_NAMESPACE_BEGIN

class CORE_EXPORT Voxel : public Shape {
    DECLARE_META_OBJECT(Voxel)
protected:
    Voxel();
    Voxel(const Voxel& other);
    ~Voxel();

    Voxel(const double x, const double y, const double z, const double dx, const double dy, const double dz,
          const int nx, const int ny, const int nz);
    Voxel(const Point3d& org, const Point3d& delta, const Point3i& number);

    static constexpr unsigned char g_v_bits[8]  = {1, 2, 4, 8, 16, 32, 64, 128};
    static constexpr unsigned char g_v_nbits[8] = {255 - 1,  255 - 2,  255 - 4,  255 - 8,
                                                   255 - 16, 255 - 32, 255 - 64, 255 - 128};

public:
    void init(const double x, const double y, const double z, const double dx, const double dy, const double dz,
              const int nx, const int ny, const int nz);
    void init(const Point3d& org, const Point3d& delta, const Point3i& number);

    void initVoxelData();
    void deleteVoxelData();

    void unite(Voxel* other);

    bool isEqualCoodinate(const Voxel* other) const;

    void setFullZeroZ(const int& iz);

    const Point3d& org() const { return m_org; }
    const Point3d& delta() const { return m_delta; }
    const Point3i& number() const { return m_number; }

    double x() const { return m_org.x(); }
    double y() const { return m_org.y(); }
    double z() const { return m_org.z(); }
    void   setX(double x) { m_org.setX(x); }
    void   setY(double y) { m_org.setY(y); }
    void   setZ(double z) { m_org.setZ(z); }

    double dX() const { return m_delta.x(); }
    double dY() const { return m_delta.y(); }
    double dZ() const { return m_delta.z(); }
    void   setDX(double dx) { m_delta.setX(dx); }
    void   setDY(double dy) { m_delta.setY(dy); }
    void   setDZ(double dz) { m_delta.setZ(dz); }

    int nX() const { return m_number.x(); }
    int nY() const { return m_number.y(); }
    int nZ() const { return m_number.z(); }

    virtual int    originalX() { return nX(); }
    virtual int    originalY() { return nY(); }
    virtual int    originalZ() { return nZ(); }
    virtual double originalDX() const { return dX(); }
    virtual double originalDY() const { return dY(); }
    virtual double originalDZ() const { return dZ(); }

    void setCell(const int ix, const int iy, const int iz, MutexWrapper& mutex);
    void setCell(const int ix, const int iy, const int iz, const bool data);
    void setXCells(const int ix_min, const int ix_max, const int iy, const int iz, const bool data);
    bool cell(const int ix, const int iy, const int iz) const;

    inline bool cell(const int ixiy, const int iz) const
    {
        if (!m_data[iz]) return false;

        return m_data[iz][ixiy >> 3] & g_v_bits[ixiy & 0x07];
    }

    void invertCell(const int ix, const int iy, const int iz);
    void invertXCells(const int ix_min, const int ix_max, const int iy, const int iz);

    void clearZ(int iz);

    bool hasBuffer() const;

    Point3d point(const int ix, const int iy, const int iz) const;

    Point3f pointf(const int ix, const int iy, const int iz) const;

    Point3i index(const float x, const float y, const float z) const;

    Point3i index(const Point3d& pos) const;
    Point3i index(const Point3f& pos) const;

    bool cell(const Point3f& pos) const;

    void createDisplayData();
    void createDisplayDataXYOnly();    /// 2次元
    void clearDisplayData();

    void  setProjectionOpt(Node* project_node) { m_projection_voxel_scalar = project_node; }
    Node* projectionOpt();

    unsigned char** data() const { return m_data; }

    void compressData();
    void decompressData(bool delete_decompress_data = true);

    bool isDrawShading() const { return m_draw_shading; }
    bool isDrawWireframe() const { return m_draw_wireframe; }

    void setDrawShading(bool shading) { m_draw_shading = shading; }
    void setDrawWireframe(bool wireframe) { m_draw_wireframe = wireframe; }

    struct ReadPerformInfo {
        Point3i           m_number;    /// 個数(元）
        std::vector<int>* m_new_old_index_x = nullptr;
        std::vector<int>* m_new_old_index_y = nullptr;
        std::vector<int>* m_new_old_index_z = nullptr;
        ~ReadPerformInfo();
    };
    ReadPerformInfo* createReadPerformInfo();
    void             unsetReadPerformInfo();

    RenderMesh* renderMesh();

    void setCreateSectionLine(bool create);

    /// BOX生成(TEST): 別要素に移動すべき
    void createBoxShape();

public:
    virtual bool isVoxel() const override { return true; }

protected:
    void initMemory();
    void deleteMemory();
    void deleteZMemory();

    int bytesPerXY() const;

protected:
    unsigned char** m_data = nullptr;    /// ビット単位でボクセル保持（XY平面でuchar配列、Z方向でその配列）

    unsigned char** m_compress_data = nullptr;
    int*            m_compress_size = nullptr;

    Point3d m_org;       /// 原点
    Point3d m_delta;     /// セルサイズ
    Point3i m_number;    /// 個数

    bool m_draw_shading   = true;
    bool m_draw_wireframe = false;

    WeakRefPtr<Node> m_projection_voxel_scalar;

    /// 高速・省メモリ化
    ReadPerformInfo* m_read_perform_info = nullptr;
};

CORE_NAMESPACE_END

#endif    // CORE_VOXEL_H
