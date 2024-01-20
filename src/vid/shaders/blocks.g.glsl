#version 460 core
layout(points) in;
layout(triangle_strip, max_vertices = 24) out;

in vec2[] DATA;

uniform mat4 VIEW;
uniform mat4 PROJECTION;
uniform float NIGHTTIME_LIGHT_MODIFIER;

out vec2 UV_BASE;
flat out vec2 UV_BLOCK;
out vec4 COLORMOD;

#define NUM_BLOCKS 97
const ivec2 block_textures[NUM_BLOCKS] = ivec2[NUM_BLOCKS](
    ivec2( 0, 14), // air/error
    ivec2( 1,  0), // stone
    ivec2( 3,  0), // grass
    ivec2( 2,  0), // dirt
    ivec2( 0,  1), // cobble
    ivec2( 4,  0), // planks
    ivec2(15,  0), // oak sampling
    ivec2( 1,  1), // bedrock
    ivec2(15, 13), // water
    ivec2(15, 13), // water
    ivec2(15, 15), // lava
    ivec2(15, 15), // lava
    ivec2( 2,  1), // sand
    ivec2( 3,  1), // gravel
    ivec2( 0,  2), // gold ore
    ivec2( 1,  2), // iron ore
    ivec2( 2,  2), // coal ore
    ivec2( 4,  1), // oak log
    ivec2( 4,  3), // oak leaves
    ivec2( 0,  3), // sponge
    ivec2( 1,  3), // glass
    ivec2( 0, 10), // lapis ore
    ivec2( 0,  9), // lapis block
    ivec2(14,  2), // dispenser
    ivec2( 0, 12), // sandstone
    ivec2(10,  4), // noteblock
    ivec2( 8,  8), // bed
    ivec2( 3, 11), // powered rail
    ivec2( 3, 12), // detector rail
    ivec2(10,  6), // sticky piston
    ivec2(11,  0), // cobweb
    ivec2( 7,  2), // tall grass
    ivec2( 7,  3), // deadbush
    ivec2(11,  6), // piston
    ivec2(12,  6), // piston head?
    ivec2( 0,  4), // wool
    ivec2( 0,  0), // top grass block thing?
    ivec2(13,  0), // yellow flower
    ivec2(12,  0), // red flower
    ivec2(13,  1), // brown mushroom
    ivec2(12,  1), // red mushroom
    ivec2( 7,  1), // gold block
    ivec2( 6,  1), // iron block
    ivec2( 5,  0), // double slab
    ivec2( 6,  0), // single slab
    ivec2( 7,  0), // bricks
    ivec2( 8,  0), // tnt
    ivec2( 3,  2), // bookshelf
    ivec2( 4,  2), // mossy cobblestone
    ivec2( 5,  2), // obsidian
    ivec2( 0,  5), // torch
    ivec2( 1, 15), // fire
    ivec2( 1,  5), // spawner
    ivec2( 4,  0), // wooden stairs
    ivec2(11,  1), // chest
    ivec2( 4, 10), // redstone dust
    ivec2( 2,  3), // diamond ore
    ivec2( 8,  1), // diamond block
    ivec2(11,  3), // workbench
    ivec2(15,  5), // crops
    ivec2( 2,  0), // farmland
    ivec2(12,  2), // furnace
    ivec2(13,  3), // burning furnace
    ivec2(14,  1), // ?
    ivec2( 1,  5), // door
    ivec2( 3,  5), // ladder
    ivec2( 0,  9), // rail
    ivec2( 0,  1), // cobblestone stairs
    ivec2(14,  1), // ?
    ivec2( 0,  7), // lever
    ivec2( 1,  0), // stone pressure plate
    ivec2( 1,  6), // iron door
    ivec2( 4,  0), // wooden pressure plate
    ivec2( 3,  3), // redstone ore
    ivec2( 3,  3), // redstone ore 2
    ivec2( 3,  7), // disabled redstone torch
    ivec2( 3,  6), // redstone torch
    ivec2( 1,  0), // stone button
    ivec2( 2,  4), // snow layer
    ivec2( 3,  4), // ice
    ivec2( 2,  4), // snow
    ivec2( 6,  4), // cactus
    ivec2( 8,  4), // clay
    ivec2( 9,  4), // reeds
    ivec2(10,  4), // jukebox
    ivec2( 4,  0), // fence
    ivec2( 7,  7), // pumpkin
    ivec2( 8,  6), // netherrack
    ivec2( 9,  6), // soul sand
    ivec2( 9,  6), // glowstone
    ivec2(14,  1), // portal todo
    ivec2( 7,  7), // jack'o lantern
    ivec2(10,  7), // cake
    ivec2(14,  1), // ?
    ivec2(14,  1), // ?
    ivec2(11,  1), // chest (the april fools one)
    ivec2( 4,  5)  // trapdoor
);

