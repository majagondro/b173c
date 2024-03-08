#include <zlib.h>
#include "world.h"
#include "client/console.h"
#include "hashmap.c/hashmap.h"
#include "client/client.h"
#include "client/cvar.h"
#include "entity.h"
#include "block.h"
#include "mathlib.h"

static const block_data AIR_BLOCK_DATA = {.id = 0, .metadata = 0, .skylight = 15, .blocklight = 0};
static const block_data EMPTY_BLOCK_DATA = {.id = 0, .metadata = 0, .skylight = 0, .blocklight = 0};
static const block_data SOLID_BLOCK_DATA = {.id = 1, .metadata = 0, .skylight = 0, .blocklight = 0};

struct hashmap *world_chunk_map = NULL;
struct hashmap *world_entity_map = NULL;

/* functions for the hashmaps */
int chunk_compare(const void *a, const void *b, void *udata attr(unused))
{
    const world_chunk *ca = a, *cb = b;
    return !(ca->x == cb->x && ca->z == cb->z);
}

uint64_t chunk_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const world_chunk *c = item;
    struct {
        int x, z;
    } coords = {c->x, c->z};
    return hashmap_xxhash3(&coords, sizeof(coords), seed0, seed1);
}

void chunk_free(void *c)
{
    world_chunk *chunk = c;

    world_free_chunk_glbufs(chunk);
    mem_free(chunk->data);
}

int entity_compare(const void *a, const void *b, void *udata attr(unused))
{
    const entity *ent1 = a, *ent2 = b;
    return ent1->id != ent2->id;
}

uint64_t entity_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const entity *ent = item;
    return hashmap_xxhash3(&ent->id, sizeof(ent->id), seed0, seed1);
}

void entity_free(void *c)
{
    entity *ent = c;
    mem_free(ent->name);
}

errcode world_init(void)
{
    world_chunk_map = hashmap_new(sizeof(world_chunk), 0, 0, 0, chunk_hash, chunk_compare, chunk_free, NULL);
    world_entity_map = hashmap_new(sizeof(entity), 0, 0, 0, entity_hash, entity_compare, entity_free, NULL);

    if(world_chunk_map == NULL || world_entity_map == NULL)
        return ERR_FATAL;
    return ERR_OK;
}

void world_shutdown(void)
{
    hashmap_free(world_chunk_map);
    hashmap_free(world_entity_map);
    world_chunk_map = NULL;
    world_entity_map = NULL;
}

void world_cleanup(void)
{
    hashmap_clear(world_chunk_map, true);
    hashmap_clear(world_entity_map, true);
}

void world_alloc_chunk(int chunk_x, int chunk_z)
{
    world_chunk chunk = {0};

    if(world_chunk_exists(chunk_x, chunk_z))
        world_free_chunk(chunk_x, chunk_z);

    chunk.x = chunk_x;
    chunk.z = chunk_z;
    chunk.data = mem_alloc(WORLD_CHUNK_SIZE * WORLD_CHUNK_SIZE * WORLD_CHUNK_HEIGHT * sizeof(block_data));
    world_init_chunk_glbufs(&chunk);

    /* the chunk is copied by hashmap_set, so it is fine to allocate it on the stack */
    hashmap_set(world_chunk_map, &chunk);
}

void world_free_chunk(int chunk_x, int chunk_z)
{
    world_chunk key = {.x = chunk_x, .z = chunk_z};
    world_chunk *value;

    value = (world_chunk *) hashmap_get(world_chunk_map, &key);
    if(!value)
        return;

    chunk_free(value);

    hashmap_delete(world_chunk_map, &key);
}

bool world_chunk_exists(int chunk_x, int chunk_z)
{
    return world_get_chunk(chunk_x, chunk_z) != NULL;
}

world_chunk *world_get_chunk(int chunk_x, int chunk_z)
{
    world_chunk key = {.x = chunk_x, .z = chunk_z};
    return (world_chunk *) hashmap_get(world_chunk_map, &key);
}

