#include "Voxel.h"
#include <execution>
#include "Node.h"

CORE_NAMESPACE_BEGIN

DEFINE_META_OBJECT(Voxel)

static constexpr int g_voxel_comp_length = 2;

Voxel::Voxel()
{
    auto renderable = RenderMesh::createRenderable();
    m_renderable    = renderable.ptr();
}

Voxel::Voxel(const Voxel& other)
{
    m_renderable = other.m_renderable;
}

Voxel::~Voxel()
{
    deleteMemory();
}

RenderMesh* Voxel::renderMesh()
{
    return (RenderMesh*)m_renderable.ptr();
}

void Voxel::setCreateSectionLine(bool create)
{
    renderMesh()->setCreateSectionLine(create);
}

void Voxel::createBoxShape()
{
    auto mesh = renderMesh();

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

Voxel::Voxel(const double x, const double y, const double z, const double dx, const double dy, const double dz,
             const int nx, const int ny, const int nz)
{
    init(x, y, z, dx, dy, dz, nx, ny, nz);
}

Voxel::Voxel(const Point3d& org, const Point3d& delta, const Point3i& number)
{
    init(org, delta, number);
}

void Voxel::init(const double x, const double y, const double z, const double dx, const double dy, const double dz,
                 const int nx, const int ny, const int nz)
{
    m_data = nullptr;
    m_org.set(x, y, z);
    m_delta.set(dx, dy, dz);
    m_number.set(nx, ny, nz);

    initMemory();
}

void Voxel::init(const Point3d& org, const Point3d& delta, const Point3i& number)
{
    m_org    = org;
    m_delta  = delta;
    m_number = number;

    initMemory();
}

void Voxel::initVoxelData()
{
    initMemory();
}

void Voxel::deleteVoxelData()
{
    deleteMemory();
}

void Voxel::unite(Voxel* other)
{
    if (!isEqualCoodinate(other)) {
        int nX = m_number.x();
        int nY = m_number.y();
        int nZ = m_number.z();

        int nX2 = other->m_number.x();
        int nY2 = other->m_number.y();
        int nZ2 = other->m_number.z();

        unsigned char** myData2 = (unsigned char**)other->m_data;

        const auto& org_other   = other->org();
        const auto& delta_other = other->delta();

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

        std::for_each(std::execution::par, z_values.begin(), z_values.end(),
                      [&](std::pair<int, std::vector<int>>& z_value) {
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
                                      if (z_buffer[index >> 3] & g_v_bits[index & 0x07]) {
                                          setCell(jx, jy, jz, true);
                                      }
                                  }
                              }
                          }
                      });
    }
    else {
        unsigned char** myData  = (unsigned char**)m_data;
        unsigned char** myData2 = (unsigned char**)other->m_data;

        int nBytesPerXY = bytesPerXY();

        std::vector<int> iz_vec(m_number.z());
        std::iota(iz_vec.begin(), iz_vec.end(), 0);

        std::for_each(std::execution::par, iz_vec.begin(), iz_vec.end(), [&](int iz) {
            if (!myData2[iz]) {
                return;
            }

            if (!myData[iz]) {
                setFullZeroZ(iz);
            }

            unsigned char* myZData  = myData[iz];
            unsigned char* myZData2 = myData2[iz];

            for (int ii = 0; ii < nBytesPerXY; ++ii) {
                if (myZData[ii] != myZData2[ii]) {
                    myZData[ii] |= myZData2[ii];
                }
            }
        });
    }
}

bool Voxel::isEqualCoodinate(const Voxel* other) const
{
    if (number() != other->number()) {
        return false;
    }

    if (other->index(m_org) != index(other->org())) {
        return false;
    }

    if (other->index(m_org
                     + Point3d(m_delta.x() * (double)m_number.x(), m_delta.y() * (double)m_number.y(),
                               m_delta.z() * (double)m_number.z()))
        != index(other->org()
                 + Point3d(other->delta().x() * (double)other->number().x(),
                           other->delta().y() * (double)other->number().y(),
                           other->delta().z() * (double)other->number().z()))) {
        return false;
    }

    return true;
}

void Voxel::setFullZeroZ(const int& iz)
{
    if (m_data) {
        auto bytes = bytesPerXY();
        if (!m_data[iz]) {
            m_data[iz] = new unsigned char[bytes];
        }
        std::memset(m_data[iz], 0, bytes * sizeof(unsigned char));
    }
}

void Voxel::initMemory()
{
    if (m_number.x() <= 0 || m_number.y() <= 0 || m_number.z() <= 0) return;

    deleteMemory();

    m_data = new unsigned char*[m_number.z()];
    std::memset(m_data, 0, m_number.z() * sizeof(unsigned char*));

    int number_z = m_number.z();
    for (int ic = 0; ic < number_z; ++ic) {
        m_data[ic] = nullptr;
    }
}

void Voxel::deleteMemory()
{
    if (m_data) {
        int number_z = m_number.z();
        for (int ic = 0; ic < number_z; ++ic) {
            delete[] m_data[ic];
        }
        delete[] m_data;
        m_data = nullptr;
    }

    if (m_compress_data) {
        int number_z = m_number.z();
        for (int ic = 0; ic < number_z; ++ic) {
            delete[] m_compress_data[ic];
        }
        delete[] m_data;
        m_data = nullptr;
    }

    if (m_compress_size) {
        delete[] m_compress_size;
        m_compress_size = nullptr;
    }

    unsetReadPerformInfo();    /// 格子数を戻す
}

void Voxel::deleteZMemory()
{
    if (!m_data) {
        return;
    }

    int number_z = m_number.z();
    for (int ic = 0; ic < number_z; ++ic) {
        delete[] m_data[ic];
        m_data[ic] = nullptr;
    }
}

int Voxel::bytesPerXY() const
{
    return (int)(ceil((m_number.x() * m_number.y()) / 8.0));
}

void Voxel::clearDisplayData()
{
    m_renderable->clearDisplayData();
    markBoxDirty();
}

