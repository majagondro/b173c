#version 430 core

layout(location = 0) in vec3 VERTEX;

uniform mat4 MODEL;
uniform mat4 VIEW;
uniform mat4 PROJECTION;

void main()
{
    gl_Position = PROJECTION * VIEW * MODEL * vec4(VERTEX, 1);
}
