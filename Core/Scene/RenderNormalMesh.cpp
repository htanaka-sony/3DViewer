#include "RenderNormalMesh.h"

#include <algorithm>
#include <cmath>

#undef min
#undef max

CORE_NAMESPACE_BEGIN

DEFINE_META_RENDERABLE(RenderNormalMesh)

RenderNormalMesh::RenderNormalMesh() {}

RenderNormalMesh::~RenderNormalMesh() {}

RenderNormalMesh::RenderNormalMesh(const RenderNormalMesh& other)
{
    m_vertices            = other.m_vertices;
    m_indices             = other.m_indices;
    m_segments_indices    = other.m_segments_indices;
    m_create_section_line = other.m_create_section_line;
}

void RenderNormalMesh::createBoxRound(const BoundingBox3f& box, float radius, float ratioX, float ratioY, float ratioZ,
                                      float tol)
{
    m_vertices.clear();
    m_indices.clear();
    m_segments_indices.clear();

    /// geom_sphereOctant_type1 と同じロジック: ratioMax で正規化した各軸の丸め半径
    const float ratioMax = std::max({ratioX, ratioY, ratioZ});
    float       rx       = radius * ratioX / ratioMax;
    float       ry       = radius * ratioY / ratioMax;
    float       rz       = radius * ratioZ / ratioMax;

    const Point3f mn = box.min_pos();
    const Point3f mx = box.max_pos();
    const float   W  = mx.x() - mn.x();
    const float   H  = mx.y() - mn.y();
    const float   D  = mx.z() - mn.z();

    /// 丸め半径をボックス半分でクランプ
    rx             = std::min(rx, W * 0.5f);
    ry             = std::min(ry, H * 0.5f);
    rz             = std::min(rz, D * 0.5f);
    const float x0 = mn.x(), y0 = mn.y(), z0 = mn.z();

    /// 各頂点に個別の法線を設定してquadを追加
    /// Triangle1: p0,p1,p2  Triangle2: p2,p3,p0
    auto addQuadN = [&](const Point3f& p0, const Point3f& n0, const Point3f& p1, const Point3f& n1, const Point3f& p2,
                        const Point3f& n2, const Point3f& p3, const Point3f& n3) {
        const unsigned int base = (unsigned int)m_vertices.size();
        m_vertices.emplace_back(p0, n0);
        m_vertices.emplace_back(p1, n1);
        m_vertices.emplace_back(p2, n2);
        m_vertices.emplace_back(p3, n3);
        m_indices.emplace_back(base + 0);
        m_indices.emplace_back(base + 1);
        m_indices.emplace_back(base + 2);
        m_indices.emplace_back(base + 2);
        m_indices.emplace_back(base + 3);
        m_indices.emplace_back(base + 0);

        /*
        if (m_create_section_line) {
            m_segments_indices.emplace_back(base + 0);
            m_segments_indices.emplace_back(base + 1);
            m_segments_indices.emplace_back(base + 1);
            m_segments_indices.emplace_back(base + 2);
            m_segments_indices.emplace_back(base + 2);
            m_segments_indices.emplace_back(base + 3);
            m_segments_indices.emplace_back(base + 3);
            m_segments_indices.emplace_back(base + 0);
        }*/
    };

    /// 全頂点に同一の法線を設定してquadを追加（平面フェース用）
    auto addQuadFlat = [&](const Point3f& p0, const Point3f& p1, const Point3f& p2, const Point3f& p3,
                           const Point3f& norm) { addQuadN(p0, norm, p1, norm, p2, norm, p3, norm); };

    /// コーナーあたりの弧分割数を許容誤差から計算 (chord error = r*(1-cos(π/N)) ≤ tol)
    const float r_ref = std::max(rx, std::max(ry, rz));
    int         segs  = 1;
    if (tol > 0.0f && r_ref > 0.0f) {
        segs = std::max(segs, (int)std::ceil((float)M_PI / std::sqrt(2.0f * tol / r_ref)));
    }

    /// 楕円弧の法線: 楕円の陰関数 ((x-cx)/rx)^2 + ((y-cy)/ry)^2 = 1 の勾配方向
    /// 各軸ペアに対して正規化した法線ベクトルを返す
    auto ellipseNormXY = [&](float cos_t, float sin_t, float dx, float dy) -> Point3f {
        return Point3f(dx * cos_t / rx, dy * sin_t / ry, 0.0f).normalized();
    };
    auto ellipseNormXZ = [&](float cos_u, float sin_u, float dx, float dz) -> Point3f {
        return Point3f(dx * cos_u / rx, 0.0f, dz * sin_u / rz).normalized();
    };
    auto ellipseNormYZ = [&](float cos_v, float sin_v, float dy, float dz) -> Point3f {
        return Point3f(0.0f, dy * cos_v / ry, dz * sin_v / rz).normalized();
    };

    /// 楕円球面コーナーの法線: 楕円球面の陰関数の勾配方向
    /// P(u,v) = (cx+dx*rx*sin(u)*cos(v), cy+dy*ry*sin(u)*sin(v), cz+dz*rz*cos(u))
    auto sphereNorm = [&](float u, float v, float dx, float dy, float dz) -> Point3f {
        return Point3f(dx * std::sin(u) * std::cos(v) / rx, dy * std::sin(u) * std::sin(v) / ry, dz * std::cos(u) / rz)
            .normalized();
    };

    // ---- 6平面フェース ----
    /// Z- 面 (法線: -Z)
    addQuadFlat({x0 + rx, y0 + ry, z0}, {x0 + rx, y0 + H - ry, z0}, {x0 + W - rx, y0 + H - ry, z0},
                {x0 + W - rx, y0 + ry, z0}, {0.0f, 0.0f, -1.0f});
    /// Z+ 面 (法線: +Z)
    addQuadFlat({x0 + rx, y0 + ry, z0 + D}, {x0 + W - rx, y0 + ry, z0 + D}, {x0 + W - rx, y0 + H - ry, z0 + D},
                {x0 + rx, y0 + H - ry, z0 + D}, {0.0f, 0.0f, 1.0f});
    /// X- 面 (法線: -X)
    addQuadFlat({x0, y0 + ry, z0 + rz}, {x0, y0 + ry, z0 + D - rz}, {x0, y0 + H - ry, z0 + D - rz},
                {x0, y0 + H - ry, z0 + rz}, {-1.0f, 0.0f, 0.0f});
    /// X+ 面 (法線: +X)
    addQuadFlat({x0 + W, y0 + ry, z0 + rz}, {x0 + W, y0 + H - ry, z0 + rz}, {x0 + W, y0 + H - ry, z0 + D - rz},
                {x0 + W, y0 + ry, z0 + D - rz}, {1.0f, 0.0f, 0.0f});
    /// Y- 面 (法線: -Y)
    addQuadFlat({x0 + rx, y0, z0 + rz}, {x0 + W - rx, y0, z0 + rz}, {x0 + W - rx, y0, z0 + D - rz},
                {x0 + rx, y0, z0 + D - rz}, {0.0f, -1.0f, 0.0f});
    /// Y+ 面 (法線: +Y)
    addQuadFlat({x0 + rx, y0 + H, z0 + rz}, {x0 + rx, y0 + H, z0 + D - rz}, {x0 + W - rx, y0 + H, z0 + D - rz},
                {x0 + W - rx, y0 + H, z0 + rz}, {0.0f, 1.0f, 0.0f});

    // ---- 12辺の円弧ストリップ ----
    /// Z平行辺 (4本): XYコーナーに沿ってZ方向のストリップ
    /// P(t, z) = (cx + dx*rx*cos(t), cy + dy*ry*sin(t), z)
    /// 法線は楕円弧の勾配: (dx*cos(t)/rx, dy*sin(t)/ry, 0) を正規化
    for (int sx = 0; sx < 2; sx++) {
        for (int sy = 0; sy < 2; sy++) {
            const float cx   = x0 + (sx ? W - rx : rx);
            const float cy   = y0 + (sy ? H - ry : ry);
            const float dx   = sx ? +1.0f : -1.0f;
            const float dy   = sy ? +1.0f : -1.0f;
            const bool  flip = (dx * dy < 0.0f);
            for (int k = 0; k < segs; k++) {
                const float   t0 = (float)k * (float)M_PI_2 / segs;
                const float   t1 = (float)(k + 1) * (float)M_PI_2 / segs;
                const float   c0 = std::cos(t0), s0 = std::sin(t0);
                const float   c1 = std::cos(t1), s1 = std::sin(t1);
                const float   px0 = cx + dx * rx * c0, py0 = cy + dy * ry * s0;
                const float   px1 = cx + dx * rx * c1, py1 = cy + dy * ry * s1;
                const Point3f n0   = ellipseNormXY(c0, s0, dx, dy);
                const Point3f n1   = ellipseNormXY(c1, s1, dx, dy);
                const Point3f pt00 = {px0, py0, z0 + rz};
                const Point3f pt01 = {px0, py0, z0 + D - rz};
                const Point3f pt10 = {px1, py1, z0 + rz};
                const Point3f pt11 = {px1, py1, z0 + D - rz};
                if (flip) {
                    addQuadN(pt00, n0, pt01, n0, pt11, n1, pt10, n1);
                }
                else {
                    addQuadN(pt00, n0, pt10, n1, pt11, n1, pt01, n0);
                }
            }
        }
    }

    /// Y平行辺 (4本): XZコーナーに沿ってY方向のストリップ
    /// P(u, y) = (cx + dx*rx*cos(u), y, cz + dz*rz*sin(u))
    /// 法線は楕円弧の勾配: (dx*cos(u)/rx, 0, dz*sin(u)/rz) を正規化
    for (int sx = 0; sx < 2; sx++) {
        for (int sz = 0; sz < 2; sz++) {
            const float cx   = x0 + (sx ? W - rx : rx);
            const float cz   = z0 + (sz ? D - rz : rz);
            const float dx   = sx ? +1.0f : -1.0f;
            const float dz   = sz ? +1.0f : -1.0f;
            const bool  flip = (dx * dz < 0.0f);
            for (int k = 0; k < segs; k++) {
                const float   u0 = (float)k * (float)M_PI_2 / segs;
                const float   u1 = (float)(k + 1) * (float)M_PI_2 / segs;
                const float   c0 = std::cos(u0), s0 = std::sin(u0);
                const float   c1 = std::cos(u1), s1 = std::sin(u1);
                const float   px0 = cx + dx * rx * c0, pz0 = cz + dz * rz * s0;
                const float   px1 = cx + dx * rx * c1, pz1 = cz + dz * rz * s1;
                const Point3f n0   = ellipseNormXZ(c0, s0, dx, dz);
                const Point3f n1   = ellipseNormXZ(c1, s1, dx, dz);
                const Point3f pt00 = {px0, y0 + ry, pz0};
                const Point3f pt01 = {px0, y0 + H - ry, pz0};
                const Point3f pt10 = {px1, y0 + ry, pz1};
                const Point3f pt11 = {px1, y0 + H - ry, pz1};
                if (flip) {
                    addQuadN(pt00, n0, pt10, n1, pt11, n1, pt01, n0);
                }
                else {
                    addQuadN(pt00, n0, pt01, n0, pt11, n1, pt10, n1);
                }
            }
        }
    }

    /// X平行辺 (4本): YZコーナーに沿ってX方向のストリップ
    /// P(v, x) = (x, cy + dy*ry*cos(v), cz + dz*rz*sin(v))
    /// 法線は楕円弧の勾配: (0, dy*cos(v)/ry, dz*sin(v)/rz) を正規化
    for (int sy = 0; sy < 2; sy++) {
        for (int sz = 0; sz < 2; sz++) {
            const float cy   = y0 + (sy ? H - ry : ry);
            const float cz   = z0 + (sz ? D - rz : rz);
            const float dy   = sy ? +1.0f : -1.0f;
            const float dz   = sz ? +1.0f : -1.0f;
            const bool  flip = (dy * dz < 0.0f);
            for (int k = 0; k < segs; k++) {
                const float   v0 = (float)k * (float)M_PI_2 / segs;
                const float   v1 = (float)(k + 1) * (float)M_PI_2 / segs;
                const float   c0 = std::cos(v0), s0 = std::sin(v0);
                const float   c1 = std::cos(v1), s1 = std::sin(v1);
                const float   py0 = cy + dy * ry * c0, pz0 = cz + dz * rz * s0;
                const float   py1 = cy + dy * ry * c1, pz1 = cz + dz * rz * s1;
                const Point3f n0   = ellipseNormYZ(c0, s0, dy, dz);
                const Point3f n1   = ellipseNormYZ(c1, s1, dy, dz);
                const Point3f pt00 = {x0 + rx, py0, pz0};
                const Point3f pt01 = {x0 + W - rx, py0, pz0};
                const Point3f pt10 = {x0 + rx, py1, pz1};
                const Point3f pt11 = {x0 + W - rx, py1, pz1};
                if (flip) {
                    addQuadN(pt00, n0, pt01, n0, pt11, n1, pt10, n1);
                }
                else {
                    addQuadN(pt00, n0, pt10, n1, pt11, n1, pt01, n0);
                }
            }
        }
    }

    // ---- 8コーナー (楕円球面の1/8パッチ) ----
    /// P(u,v) = (cx + dx*rx*sin(u)*cos(v),
    ///           cy + dy*ry*sin(u)*sin(v),
    ///           cz + dz*rz*cos(u))
    /// 法線は楕円球面の陰関数の勾配:
    ///   (dx*sin(u)*cos(v)/rx, dy*sin(u)*sin(v)/ry, dz*cos(u)/rz) を正規化
    for (int sx = 0; sx < 2; sx++) {
        for (int sy = 0; sy < 2; sy++) {
            for (int sz = 0; sz < 2; sz++) {
                const float cx  = x0 + (sx ? W - rx : rx);
                const float cy  = y0 + (sy ? H - ry : ry);
                const float cz  = z0 + (sz ? D - rz : rz);
                const float dx  = sx ? +1.0f : -1.0f;
                const float dy  = sy ? +1.0f : -1.0f;
                const float dz  = sz ? +1.0f : -1.0f;
                auto        spt = [&](int ui, int vi) -> Point3f {
                    const float u = (float)ui * (float)M_PI_2 / segs;
                    const float v = (float)vi * (float)M_PI_2 / segs;
                    return {cx + dx * rx * std::sin(u) * std::cos(v), cy + dy * ry * std::sin(u) * std::sin(v),
                            cz + dz * rz * std::cos(u)};
                };
                auto snrm = [&](int ui, int vi) -> Point3f {
                    const float u = (float)ui * (float)M_PI_2 / segs;
                    const float v = (float)vi * (float)M_PI_2 / segs;
                    return sphereNorm(u, v, dx, dy, dz);
                };
                /// マイナス符号の個数が奇数のとき（sx+sy+sz が偶数: 0,2 → マイナス1つまたは3つ）
                /// d/du×d/dv が内向きになるので巻き順を反転して外向き法線を保つ
                const bool flip = ((sx + sy + sz) % 2 == 0);
                for (int ui = 0; ui < segs; ui++) {
                    for (int vi = 0; vi < segs; vi++) {
                        if (flip) {
                            addQuadN(spt(ui, vi), snrm(ui, vi), spt(ui, vi + 1), snrm(ui, vi + 1), spt(ui + 1, vi + 1),
                                     snrm(ui + 1, vi + 1), spt(ui + 1, vi), snrm(ui + 1, vi));
                        }
                        else {
                            addQuadN(spt(ui, vi), snrm(ui, vi), spt(ui + 1, vi), snrm(ui + 1, vi), spt(ui + 1, vi + 1),
                                     snrm(ui + 1, vi + 1), spt(ui, vi + 1), snrm(ui, vi + 1));
                        }
                    }
                }
            }
        }
    }

    /// ---- ワイヤーフレーム (m_segments_indices) ----
    /// 頂点は上記で生成済みのものをそのままインデックスで参照する。新頂点は追加しない。
    if (m_create_section_line) {
        /// 線分1本分のインデックスペアを追加するヘルパー
        auto addSeg = [&](unsigned int a, unsigned int b) {
            m_segments_indices.emplace_back(a);
            m_segments_indices.emplace_back(b);
        };

        /// 暫定:固定値
        int wire_segs = 4;    /// 斜めの線分数

        /// m_segments_indices の容量を事前確保
        /// Part1: 24線分; Part2: 24弧×segs線分; Part3: 8コーナー×wire_segs×2方向×segs線分
        m_segments_indices.reserve(m_segments_indices.size() + 2 * (24 + 24 * segs + 16 * wire_segs * segs));

        /// ---- Part 1: 平面フェースの境界直線 (各フェース4辺 × 6面 = 24本) ----
        /// 頂点インデックス 0-23 は6面分の平面フェース頂点
        /// 各フェースはaddQuadFlatで追加された4頂点: v[0],v[1],v[2],v[3] (連続インデックス)
        ///
        /// Z- 面 (base=0): v0={x0+rx,y0+ry,z0}, v1={x0+rx,y0+H-ry,z0},
        ///                  v2={x0+W-rx,y0+H-ry,z0}, v3={x0+W-rx,y0+ry,z0}
        addSeg(0, 1);
        addSeg(1, 2);
        addSeg(2, 3);
        addSeg(3, 0);
        /// Z+ 面 (base=4): v0={x0+rx,y0+ry,z0+D}, v1={x0+W-rx,y0+ry,z0+D},
        ///                  v2={x0+W-rx,y0+H-ry,z0+D}, v3={x0+rx,y0+H-ry,z0+D}
        addSeg(4, 5);
        addSeg(5, 6);
        addSeg(6, 7);
        addSeg(7, 4);
        /// X- 面 (base=8): v0={x0,y0+ry,z0+rz}, v1={x0,y0+ry,z0+D-rz},
        ///                  v2={x0,y0+H-ry,z0+D-rz}, v3={x0,y0+H-ry,z0+rz}
        addSeg(8, 9);
        addSeg(9, 10);
        addSeg(10, 11);
        addSeg(11, 8);
        /// X+ 面 (base=12): v0={x0+W,y0+ry,z0+rz}, v1={x0+W,y0+H-ry,z0+rz},
        ///                   v2={x0+W,y0+H-ry,z0+D-rz}, v3={x0+W,y0+ry,z0+D-rz}
        addSeg(12, 13);
        addSeg(13, 14);
        addSeg(14, 15);
        addSeg(15, 12);
        /// Y- 面 (base=16): v0={x0+rx,y0,z0+rz}, v1={x0+W-rx,y0,z0+rz},
        ///                   v2={x0+W-rx,y0,z0+D-rz}, v3={x0+rx,y0,z0+D-rz}
        addSeg(16, 17);
        addSeg(17, 18);
        addSeg(18, 19);
        addSeg(19, 16);
        /// Y+ 面 (base=20): v0={x0+rx,y0+H,z0+rz}, v1={x0+rx,y0+H,z0+D-rz},
        ///                   v2={x0+W-rx,y0+H,z0+D-rz}, v3={x0+W-rx,y0+H,z0+rz}
        addSeg(20, 21);
        addSeg(21, 22);
        addSeg(22, 23);
        addSeg(23, 20);

        /// ---- Part 2: ストリップ↔コーナーパッチ境界の弧 ----
        /// 各辺ストリップの2つのシームエッジ (ストリップが四角形quadから成り、
        /// 各quadの "両端辺" がシームになる) を既存頂点インデックスで参照。
        ///
        /// Z平行ストリップ (start=24, 順序 sx=0,1; sy=0,1):
        ///   flip=false (sx==sy):  z=z0+rz側: B+0→B+1,  z=z0+D-rz側: B+3→B+2
        ///   flip=true  (sx!=sy):  z=z0+rz側: B+0→B+3,  z=z0+D-rz側: B+1→B+2
        for (int sx = 0; sx < 2; sx++) {
            for (int sy = 0; sy < 2; sy++) {
                const bool         flip       = (sx != sy);
                const unsigned int strip_base = 24u + (unsigned int)(sx * 2 + sy) * segs * 4;
                for (int k = 0; k < segs; k++) {
                    const unsigned int B = strip_base + (unsigned int)k * 4;
                    if (!flip) {
                        addSeg(B + 0, B + 1);
                        addSeg(B + 3, B + 2);
                    }
                    else {
                        addSeg(B + 0, B + 3);
                        addSeg(B + 1, B + 2);
                    }
                }
            }
        }
        /// Y平行ストリップ (start=24+16*segs, 順序 sx=0,1; sz=0,1):
        ///   flip=false (sx==sz):  y=y0+ry側: B+0→B+3,  y=y0+H-ry側: B+1→B+2
        ///   flip=true  (sx!=sz):  y=y0+ry側: B+0→B+1,  y=y0+H-ry側: B+3→B+2
        for (int sx = 0; sx < 2; sx++) {
            for (int sz = 0; sz < 2; sz++) {
                const bool         flip       = (sx != sz);
                const unsigned int strip_base = 24u + 16u * segs + (unsigned int)(sx * 2 + sz) * segs * 4;
                for (int k = 0; k < segs; k++) {
                    const unsigned int B = strip_base + (unsigned int)k * 4;
                    if (!flip) {
                        addSeg(B + 0, B + 3);
                        addSeg(B + 1, B + 2);
                    }
                    else {
                        addSeg(B + 0, B + 1);
                        addSeg(B + 3, B + 2);
                    }
                }
            }
        }
        /// X平行ストリップ (start=24+32*segs, 順序 sy=0,1; sz=0,1):
        ///   flip=false (sy==sz):  x=x0+rx側: B+0→B+1,  x=x0+W-rx側: B+3→B+2
        ///   flip=true  (sy!=sz):  x=x0+rx側: B+0→B+3,  x=x0+W-rx側: B+1→B+2
        for (int sy = 0; sy < 2; sy++) {
            for (int sz = 0; sz < 2; sz++) {
                const bool         flip       = (sy != sz);
                const unsigned int strip_base = 24u + 32u * segs + (unsigned int)(sy * 2 + sz) * segs * 4;
                for (int k = 0; k < segs; k++) {
                    const unsigned int B = strip_base + (unsigned int)k * 4;
                    if (!flip) {
                        addSeg(B + 0, B + 1);
                        addSeg(B + 3, B + 2);
                    }
                    else {
                        addSeg(B + 0, B + 3);
                        addSeg(B + 1, B + 2);
                    }
                }
            }
        }

        /// ---- Part 3: コーナーパッチの緯度経度線 (wire_segs 本の中間線) ----
        /// コーナーパッチ (start=24+48*segs, 順序 sx=0,1; sy=0,1; sz=0,1):
        ///   各コーナーは segs×segs 個のquadで構成。quad(ui,vi)のbase B を計算し
        ///   quad内の辺インデックスでシームを参照する。
        ///   flip=true  (sx+sy+sz偶数): 緯度シーム(u=ui+1): B+3→B+2, 経度シーム(v=vi+1): B+1→B+2
        ///   flip=false (sx+sy+sz奇数): 緯度シーム(u=ui+1): B+1→B+2, 経度シーム(v=vi+1): B+3→B+2
        if (wire_segs > 0) {
            const unsigned int base_corners = 24u + 48u * segs;
            for (int sx = 0; sx < 2; sx++) {
                for (int sy = 0; sy < 2; sy++) {
                    for (int sz = 0; sz < 2; sz++) {
                        const bool         flip = ((sx + sy + sz) % 2 == 0);
                        const unsigned int corner_base =
                            base_corners + (unsigned int)(sx * 4 + sy * 2 + sz) * segs * segs * 4;
                        /// 緯度線: u=row_ui+1 のシーム (固定u, vを[0,π/2]に沿って描く "平行圏")
                        for (int i = 1; i <= wire_segs; i++) {
                            const int row_ui =
                                std::max(0, std::min(segs - 1, (int)std::round((float)i * segs / (wire_segs + 1)) - 1));
                            for (int vi = 0; vi < segs; vi++) {
                                const unsigned int B = corner_base + (unsigned int)(row_ui * segs + vi) * 4;
                                if (flip) {
                                    addSeg(B + 3, B + 2);
                                }
                                else {
                                    addSeg(B + 1, B + 2);
                                }
                            }
                        }
                        /// 経度線: v=col_vi+1 のシーム (固定v, uを[0,π/2]に沿って描く "子午線")
                        for (int j = 1; j <= wire_segs; j++) {
                            const int col_vi =
                                std::max(0, std::min(segs - 1, (int)std::round((float)j * segs / (wire_segs + 1)) - 1));
                            for (int ui = 0; ui < segs; ui++) {
                                const unsigned int B = corner_base + (unsigned int)(ui * segs + col_vi) * 4;
                                if (flip) {
                                    addSeg(B + 1, B + 2);
                                }
                                else {
                                    addSeg(B + 3, B + 2);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    markRenderDirty();
    markBoxDirty();
}

void RenderNormalMesh::updateBoundingBox()
{
    m_bbox.init();
    for (const auto& vertex : m_vertices) {
        m_bbox.expandBy(vertex.m_position);
    }
    resetBoxDirty();
}

BoundingBox3f RenderNormalMesh::calculateBoundingBox(const Matrix4x4f& parent_matrix, bool /*only_visible*/,
                                                     bool /*including_text*/) const
{
    BoundingBox3f bbox;
    for (const auto& vertex : m_vertices) {
        bbox.expandBy(parent_matrix * vertex.m_position);
    }
    return bbox;
}

void RenderNormalMesh::appendQuad(const Point3f& p0, const Point3f& p1, const Point3f& p2, const Point3f& p3,
                                  const Point3f& norm, int get_seg_flg)
{
    unsigned int size = (unsigned int)m_vertices.size();
    m_vertices.emplace_back(p0, norm);
    m_vertices.emplace_back(p1, norm);
    m_vertices.emplace_back(p2, norm);
    m_vertices.emplace_back(p3, norm);

    m_indices.emplace_back(size);        /// 0
    m_indices.emplace_back(size + 1);    /// 1
    m_indices.emplace_back(size + 2);    /// 2

    m_indices.emplace_back(size + 2);    /// 2
    m_indices.emplace_back(size + 3);    /// 3
    m_indices.emplace_back(size);        /// 0

    if (!m_create_section_line) {
        return;
    }
    /// 線分の取得
    if (get_seg_flg == 1) {
        m_segments_indices.emplace_back(size + 1);
        m_segments_indices.emplace_back(size + 2);

        m_segments_indices.emplace_back(size + 3);
        m_segments_indices.emplace_back(size + 0);
    }
    else if (get_seg_flg == 2) {
        m_segments_indices.emplace_back(size + 0);
        m_segments_indices.emplace_back(size + 1);

        m_segments_indices.emplace_back(size + 2);
        m_segments_indices.emplace_back(size + 3);
    }
}

void RenderNormalMesh::appendQuad(const Point3f& p0, const Point3f& p1, const Point3f& p2, const Point3f& p3,
                                  int get_seg_flg)
{
    auto norm = ((p1 - p0) ^ (p2 - p0)).normalized();

    unsigned int size = (unsigned int)m_vertices.size();
    m_vertices.emplace_back(p0, norm);
    m_vertices.emplace_back(p1, norm);
    m_vertices.emplace_back(p2, norm);
    m_vertices.emplace_back(p3, norm);

    m_indices.emplace_back(size);        /// 0
    m_indices.emplace_back(size + 1);    /// 1
    m_indices.emplace_back(size + 2);    /// 2

    m_indices.emplace_back(size + 2);    /// 2
    m_indices.emplace_back(size + 3);    /// 3
    m_indices.emplace_back(size);        /// 0

    if (!m_create_section_line) {
        return;
    }
    /// 線分の取得
    if (get_seg_flg == 1) {
        m_segments_indices.emplace_back(size + 1);
        m_segments_indices.emplace_back(size + 2);

        m_segments_indices.emplace_back(size + 3);
        m_segments_indices.emplace_back(size + 0);
    }
    else if (get_seg_flg == 2) {
        m_segments_indices.emplace_back(size + 0);
        m_segments_indices.emplace_back(size + 1);

        m_segments_indices.emplace_back(size + 2);
        m_segments_indices.emplace_back(size + 3);
    }
}

void RenderNormalMesh::clearDisplayData()
{
    m_vertices.clear();
    m_indices.clear();
    m_segments_indices.clear();
    markRenderDirty();
    markBoxDirty();
}

DEFINE_META_RENDERABLENODE(RenderEditableNormalMesh, RenderNormalMesh)

RenderEditableNormalMesh::RenderEditableNormalMesh(RenderNormalMesh* mesh)
{
    setRenderableObject(mesh);
}

RenderEditableNormalMesh::~RenderEditableNormalMesh() {}

RenderEditableNormalMesh::RenderEditableNormalMesh(const RenderEditableNormalMesh& other)
{
    setRenderableObject(other.renderableObject());
    if (other.m_projection_node.isAlive()) {
        m_projection_node = other.m_projection_node;
    }
}

void RenderEditableNormalMesh::updateBoundingBox()
{
    m_bbox.init();
    if (isEnableEditDisplayData()) {
        const auto& vertices = displayEditVertices();
        for (const auto& vertex : vertices) {
            m_bbox.expandBy(vertex.m_position);
        }
    }
    else {
        m_bbox = originalMesh()->boundingBox();
    }
    // createGroupBoundingBox();
    resetBoxDirty();
}

BoundingBox3f RenderEditableNormalMesh::calculateBoundingBox(const Matrix4x4f& parent_matrix, bool only_visible,
                                                             bool /*including_text*/) const
{
    BoundingBox3f bbox;
    if (only_visible && isEnableEditDisplayData()) {
        const auto& vertices = displayEditVertices();
        for (const auto& vertex : vertices) {
            bbox.expandBy(parent_matrix * vertex.m_position);
        }
    }
    else {
        for (const auto& vertex : originalMesh()->m_vertices) {
            bbox.expandBy(parent_matrix * vertex.m_position);
        }
    }
    return bbox;
}

void RenderEditableNormalMesh::createDisplaySegmentsGroupData()
{
    if (!isSegmentsGroupDirty()) {
        return;
    }
    const auto& vertex_list      = isEnableEditDisplayData() ? displayEditVertices() : displayVertices();
    const auto& segments_indices = isEnableEditDisplayData() ? displayEditSegmentIndices() : displaySegmentIndices();

    /// 高速化のため
    int segments_group_count = (segments_indices.size() / 2 + 99) / 100;
    m_segments_group_box.resize(segments_group_count);
    m_segments_group_start_index.resize(segments_group_count, INT_MAX);
    for (size_t i = 0; i < segments_indices.size(); i += 2) {
        int group_count = (i / 2) / 100;

        m_segments_group_box[group_count].expandBy(vertex_list[segments_indices[i]].m_position);
        m_segments_group_box[group_count].expandBy(vertex_list[segments_indices[i + 1]].m_position);
        if (m_segments_group_start_index[group_count] == INT_MAX) {
            m_segments_group_start_index[group_count] = i;
        }
    }

    resetSegmentsGroupDirty();
}

void RenderEditableNormalMesh::createGroupBoundingBox()
{
    if (!isTriaGroupDirty()) {
        return;
    }
    const auto& vertex_list = isEnableEditDisplayData() ? displayEditVertices() : displayVertices();
    const auto& index_list  = isEnableEditDisplayData() ? displayEditIndices() : displayIndices();

    /// 高速化のため
    int trias_group_count = (index_list.size() / 3 + 99) / 100;
    m_tria_group_box.resize(trias_group_count);
    m_tria_group_start_index.resize(trias_group_count, INT_MAX);
    for (size_t i = 0; i < index_list.size(); i += 3) {
        int group_count = (i / 3) / 100;

        m_tria_group_box[group_count].expandBy(vertex_list[index_list[i]].m_position);
        m_tria_group_box[group_count].expandBy(vertex_list[index_list[i + 1]].m_position);
        m_tria_group_box[group_count].expandBy(vertex_list[index_list[i + 2]].m_position);
        if (m_tria_group_start_index[group_count] == INT_MAX) {
            m_tria_group_start_index[group_count] = i;
        }
    }
    resetTriaGroupDirty();
}

void RenderEditableNormalMesh::clearDisplayData()
{
    originalMesh()->clearDisplayData();
    clearDisplayEditData();
    markSegmentsGroupDirty();
    markTriaGroupDirty();
    markRenderDirty();
    markBoxDirty();
}

Node* RenderEditableNormalMesh::projectionNode()
{
    /// 暫定 - 表示フラグはGUIと連動させる
    if (m_projection_node.isAlive() && m_projection_node->isShow()) {
        return m_projection_node.ptr();
    }
    else {
        return (Node*)nullptr;
    }
}

CORE_NAMESPACE_END
