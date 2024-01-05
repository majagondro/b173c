#include <zlib.h>
#include "world.h"
#include "client/console.h"
#include "vid/vid.h"
#include "hashmap.h"

int chunk_compare(const void *a, const void *b, void *udata)
{
	// 0 if equals i think
	const struct chunk *ca = a, *cb = b;
	return !(ca->x == cb->x && ca->z == cb->z);
}

uint64_t chunk_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
	const struct chunk *c = item;
	union {
		uint64_t hash;
		byte data[sizeof(uint64_t)];
		struct {
			int x, z;
		};
	} data;
	data.x = c->x;
	data.z = c->z;
	return hashmap_xxhash3(data.data, sizeof(data.data), seed0, seed1);
}

struct hashmap *chunk_map;
long time = 0;

void world_init(void)
{
	chunk_map = hashmap_new(sizeof(struct chunk), 0, 0, 0, chunk_hash, chunk_compare, NULL, NULL);
}

void world_mark_chunk_dirty(int chunk_x, int chunk_z)
{
	if(world_chunk_exists(chunk_x, chunk_z)) {
		struct chunk *c = world_get_chunk(chunk_x, chunk_z);
		c->dirty = true;
		if(c->data) {
			/* mark render buffers dirty */
			for(int rb = 0; rb < 8; rb++) {
				if(c->data->render_bufs[rb]) {
					// dirty = true
					*c->data->render_bufs[rb] = true;
				}
			}
		}
	}
}

void world_prepare_chunk(int chunk_x, int chunk_z)
{
	struct chunk c = {0};

	if(world_chunk_exists(chunk_x, chunk_z))
		return;

	c.x = chunk_x;
	c.z = chunk_z;
	c.data = B_malloc(sizeof(struct chunk_data));

	/* chunk is copied by hashmap_set, so it can be allocated on the stack */
	hashmap_set(chunk_map, &c);

	/* mark neighbours dirty */
	world_mark_chunk_dirty(chunk_x + 1, chunk_z);
	world_mark_chunk_dirty(chunk_x - 1, chunk_z);
	world_mark_chunk_dirty(chunk_x, chunk_z + 1);
	world_mark_chunk_dirty(chunk_x, chunk_z - 1);
}

void world_delete_chunk(int chunk_x, int chunk_z)
{
	struct chunk c;
	struct chunk *cp;
	c.x = chunk_x;
	c.z = chunk_z;

	cp = (struct chunk *) hashmap_get(chunk_map, &c);
	world_renderer_free_chunk_rbufs(cp);
	B_free(cp->data);

	hashmap_delete(chunk_map, &c);
}

int inflate_data(u_byte *in, u_byte *out, size_t size_in, size_t size_out)
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

static void copy4to8(void *dest, void *src, size_t n)
{
	u_byte *in = src;
	u_byte *out = dest;
	while(n-- > 0) {
		*out++ = (*in   & 0x0f);
		*out++ = (*in++ & 0xf0) >> 4;
	}
}

