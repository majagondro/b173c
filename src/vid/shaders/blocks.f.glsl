#version 430 core

uniform int RENDER_MODE;
uniform sampler2D TEXTURE0;

in vec2 UV_BASE;
flat in vec2 UV_BLOCK;
in vec4 COLORMOD;

out vec4 COLOR;

void main()
{
    COLOR = texture(TEXTURE0, UV_BASE + UV_BLOCK) * COLORMOD;

    if(RENDER_MODE == 1 && COLOR.a == 0) {
        discard;
    } else if(RENDER_MODE == 0 && COLOR.a < 1) {
        discard;
    }
}
