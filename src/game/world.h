#ifndef B173C_WORLD_H
#define B173C_WORLD_H

#include "common.h"
#include "mathlib.h"
#include "hashmap.h"
#include "block.h"

#define WORLD_CHUNK_SIZE 16
#define WORLD_CHUNK_HEIGHT 128

#define IDX_FROM_COORDS(x, y, z) ((((x) & 15) << 11) | (((z) & 15) << 7) | ((y) & 127))

typedef struct {
	int x, z;

	/* heap allocated, always has WORLD_CHUNK_SIZE * WORLD_CHUNK_SIZE * WORLD_CHUNK_HEIGHT elements */
	block_data *data;

	/* rendering related */
	bool needs_remesh;
	bool visible;
	struct world_chunk_glbuf {
		bool visible;
		struct world_vertex {
			float x;// ubyte x : 5;
			float y;// ubyte y : 5;
			float z;// ubyte z : 5;
			// ubyte padding : 1 attr(unused);
			ubyte texture_index;
			ubyte data;
			ushort extradata;
		} attr(packed)
		*vertices, *alpha_vertices;
		ushort *indices, *alpha_indices;
		size_t num_vertices, num_alpha_vertices;
		size_t num_indices, num_alpha_indices;
		uint basevertex, alpha_basevertex;
		uint vbo, alpha_vbo;
	} glbufs[8]; /* each glbuf covers 16x16x16 blocks */
} world_chunk;

extern struct hashmap *world_chunk_map;

void world_init(void);
void world_shutdown(void); // todo: rename me to cleanup or something?

/* chunks */
void world_alloc_chunk(int chunk_x, int chunk_z);
void world_free_chunk(int chunk_x, int chunk_z);
bool world_chunk_exists(int chunk_x, int chunk_z);
world_chunk *world_get_chunk(int chunk_x, int chunk_z);
void world_mark_chunk_for_remesh(int chunk_x, int chunk_z);
void world_load_compressed_chunk_data(int x, int y, int z, int size_x, int size_y, int size_z, size_t data_size, ubyte *data);

/* blocks */
block_data world_get_block(int x, int y, int z);
void world_set_block(int x, int y, int z, block_data data);
void world_set_block_id(int x, int y, int z, block_id id);
void world_set_block_metadata(int x, int y, int z, byte new_metadata);
ubyte world_get_block_lighting(int x, int y, int z);

/* time, daylight cycle, etc. */
void world_set_time(ulong time);
ulong world_get_time(void);
float *world_calculate_sky_color(void); // fixme: 'color' type
float world_calculate_sun_angle(void);
float world_calculate_sky_light_modifier(void);

/* rendering */
void world_renderer_init(void);
void world_renderer_shutdown(void);
void world_render(void);
void world_init_chunk_glbufs(world_chunk *c);
void world_free_chunk_glbufs(world_chunk *c);
struct world_vertex world_make_vertex(float x, float y, float z, ubyte texture_index, ubyte face, ubyte light);

#endif
