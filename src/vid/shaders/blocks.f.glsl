#version 430 core

in vec3 COLORMOD;
in vec2 UV_COORD;

out vec4 COLOR;

uniform sampler2D TEXTURE;

void main()
{
    COLOR = texture(TEXTURE, UV_COORD) * vec4(COLORMOD, 1.0);
}
