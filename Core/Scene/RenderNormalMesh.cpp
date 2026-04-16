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
                                      float tol, int wire_segs)
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
    auto addQuadN = [&](const Point3f& p0, const Point3f& n0, const Point3f& p1, const Point3f& n1,
                        const Point3f& p2, const Point3f& n2, const Point3f& p3, const Point3f& n3) {
        const unsigned int base = (unsigned int)m_vertices.size();
        m_vertices.emplace_back(p0, n0);
        m_vertices.emplace_back(p1, n1);
        m_vertices.emplace_back(p2, n2);
        m_vertices.emplace_back(p3, n3);
        m_indices.push_back(base + 0);
        m_indices.push_back(base + 1);
        m_indices.push_back(base + 2);
        m_indices.push_back(base + 2);
        m_indices.push_back(base + 3);
        m_indices.push_back(base + 0);
    };

    /// 全頂点に同一の法線を設定してquadを追加（平面フェース用）
    auto addQuadFlat = [&](const Point3f& p0, const Point3f& p1, const Point3f& p2, const Point3f& p3,
                           const Point3f& norm) {
        addQuadN(p0, norm, p1, norm, p2, norm, p3, norm);
    };

    /// コーナーあたりの弧分割数を許容誤差から計算 (chord error = r*(1-cos(π/N)) ≤ tol)
    const float r_ref = std::max(rx, std::max(ry, rz));
    int         segs  = 8;
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
        return Point3f(dx * std::sin(u) * std::cos(v) / rx, dy * std::sin(u) * std::sin(v) / ry,
                       dz * std::cos(u) / rz)
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
            const float cx  = x0 + (sx ? W - rx : rx);
            const float cy  = y0 + (sy ? H - ry : ry);
            const float dx  = sx ? +1.0f : -1.0f;
            const float dy  = sy ? +1.0f : -1.0f;
            const bool  flip = (dx * dy < 0.0f);
            for (int k = 0; k < segs; k++) {
                const float t0  = (float)k * (float)M_PI_2 / segs;
                const float t1  = (float)(k + 1) * (float)M_PI_2 / segs;
                const float c0  = std::cos(t0), s0 = std::sin(t0);
                const float c1  = std::cos(t1), s1 = std::sin(t1);
                const float px0 = cx + dx * rx * c0, py0 = cy + dy * ry * s0;
                const float px1 = cx + dx * rx * c1, py1 = cy + dy * ry * s1;
                const Point3f n0  = ellipseNormXY(c0, s0, dx, dy);
                const Point3f n1  = ellipseNormXY(c1, s1, dx, dy);
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
            const float cx  = x0 + (sx ? W - rx : rx);
            const float cz  = z0 + (sz ? D - rz : rz);
            const float dx  = sx ? +1.0f : -1.0f;
            const float dz  = sz ? +1.0f : -1.0f;
            const bool  flip = (dx * dz < 0.0f);
            for (int k = 0; k < segs; k++) {
                const float u0  = (float)k * (float)M_PI_2 / segs;
                const float u1  = (float)(k + 1) * (float)M_PI_2 / segs;
                const float c0  = std::cos(u0), s0 = std::sin(u0);
                const float c1  = std::cos(u1), s1 = std::sin(u1);
                const float px0 = cx + dx * rx * c0, pz0 = cz + dz * rz * s0;
                const float px1 = cx + dx * rx * c1, pz1 = cz + dz * rz * s1;
                const Point3f n0  = ellipseNormXZ(c0, s0, dx, dz);
                const Point3f n1  = ellipseNormXZ(c1, s1, dx, dz);
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
            const float cy  = y0 + (sy ? H - ry : ry);
            const float cz  = z0 + (sz ? D - rz : rz);
            const float dy  = sy ? +1.0f : -1.0f;
            const float dz  = sz ? +1.0f : -1.0f;
            const bool  flip = (dy * dz < 0.0f);
            for (int k = 0; k < segs; k++) {
                const float v0  = (float)k * (float)M_PI_2 / segs;
                const float v1  = (float)(k + 1) * (float)M_PI_2 / segs;
                const float c0  = std::cos(v0), s0 = std::sin(v0);
                const float c1  = std::cos(v1), s1 = std::sin(v1);
                const float py0 = cy + dy * ry * c0, pz0 = cz + dz * rz * s0;
                const float py1 = cy + dy * ry * c1, pz1 = cz + dz * rz * s1;
                const Point3f n0  = ellipseNormYZ(c0, s0, dy, dz);
                const Point3f n1  = ellipseNormYZ(c1, s1, dy, dz);
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
                            addQuadN(spt(ui, vi), snrm(ui, vi), spt(ui, vi + 1), snrm(ui, vi + 1),
                                     spt(ui + 1, vi + 1), snrm(ui + 1, vi + 1), spt(ui + 1, vi), snrm(ui + 1, vi));
                        }
                        else {
                            addQuadN(spt(ui, vi), snrm(ui, vi), spt(ui + 1, vi), snrm(ui + 1, vi),
                                     spt(ui + 1, vi + 1), snrm(ui + 1, vi + 1), spt(ui, vi + 1), snrm(ui, vi + 1));
                        }
                    }
                }
            }
        }
    }

    // ---- ワイヤーフレーム (m_segments_indices) ----
    if (m_create_section_line) {
        /// ワイヤーフレーム専用の頂点に使うゼロ法線
        const Point3f kZeroNorm(0.0f, 0.0f, 0.0f);

        /// 追加される頂点数・インデックス数を事前に計算して予約
        /// Part1: 24本の直線 = 48頂点
        /// Part2: 24弧 × segs = 48*segs 頂点
        /// Part3: 8コーナー × wire_segs × 2方向 × segs = 32*wire_segs*segs 頂点
        const int wire_extra_verts = 48 + 48 * segs + 32 * wire_segs * segs;
        m_vertices.reserve(m_vertices.size() + wire_extra_verts);
        m_segments_indices.reserve(m_segments_indices.size() + wire_extra_verts);

        /// 2頂点を追加して1線分ペアを m_segments_indices に登録するヘルパー
        auto addLine = [&](const Point3f& a, const Point3f& b) {
            const unsigned int i = (unsigned int)m_vertices.size();
            m_vertices.emplace_back(a, kZeroNorm);
            m_vertices.emplace_back(b, kZeroNorm);
            m_segments_indices.push_back(i);
            m_segments_indices.push_back(i + 1);
        };

        /// 弧を segs ステップで折れ線として追加するヘルパー
        auto addArcXY = [&](float cx, float cy, float z, float dx, float dy) {
            for (int k = 0; k < segs; k++) {
                const float t0 = (float)k * (float)M_PI_2 / segs;
                const float t1 = (float)(k + 1) * (float)M_PI_2 / segs;
                addLine({cx + dx * rx * std::cos(t0), cy + dy * ry * std::sin(t0), z},
                        {cx + dx * rx * std::cos(t1), cy + dy * ry * std::sin(t1), z});
            }
        };
        auto addArcXZ = [&](float cx, float y, float cz, float dx, float dz) {
            for (int k = 0; k < segs; k++) {
                const float t0 = (float)k * (float)M_PI_2 / segs;
                const float t1 = (float)(k + 1) * (float)M_PI_2 / segs;
                addLine({cx + dx * rx * std::cos(t0), y, cz + dz * rz * std::sin(t0)},
                        {cx + dx * rx * std::cos(t1), y, cz + dz * rz * std::sin(t1)});
            }
        };
        auto addArcYZ = [&](float x, float cy, float cz, float dy, float dz) {
            for (int k = 0; k < segs; k++) {
                const float t0 = (float)k * (float)M_PI_2 / segs;
                const float t1 = (float)(k + 1) * (float)M_PI_2 / segs;
                addLine({x, cy + dy * ry * std::cos(t0), cz + dz * rz * std::sin(t0)},
                        {x, cy + dy * ry * std::cos(t1), cz + dz * rz * std::sin(t1)});
            }
        };

        // ---- Part 1: 平面フェースの境界直線 (各フェース4辺 × 6面 = 24本) ----
        /// Z- 面 (z = z0)
        addLine({x0 + rx, y0 + ry, z0}, {x0 + W - rx, y0 + ry, z0});
        addLine({x0 + rx, y0 + H - ry, z0}, {x0 + W - rx, y0 + H - ry, z0});
        addLine({x0 + rx, y0 + ry, z0}, {x0 + rx, y0 + H - ry, z0});
        addLine({x0 + W - rx, y0 + ry, z0}, {x0 + W - rx, y0 + H - ry, z0});
        /// Z+ 面 (z = z0+D)
        addLine({x0 + rx, y0 + ry, z0 + D}, {x0 + W - rx, y0 + ry, z0 + D});
        addLine({x0 + rx, y0 + H - ry, z0 + D}, {x0 + W - rx, y0 + H - ry, z0 + D});
        addLine({x0 + rx, y0 + ry, z0 + D}, {x0 + rx, y0 + H - ry, z0 + D});
        addLine({x0 + W - rx, y0 + ry, z0 + D}, {x0 + W - rx, y0 + H - ry, z0 + D});
        /// X- 面 (x = x0)
        addLine({x0, y0 + ry, z0 + rz}, {x0, y0 + H - ry, z0 + rz});
        addLine({x0, y0 + ry, z0 + D - rz}, {x0, y0 + H - ry, z0 + D - rz});
        addLine({x0, y0 + ry, z0 + rz}, {x0, y0 + ry, z0 + D - rz});
        addLine({x0, y0 + H - ry, z0 + rz}, {x0, y0 + H - ry, z0 + D - rz});
        /// X+ 面 (x = x0+W)
        addLine({x0 + W, y0 + ry, z0 + rz}, {x0 + W, y0 + H - ry, z0 + rz});
        addLine({x0 + W, y0 + ry, z0 + D - rz}, {x0 + W, y0 + H - ry, z0 + D - rz});
        addLine({x0 + W, y0 + ry, z0 + rz}, {x0 + W, y0 + ry, z0 + D - rz});
        addLine({x0 + W, y0 + H - ry, z0 + rz}, {x0 + W, y0 + H - ry, z0 + D - rz});
        /// Y- 面 (y = y0)
        addLine({x0 + rx, y0, z0 + rz}, {x0 + W - rx, y0, z0 + rz});
        addLine({x0 + rx, y0, z0 + D - rz}, {x0 + W - rx, y0, z0 + D - rz});
        addLine({x0 + rx, y0, z0 + rz}, {x0 + rx, y0, z0 + D - rz});
        addLine({x0 + W - rx, y0, z0 + rz}, {x0 + W - rx, y0, z0 + D - rz});
        /// Y+ 面 (y = y0+H)
        addLine({x0 + rx, y0 + H, z0 + rz}, {x0 + W - rx, y0 + H, z0 + rz});
        addLine({x0 + rx, y0 + H, z0 + D - rz}, {x0 + W - rx, y0 + H, z0 + D - rz});
        addLine({x0 + rx, y0 + H, z0 + rz}, {x0 + rx, y0 + H, z0 + D - rz});
        addLine({x0 + W - rx, y0 + H, z0 + rz}, {x0 + W - rx, y0 + H, z0 + D - rz});

        // ---- Part 2: コーナーパッチ↔辺ストリップ境界の弧 ----
        /// XY 平面弧: z=z0+rz および z=z0+D-rz (Z平行ストリップとコーナーの境界)
        for (int sx = 0; sx < 2; sx++) {
            for (int sy = 0; sy < 2; sy++) {
                const float cx = x0 + (sx ? W - rx : rx);
                const float cy = y0 + (sy ? H - ry : ry);
                const float dx = sx ? +1.0f : -1.0f;
                const float dy = sy ? +1.0f : -1.0f;
                addArcXY(cx, cy, z0 + rz, dx, dy);
                addArcXY(cx, cy, z0 + D - rz, dx, dy);
            }
        }
        /// XZ 平面弧: y=y0+ry および y=y0+H-ry (Y平行ストリップとコーナーの境界)
        for (int sx = 0; sx < 2; sx++) {
            for (int sz = 0; sz < 2; sz++) {
                const float cx = x0 + (sx ? W - rx : rx);
                const float cz = z0 + (sz ? D - rz : rz);
                const float dx = sx ? +1.0f : -1.0f;
                const float dz = sz ? +1.0f : -1.0f;
                addArcXZ(cx, y0 + ry, cz, dx, dz);
                addArcXZ(cx, y0 + H - ry, cz, dx, dz);
            }
        }
        /// YZ 平面弧: x=x0+rx および x=x0+W-rx (X平行ストリップとコーナーの境界)
        for (int sy = 0; sy < 2; sy++) {
            for (int sz = 0; sz < 2; sz++) {
                const float cy = y0 + (sy ? H - ry : ry);
                const float cz = z0 + (sz ? D - rz : rz);
                const float dy = sy ? +1.0f : -1.0f;
                const float dz = sz ? +1.0f : -1.0f;
                addArcYZ(x0 + rx, cy, cz, dy, dz);
                addArcYZ(x0 + W - rx, cy, cz, dy, dz);
            }
        }

        // ---- Part 3: コーナーパッチの緯度経度線 (wire_segs 本の中間線) ----
        /// 緯度線 (一定 u): v 方向に弧を描く "平行圏"
        /// 経度線 (一定 v): u 方向に弧を描く "子午線"
        if (wire_segs > 0) {
            for (int sx = 0; sx < 2; sx++) {
                for (int sy = 0; sy < 2; sy++) {
                    for (int sz = 0; sz < 2; sz++) {
                        const float cx = x0 + (sx ? W - rx : rx);
                        const float cy = y0 + (sy ? H - ry : ry);
                        const float cz = z0 + (sz ? D - rz : rz);
                        const float dx = sx ? +1.0f : -1.0f;
                        const float dy = sy ? +1.0f : -1.0f;
                        const float dz = sz ? +1.0f : -1.0f;
                        auto spt = [&](float u, float v) -> Point3f {
                            return {cx + dx * rx * std::sin(u) * std::cos(v),
                                    cy + dy * ry * std::sin(u) * std::sin(v),
                                    cz + dz * rz * std::cos(u)};
                        };
                        /// 緯度線: 固定 u で v を [0, π/2] に沿って描く
                        for (int i = 1; i <= wire_segs; i++) {
                            const float u = (float)i * (float)M_PI_2 / (wire_segs + 1);
                            for (int k = 0; k < segs; k++) {
                                const float v0 = (float)k * (float)M_PI_2 / segs;
                                const float v1 = (float)(k + 1) * (float)M_PI_2 / segs;
                                addLine(spt(u, v0), spt(u, v1));
                            }
                        }
                        /// 経度線: 固定 v で u を [0, π/2] に沿って描く
                        for (int j = 1; j <= wire_segs; j++) {
                            const float v = (float)j * (float)M_PI_2 / (wire_segs + 1);
                            for (int k = 0; k < segs; k++) {
                                const float u0 = (float)k * (float)M_PI_2 / segs;
                                const float u1 = (float)(k + 1) * (float)M_PI_2 / segs;
                                addLine(spt(u0, v), spt(u1, v));
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
        for (const auto& vertex : originalMesh()->m_vertices) {
            m_bbox.expandBy(vertex.m_position);
        }
    }
    createGroupBoundingBox();
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
}

void RenderEditableNormalMesh::clearDisplayData()
{
    originalMesh()->clearDisplayData();
    clearDisplayEditData();
    markSegmentsGroupDirty();
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
