#version 430 core
in vec3 VERTEX_IN;
in vec2 DATA_IN;

out vec2 DATA;

void main()
{
    gl_Position = vec4(VERTEX_IN.xyz, 1.0f) + vec4(0.5f, 0.5f, 0.5f, 0.0f);
    DATA = DATA_IN;
}
