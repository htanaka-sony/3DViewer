#include "VoxelScalar.h"
#include <execution>

CORE_NAMESPACE_BEGIN

DEFINE_META_OBJECT(VoxelScalar)

VoxelScalar::VoxelScalar() : Voxel() {}

VoxelScalar::VoxelScalar(const VoxelScalar& other) {}

VoxelScalar::~VoxelScalar()
{
    deleteRenderTextureData(false);
    if (m_need_to_delete_scalar_data) {
        delete[] m_scalar_data;
    }
}

void VoxelScalar::markRenderTextureDirty()
{
    m_render_texture_dirty = true;
}

void VoxelScalar::resetRenderTextureDirty()
{
    m_render_texture_dirty = false;
}

void VoxelScalar::setRenderTextureData(void* render_data, std::function<void(void*, bool)> deleter)
{
    m_render_texture_data         = render_data;
    m_render_texture_data_deleter = deleter;

    resetRenderTextureDirty();
}

bool VoxelScalar::isRenderTextureDirty() const
{
    return m_render_texture_dirty;
}

void VoxelScalar::deleteRenderTextureData(bool direct_delete)
{
    /// Render
    if (m_render_texture_data && m_render_texture_data_deleter) {
        m_render_texture_data_deleter(m_render_texture_data, direct_delete);
    }
    m_render_texture_data         = nullptr;
    m_render_texture_data_deleter = nullptr;
}

const float* VoxelScalar::scalarData() const
{
    return m_scalar_data;
}

void VoxelScalar::setScalarData(float* data, bool need_to_delete)
{
    m_scalar_data                = data;
    m_need_to_delete_scalar_data = need_to_delete;
}

//// 3Dテクスチャ用のBox（固定）
////  ※ クリッピングも対応する
void VoxelScalar::createShapeForTexture()
{
    auto mesh = renderMesh();

    if (is2DTexture()) {
        Point3f min_pos = Point3f(m_org.x(), m_org.y(), m_org.z());
        Point3f max_pos = min_pos + Point3f(m_delta.x() * m_number.x(), m_delta.y() * m_number.y(), 0.0f);

        mesh->appendQuad(min_pos, Point3f(max_pos.x(), min_pos.y(), min_pos.z()), max_pos,
                         Point3f(min_pos.x(), max_pos.y(), min_pos.z()), /*Point3f(0, 0, 1),*/ 0);
    }
    else {
        Point3f min_pos = Point3f(m_org.x(), m_org.y(), m_org.z());
        Point3f max_pos =
            min_pos + Point3f(m_delta.x() * m_number.x(), m_delta.y() * m_number.y(), m_delta.z() * m_number.z());

        /// Z-
        mesh->appendQuad(min_pos, Point3f(min_pos.x(), max_pos.y(), min_pos.z()),
                         Point3f(max_pos.x(), max_pos.y(), min_pos.z()), Point3f(max_pos.x(), min_pos.y(), min_pos.z()),
                         /*Point3f(0, 0, -1),*/ 0);

        /// Z+
        mesh->appendQuad(Point3f(min_pos.x(), min_pos.y(), max_pos.z()), Point3f(max_pos.x(), min_pos.y(), max_pos.z()),
                         max_pos, Point3f(min_pos.x(), max_pos.y(), max_pos.z()), /*Point3f(0, 0, 1),*/ 0);

        /// X-
        mesh->appendQuad(min_pos, Point3f(min_pos.x(), min_pos.y(), max_pos.z()),
                         Point3f(min_pos.x(), max_pos.y(), max_pos.z()), Point3f(min_pos.x(), max_pos.y(), min_pos.z()),
                         /*Point3f(-1, 0, 0),*/ 0);

        /// X+
        mesh->appendQuad(Point3f(max_pos.x(), min_pos.y(), min_pos.z()), Point3f(max_pos.x(), max_pos.y(), min_pos.z()),
                         max_pos, Point3f(max_pos.x(), min_pos.y(), max_pos.z()), /*Point3f(1, 0, 0),*/ 0);

        /// Y-
        mesh->appendQuad(min_pos, Point3f(max_pos.x(), min_pos.y(), min_pos.z()),
                         Point3f(max_pos.x(), min_pos.y(), max_pos.z()), Point3f(min_pos.x(), min_pos.y(), max_pos.z()),
                         /*Point3f(0, -1, 0),*/ 0);

        /// Y+
        mesh->appendQuad(Point3f(min_pos.x(), max_pos.y(), min_pos.z()), Point3f(min_pos.x(), max_pos.y(), max_pos.z()),
                         max_pos, Point3f(max_pos.x(), max_pos.y(), min_pos.z()), /*Point3f(0, 1, 0),*/ 0);
    }
}

