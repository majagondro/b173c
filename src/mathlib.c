#include "mathlib.h"
#include "common.h"
#include <math.h>

vec2_t vec2_rotate(vec2_t v, float angle, vec2_t o)
{
    float cosa = cosf(deg2rad(angle));
    float sina = sinf(deg2rad(angle));
    vec2_t ret;

    ret.x = ((v.x - o.x) * cosa - (v.y - o.y) * sina) + o.x;
    ret.y = ((v.x - o.x) * sina + (v.y - o.y) * cosa) + o.y;

    return ret;
}

vec3_t vec3_normalize(vec3_t v)
{
    return vec3_div(v, vec3_len(v));
}

vec3_t vec3_cross(vec3_t a, vec3_t b)
{
    vec3_t s;

    s.x = a.y * b.z - a.z * b.y;
    s.y = a.z * b.x - a.x * b.z;
    s.z = a.x * b.y - a.y * b.x;

    return vec3_normalize(s);
}

void mat4_multiply(mat4_t dest, mat4_t a, mat4_t b)
{
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

vec4_t mat4_multiply_vec4(mat4_t m, vec4_t v)
{
    vec4_t ret;

    ret.x = m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z + m[3][0] * v.w;
    ret.y = m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1] * v.w;
    ret.z = m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2] * v.w;
    ret.w = m[0][3] * v.x + m[1][3] * v.y + m[2][3] * v.z + m[3][3] * v.w;

    return ret;
}

void mat4_identity(mat4_t dest)
{
    memset(dest, 0, 16 * sizeof(float));
    for(int i = 0; i < 4; i++) {
        dest[i][i] = 1;
    }
}

void mat4_translation(mat4_t dest, vec3_t translation)
{
    dest[3][0] = translation.x;
    dest[3][1] = translation.y;
    dest[3][2] = translation.z;
}

void mat4_rotation(mat4_t dest, vec3_t rotation)
{
    // https://en.wikipedia.org/wiki/Rotation_matrix#General_3D_rotations
    // THX

    float sina = sinf(deg2rad(rotation.pitch));
    float cosa = cosf(deg2rad(rotation.pitch));
    float sinb = sinf(deg2rad(rotation.yaw));
    float cosb = cosf(deg2rad(rotation.yaw));
    float siny = sinf(deg2rad(rotation.roll));
    float cosy = cosf(deg2rad(rotation.roll));

    dest[0][0] = cosb * cosy;
    dest[0][1] = cosb * siny;
    dest[0][2] = -sinb;

    dest[1][0] = sina * sinb * cosy - cosa * siny;
    dest[1][1] = sina * sinb * siny + cosa * cosy;
    dest[1][2] = sina * cosb;

    dest[2][0] = cosa * sinb * cosy + sina * siny;
    dest[2][1] = cosa * sinb * siny - sina * cosy;
    dest[2][2] = cosa * cosb;
}

void mat4_scale(mat4_t dest, vec3_t scale)
{
    dest[0][0] = scale.x;
    dest[1][1] = scale.y;
    dest[2][2] = scale.z;
}

