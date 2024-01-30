#include <unistd.h>
#include "vtxbuf.h"

#define DEFAULT_CAPACITY 1024

// functions for the hashmap
static int vtxidxcmp(const void *a, const void *b, void *udata)
{
	return memcmp(a, b, sizeof(struct vtx));
}

static uint64_t vtxidxhash(const void *item, uint64_t seed0, uint64_t seed1)
{
	return hashmap_xxhash3(item, sizeof(struct vtx), seed0, seed1);
}

void vtxbuf_new(struct vtxbuf *vtxbuf, size_t cap)
{
	memset(vtxbuf, 0, sizeof(*vtxbuf));

	vtxbuf->_vtx2idx_map = hashmap_new(sizeof(struct vtxidxpair), 0, 0, 0, vtxidxhash, vtxidxcmp, NULL, NULL);

	if(cap == 0)
		cap = DEFAULT_CAPACITY;
	vtxbuf->_index_cap = cap;
	vtxbuf->_vertex_cap = cap;

	vtxbuf->vertices = mem_alloc(cap * sizeof(*vtxbuf->vertices));
	vtxbuf->vertex_count = 0;
	vtxbuf->indices = mem_alloc(cap * sizeof(*vtxbuf->indices));
	vtxbuf->index_count = 0;
}

void vtxbuf_free(struct vtxbuf *vtxbuf)
{
	vtxbuf_free_leave_buffers(vtxbuf);
	mem_free(vtxbuf->vertices);
	mem_free(vtxbuf->indices);
}

void vtxbuf_free_leave_buffers(struct vtxbuf *vtxbuf)
{
	hashmap_free(vtxbuf->_vtx2idx_map);
}

void vtxbuf_emit_index(struct vtxbuf *vtxbuf, u_short idx)
{
	if(vtxbuf->index_count == vtxbuf->_index_cap) {
		vtxbuf->_index_cap += DEFAULT_CAPACITY;
		vtxbuf->indices = realloc(vtxbuf->indices, vtxbuf->_index_cap * sizeof(*vtxbuf->indices));
	}
	vtxbuf->indices[vtxbuf->index_count] = idx;
	vtxbuf->index_count++;
}

void vtxbuf_emit_vertex(struct vtxbuf *vtxbuf, struct vtx vtx)
{
	struct vtxidxpair pair = {.vtx = vtx};
	struct vtxidxpair *pair2;

	//if((pair2 = hashmap_get(vtxbuf->_vtx2idx_map, &pair)) != NULL) {
		// vertex already exists
		//vtxbuf_emit_index(vtxbuf, pair2->index);
		//return;
	//}

	if(vtxbuf->vertex_count == vtxbuf->_vertex_cap) {
		vtxbuf->_vertex_cap += DEFAULT_CAPACITY;
		vtxbuf->vertices = realloc(vtxbuf->vertices, vtxbuf->_vertex_cap * sizeof(*vtxbuf->vertices));
	}

	pair.index = vtxbuf->vertex_count;
	vtxbuf->vertices[pair.index] = vtx;
	//vtxbuf_emit_index(vtxbuf, pair.index);
	hashmap_set(vtxbuf->_vtx2idx_map, &pair);

	vtxbuf->vertex_count++;
}

void vtxbuf_emit_last(struct vtxbuf *vtxbuf)
{
	if(vtxbuf->vertex_count == 0)
		return;
	//vtxbuf_emit_index(vtxbuf, vtxbuf->indices[vtxbuf->index_count - 1]);
	vtxbuf_emit_vertex(vtxbuf, vtxbuf->vertices[vtxbuf->vertex_count-1]);
}

struct vtx vtx_make(float x, float y, float z, u_byte t, u_byte d)
{
	return (struct vtx){(float)x,(float)y,(float)z,(float)t,(float)d};
}
