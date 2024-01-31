#include <zlib.h>
#include "world.h"
#include "client/console.h"
#include "vid/vid.h"
#include "hashmap.h"

#define GET4BIT(arr, idx) (((arr)[(idx) >> 1] >> ((idx & 1) ? 4 : 0)) & 15)

const static block_data AIR_BLOCK_DATA = {.id = 0, .metadata = 0, .skylight = 15, .blocklight = 0};
const static block_data EMPTY_BLOCK_DATA = {.id = 0, .metadata = 0, .skylight = 0, .blocklight = 0};
const static block_data SOLID_BLOCK_DATA = {.id = 1, .metadata = 0, .skylight = 0, .blocklight = 0};

struct hashmap *world_chunk_map = NULL;
ulong time = 0;

/* functions for the hashmap */
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

void world_init(void)
{
	world_chunk_map = hashmap_new(sizeof(world_chunk), 0, 0, 0, chunk_hash, chunk_compare, chunk_free, NULL);
}

void world_shutdown(void)
{
	hashmap_free(world_chunk_map);
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

void world_mark_chunk_for_remesh(int chunk_x, int chunk_z)
{
	world_chunk *chunk = world_get_chunk(chunk_x, chunk_z);
	if(chunk != NULL) {
		chunk->needs_remesh = true;
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

void world_load_compressed_chunk_data(int x, int y, int z, int size_x, int size_y, int size_z, size_t data_size, ubyte *data)
{
	int x_start, y_start, z_start;
	int x_end, y_end, z_end;
	int chunk_x = x >> 4;
	int chunk_z = z >> 4;
	// 1 byte id, 0.5 byte metadata, 2x0.5 byte light data
	ubyte decompressed[size_x * size_y * size_z * 5 / 2];
	ubyte *data_ptr;
	world_chunk *chunk;
	int err;

	world_alloc_chunk(chunk_x, chunk_z);
	chunk = world_get_chunk(chunk_x, chunk_z);

	if((err = inflate_data(data, decompressed, data_size, sizeof(decompressed))) != Z_OK) {
		con_printf(COLOR_RED"error while decompressing chunk data:"COLOR_WHITE"%s\n", zError(err)); //should this print?
		return;
	}

	/* mark for remeshing */
	world_mark_chunk_for_remesh(chunk_x, chunk_z);
	world_mark_chunk_for_remesh(chunk_x + 1, chunk_z);
	world_mark_chunk_for_remesh(chunk_x - 1, chunk_z);
	world_mark_chunk_for_remesh(chunk_x, chunk_z + 1);
	world_mark_chunk_for_remesh(chunk_x, chunk_z - 1);

	/* local coordinates */
	x_start = x & (WORLD_CHUNK_SIZE - 1);
	y_start = y & (WORLD_CHUNK_HEIGHT - 1);
	z_start = z & (WORLD_CHUNK_SIZE - 1);
	x_end = x_start + size_x;
	y_end = y_start + size_y;
	z_end = z_start + size_z;

	data_ptr = decompressed;

	/* block ids */
	for(x = x_start; x < x_end; x++) {
		for(z = z_start; z < z_end; z++) {
			for(y = y_start; y < y_end; y++, data_ptr++) {
				int index = IDX_FROM_COORDS(x, y, z);
				chunk->data[index].id = *data_ptr;
			}
		}
	}


	/* metadata */
	/*for(x = x_start; x < x_end; ++x) {
		for(z = z_start; z < z_end; ++z) {
			for(y = y_start; y < y_end; y += 2, data_ptr++) {
				int i1 = IDX_FROM_COORDS(x, y, z);
				int i2 = IDX_FROM_COORDS(x, y + 1, z);
				chunk->data[i1].metadata = GET4BIT(data_ptr, i1);
				chunk->data[i2].metadata = GET4BIT(data_ptr, i2);
			}
		}
	}*/

	/* block light */
	/*for(x = x_start; x < x_end; ++x) {
		for(z = z_start; z < z_end; ++z) {
			for(y = y_start; y < y_end; y += 2, data_ptr++) {
				int i1 = IDX_FROM_COORDS(x, y, z);
				int i2 = IDX_FROM_COORDS(x, y + 1, z);
				chunk->data[i1].blocklight = GET4BIT(data_ptr, i1);
				chunk->data[i2].blocklight = GET4BIT(data_ptr, i2);
			}
		}
	}*/

	/* sky light */
	/*for(x = x_start; x < x_end; ++x) {
		for(z = z_start; z < z_end; ++z) {
			for(y = y_start; y < y_end; y += 2, data_ptr++) {
				int i1 = IDX_FROM_COORDS(x, y, z);
				int i2 = IDX_FROM_COORDS(x, y + 1, z);
				chunk->data[i1].skylight = GET4BIT(data_ptr, i1);
				chunk->data[i2].skylight = GET4BIT(data_ptr, i2);
			}
		}
	}*/
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
/*
ubyte chunk_get_block_lighting(world_chunk *c, int x, int y, int z)
{
	int i = IDX_FROM_COORDS(x, y, z);
	int block_id = c->data->blocks[i];
	int l, bl;
	if(block_id == 44 || block_id == 60 || block_id == 53 || block_id == 67) {
		int py = world_get_block_lighting(x, y + 1, z);
		int px = world_get_block_lighting(x + 1, y, z);
		int nx = world_get_block_lighting(x - 1, y, z);
		int pz = world_get_block_lighting(x, y, z + 1);
		int nz = world_get_block_lighting(x, y, z - 1);
		return max(py, max(px, max(nx, max(pz, nz))));
	}
	l = GET4BIT(c->data->skylight, i);
	bl = GET4BIT(c->data->blocklight, i);
	return (bl > l ? bl : l) & 15;
}

ubyte world_get_block_lighting(int x, int y, int z)
{
	world_chunk *cp;
	if(y < 0)
		return 0;
	if(y >= 128)
		return 15;
	cp = world_get_chunk(x >> 4, z >> 4);
	if(!cp)
		return 15;
	return chunk_get_block_lighting(cp, x & 15, y, z & 15);
}
*/

void world_set_block(int x, int y, int z, block_data data)
{
	world_chunk *chunk;

	if(y < 0 || y >= 128)
		return;

	chunk = world_get_chunk(x >> 4, z >> 4);
	if(!chunk)
		return;

	world_mark_chunk_for_remesh(x >> 4, z >> 4);

	chunk->data[IDX_FROM_COORDS(x, y, z)] = data;
}

void world_set_block_id(int x, int y, int z, block_id new_id)
{
	block_data data = world_get_block(x, y, z);
	data.id = new_id;
	world_set_block(x, y, z, data);
}

void world_set_block_metadata(int x, int y, int z, byte new_metadata)
{
	block_data data = world_get_block(x, y, z);
	data.metadata = new_metadata;
	world_set_block(x, y, z, data);
}

void world_set_time(ulong t)
{
	time = t;
}

ulong world_get_time(void)
{
	return time;
}


#define is_between(t, a, b) t >= a && t <= b
float *world_calculate_sky_color(void)
{
	static vec3 color;

	int t = time % 24000;

	if(is_between(t, 14000, 22000)) {
		// night
		color[0] = 0.01f;
		color[1] = 0.01f;
		color[2] = 0.03f;
	} else if(is_between(t, 11000, 14000)) {
		// start of night
		// interpolate between day (11000) and night (14000)
		float f = (float) (t - 11000) / 3000.0f;
		color[0] = f * 0.01f + (1.0f - f) * 0.55f;
		color[1] = f * 0.01f + (1.0f - f) * 0.7f;
		color[2] = f * 0.03f + (1.0f - f) * 1.0f;
	} else if(is_between(t, 22000, 24000)) {
		// start of day
		// interpolate between night (22000) and day (24000)
		float f = (float) (t - 22000) / 2000.0f;
		color[0] = f * 0.55f + (1.0f - f) * 0.01f;
		color[1] = f * 0.7f + (1.0f - f) * 0.01f;
		color[2] = f * 1.0f + (1.0f - f) * 0.03f;
	} else {
		// day
		color[0] = 0.55f;
		color[1] = 0.7f;
		color[2] = 1.0f;
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
	int t = time % 24000;
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