void cam_angles(vec3_t *fwd, vec3_t *side, vec3_t *up, float yaw, float pitch)
{
    float cp, sp, cy, sy;
    cp = cosf(deg2rad(pitch));
    sp = sinf(deg2rad(pitch));
    cy = cosf(deg2rad(yaw));
    sy = sinf(deg2rad(yaw));

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

void mat4_view(mat4_t dest, vec3_t pos, vec3_t ang)
{
    vec3_t x, y, z;

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

void mat4_frustrum(mat4_t dest, float l, float r, float b, float t, float n, float f)
{
    float z2, dw, dh, dz;

    memset(dest, 0, 16 * sizeof(float));

    z2 = 2.0f * n;
    dw = r - l;
    dh = t - b;
    dz = f - n;
    dest[0][0] = z2 / dw;
    dest[1][1] = z2 / dh;
    dest[2][0] = (r + l) / dw;
    dest[2][1] = (t + b) / dh;
    dest[2][2] = (-f + n) / dz;
    dest[2][3] = -1.0f;
    dest[3][2] = (-z2 * f) / dz;
}

void mat4_projection(mat4_t dest, float fov, float aspect, float znear, float zfar)
{
    float ymax, xmax;
    ymax = znear * tanf(deg2rad(fov) * 0.5f);
    xmax = ymax * aspect;
    mat4_frustrum(dest, -xmax, xmax, -ymax, ymax, znear, zfar);
}

bbox_t bbox_offset(bbox_t bbox, vec3_t offset)
{
    bbox_t ret = bbox;
    ret.mins = vec3_add(ret.mins, offset);
    ret.maxs = vec3_add(ret.maxs, offset);
    return ret;
}

bbox_t bbox_join(bbox_t a, bbox_t b)
{
    bbox_t ret;

    ret.mins.x = min(a.mins.x, b.mins.x);
    ret.mins.y = min(a.mins.y, b.mins.y);
    ret.mins.z = min(a.mins.z, b.mins.z);
    ret.maxs.x = max(a.maxs.x, b.maxs.x);
    ret.maxs.y = max(a.maxs.y, b.maxs.y);
    ret.maxs.z = max(a.maxs.z, b.maxs.z);

    return ret;
}

bool bbox_null(bbox_t bbox)
{
    return bbox.mins.x == -1 && bbox.mins.y == -1 && bbox.mins.z == -1 &&
           bbox.maxs.x == -1 && bbox.maxs.y == -1 && bbox.maxs.z == -1;
}

bool bbox_intersects(bbox_t self, bbox_t other)
{
    bool overlap_x = other.maxs.x > self.mins.x && other.mins.x < self.maxs.x;
    bool overlap_y = other.maxs.y > self.mins.y && other.mins.y < self.maxs.y;
    bool overlap_z = other.maxs.z > self.mins.z && other.mins.z < self.maxs.z;
    return overlap_x && overlap_y && overlap_z;
}

bool bbox_intersects_line(bbox_t bbox, vec3_t start, vec3_t end, int *face)
{
    // https://gamedev.stackexchange.com/questions/18436/most-efficient-aabb-vs-ray-collision-algorithms/18459#18459
    // https://computergraphics.stackexchange.com/questions/9504/ray-vs-aabb-algorithm-that-also-gives-which-side-was-hit
    // yeah, i am bad at math

    vec3_t dir = vec3_normalize(vec3_sub(end, start));
    vec3_t dirfrac = vec3(1.0f / not0(dir.x), 1.0f / not0(dir.y), 1.0f / not0(dir.z));
    float t1 = (bbox.mins.x - start.x) * dirfrac.x;
    float t2 = (bbox.maxs.x - start.x) * dirfrac.x;
    float t3 = (bbox.mins.y - start.y) * dirfrac.y;
    float t4 = (bbox.maxs.y - start.y) * dirfrac.y;
    float t5 = (bbox.mins.z - start.z) * dirfrac.z;
    float t6 = (bbox.maxs.z - start.z) * dirfrac.z;

    float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
    float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

    if(tmax < 0) {
        return false;
    }

    if(tmin > tmax) {
        return false;
    }

    if(face != NULL) {
        /*
        BLOCK_FACE_Y_NEG = 0,
        BLOCK_FACE_Y_POS = 1,
        BLOCK_FACE_Z_NEG = 2,
        BLOCK_FACE_Z_POS = 3,
        BLOCK_FACE_X_NEG = 4,
        BLOCK_FACE_X_POS = 5
        */
        float tminx = min(t1, t2);
        float tminy = min(t3, t4);
        int axis = 2;
        *face = 2;

        tmin = max(max(tminx, tminy), min(t5, t6));
        if (tmin == tminx) {
            axis = 0;
            *face = 4;
        }
        if (tmin == tminy) {
            axis = 1;
            *face = 0;
        }
        if(dir.array[axis] < 0.0f) {
            (*face)++;
        }
    }

    return true;
}

vec3_t cam_project_3d_to_2d(vec3_t pos, mat4_t proj, mat4_t modelview, vec2_t vp)
{
    vec4_t in = vec4(pos.x, pos.y, pos.z, 1);
    vec4_t out;
    vec3_t in3;
    float viewport[4] = {0, 0, vp.x, vp.y};

    out = mat4_multiply_vec4(modelview, in);
    in = mat4_multiply_vec4(proj, out);

    if(!in.w)
        return vec3_1(0);

    in3 = vec3_div(in, in.w);
    in.x = in3.x, in.y = in3.y, in.z = in3.z;

    out.x = viewport[0] + (1 + in.x) * viewport[2] / 2.0f;
    out.y = viewport[3] - (viewport[1] + (1 + in.y) * viewport[3] / 2.0f);
    out.z = (1 + in.z) / 2.0f;

    return vec3(out.x, out.y, out.z);
}