void VoxelScalar::setMinMax(float min_value, float max_value)
{
    if (min_value < 0 || max_value < 0) {
        float  min_temp  = FLT_MAX;
        float  max_temp  = -FLT_MAX;
        size_t data_size = (size_t)m_number.x() * (size_t)m_number.y() * (size_t)m_number.z();
        for (size_t ic = 0; ic < data_size; ++ic) {
            /// ゼロデータは除外
            if (m_scalar_data[ic] == 0) {
                continue;
            }

            if (min_temp > m_scalar_data[ic]) {
                min_temp = m_scalar_data[ic];
            }
            if (max_temp < m_scalar_data[ic]) {
                max_temp = m_scalar_data[ic];
            }
        }
        if (min_value < 0) {
            min_value = min_temp;
        }
        if (max_value < 0) {
            max_value = max_temp;
        }
    }

    m_min_value = min_value;
    m_max_value = max_value;
}

void VoxelScalar::setRangeMinMax(const std::vector<std::pair<float, float>>& merged_range)
{
    m_merged_range_min.resize(merged_range.size());
    m_merged_range_max.resize(merged_range.size());
    for (int ic = 0; ic < merged_range.size(); ++ic) {
        m_merged_range_min[ic] = merged_range[ic].first;
        m_merged_range_max[ic] = merged_range[ic].second;
    }
}

bool VoxelScalar::isIncludingRangeMinMax(float value) const
{
    if (m_merged_range_min.size() == 0) {
        return true;    /// 制限なし
    }

    size_t size =
        (m_merged_range_min.size() < m_merged_range_max.size()) ? m_merged_range_min.size() : m_merged_range_max.size();
    for (int ic = 0; ic < size; ++ic) {
        if ((m_merged_range_min[ic] <= value) && (value <= m_merged_range_max[ic])) {
            return true;
        }
    }
    return false;
}

