#version 430 core

layout(location = 0) in vec2 VERTEX;
layout(location = 1) in vec2 TEXCOORD;
layout(location = 2) in vec4 FONTDATA;

out vec2 UV;
flat out int FONTCHAR;
flat out int FONTCOLOR;

void main()
{
    /* move to top left */
    vec2 org = VERTEX + vec2(-1.0f, 1.0f);
    /* add instance offset */
    /* mul by 2 because opengl is -1 to 1 (range 2), offset is 0 to 1 (range 1) */
    org += vec2(FONTDATA.x, -FONTDATA.y) * 2;
    gl_Position = vec4(org, -1.0f, 1.0f);
    UV = TEXCOORD;

    FONTCHAR = int(FONTDATA.z);
    FONTCOLOR = int(FONTDATA.w);

    if(FONTCOLOR < 0) {
        /* shadow - draw behind */
        gl_Position.z += 0.001f;
    }
}