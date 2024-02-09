#version 430 core

in vec2 UV;
flat in int FONTCHAR;
flat in int FONTCOLOR;

out vec4 COLOR;

uniform sampler2D TEXTURE;
uniform int CON_CHAR_SIZE;

vec4 get_color_from_code(int code)
{
    float l = 170.0 / 256.0;
    float d = 85.0 / 256.0;
    vec4[] colorcodes = vec4[](
        vec4(vec3(0, 0, 0), 1),
        vec4(vec3(0, 0, l), 1),
        vec4(vec3(0, l, 0), 1),
        vec4(vec3(0, l, l), 1),
        vec4(vec3(l, 0, 0), 1),
        vec4(vec3(l, 0, l), 1),
        vec4(vec3(1, l, 0), 1),
        vec4(vec3(l, l, l), 1),
        vec4(vec3(d, d, d), 1),
        vec4(vec3(d, d, 1), 1),
        vec4(vec3(d, 1, d), 1),
        vec4(vec3(d, 1, 1), 1),
        vec4(vec3(1, d, d), 1),
        vec4(vec3(1, d, 1), 1),
        vec4(vec3(1, 1, d), 1),
        vec4(vec3(1, 1, 1), 1)
    );

    float mul = 1.0;
    if (code < 0) { // hack for shadows
        code = -code;
        mul = 0.25;
    }

    if(0x0 <= code && code <= 0xf) {
        return colorcodes[code] * vec4(mul, mul, mul, 1);
    }

    return vec4(mul, mul, mul, 1);
}

void main()
{
    vec2 newuv;
    vec2 texture_size = vec2(textureSize(TEXTURE, 0));

    newuv.x = float((FONTCHAR % 16 + UV.x) * CON_CHAR_SIZE) / texture_size.x;
    newuv.y = float((FONTCHAR / 16 + UV.y) * CON_CHAR_SIZE) / texture_size.y;

    COLOR = texture(TEXTURE, newuv) * get_color_from_code(FONTCOLOR);
    if (COLOR.a < 0.1) {
       discard;
    }
}