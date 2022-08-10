#version 430 core

out vec4 FragColor;

in vec2 TexCoord;

uniform vec4 color;


void main() {
    FragColor = vec4(color);
}
