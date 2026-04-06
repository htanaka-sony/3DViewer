#version 330 core

layout(lines) in;
layout(line_strip, max_vertices = 2) out;

in vec3 inFragObjPosition[]; // 頂点シェーダーから受け取る
out vec3 fragObjPosition;

in vec3 inFragNormal[];
out vec3 fragNormal;

in vec2 inScreenPos[];
out vec2 screenPos;

uniform mat4 mvp_matrix;
uniform mat4 vp_matrix;
uniform mat4 m_matrix;
uniform mat3 nrm_matrix;
uniform vec3 camera_dir;      // カメラ視線方向（ワールド空間）
uniform vec3 camera_pos; // カメラ位置（ワールド空間）
uniform bool is_perspective; // 透視投影かどうか

uniform bool direct_offset;
uniform float line_offset_value_front;
uniform float line_offset_value_back;

void main()
{
    vec4 wp0 = m_matrix * vec4(inFragObjPosition[0], 1.0);
    vec4 wp1 = m_matrix * vec4(inFragObjPosition[1], 1.0);

    // 線分の方向ベクトル
    vec3 dir = normalize(wp1.xyz - wp0.xyz);

    float dot_val = abs(dot(dir, camera_dir));

    float threshold = 0.26; // 垂直に近い場合のしきい値（調整可能）

    vec4 clip0 = mvp_matrix * vec4(inFragObjPosition[0], 1.0);
    vec4 clip1 = mvp_matrix * vec4(inFragObjPosition[1], 1.0);

    // スクリーン空間座標
    vec2 screen0 = clip0.xy / clip0.w;
    vec2 screen1 = clip1.xy / clip1.w;

    float distance_screen = distance(screen0, screen1);

    for (int i = 0; i < 2; ++i) {
        /// 画面に平行
        if (dot_val < threshold){
            vec4 world_pos = m_matrix * vec4(inFragObjPosition[i], 1.0);
            if (is_perspective){
               vec3 view_dir = normalize(world_pos.xyz - camera_pos); // 各頂点ごとのカメラ→頂点方向
               vec3 offset_pos = world_pos.xyz + line_offset_value_front * view_dir;
               gl_Position = vp_matrix * vec4(offset_pos, 1.0);
            }
            else{
                vec3 offset_pos = world_pos.xyz + line_offset_value_front * camera_dir;
                gl_Position = vp_matrix * vec4(offset_pos, 1.0);
            }
        /// 画面に平行でない
        } else {
            if (distance_screen < 0.001){
                /// 線が浮き上がってきたなく見えるので回避する目的
                /// 平行に近くない場合微小線は描画しない
                continue;
            }

            vec4 world_pos = m_matrix * vec4(inFragObjPosition[i], 1.0);
            if (is_perspective){
               vec3 view_dir = normalize(world_pos.xyz - camera_pos); // 各頂点ごとのカメラ→頂点方向
               vec3 offset_pos = world_pos.xyz + line_offset_value_back * view_dir;
               gl_Position = vp_matrix * vec4(offset_pos, 1.0);
            }
            else{
                vec3 offset_pos = world_pos.xyz + line_offset_value_back * camera_dir;
                gl_Position = vp_matrix * vec4(offset_pos, 1.0);
            }
        }
        screenPos = inScreenPos[i];
        EmitVertex();
    }
    EndPrimitive();
}