#define _0px 0.0f
#define _1px (1.0f / 16.0f)
#define _2px (_1px * 2)
#define _3px (_1px * 3)
#define _4px (_1px * 4)
#define _5px (_1px * 5)
#define _6px (_1px * 6)
#define _7px (_1px * 7)
#define _8px (_1px * 8)
#define _9px (_1px * 9)
#define _10px (_1px * 10)
#define _11px (_1px * 11)
#define _12px (_1px * 12)
#define _13px (_1px * 13)
#define _14px (_1px * 14)
#define _15px (_1px * 15)
#define _16px 1.0f


#define GRASS_COLORMOD vec3(0.443f, 0.645f, 0.279f)

#define SIDE_Z_NEG 32
#define SIDE_Z_POS 16
#define SIDE_X_NEG 8
#define SIDE_X_POS 4
#define SIDE_Y_NEG 2
#define SIDE_Y_POS 1

#define SIDE_ALL (SIDE_X_POS|SIDE_X_NEG|SIDE_Y_NEG|SIDE_Y_POS|SIDE_Z_NEG|SIDE_Z_POS)

int get_block_render_type(int block_id)
{
    switch(block_id) {
    case 6:
    case 30:
    case 31:
    case 32:
    case 37:
    case 38:
    case 39:
    case 40:
    case 83:
        return 1;
    case 50:
    case 75:
    case 76:
        return 2;
    case 51:
        return 3;
    case 8:
    case 9:
    case 10:
    case 11:
        return 4;
    case 55:
        return 5;
    case 59:
        return 6;
    case 64:
    case 71:
        return 7;
    case 65:
        return 8;
    case 27:
    case 28:
    case 66:
        return 9;
    case 53:
    case 67:
        return 10;
    case 85:
        return 11;
    case 69:
        return 12;
    case 81:
        return 13;
    case 26:
        return 14;
    case 93:
    case 94:
        return 15;
    case 29:
    case 33:
        return 16;
    case 34:
        return 17;
    case 36:
    case 63:
    case 68:
        return -1;
    }
    return 0;
}

vec2 face_base(int block_id, int light_level, int metadata)
{
    COLORMOD = vec4(1);

    if(block_id == 31 || block_id == 18) {
        // tall grass/fern and leaves get foliage color on all sides
        COLORMOD.rgb = GRASS_COLORMOD;
    }

    COLORMOD.rgb *= float(light_level) / 15.0f;// * NIGHTTIME_LIGHT_MODIFIER / 15.0f;


    if(block_id == 35) { // wool
        if(metadata == 0) {
            return vec2(block_textures[block_id]) / 16.0f;
        } else {
            int x_off = (metadata < 8) ? (2) : (1);
            int y_off = (14 - metadata) + ((metadata < 8) ? (0) : (8));
            return vec2(ivec2(x_off, y_off)) / 16.0f;
        }
    }
    if(0 <= block_id && block_id <= NUM_BLOCKS - 1) {
        return vec2(block_textures[block_id]) / 16.0f;
    }
    return vec2(block_textures[0]) / 16.0f;
}

