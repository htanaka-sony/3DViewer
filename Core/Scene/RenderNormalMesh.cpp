#include "RenderNormalMesh.h"

#include <algorithm>
#include <array>
#include <cmath>
#include "earcut.hpp"

#include "Math/Point2f.h"

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

    /// 頂点・インデックスバッファへの書き込みカーソル (resize後のインデックス書き込みに使用)
    int vIdx = 0;
    int iIdx = 0;

    /// 各頂点に個別の法線を設定してquadを追加
    /// Triangle1: p0,p1,p2  Triangle2: p2,p3,p0
    auto addQuadN = [&](const Point3f& p0, const Point3f& n0, const Point3f& p1, const Point3f& n1, const Point3f& p2,
                        const Point3f& n2, const Point3f& p3, const Point3f& n3) {
        const unsigned int base = (unsigned int)vIdx;
        m_vertices[vIdx++]      = {p0, n0};
        m_vertices[vIdx++]      = {p1, n1};
        m_vertices[vIdx++]      = {p2, n2};
        m_vertices[vIdx++]      = {p3, n3};
        m_indices[iIdx++]       = base + 0;
        m_indices[iIdx++]       = base + 1;
        m_indices[iIdx++]       = base + 2;
        m_indices[iIdx++]       = base + 2;
        m_indices[iIdx++]       = base + 3;
        m_indices[iIdx++]       = base + 0;

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
        // segs = std::min(segs, 64);    /// 過剰分割を防ぐ上限 (segs=64 で約 32×64×64×8=1M 頂点規模)
    }

    /// sin/cos テーブルを事前計算して使い回す
    /// angle[k] = k * (π/2) / segs, k = 0..segs
    std::vector<float> cosTab(segs + 1), sinTab(segs + 1);
    {
        const float step = (float)M_PI_2 / (float)segs;
        for (int k = 0; k <= segs; k++) {
            cosTab[k] = std::cos((float)k * step);
            sinTab[k] = std::sin((float)k * step);
        }
        /// 端点は丸め誤差を排除して厳密値を設定
        cosTab[0]    = 1.0f;
        sinTab[0]    = 0.0f;
        cosTab[segs] = 0.0f;
        sinTab[segs] = 1.0f;
    }

    /// 頂点・インデックスバッファを事前生成
    /// 6面: 6×4頂点, 12辺: 12×segs×4頂点, 8コーナー: 8×segs²×4頂点
    {
        const int nv = 24 + 48 * segs + 32 * segs * segs;
        m_vertices.resize(nv);
        m_indices.resize(nv / 4 * 6);    /// quad ごとに6インデックス (4頂点で2三角形)
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
                const float   c0 = cosTab[k], s0 = sinTab[k];
                const float   c1 = cosTab[k + 1], s1 = sinTab[k + 1];
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
                const float   c0 = cosTab[k], s0 = sinTab[k];
                const float   c1 = cosTab[k + 1], s1 = sinTab[k + 1];
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
                const float   c0 = cosTab[k], s0 = sinTab[k];
                const float   c1 = cosTab[k + 1], s1 = sinTab[k + 1];
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
    ///
    /// 格子点を事前計算して重複演算を排除 (各 (ui,vi) は隣接 quad から最大4回参照される)
    /// ベクタをループ外で確保してリアロケーションを回避する
    const int            gridStride = segs + 1;
    std::vector<Point3f> gridPt(gridStride * gridStride), gridNrm(gridStride * gridStride);
    for (int sx = 0; sx < 2; sx++) {
        for (int sy = 0; sy < 2; sy++) {
            for (int sz = 0; sz < 2; sz++) {
                const float cx = x0 + (sx ? W - rx : rx);
                const float cy = y0 + (sy ? H - ry : ry);
                const float cz = z0 + (sz ? D - rz : rz);
                const float dx = sx ? +1.0f : -1.0f;
                const float dy = sy ? +1.0f : -1.0f;
                const float dz = sz ? +1.0f : -1.0f;

                /// 格子点と法線を一括計算
                for (int ui = 0; ui <= segs; ui++) {
                    const float su = sinTab[ui], cu = cosTab[ui];
                    for (int vi = 0; vi <= segs; vi++) {
                        const float cosV = cosTab[vi], sinV = sinTab[vi];
                        const int   idx = ui * gridStride + vi;
                        gridPt[idx]     = {cx + dx * rx * su * cosV, cy + dy * ry * su * sinV, cz + dz * rz * cu};
                        gridNrm[idx]    = Point3f(dx * su * cosV / rx, dy * su * sinV / ry, dz * cu / rz).normalized();
                    }
                }

                /// マイナス符号の個数が奇数のとき（sx+sy+sz が偶数: 0,2 → マイナス1つまたは3つ）
                /// d/du×d/dv が内向きになるので巻き順を反転して外向き法線を保つ
                const bool flip = ((sx + sy + sz) % 2 == 0);
                for (int ui = 0; ui < segs; ui++) {
                    for (int vi = 0; vi < segs; vi++) {
                        const int i00 = ui * gridStride + vi;
                        const int i01 = ui * gridStride + vi + 1;
                        const int i10 = (ui + 1) * gridStride + vi;
                        const int i11 = (ui + 1) * gridStride + vi + 1;
                        if (flip) {
                            addQuadN(gridPt[i00], gridNrm[i00], gridPt[i01], gridNrm[i01], gridPt[i11], gridNrm[i11],
                                     gridPt[i10], gridNrm[i10]);
                        }
                        else {
                            addQuadN(gridPt[i00], gridNrm[i00], gridPt[i10], gridNrm[i10], gridPt[i11], gridNrm[i11],
                                     gridPt[i01], gridNrm[i01]);
                        }
                    }
                }
            }
        }
    }

    /// ---- ワイヤーフレーム (m_segments_indices) ----
    /// 頂点は上記で生成済みのものをそのままインデックスで参照する。新頂点は追加しない。
    if (m_create_section_line) {
        int sIdx = 0;
        /// 線分1本分のインデックスペアを追加するヘルパー
        auto addSeg = [&](unsigned int a, unsigned int b) {
            m_segments_indices[sIdx++] = a;
            m_segments_indices[sIdx++] = b;
        };

        /// 暫定:固定値
        int wire_segs = 4;    /// 斜めの線分数

        /// m_segments_indices の容量を事前確保
        /// Part1: 24線分; Part2: 24弧×segs線分; Part3: 8コーナー×wire_segs×2方向×segs線分
        m_segments_indices.resize(m_segments_indices.size() + 2 * (24 + 24 * segs + 16 * wire_segs * segs));

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

void RenderNormalMesh::createEllipsoid(float radius_x, float radius_y, float radius_z, float tol)
{
    m_vertices.clear();
    m_indices.clear();
    m_segments_indices.clear();

    const float rx = radius_x;
    const float ry = radius_y;
    const float rz = radius_z;

    /// 許容誤差から分割数を計算（createBoxRound と同じ基準: 1/4円あたりの弦誤差）
    const float r_ref = std::max({rx, ry, rz});
    int         segs  = 1;
    if (tol > 0.0f && r_ref > 0.0f) {
        segs = std::max(segs, (int)std::ceil((float)M_PI / std::sqrt(2.0f * tol / r_ref)));
    }

    /// 緯度 (u: 0..π) = 2*segs 分割, 経度 (v: 0..2π) = 4*segs 分割
    const int segs_lat = 2 * segs;
    const int segs_lon = 4 * segs;

    /// sin/cos テーブルを事前計算
    std::vector<float> cosU(segs_lat + 1), sinU(segs_lat + 1);
    std::vector<float> cosV(segs_lon + 1), sinV(segs_lon + 1);
    {
        const float stepU = (float)M_PI / (float)segs_lat;
        for (int i = 0; i <= segs_lat; i++) {
            cosU[i] = std::cos((float)i * stepU);
            sinU[i] = std::sin((float)i * stepU);
        }
        /// 極点の丸め誤差を排除
        cosU[0]        = 1.0f;
        sinU[0]        = 0.0f;
        cosU[segs_lat] = -1.0f;
        sinU[segs_lat] = 0.0f;

        const float stepV = 2.0f * (float)M_PI / (float)segs_lon;
        for (int j = 0; j <= segs_lon; j++) {
            cosV[j] = std::cos((float)j * stepV);
            sinV[j] = std::sin((float)j * stepV);
        }
        /// ループを厳密に閉じる
        cosV[segs_lon] = 1.0f;
        sinV[segs_lon] = 0.0f;
    }

    /// 楕円面上の頂点座標: P(u,v) = (rx*sin(u)*cos(v), ry*sin(u)*sin(v), rz*cos(u))
    auto getPos = [&](int ui, int vj) -> Point3f {
        return {rx * sinU[ui] * cosV[vj], ry * sinU[ui] * sinV[vj], rz * cosU[ui]};
    };

    /// 楕円面の外向き法線: ∇((x/rx)²+(y/ry)²+(z/rz)²) = (x/rx², y/ry², z/rz²) を正規化
    auto getNorm = [&](int ui, int vj) -> Point3f {
        return Point3f(sinU[ui] * cosV[vj] / rx, sinU[ui] * sinV[vj] / ry, cosU[ui] / rz).normalized();
    };

    /// 各頂点に個別の法線を設定してquadを追加
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
    };

    /// 楕円面を緯度×経度のquadグリッドとして生成
    /// quad(ui,vj): p(ui,vj), p(ui,vj+1), p(ui+1,vj+1), p(ui+1,vj) → 外向きCCW巻き
    /// 極（ui=0 or ui=segs_lat）では quad が三角形に縮退する
    for (int ui = 0; ui < segs_lat; ui++) {
        for (int vj = 0; vj < segs_lon; vj++) {
            addQuadN(getPos(ui, vj), getNorm(ui, vj), getPos(ui, vj + 1), getNorm(ui, vj + 1), getPos(ui + 1, vj + 1),
                     getNorm(ui + 1, vj + 1), getPos(ui + 1, vj), getNorm(ui + 1, vj));
        }
    }

    /// ---- ワイヤーフレーム (m_segments_indices) ----
    if (m_create_section_line) {
        /// quad(ui,vj) の頂点レイアウト: base = (ui*segs_lon + vj)*4
        ///   base+0 = p(ui,  vj),   base+1 = p(ui,  vj+1)
        ///   base+2 = p(ui+1,vj+1), base+3 = p(ui+1,vj)

        /// 緯度線: 等間隔で wire_lat 本の中間緯線を描く
        const int wire_lat = 4;
        for (int i = 0; i < wire_lat; i++) {
            const int ui =
                std::max(0, std::min(segs_lat - 1, (int)std::round((float)(i + 1) * segs_lat / (wire_lat + 1)) - 1));
            for (int vj = 0; vj < segs_lon; vj++) {
                const unsigned int base = (unsigned int)(ui * segs_lon + vj) * 4;
                m_segments_indices.emplace_back(base + 3);    /// p(ui+1, vj)
                m_segments_indices.emplace_back(base + 2);    /// p(ui+1, vj+1)
            }
        }

        /// 経度線: wire_lon 本を等間隔で描く
        const int wire_lon = 4;
        for (int j = 0; j < wire_lon; j++) {
            const int vj = (int)std::round((float)j * segs_lon / wire_lon) % segs_lon;
            for (int ui = 0; ui < segs_lat; ui++) {
                const unsigned int base = (unsigned int)(ui * segs_lon + vj) * 4;
                m_segments_indices.emplace_back(base + 0);    /// p(ui,   vj)
                m_segments_indices.emplace_back(base + 3);    /// p(ui+1, vj)
            }
        }
    }

    markRenderDirty();
    markBoxDirty();
}

void RenderNormalMesh::createEllipticalCylinder(float radius_x, float radius_y, float height, float taper_dist,
                                                float tol)
{
    m_vertices.clear();
    m_indices.clear();
    m_segments_indices.clear();

    /// テーパー方向に応じて上底・下底の半径を決定
    /// taper_dist > 0: 上部縮小、taper_dist < 0: 下部縮小
    float rx_bot, ry_bot, rx_top, ry_top;
    if (taper_dist >= 0.0f) {
        rx_bot = radius_x;
        ry_bot = radius_y;
        rx_top = std::max(0.0f, radius_x - taper_dist);
        ry_top = std::max(0.0f, radius_y - taper_dist);
    }
    else {
        rx_bot = std::max(0.0f, radius_x + taper_dist);
        ry_bot = std::max(0.0f, radius_y + taper_dist);
        rx_top = radius_x;
        ry_top = radius_y;
    }

    /// 許容誤差から分割数を計算（createBoxRound と同じ基準）
    const float r_ref = std::max(radius_x, radius_y);
    int         segs  = 1;
    if (tol > 0.0f && r_ref > 0.0f) {
        segs = std::max(segs, (int)std::ceil((float)M_PI / std::sqrt(2.0f * tol / r_ref)));
    }

    /// 経度分割数 (全円 = 4*segs)
    const int segs_v = 4 * segs;

    /// sin/cos テーブルを事前計算
    std::vector<float> cosV(segs_v + 1), sinV(segs_v + 1);
    {
        const float step = 2.0f * (float)M_PI / (float)segs_v;
        for (int j = 0; j <= segs_v; j++) {
            cosV[j] = std::cos((float)j * step);
            sinV[j] = std::sin((float)j * step);
        }
        cosV[segs_v] = 1.0f;
        sinV[segs_v] = 0.0f;
    }

    /// 各頂点に個別の法線を設定してquadを追加
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
    };

    auto addQuadFlat = [&](const Point3f& p0, const Point3f& p1, const Point3f& p2, const Point3f& p3,
                           const Point3f& norm) { addQuadN(p0, norm, p1, norm, p2, norm, p3, norm); };

    /// テーパー側面の外向き法線
    /// 楕円錐台の陰関数の勾配を用いた正確な法線計算:
    ///   ∂P/∂t = (-rx(z)*sin(t), ry(z)*cos(t), 0)
    ///   ∂P/∂z = (drx*cos(t), dry*sin(t), 1)
    ///   N = ∂P/∂t × ∂P/∂z = (ry(z)*cos(t), rx(z)*sin(t), -rx(z)*dry*sin²(t) - ry(z)*drx*cos²(t))
    const float drx   = (height > 0.0f) ? (rx_top - rx_bot) / height : 0.0f;
    const float dry   = (height > 0.0f) ? (ry_top - ry_bot) / height : 0.0f;
    auto        sideN = [&](float rxi, float ryi, float ct, float st) -> Point3f {
        return Point3f(ryi * ct, rxi * st, -rxi * dry * st * st - ryi * drx * ct * ct).normalized();
    };

    /// ---- 側面: segs_v 枚のquad ----
    /// quad(vj): pBot[vj], pBot[vj+1], pTop[vj+1], pTop[vj] → 外向きCCW巻き
    for (int vj = 0; vj < segs_v; vj++) {
        const float   ct0 = cosV[vj], st0 = sinV[vj];
        const float   ct1 = cosV[vj + 1], st1 = sinV[vj + 1];
        const Point3f pBot0 = {rx_bot * ct0, ry_bot * st0, 0.0f};
        const Point3f pBot1 = {rx_bot * ct1, ry_bot * st1, 0.0f};
        const Point3f pTop0 = {rx_top * ct0, ry_top * st0, height};
        const Point3f pTop1 = {rx_top * ct1, ry_top * st1, height};
        addQuadN(pBot0, sideN(rx_bot, ry_bot, ct0, st0), pBot1, sideN(rx_bot, ry_bot, ct1, st1), pTop1,
                 sideN(rx_top, ry_top, ct1, st1), pTop0, sideN(rx_top, ry_top, ct0, st0));
    }

    /// ---- 下底面 (z=0, 法線: -Z) ----
    /// 縮退quad (中心点 + 外周2点) で三角形を生成。上から見てCW巻き = -Z側からCCW ✓
    const Point3f botCenter = {0.0f, 0.0f, 0.0f};
    const Point3f normBot   = {0.0f, 0.0f, -1.0f};
    for (int vj = 0; vj < segs_v; vj++) {
        const Point3f eBot0 = {rx_bot * cosV[vj], ry_bot * sinV[vj], 0.0f};
        const Point3f eBot1 = {rx_bot * cosV[vj + 1], ry_bot * sinV[vj + 1], 0.0f};
        addQuadFlat(botCenter, eBot1, eBot0, botCenter, normBot);
    }

    /// ---- 上底面 (z=height, 法線: +Z) ----
    /// 縮退quad (中心点 + 外周2点) で三角形を生成。上から見てCCW巻き = +Z側からCCW ✓
    const Point3f topCenter = {0.0f, 0.0f, height};
    const Point3f normTop   = {0.0f, 0.0f, 1.0f};
    for (int vj = 0; vj < segs_v; vj++) {
        const Point3f eTop0 = {rx_top * cosV[vj], ry_top * sinV[vj], height};
        const Point3f eTop1 = {rx_top * cosV[vj + 1], ry_top * sinV[vj + 1], height};
        addQuadFlat(topCenter, eTop0, eTop1, topCenter, normTop);
    }

    /// ---- ワイヤーフレーム (m_segments_indices) ----
    if (m_create_section_line) {
        /// 側面 quad(vj) のベース頂点インデックス: vj * 4
        ///   base+0 = pBot[vj], base+1 = pBot[vj+1]
        ///   base+2 = pTop[vj+1], base+3 = pTop[vj]

        /// 上底・下底の外周楕円輪郭線
        for (int vj = 0; vj < segs_v; vj++) {
            const unsigned int base = (unsigned int)vj * 4;
            m_segments_indices.emplace_back(base + 0);    /// pBot[vj]
            m_segments_indices.emplace_back(base + 1);    /// pBot[vj+1]
            m_segments_indices.emplace_back(base + 3);    /// pTop[vj]
            m_segments_indices.emplace_back(base + 2);    /// pTop[vj+1]
        }

        /// 縦辺: wire_lon 本を等間隔で描く
        const int wire_lon = 4;
        for (int i = 0; i < wire_lon; i++) {
            const int          vj   = (int)std::round((float)i * segs_v / wire_lon) % segs_v;
            const unsigned int base = (unsigned int)vj * 4;
            m_segments_indices.emplace_back(base + 0);    /// pBot[vj]
            m_segments_indices.emplace_back(base + 3);    /// pTop[vj]
        }
    }

    markRenderDirty();
    markBoxDirty();
}

