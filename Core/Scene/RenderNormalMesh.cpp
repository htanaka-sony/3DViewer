#include "RenderNormalMesh.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include "earcut.hpp"

#include "Math/Point2f.h"

#undef min
#undef max

CORE_NAMESPACE_BEGIN

DEFINE_META_RENDERABLE(RenderNormalMesh)

RenderNormalMesh::RenderNormalMesh() {}

RenderNormalMesh::~RenderNormalMesh() {}

RenderNormalMesh::RenderNormalMesh(const RenderNormalMesh& other) : RenderableObject(other)
{
    m_vertices            = other.m_vertices;
    m_indices             = other.m_indices;
    m_segments_indices    = other.m_segments_indices;
    m_create_section_line = other.m_create_section_line;
}

void RenderNormalMesh::setFromEditable(const RenderEditableNormalMesh& other)
{
    m_vertices = other.isEnableEditDisplayData() ? other.displayEditVertices() : other.displayVertices();
    m_indices  = other.isEnableEditDisplayData() ? other.displayEditIndices() : other.displayIndices();
    m_segments_indices =
        other.isEnableEditDisplayData() ? other.displayEditSegmentIndices() : other.displaySegmentIndices();
    m_create_section_line = other.originalMesh()->isCreateSectionLine();

    setColor(other.color());
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

constexpr float kPolygonEps                   = 1.0e-6f;
constexpr int   kPairGridThreshold            = 96;
constexpr int   kPairGridCellSpanSafety       = 64;
constexpr int   kCollinearFilterMaxIterations = 8;

struct Segment2D {
    Point2f            p0;
    Point2f            p1;
    std::vector<float> ts;
};

struct SplitEdge {
    int v0;
    int v1;
};

struct DirectedEdge {
    int from = -1;
    int to   = -1;
    int twin = -1;
    int next = -1;
    int face = -1;    // left face
};

struct Face2D {
    std::vector<int> edges;
    std::vector<int> vertices;
    float            area   = 0.0f;
    int              parity = -1;
};

static inline float cross2d(const Point2f& a, const Point2f& b)
{
    return a.x() * b.y() - a.y() * b.x();
}

static inline Point2f sub2d(const Point2f& a, const Point2f& b)
{
    return {a.x() - b.x(), a.y() - b.y()};
}

static inline float signedArea(const std::vector<Point2f>& poly)
{
    float area = 0.0f;
    for (size_t i = 0; i < poly.size(); i++) {
        const size_t j = (i + 1) % poly.size();
        area += poly[i].x() * poly[j].y() - poly[j].x() * poly[i].y();
    }
    return area * 0.5f;
}

static inline bool nearEq(float a, float b, float eps)
{
    return std::fabs(a - b) <= eps;
}

static inline bool pointNear(const Point2f& a, const Point2f& b, float eps)
{
    return nearEq(a.x(), b.x(), eps) && nearEq(a.y(), b.y(), eps);
}

static std::vector<Point2f> normalizeStrokeLoop(const VecPoint2f& vertices, float eps)
{
    std::vector<Point2f> poly;
    poly.reserve(vertices.size());
    for (const auto& p : vertices) {
        if (poly.empty() || !pointNear(poly.back(), p, eps)) {
            poly.emplace_back(p);
        }
    }
    if (poly.size() >= 2 && pointNear(poly.front(), poly.back(), eps)) {
        poly.pop_back();
    }
    if (poly.size() < 3) {
        return {};
    }

    bool changed = true;
    int  guard   = 0;
    while (changed && poly.size() >= 3 && guard < kCollinearFilterMaxIterations) {
        changed = false;
        guard++;
        std::vector<Point2f> filtered;
        filtered.reserve(poly.size());
        const size_t n = poly.size();
        for (size_t i = 0; i < n; i++) {
            const Point2f& a      = poly[(i + n - 1) % n];
            const Point2f& b      = poly[i];
            const Point2f& c      = poly[(i + 1) % n];
            const Point2f  ab     = sub2d(b, a);
            const Point2f  bc     = sub2d(c, b);
            const float    len_ab = std::sqrt(ab.x() * ab.x() + ab.y() * ab.y());
            const float    len_bc = std::sqrt(bc.x() * bc.x() + bc.y() * bc.y());
            const float    denom  = std::max(len_ab + len_bc, eps);
            const float    area2  = std::fabs(cross2d(ab, bc));
            if (area2 <= eps * denom) {
                changed = true;
                continue;
            }
            filtered.emplace_back(b);
        }
        poly.swap(filtered);
    }

    if (poly.size() < 3 || std::fabs(signedArea(poly)) < eps) {
        return {};
    }
    return poly;
}

static bool intersectSegments(const Point2f& a, const Point2f& b, const Point2f& c, const Point2f& d, float eps,
                              float& t_out, float& u_out, Point2f& p_out)
{
    const Point2f r   = sub2d(b, a);
    const Point2f s   = sub2d(d, c);
    const Point2f w   = sub2d(c, a);
    const float   den = cross2d(r, s);

    if (std::fabs(den) <= eps) {
        // parallel/collinear: overlap case is treated as "intersecting" for fast-path rejection
        if (std::fabs(cross2d(w, r)) <= eps) {
            const float minax   = std::min(a.x(), b.x()) - eps;
            const float maxax   = std::max(a.x(), b.x()) + eps;
            const float minay   = std::min(a.y(), b.y()) - eps;
            const float maxay   = std::max(a.y(), b.y()) + eps;
            const float mincx   = std::min(c.x(), d.x()) - eps;
            const float maxcx   = std::max(c.x(), d.x()) + eps;
            const float mincy   = std::min(c.y(), d.y()) - eps;
            const float maxcy   = std::max(c.y(), d.y()) + eps;
            const bool  overlap = (std::max(minax, mincx) <= std::min(maxax, maxcx))
                              && (std::max(minay, mincy) <= std::min(maxay, maxcy));
            if (overlap) {
                t_out = 0.0f;
                u_out = 0.0f;
                p_out = a;
                return true;
            }
        }
        return false;
    }

    const float t = cross2d(w, s) / den;
    const float u = cross2d(w, r) / den;
    if (t < -eps || t > 1.0f + eps || u < -eps || u > 1.0f + eps) {
        return false;
    }

    const float tc = std::clamp(t, 0.0f, 1.0f);
    const float uc = std::clamp(u, 0.0f, 1.0f);
    t_out          = tc;
    u_out          = uc;
    p_out          = {a.x() + (b.x() - a.x()) * tc, a.y() + (b.y() - a.y()) * tc};
    return true;
}

static inline bool isAdjacent(size_t i, size_t j, size_t n)
{
    if (i == j) {
        return true;
    }
    if ((i + 1) % n == j || (j + 1) % n == i) {
        return true;
    }
    return false;
}

static uint64_t pairKey32(uint32_t a, uint32_t b)
{
    const uint32_t lo = std::min(a, b);
    const uint32_t hi = std::max(a, b);
    return (uint64_t)hi << 32 | (uint64_t)lo;
}

static std::vector<std::pair<int, int>> enumerateCandidatePairs(const std::vector<Point2f>& poly)
{
    const size_t                     n = poly.size();
    std::vector<std::pair<int, int>> out;
    if (n < 4) {
        return out;
    }

    if ((int)n <= kPairGridThreshold) {
        out.reserve(n * 3);
        for (size_t i = 0; i < n; i++) {
            for (size_t j = i + 1; j < n; j++) {
                if (isAdjacent(i, j, n)) {
                    continue;
                }
                out.emplace_back((int)i, (int)j);
            }
        }
        return out;
    }

    float minx    = std::numeric_limits<float>::max();
    float miny    = std::numeric_limits<float>::max();
    float maxx    = -std::numeric_limits<float>::max();
    float maxy    = -std::numeric_limits<float>::max();
    float sum_len = 0.0f;
    for (size_t i = 0; i < n; i++) {
        const Point2f& p0 = poly[i];
        const Point2f& p1 = poly[(i + 1) % n];
        minx              = std::min({minx, p0.x(), p1.x()});
        miny              = std::min({miny, p0.y(), p1.y()});
        maxx              = std::max({maxx, p0.x(), p1.x()});
        maxy              = std::max({maxy, p0.y(), p1.y()});
        const float dx    = p1.x() - p0.x();
        const float dy    = p1.y() - p0.y();
        sum_len += std::sqrt(dx * dx + dy * dy);
    }
    const float avg_len  = std::max(sum_len / (float)n, kPolygonEps * 16.0f);
    const float cellSize = avg_len;

    auto cellKey = [](int ix, int iy) -> uint64_t { return (uint64_t)(uint32_t)ix << 32 | (uint32_t)iy; };

    std::unordered_map<uint64_t, std::vector<int>> buckets;
    buckets.reserve(n * 2);
    std::unordered_set<uint64_t> pair_set;
    pair_set.reserve(n * 8);

    for (size_t i = 0; i < n; i++) {
        const Point2f& a    = poly[i];
        const Point2f& b    = poly[(i + 1) % n];
        const float    xmin = std::min(a.x(), b.x());
        const float    xmax = std::max(a.x(), b.x());
        const float    ymin = std::min(a.y(), b.y());
        const float    ymax = std::max(a.y(), b.y());
        int            ix0  = (int)std::floor((xmin - minx) / cellSize);
        int            ix1  = (int)std::floor((xmax - minx) / cellSize);
        int            iy0  = (int)std::floor((ymin - miny) / cellSize);
        int            iy1  = (int)std::floor((ymax - miny) / cellSize);

        if (ix1 - ix0 > kPairGridCellSpanSafety || iy1 - iy0 > kPairGridCellSpanSafety) {
            for (size_t j = 0; j < i; j++) {
                if (isAdjacent(i, j, n)) {
                    continue;
                }
                pair_set.insert(pairKey32((uint32_t)i, (uint32_t)j));
            }
            continue;
        }

        for (int ix = ix0; ix <= ix1; ix++) {
            for (int iy = iy0; iy <= iy1; iy++) {
                auto& cell = buckets[cellKey(ix, iy)];
                for (int j : cell) {
                    if (isAdjacent(i, (size_t)j, n)) {
                        continue;
                    }
                    pair_set.insert(pairKey32((uint32_t)i, (uint32_t)j));
                }
                cell.emplace_back((int)i);
            }
        }
    }

    out.reserve(pair_set.size());
    for (uint64_t key : pair_set) {
        out.emplace_back((int)(key & 0xffffffffu), (int)(key >> 32));
    }
    return out;
}

static bool hasSelfIntersection(const std::vector<Point2f>& poly, float eps)
{
    const auto pairs = enumerateCandidatePairs(poly);
    for (const auto& [i, j] : pairs) {
        float   t = 0.0f, u = 0.0f;
        Point2f p;
        if (intersectSegments(poly[i], poly[(i + 1) % poly.size()], poly[j], poly[(j + 1) % poly.size()], eps, t, u,
                              p)) {
            return true;
        }
    }
    return false;
}

static bool appendUniqueParam(std::vector<float>& ts, float t, float eps)
{
    for (float v : ts) {
        if (std::fabs(v - t) <= eps) {
            return false;
        }
    }
    ts.emplace_back(t);
    return true;
}

static bool buildPlanarSplitGraph(const std::vector<Point2f>& poly, float eps, std::vector<Point2f>& out_vertices,
                                  std::vector<SplitEdge>& out_edges)
{
    const size_t n = poly.size();
    if (n < 3) {
        return false;
    }

    std::vector<Segment2D> segs(n);
    for (size_t i = 0; i < n; i++) {
        segs[i].p0 = poly[i];
        segs[i].p1 = poly[(i + 1) % n];
        segs[i].ts = {0.0f, 1.0f};
    }

    const auto pairs = enumerateCandidatePairs(poly);
    for (const auto& [i, j] : pairs) {
        float   t = 0.0f, u = 0.0f;
        Point2f p;
        if (!intersectSegments(segs[i].p0, segs[i].p1, segs[j].p0, segs[j].p1, eps, t, u, p)) {
            continue;
        }
        appendUniqueParam(segs[i].ts, std::clamp(t, 0.0f, 1.0f), eps);
        appendUniqueParam(segs[j].ts, std::clamp(u, 0.0f, 1.0f), eps);
    }

    struct Key {
        int64_t x = 0;
        int64_t y = 0;
        bool    operator==(const Key& other) const { return x == other.x && y == other.y; }
    };
    struct KeyHash {
        size_t operator()(const Key& k) const
        {
            return std::hash<uint64_t>{}((uint64_t)k.x * 1315423911ull ^ (uint64_t)k.y * 2654435761ull);
        }
    };

    auto makeKey = [eps](const Point2f& p) -> Key {
        return {std::llround((double)p.x() / (double)eps), std::llround((double)p.y() / (double)eps)};
    };

    std::unordered_map<Key, int, KeyHash> vertex_map;
    vertex_map.reserve(n * 3);

    auto vertexId = [&](const Point2f& p) -> int {
        Key  key = makeKey(p);
        auto it  = vertex_map.find(key);
        if (it != vertex_map.end()) {
            return it->second;
        }
        const int id = (int)out_vertices.size();
        out_vertices.emplace_back(p);
        vertex_map.emplace(key, id);
        return id;
    };

    std::unordered_set<uint64_t> edge_set;
    edge_set.reserve(n * 4);

    auto addEdge = [&](int a, int b) {
        if (a == b) {
            return;
        }
        uint32_t lo  = (uint32_t)std::min(a, b);
        uint32_t hi  = (uint32_t)std::max(a, b);
        uint64_t key = ((uint64_t)hi << 32) | (uint64_t)lo;
        if (!edge_set.insert(key).second) {
            return;
        }
        out_edges.push_back({a, b});
    };

    const float split_eps = std::max(eps * 0.5f, 1.0e-7f);
    for (auto& seg : segs) {
        auto& ts = seg.ts;
        std::sort(ts.begin(), ts.end());
        ts.erase(
            std::unique(ts.begin(), ts.end(), [split_eps](float a, float b) { return std::fabs(a - b) <= split_eps; }),
            ts.end());
        for (size_t k = 0; k + 1 < ts.size(); k++) {
            const float t0 = ts[k];
            const float t1 = ts[k + 1];
            if (t1 - t0 <= split_eps) {
                continue;
            }
            const Point2f p0 = {seg.p0.x() + (seg.p1.x() - seg.p0.x()) * t0,
                                seg.p0.y() + (seg.p1.y() - seg.p0.y()) * t0};
            const Point2f p1 = {seg.p0.x() + (seg.p1.x() - seg.p0.x()) * t1,
                                seg.p0.y() + (seg.p1.y() - seg.p0.y()) * t1};
            addEdge(vertexId(p0), vertexId(p1));
        }
    }

    return out_vertices.size() >= 3 && !out_edges.empty();
}

static bool buildFacesFromPlanarGraph(const std::vector<Point2f>& vtx2d, const std::vector<SplitEdge>& edges,
                                      std::vector<DirectedEdge>& out_dir_edges, std::vector<Face2D>& out_faces)
{
    if (vtx2d.size() < 3 || edges.empty()) {
        return false;
    }

    out_dir_edges.clear();
    out_faces.clear();
    out_dir_edges.reserve(edges.size() * 2);

    std::vector<std::vector<int>> outgoing(vtx2d.size());
    for (const auto& e : edges) {
        const int idx0 = (int)out_dir_edges.size();
        out_dir_edges.push_back({e.v0, e.v1, idx0 + 1, -1, -1});
        out_dir_edges.push_back({e.v1, e.v0, idx0 + 0, -1, -1});
        outgoing[e.v0].push_back(idx0 + 0);
        outgoing[e.v1].push_back(idx0 + 1);
    }

    std::vector<int> edge_rank(out_dir_edges.size(), -1);
    for (size_t v = 0; v < outgoing.size(); v++) {
        auto& out = outgoing[v];
        std::sort(out.begin(), out.end(), [&](int lhs, int rhs) {
            const Point2f& p  = vtx2d[v];
            const Point2f& a  = vtx2d[out_dir_edges[lhs].to];
            const Point2f& b  = vtx2d[out_dir_edges[rhs].to];
            const float    aa = std::atan2(a.y() - p.y(), a.x() - p.x());
            const float    bb = std::atan2(b.y() - p.y(), b.x() - p.x());
            return aa < bb;
        });
        for (int i = 0; i < (int)out.size(); i++) {
            edge_rank[out[i]] = i;
        }
    }

    for (size_t e = 0; e < out_dir_edges.size(); e++) {
        const int twin = out_dir_edges[e].twin;
        const int v    = out_dir_edges[e].to;
        auto&     out  = outgoing[v];
        if (out.empty()) {
            continue;
        }
        const int rank = edge_rank[twin];
        if (rank < 0) {
            continue;
        }
        const int prev_rank   = (rank - 1 + (int)out.size()) % (int)out.size();
        out_dir_edges[e].next = out[prev_rank];
    }

    std::vector<char> visited(out_dir_edges.size(), 0);
    for (size_t start = 0; start < out_dir_edges.size(); start++) {
        if (visited[start] || out_dir_edges[start].next < 0) {
            continue;
        }
        std::vector<int> cycle_edges;
        std::vector<int> cycle_vertices;
        int              e     = (int)start;
        int              guard = 0;
        while (!visited[e] && guard < (int)out_dir_edges.size()) {
            visited[e] = 1;
            cycle_edges.push_back(e);
            cycle_vertices.push_back(out_dir_edges[e].from);
            e = out_dir_edges[e].next;
            if (e == (int)start) {
                break;
            }
            guard++;
        }
        if (cycle_edges.size() < 3 || e != (int)start) {
            continue;
        }

        float area = 0.0f;
        for (size_t i = 0; i < cycle_vertices.size(); i++) {
            const Point2f& p0 = vtx2d[cycle_vertices[i]];
            const Point2f& p1 = vtx2d[cycle_vertices[(i + 1) % cycle_vertices.size()]];
            area += p0.x() * p1.y() - p1.x() * p0.y();
        }
        area *= 0.5f;
        if (std::fabs(area) <= kPolygonEps) {
            continue;
        }

        const int fid = (int)out_faces.size();
        out_faces.push_back({std::move(cycle_edges), std::move(cycle_vertices), area, -1});
        for (int edge_id : out_faces.back().edges) {
            out_dir_edges[edge_id].face = fid;
        }
    }

    return !out_faces.empty();
}

static void computeParity(std::vector<DirectedEdge>& dir_edges, std::vector<Face2D>& faces)
{
    if (faces.empty()) {
        return;
    }

    int   outside  = 0;
    float min_area = std::numeric_limits<float>::max();
    for (size_t i = 0; i < faces.size(); i++) {
        if (faces[i].area < min_area) {
            min_area = faces[i].area;
            outside  = (int)i;
        }
    }

    std::vector<std::vector<int>> graph(faces.size());
    for (size_t e = 0; e + 1 < dir_edges.size(); e += 2) {
        const int f0 = dir_edges[e].face;
        const int f1 = dir_edges[e + 1].face;
        if (f0 < 0 || f1 < 0 || f0 == f1) {
            continue;
        }
        graph[f0].push_back(f1);
        graph[f1].push_back(f0);
    }

    std::queue<int> q;
    faces[outside].parity = 0;
    q.push(outside);
    while (!q.empty()) {
        const int f = q.front();
        q.pop();
        for (int nb : graph[f]) {
            if (faces[nb].parity >= 0) {
                continue;
            }
            faces[nb].parity = faces[f].parity ^ 1;
            q.push(nb);
        }
    }
    for (auto& f : faces) {
        if (f.parity < 0) {
            f.parity = 0;
        }
    }
}

static void appendSideQuad(std::vector<Vertexf>& m_vertices, std::vector<unsigned int>& m_indices,
                           std::vector<unsigned int>& m_segments_indices, bool m_create_section_line, const Point3f& p0,
                           const Point3f& p1, const Point3f& p2, const Point3f& p3, int axis)
{
    auto addQuadN = [&](const Point3f& q0, const Point3f& n0, const Point3f& q1, const Point3f& n1, const Point3f& q2,
                        const Point3f& n2, const Point3f& q3, const Point3f& n3) {
        const unsigned int base = (unsigned int)m_vertices.size();
        m_vertices.emplace_back(q0, n0);
        m_vertices.emplace_back(q1, n1);
        m_vertices.emplace_back(q2, n2);
        m_vertices.emplace_back(q3, n3);
        m_indices.emplace_back(base + 0);
        m_indices.emplace_back(base + 1);
        m_indices.emplace_back(base + 2);
        m_indices.emplace_back(base + 2);
        m_indices.emplace_back(base + 3);
        m_indices.emplace_back(base + 0);
        if (m_create_section_line) {
            m_segments_indices.emplace_back(base + 0);
            m_segments_indices.emplace_back(base + 1);
            m_segments_indices.emplace_back(base + 3);
            m_segments_indices.emplace_back(base + 2);
            m_segments_indices.emplace_back(base + 0);
            m_segments_indices.emplace_back(base + 3);
        }
    };

    if (axis == 1) {
        const Point3f n = ((p0 - p1) ^ (p3 - p1)).normalized();
        addQuadN(p1, n, p0, n, p3, n, p2, n);
    }
    else {
        const Point3f n = ((p1 - p0) ^ (p2 - p0)).normalized();
        addQuadN(p0, n, p1, n, p2, n, p3, n);
    }
}

static void appendCapTri(std::vector<Vertexf>& m_vertices, std::vector<unsigned int>& m_indices, const Point3f& a,
                         const Point3f& b, const Point3f& c, const Point3f& n)
{
    const unsigned int base = (unsigned int)m_vertices.size();
    m_vertices.emplace_back(a, n);
    m_vertices.emplace_back(b, n);
    m_vertices.emplace_back(c, n);
    m_indices.emplace_back(base + 0);
    m_indices.emplace_back(base + 1);
    m_indices.emplace_back(base + 2);
}

static void buildSimplePolygonPrism(std::vector<Vertexf>& m_vertices, std::vector<unsigned int>& m_indices,
                                    std::vector<unsigned int>& m_segments_indices, bool m_create_section_line,
                                    const std::vector<Point2f>& poly, float height, int axis)
{
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

    // 側面
    for (size_t i = 0; i < poly.size(); i++) {
        const size_t  j  = (i + 1) % poly.size();
        const Point3f p0 = makeBase(poly[i].x(), poly[i].y());
        const Point3f p1 = makeBase(poly[j].x(), poly[j].y());
        const Point3f p2 = makeTop(poly[j].x(), poly[j].y());
        const Point3f p3 = makeTop(poly[i].x(), poly[i].y());
        appendSideQuad(m_vertices, m_indices, m_segments_indices, m_create_section_line, p0, p1, p2, p3, axis);
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
            appendCapTri(m_vertices, m_indices, cap_base[i0], cap_base[i1], cap_base[i2], norm_base);
            appendCapTri(m_vertices, m_indices, cap_top[i2], cap_top[i1], cap_top[i0], norm_top);
        }
        else {
            appendCapTri(m_vertices, m_indices, cap_base[i2], cap_base[i1], cap_base[i0], norm_base);
            appendCapTri(m_vertices, m_indices, cap_top[i0], cap_top[i1], cap_top[i2], norm_top);
        }
    }
}

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

    std::vector<Point2f> poly = normalizeStrokeLoop(vertices, kPolygonEps);
    if (poly.size() < 3) {
        return;
    }

    float signed_area = signedArea(poly);
    if (signed_area < 0.0f) {
        std::reverse(poly.begin(), poly.end());
        signed_area = -signed_area;
    }
    if (signed_area < kPolygonEps) {
        return;
    }

    // Fast path: 単純ループは従来の earcut を直接使用
    if (!hasSelfIntersection(poly, kPolygonEps)) {
        buildSimplePolygonPrism(m_vertices, m_indices, m_segments_indices, m_create_section_line, poly, height, axis);
        return;
    }

    // General path: 交点分割 → 平面グラフ → face parity
    std::vector<Point2f>   graph_vertices;
    std::vector<SplitEdge> graph_edges;
    if (!buildPlanarSplitGraph(poly, kPolygonEps, graph_vertices, graph_edges)) {
        return;
    }

    std::vector<DirectedEdge> dir_edges;
    std::vector<Face2D>       faces;
    if (!buildFacesFromPlanarGraph(graph_vertices, graph_edges, dir_edges, faces)) {
        return;
    }
    computeParity(dir_edges, faces);

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

    // parity が異なる面を隔てる辺のみ側面化
    for (size_t e = 0; e + 1 < dir_edges.size(); e += 2) {
        const int f0 = dir_edges[e].face;
        const int f1 = dir_edges[e + 1].face;
        const int p0 = (f0 >= 0) ? faces[f0].parity : 0;
        const int p1 = (f1 >= 0) ? faces[f1].parity : 0;
        if (p0 == p1) {
            continue;
        }
        const int     edge_id = (p0 == 1) ? (int)e : (int)e + 1;
        const int     from    = dir_edges[edge_id].from;
        const int     to      = dir_edges[edge_id].to;
        const Point2f a2      = graph_vertices[from];
        const Point2f b2      = graph_vertices[to];
        const Point3f pbase0  = makeBase(a2.x(), a2.y());
        const Point3f pbase1  = makeBase(b2.x(), b2.y());
        const Point3f ptop1   = makeTop(b2.x(), b2.y());
        const Point3f ptop0   = makeTop(a2.x(), a2.y());
        appendSideQuad(m_vertices, m_indices, m_segments_indices, m_create_section_line, pbase0, pbase1, ptop1, ptop0,
                       axis);
    }

    // parity=1 face だけ蓋を生成
    using ECPolygon = std::vector<std::array<float, 2>>;
    for (const auto& face : faces) {
        if (face.parity != 1 || face.vertices.size() < 3) {
            continue;
        }

        std::vector<int> ring(face.vertices.begin(), face.vertices.end());
        if (face.area < 0.0f) {
            std::reverse(ring.begin(), ring.end());
        }
        if (ring.size() < 3) {
            continue;
        }

        std::vector<std::vector<std::array<float, 2>>> polygons(1);
        polygons[0].reserve(ring.size());
        std::vector<Point3f> cap_base, cap_top;
        cap_base.reserve(ring.size());
        cap_top.reserve(ring.size());
        for (int vid : ring) {
            const Point2f& p = graph_vertices[vid];
            polygons[0].push_back({p.x(), p.y()});
            cap_base.emplace_back(makeBase(p.x(), p.y()));
            cap_top.emplace_back(makeTop(p.x(), p.y()));
        }

        const std::vector<unsigned int> ear = mapbox::earcut<unsigned int>(polygons);
        for (size_t k = 0; k + 2 < ear.size(); k += 3) {
            const unsigned int i0 = ear[k + 0], i1 = ear[k + 1], i2 = ear[k + 2];
            if (axis == 1) {
                appendCapTri(m_vertices, m_indices, cap_base[i0], cap_base[i1], cap_base[i2], norm_base);
                appendCapTri(m_vertices, m_indices, cap_top[i2], cap_top[i1], cap_top[i0], norm_top);
            }
            else {
                appendCapTri(m_vertices, m_indices, cap_base[i2], cap_base[i1], cap_base[i0], norm_base);
                appendCapTri(m_vertices, m_indices, cap_top[i0], cap_top[i1], cap_top[i2], norm_top);
            }
        }
    }
}

}    // namespace

void RenderNormalMesh::createPolygonPrismX(const VecPoint2f& vertices, float height)
{
    buildPolygonPrism(m_vertices, m_indices, m_segments_indices, m_create_section_line, vertices, height, 0);
    markRenderDirty();
    markBoxDirty();
}

void RenderNormalMesh::createPolygonPrismY(const VecPoint2f& vertices, float height)
{
    buildPolygonPrism(m_vertices, m_indices, m_segments_indices, m_create_section_line, vertices, height, 1);
    markRenderDirty();
    markBoxDirty();
}

void RenderNormalMesh::createPolygonPrismZ(const VecPoint2f& vertices, float height)
{
    buildPolygonPrism(m_vertices, m_indices, m_segments_indices, m_create_section_line, vertices, height, 2);
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