void Voxel::setCell(const int ix, const int iy, const int iz, const bool data)
{
    if (data) {
        if (!m_data[iz]) {
            auto bytes = bytesPerXY();
            m_data[iz] = new unsigned char[bytes];
            std::memset(m_data[iz], 0, bytes * sizeof(unsigned char));
        }

        int index = ix + m_number.x() * iy;
        if (!(m_data[iz][index >> 3] & g_v_bits[index & 0x07])) {
            m_data[iz][index >> 3] |= g_v_bits[index & 0x07];
        }
    }
    else if (m_data[iz]) {
        int index = ix + m_number.x() * iy;
        if (m_data[iz][index >> 3] & g_v_bits[index & 0x07]) {
            m_data[iz][index >> 3] &= g_v_bits[index & 0x07];
        }
    }
}

void Voxel::setCell(const int ix, const int iy, const int iz, MutexWrapper& mutex)
{
    mutex.lock();
    if (!m_data[iz]) {
        auto bytes = bytesPerXY();
        m_data[iz] = new unsigned char[bytes];
        std::memset(m_data[iz], 0, bytes * sizeof(unsigned char));
    }

    int index = ix + m_number.x() * iy;
    if (!(m_data[iz][index >> 3] & g_v_bits[index & 0x07])) {
        m_data[iz][index >> 3] |= g_v_bits[index & 0x07];
    }
    mutex.unlock();
}

void Voxel::setXCells(const int ix_min, const int ix_max, const int iy, const int iz, const bool data)
{
    if (data) {
        auto& zdata = m_data[iz];

        if (!zdata) {
            auto bytes = bytesPerXY();
            zdata      = new unsigned char[bytes];
            std::memset(zdata, 0, bytes * sizeof(unsigned char));
        }

        int start_xbyte = (ix_min + m_number.x() * iy) >> 3;
        int start_xbit  = (ix_min + m_number.x() * iy) & 0x07;
        int end_xbyte   = (ix_max + m_number.x() * iy) >> 3;
        int end_xbit    = (ix_max + m_number.x() * iy) & 0x07;

        /// 最初のビット部分
        if (start_xbyte == end_xbyte) {
            /// start_xbyte と end_xbyte が同じ場合、同じバイト内で処理
            for (int ix = start_xbit; ix <= end_xbit; ++ix) {
                if (!(zdata[start_xbyte] & g_v_bits[ix])) {
                    zdata[start_xbyte] |= g_v_bits[ix];
                }
            }
        }
        else {
            /// start_xbyte と end_xbyte が異なる場合
            /// 最初のバイトのビット部分
            for (int ix = start_xbit; ix < 8; ++ix) {
                if (!(zdata[start_xbyte] & g_v_bits[ix])) {
                    zdata[start_xbyte] |= g_v_bits[ix];
                }
            }
            ++start_xbyte;    /// 次のバイトから処理を開始

            /// 中間のバイト部分
            if (start_xbyte < end_xbyte) {
                std::memset(&zdata[start_xbyte], 0xFF, end_xbyte - start_xbyte);
            }

            /// 最後のバイトのビット部分
            for (int ix = 0; ix <= end_xbit; ++ix) {
                if (!(zdata[end_xbyte] & g_v_bits[ix])) {
                    zdata[end_xbyte] |= g_v_bits[ix];
                }
            }
        }
    }
    else if (m_data[iz]) {
        auto& zdata = m_data[iz];

        int start_xbyte = (ix_min + m_number.x() * iy) >> 3;
        int start_xbit  = (ix_min + m_number.x() * iy) & 0x07;
        int end_xbyte   = (ix_max + m_number.x() * iy) >> 3;
        int end_xbit    = (ix_max + m_number.x() * iy) & 0x07;

        /// 最初のビット部分
        if (start_xbyte == end_xbyte) {
            /// start_xbyte と end_xbyte が同じ場合、同じバイト内で処理
            for (int ix = start_xbit; ix <= end_xbit; ++ix) {
                if (zdata[start_xbyte] & g_v_bits[ix]) {
                    zdata[start_xbyte] &= g_v_bits[ix];
                }
            }
        }
        else {
            /// start_xbyte と end_xbyte が異なる場合
            /// 最初のバイトのビット部分
            for (int ix = start_xbit; ix < 8; ++ix) {
                if (zdata[start_xbyte] & g_v_bits[ix]) {
                    zdata[start_xbyte] &= g_v_bits[ix];
                }
            }
            ++start_xbyte;    /// 次のバイトから処理を開始

            /// 中間のバイト部分
            if (start_xbyte < end_xbyte) {
                std::memset(&zdata[start_xbyte], 0x0, end_xbyte - start_xbyte);
            }

            /// 最後のバイトのビット部分
            for (int ix = 0; ix <= end_xbit; ++ix) {
                if (zdata[end_xbyte] & g_v_bits[ix]) {
                    zdata[end_xbyte] &= g_v_bits[ix];
                }
            }
        }
    }
}

bool Voxel::cell(const int ix, const int iy, const int iz) const
{
    if (!m_data[iz]) return false;

    return m_data[iz][(ix + m_number.x() * iy) >> 3] & g_v_bits[(ix + m_number.x() * iy) & 0x07];
}

void Voxel::invertCell(const int ix, const int iy, const int iz)
{
    if (!m_data[iz]) {
        auto bytes = bytesPerXY();
        m_data[iz] = new unsigned char[bytes];
        std::memset(m_data[iz], 0, bytes * sizeof(unsigned char));
    }

    int index = ix + m_number.x() * iy;
    m_data[iz][index >> 3] ^= g_v_bits[index & 0x07];
}