void VoxelScalar::rangeIndex(float range_min, float range_max, Point3i& min_index, Point3i& max_index) const
{
    if (!m_scalar_data) {
        return;
    }

    if (is2DTexture()) {
        int Nx = nX();
        int Ny = nY();

        min_index.set(Nx - 1, Ny - 1, 0);
        max_index.set(0, 0, 0);

        for (int iy = 0; iy < Ny; ++iy) {
            __int64 index = (__int64)iy * (__int64)Nx;
            for (int ix = 0; ix < Nx; ++ix) {
                float value = m_scalar_data[index + (__int64)ix];
                if (value >= range_min && value <= range_max) {
                    if (ix < min_index.x()) min_index.setX(ix);
                    if (iy < min_index.y()) min_index.setY(iy);

                    if (ix > max_index.x()) max_index.setX(ix);
                    if (iy > max_index.y()) max_index.setY(iy);
                }
            }
        }
    }
    else {
        int Nx = nX();
        int Ny = nY();
        int Nz = nZ();

        min_index.set(Nx - 1, Ny - 1, Nz - 1);
        max_index.set(0, 0, 0);

        struct MinMax {
            int min_ix, min_iy, min_iz;
            int max_ix, max_iy, max_iz;
            MinMax(int Nx, int Ny, int Nz)
                : min_ix(Nx - 1)
                , min_iy(Ny - 1)
                , min_iz(Nz - 1)
                , max_ix(0)
                , max_iy(0)
                , max_iz(0)
            {
            }
            MinMax() {}
        };

        std::vector<MinMax> thread_minmax(Nx);

        std::vector<int> ix_vec(Nx);
        std::iota(ix_vec.begin(), ix_vec.end(), 0);

        std::for_each(std::execution::par, ix_vec.begin(), ix_vec.end(), [&](int ix) {
            MinMax local(Nx, Ny, Nz);
            for (int iy = 0; iy < Ny; ++iy) {
                for (int iz = 0; iz < Nz; ++iz) {
                    __int64 index = (__int64)ix * ((__int64)Ny * (__int64)Nz) + (__int64)iy * (__int64)Nz + (__int64)iz;
                    float   value = m_scalar_data[index];
                    if (value >= range_min && value <= range_max) {
                        if (ix < local.min_ix) local.min_ix = ix;
                        if (iy < local.min_iy) local.min_iy = iy;
                        if (iz < local.min_iz) local.min_iz = iz;
                        if (ix > local.max_ix) local.max_ix = ix;
                        if (iy > local.max_iy) local.max_iy = iy;
                        if (iz > local.max_iz) local.max_iz = iz;
                    }
                }
            }
            thread_minmax[ix] = local;
        });

        for (const auto& mm : thread_minmax) {
            if (mm.min_ix < min_index.x()) min_index.setX(mm.min_ix);
            if (mm.min_iy < min_index.y()) min_index.setY(mm.min_iy);
            if (mm.min_iz < min_index.z()) min_index.setZ(mm.min_iz);
            if (mm.max_ix > max_index.x()) max_index.setX(mm.max_ix);
            if (mm.max_iy > max_index.y()) max_index.setY(mm.max_iy);
            if (mm.max_iz > max_index.z()) max_index.setZ(mm.max_iz);
        }
        /*
        for (int ix = 0; ix < Nx; ++ix) {
            for (int iy = 0; iy < Ny; ++iy) {
                __int64 index = (__int64)ix * ((__int64)Ny * (__int64)Nz) + (__int64)iy * (__int64)Nz;
                for (int iz = 0; iz < Nz; ++iz) {
                    float value = m_scalar_data[index + (__int64)iz];
                    if (value >= range_min && value <= range_max) {
                        if (ix < min_index.x()) min_index.setX(ix);
                        if (iy < min_index.y()) min_index.setY(iy);
                        if (iz < min_index.z()) min_index.setZ(iz);

                        if (ix > max_index.x()) max_index.setX(ix);
                        if (iy > max_index.y()) max_index.setY(iy);
                        if (iz > max_index.z()) max_index.setZ(iz);
                    }
                }
            }
        }
        */
    }
}

