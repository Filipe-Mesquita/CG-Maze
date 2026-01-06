#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;

out vec2 vUV;
uniform mat4 uProj;

void main() {
    vUV = aUV;
    gl_Position = uProj * vec4(aPos.xy, 0.0, 1.0);
}