namespace {

// axis: 0=X (断面YZ, 押し出しX方向), 1=Y (断面XZ, 押し出しY方向), 2=Z (断面XY, 押し出しZ方向)
static void buildPolygonPrism(std::vector<Vertexf>& m_vertices, std::vector<unsigned int>& m_indices,
                              std::vector<unsigned int>& m_segments_indices, bool m_create_section_line,
                              const VecPoint2f& vertices, float height, int axis)
{
    m_vertices.clear();
    m_indices.clear();
    m_segments_indices.clear();

    if (vertices.size() < 3 || height <= 0.0f) {
        return;
    }

    std::vector<Point2f> poly(vertices.begin(), vertices.end());
    if (poly.size() >= 2) {
        const Point2f& a = poly.front();
        const Point2f& b = poly.back();
        if (std::fabs(a.x() - b.x()) < 1.0e-7f && std::fabs(a.y() - b.y()) < 1.0e-7f) {
            poly.pop_back();
        }
    }
    if (poly.size() < 3) {
        return;
    }

    float signed_area = 0.0f;
    for (size_t i = 0; i < poly.size(); i++) {
        const size_t j = (i + 1) % poly.size();
        signed_area += poly[i].x() * poly[j].y() - poly[j].x() * poly[i].y();
    }
    signed_area *= 0.5f;
    if (std::fabs(signed_area) < 1.0e-7f) {
        return;
    }
    // 全軸でearcutに渡す前にCCW(正の符号付き面積)に統一する
    if (signed_area < 0.0f) {
        std::reverse(poly.begin(), poly.end());
    }

    // 2D断面座標 (a, b) → 3D底面点 / 頂面点
    auto makeBase = [&](float a, float b) -> Point3f {
        switch (axis) {
            case 0:
                return {0.0f, a, b};    // X軸: 底面 x=0
            case 1:
                return {a, 0.0f, b};    // Y軸: 底面 y=0
            default:
                return {a, b, 0.0f};    // Z軸: 底面 z=0
        }
    };
    auto makeTop = [&](float a, float b) -> Point3f {
        switch (axis) {
            case 0:
                return {height, a, b};    // X軸: 頂面 x=height
            case 1:
                return {a, height, b};    // Y軸: 頂面 y=height
            default:
                return {a, b, height};    // Z軸: 頂面 z=height
        }
    };

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
    };
    auto addTriFlat = [&](const Point3f& a, const Point3f& b, const Point3f& c, const Point3f& n) {
        const unsigned int base = (unsigned int)m_vertices.size();
        m_vertices.emplace_back(a, n);
        m_vertices.emplace_back(b, n);
        m_vertices.emplace_back(c, n);
        m_indices.emplace_back(base + 0);
        m_indices.emplace_back(base + 1);
        m_indices.emplace_back(base + 2);
    };

