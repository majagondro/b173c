#ifndef B173C_WORLD_H
#define B173C_WORLD_H

#include "common.h"


struct chunk_data {
	byte blocks[16*16*128];
};

struct chunk {
	int x, z;
	struct chunk_data *data;
	struct chunk *next;
};


byte world_get_block(int x, byte y, int z);
bool world_chunk_exists(int chunk_x, int chunk_z);
void world_prepare_chunk(int chunk_x, int chunk_z);
void world_delete_chunk(int chunk_x, int chunk_z);
struct chunk *world_get_chunk(int chunk_x, int chunk_z);
void world_load_region_data(int x, short y, int z, int size_x, int size_y, int size_z, int data_size, u_byte *data);

#endif
