#ifndef CORE_VOXELSCALAR_H
#define CORE_VOXELSCALAR_H

#include "Scene/Voxel.h"

CORE_NAMESPACE_BEGIN

/// コメント
/// 現状は3Dテクスチャでの描画 / 値限定してのオブジェクト表示の時はVoxelとして描画　を想定
/// 3Dテクスチャは0-1範囲だが、シェーダー側で考慮するので通常の形状の大きさで描画

class CORE_EXPORT VoxelScalar : public Voxel {
    DECLARE_META_OBJECT(VoxelScalar)
protected:
    VoxelScalar();
    VoxelScalar(const VoxelScalar& other);
    ~VoxelScalar();

public:
    virtual ObjectType type() const override { return ObjectType::VoxelScalar; }

    void  markRenderTextureDirty();
    void  resetRenderTextureDirty();
    void  setRenderTextureData(void* render_data, std::function<void(void*, bool)> deleter);
    bool  isRenderTextureDirty() const;
    void  deleteRenderTextureData(bool direct_delete);
    void* renderTextureData() const { return m_render_texture_data; }

    const float* scalarData() const;

    void setScalarData(float* data, bool need_to_delete);

    void createShapeForTexture();

    void setMinMax(float min_value = -1, float max_value = -1);

    float minValue() const { return m_min_value; }
    float maxValue() const { return m_max_value; }

    void setRangeMinMax(const std::vector<std::pair<float, float>>& merged_range);
    void setRangeMinMax(const std::vector<float>& range_min, const std::vector<float>& range_max)
    {
        m_merged_range_min = range_min;
        m_merged_range_max = range_max;
    }
    const auto& rangeMin() const { return m_merged_range_min; }
    const auto& rangeMax() const { return m_merged_range_max; }

    bool isIncludingRangeMinMax(float value) const;

    void rangeIndex(float range_min, float range_max, Point3i& min_index, Point3i& max_index) const;

    void set2DTexture(bool texture_2d) { m_2d_texture = texture_2d; }
    bool is2DTexture() const { return m_2d_texture; }

    virtual bool isCreateFill() const override { return !m_2d_texture; }

    /// 復帰可能な無効データを設定する
    /// 現状マイナスがありえないのでマイナスにし、復帰時にマイナスはプラスに戻す
    bool setInvalidData(Voxel* invalid_area);
    bool setInvalid2dData(Voxel* invalid_area, const Matrix4x4f& path_matrix, float eps);
    void backInvalidData();

    Point3f pointf_nearest_x_lower(float x_base, float x_new_delta, int ix, int iy, int iz) const;
    Point3f pointf_nearest_x_upper(float x_base, float x_new_delta, int ix, int iy, int iz) const;
    bool    isBoundaryVoxel(unsigned char* boundary_data, int ix, int iy, int* offset, __int64 bNx, __int64 bNy);

    void   createDisplayDataXYOnlyBoundary(Voxel* bundary_vox, int* offset);
    Voxel* boundaryVoxel() { return m_range_boundary_vox.ptr(); }

    void  setBoundaryDeltaX(float delta) { m_boundary_delta_x = delta; }
    void  setBoundaryOrgX(float org) { m_boundary_org_x = org; }
    float boundaryDeltaX() { return m_boundary_delta_x; }
    float boundaryOrgX() { return m_boundary_org_x; }

    virtual int    originalX() { return m_data_compress ? m_original_number.x() : nX(); }
    virtual int    originalY() { return m_data_compress ? m_original_number.y() : nY(); }
    virtual int    originalZ() { return m_data_compress ? m_original_number.z() : nZ(); }
    virtual double originalDX() const { return m_data_compress ? m_original_delta.x() : dX(); }
    virtual double originalDY() const { return m_data_compress ? m_original_delta.y() : dY(); }
    virtual double originalDZ() const { return m_data_compress ? m_original_delta.z() : dZ(); }

    bool isDataCompress() const { return m_data_compress; }

    void setOriginalData(Point3i original_number, Point3d original_delta)
    {
        m_data_compress   = true;
        m_original_number = original_number;
        m_original_delta  = original_delta;
    }

    float value(const Point3f& pos);

protected:
    float* m_scalar_data                = nullptr;    /// 元データ
    bool   m_need_to_delete_scalar_data = false;

    bool m_2d_texture = false;    /// 2D解析結果も扱う（ほぼ同一しょりなので）

    /// Render Texture Data
    void*                            m_render_texture_data         = nullptr;
    std::function<void(void*, bool)> m_render_texture_data_deleter = nullptr;
    bool                             m_render_texture_dirty        = true;

    float m_min_value = -1;
    float m_max_value = -1;

    /// 必ず同数
    std::vector<float> m_merged_range_min;
    std::vector<float> m_merged_range_max;

    /// 暫定 - データ圧縮したときの元データサイズ
    bool    m_data_compress = false;
    Point3d m_original_delta;     /// セルサイズ
    Point3i m_original_number;    /// 個数

    /// 暫定 - 斜め解析結果断面のセルずれ対応
    /// 　※ 汎用性がない（汎用的には斜めに限らず全部考慮するか別ロジックで）
    /// 　　 とりあえず現状の利用用途的に足りるように実装
    RefPtr<Voxel> m_range_boundary_vox;
    float         m_boundary_delta_x = 0.0f;
    float         m_boundary_org_x   = 0.0f;
};

CORE_NAMESPACE_END

#endif    // CORE_VOXELSCALAR_H