void world_mark_region_for_remesh(int x_start, int y_start, int z_start, int x_end, int y_end, int z_end)
{
    int cxs = x_start >> 4;
    int cxe = x_end >> 4;
    int cys = y_start >> 4;
    int cye = y_end >> 4;
    int czs = z_start >> 4;
    int cze = z_end >> 4;
    for(int cx = cxs; cx <= cxe; cx++) {
        for(int cz = czs; cz <= cze; cz++) {
            for(int cy = cys; cy <= cye; cy++) {
                world_chunk *chunk = world_get_chunk(cx, cz);
                if(!chunk)
                    continue;
                chunk->gl.needs_remesh_simple = true;
                chunk->gl.needs_remesh_complex = true;
            }
        }
    }
}

void world_mark_all_for_remesh(void)
{
    void *it;
    size_t i = 0;
    while(hashmap_iter(world_chunk_map, &i, &it)) {
        world_chunk *chunk = it;
        chunk->gl.needs_remesh_simple = true;
        chunk->gl.needs_remesh_complex = true;
    }
}

static int inflate_data(ubyte *in, ubyte *out, size_t size_in, size_t size_out)
{
    int ret;
    z_stream strm;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if(ret != Z_OK) {
        con_printf("inflate_data: zlib error: %s\n", zError(ret));
        return ret;
    }

    strm.avail_in = size_in;
    strm.next_in = in;
    strm.avail_out = size_out;
    strm.next_out = out;

    ret = inflate(&strm, Z_NO_FLUSH);
    switch(ret) {
    case Z_NEED_DICT:
        ret = Z_DATA_ERROR;
        /* fall through */
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
        return ret;
    }

    inflateEnd(&strm);

    return Z_OK;
}

static int
world_set_chunk_data(world_chunk *chunk, const ubyte *data, int x_start, int y_start, int z_start, int x_end, int y_end,
                     int z_end, int data_pos)
{
    for(int x = x_start; x < x_end; x++) {
        for(int z = z_start; z < z_end; z++) {
            for(int y = y_start; y < y_end; y++, data_pos++) {
                int index = IDX_FROM_COORDS(x, y, z);
                chunk->data[index].id = data[data_pos];
            }
        }
    }

    for(int x = x_start; x < x_end; x++) {
        for(int z = z_start; z < z_end; z++) {
            bool next = false;
            for(int y = y_start; y < y_end; y++) {
                int index = IDX_FROM_COORDS(x, y, z);
                ubyte value = data[data_pos];
                chunk->data[index].metadata = !(y & 1) ? (value & 15) : ((value >> 4) & 15);
                if(next)
                    data_pos++;
                next = !next;
            }
        }
    }

    for(int x = x_start; x < x_end; x++) {
        for(int z = z_start; z < z_end; z++) {
            bool next = false;
            for(int y = y_start; y < y_end; y++) {
                int index = IDX_FROM_COORDS(x, y, z);
                ubyte value = data[data_pos];
                chunk->data[index].blocklight = !(y & 1) ? (value & 15) : ((value >> 4) & 15);
                if(next)
                    data_pos++;
                next = !next;
            }
        }
    }

    for(int x = x_start; x < x_end; x++) {
        for(int z = z_start; z < z_end; z++) {
            bool next = false;
            for(int y = y_start; y < y_end; y++) {
                int index = IDX_FROM_COORDS(x, y, z);
                ubyte value = data[data_pos];
                chunk->data[index].skylight = !(y & 1) ? (value & 15) : ((value >> 4) & 15);
                if(next)
                    data_pos++;
                next = !next;
            }
        }
    }

    return data_pos;
}