void Voxel::invertXCells(const int ix_min, const int ix_max, const int iy, const int iz)
{
    auto& zdata = m_data[iz];

    if (!zdata) {
        auto bytes = bytesPerXY();
        zdata      = new unsigned char[bytes];
        std::memset(zdata, 0, bytes * sizeof(unsigned char));
    }

    int start_xbyte = (ix_min + m_number.x() * iy) >> 3;
    int start_xbit  = (ix_min + m_number.x() * iy) & 0x07;
    int end_xbyte   = (ix_max + m_number.x() * iy) >> 3;
    int end_xbit    = (ix_max + m_number.x() * iy) & 0x07;

    /// 最初のビット部分
    if (start_xbyte == end_xbyte) {
        /// start_xbyte と end_xbyte が同じ場合、同じバイト内で処理
        for (int ix = start_xbit; ix <= end_xbit; ++ix) {
            zdata[start_xbyte] ^= g_v_bits[ix];
        }
    }
    else {
        /// start_xbyte と end_xbyte が異なる場合
        /// 最初のバイトのビット部分
        for (int ix = start_xbit; ix < 8; ++ix) {
            zdata[start_xbyte] ^= g_v_bits[ix];
        }
        ++start_xbyte;    /// 次のバイトから処理を開始

        /// 中間のバイト部分
        if (start_xbyte < end_xbyte) {
            for (int ic = start_xbyte; ic < end_xbyte; ++ic) {
                zdata[ic] ^= 0xFF;
            }
        }

        /// 最後のバイトのビット部分
        for (int ix = 0; ix <= end_xbit; ++ix) {
            zdata[end_xbyte] ^= g_v_bits[ix];
        }
    }
}

void Voxel::clearZ(int iz)
{
    if (m_data[iz]) {
        auto bytes = bytesPerXY();
        std::memset(m_data[iz], 0, bytes * sizeof(unsigned char));
    }
}

bool Voxel::hasBuffer() const
{
    if (!m_data) {
        return false;
    }

    int number_z = m_number.z();
    for (int ic = 0; ic < number_z; ++ic) {
        if (m_data[ic]) {
            return true;
        }
    }

    return false;
}

Point3d Voxel::point(const int ix, const int iy, const int iz) const
{
    return Point3d(m_org[0] + m_delta[0] * (double)ix, m_org[1] + m_delta[1] * (double)iy,
                   m_org[2] + m_delta[2] * (double)iz);
}

Point3f Voxel::pointf(const int ix, const int iy, const int iz) const
{
    if (m_read_perform_info) {
        return Point3f(m_read_perform_info->m_new_old_index_x
                           ? (float)m_org[0] + (float)m_delta[0] * (float)(*m_read_perform_info->m_new_old_index_x)[ix]
                           : (float)m_org[0] + (float)m_delta[0] * (float)ix,
                       m_read_perform_info->m_new_old_index_y
                           ? (float)m_org[1] + (float)m_delta[1] * (float)(*m_read_perform_info->m_new_old_index_y)[iy]
                           : (float)m_org[1] + (float)m_delta[1] * (float)iy,
                       m_read_perform_info->m_new_old_index_z
                           ? (float)m_org[2] + (float)m_delta[2] * (float)(*m_read_perform_info->m_new_old_index_z)[iz]
                           : (float)m_org[2] + (float)m_delta[2] * (float)iz);
    }
    else {
        return Point3f((float)m_org[0] + (float)m_delta[0] * (float)ix, (float)m_org[1] + (float)m_delta[1] * (float)iy,
                       (float)m_org[2] + (float)m_delta[2] * (float)iz);
    }
}

Point3i Voxel::index(const float x, const float y, const float z) const
{
    return Point3i((x - (float)m_org[0]) / (float)m_delta[0], (y - (float)m_org[1]) / (float)m_delta[1],
                   (z - (float)m_org[2]) / (float)m_delta[2]);
}

Point3i Voxel::index(const Point3d& pos) const
{
    return Point3i(((float)pos.x() - (float)m_org[0]) / (float)m_delta[0],
                   ((float)pos.y() - (float)m_org[1]) / (float)m_delta[1],
                   ((float)pos.z() - (float)m_org[2]) / (float)m_delta[2]);
}

Point3i Voxel::index(const Point3f& pos) const
{
    return Point3i((pos.x() - (float)m_org[0]) / (float)m_delta[0], (pos.y() - (float)m_org[1]) / (float)m_delta[1],
                   (pos.z() - (float)m_org[2]) / (float)m_delta[2]);
}

bool Voxel::cell(const Point3f& pos) const
{
    int iz = ((float)pos.z() - (float)m_org[2]) / (float)m_delta[2];
    if (iz < 0) return false;
    if (iz >= m_number.z()) return false;

    if (!m_data[iz]) return false;

    int ix = ((float)pos.x() - (float)m_org[0]) / (float)m_delta[0];
    if (ix < 0) return false;
    if (ix >= m_number.x()) return false;

    int iy = ((float)pos.y() - (float)m_org[1]) / (float)m_delta[1];
    if (iy < 0) return false;
    if (iy >= m_number.y()) return false;

    return m_data[iz][(ix + m_number.x() * iy) >> 3] & g_v_bits[(ix + m_number.x() * iy) & 0x07];
}