bool VoxelScalar::setInvalidData(Voxel* invalid_area)
{
    /// 暫定 - 2Dは対象外（座標系考慮した別ロジック必要）
    if (is2DTexture()) {
        return false;
    }

    if (!invalid_area) {
        return false;
    }
    int nX = this->nX();
    int nY = this->nY();
    int nZ = this->nZ();

    if (!isEqualCoodinate(invalid_area)) {
        __int64 size = (__int64)nX * (__int64)nY * (__int64)nZ;
        for (__int64 ic = 0; ic < size; ++ic) {
            if (m_scalar_data[ic] == 0) {
                m_scalar_data[ic] = -FLT_MAX;
            }
            else {
                m_scalar_data[ic] = -m_scalar_data[ic];
            }
        }

        unsigned char** myData2 = (unsigned char**)invalid_area->data();

        const auto& org_other   = invalid_area->org();
        const auto& delta_other = invalid_area->delta();

        int nX2 = invalid_area->nX();
        int nY2 = invalid_area->nY();
        int nZ2 = invalid_area->nZ();

        /// 座標系重複あるので考慮
        int                                           pre_z = -1;
        std::vector<std::pair<int, std::vector<int>>> z_values;
        for (int iz = 0; iz < nZ2; ++iz) {
            if (!myData2[iz]) {
                continue;
            }
            double z  = org_other[2] + delta_other[2] * iz;
            int    jz = (int)((z - m_org[2]) / m_delta[2]);
            if (jz < 0) {
                continue;
            }
            if (jz >= nZ) {
                break;
            }

            if (pre_z != jz) {
                std::pair<int, std::vector<int>> temp;
                temp.first = jz;
                temp.second.emplace_back(iz);
                z_values.emplace_back(temp);
                pre_z = jz;
            }
            else {
                z_values.back().second.emplace_back(iz);
            }
        }

        std::vector<int> ix_to_jx(nX2);
        for (int ix = 0; ix < nX2; ++ix) {
            double x     = org_other[0] + delta_other[0] * ix;
            int    jx    = (int)((x - m_org[0]) / m_delta[0]);
            ix_to_jx[ix] = (jx >= 0 && jx < nX) ? jx : -1;
        }

        std::vector<int> iy_to_jy(nY2);
        for (int iy = 0; iy < nY2; ++iy) {
            double y     = org_other[1] + delta_other[1] * iy;
            int    jy    = (int)((y - m_org[1]) / m_delta[1]);
            iy_to_jy[iy] = (jy >= 0 && jy < nY) ? jy : -1;
        }

        std::for_each(
            std::execution::par, z_values.begin(), z_values.end(), [&](std::pair<int, std::vector<int>>& z_value) {
                int               jz  = z_value.first;
                std::vector<int>& izs = z_value.second;
                for (auto& iz : izs) {
                    auto z_buffer = myData2[iz];

                    for (int iy = 0; iy < nY2; ++iy) {
                        int jy = iy_to_jy[iy];
                        if (jy == -1) continue;

                        for (int ix = 0; ix < nX2; ++ix) {
                            int jx = ix_to_jx[ix];
                            if (jx == -1) continue;

                            int index = ix + nX2 * iy;
                            if ((z_buffer[index >> 3] & g_v_bits[index & 0x07])) {
                                __int64 index =
                                    (__int64)jx * ((__int64)nY * (__int64)nZ) + (__int64)jy * (__int64)nZ + (__int64)jz;
                                if (m_scalar_data[index] < 0) {
                                    if (m_scalar_data[index] == -FLT_MAX) {
                                        m_scalar_data[index] = 0;
                                    }
                                    else {
                                        m_scalar_data[index] = -m_scalar_data[index];
                                    }
                                }
                            }
                        }
                    }
                }
            });
    }
    else {
        std::vector<int> iz_vec(nZ);
        std::iota(iz_vec.begin(), iz_vec.end(), 0);

        const auto data = invalid_area->data();

        std::for_each(std::execution::par, iz_vec.begin(), iz_vec.end(), [this, data, nX, nY, nZ](int iz) {
            if (!data[iz]) {
                for (int iy = 0; iy < nY; ++iy) {
                    for (int ix = 0; ix < nX; ++ix) {
                        __int64 index =
                            (__int64)ix * ((__int64)nY * (__int64)nZ) + (__int64)iy * (__int64)nZ + (__int64)iz;
                        if (m_scalar_data[index] == 0) {
                            m_scalar_data[index] = -FLT_MAX;
                        }
                        else {
                            m_scalar_data[index] = -m_scalar_data[index];
                        }
                    }
                }
            }
            else {
                auto z_buffer = data[iz];

                for (int iy = 0; iy < nY; ++iy) {
                    __int64 index_y = (__int64)m_number.x() * (__int64)iy;
                    for (int ix = 0; ix < nX; ++ix) {
                        __int64 index = (__int64)ix + index_y;
                        if (!(z_buffer[index >> 3] & g_v_bits[index & 0x07])) {
                            __int64 index_ =
                                (__int64)ix * ((__int64)nY * (__int64)nZ) + (__int64)iy * (__int64)nZ + (__int64)iz;
                            if (m_scalar_data[index_] == 0) {
                                m_scalar_data[index_] = -FLT_MAX;
                            }
                            else {
                                m_scalar_data[index_] = -m_scalar_data[index_];
                            }
                        }
                    }
                }
            }
        });
    }

    return true;
}

