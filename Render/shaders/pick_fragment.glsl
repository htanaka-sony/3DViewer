#version 330 core

uniform vec3 objectIDColor;  // オブジェクトのIDをエンコードした色

out vec4 finalColor;

void main() {
    finalColor = vec4(objectIDColor, 1.0f);
}

