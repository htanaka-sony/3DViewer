#version 330 core

uniform vec3 color_top;
uniform vec3 color_bottom;
in float v_posY;
out vec4 FragColor;

void main()
{
    vec3 color = mix(color_bottom, color_top, v_posY);
    FragColor = vec4(color, 1.0);
}
