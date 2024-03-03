#ifndef B173C_WORLD_H
#define B173C_WORLD_H

#include "common.h"
#include "mathlib.h"
#include "hashmap.c/hashmap.h"
#include "block.h"
#include "entity.h"

// todo: start using this xd
#define WORLD_CHUNK_SIZE   16
#define WORLD_CHUNK_HEIGHT 128

#define IDX_FROM_COORDS(x, y, z) ((((x) & 15) << 11) | (((z) & 15) << 7) | ((y) & 127))

typedef struct {
    int x, z;

    /* heap allocated, always has
     * WORLD_CHUNK_SIZE * WORLD_CHUNK_SIZE * WORLD_CHUNK_HEIGHT
     * elements */
    block_data *data;

    /* rendering related */
    struct chunk_render_data {
        bool visible;
        bool needs_remesh_simple;
        bool needs_remesh_complex;

        struct vert_simple {
            // XXXXXYYY
            // YYZZZZZP
            // THIS IS HOW THE COORDS ARE LAID OUT BECAUSE OF THE PACKED ATTR
            // WRITING THIS INSTEAD OF THE UGLY GCC NOTE
            ubyte x : 5;
            ubyte y : 5;
            ubyte z : 5;
            ubyte padding : 1;
            ubyte texture_index;
            ubyte data;
        } attr(packed) *verts_simple;

        struct vert_complex {
            vec3_t pos;
            vec2_t uv;
            ubyte texture_index;
            ubyte data;
            ubyte r,g,b;
        } attr(packed) *verts_complex;

        size_t n_verts_simple, n_verts_complex;
        uint vbo_simple, vbo_complex;
        uint light_tex;
    } gl;
} world_chunk;

extern struct hashmap *world_chunk_map;

errcode world_init(void);
void world_shutdown(void); // todo: rename me to cleanup or something?
// fixme
#define world_is_init() (cl.state == cl_connected && world_chunk_map != NULL)

/* chunks */
void world_alloc_chunk(int chunk_x, int chunk_z);
void world_free_chunk(int chunk_x, int chunk_z);
bool world_chunk_exists(int chunk_x, int chunk_z);
world_chunk *world_get_chunk(int chunk_x, int chunk_z);
void world_mark_region_for_remesh(int x_start, int y_start, int z_start, int x_end, int y_end, int z_end);
void world_mark_all_for_remesh(void);
void world_load_compressed_chunk_data(int x, int y, int z, int sx, int sy, int sz, size_t size, ubyte *data);

/* blocks */
// todo: define in block.c maybe
block_data world_get_block(int x, int y, int z);
block_data world_get_block_fast(world_chunk *chunk, int x, int y, int z);
block_data world_get_blockf(float x, float y, float z);
void world_set_block(int x, int y, int z, block_data data);
void world_set_block_id(int x, int y, int z, block_id id);
void world_set_block_metadata(int x, int y, int z, ubyte new_metadata);
ubyte world_get_block_lighting(int x, int y, int z);
ubyte world_get_block_lighting_fast(block_data block, int x, int y, int z);
bbox_t *world_get_colliding_blocks(bbox_t box);
bool block_should_face_be_rendered_fast(world_chunk *chunk, int x, int y, int z, block_data self, block_face face);

/* entities */
entity *world_get_entity(int entity_id);
void world_add_entity(entity *ent); // ent is copied so dw about it
void world_remove_entity(int entity_id);

/* daylight cycle stuff */
vec4_t world_calculate_sky_color(void);
float world_calculate_sun_angle(void);
float world_calculate_sky_light_modifier(void);

/* ray tracing */
struct trace_result {
    int x, y, z;
    block_data block;
    block_face hit_face;
    bool reached_end;
} world_trace_ray(vec3_t origin, vec3_t dir, float max_length);

/* rendering */
void world_renderer_init(void);
void world_renderer_shutdown(void);
void world_render(void);
void world_init_chunk_glbufs(world_chunk *c);
void world_free_chunk_glbufs(world_chunk *c);

#endif
