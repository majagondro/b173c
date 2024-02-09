#version 430 core

layout(location = 0) in vec2 VERTEX;
layout(location = 1) in vec2 TEXCOORD;
layout(location = 2) in vec4 FONTDATA;

out vec2 UV;
flat out int FONTCHAR;
flat out int FONTCOLOR;

void main()
{
    // multiply by 2 and move because screen coordinates range from -1 to 1
    vec2 org = 2.0 * VERTEX + 2.0 * vec2(FONTDATA.x, -FONTDATA.y);
    org -= vec2(1.0, -1.0);

    gl_Position = vec4(org, -1.0, 1.0);
    UV = TEXCOORD;

    FONTCHAR = int(FONTDATA.z);
    FONTCOLOR = int(FONTDATA.w);

    if(FONTCOLOR < 0) {
        // shadow, add some z to draw behind the normal text
        gl_Position.z += 0.001;
    }
}