void face_yp(int block_id, int metadata, int light_level)
{
    UV_BLOCK = face_base(block_id, light_level, metadata);
    
    switch(block_id) {
    case 2: // grass
        COLORMOD.rgb *= GRASS_COLORMOD;
        UV_BLOCK = vec2(0, 0);
        break;
    case 24: // sandstone
    case 58: // workbench
    case 86: // pumpkin
    case 91: // jack'o lantern
        UV_BLOCK.y -= _1px;
        break;
    case 23: // dispenser
    case 61: // furnace
    case 62: // burning furnace
        UV_BLOCK = vec2(14, 3) * _1px;
        break;
    case 54: // chest
    case 95: // prank chest
        UV_BLOCK.x -= 2 * _1px;
        break;
    case 47: // bookshelf
        UV_BLOCK = vec2(4, 0) * _1px;
        break;
    case 60: // farmland
        UV_BLOCK = vec2(7, 5) * _1px;
        if(metadata > 0) {
            UV_BLOCK.x -= _1px;
        }
        break;
    case 81: // cactus
        UV_BLOCK.x -= _1px;
        break;
    case 17: // log
    case 46: // tnt
    case 84: // jukebox
        UV_BLOCK.x += _1px;
        break;
    }
}

void face_yn(int block_id, int metadata, int light_level)
{
    UV_BLOCK = face_base(block_id, light_level, metadata);
    switch(block_id) {
    case 2: // grass
        UV_BLOCK = vec2(2, 0) * _1px;
        break;
    case 24: // sandstone
        UV_BLOCK.y += _1px;
        break;
    case 86: // pumpkin
    case 91: // jack'o lantern
        UV_BLOCK.y -= _1px;
        break;
    case 23: // dispenser
    case 61: // furnace
    case 62: // burning furnace
        UV_BLOCK = vec2(14, 3) * _1px;
        break;
    case 54: // chest
    case 95: // prank chest
        UV_BLOCK.x -= 2 * _1px;
        break;
    case 47: // bookshelf
    case 58: // workbench
        UV_BLOCK = vec2(4, 0) * _1px;
        break;
    case 81: // cactus
    case 17: // log
        UV_BLOCK.x += _1px;
        break;
    case 46: // tnt
        UV_BLOCK.x += 2 * _1px;
        break;
    case 92: // cake
        UV_BLOCK.x += 3 * _1px;
        break;
    }
    COLORMOD.rgb *= 0.4f;
}

void face_xp(int block_id, int metadata, int light_level)
{
    UV_BLOCK = face_base(block_id, light_level, metadata);
    COLORMOD.rgb *= 0.8f;
}

void face_xn(int block_id, int metadata, int light_level)
{
    UV_BLOCK = face_base(block_id, light_level, metadata);
    COLORMOD.rgb *= 0.8f;
}

void face_zp(int block_id, int metadata, int light_level)
{
    UV_BLOCK = face_base(block_id, light_level, metadata);
    COLORMOD.rgb *= 0.6f;
}

void face_zn(int block_id, int metadata, int light_level)
{
    UV_BLOCK = face_base(block_id, light_level, metadata);
    COLORMOD.rgb *= 0.6f;
}

void vert(float ox, float oy, float oz, vec2 uv)
{
    gl_Position = PROJECTION * VIEW * (vec4(gl_in[0].gl_Position.xyz, 1.0f) + vec4(ox - 0.5f, oy - 0.5f, oz - 0.5f, 0.0f));
    UV_BASE = uv / 16.0f;
    EmitVertex();
}