bool VoxelScalar::setInvalid2dData(Voxel* invalid_area, const Matrix4x4f& path_matrix, float eps)
{
    if (!invalid_area) {
        return false;
    }

    if (!is2DTexture()) {
        return false;
    }

    m_range_boundary_vox = nullptr;

    /*
    bool    x_delta_diff = false;
    Point3d max_pos      = m_org
                    + Point3d(m_delta.x() * (double)m_number.x(), m_delta.y() * (double)m_number.y(),
                              m_delta.z() * (double)m_number.z());
    Point3d min_pos     = m_org;
    Point3d diff_conv   = path_matrix * max_pos - path_matrix * min_pos;
    double  diff_conv_x = diff_conv.x() / (double)m_number.x();
    if (fabs(m_delta.x() - diff_conv_x) > eps) {
        x_delta_diff       = true;
        m_boundary_delta_x = diff_conv_x;

        /// 煩雑だが汎用？で実装
        /// グローバル座標系に戻してセルのずれを出す
        auto g_org = path_matrix * m_org;

        /// 上限（超えない最小の整数倍）
        int   m         = int(ceil(g_org.x() / m_delta[0]));
        float nearest_x = m * m_delta[0];

        nearest_x - g_org.x() ;

    }
    */
    bool x_delta_diff = (m_boundary_delta_x > eps);    // && (m_boundary_org_x <= eps);

    bool adjust_shape = (fabs(m_boundary_org_x) <= eps);

    int Nx = nX();
    int Ny = nY();

    std::vector<int> iy_vec(Ny);
    std::iota(iy_vec.begin(), iy_vec.end(), 0);

    // std::for_each(std::execution::par, iy_vec.begin(), iy_vec.end(),
    //                [this, path_matrix, x_delta_diff, invalid_area, Nx, Ny](int iy) {
    for (int iy = 0; iy < Ny; ++iy) {
        __int64 index = (__int64)iy * (__int64)Nx;

        Point3f local_pos(0, 0, (float)m_org[2]);
        local_pos[1] = (float)m_org[1] + (float)m_delta[1] * (float)((float)iy + 0.5);

        for (int ix = 0; ix < Nx; ++ix) {
            if (x_delta_diff) {
                local_pos[0]                = (float)m_org[0] + (float)m_delta[0] * ((float)ix + 0.001f);
                const Point3f& global_pos_0 = path_matrix * local_pos;
                bool           has_data_0   = invalid_area->cell(global_pos_0);

                local_pos[0]                = (float)m_org[0] + (float)m_delta[0] * ((float)ix + 0.5f);
                const Point3f& global_pos_1 = path_matrix * local_pos;
                bool           has_data_1   = invalid_area->cell(global_pos_1);

                local_pos[0]                = (float)m_org[0] + (float)m_delta[0] * ((float)ix + 0.999f);
                const Point3f& global_pos_2 = path_matrix * local_pos;
                bool           has_data_2   = invalid_area->cell(global_pos_2);

                if (has_data_0 && has_data_1 && has_data_2) {
                    continue;
                }
                else if (has_data_0 || has_data_1 || has_data_2) {
                    if (adjust_shape) {
                        /// 境界ボクセルを保持
                        if (m_range_boundary_vox == nullptr) {
                            m_range_boundary_vox = Voxel::createObject();
                            auto number          = m_number;
                            // number.setZ(3);    /// 判定のため3つ使う
                            number.setZ(2);
                            m_range_boundary_vox->init(m_org, m_delta, number);
                        }
                        if (has_data_2) {    /// 下限側の調整
                            m_range_boundary_vox->setCell(ix, iy, 0, true);
                        }
                        if (has_data_0) {    /// 上限側の調整
                            m_range_boundary_vox->setCell(ix, iy, 1, true);
                        }
                        // if (has_data_0 && has_data_2) {    /// 間空きの調整
                        //     m_range_boundary_vox->setCell(ix, iy, 2, true);
                        // }
                    }
                    continue;
                }
            }
            else {
                local_pos[0]                = (float)m_org[0] + (float)m_delta[0] * ((float)ix + 0.5f);
                const Point3f& global_pos_1 = path_matrix * local_pos;
                if (invalid_area->cell(global_pos_1)) {
                    continue;
                }
            }

            __int64 index_ = index + (__int64)ix;
            if (m_scalar_data[index_] == 0) {
                m_scalar_data[index_] = -FLT_MAX;
            }
            else {
                m_scalar_data[index_] = -m_scalar_data[index_];
            }
        }
    }    //);

    return true;
}