void Voxel::createDisplayData()
{
    int nz = m_number.z();

    /// Z範囲（※Zに積みあがるので、たいていの場合範囲が絞られる）
    int min_z = -1;
    for (int z = 0; z < nz; ++z) {
        if (!m_data[z]) {
            continue;
        }
        min_z = z;
        break;
    }
    if (min_z < 0) {
        return;
    }
    int max_z = -1;
    for (int z = nz - 1; z >= min_z; --z) {
        if (!m_data[z]) {
            continue;
        }
        max_z = z;
        break;
    }

    Voxel m_work[2];
    m_work[0].init(m_org, m_delta, m_number);
    m_work[1].init(m_org, m_delta, m_number);

    int nx = m_number.x();
    int ny = m_number.y();

    std::vector<int> x_range(nx);
    std::iota(x_range.begin(), x_range.end(), 0);

    std::vector<int> y_range(ny);
    std::iota(y_range.begin(), y_range.end(), 0);

    std::vector<int> z_range(max_z - min_z + 1);
    std::iota(z_range.begin(), z_range.end(), min_z);

    /// 大量のループなので逆にcriticalsectionのせいで遅い＆かつメモリが解放されない(競合起こしていたので直せば大丈夫かもしれないが）のでやめる
    // MutexWrapper mutex;

    auto mesh = renderMesh();

    std::map<std::pair<int, int>, std::vector<int>> ranges_value;

    /// 線分の取得フラグ
    int get_seg_flg = 2;

    /// 表面取り出し
    // for (int y = 0; y < ny; ++y) {
    std::for_each(std::execution::seq, y_range.begin(), y_range.end(), [&](int y) {
        // for (int x = 0; x < nx; ++x) {
        std::for_each(std::execution::seq, x_range.begin(), x_range.end(), [&](int x) {
            bool inCell = false;
            int  ixiy   = x + nx * y;
            for (int z = min_z; z <= max_z; ++z) {
                if (!inCell) {
                    if (cell(ixiy, z)) {
                        /// -Z方向の面
                        inCell = true;
                        m_work[0].setCell(x, y, z, true);
                        if (z == max_z) {
                            /// +Z方向の面
                            m_work[1].setCell(x, y, z, true);
                        }
                    }
                }
                else {
                    if (!cell(ixiy, z)) {
                        /// +Z方向の面
                        inCell = false;
                        m_work[1].setCell(x, y, z - 1, true);
                    }
                    else if (z == max_z) {
                        /// +Z方向の面
                        m_work[1].setCell(x, y, z, true);
                    }
                }
            }
        });
    });

    Point3f norm(0, 0, -1);

    for (int z = min_z; z <= max_z; ++z) {
        if (!m_work[0].m_data[z]) {
            continue;
        }
        auto z_buffer = m_work[0].m_data[z];

        int start_x;
        ranges_value.clear();

        for (int y = 0; y < ny; ++y) {
            bool inCell  = false;
            int  index_y = nx * y;
            for (int x = 0; x < nx; ++x) {
                int index = x + index_y;
                if (!inCell) {
                    if (z_buffer[index >> 3] & g_v_bits[index & 0x07]) {
                        inCell = true;
                        /// 始点
                        start_x = x;
                        if (x == nx - 1) {
                            /// 終点
                            ranges_value[std::pair<int, int>(start_x, x + 1)].emplace_back(y);
                        }
                    }
                }
                else {
                    if (!(z_buffer[index >> 3] & g_v_bits[index & 0x07])) {
                        inCell = false;
                        /// 終点
                        ranges_value[std::pair<int, int>(start_x, x)].emplace_back(y);
                    }
                    else if (x == nx - 1) {
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
                        /// マージできない場合、現在の四角形を保存し、新しい四角形を開始
                        const Point3f& start1 = pointf(start_x, start_y, z);
                        const Point3f& start2 = pointf(start_x, end_y + 1, z);
                        const Point3f& end1   = pointf(end_x, start_y, z);
                        const Point3f& end2   = pointf(end_x, end_y + 1, z);
                        mesh->appendQuad(start1, start2, end2, end1, /*norm,*/ get_seg_flg);
                        start_y = y_values[ic];
                        end_y   = start_y;
                    }
                }

                /// 最後の四角形を保存
                const Point3f& start1 = pointf(start_x, start_y, z);
                const Point3f& start2 = pointf(start_x, end_y + 1, z);
                const Point3f& end1   = pointf(end_x, start_y, z);
                const Point3f& end2   = pointf(end_x, end_y + 1, z);
                mesh->appendQuad(start1, start2, end2, end1, /*norm,*/ get_seg_flg);
            }
        }
    }

    get_seg_flg = 1;
    norm.set(0, 0, 1);
    for (int z = min_z; z <= max_z; ++z) {
        if (!m_work[1].m_data[z]) {
            continue;
        }
        auto z_buffer = m_work[1].m_data[z];

        int start_x;
        ranges_value.clear();

        for (int y = 0; y < ny; ++y) {
            bool inCell  = false;
            int  index_y = nx * y;
            for (int x = 0; x < nx; ++x) {
                int index = x + index_y;
                if (!inCell) {
                    if (z_buffer[index >> 3] & g_v_bits[index & 0x07]) {
                        inCell = true;
                        /// 始点
                        start_x = x;
                        if (x == nx - 1) {
                            /// 終点
                            ranges_value[std::pair<int, int>(start_x, x + 1)].emplace_back(y);
                        }
                    }
                }
                else {
                    if (!(z_buffer[index >> 3] & g_v_bits[index & 0x07])) {
                        inCell = false;
                        /// 終点
                        ranges_value[std::pair<int, int>(start_x, x)].emplace_back(y);
                    }
                    else if (x == nx - 1) {
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
                        /// マージできない場合、現在の四角形を保存し、新しい四角形を開始
                        const Point3f& start1 = pointf(start_x, start_y, z + 1);
                        const Point3f& start2 = pointf(start_x, end_y + 1, z + 1);
                        const Point3f& end1   = pointf(end_x, start_y, z + 1);
                        const Point3f& end2   = pointf(end_x, end_y + 1, z + 1);
                        mesh->appendQuad(start1, end1, end2, start2, /*norm,*/ get_seg_flg);
                        start_y = y_values[ic];
                        end_y   = start_y;
                    }
                }

                /// 最後の四角形を保存
                const Point3f& start1 = pointf(start_x, start_y, z + 1);
                const Point3f& start2 = pointf(start_x, end_y + 1, z + 1);
                const Point3f& end1   = pointf(end_x, start_y, z + 1);
                const Point3f& end2   = pointf(end_x, end_y + 1, z + 1);
                mesh->appendQuad(start1, end1, end2, start2, /*norm,*/ get_seg_flg);
            }
        }
    }

    m_work[0].deleteMemory();
    m_work[1].deleteMemory();
    m_work[0].init({m_org.y(), m_org.z(), m_org.x()}, {m_delta.y(), m_delta.z(), m_delta.x()}, {ny, nz, nx});
    m_work[1].init({m_org.y(), m_org.z(), m_org.x()}, {m_delta.y(), m_delta.z(), m_delta.x()}, {ny, nz, nx});

    // for (int z = min_z; z <= max_z; ++z) {
    std::for_each(std::execution::seq, z_range.begin(), z_range.end(), [&](int z) {
        if (!m_data[z]) {
            // continue;
            return;    /// 並列ループの関数の終わり。continueと同じ
        }
        auto z_buffer = m_data[z];
        for (int y = 0; y < ny; ++y) {
            bool inCell  = false;
            int  index_y = nx * y;
            for (int x = 0; x < nx; ++x) {
                int index = x + index_y;
                if (!inCell) {
                    if (z_buffer[index >> 3] & g_v_bits[index & 0x07]) {
                        /// -X方向の面
                        inCell = true;
                        m_work[0].setCell(y, z, x, true);    /// Xをバッファで扱うため
                        if (x == nx - 1) {
                            /// +X方向の面
                            m_work[1].setCell(y, z, x, true);
                        }
                    }
                }
                else {
                    if (!(z_buffer[index >> 3] & g_v_bits[index & 0x07])) {
                        /// +X方向の面
                        inCell = false;
                        m_work[1].setCell(y, z, x - 1, true);
                    }
                    else if (x == nx - 1) {
                        /// +X方向の面
                        m_work[1].setCell(y, z, x, true);
                    }
                }
            }
        }
    });

    get_seg_flg = 2;
    norm.set(-1, 0, 0);
    for (int x = 0; x < nx; ++x) {
        if (!m_work[0].m_data[x]) {
            continue;
        }

        int start_y;
        ranges_value.clear();

        for (int z = min_z; z <= max_z; ++z) {
            auto x_buffer = m_work[0].m_data[x];

            bool inCell = false;
            for (int y = 0; y < ny; ++y) {
                int index = y + ny * z;
                if (!inCell) {
                    if (x_buffer[index >> 3] & g_v_bits[index & 0x07]) {
                        inCell = true;
                        /// 始点
                        start_y = y;
                        if (y == ny - 1) {
                            /// 終点
                            ranges_value[std::pair<int, int>(start_y, y + 1)].emplace_back(z);
                        }
                    }
                }
                else {
                    if (!(x_buffer[index >> 3] & g_v_bits[index & 0x07])) {
                        inCell = false;
                        /// 終点
                        ranges_value[std::pair<int, int>(start_y, y)].emplace_back(z);
                    }
                    else if (y == ny - 1) {
                        /// 終点
                        ranges_value[std::pair<int, int>(start_y, y + 1)].emplace_back(z);
                    }
                }
            }
        }

        for (const auto& range : ranges_value) {
            auto [start_y, end_y] = range.first;
            const auto& z_values  = range.second;

            if (!z_values.empty()) {
                int start_z = z_values[0];
                int end_z   = start_z;

                for (int ic = 1; ic < (int)z_values.size(); ++ic) {
                    if (z_values[ic] == end_z + 1) {
                        /// Y方向が連続している場合に拡張
                        end_z = z_values[ic];
                    }
                    else {
                        /// マージできない場合、現在の四角形を保存し、新しい四角形を開始
                        const Point3f& start1 = pointf(x, start_y, start_z);
                        const Point3f& start2 = pointf(x, start_y, end_z + 1);
                        const Point3f& end1   = pointf(x, end_y, start_z);
                        const Point3f& end2   = pointf(x, end_y, end_z + 1);
                        mesh->appendQuad(start1, start2, end2, end1, /*norm,*/ get_seg_flg);
                        start_z = z_values[ic];
                        end_z   = start_z;
                    }
                }

                /// 最後の四角形を保存
                const Point3f& start1 = pointf(x, start_y, start_z);
                const Point3f& start2 = pointf(x, start_y, end_z + 1);
                const Point3f& end1   = pointf(x, end_y, start_z);
                const Point3f& end2   = pointf(x, end_y, end_z + 1);
                mesh->appendQuad(start1, start2, end2, end1, /*norm,*/ get_seg_flg);
            }
        }
    }

    get_seg_flg = 1;
    norm.set(1, 0, 0);
    for (int x = 0; x < nx; ++x) {
        if (!m_work[1].m_data[x]) {
            continue;
        }
        auto x_buffer = m_work[1].m_data[x];

        int start_y;
        ranges_value.clear();

        for (int z = min_z; z <= max_z; ++z) {
            bool inCell = false;
            for (int y = 0; y < ny; ++y) {
                int index = y + ny * z;
                if (!inCell) {
                    if (x_buffer[index >> 3] & g_v_bits[index & 0x07]) {
                        inCell = true;
                        /// 始点
                        start_y = y;
                        if (y == ny - 1) {
                            /// 終点
                            ranges_value[std::pair<int, int>(start_y, y + 1)].emplace_back(z);
                        }
                    }
                }
                else {
                    if (!(x_buffer[index >> 3] & g_v_bits[index & 0x07])) {
                        inCell = false;
                        /// 終点
                        ranges_value[std::pair<int, int>(start_y, y)].emplace_back(z);
                    }
                    else if (y == ny - 1) {
                        /// 終点
                        ranges_value[std::pair<int, int>(start_y, y + 1)].emplace_back(z);
                    }
                }
            }
        }

        for (const auto& range : ranges_value) {
            auto [start_y, end_y] = range.first;
            const auto& z_values  = range.second;

            if (!z_values.empty()) {
                int start_z = z_values[0];
                int end_z   = start_z;

                for (int ic = 1; ic < (int)z_values.size(); ++ic) {
                    if (z_values[ic] == end_z + 1) {
                        /// Y方向が連続している場合に拡張
                        end_z = z_values[ic];
                    }
                    else {
                        /// マージできない場合、現在の四角形を保存し、新しい四角形を開始
                        const Point3f& start1 = pointf(x + 1, start_y, start_z);
                        const Point3f& start2 = pointf(x + 1, start_y, end_z + 1);
                        const Point3f& end1   = pointf(x + 1, end_y, start_z);
                        const Point3f& end2   = pointf(x + 1, end_y, end_z + 1);
                        mesh->appendQuad(start1, end1, end2, start2, /*norm,*/ get_seg_flg);
                        start_z = z_values[ic];
                        end_z   = start_z;
                    }
                }

                /// 最後の四角形を保存
                const Point3f& start1 = pointf(x + 1, start_y, start_z);
                const Point3f& start2 = pointf(x + 1, start_y, end_z + 1);
                const Point3f& end1   = pointf(x + 1, end_y, start_z);
                const Point3f& end2   = pointf(x + 1, end_y, end_z + 1);
                mesh->appendQuad(start1, end1, end2, start2, /*norm,*/ get_seg_flg);
            }
        }
    }

    m_work[0].deleteMemory();
    m_work[1].deleteMemory();
    m_work[0].init({m_org.x(), m_org.z(), m_org.y()}, {m_delta.x(), m_delta.z(), m_delta.y()}, {nx, nz, ny});
    m_work[1].init({m_org.x(), m_org.z(), m_org.y()}, {m_delta.x(), m_delta.z(), m_delta.y()}, {nx, nz, ny});

    // for (int z = min_z; z <= max_z; ++z) {
    std::for_each(std::execution::seq, z_range.begin(), z_range.end(), [&](int z) {
        if (!m_data[z]) {
            // continue;
            return;
        }
        auto z_buffer = m_data[z];
        for (int x = 0; x < nx; ++x) {
            bool inCell = false;
            for (int y = 0; y < ny; ++y) {
                int index = x + nx * y;
                if (!inCell) {
                    if (z_buffer[index >> 3] & g_v_bits[index & 0x07]) {
                        /// -Y方向の面
                        inCell = true;
                        m_work[0].setCell(x, z, y, true);
                        if (y == ny - 1) {
                            /// +Y方向の面
                            m_work[1].setCell(x, z, y, true);
                        }
                    }
                }
                else {
                    if (!(z_buffer[index >> 3] & g_v_bits[index & 0x07])) {
                        /// +Y方向の面
                        inCell = false;
                        m_work[1].setCell(x, z, y - 1, true);
                    }
                    else if (y == ny - 1) {
                        /// +Y方向の面
                        m_work[1].setCell(x, z, y, true);
                    }
                }
            }
        }
    });

    get_seg_flg = 2;
    norm.set(0, -1, 0);
    for (int y = 0; y < ny; ++y) {
        if (!m_work[0].m_data[y]) {
            continue;
        }
        auto y_buffer = m_work[0].m_data[y];

        int start_z;
        ranges_value.clear();

        for (int x = 0; x < nx; ++x) {
            bool inCell  = false;
            int  index_x = x;
            for (int z = min_z; z <= max_z; ++z) {
                int index = index_x + nx * z;
                if (!inCell) {
                    if (y_buffer[index >> 3] & g_v_bits[index & 0x07]) {
                        inCell = true;
                        /// 始点
                        start_z = z;
                        if (z == max_z) {
                            /// 終点
                            ranges_value[std::pair(start_z, z + 1)].emplace_back(x);
                        }
                    }
                }
                else {
                    if (!(y_buffer[index >> 3] & g_v_bits[index & 0x07])) {
                        inCell = false;
                        /// 終点
                        ranges_value[std::pair(start_z, z)].emplace_back(x);
                    }
                    else if (z == max_z) {
                        /// 終点
                        ranges_value[std::pair(start_z, z + 1)].emplace_back(x);
                    }
                }
            }
        }

        for (const auto& range : ranges_value) {
            auto [start_z, end_z] = range.first;
            const auto& x_values  = range.second;

            if (!x_values.empty()) {
                int start_x = x_values[0];
                int end_x   = start_x;

                for (int ic = 1; ic < (int)x_values.size(); ++ic) {
                    if (x_values[ic] == end_x + 1) {
                        end_x = x_values[ic];
                    }
                    else {
                        // 四角形を保存
                        const Point3f& start1 = pointf(start_x, y, start_z);
                        const Point3f& start2 = pointf(end_x + 1, y, start_z);
                        const Point3f& end1   = pointf(start_x, y, end_z);
                        const Point3f& end2   = pointf(end_x + 1, y, end_z);
                        mesh->appendQuad(start1, start2, end2, end1, /*norm,*/ get_seg_flg);
                        start_x = x_values[ic];
                        end_x   = start_x;
                    }
                }
                // 最後の四角形を保存
                const Point3f& start1 = pointf(start_x, y, start_z);
                const Point3f& start2 = pointf(end_x + 1, y, start_z);
                const Point3f& end1   = pointf(start_x, y, end_z);
                const Point3f& end2   = pointf(end_x + 1, y, end_z);
                mesh->appendQuad(start1, start2, end2, end1, /*norm,*/ get_seg_flg);
            }
        }
    }

    get_seg_flg = 1;
    norm.set(0, 1, 0);
    for (int y = 0; y < ny; ++y) {
        if (!m_work[1].m_data[y]) {
            continue;
        }
        auto y_buffer = m_work[1].m_data[y];

        int start_z;
        ranges_value.clear();

        for (int x = 0; x < nx; ++x) {
            bool inCell  = false;
            int  index_x = x;
            for (int z = min_z; z <= max_z; ++z) {
                int index = index_x + nx * z;
                if (!inCell) {
                    if (y_buffer[index >> 3] & g_v_bits[index & 0x07]) {
                        inCell = true;
                        /// 始点
                        start_z = z;
                        if (z == max_z) {
                            /// 終点
                            ranges_value[std::pair<int, int>(start_z, z + 1)].emplace_back(x);
                        }
                    }
                }
                else {
                    if (!(y_buffer[index >> 3] & g_v_bits[index & 0x07])) {
                        inCell = false;
                        /// 終点
                        ranges_value[std::pair<int, int>(start_z, z)].emplace_back(x);
                    }
                    else if (z == max_z) {
                        /// 終点
                        ranges_value[std::pair<int, int>(start_z, z + 1)].emplace_back(x);
                    }
                }
            }
        }

        for (const auto& range : ranges_value) {
            auto [start_z, end_z] = range.first;
            const auto& x_values  = range.second;

            if (!x_values.empty()) {
                int start_x = x_values[0];
                int end_x   = start_x;

                for (int ic = 1; ic < (int)x_values.size(); ++ic) {
                    if (x_values[ic] == end_x + 1) {
                        end_x = x_values[ic];
                    }
                    else {
                        // 四角形を保存
                        const Point3f& start1 = pointf(start_x, y + 1, start_z);
                        const Point3f& start2 = pointf(end_x + 1, y + 1, start_z);
                        const Point3f& end1   = pointf(start_x, y + 1, end_z);
                        const Point3f& end2   = pointf(end_x + 1, y + 1, end_z);
                        mesh->appendQuad(start1, end1, end2, start2, /*norm,*/ get_seg_flg);
                        start_x = x_values[ic];
                        end_x   = start_x;
                    }
                }
                // 最後の四角形を保存
                const Point3f& start1 = pointf(start_x, y + 1, start_z);
                const Point3f& start2 = pointf(end_x + 1, y + 1, start_z);
                const Point3f& end1   = pointf(start_x, y + 1, end_z);
                const Point3f& end2   = pointf(end_x + 1, y + 1, end_z);
                mesh->appendQuad(start1, end1, end2, start2, /*norm,*/ get_seg_flg);
            }
        }
    }

    markBoxDirty();
}

