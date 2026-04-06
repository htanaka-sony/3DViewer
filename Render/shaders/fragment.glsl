#version 330 core

/// TODO:
/// いろいろ混在している
/// それぞれの要素単位で分離してシェーダーわけるのがいい
/// 共通のメモリは使いたいので、UBOを使えばいい？

in vec3 fragNormal;
in vec3 fragObjPosition;
in vec2 screenPos;

uniform vec3  color;           // オブジェクトの色
uniform float transparency;    // 透明度
uniform bool  disable_light;
uniform bool  point_display;
uniform bool  point_display_line;
uniform bool  dashed_line;
uniform float dashed_line_pixel;    // 点線の間隔（ピクセル単位）
uniform vec3  line_start;
uniform float point_display_line_width;
uniform vec2 point_display_line_dir;

uniform vec2 screen_size;
uniform vec3 light_dir;      // 光源の位置
uniform vec3 light_color;    // 光源の色

out vec4 finalColor;

uniform mat4 mvp_matrix;

uniform bool      texture_3d;
uniform int       division_size;
uniform float     divisions[255];    // 分割の境界値（降順にソートされている必要あり）
uniform vec4      colors[256];       // 各範囲に対応する色(division_size＋１になる)

/// 安定性のため2Dも3D(Z=1)で扱う
///  ※ sampler2Dとsampler3Dを32宣言して参照する(参照コメントアウトすると表示)だけで表示できない環境あり 32-1で宣言して回避はできたが。。。
#define MAX_TILE_COUNT 31 /// 32-1にしておく
uniform sampler3D volumeTex[MAX_TILE_COUNT];
uniform int tileCount;
uniform int tileXCount;
uniform int tileYCount;
uniform int tileZCount;
uniform float tileOffsetX[MAX_TILE_COUNT];
uniform float tileOffsetY[MAX_TILE_COUNT];
uniform float tileOffsetZ[MAX_TILE_COUNT];
uniform float tileSizeX[MAX_TILE_COUNT];
uniform float tileSizeY[MAX_TILE_COUNT];
uniform float tileSizeZ[MAX_TILE_COUNT];

uniform vec3      voxel_range_min;
uniform vec3      voxel_range_max;
uniform int       rangeCount;
uniform float     rangeStart[30];
uniform float     rangeEnd[30];
uniform bool      discard_out_range;

uniform sampler2D planeTexture;   // 2Dテクスチャ
uniform bool      texture_2d;  // 2Dテクスチャを使うかどうか

vec4 calcLitColor(vec3 normal, vec3 color, float transparency) {
    float diff = max(abs(dot(normal, light_dir)), 0.0);

    vec3 ambient = vec3(0.62) * color;           // 環境光
    vec3 diffuse = diff * 0.38 * light_color * color; // 陰影

    vec3 result = ambient + diffuse;
    result = min(result, vec3(1.0)); // 白飛び防止
    return vec4(result, transparency);
}

vec4 calcPlainColor(vec3 color, float transparency) {
    return vec4(color, transparency);
}

vec4 getColor(float value)
{
    // 二分探索による範囲判定
    int low  = 0;
    int high = division_size - 1;    // 配列の最後のインデックス
    while (low <= high) {
        int mid = (low + high) / 2;    // 中央のインデックスを計算
        if (value > divisions[mid]) {
            high = mid - 1;    // 左側を探索
        }
        else {
            low = mid + 1;    // 右側を探索
        }
    }
    if (low > division_size) return colors[division_size];
    if (low < 0) return colors[0];

    // `low` が属する範囲のインデックス
    return colors[low];
}

vec4 getColorIndex(int color)
{
    // `low` が属する範囲のインデックス
    return colors[color];
}

int lowerBound(float x) {
    int left = 0;
    int right = rangeCount;
    while (left < right) {
        int mid = (left + right) / 2;
        if (rangeEnd[mid] < x) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }
    return left;
}

