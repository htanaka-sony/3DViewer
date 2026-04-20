#version 330 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 inFragObjPosition[]; // 頂点シェーダーから受け取る
out vec3 fragObjPosition;  // フラグメントシェーダーへ渡す（同じ名前）

in vec3 inFragNormal[];
out vec3 fragNormal;

in vec2 inScreenPos[];
out vec2 screenPos;

uniform mat3 nrm_matrix;
uniform mat4 m_matrix;
uniform mat4 vp_matrix;

uniform bool      texture_3d;
uniform vec3      voxel_size;

uniform bool  direct_offset;
uniform float direct_offset_value;

void main() {
    // 面法線（オブジェクト空間）
    vec3 p0 = inFragObjPosition[0];
    vec3 p1 = inFragObjPosition[1];
    vec3 p2 = inFragObjPosition[2];
    vec3 n_obj = normalize(cross(p1 - p0, p2 - p0));
    vec3 n = normalize(nrm_matrix * n_obj);

    if (texture_3d){
        // 法線方向にオフセット
        vec3 epsilon = 1 / voxel_size * 1.0e-2;

        for (int i = 0; i < 3; ++i) {
            fragObjPosition = inFragObjPosition[i] - n_obj * epsilon; // ここで書き換え
            fragNormal = n;
            screenPos = inScreenPos[i];
            if (direct_offset) {
                vec4 world_pos = m_matrix * vec4(inFragObjPosition[i], 1.0);
                gl_Position = vp_matrix * vec4(world_pos.xyz + direct_offset_value * n, 1.0);
            } else {
                gl_Position = gl_in[i].gl_Position;
            }
            EmitVertex();
        }
        EndPrimitive();
    }
    else {
        // デフォルト動作（元の頂点情報をそのまま出力）
        for (int i = 0; i < 3; ++i) {
            fragObjPosition = inFragObjPosition[i];
            fragNormal = n;
            screenPos = inScreenPos[i];
            if (direct_offset) {
                vec4 world_pos = m_matrix * vec4(inFragObjPosition[i], 1.0);
                gl_Position = vp_matrix * vec4(world_pos.xyz + direct_offset_value * n, 1.0);
            } else {
                gl_Position = gl_in[i].gl_Position;
            }
            EmitVertex();
        }
        EndPrimitive();
    }
}
