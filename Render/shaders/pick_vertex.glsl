#version 330 core

layout(location = 0) in vec3 position;  // 頂点座標
layout(location = 1) in vec3 normal; // 法線

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
        vec3 world_normal = nrm_matrix * normal;
        if (length(world_normal) > 0.001) {
            // 法線方向にオフセット
            vec3 offset_pos = world_pos.xyz + direct_offset_value * normalize(world_normal);
            gl_Position = vp_matrix * vec4(offset_pos, 1.0);
        } else {
            // 法線がない場合はカメラ方向にフォールバック
            if (is_perspective){
                vec3 view_dir = normalize(world_pos.xyz - camera_pos);
                vec3 offset_pos = world_pos.xyz + direct_offset_value * view_dir;
                gl_Position = vp_matrix * vec4(offset_pos, 1.0);
            }
            else{
                vec3 offset_pos = world_pos.xyz + direct_offset_value * camera_dir;
                gl_Position = vp_matrix * vec4(offset_pos, 1.0);
            }
        }
    }
}