bool isInMergedRange(float x) {
    int idx = lowerBound(x);
    if (idx == rangeCount) return false;
    return rangeStart[idx] <= x && x <= rangeEnd[idx];
}

int findTile(vec2 uv, int tileXCount, int tileYCount) {

    // タイルが1個だけの場合の範囲チェックつき高速パス
    if (tileXCount == 1 && tileYCount == 1) {
        float startX = tileOffsetX[0];
        float endX = startX + tileSizeX[0];
        float startY = tileOffsetY[0];
        float endY = startY + tileSizeY[0];
        // 末尾はendも含む
        if (uv.x >= startX && uv.x <= endX && uv.y >= startY && uv.y <= endY)
            return 0;
        else
            return -1;
    }

    // 1. Y行を検索
    int row = -1;
    {
        int low = 0;
        int high = tileYCount - 1;
        while (low <= high) {
            int mid = (low + high) / 2;
            int idx = mid * tileXCount;
            float start = tileOffsetY[idx];
            float end = start + tileSizeY[idx];
            bool isLast = (mid == tileYCount - 1);
            if (uv.y < start)
                high = mid - 1;
            else if ((!isLast && uv.y >= end) || (isLast && uv.y > end))
                low = mid + 1;
            else {
                row = mid;
                break;
            }
        }
    }
    if (row < 0) return -1;

    // 2. X列を検索
    int col = -1;
    {
        int rowStart = row * tileXCount;
        int low = 0;
        int high = tileXCount - 1;
        while (low <= high) {
            int mid = (low + high) / 2;
            int idx = rowStart + mid;
            float start = tileOffsetX[idx];
            float end = start + tileSizeX[idx];
            bool isLast = (mid == tileXCount - 1);
            if (uv.x < start)
                high = mid - 1;
            else if ((!isLast && uv.x >= end) || (isLast && uv.x > end))
                low = mid + 1;
            else {
                col = mid;
                break;
            }
        }
    }
    if (col < 0) return -1;

    // 総合インデックス
    return row * tileXCount + col;
}

int findTile3D(vec3 uv, int tileXCount, int tileYCount, int tileZCount) {

    // タイルが1個だけの場合の範囲チェックつき高速パス
    if (tileXCount == 1 && tileYCount == 1 && tileZCount == 1) {
        float startX = tileOffsetX[0];
        float endX = startX + tileSizeX[0];
        float startY = tileOffsetY[0];
        float endY = startY + tileSizeY[0];
        float startZ = tileOffsetZ[0];
        float endZ = startZ + tileSizeZ[0];
        // 末尾はendも含む
        if (uv.x >= startX && uv.x <= endX && uv.y >= startY && uv.y <= endY && uv.z >= startZ && uv.z <= endZ)
            return 0;
        else
            return -1;
    }

    // X方向
    int xid = -1;
    {
        int low = 0, high = tileXCount - 1;
        while (low <= high) {
            int mid = (low + high) / 2;
            // X最外なのでmid進める
            int ref = mid * tileYCount * tileZCount; // (x * yz)
            float start = tileOffsetX[ref];
            float end = start + tileSizeX[ref];
            // 最後のタイルは<=endを許容
            bool isLast = (mid == tileXCount - 1);
            if (uv.x < start) { high = mid - 1; }
            else if ((!isLast && uv.x >= end) || (isLast && uv.x > end)) { low = mid + 1; }
            else { xid = mid; break; }
        }
    }
    if (xid < 0) return -1;

    // Y方向
    int yid = -1;
    {
        int low = 0, high = tileYCount - 1;
        while (low <= high) {
            int mid = (low + high) / 2;
            // xid決定済みなので、その塊内でmidだけy変える
            int ref = xid * tileYCount * tileZCount + mid * tileZCount; // (x * yz + y * z)
            float start = tileOffsetY[ref];
            float end = start + tileSizeY[ref];
            bool isLast = (mid == tileYCount - 1);
            if (uv.y < start) { high = mid - 1; }
            else if ((!isLast && uv.y >= end) || (isLast && uv.y > end)) { low = mid + 1; }
            else { yid = mid; break; }
        }
    }
    if (yid < 0) return -1;

    // Z方向
    int zid = -1;
    {
        int low = 0, high = tileZCount - 1;
        int base = xid * tileYCount * tileZCount + yid * tileZCount; // (x * yz + y * z)
        while (low <= high) {
            int mid = (low + high) / 2;
            int ref = base + mid; // z最内
            float start = tileOffsetZ[ref];
            float end = start + tileSizeZ[ref];
            bool isLast = (mid == tileZCount - 1);
            if (uv.z < start) { high = mid - 1; }
            else if ((!isLast && uv.z >= end) || (isLast && uv.z > end)) { low = mid + 1; }
            else { zid = mid; break; }
        }
    }
    if (zid < 0) return 1;

    // 3重ループ=X最外,Z最内と揃えて添字計算
    return xid * tileYCount * tileZCount + yid * tileZCount + zid;
}

