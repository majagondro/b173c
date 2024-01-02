#ifndef B173C_MATHLIB_H
#define B173C_MATHLIB_H

#include "mathlib.h"
#include "common.h"
#include <math.h>

#define DEG2RAD(d) ((d) * (3.1415f / 180.0f))

#define vec3_copy(dest, a) dest[0] = a[0], dest[1] = a[1], dest[2] = a[2]
#define vec3_add(dest, a, b) dest[0] = a[0] + b[0], dest[1] = a[1] + b[1], dest[2] = a[2] + b[2]
#define vec3_sub(dest, a, b) dest[0] = a[0] - b[0], dest[1] = a[1] - b[1], dest[2] = a[2] - b[2]
#define vec3_invert(dest, a) dest[0] = -a[0], dest[1] = -a[1], dest[2] = -a[2]
#define vec3_mul_scalar(dest, a, b) dest[0] = a[0] * b, dest[1] = a[1] * b, dest[2] = a[2] * b
#define vec3_dot(a, b) (a[0]*b[0]+a[1]*b[1]+a[2]*b[2])
#define vec3_len(a) sqrtf(a[0]*a[0]+a[1]*a[1]+a[2]*a[2])

typedef float vec4[4];
typedef float vec3[3];
typedef float vec2[2];
typedef float mat4[4][4];

void mat_multiply(mat4 dest, const mat4 a, const mat4 b);
void mat_identity(mat4 dest);
void mat_view(mat4 dest, const vec3 pos, vec3 ang);
void mat_frustrum(mat4 dest, float l, float r, float b, float t, float n, float f);
void mat_projection(mat4 dest, float fov, float aspect, float znear, float zfar);
void vec3_cross(vec3 dest, const vec3 a, const vec3 b);
void cam_angles(vec3 fwd, vec3 side, vec3 up, float yaw, float pitch);

#endif
