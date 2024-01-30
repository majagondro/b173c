#include "mathlib.h"
#include "common.h"
#include <math.h>

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

void cam_angles(vec3 fwd, vec3 side, vec3 up, float yaw, float pitch)
{
	float cp, sp, cy, sy;
	cp = cosf(DEG2RAD(pitch));
	sp = sinf(DEG2RAD(pitch));
	cy = cosf(DEG2RAD(yaw));
	sy = sinf(DEG2RAD(yaw));

	side[0] = -cy;
	side[1] = 0;
	side[2] = sy;

	up[0] = sy * sp;
	up[1] = cp;
	up[2] = cy * sp;

	fwd[0] = sy * cp;
	fwd[1] = -sp;
	fwd[2] = cp * cy;
}

void mat_view(mat4 dest, const vec3 pos, vec3 ang)
{
	vec3 x, y, z;

	cam_angles(z, x, y, ang[1], ang[0]);
	vec3_invert(z, z);
	//vec3_invert(x, x);

	for(int i = 0; i < 3; i++) {
		dest[i][0] = x[i];
		dest[i][1] = y[i];
		dest[i][2] = z[i];
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

void vec3_normalize(vec3 dest, const vec3 a)
{
	float len = sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
	dest[0] = a[0] / len;
	dest[1] = a[1] / len;
	dest[2] = a[2] / len;
}

void vec3_cross(vec3 dest, const vec3 a, const vec3 b)
{
	dest[0] = a[1] * b[2] - a[2] * b[1];
	dest[1] = a[2] * b[0] - a[0] * b[2];
	dest[2] = a[0] * b[1] - a[1] * b[0];
	vec3_normalize(dest, dest);
}