void main()
{
    /// 3Dテクスチャ
    if (texture_3d) {

        vec3 localfragObjPosition = (fragObjPosition - voxel_range_min) / (voxel_range_max - voxel_range_min);

        vec3 offsetPos = localfragObjPosition;

        int idx3d = findTile3D(offsetPos, tileXCount, tileYCount, tileZCount);
        if (idx3d >= 0) {
            /// 選ばれたタイルの範囲内で正規化
            vec3 tileMin = vec3(tileOffsetX[idx3d], tileOffsetY[idx3d], tileOffsetZ[idx3d]);
            vec3 tileSize = vec3(tileSizeX[idx3d], tileSizeY[idx3d], tileSizeZ[idx3d]);
            vec3 tileUV = (offsetPos - tileMin) / tileSize;

            vec3  swap_offsetPos = vec3(tileUV.z, tileUV.y, tileUV.x);

            float intensity = texture(volumeTex[idx3d], swap_offsetPos).r;
            /// 値の範囲指定がある場合
            if (rangeCount > 0 && !isInMergedRange(intensity)) {
                /// 描画しない
                if (discard_out_range){
                    discard;
                }
                /// 通常の形状描画
                else if (disable_light) {
                    finalColor = calcPlainColor(color, transparency);
                } else {
                    finalColor = calcLitColor(fragNormal, color, transparency);
                }
            }
            else {
            #if 1    /// 通常
                finalColor = getColor(intensity);
            #else    /// 直接設定時
                int colorIndex = int(clamp(intensity, 0.0, 1.0) * 255.0);    // 0-255に変換
                finalColor     = getColorIndex(colorIndex);
            #endif
                finalColor[3] = transparency;
            }
        } else {
            /// タイル範囲外
            if (discard_out_range) {discard;}
            else if (disable_light) {finalColor = calcPlainColor(color, transparency);}
            else {finalColor = calcLitColor(fragNormal, color, transparency);}
        }
        /*
        /// 範囲外の場合
        if(any(lessThan(swap_offsetPos, vec3(0.0))) || any(greaterThan(swap_offsetPos, vec3(1.0)))) {
            /// 描画しない
            if (discard_out_range){
                discard;
            }
            /// 通常の形状描画
            else if (disable_light) {
                finalColor = calcPlainColor(color, transparency);
            } else {
                finalColor = calcLitColor(fragNormal, color, transparency);
            }
        }
        else {
            float intensity = texture(volumeTexture, swap_offsetPos).r;
            /// 値の範囲指定がある場合
            if (rangeCount > 0 && !isInMergedRange(intensity)) {
                /// 描画しない
                if (discard_out_range){
                    discard;
                }
                /// 通常の形状描画
                else if (disable_light) {
                    finalColor = calcPlainColor(color, transparency);
                } else {
                    finalColor = calcLitColor(fragNormal, color, transparency);
                }
            }
            else {
    #if 1    /// 通常
                finalColor = getColor(intensity);
    #else    /// 直接設定時
                int colorIndex = int(clamp(intensity, 0.0, 1.0) * 255.0);    // 0-255に変換
                finalColor     = getColorIndex(colorIndex);
    #endif
                finalColor[3] = transparency;
            }
        }
        */
    }
    else if (texture_2d) { /// 3DテクスチャのZ=0固定にする（2Dがうまくいかないので）

        vec3 localfragObjPosition = (fragObjPosition - voxel_range_min) / (voxel_range_max - voxel_range_min);

        vec2 uv = localfragObjPosition.xy;

        int idx = findTile(uv, tileXCount, tileYCount);
        if (idx >= 0) {
            vec2 tileUV = (uv - vec2(tileOffsetX[idx], tileOffsetY[idx])) / vec2(tileSizeX[idx],tileSizeY[idx]);

            float intensity = texture(volumeTex[idx], vec3(tileUV, 0.0)).r;
            if (rangeCount > 0 && !isInMergedRange(intensity)) {
                if (discard_out_range) {discard;}
                else if (disable_light) {finalColor = calcPlainColor(color, transparency);}
                else {finalColor = calcLitColor(fragNormal, color, transparency);}
            }
            else {
                finalColor = getColor(intensity);
                finalColor.a = transparency;
            }
        }else{
            /// 範囲外
            if (discard_out_range) discard;
            else if (disable_light) finalColor = calcPlainColor(color, transparency);
            else finalColor = calcLitColor(fragNormal, color, transparency);
        }
    }
    /*
    else if (texture_2d){
        vec3 localfragObjPosition = (fragObjPosition - voxel_range_min) / (voxel_range_max - voxel_range_min);

        vec2 uv = localfragObjPosition.xy;

        /// 範囲外の場合
        if(any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0)))) {
            /// 通常の形状描画
            if (disable_light) {
                finalColor = calcPlainColor(color, transparency);
            } else {
                finalColor = calcLitColor(fragNormal, color, transparency);
            }
        }
        float intensity = texture(planeTexture, uv).r;

         if (rangeCount > 0 && !isInMergedRange(intensity)) {
             if (disable_light) {
                 finalColor = calcPlainColor(color, transparency);
             } else {
                 finalColor = calcLitColor(fragNormal, color, transparency);
             }
         } else {
             finalColor = getColor(intensity);
             finalColor[3] = transparency;
         }
    }
    */
    /// 破線描画
    else if (dashed_line) {
        /// 点線の描画
        vec4 line_start_pos = mvp_matrix * vec4(line_start, 1.0);
        vec2 line_screen    = line_start_pos.xy / line_start_pos.w;

        vec2  pixelPos     = (screenPos * 0.5 + vec2(0.5)) * screen_size;
        vec2  line_screen_ = (line_screen * 0.5 + vec2(0.5)) * screen_size;
        float distance     = length(pixelPos - line_screen_);
        if (mod(distance, dashed_line_pixel) < (dashed_line_pixel / 2.0)) {
            finalColor = vec4(color, transparency);
        }
        else {
            discard;
        }
    }
    /// 点描画
    else if (point_display) {
        if(point_display_line){
            vec2 coord = gl_PointCoord - vec2(0.5);
            float d = dot(coord, point_display_line_dir);

            if (abs(d) < point_display_line_width * 0.5) {
                finalColor = vec4(color, transparency);
            } else {
                discard;
            }
        }
        else{
            // 点の中心からの距離を計算
            vec2  coord = gl_PointCoord - vec2(0.5);    // gl_PointCoordは点のテクスチャ座標
            float dist  = length(coord);

            // 円で描画
            if (dist <= 0.5) {
                finalColor = vec4(color, transparency);
            }
            else {
                discard;
            }
        }
    }
    /// 形状描画
    else if (disable_light) {
        finalColor = calcPlainColor(color, transparency);
    } else {
        finalColor = calcLitColor(fragNormal, color, transparency);
    }
}