    // 側面
    for (size_t i = 0; i < poly.size(); i++) {
        const size_t  j  = (i + 1) % poly.size();
        const Point3f p0 = makeBase(poly[i].x(), poly[i].y());
        const Point3f p1 = makeBase(poly[j].x(), poly[j].y());
        const Point3f p2 = makeTop(poly[j].x(), poly[j].y());
        const Point3f p3 = makeTop(poly[i].x(), poly[i].y());
        if (axis == 1) {
            // Y軸: XZ断面でCCWは法線が-Y方向になるため、頂点順を逆にして外向き法線を得る
            // 頂点順: q0=p1, q1=p0, q2=p3, q3=p2 → 法線 = (q1-q0)×(q2-q0) = (p0-p1)×(p3-p1)
            const Point3f n = ((p0 - p1) ^ (p3 - p1)).normalized();
            addQuadN(p1, n, p0, n, p3, n, p2, n);
        }
        else {
            const Point3f n = ((p1 - p0) ^ (p2 - p0)).normalized();
            addQuadN(p0, n, p1, n, p2, n, p3, n);
        }
    }

    // 蓋 (earcut による三角形分割)
    using ECPolygon  = std::vector<std::array<float, 2>>;
    using ECPolygons = std::vector<ECPolygon>;
    ECPolygons polygons(1);
    polygons[0].reserve(poly.size());
    std::vector<Point3f> cap_base, cap_top;
    cap_base.reserve(poly.size());
    cap_top.reserve(poly.size());
    for (const auto& p : poly) {
        polygons[0].push_back({p.x(), p.y()});
        cap_base.emplace_back(makeBase(p.x(), p.y()));
        cap_top.emplace_back(makeTop(p.x(), p.y()));
    }
    const std::vector<unsigned int> ear = mapbox::earcut<unsigned int>(polygons);

