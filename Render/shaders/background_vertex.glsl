#version 330 core

layout(location = 0) in vec2 position;
out float v_posY;

void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
    v_posY = (position.y + 1.0) * 0.5; // NDC(-1~1)→0~1
}