void vert2(float ox, float oy, float oz, vec2 uv)
{
    vec3 op = gl_in[0].gl_Position.xyz;
    int x = int(op.x), y = int(op.y), z = int(op.z);
    int hash = (x * 3129871) ^ (z * 116129781) ^ y;
    hash = hash * hash * 42317861 + hash * 11;

    op.x += (float(hash >> 16 & 15) / 15.0f - 0.5f) * 0.5f;
    op.y += (float(hash >> 20 & 15) / 15.0f - 1.0f) * 0.2f;
    op.z += (float(hash >> 24 & 15) / 15.0f - 0.5f) * 0.5f;

    gl_Position = PROJECTION * VIEW * (vec4(op, 1.0f) + vec4(ox - 0.5f, oy - 0.5f, oz - 0.5f, 0.0f));
    UV_BASE = uv / 16.0f;
    EmitVertex();
}

void render_block_standard_impl(vec3 min, vec3 max, int block_id, int metadata, int[7] light, int sides)
{
    if((sides & SIDE_Z_NEG) != 0 || min.z != 0.0f) {
        float u1 = min.x;
        float v1 = min.y;
        float u2 = max.x;
        float v2 = max.y;
        /* z- */
        face_zn(block_id, metadata, light[5]);
        vert(min.x, min.y, min.z, vec2(u2, v2));
        vert(min.x, max.y, min.z, vec2(u2, v1));
        vert(max.x, min.y, min.z, vec2(u1, v2));
        vert(max.x, max.y, min.z, vec2(u1, v1));
        EndPrimitive();
    }
    if((sides & SIDE_Z_POS) != 0 || max.z != 1.0f) {
        float u1 = min.x;
        float v1 = min.y;
        float u2 = max.x;
        float v2 = max.y;
        /* z+ */
        face_zp(block_id, metadata, light[4]);
        vert(min.x, min.y, max.z, vec2(u1, v2));
        vert(max.x, min.y, max.z, vec2(u2, v2));
        vert(min.x, max.y, max.z, vec2(u1, v1));
        vert(max.x, max.y, max.z, vec2(u2, v1));
        EndPrimitive();
    }
    if((sides & SIDE_X_NEG) != 0 || min.x != 0.0f) {
        float u1 = min.z;
        float v1 = min.y;
        float u2 = max.z;
        float v2 = max.y;
        /* x- */
        face_xn(block_id, metadata, light[3]);
        vert(min.x, min.y, min.z, vec2(u1, v2));
        vert(min.x, min.y, max.z, vec2(u2, v2));
        vert(min.x, max.y, min.z, vec2(u1, v1));
        vert(min.x, max.y, max.z, vec2(u2, v1));
        EndPrimitive();
    }
    if((sides & SIDE_X_POS) != 0 || max.x != 1.0f) {
        float u1 = min.z;
        float v1 = min.y;
        float u2 = max.z;
        float v2 = max.y;
        /* x+ */
        face_xp(block_id, metadata, light[2]);
        vert(max.x, min.y, min.z, vec2(u2, v2));
        vert(max.x, max.y, min.z, vec2(u2, v1));
        vert(max.x, min.y, max.z, vec2(u1, v2));
        vert(max.x, max.y, max.z, vec2(u1, v1));
        EndPrimitive();
    }
    if((sides & SIDE_Y_NEG) != 0 || min.y != 0.0f) {
        float u1 = min.x;
        float v1 = min.z;
        float u2 = max.x;
        float v2 = max.z;
        /* y- */
        face_yn(block_id, metadata, light[1]);
        vert(min.x, min.y, min.z, vec2(u1, v1));
        vert(max.x, min.y, min.z, vec2(u2, v1));
        vert(min.x, min.y, max.z, vec2(u1, v2));
        vert(max.x, min.y, max.z, vec2(u2, v2));
        EndPrimitive();
    }
    if((sides & SIDE_Y_POS) != 0 || max.y != 1.0f) {
        float u1 = min.x;
        float v1 = min.z;
        float u2 = max.x;
        float v2 = max.z;
        /* y+ */
        face_yp(block_id, metadata, light[0]);
        vert(min.x, max.y, min.z, vec2(u1, v1));
        vert(min.x, max.y, max.z, vec2(u1, v2));
        vert(max.x, max.y, min.z, vec2(u2, v1));
        vert(max.x, max.y, max.z, vec2(u2, v2));
        EndPrimitive();
    }
}

