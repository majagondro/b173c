#ifndef B173C_ENTITY_H
#define B173C_ENTITY_H

#include "mathlib.h"

typedef struct {
	vec3 mins, maxs;
} bbox;

typedef struct {
	int id;
	bbox bbox;
	vec3 position;

} entity;



#endif
