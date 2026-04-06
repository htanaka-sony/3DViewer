#ifndef CORE_VOXEL_H
#define CORE_VOXEL_H

#include "Math/Point3d.h"
#include "Math/Point3i.h"

#include "MutexWrapper.h"
#include "Scene/Node.h"
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

    void  markRenderDirty();
    void  resetRenderDirty();
    void  setRenderData(void* render_data, std::function<void(void*, bool)> deleter);
    bool  isRenderDirty() const;
    void  deleteRenderData(bool direct_delete);
    void* renderData() { return m_render_data; }

    const std::vector<Point3f>&      displayVertices() const { return m_vertices; }
    const std::vector<unsigned int>& displayIndices() const { return m_indices; }

    std::vector<Point3f>&      displayVertices() { return m_vertices; }
    std::vector<unsigned int>& displayIndices() { return m_indices; }

    const std::vector<Point3f>& displayEditVertices() const
    {
        return m_edit_buffer_2 ? m_edit_vertices_2 : m_edit_vertices;
    }
    const std::vector<unsigned int>& displayEditIndices() const
    {
        return m_edit_buffer_2 ? m_edit_indices_2 : m_edit_indices;
    }
    const std::vector<unsigned int>& displayEditSegmentIndices() const
    {
        return m_edit_buffer_2 ? m_edit_segments_indices_2 : m_edit_segments_indices;
    }

    std::vector<Point3f>& displayEditVerticesTemp() { return !m_edit_buffer_2 ? m_edit_vertices_2 : m_edit_vertices; }
    std::vector<unsigned int>& displayEditIndicesTemp() { return !m_edit_buffer_2 ? m_edit_indices_2 : m_edit_indices; }
    std::vector<unsigned int>& displayEditSegmentIndicesTemp()
    {
        return !m_edit_buffer_2 ? m_edit_segments_indices_2 : m_edit_segments_indices;
    }

    void switchEditDisplayBuffer() { m_edit_buffer_2 = !m_edit_buffer_2; }
    bool isEditDisplayBuffer2() { return m_edit_buffer_2; }
    bool isEnableEditDisplayData() const { return m_eneble_edit_display; }
    void setEnalbeEditDisplayData(bool enable) { m_eneble_edit_display = enable; }
    void setDisplayEditData(std::vector<Point3f>& verticies, std::vector<unsigned int>& indices)
    {
        if (m_edit_buffer_2) {
            m_edit_vertices_2 = verticies;
            m_edit_indices_2  = indices;
        }
        else {
            m_edit_vertices = verticies;
            m_edit_indices  = indices;
        }
    }
    void clearDisplayEditData()
    {
        m_eneble_edit_display = false;
        m_edit_vertices.clear();
        m_edit_indices.clear();
        m_edit_vertices_2.clear();
        m_edit_indices_2.clear();
        m_edit_segments_indices.clear();
        m_edit_segments_indices_2.clear();
        m_edit_buffer_2 = false;
    }
    void setDisplayData(const std::vector<Point3f>& verticies, std::vector<unsigned int>& indices)
    {
        m_vertices = verticies;
        m_indices  = indices;
    }

    void markSegmentsGroupDirty() { m_segments_group_dirty = true; }
    void resetSegmentsGroupDirty() { m_segments_group_dirty = false; }
    bool isSegmentsGroupDirty() const { return m_segments_group_dirty; }
    void createDisplaySegmentsGroupData();

    const std::vector<unsigned int>&  displaySegmentIndices() const { return m_segments_indices; }
    const std::vector<BoundingBox3f>& displaySegmentsGroupBox() const { return m_segments_group_box; }
    const std::vector<unsigned int>&  displaySegmentsGroupStart() const { return m_segments_group_start_index; }

    const std::vector<BoundingBox3f>& displayTriaGroupBox() const { return m_tria_group_box; }
    const std::vector<unsigned int>&  displayTriaGroupStart() const { return m_tria_group_start_index; }
    void                              createGroupBoundingBox();

    void  setProjectionOpt(Node* project_node) { m_projection_voxel_scalar = project_node; }
    Node* projectionOpt();

    void setCreateSectionLine(bool create) { m_create_section_line = create; }
    bool isCreateSectionLine() const { return m_create_section_line; }

    virtual bool isCreateFill() const { return true; }

    void setVboUse(bool use) { m_vbo_use = use; }
    bool isVboUse() const { return m_vbo_use; }

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

public:
    virtual ObjectType    type() const override { return ObjectType::Voxel; }
    virtual BoundingBox3f calculateBoundingBox(const Matrix4x4f& parent_matrix, bool only_visible,
                                               bool including_text) const override;

    virtual void updateBoundingBox() override;

    virtual bool isVoxel() const override { return true; }

protected:
    void initMemory();
    void deleteMemory();
    void deleteZMemory();

    int bytesPerXY() const;

    void appendQuad(const Point3f& p0, const Point3f& p1, const Point3f& p2, const Point3f& p3, /*const Point3f& norm,*/
                    int get_seg_flg);

protected:
    unsigned char** m_data = nullptr;    /// ビット単位でボクセル保持（XY平面でuchar配列、Z方向でその配列）

    unsigned char** m_compress_data = nullptr;
    int*            m_compress_size = nullptr;

    Point3d m_org;       /// 原点
    Point3d m_delta;     /// セルサイズ
    Point3i m_number;    /// 個数

    /// Display Data
    std::vector<Point3f>      m_vertices;
    std::vector<unsigned int> m_indices;

    std::vector<unsigned int> m_segments_indices;
    /// TODO: 暫定: 描画高速化のため線分をグループで分ける
    std::vector<BoundingBox3f> m_segments_group_box;
    std::vector<unsigned int>  m_segments_group_start_index;

    /// TODO: 暫定: 高速化のためトリアをグループで分ける
    std::vector<BoundingBox3f> m_tria_group_box;
    std::vector<unsigned int>  m_tria_group_start_index;

    /// TODO: インスタンスが考慮できていない（配置違いで結果が変わる）Nodeとのmapで持つとか検討必要
    /// Display Edit Data(ClippingData)
    std::vector<Point3f>      m_edit_vertices;
    std::vector<unsigned int> m_edit_indices;
    std::vector<unsigned int> m_edit_segments_indices;

    /// TODO: 暫定　高速化のため、バッファを二つもつ
    std::vector<Point3f>      m_edit_vertices_2;
    std::vector<unsigned int> m_edit_indices_2;
    std::vector<unsigned int> m_edit_segments_indices_2;

    /// Render Data
    void*                            m_render_data         = nullptr;
    std::function<void(void*, bool)> m_render_data_deleter = nullptr;

    /// フラグまとめ(暫定のものも含む）
    bool m_eneble_edit_display  = false;
    bool m_edit_buffer_2        = false;
    bool m_render_dirty         = true;
    bool m_segments_group_dirty = true;

    bool m_create_section_line = true;
    bool m_vbo_use             = true;

    bool m_draw_shading   = true;
    bool m_draw_wireframe = false;

    WeakRefPtr<Node> m_projection_voxel_scalar;

    /// 高速・省メモリ化
    ReadPerformInfo* m_read_perform_info = nullptr;
};

CORE_NAMESPACE_END

#endif    // CORE_VOXEL_H