    Point3f norm_base, norm_top;
    if (axis == 0) {
        norm_base = {-1.0f, 0.0f, 0.0f};
        norm_top  = {1.0f, 0.0f, 0.0f};
    }
    else if (axis == 1) {
        norm_base = {0.0f, -1.0f, 0.0f};
        norm_top  = {0.0f, 1.0f, 0.0f};
    }
    else {
        norm_base = {0.0f, 0.0f, -1.0f};
        norm_top  = {0.0f, 0.0f, 1.0f};
    }

    for (size_t k = 0; k + 2 < ear.size(); k += 3) {
        const unsigned int i0 = ear[k + 0], i1 = ear[k + 1], i2 = ear[k + 2];
        if (axis == 1) {
            // Y軸: earcutのCCW三角形はXZ断面で法線-Y → 底面(y=0)は同順(外向き-Y), 頂面(y=h)は逆順(外向き+Y)
            addTriFlat(cap_base[i0], cap_base[i1], cap_base[i2], norm_base);
            addTriFlat(cap_top[i2], cap_top[i1], cap_top[i0], norm_top);
        }
        else {
            // X軸: CCW in YZ → 法線+X; Z軸: CCW in XY → 法線+Z
            // 底面(x=0 or z=0): 逆順(外向き-X or -Z), 頂面: 同順(外向き+X or +Z)
            addTriFlat(cap_base[i2], cap_base[i1], cap_base[i0], norm_base);
            addTriFlat(cap_top[i0], cap_top[i1], cap_top[i2], norm_top);
        }
    }