void render_block_standard(int block_id, int metadata, int[7] light, int sides)
{
    vec3 min = vec3(0.0f);
    vec3 max = vec3(1.0f);

    if (block_id == 44) {
        // single slab
       // max.y = 0.5f;
    } else if (block_id == 70 || block_id == 72) {
        // pressure plates
        min.x = _1px;
        min.z = _1px;
        max.x = _15px;
        max.y = _1px * (metadata == 1 ? 0.5f : 1.0f); // half size if pressed
        max.z = _15px;
        light[5] = light[4] = light[3] = light[2] = light[1] = light[0] = light[6];
    } else if (block_id == 77) {
        // button
        bool pressed = (metadata & 8) != 0;
        int side = metadata & 7;
        float ymin = _6px;
        float ymax = _10px;
        float side1 = _3px;
        float side2 = (pressed ? 1.0f : 2.0f) / 16.0f;
        if (side == 1) {
            min = vec3(0.0f, ymin, 0.5f - side1);
            max = vec3(side2, ymax, 0.5f + side1);
        } else if (side == 2) {
            min = vec3(1.0f - side2, ymin, 0.5f - side1);
            max = vec3(1.0f, ymax, 0.5f + side1);
        } else if (side == 3) {
            min = vec3(0.5f - side1, ymin, 0.0f);
            max = vec3(0.5f + side1, ymax, side2);
        } else if (side == 4) {
            min = vec3(0.5f - side1, ymin, 1.0f - side2);
            max = vec3(0.5f + side1, ymax, 1.0f);
        }
        light[5] = light[4] = light[3] = light[2] = light[1] = light[0] = light[6];
    }

    render_block_standard_impl(min, max, block_id, metadata, light, sides);
}

void render_block_cross(int block_id, int metadata, int light_level)
{
    vec3 min = vec3(0.0f);
    vec3 max = vec3(1.0f);
    float u1 = min.x;
    float v1 = min.y;
    float u2 = max.x;
    float v2 = max.y;

    UV_BLOCK = face_base(block_id, light_level, metadata);

    if(block_id == 31) {
        vert2(min.x, min.y, min.z, vec2(u2, v2));
        vert2(min.x, max.y, min.z, vec2(u2, v1));
        vert2(max.x, min.y, max.z, vec2(u1, v2));
        vert2(max.x, max.y, max.z, vec2(u1, v1));
        EndPrimitive();
        vert2(min.x, min.y, max.z, vec2(u1, v2));
        vert2(max.x, min.y, min.z, vec2(u2, v2));
        vert2(min.x, max.y, max.z, vec2(u1, v1));
        vert2(max.x, max.y, min.z, vec2(u2, v1));
        EndPrimitive();
        vert2(min.x, min.y, min.z, vec2(u1, v2));
        vert2(max.x, min.y, max.z, vec2(u2, v2));
        vert2(min.x, max.y, min.z, vec2(u1, v1));
        vert2(max.x, max.y, max.z, vec2(u2, v1));
        EndPrimitive();
        vert2(min.x, min.y, max.z, vec2(u2, v2));
        vert2(min.x, max.y, max.z, vec2(u2, v1));
        vert2(max.x, min.y, min.z, vec2(u1, v2));
        vert2(max.x, max.y, min.z, vec2(u1, v1));
        EndPrimitive();
    } else {
        vert(min.x, min.y, min.z, vec2(u2, v2));
        vert(min.x, max.y, min.z, vec2(u2, v1));
        vert(max.x, min.y, max.z, vec2(u1, v2));
        vert(max.x, max.y, max.z, vec2(u1, v1));
        EndPrimitive();
        vert(min.x, min.y, max.z, vec2(u1, v2));
        vert(max.x, min.y, min.z, vec2(u2, v2));
        vert(min.x, max.y, max.z, vec2(u1, v1));
        vert(max.x, max.y, min.z, vec2(u2, v1));
        EndPrimitive();
        vert(min.x, min.y, min.z, vec2(u1, v2));
        vert(max.x, min.y, max.z, vec2(u2, v2));
        vert(min.x, max.y, min.z, vec2(u1, v1));
        vert(max.x, max.y, max.z, vec2(u2, v1));
        EndPrimitive();
        vert(min.x, min.y, max.z, vec2(u2, v2));
        vert(min.x, max.y, max.z, vec2(u2, v1));
        vert(max.x, min.y, min.z, vec2(u1, v2));
        vert(max.x, max.y, min.z, vec2(u1, v1));
        EndPrimitive();
    }
}