void world_load_compressed_chunk_data(int x, int y, int z, int sx, int sy, int sz, size_t size, ubyte *compressed)
{
    // 1 byte id, 0.5 byte metadata, 2x0.5 byte light data
    ubyte data[sx * sy * sz * 5 / 2];
    int i;
    int chunk_x_start = x >> 4;
    int chunk_z_start = z >> 4;
    int chunk_x_end = (x + sx - 1) >> 4;
    int chunk_z_end = (z + sz - 1) >> 4;
    int y_start, y_end;

    if((i = inflate_data(compressed, data, size, sizeof(data))) != Z_OK) {
        con_printf(CON_STYLE_RED"error while decompressing chunk data:"CON_STYLE_WHITE"%size\n",
                   zError(i)); // should this print?
        return;
    }

    y_start = y;
    y_end = y + sy;

    if(y_start < 0)
        y_start = 0;
    if(y_end > 128)
        y_end = 128;

    i = 0;
    for(int cx = chunk_x_start; cx <= chunk_x_end; cx++) {
        int x_start = x - cx * 16;
        int x_end = x + sx - cx * 16;

        if(x_start < 0)
            x_start = 0;
        if(x_end > 16)
            x_end = 16;

        for(int cz = chunk_z_start; cz <= chunk_z_end; cz++) {
            int z_start = z - cz * 16;
            int z_end = z + sz - cz * 16;

            if(z_start < 0)
                z_start = 0;
            if(z_end > 16)
                z_end = 16;

            /* should have been allocated by a PreChunk packet, but we check anyway */
            if(!world_chunk_exists(cx, cz))
                world_alloc_chunk(cx, cz);

            world_mark_region_for_remesh(cx * 16 + x_start - 1, y_start - 1, cz * 16 + z_start - 1, cx * 16 + x_end + 1,
                                         y_end + 1, cz * 16 + z_end + 1);
            i = world_set_chunk_data(world_get_chunk(cx, cz), data, x_start, y_start, z_start, x_end, y_end, z_end, i);
        }
    }
}

block_data world_get_blockf(float x, float y, float z)
{
    return world_get_block((int) floorf(x), (int) floorf(y), (int) floorf(z));
}

block_data world_get_block(int x, int y, int z)
{
    static struct {
        int x, z;
        world_chunk *chunk;
    } cache;

    if(y < 0) // below the world
        return SOLID_BLOCK_DATA; // returning solid helps avoid creating mesh faces which won't be seen
    if(y >= 128) // way up in the sky
        return AIR_BLOCK_DATA;

    if(cache.x != (x >> 4) || cache.z != (z >> 4)) {
        cache.x = (x >> 4);
        cache.z = (z >> 4);
        cache.chunk = world_get_chunk(cache.x, cache.z);
    }

    if(cache.chunk != NULL)
        return cache.chunk->data[IDX_FROM_COORDS(x, y, z)];
    return EMPTY_BLOCK_DATA;
}

block_data world_get_block_fast(world_chunk *chunk, int x, int y, int z)
{
    if(y < 0) // below the world
        return SOLID_BLOCK_DATA; // returning solid helps avoid creating mesh faces which won't be seen
    if(y >= 128) // way up in the sky
        return AIR_BLOCK_DATA;

    if(chunk != NULL)
        return chunk->data[IDX_FROM_COORDS(x, y, z)];
    return EMPTY_BLOCK_DATA;
}

bool block_should_face_be_rendered_fast(world_chunk *chunk, int x, int y, int z, block_data self, block_face face)
{
    vec3_t face_offsets[6] = {
            [BLOCK_FACE_Y_NEG] = vec3(0, -1, 0),
            [BLOCK_FACE_Y_POS] = vec3(0, 1, 0),
            [BLOCK_FACE_Z_NEG] = vec3(0, 0, -1),
            [BLOCK_FACE_Z_POS] = vec3(0, 0, 1),
            [BLOCK_FACE_X_NEG] = vec3(-1, 0, 0),
            [BLOCK_FACE_X_POS] = vec3(1, 0, 0),
    };

    block_data other;
    vec3_t pos2 = vec3(x, y, z);
    pos2 = vec3_add(pos2, face_offsets[face]);

    other = world_get_block_fast(chunk, (int) pos2.x, (int) pos2.y, (int) pos2.z);

    if(block_is_semi_transparent(self) && block_is_semi_transparent(other))
        return false;

    if(self.id != BLOCK_LEAVES || r_smartleaves.integer)
        if(other.id == self.id)
            return false;

    if(block_get_properties(self.id).render_type == RENDER_FLUID) {
        if(face == BLOCK_FACE_Y_POS) {
            return true;
        }
    }
    return block_is_transparent(other);
}

