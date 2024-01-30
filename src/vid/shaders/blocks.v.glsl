#version 430 core

//layout(location=0) in int data_in;
/*layout(location=1) in int y;
layout(location=2) in int texture_index;
layout(location=3) in int data;*/

in vec3 pos;
in vec2 data;

uniform vec2 CHUNK_POS;
uniform mat4 VIEW;
uniform mat4 PROJECTION;

out vec2 UV_COORD;
out vec3 COLORMOD;

#define FACE_Y_POS 1
#define FACE_Y_NEG 2
#define FACE_X_POS 4
#define FACE_X_NEG 8
#define FACE_Z_POS 16
#define FACE_Z_NEG 32

mat3 rotmat_z(float angle)
{
    mat3 rotmat = mat3(1);

    angle = radians(angle);

    rotmat[0][0] =  cos(angle);
    rotmat[0][1] = -sin(angle);
    rotmat[1][0] =  sin(angle);
    rotmat[1][1] =  cos(angle);

    return rotmat;
}

mat3 rotmat_x(float angle)
{
    mat3 rotmat = mat3(1);

    angle = radians(angle);

    rotmat[1][1] =  cos(angle);
    rotmat[1][2] = -sin(angle);
    rotmat[2][1] =  sin(angle);
    rotmat[2][2] =  cos(angle);

    return rotmat;
}

vec3 transform_offset(vec3 offset, uint face)
{

    if(face == FACE_Y_POS) {
        offset.y += 0.5;
    } else if(face == FACE_Y_NEG) {
        mat3 rotmat = rotmat_z(180.0);

        offset = rotmat * offset;
        offset.y -= 0.5;
    } else if(face == FACE_X_POS) {
        mat3 rotmat = rotmat_z(90.0);

        offset = rotmat * offset;
        offset.x += 0.5;
    } else if(face == FACE_X_NEG) {
        mat3 rotmat = rotmat_z(270.0);

        offset = rotmat * offset;
        offset.x -= 0.5;
    } else if(face == FACE_Z_POS) {
        mat3 rotmat = rotmat_x(270.0);

        offset = rotmat * offset;
        offset.z += 0.5;
    } else if(face == FACE_Z_NEG) {
        mat3 rotmat = rotmat_x(90.0);

        offset = rotmat * offset;
        offset.z -= 0.5;
    }
    else {
        offset = vec3(0);
    }

    // block positions are corners, we want the center here
    offset += vec3(0.5);
    return offset;
}

vec3 get_offset(int vertex_id, int face)
{
    vec3 off = vec3(1000);

    vertex_id %= 6;

    if(vertex_id == 1 || vertex_id == 3) {
        off = vec3(-0.5, 0.0, 0.5);
    } else if(vertex_id == 2 || vertex_id == 5) {
        off = vec3(0.5, 0.0, -0.5);
    } else if(vertex_id == 0) {
        off = vec3(-0.5, 0.0, -0.5);
    } else if(vertex_id == 4) {
        off = vec3(0.5, 0.0, 0.5);
    }

    return transform_offset(off, face);
}

vec2 get_uv_coord(int vertex_id, int texture_index)
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
    int x, y, z, texture_index, face;

    x = int(pos.x);//(data_in & 0x0f);
    y = int(pos.y);//(data_in & 0xff00) >> 8;
    z = int(pos.z);//(data_in & 0xf0) >> 4;
    texture_index = int(data.x);//(data_in & 0xff0000) >> 16;
    face = int(data.y);//(data_in & 0xff000000) >> 24;

    vec3 offset = get_offset(gl_VertexID, face);
    vec3 chunk_pos = vec3(CHUNK_POS.x, 0.0, CHUNK_POS.y);

    vec3 block_pos = vec3(x, y, z) + offset + chunk_pos;

    COLORMOD = vec3(1);
    UV_COORD = get_uv_coord(gl_VertexID, texture_index);

    gl_Position = PROJECTION * VIEW * (vec4(block_pos, 1.0));
}