void Voxel::createDisplayDataXYOnly()
{
    if (!m_data[0]) {
        return;
    }

    auto mesh = renderMesh();

    int nx = m_number.x();
    int ny = m_number.y();

    std::vector<int> x_range(nx);
    std::iota(x_range.begin(), x_range.end(), 0);

    std::vector<int> y_range(ny);
    std::iota(y_range.begin(), y_range.end(), 0);

    std::map<std::pair<int, int>, std::vector<int>> ranges_value;

    /// 線分の取得フラグ
    int get_seg_flg = 2;

    auto z_buffer = m_data[0];

    int start_x;
    ranges_value.clear();

    for (int y = 0; y < ny; ++y) {
        bool inCell  = false;
        int  index_y = nx * y;
        for (int x = 0; x < nx; ++x) {
            int index = x + index_y;
            if (!inCell) {
                if (z_buffer[index >> 3] & g_v_bits[index & 0x07]) {
                    inCell = true;
                    /// 始点
                    start_x = x;
                    if (x == nx - 1) {
                        /// 終点
                        ranges_value[std::pair<int, int>(start_x, x + 1)].emplace_back(y);
                    }
                }
            }
            else {
                if (!(z_buffer[index >> 3] & g_v_bits[index & 0x07])) {
                    inCell = false;
                    /// 終点
                    ranges_value[std::pair<int, int>(start_x, x)].emplace_back(y);
                }
                else if (x == nx - 1) {
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
                    /// マージできない場合、現在の四角形を保存し、新しい四角形を開始
                    const Point3f& start1 = pointf(start_x, start_y, 0);
                    const Point3f& start2 = pointf(start_x, end_y + 1, 0);
                    const Point3f& end1   = pointf(end_x, start_y, 0);
                    const Point3f& end2   = pointf(end_x, end_y + 1, 0);
                    mesh->appendQuad(start1, start2, end2, end1, /*norm,*/ get_seg_flg);
                    start_y = y_values[ic];
                    end_y   = start_y;
                }
            }

            /// 最後の四角形を保存
            const Point3f& start1 = pointf(start_x, start_y, 0);
            const Point3f& start2 = pointf(start_x, end_y + 1, 0);
            const Point3f& end1   = pointf(end_x, start_y, 0);
            const Point3f& end2   = pointf(end_x, end_y + 1, 0);
            mesh->appendQuad(start1, start2, end2, end1, /*norm,*/ get_seg_flg);
        }
    }
}