void VoxelScalar::backInvalidData()
{
    size_t max_index = nX() * nY() * nZ();
    for (size_t index = 0; index < max_index; ++index) {
        if (m_scalar_data[index] == -FLT_MAX) {
            m_scalar_data[index] = 0;
        }
        else if (m_scalar_data[index] < 0) {
            m_scalar_data[index] = -m_scalar_data[index];
        }
    }

    m_range_boundary_vox = nullptr;
}

Point3f VoxelScalar::pointf_nearest_x_lower(float x_base, float x_new_delta, int ix, int iy, int iz) const
{
    float x_target = m_org[0] + m_delta[0] * (float)ix;

    float rel_x = x_target - x_base;

    /// 下限（未満で最大の整数倍）
    int   m         = int(rel_x / x_new_delta);
    float nearest_x = x_base + m * x_new_delta;

    return Point3f(nearest_x, m_org[1] + m_delta[1] * (float)iy, m_org[2] + m_delta[2] * (float)iz);
}

Point3f VoxelScalar::pointf_nearest_x_upper(float x_base, float x_new_delta, int ix, int iy, int iz) const
{
    float x_target = m_org[0] + m_delta[0] * (float)ix;

    float rel_x = x_target - x_base;

    /// 上限（超えない最小の整数倍）
    int   m         = int(ceil(rel_x / x_new_delta));
    float nearest_x = x_base + m * x_new_delta;

    return Point3f(nearest_x, m_org[1] + m_delta[1] * (float)iy, m_org[2] + m_delta[2] * (float)iz);
}

bool VoxelScalar::isBoundaryVoxel(unsigned char* boundary_data, int ix, int iy, int* offset, __int64 bNx, __int64 bNxNy)
{
    if (!boundary_data) {
        return false;
    }
    __int64 index = (__int64)(ix + (offset ? offset[0] : 0)) + bNx * (__int64)(iy + (offset ? offset[1] : 0));
    if (index < 0 || index >= bNxNy) {
        return false;
    }
    return (boundary_data[index >> 3] & g_v_bits[index & 0x07]);
}

