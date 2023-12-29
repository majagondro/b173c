#ifndef B173C_MATHLIB_H
#define B173C_MATHLIB_H

#include "mathlib.h"
#include "common.h"
#include <math.h>

#define DEG2RAD(d) (d * (3.1415f / 180.0f))
#define DOT(a, b) (a[0]*b[0]+a[1]*b[1]+a[2]*b[2])

typedef float vec4[4];
typedef float vec3[3];
typedef float vec2[2];
typedef float mat4[4][4];

void mat_multiply(mat4 dest, const mat4 a, const mat4 b);
void mat_identity(mat4 dest);
void mat_view(mat4 dest, const vec3 pos, vec3 ang);
void mat_frustrum(mat4 dest, float left, float right, float bottom, float top, float znear, float zfar);
void mat_projection(mat4 dest, float fov, float aspect, float znear, float zfar);

#endif