Node* Voxel::projectionOpt()
{
    /// 暫定 - 表示フラグはGUIと連動させる
    if (m_projection_voxel_scalar.isAlive() && m_projection_voxel_scalar->isShow()) {
        return m_projection_voxel_scalar.ptr();
    }
    else {
        return (Node*)nullptr;
    }
}

void Voxel::compressData()
{
    if (!m_data) {
        return;
    }
    if (m_compress_data) {
        int nb_z = m_number.z();
        for (int iz = 0; iz < nb_z; ++iz) {
            if (m_compress_size[iz] == 0) {
                continue;
            }
            else if (m_compress_size[iz] == -1) {
                continue;
            }
            delete[] m_data[iz];
        }
        delete[] m_data;
        m_data = nullptr;
        return;
    }

    int nb_z         = m_number.z();
    int bytes_per_xy = bytesPerXY();
    if (bytes_per_xy == 0) return;

    __int64 all_org_size = 0, all_size = 0;
    for (int iz = 0; iz < nb_z; ++iz) {
        if (!m_data[iz]) continue;
        all_org_size += bytes_per_xy;
    }

    m_compress_data     = new unsigned char*[nb_z]();
    m_compress_size     = new int[nb_z]();
    unsigned char* work = new unsigned char[bytes_per_xy + 12]();

    /// 圧縮
    for (int iz = 0; iz < nb_z; ++iz) {
        if (!m_data[iz]) {
            m_compress_data[iz] = nullptr;
            m_compress_size[iz] = 0;
            continue;
        }

        unsigned char* my_z_data = m_data[iz];
        unsigned char  pre       = my_z_data[0];
        int            new_size  = 0;
        work[new_size]           = pre;
        int count                = 1;

        int ii;
        for (ii = 1; ii < bytes_per_xy; ++ii) {
            if (my_z_data[ii] != pre) {
                if (count >= g_voxel_comp_length) {
                    if (count >= USHRT_MAX) {
                        work[++new_size] = UCHAR_MAX;
                        work[++new_size] = UCHAR_MAX;
                        work[++new_size] = UCHAR_MAX;
                        ++new_size;
                        std::memcpy(&work[new_size], &count, sizeof(int));
                        new_size += sizeof(int) - 1;
                    }
                    else if (count >= UCHAR_MAX) {
                        work[++new_size] = UCHAR_MAX;
                        ++new_size;
                        unsigned short count_short = static_cast<unsigned short>(count);
                        std::memcpy(&work[new_size], &count_short, sizeof(unsigned short));
                        new_size += sizeof(unsigned short) - 1;
                    }
                    else {
                        work[++new_size] = static_cast<unsigned char>(count);
                    }
                }
                work[++new_size] = my_z_data[ii];
                if (new_size >= bytes_per_xy) {
                    goto NEXT;
                }
                count = 1;
                pre   = my_z_data[ii];
            }
            else {
                if (count < g_voxel_comp_length) {
                    work[++new_size] = my_z_data[ii];
                    if (new_size >= bytes_per_xy) {
                        goto NEXT;
                    }
                }
                ++count;
            }
        }

        if (count >= g_voxel_comp_length) {
            if (count >= USHRT_MAX) {
                work[++new_size] = UCHAR_MAX;
                work[++new_size] = UCHAR_MAX;
                work[++new_size] = UCHAR_MAX;
                ++new_size;
                std::memcpy(&work[new_size], &count, sizeof(int));
                new_size += sizeof(int) - 1;
            }
            else if (count >= UCHAR_MAX) {
                work[++new_size] = UCHAR_MAX;
                ++new_size;
                unsigned short count_short = static_cast<unsigned short>(count);
                std::memcpy(&work[new_size], &count_short, sizeof(unsigned short));
                new_size += sizeof(unsigned short) - 1;
            }
            else {
                work[++new_size] = static_cast<unsigned char>(count);
            }
        }
        ++new_size;

        if (new_size >= bytes_per_xy) {
            goto NEXT;
        }

        all_size += (__int64)(new_size);
        if (all_size >= all_org_size) {
            delete[] work;
            /// 残りを一旦設定
            for (iz; iz < nb_z; ++iz) {
                if (m_data[iz]) {
                    m_compress_data[iz] = m_data[iz];
                    m_compress_size[iz] = -1;
                }
                else {
                    m_compress_data[iz] = nullptr;
                    m_compress_size[iz] = 0;
                }
                m_data[iz] = nullptr;
            }
            delete[] m_data;
            m_data = nullptr;

            /// 圧縮を戻す
            decompressData();
            return;
        }

        m_compress_size[iz] = new_size;
        m_compress_data[iz] = new unsigned char[new_size];
        std::memcpy(m_compress_data[iz], work, new_size);

        delete[] m_data[iz];
        m_data[iz] = nullptr;
        continue;

    NEXT:
        all_size += (__int64)(bytes_per_xy);
        if (all_size >= all_org_size) {
            delete[] work;
            /// 残りを一旦設定
            for (iz; iz < nb_z; ++iz) {
                if (m_data[iz]) {
                    m_compress_data[iz] = m_data[iz];
                    m_compress_size[iz] = -1;
                }
                else {
                    m_compress_data[iz] = nullptr;
                    m_compress_size[iz] = 0;
                }
                m_data[iz] = nullptr;
            }
            delete[] m_data;
            m_data = nullptr;

            /// 圧縮を戻す
            decompressData();
            return;
        }

        /// 元データ
        m_compress_size[iz] = -1;
        m_compress_data[iz] = m_data[iz];
        m_data[iz]          = nullptr;
    }
    delete[] work;
    delete[] m_data;
    m_data = nullptr;
}