void VoxelScalar::createDisplayDataXYOnlyBoundary(Voxel* bundary_vox, int* offset)
{
    if (!m_data[0]) {
        return;
    }

    if (bundary_vox == nullptr) {
        createDisplayDataXYOnly();
        return;
    }

    auto boundary_data = bundary_vox->data();
    if (boundary_data == nullptr) {
        createDisplayDataXYOnly();
        return;
    }
    auto boundary_data_0 = boundary_data[0];
    auto boundary_data_1 = boundary_data[1];
    // auto boundary_data_2 = boundary_data[2];

    /// test
    /*
    bundary_vox->createDisplayDataXYOnly();
    boundary_data[0] = boundary_data_1;
    bundary_vox->createDisplayDataXYOnly();
    boundary_data[0] = boundary_data_0;
    m_indices        = bundary_vox->displayIndices();
    m_vertices       = bundary_vox->displayVertices();
    return;
    */

    auto mesh = renderMesh();

    std::vector<int> x_range(m_number.x());
    std::iota(x_range.begin(), x_range.end(), 0);

    std::vector<int> y_range(m_number.y());
    std::iota(y_range.begin(), y_range.end(), 0);

    std::map<std::pair<int, int>, std::vector<int>> ranges_value;

    /// 線分の取得フラグ
    int get_seg_flg = 2;

    auto z_buffer = m_data[0];

    int start_x;
    ranges_value.clear();

    float delta_x = m_boundary_delta_x;
    float org_x   = m_boundary_org_x;

    __int64 bNx   = bundary_vox->nX();
    __int64 bNxNy = (__int64)bundary_vox->nX() * (__int64)bundary_vox->nY();

    for (int y = 0; y < m_number.y(); ++y) {
        bool inCell  = false;
        int  index_y = m_number.x() * y;
        //__int64 index_by = bNx * (__int64)(y + (offset ? offset[1] : 0));

        for (int x = 0; x < m_number.x(); ++x) {
            int index = x + index_y;
            if (!inCell) {
                if (z_buffer[index >> 3] & g_v_bits[index & 0x07]) {
                    inCell = true;
                    /// 始点
                    start_x = x;
                    if (x == m_number.x() - 1) {
                        /// 終点
                        ranges_value[std::pair<int, int>(start_x, x + 1)].emplace_back(y);
                    }
                    else { /*
                         /// 終点を格納して始点開始する（中間対策）
                         if (boundary_data_2) {
                             __int64 index_b = (__int64)(x + (offset ? offset[0] : 0)) + index_by;
                             if (boundary_data_2[index_b >> 3] & g_v_bits[index_b & 0x07]) {
                                 inCell = false;
                                 /// 終点
                                 ranges_value[std::pair<int, int>(start_x, x)].emplace_back(y);

                                 inCell = true;
                                 /// 始点
                                 start_x = x;
                             }
                         }*/
                    }
                }
            }
            else {
                /// 終点を格納して始点開始する（中間対策）
                /*if (boundary_data_2) {
                    __int64 index_b = (__int64)(x + (offset ? offset[0] : 0)) + index_by;
                    if (boundary_data_2[index_b >> 3] & g_v_bits[index_b & 0x07]) {
                        inCell = false;
                        /// 終点
                        ranges_value[std::pair<int, int>(start_x, x)].emplace_back(y);

                        inCell = true;
                        /// 始点
                        start_x = x;
                    }
                }*/

                if (!(z_buffer[index >> 3] & g_v_bits[index & 0x07])) {
                    inCell = false;
                    /// 終点
                    ranges_value[std::pair<int, int>(start_x, x)].emplace_back(y);
                }
                else if (x == m_number.x() - 1) {
                    /// 終点
                    ranges_value[std::pair<int, int>(start_x, x + 1)].emplace_back(y);
                }
            }
        }
    }

    for (const auto& range : ranges_value) {
        auto [start_x, end_x] = range.first;
        const auto& y_values  = range.second;

        if (!y_values.empty()) {
            int start_y = y_values[0];
            int end_y   = start_y;

            for (int ic = 1; ic < (int)y_values.size(); ++ic) {
                if (y_values[ic] == end_y + 1) {
                    /// Y方向が連続している場合に拡張
                    end_y = y_values[ic];
                }
                else {
                    /// 境界上かチェック（ ※ end_y + 1はその次のセルを指すのでend_yであっている）
                    bool start_0 = isBoundaryVoxel(boundary_data_0, start_x, start_y, offset, bNx, bNxNy);
                    bool start_1 = isBoundaryVoxel(boundary_data_0, start_x, end_y, offset, bNx, bNxNy);
                    /// 両方trueなら調整（片方なら元データのままにする。解析データと形状不一致なので）
                    const Point3f& start1 = (start_0 && start_1)
                                              ? pointf_nearest_x_upper(org_x, delta_x, start_x, start_y, 0)
                                              : pointf(start_x, start_y, 0);
                    const Point3f& start2 = (start_0 && start_1)
                                              ? pointf_nearest_x_upper(org_x, delta_x, start_x, end_y + 1, 0)
                                              : pointf(start_x, end_y + 1, 0);

                    /// 境界上かチェック
                    bool end_0 = isBoundaryVoxel(boundary_data_1, end_x - 1, start_y, offset, bNx, bNxNy);
                    bool end_1 = isBoundaryVoxel(boundary_data_1, end_x - 1, end_y, offset, bNx, bNxNy);
                    /// 両方trueなら調整（片方なら元データのままにする。解析データと形状不一致なので）
                    const Point3f& end1 = (end_0 && end_1) ? pointf_nearest_x_lower(org_x, delta_x, end_x, start_y, 0)
                                                           : pointf(end_x, start_y, 0);
                    const Point3f& end2 = (end_0 && end_1) ? pointf_nearest_x_lower(org_x, delta_x, end_x, end_y + 1, 0)
                                                           : pointf(end_x, end_y + 1, 0);

                    mesh->appendQuad(start1, start2, end2, end1, /*norm,*/ get_seg_flg);
                    start_y = y_values[ic];
                    end_y   = start_y;
                }
            }

            /// 境界上かチェック（ ※ end_y + 1はその次のセルを指すのでend_yであっている）
            bool start_0 = isBoundaryVoxel(boundary_data_0, start_x, start_y, offset, bNx, bNxNy);
            bool start_1 = isBoundaryVoxel(boundary_data_0, start_x, end_y, offset, bNx, bNxNy);
            /// 両方trueなら調整（片方なら元データのままにする）
            const Point3f& start1 = (start_0 && start_1) ? pointf_nearest_x_upper(org_x, delta_x, start_x, start_y, 0)
                                                         : pointf(start_x, start_y, 0);
            const Point3f& start2 = (start_0 && start_1) ? pointf_nearest_x_upper(org_x, delta_x, start_x, end_y + 1, 0)
                                                         : pointf(start_x, end_y + 1, 0);

            /// 境界上かチェック
            bool end_0 = isBoundaryVoxel(boundary_data_1, end_x - 1, start_y, offset, bNx, bNxNy);
            bool end_1 = isBoundaryVoxel(boundary_data_1, end_x - 1, end_y, offset, bNx, bNxNy);
            /// 両方trueなら調整（片方なら元データのままにする）
            const Point3f& end1 = (end_0 && end_1) ? pointf_nearest_x_lower(org_x, delta_x, end_x, start_y, 0)
                                                   : pointf(end_x, start_y, 0);
            const Point3f& end2 = (end_0 && end_1) ? pointf_nearest_x_lower(org_x, delta_x, end_x, end_y + 1, 0)
                                                   : pointf(end_x, end_y + 1, 0);
            mesh->appendQuad(start1, start2, end2, end1, /*norm,*/ get_seg_flg);
        }
    }
}

float VoxelScalar::value(const Point3f& pos)
{
    if (!m_scalar_data) {
        return -1;
    }
    Point3i ind = index(pos);

    int ix = std::clamp(ind.x(), 0, nX() - 1);
    int iy = std::clamp(ind.y(), 0, nY() - 1);

    if (is2DTexture()) {
        __int64 index = (__int64)iy * (__int64)nX() + (__int64)ix;
        return m_scalar_data[index];
    }
    else {
        int iz = std::clamp(ind.z(), 0, nZ() - 1);

        __int64 index = (__int64)ix * ((__int64)nY() * (__int64)nZ()) + (__int64)iy * (__int64)nZ() + (__int64)iz;
        return m_scalar_data[index];
    }
}

CORE_NAMESPACE_END
