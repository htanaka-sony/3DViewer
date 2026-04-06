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

uniform bool      texture_3d;
uniform vec3      voxel_size;

void main() {
    if (texture_3d){
        // 法線方向にオフセット
        vec3 p0 = inFragObjPosition[0];
        vec3 p1 = inFragObjPosition[1];
        vec3 p2 = inFragObjPosition[2];
        vec3 n = normalize(cross(p1 - p0, p2 - p0));
        vec3 n1 = normalize(nrm_matrix * n);

        vec3 epsilon = 1 / voxel_size * 1.0e-2;

        for (int i = 0; i < 3; ++i) {
            fragObjPosition = inFragObjPosition[i] - n * epsilon; // ここで書き換え
            fragNormal = n1;
            screenPos = inScreenPos[i];
            gl_Position = gl_in[i].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
    }
    else {
        // 法線
        vec3 p0 = inFragObjPosition[0];
        vec3 p1 = inFragObjPosition[1];
        vec3 p2 = inFragObjPosition[2];
        vec3 n = normalize(nrm_matrix * cross(p1 - p0, p2 - p0));

        // デフォルト動作（元の頂点情報をそのまま出力）
        for (int i = 0; i < 3; ++i) {
            fragObjPosition = inFragObjPosition[i];
            fragNormal = n;
            screenPos = inScreenPos[i];
            gl_Position = gl_in[i].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
    }
}