    // セクションライン
    if (m_create_section_line) {
        for (size_t i = 0; i < poly.size(); i++) {
            const size_t side_base = i * 4;
            m_segments_indices.emplace_back((unsigned int)(side_base + 0));
            m_segments_indices.emplace_back((unsigned int)(side_base + 1));
            m_segments_indices.emplace_back((unsigned int)(side_base + 3));
            m_segments_indices.emplace_back((unsigned int)(side_base + 2));
            m_segments_indices.emplace_back((unsigned int)(side_base + 0));
            m_segments_indices.emplace_back((unsigned int)(side_base + 3));
        }
    }
}

}    // namespace

void RenderNormalMesh::createPolygonPrismX(const VecPoint2f& vertices, float height)
{
    buildPolygonPrism(m_vertices, m_indices, m_segments_indices, m_create_section_line, vertices, height,
                      0);
    markRenderDirty();
    markBoxDirty();
}

void RenderNormalMesh::createPolygonPrismY(const VecPoint2f& vertices, float height)
{
    buildPolygonPrism(m_vertices, m_indices, m_segments_indices, m_create_section_line, vertices, height,
                      1);
    markRenderDirty();
    markBoxDirty();
}

void RenderNormalMesh::createPolygonPrismZ(const VecPoint2f& vertices, float height)
{
    buildPolygonPrism(m_vertices, m_indices, m_segments_indices, m_create_section_line, vertices, height,
                      2);
    markRenderDirty();
    markBoxDirty();
}

