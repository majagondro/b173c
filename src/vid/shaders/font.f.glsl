#version 430 core

in vec2 UV;
flat in int FONTCHAR;
flat in int FONTCOLOR;

out vec4 COLOR;

uniform sampler2D TEXTURE0;

vec4 get_color_from_code(int code)
{
    float a = 170.0f / 256.0f;
    float b = 85.0f / 256.0f;
    vec4[] colorcodes = vec4[](
        vec4(vec3(0, 0, 0), 1),
        vec4(vec3(0, 0, a), 1),
        vec4(vec3(0, a, 0), 1),
        vec4(vec3(0, a, a), 1),
        vec4(vec3(a, 0, 0), 1),
        vec4(vec3(a, 0, a), 1),
        vec4(vec3(1, a, 0), 1),
        vec4(vec3(a, a, a), 1),
        vec4(vec3(b, b, b), 1),
        vec4(vec3(b, b, 1), 1),
        vec4(vec3(b, 1, b), 1),
        vec4(vec3(b, 1, 1), 1),
        vec4(vec3(1, b, b), 1),
        vec4(vec3(1, b, 1), 1),
        vec4(vec3(1, 1, b), 1),
        vec4(vec3(1, 1, 1), 1)
    );

    float mul = 1.0f;
    if (code < 0) { // hack for shadows
        code = -code;
        mul = 0.25f;
    }

    if(0x0 <= code && code <= 0xf) {
        return colorcodes[code] * vec4(mul, mul, mul, 1);
    }

    return vec4(mul, mul, mul, 1);
}

void main()
{
    vec2 newuv;

    newuv.x = float((FONTCHAR % 16 + UV.x) * 8) / 128.0f;
    newuv.y = float((FONTCHAR / 16 + UV.y) * 8) / 128.0f;

    COLOR = texture(TEXTURE0, newuv) * get_color_from_code(FONTCOLOR);
    if (COLOR.a < 0.5) {
        discard;
    }
}