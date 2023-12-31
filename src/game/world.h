#ifndef B173C_WORLD_H
#define B173C_WORLD_H

#include "common.h"
#include "mathlib.h"

#define IDX_FROM_COORDS(x, y, z) ((x) << 11 | (z) << 7 | (y))

struct chunk_data {
	byte blocks[16*16*128];
};

struct chunk {
	int x, z;
	bool dirty;
	struct chunk_data *data;
	struct chunk *next;
};

byte world_get_block(int x, int y, int z);
void world_set_block(int x, int y, int z, byte id);
bool world_chunk_exists(int chunk_x, int chunk_z);
void world_prepare_chunk(int chunk_x, int chunk_z);
void world_delete_chunk(int chunk_x, int chunk_z);
struct chunk *world_get_chunk(int chunk_x, int chunk_z);
void world_load_region_data(int x, short y, int z, int size_x, int size_y, int size_z, int data_size, u_byte *data);
void world_set_time(long t);
float *world_get_sky_color(void);

#endif