void add_stair_bboxes(bbox_t *colliders, size_t *n_colliders, block_data block, int x, int y, int z)
{
    int facing = block.metadata;
    switch(facing) {
    case 0:
        colliders[(*n_colliders)++] = (bbox_t) {vec3(x, y, z), vec3(x + 0.5f, y + 0.5f, z + 1)};
        colliders[(*n_colliders)++] = (bbox_t) {vec3(x + 0.5f, y, z), vec3(x + 1, y + 1, z + 1)};
        break;
    case 1:
        colliders[(*n_colliders)++] = (bbox_t) {vec3(x, y, z), vec3(x + 0.5f, y + 1, z + 1)};
        colliders[(*n_colliders)++] = (bbox_t) {vec3(x + 0.5f, y, z), vec3(x + 1, y + 0.5f, z + 1)};
        break;
    case 2:
        colliders[(*n_colliders)++] = (bbox_t) {vec3(x, y, z), vec3(x + 1, y + 0.5f, z + 0.5f)};
        colliders[(*n_colliders)++] = (bbox_t) {vec3(x, y, z + 0.5f), vec3(x + 1, y + 1, z + 1)};
        break;
    case 3:
        colliders[(*n_colliders)++] = (bbox_t) {vec3(x, y, z), vec3(x + 1, y + 1, z + 0.5f)};
        colliders[(*n_colliders)++] = (bbox_t) {vec3(x, y, z + 0.5f), vec3(x + 1, y + 0.5f, z + 1)};
        break;
    default:
        break;
    }

}

bbox_t *world_get_colliding_blocks(bbox_t box)
{
    static bbox_t colliders[64] = {0};
    size_t n_colliders = 0;

    int x0 = (int) floorf(box.mins.x);
    int y0 = (int) floorf(box.mins.y - 0.01f);
    int z0 = (int) floorf(box.mins.z);
    int x1 = (int) floorf(box.maxs.x + 1.0f);
    int y1 = (int) floorf(box.maxs.y + 1.0f);
    int z1 = (int) floorf(box.maxs.z + 1.0f);

    memset(colliders, 0, sizeof(colliders));

    for(int x = x0; x < x1; x++) {
        for(int z = z0; z < z1; z++) {
            for(int y = y0 - 1; y < y1; y++) {
                block_data block = world_get_block(x, y, z);
                if(block_is_collidable(block)) {
                    if((block.id == BLOCK_STAIRS_STONE || block.id == BLOCK_STAIRS_WOOD) && n_colliders < 62) {
                        add_stair_bboxes(colliders, &n_colliders, block, x, y, z);
                    } else {
                        bbox_t bb = block_get_bbox(block, x, y, z, false);
                        if(!bbox_null(bb)) {
                            colliders[n_colliders++] = bb;
                        }
                    }
                    if(n_colliders >= 63) {
                        goto end; // realistically shouldn't happen
                    }
                }
            }
        }
    }

    end:
    colliders[n_colliders] = (bbox_t) {vec3_1(-1), vec3_1(-1)};
    return colliders;
}

