#version 330 core

layout(location = 0) in vec3 position;  // 頂点座標
layout(location = 1) in vec3 normal; // 法線

out vec3 fragObjPosition; // フラグメントorジオメトリシェーダーに渡す頂点(オブジェクト座標)
out vec3 fragNormal; // フラグメントorジオメトリシェーダーに渡す法線(マトリックス適用済)
out vec2 screenPos;

uniform bool disable_light;
uniform bool point_display;
uniform float point_size;                    // ピクセルサイズ
uniform bool dashed_line;
uniform bool texture_3d;

uniform mat4 mvp_matrix;
uniform mat4 vp_matrix;
uniform mat4 m_matrix;
uniform mat3 nrm_matrix;

uniform bool direct_offset;
uniform float direct_offset_value;
uniform vec3 camera_dir;      // 光源の位置
uniform vec3 camera_pos; // カメラ位置（ワールド空間）
uniform bool is_perspective; // 透視投影かどうか

void main() {
    if (!direct_offset){
        gl_Position = mvp_matrix * vec4(position, 1.0);
    }else{
        vec4 world_pos = m_matrix * vec4(position, 1.0);
        vec3 world_normal = normalize(nrm_matrix * normal);
        vec3 offset_pos = world_pos.xyz + direct_offset_value * world_normal;
        gl_Position = vp_matrix * vec4(offset_pos, 1.0);
    }
    fragObjPosition = position;

    if (!disable_light){
        fragNormal =  normalize(nrm_matrix * normal);
    }

    if (point_display){
        gl_PointSize = point_size;
    }else{
        gl_PointSize = 1;
    }

    if (dashed_line){
        screenPos = gl_Position.xy / gl_Position.w;
    }
}

