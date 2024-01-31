#version 430 core

layout(location=0) in uint IN;

uniform vec3 CHUNK_POS;
uniform mat4 VIEW;
uniform mat4 PROJECTION;

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
    uint x, y, z, texture_index, face;

    x = (IN >> 0) & 31u;
    y = (IN >> 5) & 31u;
    z = (IN >> 10) & 31u;
    texture_index = (IN & uint(0x00ff0000)) >> 16;
    face =          (IN & uint(0xff000000)) >> 24;

    vec3 block_pos = vec3(x, y, z) + CHUNK_POS;

    COLORMOD = vec3(1);
    if(texture_index == 0) {
        COLORMOD = vec3(0.34375,0.4453,0.207)*1.9;
    }

    if(face == 0) {
        COLORMOD *= 0.3;
    } else if(face == 1) {
        COLORMOD *= 1.0;
    } else if(face == 2 || face == 3) {
        COLORMOD *= 0.8;
    } else if(face == 4 || face == 5) {
        COLORMOD *= 0.6;
    }

    UV_COORD = get_uv_coord(gl_VertexID, texture_index);

    gl_Position = PROJECTION * VIEW * (vec4(block_pos, 1.0));
}
