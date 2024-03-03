#version 430 core

layout(location=0) in vec3 IN_POS;
layout(location=1) in vec2 IN_UV;
layout(location=2) in uint IN_DATA;
layout(location=3) in ivec3 IN_COLORMOD;

uniform vec3 CHUNK_POS;
uniform mat4 VIEW;
uniform mat4 PROJECTION;
uniform float NIGHTTIME_LIGHT_MODIFIER;
uniform usampler3D LIGHT_TEX;

out vec2 UV_COORD;
out vec3 COLORMOD;

vec2 get_uv_coord(uint texture_index)
{
    vec2 ofs = IN_UV;
    ofs.x += texture_index % 16;
    ofs.y += texture_index / 16;
    ofs /= 16.0;
    return ofs;
}

void main()
{
    uint texture_index, face, light, bl, sl;

    texture_index = ((IN_DATA & uint(0x00ff)) >> 0) & 255u;
    face          = ((IN_DATA & uint(0xff00)) >> 8) & 7u;

    vec3 coords = vec3((IN_POS.x), (IN_POS.z), floor(IN_POS.y));
    vec3 offset = vec3(999);

    float d = 1;
    if(face == 0)
        offset = vec3(0, 0, -d);
    else if(face == 1)
        offset = vec3(0, 0, d);
    else if(face == 2)
        offset = vec3(0, 0, -d);
    else if(face == 3)
        offset = vec3(0, 0, d);
    else if(face == 4)
        offset = vec3(-d, 0, 0);
    else if(face == 5)
        offset = vec3(d, 0, 0);
    //coords += offset;
    coords.x /= 32.0;
    coords.y /= 32.0;
    coords.z /= 128.0;
    coords.x += 1 / 32.0;
    coords.y += 1 / 32.0;
    coords.z += 1 / 128.0;

    light = uint(texture(LIGHT_TEX, coords).r) & 0xffu;

    COLORMOD = vec3(1);//vec3(IN_COLORMOD.rgb / 255.0f);
    if (face == 0) { // -Y
        COLORMOD *= 0.5;
    } else if (face == 1) { // +Y
        COLORMOD *= 1.0;
    } else if (face == 2 || face == 3) { // +-Z
        COLORMOD *= 0.8;
    } else if (face == 4 || face == 5) { // +-X
        COLORMOD *= 0.6;
    }

    COLORMOD *= float(light) / 255.0f; // * NIGHTTIME_LIGHT_MODIFIER;

    vec3 block_pos = IN_POS + CHUNK_POS;

    UV_COORD = get_uv_coord(texture_index);

    gl_Position = PROJECTION * VIEW * (vec4(block_pos, 1.0));
}