void render_block_torch(int block_id, int metadata, int light_level)
{
    float skew_x = 0.0f;
    float skew_z = 0.0f;

    float x = 0.5f;
    float y = 0.2f;
    float z = 0.5f;

    UV_BLOCK = face_base(block_id, light_level, metadata);

    if(metadata == 1) {
        skew_x = -0.4f;
        x -= 0.1f;
    } else if(metadata == 2) {
        skew_x = 0.4f;
        z += 0.1f;
    } else if(metadata == 3) {
        skew_z = -0.4f;
        z -= 0.1f;
    } else if(metadata == 4) {
        skew_z = 0.4f;
        z += 0.1f;
    } else {
        y = 0.0f;
    }

    float x_mhalf = x - 0.5f;
    float x_phalf = x + 0.5f;
    float z_mhalf = z - 0.5f;
    float z_phalf = z + 0.5f;

    // +y
    vert(x + skew_x * 0.375f - _1px, y + 0.625f, z + skew_z * 0.375f - _1px, vec2(_7px, _6px));
    vert(x + skew_x * 0.375f - _1px, y + 0.625f, z + skew_z * 0.375f + _1px, vec2(_7px, _8px));
    vert(x + skew_x * 0.375f + _1px, y + 0.625f, z + skew_z * 0.375f - _1px, vec2(_9px, _6px));
    vert(x + skew_x * 0.375f + _1px, y + 0.625f, z + skew_z * 0.375f + _1px, vec2(_9px, _8px));
    EndPrimitive();

    // -x
    vert(x - _1px         , y + 1.0f, z_mhalf         , vec2(0, 0));
    vert(x - _1px + skew_x, y + 0.0f, z_mhalf + skew_z, vec2(0, 1));
    vert(x - _1px         , y + 1.0f, z_phalf         , vec2(1, 0));
    vert(x - _1px + skew_x, y + 0.0f, z_phalf + skew_z, vec2(1, 1));
    EndPrimitive();

    // +x
    vert(x + _1px         , y + 1.0f, z_phalf         , vec2(0, 0));
    vert(x + _1px + skew_x, y + 0.0f, z_phalf + skew_z, vec2(0, 1));
    vert(x + _1px         , y + 1.0f, z_mhalf         , vec2(1, 0));
    vert(x + _1px + skew_x, y + 0.0f, z_mhalf + skew_z, vec2(1, 1));
    EndPrimitive();

    // -z
    vert(x_mhalf + skew_x, y + 0.0f, z - _1px + skew_z, vec2(1, 1));
    vert(x_mhalf         , y + 1.0f, z - _1px         , vec2(1, 0));
    vert(x_phalf + skew_x, y + 0.0f, z - _1px + skew_z, vec2(0, 1));
    vert(x_phalf         , y + 1.0f, z - _1px         , vec2(0, 0));
    EndPrimitive();

    // +z
    vert(x_phalf + skew_x, y + 0.0f, z + _1px + skew_z, vec2(1, 1));
    vert(x_phalf         , y + 1.0f, z + _1px         , vec2(1, 0));
    vert(x_mhalf + skew_x, y + 0.0f, z + _1px + skew_z, vec2(0, 1));
    vert(x_mhalf         , y + 1.0f, z + _1px         , vec2(0, 0));
    EndPrimitive();
}

