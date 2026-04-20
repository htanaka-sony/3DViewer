#version 330 core

layout(location = 0) in vec3 position;  // 頂点座標
layout(location = 1) in vec3 normal; // 法線(分離したので消せるが、Pickの方は共通なのと、ダミー設定で問題ないので残しておく

out vec3 inFragObjPosition; // フラグメントorジオメトリシェーダーに渡す頂点(オブジェクト座標)
out vec3 inFragNormal; // フラグメントorジオメトリシェーダーに渡す法線(マトリックス適用済)
out vec2 inScreenPos; // Dummy（フラグメントシェーダーが共通なためダミーで残す必要ある）

uniform bool texture_3d;

uniform mat4 mvp_matrix;

void main() {
    gl_Position = mvp_matrix * vec4(position, 1.0);
    inFragObjPosition = position;    
    inScreenPos = vec2(0);// Dummy
}

