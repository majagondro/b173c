#version 430 core

out vec4 COLOR;

in vec2 UV_COORD;

void main()
{
    COLOR = vec4(UV_COORD,1,1);
}
