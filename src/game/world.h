#ifndef B173C_WORLD_H
#define B173C_WORLD_H

#include "common.h"
#include "mathlib.h"

#define IDX_FROM_COORDS(x, y, z) ((((x) & 15) << 11) | (((z) & 15) << 7) | ((y) & 127))
#define GET4BIT(arr, idx) (((arr)[(idx) >> 1] >> ((idx & 1) ? 4 : 0)) & 15)

struct chunk_data {
	// laid out y,z,x
	u_byte blocks[16*16*128];
	// :-( these values are 4-bit...
	u_byte metadata  [16 * 16 * 64];
	u_byte skylight  [16 * 16 * 64];
	u_byte blocklight[16 * 16 * 64];

	// todo: maybe hide this or move somewhere or idk
	bool gl_init;
	u_int gl_vbo;
	struct block_vertex {
		/* modified me? change vao init in world_renderer.c as well */
		float x, y, z;
		float tx_idx, face;
	} *verts;
	size_t num_verts;
};

struct chunk {
	int x, z;
	bool dirty, visible;
	struct chunk_data *data;
};

void world_init(void);
byte world_get_block(int x, int y, int z);
void world_set_block(int x, int y, int z, byte id);
void world_set_metadata(int x, int y, int z, byte metadata);
bool world_chunk_exists(int chunk_x, int chunk_z);
void world_prepare_chunk(int chunk_x, int chunk_z);
void world_delete_chunk(int chunk_x, int chunk_z);
struct chunk *world_get_chunk(int chunk_x, int chunk_z);
void world_load_region_data(int x, short y, int z, int size_x, int size_y, int size_z, int data_size, u_byte *data);
void world_set_time(long t);
float *world_get_sky_color(void);
float world_get_light_modifier(void);
void world_mark_chunk_dirty(int chunk_x, int chunk_z);
u_byte world_get_block_lighting(int x, int y, int z);
u_byte chunk_get_block_lighting(struct chunk *c, int x, int y, int z);

#endif
