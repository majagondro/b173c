#include <zlib.h>
#include "world.h"
#include "client/console.h"

struct chunk *chunks;
long time = 0;

void world_prepare_chunk(int chunk_x, int chunk_z)
{
	struct chunk *c;

	/* alloc new chunk */
	c = B_malloc(sizeof(*c));
	c->x = chunk_x;
	c->z = chunk_z;
	c->data = B_malloc(sizeof(struct chunk_data));
	bzero(c->data, sizeof(struct chunk_data));

	/* add to list */
	c->next = chunks;
	chunks = c;
}

void world_delete_chunk(int chunk_x, int chunk_z)
{
	struct chunk *c, *prev;

	/* find chunk */
	for(c = chunks, prev = NULL; c != NULL; prev = c, c = c->next) {
		if(c->x == chunk_x && c->z == chunk_z) {
			if(!prev)
				chunks = c->next;
			else
				prev->next = c->next;

			B_free(c->data);
			B_free(c);
			return;
		}
	}
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
	if (ret != Z_OK) {
		con_printf("inflate_data: zlib error: %s\n", zError(ret));
		return ret;
	}

	strm.avail_in = size_in;
	strm.next_in = in;
	strm.avail_out = size_out;
	strm.next_out = out;

	ret = inflate(&strm, Z_NO_FLUSH);
	switch (ret) {
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

void world_load_region_data(int x, short y, int z, int size_x, int size_y, int size_z, int data_size, u_byte *data)
{
	int xStart, yStart, zStart;
	int xEnd, yEnd, zEnd;
	int dataPos;
	struct chunk_data *chunk_data;
	u_byte decomp[size_x * size_y * size_z * 5 / 2];

	//con_printf("load region %d %d %d size %d %d %d\n", x,y,z,size_x,size_y,size_z);

	// this region is always contained within a single chunk
	// i think xd
	if(!world_chunk_exists(x >> 4, z >> 4)) {
		//con_printf("world_load_region_data: chunk %d %d does not exist\n", x >> 4, z >> 4);
		return;
	}

	chunk_data = world_get_chunk(x >> 4, z >> 4)->data;
	if((dataPos = inflate_data(data, decomp, data_size, sizeof(decomp))) != Z_OK) {
		con_printf("error while decompressing chunk data: %s\n", zError(dataPos));
		return;
	}


	xStart = x & 15;
	yStart = y & 127;
	zStart = z & 15;

	xEnd = xStart + size_x;
	yEnd = yStart + size_y;
	zEnd = zStart + size_z;

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
			//int index = (x << 11 | z << 7 | yStart) >> 1;
			int ySize = (yEnd - yStart) / 2;
			// System.arraycopy(data, dataPos, this.data.data, index, ySize);
			dataPos += ySize;
		}
	}

	for(x = xStart; x < xEnd; ++x) {
		for(z = zStart; z < zEnd; ++z) {
			//int index = (x << 11 | z << 7 | yStart) >> 1;
			int ySize = (yEnd - yStart) / 2;
			// System.arraycopy(data, dataPos, this.blocklightMap.data, index, ySize);
			dataPos += ySize;
		}
	}

	for(x = xStart; x < xEnd; ++x) {
		for(z = zStart; z < zEnd; ++z) {
			//int index = (x << 11 | z << 7 | yStart) >> 1;
			int ySize = (yEnd - yStart) / 2;
			// System.arraycopy(data, dataPos, this.skylightMap.data, index, ySize);
			dataPos += ySize;
		}
	}

}

byte world_get_block(int x, int y, int z)
{
	if(!world_chunk_exists(x >> 4, z >> 4))
		return 0;
	if(y < 0 || y >= 128)
		return 0;
	return 1;
}

void world_set_block(int x, int y, int z, byte id)
{
	struct chunk *c;
	if(!world_chunk_exists(x >> 4, z >> 4))
		return;
	if(y < 0 || y >= 128)
		return;

	c = world_get_chunk(x >> 4, z >> 4);
	c->data->blocks[IDX_FROM_COORDS(x & 15, y, z & 15)] = id;
}

bool world_chunk_exists(int chunk_x, int chunk_z)
{
	return world_get_chunk(chunk_x, chunk_z) != NULL;
}

struct chunk *world_get_chunk(int chunk_x, int chunk_z)
{
	struct chunk *c;
	for(c = chunks; c != NULL; c = c->next) {
		if(c->x == chunk_x && c->z == chunk_z) {
			return c;
		}
	}
	return NULL;
}

void world_set_time(long t)
{
	time = t;
}

#define is_between(t,a,b) t >= a && t <= b
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
		float f = (float)(t - 11000) / 3000.0f;
		color[0] = f * 0.01f + (1.0f - f) * 0.55f;
		color[1] = f * 0.01f + (1.0f - f) * 0.7f;
		color[2] = f * 0.03f + (1.0f - f) * 0.9f;
	} else if(is_between(t, 22000, 24000)) {
		// start of day
		// interpolate between night (22000) and day (24000)
		float f = (float)(t - 22000) / 2000.0f;
		color[0] = f * 0.55f + (1.0f - f) * 0.01f;
		color[1] = f * 0.7f + (1.0f - f) * 0.01f;
		color[2] = f * 0.9f + (1.0f - f) * 0.03f;
	} else {
		// day
		color[0] = 0.55f;
		color[1] = 0.7f;
		color[2] = 1.0f;
	}

	return color;
}
