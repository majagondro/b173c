#include "mathlib.h"
#include "common.h"
#include <math.h>

vec3 vec3_normalize(vec3 v)
{
	return vec3_div(v, vec3_len(v));
}

vec3 vec3_cross(vec3 a, vec3 b)
{
	vec3 s;

	s.x = a.y * b.z - a.z * b.y;
	s.y = a.z * b.x - a.x * b.z;
	s.z = a.x * b.y - a.y * b.x;

	return vec3_normalize(s);
}

void mat_multiply(mat4 dest, const mat4 a, const mat4 b) {
	dest[0][0] = a[0][0] * b[0][0] + a[0][1] * b[1][0] + a[0][2] * b[2][0] + a[0][3] * b[3][0];
	dest[0][1] = a[0][0] * b[0][1] + a[0][1] * b[1][1] + a[0][2] * b[2][1] + a[0][3] * b[3][1];
	dest[0][2] = a[0][0] * b[0][2] + a[0][1] * b[1][2] + a[0][2] * b[2][2] + a[0][3] * b[3][2];
	dest[0][3] = a[0][0] * b[0][3] + a[0][1] * b[1][3] + a[0][2] * b[2][3] + a[0][3] * b[3][3];

	dest[1][0] = a[1][0] * b[0][0] + a[1][1] * b[1][0] + a[1][2] * b[2][0] + a[1][3] * b[3][0];
	dest[1][1] = a[1][0] * b[0][1] + a[1][1] * b[1][1] + a[1][2] * b[2][1] + a[1][3] * b[3][1];
	dest[1][2] = a[1][0] * b[0][2] + a[1][1] * b[1][2] + a[1][2] * b[2][2] + a[1][3] * b[3][2];
	dest[1][3] = a[1][0] * b[0][3] + a[1][1] * b[1][3] + a[1][2] * b[2][3] + a[1][3] * b[3][3];

	dest[2][0] = a[2][0] * b[0][0] + a[2][1] * b[1][0] + a[2][2] * b[2][0] + a[2][3] * b[3][0];
	dest[2][1] = a[2][0] * b[0][1] + a[2][1] * b[1][1] + a[2][2] * b[2][1] + a[2][3] * b[3][1];
	dest[2][2] = a[2][0] * b[0][2] + a[2][1] * b[1][2] + a[2][2] * b[2][2] + a[2][3] * b[3][2];
	dest[2][3] = a[2][0] * b[0][3] + a[2][1] * b[1][3] + a[2][2] * b[2][3] + a[2][3] * b[3][3];

	dest[3][0] = a[3][0] * b[0][0] + a[3][1] * b[1][0] + a[3][2] * b[2][0] + a[3][3] * b[3][0];
	dest[3][1] = a[3][0] * b[0][1] + a[3][1] * b[1][1] + a[3][2] * b[2][1] + a[3][3] * b[3][1];
	dest[3][2] = a[3][0] * b[0][2] + a[3][1] * b[1][2] + a[3][2] * b[2][2] + a[3][3] * b[3][2];
	dest[3][3] = a[3][0] * b[0][3] + a[3][1] * b[1][3] + a[3][2] * b[2][3] + a[3][3] * b[3][3];
}

void mat_identity(float dest[4][4])
{
	int i;
	memset(dest, 0, 16 * sizeof(float));
	for(i = 0; i < 4; i++) {
		dest[i][i] = 1;
	}
}

void mat_translate(mat4 dest, float x, float y, float z)
{
	dest[0][3] = x;
	dest[1][3] = y;
	dest[2][3] = z;
}

void cam_angles(vec3 *fwd, vec3 *side, vec3 *up, float yaw, float pitch)
{
	float cp, sp, cy, sy;
	cp = cosf(DEG2RAD(pitch));
	sp = sinf(DEG2RAD(pitch));
	cy = cosf(DEG2RAD(yaw));
	sy = sinf(DEG2RAD(yaw));

	if(side != NULL) {
		side->x = -cy;
		side->y = 0;
		side->z = sy;
	}

	if(up != NULL) {
		up->x = sy * sp;
		up->y = cp;
		up->z = cy * sp;
	}

	if(fwd != NULL) {
		fwd->x = sy * cp;
		fwd->y = -sp;
		fwd->z = cp * cy;
	}
}

void mat_view(mat4 dest, vec3 pos, vec3 ang)
{
	vec3 x, y, z;

	cam_angles(&z, &x, &y, ang.yaw, ang.pitch);
	z = vec3_invert(z);

	for(int i = 0; i < 3; i++) {
		dest[i][0] = x.array[i];
		dest[i][1] = y.array[i];
		dest[i][2] = z.array[i];
		dest[i][3] = 0.0f;
	}

	dest[3][0] = -vec3_dot(x, pos);
	dest[3][1] = -vec3_dot(y, pos);
	dest[3][2] = -vec3_dot(z, pos);
	dest[3][3] = 1.0f;
}

void mat_frustrum(mat4 dest, float left, float right, float bottom, float top, float znear, float zfar)
{
	float z2, dw, dh, dz;

	memset(dest, 0, 16 * sizeof(float));

	z2 = 2.0f * znear;
	dw = right - left;
	dh = top - bottom;
	dz = zfar - znear;
	dest[0][0] = z2 / dw;
	dest[1][1] = z2 / dh;
	dest[2][0] = (right + left) / dw;
	dest[2][1] = (top + bottom) / dh;
	dest[2][2] = (-zfar + znear) / dz;
	dest[2][3] = -1.0f;
	dest[3][2] = (-z2 * zfar) / dz;
}

void mat_projection(mat4 dest, float fov, float aspect, float znear, float zfar)
{
	float ymax, xmax;
	ymax = znear * tanf(DEG2RAD(fov) * 0.5f);
	xmax = ymax * aspect;
	mat_frustrum(dest, -xmax, xmax, -ymax, ymax, znear, zfar);
}
