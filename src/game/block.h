#ifndef B173C_BLOCK_H
#define B173C_BLOCK_H

#include "common.h"
#include "block_ids.h"

typedef enum {
	 BLOCK_FACE_Y_NEG,
	 BLOCK_FACE_Y_POS,
	 BLOCK_FACE_Z_NEG,
	 BLOCK_FACE_Z_POS,
	 BLOCK_FACE_X_NEG,
	 BLOCK_FACE_X_POS
} block_face;

typedef enum {
	RENDER_NONE = -1,
	RENDER_CUBE,
	RENDER_CROSS,
	RENDER_TORCH,
	RENDER_FIRE,
	RENDER_FLUID,
	RENDER_WIRE,
	RENDER_CROPS,
	RENDER_DOOR,
	RENDER_LADDER,
	RENDER_RAIL,
	RENDER_STAIRS,
	RENDER_FENCE,
	RENDER_LEVER,
	RENDER_CACTUS,
	RENDER_BED,
	RENDER_REPEATER,
	RENDER_PISTON_BASE,
	RENDER_PISTON_EXTENSION,
	RENDER_CUBE_SPECIAL
} block_render_type;

typedef ubyte block_id;

typedef struct {
	block_id id;
	ubyte metadata   : 4;
	ubyte skylight   : 4;
	ubyte blocklight : 4;
} block_data;

typedef struct {
	char *name;
	ubyte texture_indices[6];
	bool opaque;
	block_render_type render_type;
} block_properties;


#define block_is_empty(b) ((b).id == 0)
#define block_is_transparent(b) (block_get_properties(b.id).opaque == 0)
#define block_is_semi_transparent(b) ((b).id == BLOCK_WATER_STILL || (b).id == BLOCK_WATER_MOVING || (b).id == BLOCK_ICE)
block_properties block_get_properties(block_id id);
bool block_should_face_be_rendered(int x, int y, int z, block_data self, block_face face);

#endif