void render_block_fence(int block_id, int metadata, int light_self)
{
    vec3 min = vec3(_6px, _0px, _6px);
    vec3 max = vec3(_10px, _16px, _10px);
    int[7] light = int[7](light_self, light_self, light_self, light_self, light_self, light_self, light_self);
    int sides = SIDE_ALL;

    render_block_standard_impl(min, max, block_id, metadata, light, sides);

    if((metadata & 1) != 0) {
        // connected to -x
        int s = SIDE_ALL - SIDE_X_POS;
        min = vec3(0.0, _12px, _7px);
        max = vec3(0.5, _15px, _9px);
        if((metadata & 2) != 0) {
            // also connected to +x
            max.x = 1.0f;
            s |= SIDE_X_POS;
            metadata &= ~2;
        }
        render_block_standard_impl(min, max, block_id, metadata, light, s);
    }

    if((metadata & 2) != 0) {
        // connected to +x
        int s = SIDE_ALL - SIDE_X_NEG;
        min = vec3(0.5, _12px, _7px);
        max = vec3(1.0, _15px, _9px);
        render_block_standard_impl(min, max, block_id, metadata, light, s);
    }

    if((metadata & 4) != 0) {
        // connected to -z
        int s = SIDE_ALL - SIDE_Z_POS;
        min = vec3(_7px, _12px, 0.0);
        max = vec3(_9px, _15px, 0.5);
        if((metadata & 8) != 0) {
            // also connected to +x
            max.z = 1.0f;
            s |= SIDE_Z_POS;
            metadata &= ~8;
        }
        render_block_standard_impl(min, max, block_id, metadata, light, s);
    }

    if((metadata & 8) != 0) {
        // connected to +z
        int s = SIDE_ALL - SIDE_Z_NEG;
        min = vec3(_7px, _12px, 0.5);
        max = vec3(_9px, _15px, 1.0);
        render_block_standard_impl(min, max, block_id, metadata, light, s);
    }
}

void main()
{
    int d1 = int(DATA[0].x);
    int d2 = int(DATA[0].y);

    int sides = (d1 >>  0) & 63;
    int block_id = (d1 >>  6) & 255;
    int metadata = (d1 >> 14) & 15;
    int light_self = (d1 >> 18) & 15;

    int l = d2;
    int[7] light = int[7](
        (l >>  0) & 15,
        (l >>  4) & 15,
        (l >>  8) & 15,
        (l >> 12) & 15,
        (l >> 16) & 15,
        (l >> 20) & 15,
        light_self
    );

    COLORMOD = vec4(1);

    render_block_standard(block_id, metadata, light, sides);
    //int rt = get_block_render_type(block_id);
   /*switch(rt) {
        case 0: render_block_standard(block_id, metadata, light, sides); break;
        case 1: render_block_cross(block_id, metadata, light_self); break;
        case 2: render_block_torch(block_id, metadata, light_self); break;
//        case 3: render_block_standard(block_id, metadata, light, sides); break;
//        case 4: render_block_standard(block_id, metadata, light, sides); break;
//        case 5: render_block_standard(block_id, metadata, light, sides); break;
//        case 6: render_block_standard(block_id, metadata, light, sides); break;
//        case 7: render_block_standard(block_id, metadata, light, sides); break;
//        case 8: render_block_standard(block_id, metadata, light, sides); break;
//        case 9: render_block_standard(block_id, metadata, light, sides); break;
//        case 10: render_block_standard(block_id, metadata, light, sides); break;
        case 11: render_block_fence(block_id, metadata, light_self); break;
//        case 12: render_block_standard(block_id, metadata, light, sides); break;
//        case 13: render_block_standard(block_id, metadata, light, sides); break;
//        case 14: render_block_standard(block_id, metadata, light, sides); break;
//        case 15: render_block_standard(block_id, metadata, light, sides); break;
//        case 16: render_block_standard(block_id, metadata, light, sides); break;
//        case 17: render_block_standard(block_id, metadata, light, sides); break;
    }*/
}