ubyte world_get_block_lighting_ex(int x, int y, int z, bool recurse);
static ubyte chunk_get_block_lighting(world_chunk *c, int x, int y, int z, bool recurse)
{
    int i = IDX_FROM_COORDS(x, y, z);
    block_data block = c->data[i];
    int sl, bl;
    if(recurse && (block.id == BLOCK_SLAB_SINGLE || block.id == BLOCK_FARMLAND ||
       block.id == BLOCK_STAIRS_WOOD || block.id == BLOCK_STAIRS_STONE)) {
        int y1 = world_get_block_lighting_ex((c->x << 4) + x, y + 1, (c->z << 4) + z, false);
        int x1 = world_get_block_lighting_ex((c->x << 4) + x + 1, y, (c->z << 4) + z, false);
        int x0 = world_get_block_lighting_ex((c->x << 4) + x - 1, y, (c->z << 4) + z, false);
        int z1 = world_get_block_lighting_ex((c->x << 4) + x, y,     (c->z << 4) + z + 1, false);
        int z0 = world_get_block_lighting_ex((c->x << 4) + x, y,     (c->z << 4) + z - 1, false);
        if(x1 > y1)
            y1 = x1;
        if(x0 > y1)
            y1 = x0;
        if(z1 > y1)
            y1 = z1;
        if(z0 > y1)
            y1 = z0;
        return y1;
    }
    sl = block.skylight;
    bl = block.blocklight;
    return (bl > sl ? bl : sl) & 15;
}

ubyte world_get_block_lighting_ex(int x, int y, int z, bool recurse)
{
    world_chunk *cp;
    if(y < 0)
        return 0;
    if(y >= 128)
        return 15;
    cp = world_get_chunk(x >> 4, z >> 4);
    if(!cp)
        return 0;
    return chunk_get_block_lighting(cp, x & 15, y, z & 15, recurse);
}

ubyte world_get_block_lighting(int x, int y, int z)
{
    return world_get_block_lighting_ex(x, y, z, true);
}

ubyte world_get_block_lighting_fast(block_data block, int x, int y, int z)
{
    int sl, bl;
    if(block.id == BLOCK_SLAB_SINGLE || block.id == BLOCK_FARMLAND ||
       block.id == BLOCK_STAIRS_WOOD || block.id == BLOCK_STAIRS_STONE) {
        return world_get_block_lighting_ex(x, y, z, true);
    }
    sl = block.skylight;
    bl = block.blocklight;
    return (bl > sl ? bl : sl) & 15;
}

void world_set_block(int x, int y, int z, block_data data)
{
    world_chunk *chunk;

    if(y < 0 || y >= 128)
        return;

    chunk = world_get_chunk(x >> 4, z >> 4);
    if(!chunk)
        return;

    world_mark_region_for_remesh(x - 1, y - 1, z - 1, x + 1, y + 1, z + 1);

    chunk->data[IDX_FROM_COORDS(x, y, z)] = data;
}

void world_set_block_id(int x, int y, int z, block_id new_id)
{
    block_data data = world_get_block(x, y, z);
    data.id = new_id;
    world_set_block(x, y, z, data);
}

void world_set_block_metadata(int x, int y, int z, ubyte new_metadata)
{
    block_data data = world_get_block(x, y, z);
    data.metadata = new_metadata;
    world_set_block(x, y, z, data);
}

#define is_between(t, a, b) t >= a && t <= b

vec4_t world_calculate_sky_color(void)
{
    static vec4_t color = {.a = 1};

    int t = cl.game.time % 24000;

    if(is_between(t, 14000, 22000)) {
        // night
        color.r = 0.01f;
        color.g = 0.01f;
        color.b = 0.03f;
    } else if(is_between(t, 11000, 14000)) {
        // start of night
        // interpolate between day (11000) and night (14000)
        float f = (float) (t - 11000) / 3000.0f;
        color.r = f * 0.01f + (1.0f - f) * 0.55f;
        color.g = f * 0.01f + (1.0f - f) * 0.7f;
        color.b = f * 0.03f + (1.0f - f) * 1.0f;
    } else if(is_between(t, 22000, 24000)) {
        // start of day
        // interpolate between night (22000) and day (24000)
        float f = (float) (t - 22000) / 2000.0f;
        color.r = f * 0.55f + (1.0f - f) * 0.01f;
        color.g = f * 0.7f + (1.0f - f) * 0.01f;
        color.b = f * 1.0f + (1.0f - f) * 0.03f;
    } else {
        // day
        color.r = 0.55f;
        color.g = 0.7f;
        color.b = 1.0f;
    }

    return color;
}

