#version 320 es
in vec4 aPos;
in vec2 aTexcoord;
out vec4 in_TEXCOORD0;

void main() {
    gl_Position = aPos;
    in_TEXCOORD0.xy = aTexcoord;
}