void world_load_region_data(int x, short y, int z, int size_x, int size_y, int size_z, int data_size, u_byte *data)
{
	int xStart, yStart, zStart;
	int xEnd, yEnd, zEnd;
	int dataPos;
	struct chunk_data *chunk_data;
	u_byte decomp[size_x * size_y * size_z * 5 / 2];

	// this region is always contained within a single chunk
	// i think xd
	if(!world_chunk_exists(x >> 4, z >> 4)) {
		world_prepare_chunk(x >> 4, z >> 4);
	} else {
		int chunk_x = x >> 4;
		int chunk_z = z >> 4;
		/* mark neighbours dirty */
		world_mark_chunk_dirty(chunk_x + 1, chunk_z);
		world_mark_chunk_dirty(chunk_x - 1, chunk_z);
		world_mark_chunk_dirty(chunk_x, chunk_z + 1);
		world_mark_chunk_dirty(chunk_x, chunk_z - 1);
	}

	if((dataPos = inflate_data(data, decomp, data_size, sizeof(decomp))) != Z_OK) {
		con_printf(COLOR_RED"error while decompressing chunk data:"COLOR_WHITE"%s\n", zError(dataPos));
		return;
	}

	chunk_data = world_get_chunk(x >> 4, z >> 4)->data;

	xStart = x & 15;
	yStart = y & 127;
	zStart = z & 15;

	xEnd = xStart + size_x;
	yEnd = yStart + size_y;
	zEnd = zStart + size_z;

	world_get_chunk(x >> 4, z >> 4)->dirty = true;
	for(y = yStart >> 4; y < yEnd >> 4; y++) {
		if(chunk_data->render_bufs[y]) {
			// dirty = true
			*chunk_data->render_bufs[y] = true;
		}
	}

	dataPos = 0;

	for(x = xStart; x < xEnd; ++x) {
		for(z = zStart; z < zEnd; ++z) {
			int index = x << 11 | z << 7 | yStart;
			int ySize = yEnd - yStart;
			memcpy(chunk_data->blocks + index, decomp + dataPos, ySize);
			dataPos += ySize;
		}
	}

	for(x = xStart; x < xEnd; ++x) {
		for(z = zStart; z < zEnd; ++z) {
			int index = (x << 11 | z << 7 | yStart) >> 1;
			int ySize = (yEnd - yStart) / 2;
			memcpy(chunk_data->metadata + index, decomp + dataPos, ySize);
			dataPos += ySize;
		}
	}

	for(x = xStart; x < xEnd; ++x) {
		for(z = zStart; z < zEnd; ++z) {
			int index = (x << 11 | z << 7 | yStart) >> 1;
			int ySize = (yEnd - yStart) / 2;
			memcpy(chunk_data->blocklight + index, decomp + dataPos, ySize);
			dataPos += ySize;
		}
	}

	for(x = xStart; x < xEnd; ++x) {
		for(z = zStart; z < zEnd; ++z) {
			int index = (x << 11 | z << 7 | yStart) >> 1;
			int ySize = (yEnd - yStart) / 2;
			memcpy(chunk_data->skylight + index, decomp + dataPos, ySize);
			dataPos += ySize;
		}
	}
}

byte chunk_get_block(struct chunk *c, int x, int y, int z)
{
	return c->data->blocks[IDX_FROM_COORDS(x, y, z)];
}

byte world_get_block(int x, int y, int z)
{
	struct chunk *cp;
	if(y < 0 || y >= 128)
		return 0;
	cp = world_get_chunk(x >> 4, z >> 4);
	if(!cp)
		return 1;
	return chunk_get_block(cp, x & 15, y, z & 15);
}

u_byte chunk_get_block_lighting(struct chunk *c, int x, int y, int z)
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

u_byte world_get_block_lighting(int x, int y, int z)
{
	struct chunk *cp;
	if(y < 0)
		return 0;
	if(y >= 128)
		return 15;
	cp = world_get_chunk(x >> 4, z >> 4);
	if(!cp)
		return 15;
	return chunk_get_block_lighting(cp, x & 15, y, z & 15);
}


void world_set_block(int x, int y, int z, byte id)
{
	struct chunk *c;
	if(!world_chunk_exists(x >> 4, z >> 4))
		return;
	if(y < 0 || y >= 128)
		return;

	c = world_get_chunk(x >> 4, z >> 4);

	// mark dirty
	c->dirty = true;
	if(c->data->render_bufs[y >> 4])
		*c->data->render_bufs[y >> 4] = 1;

	c->data->blocks[IDX_FROM_COORDS(x & 15, y, z & 15)] = id;
}

void world_set_metadata(int x, int y, int z, byte metadata)
{
	struct chunk *c;
	if(!world_chunk_exists(x >> 4, z >> 4))
		return;
	if(y < 0 || y >= 128)
		return;

	c = world_get_chunk(x >> 4, z >> 4);

	// mark dirty
	c->dirty = true;
	if(c->data->render_bufs[y >> 4])
		*c->data->render_bufs[y >> 4] = true;

	c->data->metadata[IDX_FROM_COORDS(x & 15, y, z & 15) >> 1] = metadata;
}

bool world_chunk_exists(int chunk_x, int chunk_z)
{
	return world_get_chunk(chunk_x, chunk_z) != NULL;
}

struct chunk *world_get_chunk(int chunk_x, int chunk_z)
{
	struct chunk c;
	c.x = chunk_x;
	c.z = chunk_z;
	return (struct chunk *) hashmap_get(chunk_map, &c);
}

void world_set_time(long t)
{
	time = t;
}

#define is_between(t, a, b) t >= a && t <= b

float *world_get_sky_color(void)
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

float world_get_light_modifier(void)
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