float world_calculate_sun_angle(void)
{
    return 0.0f; // todo
}

float world_calculate_sky_light_modifier(void)
{
    float lmod;
    int64_t t = cl.game.time % 24000;
    if(is_between(t, 14000, 22000)) {
        // darkest night
        lmod = 1.0f;
    } else if(is_between(t, 11000, 14000)) {
        // start of night
        // interpolate between day (11000) and night (14000)
        float f = (float) (t - 11000) / 3000.0f;
        lmod = f * 1.0f + (1.0f - f) * 15.0f;
    } else if(is_between(t, 22000, 24000)) {
        // start of day
        // interpolate between night (22000) and day (24000)
        float f = (float) (t - 22000) / 2000.0f;
        lmod = f * 15.0f + (1.0f - f) * 1.0f;
    } else {
        // day
        lmod = 15.0f;
    }
    return lmod / 15.0f;
}

struct trace_result world_trace_ray(vec3_t origin, vec3_t dir, float maxlen)
{
    struct trace_result res = {0};
    float dist = 0.0f;
    int step[3];
    block_face ofsface[3];
    vec3_t delta = {0};
    vec3_t edgedist = {0};
    vec3_t end = vec3_add(origin, vec3_mul(dir, maxlen));

    for(int axis = 0; axis < 3; axis++) {
        delta.array[axis] = dir.array[axis] == 0.0f ? 9999.0f : fabsf(1.0f / dir.array[axis]);

        if(dir.array[axis] < 0.0f) {
            step[axis] = -1;
            ofsface[axis] = 1;
            edgedist.array[axis] = (origin.array[axis] - floorf(origin.array[axis])) * delta.array[axis];
        } else {
            step[axis] = 1;
            ofsface[axis] = 0;
            edgedist.array[axis] = (floorf(origin.array[axis]) + 1.0f - origin.array[axis]) * delta.array[axis];
        }
    }

    res.x = (int) floorf(origin.x);
    res.y = (int) floorf(origin.y);
    res.z = (int) floorf(origin.z);

    res.reached_end = true; // by default

    while(dist < maxlen) {
        if(edgedist.x < edgedist.y && edgedist.x < edgedist.z) {
            edgedist.x += delta.x;
            res.x += step[0];
            res.hit_face = BLOCK_FACE_X_NEG + ofsface[0];
        } else if(edgedist.y < edgedist.z) {
            edgedist.y += delta.y;
            res.y += step[1];
            res.hit_face = BLOCK_FACE_Y_NEG + ofsface[1];
        } else {
            edgedist.z += delta.z;
            res.z += step[2];
            res.hit_face = BLOCK_FACE_Z_NEG + ofsface[2];
        }

        // fixme
        dist = vec3_len(vec3_sub(vec3_add(res, vec3_1(0.5f)), origin));
        if(dist > maxlen)
            break;

        res.block = world_get_block(res.x, res.y, res.z);
        if(block_is_selectable(res.block)) {
            bbox_t bbox = block_get_bbox(res.block, res.x, res.y, res.z, true);
            if(bbox_intersects_line(bbox, origin, end)) {
                res.reached_end = false;
                break;
            }
        }

    }

    return res;
}

entity *world_get_entity(int entity_id)
{
    entity key = {.id = entity_id};
    return (entity *) hashmap_get(world_entity_map, &key);
}

void world_add_entity(entity *ent)
{
    entity_set_position(ent, ent->position); // update bbox
    hashmap_set(world_entity_map, ent);
    // in case the hashmap resizes, we need to find our player entity again
    // todo: maybe i can put this in entity_free() instead? (see top of this file)
    cl.game.our_ent = world_get_entity(cl.game.our_id);
}

void world_remove_entity(int entity_id)
{
    entity key = {.id = entity_id};
    hashmap_delete(world_entity_map, &key);
    // in case the hashmap resizes, we need to find our player entity again
    // todo: maybe i can put this in entity_free() instead? (see top of this file)
    cl.game.our_ent = world_get_entity(cl.game.our_id);
}