void RenderNormalMesh::createAperture(float outer_xlen, float outer_ylen, float z_len, float ap_x_offset,
                                      float ap_y_offset, float ap_xlen, float ap_ylen, int round_state, float radius,
                                      float ratio_x, float ratio_y, int taper_state, float taper_dist, float tol)
{
    m_vertices.clear();
    m_indices.clear();
    m_segments_indices.clear();

    if (outer_xlen <= 0.0f || outer_ylen <= 0.0f || z_len <= 0.0f || ap_xlen <= 0.0f || ap_ylen <= 0.0f) {
        markRenderDirty();
        markBoxDirty();
        return;
    }

    /// ---- opening bounds ----
    const float eps = 1.0e-6f;
    float       ix0 = std::clamp(ap_x_offset, 0.0f, outer_xlen - eps);
    float       iy0 = std::clamp(ap_y_offset, 0.0f, outer_ylen - eps);
    float       ix1 = std::clamp(ap_x_offset + ap_xlen, ix0 + eps, outer_xlen);
    float       iy1 = std::clamp(ap_y_offset + ap_ylen, iy0 + eps, outer_ylen);

    /// taper: opening at z=z_len may be narrower/wider
    float ixt0 = ix0, iyt0 = iy0, ixt1 = ix1, iyt1 = iy1;
    if (taper_state != 0 && std::fabs(taper_dist) > 0.0f) {
        ixt0 += taper_dist;
        iyt0 += taper_dist;
        ixt1 -= taper_dist;
        iyt1 -= taper_dist;
        ixt0 = std::clamp(ixt0, 0.0f, outer_xlen - eps);
        iyt0 = std::clamp(iyt0, 0.0f, outer_ylen - eps);
        ixt1 = std::clamp(ixt1, ixt0 + eps, outer_xlen);
        iyt1 = std::clamp(iyt1, iyt0 + eps, outer_ylen);
    }

    /// ---- vertex helpers ----
    auto addQuadFlatAuto = [&](const Point3f& p0, const Point3f& p1, const Point3f& p2, const Point3f& p3) {
        const Point3f      n    = ((p1 - p0) ^ (p2 - p0)).normalized();
        const unsigned int base = (unsigned int)m_vertices.size();
        m_vertices.emplace_back(p0, n);
        m_vertices.emplace_back(p1, n);
        m_vertices.emplace_back(p2, n);
        m_vertices.emplace_back(p3, n);
        m_indices.emplace_back(base + 0);
        m_indices.emplace_back(base + 1);
        m_indices.emplace_back(base + 2);
        m_indices.emplace_back(base + 2);
        m_indices.emplace_back(base + 3);
        m_indices.emplace_back(base + 0);
    };
    auto addTriFlat = [&](const Point3f& a, const Point3f& b, const Point3f& c, const Point3f& n) {
        const unsigned int base = (unsigned int)m_vertices.size();
        m_vertices.emplace_back(a, n);
        m_vertices.emplace_back(b, n);
        m_vertices.emplace_back(c, n);
        m_indices.emplace_back(base + 0);
        m_indices.emplace_back(base + 1);
        m_indices.emplace_back(base + 2);
    };

    /// ---- outer walls (always sharp rectangles) ----
    std::array<Point3f, 4> ob{
        Point3f{0.0f,       0.0f,       0.0f},
        Point3f{outer_xlen, 0.0f,       0.0f},
        Point3f{outer_xlen, outer_ylen, 0.0f},
        Point3f{0.0f,       outer_ylen, 0.0f}
    };
    std::array<Point3f, 4> ot{
        Point3f{0.0f,       0.0f,       z_len},
        Point3f{outer_xlen, 0.0f,       z_len},
        Point3f{outer_xlen, outer_ylen, z_len},
        Point3f{0.0f,       outer_ylen, z_len}
    };
    for (int i = 0; i < 4; i++) {
        const int j = (i + 1) % 4;
        addQuadFlatAuto(ob[i], ob[j], ot[j], ot[i]);
    }
    /// outer walls: 4 quads × 4 vertices = 16 vertices starting at index 0.

    /// ---- rounding parameters ----
    /// EyerisProject convention (same as createBoxRound):
    ///   rx = radius * ratio_x  ry = radius * ratio_y
    ///   clamp to half the opening dimensions at each face independently.
    float      rx_b = 0.0f, ry_b = 0.0f, rx_t = 0.0f, ry_t = 0.0f;
    int        segs     = 1;
    const bool do_round = (round_state != 0 && radius > 0.0f && ratio_x > 0.0f && ratio_y > 0.0f);
    if (do_round) {
        rx_b = radius * ratio_x;
        ry_b = radius * ratio_y;
        rx_b = std::min(rx_b, (ix1 - ix0) * 0.5f);
        ry_b = std::min(ry_b, (iy1 - iy0) * 0.5f);
        rx_t = std::min(rx_b, (ixt1 - ixt0) * 0.5f);
        ry_t = std::min(ry_b, (iyt1 - iyt0) * 0.5f);
        /// Arc segment count from tolerance (same formula as createBoxRound):
        ///   chord error = r*(1 - cos(π/N)) ≤ tol  →  N ≥ π / sqrt(2*tol/r)
        const float r_ref = std::max(rx_b, ry_b);
        if (tol > 0.0f && r_ref > 0.0f) {
            segs = std::max(segs, (int)std::ceil((float)M_PI / std::sqrt(2.0f * tol / r_ref)));
        }
    }

    using ECPolygon  = std::vector<std::array<float, 2>>;
    using ECPolygons = std::vector<ECPolygon>;

    /// Build a CCW rounded-rectangle 2D contour.
    /// Generates 4*(segs+1) points: 4 arc corners (BR→TR→TL→BL) each with segs+1 arc samples.
    /// Straight sections between arcs are the segments connecting adjacent arc endpoints.
    /// When rx==ry==0 and segs==1, degenerates to the 4-point sharp rectangle.
    auto makeCCWContour = [&](float x0, float y0, float x1, float y1, float rx, float ry) -> ECPolygon {
        ECPolygon   pts;
        const float step = (float)M_PI_2 / segs;
        struct ArcInfo {
            float cx, cy, a0;
        };
        const ArcInfo arcs[4] = {
            {x1 - rx, y0 + ry, -(float)M_PI_2}, /// BR: −90° → 0°
            {x1 - rx, y1 - ry, 0.0f          }, /// TR:   0° → 90°
            {x0 + rx, y1 - ry, (float)M_PI_2 }, /// TL:  90° → 180°
            {x0 + rx, y0 + ry, (float)M_PI   }, /// BL: 180° → 270°
        };
        for (const auto& a : arcs) {
            for (int k = 0; k <= segs; k++) {
                const float angle = a.a0 + k * step;
                pts.push_back({a.cx + rx * std::cos(angle), a.cy + ry * std::sin(angle)});
            }
        }
        return pts;    /// 4*(segs+1) points
    };

    /// Build the inner-opening contour.
    /// CW winding (= reversed CCW) is needed so that:
    ///   - addQuadFlatAuto(bot[i], bot[j], top[j], top[i]) gives inward-facing normals.
    ///   - earcut treats it as a CW hole polygon (earcut: outer=CCW, holes=CW).
    auto makeInnerCW = [&](float x0, float y0, float x1, float y1, float rx, float ry) -> ECPolygon {
        auto ccw = makeCCWContour(x0, y0, x1, y1, rx, ry);
        return ECPolygon(ccw.rbegin(), ccw.rend());
    };

    /// When do_round is false, use the same 4-point sharp rectangle (CW) to avoid
    /// degenerate quads from zero-radius arc collapse.
    ECPolygon inner_bot_cw, inner_top_cw;
    if (do_round) {
        inner_bot_cw = makeInnerCW(ix0, iy0, ix1, iy1, rx_b, ry_b);
        inner_top_cw = makeInnerCW(ixt0, iyt0, ixt1, iyt1, rx_t, ry_t);
    }
    else {
        inner_bot_cw = {
            {ix0, iy0},
            {ix0, iy1},
            {ix1, iy1},
            {ix1, iy0}
        };
        inner_top_cw = {
            {ixt0, iyt0},
            {ixt0, iyt1},
            {ixt1, iyt1},
            {ixt1, iyt0}
        };
    }
    const size_t nInner = inner_bot_cw.size();    /// 4 (sharp) or 4*(segs+1) (round)

    /// ---- inner walls ----
    /// Each step around the CW contour (bot[i]→bot[j], top[j]→top[i]) forms one quad
    /// with inward-facing auto-computed normal.
    const size_t inner_wall_vtx_start = m_vertices.size();    /// = 16
    for (size_t i = 0; i < nInner; i++) {
        const size_t j = (i + 1) % nInner;
        addQuadFlatAuto({inner_bot_cw[i][0], inner_bot_cw[i][1], 0.0f}, {inner_bot_cw[j][0], inner_bot_cw[j][1], 0.0f},
                        {inner_top_cw[j][0], inner_top_cw[j][1], z_len},
                        {inner_top_cw[i][0], inner_top_cw[i][1], z_len});
    }

    /// ---- cap faces (earcut: outer CCW, hole CW) ----
    ECPolygons bottom_polys(2), top_polys(2);
    bottom_polys[0] = {
        {0.0f,       0.0f      },
        {outer_xlen, 0.0f      },
        {outer_xlen, outer_ylen},
        {0.0f,       outer_ylen}
    };
    bottom_polys[1] = inner_bot_cw;
    top_polys[0]    = {
        {0.0f,       0.0f      },
        {outer_xlen, 0.0f      },
        {outer_xlen, outer_ylen},
        {0.0f,       outer_ylen}
    };
    top_polys[1] = inner_top_cw;

    /// earcut indices map into the flattened ring list: outer 4 pts first, then hole nInner pts.
    std::vector<Point3f> bpoints, tpoints;
    bpoints.reserve(4 + nInner);
    tpoints.reserve(4 + nInner);
    bpoints.insert(
        bpoints.end(),
        {
            {0.0f,       0.0f,       0.0f},
            {outer_xlen, 0.0f,       0.0f},
            {outer_xlen, outer_ylen, 0.0f},
            {0.0f,       outer_ylen, 0.0f}
    });
    tpoints.insert(tpoints.end(), {
                                      {0.0f,       0.0f,       z_len},
                                      {outer_xlen, 0.0f,       z_len},
                                      {outer_xlen, outer_ylen, z_len},
                                      {0.0f,       outer_ylen, z_len}
    });
    for (const auto& p : inner_bot_cw)
        bpoints.push_back({p[0], p[1], 0.0f});
    for (const auto& p : inner_top_cw)
        tpoints.push_back({p[0], p[1], z_len});

    const auto ear_bottom = mapbox::earcut<unsigned int>(bottom_polys);
    for (size_t k = 0; k + 2 < ear_bottom.size(); k += 3) {
        const unsigned int i0 = ear_bottom[k], i1 = ear_bottom[k + 1], i2 = ear_bottom[k + 2];
        addTriFlat(bpoints[i2], bpoints[i1], bpoints[i0], Point3f{0.0f, 0.0f, -1.0f});
    }
    const auto ear_top = mapbox::earcut<unsigned int>(top_polys);
    for (size_t k = 0; k + 2 < ear_top.size(); k += 3) {
        const unsigned int i0 = ear_top[k], i1 = ear_top[k + 1], i2 = ear_top[k + 2];
        addTriFlat(tpoints[i0], tpoints[i1], tpoints[i2], Point3f{0.0f, 0.0f, 1.0f});
    }

    /// ---- wireframe ----
    if (m_create_section_line) {
        /// outer walls: 4 quads at vertex base 0
        for (int i = 0; i < 4; i++) {
            const size_t obase = (size_t)i * 4;
            m_segments_indices.emplace_back((unsigned int)(obase + 0));
            m_segments_indices.emplace_back((unsigned int)(obase + 1));
            m_segments_indices.emplace_back((unsigned int)(obase + 3));
            m_segments_indices.emplace_back((unsigned int)(obase + 2));
            m_segments_indices.emplace_back((unsigned int)(obase + 0));
            m_segments_indices.emplace_back((unsigned int)(obase + 3));
        }
        /// inner walls: nInner quads starting at inner_wall_vtx_start.
        /// Trace bottom contour, top contour, and one vertical per corner transition.
        for (size_t i = 0; i < nInner; i++) {
            const size_t ibase = inner_wall_vtx_start + i * 4;
            m_segments_indices.emplace_back((unsigned int)(ibase + 0));    /// bot[i]
            m_segments_indices.emplace_back((unsigned int)(ibase + 1));    /// bot[j]
            m_segments_indices.emplace_back((unsigned int)(ibase + 3));    /// top[i]
            m_segments_indices.emplace_back((unsigned int)(ibase + 2));    /// top[j]
        }
        /// vertical edges at the 4 arc→straight transitions.
        /// corner_step = nInner/4 (= 1 for sharp, = segs+1 for round).
        const size_t corner_step = nInner / 4;
        for (int corner = 0; corner < 4; corner++) {
            const size_t ibase = inner_wall_vtx_start + (size_t)corner * corner_step * 4;
            m_segments_indices.emplace_back((unsigned int)(ibase + 0));    /// bot
            m_segments_indices.emplace_back((unsigned int)(ibase + 3));    /// top
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

RenderEditableNormalMesh::RenderEditableNormalMesh(RenderNormalMesh* mesh) : RenderableNode(mesh) {}

RenderEditableNormalMesh::~RenderEditableNormalMesh() {}

RenderEditableNormalMesh::RenderEditableNormalMesh(const RenderEditableNormalMesh& other) : RenderableNode(other)
{
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