void Voxel::decompressData(bool delete_decompress_data)
{
    if (!m_compress_data) {
        return;
    }
    if (m_data) {
        if (delete_decompress_data) {
            int nb_z = m_number.z();

            for (int iz = 0; iz < nb_z; ++iz) {
                if (m_compress_size[iz] > 0) {
                    delete[] m_compress_data[iz];
                }
            }
            delete[] m_compress_data;
            delete[] m_compress_size;
            m_compress_data = nullptr;
            m_compress_size = nullptr;
        }
        return;
    }

    int nb_z = m_number.z();

    unsigned char* my_z_comp_data;

    m_data = new unsigned char*[nb_z]();
    unsigned char* my_z_data;

    int bytes_per_xy = bytesPerXY();

    int           size, new_size, count, count2;
    unsigned char pre;

    int iz, ii;
    for (iz = 0; iz < nb_z; ++iz) {
        if (m_compress_size[iz] == 0) {
            m_data[iz] = nullptr;
            continue;
        }
        else if (m_compress_size[iz] == -1) {
            m_data[iz] = m_compress_data[iz];
            if (delete_decompress_data) {
                m_compress_data[iz] = nullptr;
            }
            continue;
        }

        my_z_comp_data = m_compress_data[iz];

        m_data[iz] = new unsigned char[bytes_per_xy];
        my_z_data  = m_data[iz];

        pre = my_z_comp_data[0];

        count               = 1;
        new_size            = 0;
        my_z_data[new_size] = pre;

        size = m_compress_size[iz];
        for (ii = 1; ii < size; ++ii) {
            if (my_z_comp_data[ii] == pre) {
                ++count;

                if (count == g_voxel_comp_length) {
                    ++ii;

                    count2 = my_z_comp_data[ii];
                    ++ii;

                    if (count2 == UCHAR_MAX) {
                        count2 = *(unsigned short*)&my_z_comp_data[ii];
                        ii += 2;

                        if (count2 == USHRT_MAX) {
                            count2 = *(unsigned int*)&my_z_comp_data[ii];
                            ii += 4;
                        }
                    }

                    count2 -= (g_voxel_comp_length - 1);
                    if (count2 > 0) {
                        memset(&my_z_data[new_size + 1], pre, sizeof(unsigned char) * count2);
                        new_size += count2;
                    }

                    if (ii < size) {
                        count                 = 1;
                        my_z_data[++new_size] = my_z_comp_data[ii];
                        pre                   = my_z_comp_data[ii];
                    }
                }
                else {
                    my_z_data[++new_size] = my_z_comp_data[ii];
                }
            }
            else {
                count                 = 1;
                my_z_data[++new_size] = my_z_comp_data[ii];
                pre                   = my_z_comp_data[ii];
            }
        }

        if (delete_decompress_data) {
            delete[] m_compress_data[iz];
            m_compress_data[iz] = nullptr;
        }
    }

    if (delete_decompress_data) {
        delete[] m_compress_data;
        delete[] m_compress_size;
        m_compress_data = nullptr;
        m_compress_size = nullptr;
    }
}

Voxel::ReadPerformInfo* Voxel::createReadPerformInfo()
{
    m_read_perform_info = new ReadPerformInfo;
    return m_read_perform_info;
}

void Voxel::unsetReadPerformInfo()
{
    if (m_read_perform_info) {
        m_number = m_read_perform_info->m_number;
        delete m_read_perform_info;
        m_read_perform_info = nullptr;
    }
}

Voxel::ReadPerformInfo::~ReadPerformInfo()
{
    delete m_new_old_index_x;
    delete m_new_old_index_y;
    delete m_new_old_index_z;
}

CORE_NAMESPACE_END
