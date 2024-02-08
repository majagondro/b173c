#ifndef B173C_ASSETS_H
#define B173C_ASSETS_H

#include "common.h"

typedef enum {
	ASSET_NULL,
	ASSET_TEXTURE_TERRAIN,
	ASSET_TEXTURE_FONT_DEFAULT,
	ASSET_COUNT
} asset_name;


typedef struct {
	size_t length;
	char *data;
} asset_text;

typedef struct {
	int width, height;
	int channels;
	ubyte *data;
} asset_image;

struct asset_storage {
	byte data[24]; /* expand if necessary */
};

void assets_init(void);
asset_text *asset_get_text(asset_name name);
asset_image *asset_get_image(asset_name name);

#endif