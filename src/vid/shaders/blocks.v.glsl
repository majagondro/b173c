#version 430 core

in vec3 VERTEX_IN;

uniform mat4 PROJECTION;
uniform mat4 VIEW;

void main()
{
    gl_Position = PROJECTION * VIEW * (vec4(VERTEX_IN.xyz, 1.0f) + vec4(0.5f, 0.5f, 0.5f, 0.0f));
}
