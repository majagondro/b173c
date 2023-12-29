#include <zlib.h>
#include <assert.h>
#include "world.h"

#define IDX_FROM_COORDS(x, y, z) ((((x) & 15) | (((z) & 15) << 4)) | ((y) << 8))


struct chunk *chunks;


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

			B_free(c);
			B_free(c->data);
			return;
		}
	}
}

static void zerr(int ret)
{
	fputs("zpipe: ", stderr);
	switch(ret) {
		case Z_ERRNO:
			if(ferror(stdin))
				fputs("error reading stdin\n", stderr);
			if(ferror(stdout))
				fputs("error writing stdout\n", stderr);
			break;
		case Z_STREAM_ERROR:
			fputs("invalid compression level\n", stderr);
			break;
		case Z_DATA_ERROR:
			fputs("invalid or incomplete deflate data\n", stderr);
			break;
		case Z_MEM_ERROR:
			fputs("out of memory\n", stderr);
			break;
		case Z_VERSION_ERROR:
			fputs("zlib version mismatch!\n", stderr);
			break;
		default:
			fprintf(stderr, "other: %s\n", zError(ret));
	}
}

int inf(u_byte *source, u_byte *dest, size_t size_in, size_t size_out)
{
	int ret;
	z_stream strm;

	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit(&strm);
	if (ret != Z_OK)
		return ret;

	/* decompress until deflate stream ends or end of file */
	do {
		strm.avail_in = size_in;
		strm.next_in = (u_byte *) source;

		/* run inflate() on input until output buffer not full */
		do {
			strm.avail_out = size_out;
			strm.next_out = (u_byte *) dest;
			ret = inflate(&strm, Z_NO_FLUSH);
			assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			switch (ret) {
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;     /* and fall through */
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					(void)inflateEnd(&strm);
					return ret;
			}
		} while (strm.avail_out == 0);

		/* done when inflate() says it's done */
	} while (ret != Z_STREAM_END);

	/* clean up and return */
	(void)inflateEnd(&strm);
	return Z_OK;
}

void world_load_region_data(int x, short y, int z, int size_x, int size_y, int size_z, int data_size, u_byte *data)
{
	int local_x, local_y, local_z;
	struct chunk_data *chunk_data;
	z_stream strm = {0};
	byte decomp[(size_x + 1) * (size_y + 1) * (size_z + 1) * 5 / 2];
	int ret;

	printf("load region %d %d %d size %d %d %d\n", x,y,z,size_x,size_y,size_z);

	// this region is always contained within a single chunk
	// i think xd
	if(!world_chunk_exists(x >> 4, z >> 4)) {
		printf("world_load_region_data: chunk %d %d does not exist\n", x >> 4, z >> 4);
		return; // todo: log?
	}

	chunk_data = world_get_chunk(x >> 4, z >> 4)->data;
	inf(data, (u_byte *) decomp, data_size, sizeof(decomp));


	local_x = x & 15;
	local_y = y & 127;
	local_z = z & 15;

	for(x = local_x; x < local_x + size_x; x++) {
		for(z = local_z; z < local_z + size_z; z++) {
			for(y = local_y; y < local_y + size_y; y++) {
				int idx = y + (z * (size_y+1)) + (x * (size_y+1) * (size_z+1));
				chunk_data->blocks[IDX_FROM_COORDS(x,y,z)] = decomp[idx];
				//printf("%d %d %d -> %d\n", x,y,z,decomp[idx]);
			}
		}
	}

	printf("load ok?\n");

}

byte world_get_block(int x, byte y, int z)
{
	if(!world_chunk_exists(x >> 4, z >> 4))
		return 0;
	if(y < 0)
		return 0;
	return 1;
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
