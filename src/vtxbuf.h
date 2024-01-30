#ifndef B173C_VTXBUF_H
#define B173C_VTXBUF_H

#include "common.h"
#include "hashmap.h"

struct vtx {
	float x, y, z;
	float tx_idx, face;
};

struct vtxidxpair {
	struct vtx vtx;
	u_short index;
};

struct vtxbuf {
	struct hashmap *_vtx2idx_map;
	struct vtx *vertices;
	u_short *indices;
	size_t index_count, vertex_count;
	size_t _index_cap, _vertex_cap;
};

struct vtx vtx_make(float x, float y, float z, u_byte t, u_byte d);
void vtxbuf_new(struct vtxbuf *vtxbuf, size_t initial_capacity);
void vtxbuf_free(struct vtxbuf *vtxbuf);
void vtxbuf_free_leave_buffers(struct vtxbuf *vtxbuf);
void vtxbuf_emit_index(struct vtxbuf *vtxbuf, u_short idx);
void vtxbuf_emit_vertex(struct vtxbuf *vtxbuf, struct vtx vtx);
void vtxbuf_emit_last(struct vtxbuf *vtxbuf);

#endif
