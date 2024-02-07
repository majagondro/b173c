#version 430 core

layout(location=0) in vec3 IN_POS;
layout(location=1) in uint IN_DATA;

uniform vec3 CHUNK_POS;
uniform mat4 VIEW;
uniform mat4 PROJECTION;
uniform float NIGHTTIME_LIGHT_MODIFIER;

out vec2 UV_COORD;
out vec3 COLORMOD;

vec2 get_uv_coord(int vertex_id, uint texture_index)
{
    vec2 ofs = vec2(0);

    vertex_id %= 6;

    if(vertex_id == 1 || vertex_id == 3) {
        ofs = vec2(0.0, 1.0);
    } else if(vertex_id == 2 || vertex_id == 5) {
        ofs = vec2(1.0, 0.0);
    } else if(vertex_id == 0) {
        ofs = vec2(0.0, 0.0);
    } else if(vertex_id == 4) {
        ofs = vec2(1.0, 1.0);
    }

    ofs.x += texture_index % 16;
    ofs.y += texture_index / 16;

    ofs /= 16.0;

    return ofs;
}

void main()
{
    uint texture_index, face, light;
    int extradata;

    texture_index = (IN_DATA & uint(0x00ff)) >> 0;
    face          = ((IN_DATA & uint(0xff00)) >> 8) & 7u;
    light         = ((IN_DATA & uint(0xff00)) >> (8+3)) & uint(255>>3);
    extradata     = (int(IN_DATA) & 0xffff0000) >> 16;

    COLORMOD = vec3(1);
    if(texture_index == 0) { // temp: grass color todo: removeme
        COLORMOD = vec3(0.34375,0.4453,0.207)*1.9;
    }

    if(face == 0) { // -Y
        COLORMOD *= 0.5;
    } else if(face == 1) { // +Y
        COLORMOD *= 1.0;
    } else if(face == 2 || face == 3) { // +-Z
        COLORMOD *= 0.8;
    } else if(face == 4 || face == 5) { // +-X
        COLORMOD *= 0.6;
    }

    COLORMOD *= float(light) / 15.0f * NIGHTTIME_LIGHT_MODIFIER;

    vec3 block_pos = IN_POS + CHUNK_POS;
    vec2 uv = get_uv_coord(gl_VertexID, texture_index);
    // float angl = radians(float(extradata));
    // UV_COORD.x = uv.x * cos(angl) - uv.y * sin(angl);
    // UV_COORD.y = uv.x * sin(angl) + uv.y * cos(angl);
    UV_COORD = uv;

    gl_Position = PROJECTION * VIEW * (vec4(block_pos, 1.0));
}
