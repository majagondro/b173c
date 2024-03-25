#include "vid.h"
#include "mathlib.h"
#include "common.h"
#include "client/client.h"
#include "glad/glad.h"
#include "game/world.h"
#include "hashmap.c/hashmap.h"
#include "meshbuilder.h"
#include "assets.h"
#include "client/cvar.h"

// todo: mesher thread

mat4_t view_mat = {0};
mat4_t proj_mat = {0};
static uint32_t gl_world_vao_simple, gl_world_vao_complex;
static uint32_t gl_world_texture; // fixme
static uint32_t gl_block_selection_vbo;
static GLint loc_chunkpos, loc_proj, loc_view, loc_nightlightmod;
static GLint loc_chunkpos2, loc_proj2, loc_view2, loc_nightlightmod2, loc_lighttex, loc_terraintex;

static int num_remeshed = 0;

struct {
    struct plane {
        vec3_t normal;
        float dist;
    } top, bottom, right, left, far, near;
} frustum = {0};

extern struct gl_state gl;

struct vert_simple makevert_simple(int x, int y, int z, ubyte texture_index, ubyte light, block_face face)
{
    struct vert_simple v = {0};
    v.x = x & 15;
    v.y = y;
    v.z = z & 15;
    v.texture_index = texture_index;
    v.data = face | (light << 3);
    return v;
}

struct vert_complex makevert_complex(vec3_t pos, vec2_t uv, ubyte texture_index, block_face face, ubyte light)
{
    struct vert_complex v = {0};
    v.pos = pos;
    v.uv = uv;
    v.texture_index = texture_index;
    v.data = face | (light << 3);
    v.r = 0xff;
    v.g = 0xff;
    v.b = 0xff;
    return v;
}

struct vert_complex *update_block_selection_box(bbox_t box)
{
    static struct vert_complex b[16];
    const float grow = 0.002f;
    float x0 = box.mins.x - grow;
    float y0 = box.mins.y - grow;
    float z0 = box.mins.z - grow;
    float x1 = box.maxs.x + grow;
    float y1 = box.maxs.y + grow;
    float z1 = box.maxs.z + grow;

    b[0] = makevert_complex(vec3(x0, y0, z0), vec2(0, 0), 0, 0, 0);
    b[1] = makevert_complex(vec3(x1, y0, z0), vec2(0, 0), 0, 0, 0);
    b[2] = makevert_complex(vec3(x1, y0, z1), vec2(0, 0), 0, 0, 0);
    b[3] = makevert_complex(vec3(x0, y0, z1), vec2(0, 0), 0, 0, 0);
    b[4] = b[0];
    b[5] = makevert_complex(vec3(x0, y1, z0), vec2(0, 0), 0, 0, 0);
    b[6] = makevert_complex(vec3(x1, y1, z0), vec2(0, 0), 0, 0, 0);
    b[7] = b[1];
    b[8] = b[6];
    b[9] = makevert_complex(vec3(x1, y1, z1), vec2(0, 0), 0, 0, 0);
    b[10] = b[2];
    b[11] = b[9];
    b[12] = makevert_complex(vec3(x0, y1, z1), vec2(0, 0), 0, 0, 0);
    b[13] = b[3];
    b[14] = b[12];
    b[15] = b[5];

    for(int i = 0; i < 16; i++)
        b[i].r = b[i].g = b[i].b = 0;

    return b;
}

static float get_dist_to_plane(const struct plane *p, vec3_t point)
{
    return vec3_dot(p->normal, point) - p->dist;
}

static struct plane make_plane(vec3_t point, vec3_t normal)
{
    struct plane p;
    p.normal = normal;
    p.dist = vec3_dot(normal, point);
    return p;
}

static bool is_visible_on_frustum(vec3_t point, float radius)
{
    return
            get_dist_to_plane(&frustum.right, point) > -radius &&
            get_dist_to_plane(&frustum.left, point) > -radius &&
            get_dist_to_plane(&frustum.bottom, point) > -radius &&
            get_dist_to_plane(&frustum.top, point) > -radius &&
            get_dist_to_plane(&frustum.far, point) > -radius &&
            get_dist_to_plane(&frustum.near, point) > -radius;
}

void world_renderer_update_chunk_visibility(world_chunk *chunk);

static void update_view_matrix_and_frustum(void)
{
    float half_h = r_zfar.value * tanf(deg2rad(fov.value) * 0.5f);
    float half_w = half_h * (vid_width.value / vid_height.value);
    vec3_t right, up, forward;
    vec3_t fwdFar, fwdNear;
    vec3_t tmp;

    static float accumulated_dt = 0.0f;
    static vec3_t lerp_pos;

    if(cl.is_physframe)
        accumulated_dt = 0.0f;
    tmp = vec3_mul(vec3_sub(cl.game.our_ent->position, cl.game.our_ent->position_old), accumulated_dt * 20.0f);
    lerp_pos = vec3_add(cl.game.our_ent->position_old, tmp);
    accumulated_dt += cl.frametime;

    if(cl_freecamera.integer)
        lerp_pos = cl.game.cam_pos;

    cam_angles(&forward, &right, &up, cl.game.our_ent->rotation.yaw, cl.game.our_ent->rotation.pitch);
    fwdFar = vec3_mul(forward, r_zfar.value);
    fwdNear = vec3_mul(forward, r_znear.value);

    /* update frustum */
    frustum.far = make_plane(vec3_add(lerp_pos, fwdFar), vec3_invert(forward));
    frustum.near = make_plane(vec3_add(lerp_pos, fwdNear), forward);

    tmp = vec3_mul(right, half_w);
    frustum.left = make_plane(lerp_pos, vec3_cross(vec3_sub(fwdFar, tmp), up));
    frustum.right = make_plane(lerp_pos, vec3_cross(up, vec3_add(fwdFar, tmp)));

    tmp = vec3_mul(up, half_h);
    frustum.top = make_plane(lerp_pos, vec3_cross(right, vec3_sub(fwdFar, tmp)));
    frustum.bottom = make_plane(lerp_pos, vec3_cross(vec3_add(fwdFar, tmp), right));

    /* update view matrix */
    mat4_view(view_mat, lerp_pos, cl.game.our_ent->rotation);

    /* update look trace and selection box */
    if(cl.state == cl_connected) {
        struct vert_complex *selection_box;
        bbox_t bbox;

        cl.game.look_trace = world_trace_ray(lerp_pos, forward, 5.0f);

        bbox = block_get_bbox(cl.game.look_trace.block, vec3_unpack(cl.game.look_trace), true);
        selection_box = update_block_selection_box(bbox);

        glBindBuffer(GL_ARRAY_BUFFER, gl_block_selection_vbo);
        glBufferData(GL_ARRAY_BUFFER,
                /*  size */ 16 * sizeof(struct vert_complex),
                /*  data */ selection_box,
                /* usage */ GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    /* update visibility of chunks */
    if(world_chunk_map != NULL && hashmap_count(world_chunk_map) > 0) {
        size_t i = 0;
        void *it;
        while(hashmap_iter(world_chunk_map, &i, &it)) {
            world_chunk *chunk = (world_chunk *) it;
            world_renderer_update_chunk_visibility(chunk);
        }
    }
}

void recalculate_projection_matrix(void)
{
    float aspect = (vid_width.value / vid_height.value);
    mat4_projection(proj_mat, fov.value, aspect, r_znear.value, r_zfar.value);
    update_view_matrix_and_frustum();
}

void world_renderer_init(void)
{
    asset_image *terrain_asset;

    recalculate_projection_matrix();

    loc_chunkpos = glGetUniformLocation(gl.shader_blocks_simple, "CHUNK_POS");
    loc_view = glGetUniformLocation(gl.shader_blocks_simple, "VIEW");
    loc_proj = glGetUniformLocation(gl.shader_blocks_simple, "PROJECTION");
    loc_nightlightmod = glGetUniformLocation(gl.shader_blocks_simple, "NIGHTTIME_LIGHT_MODIFIER");

    loc_chunkpos2 = glGetUniformLocation(gl.shader_blocks_complex, "CHUNK_POS");
    loc_view2 = glGetUniformLocation(gl.shader_blocks_complex, "VIEW");
    loc_proj2 = glGetUniformLocation(gl.shader_blocks_complex, "PROJECTION");
    loc_nightlightmod2 = glGetUniformLocation(gl.shader_blocks_complex, "NIGHTTIME_LIGHT_MODIFIER");
    loc_lighttex = glGetUniformLocation(gl.shader_blocks_complex, "LIGHT_TEX");
    loc_terraintex = glGetUniformLocation(gl.shader_blocks_complex, "TEXTURE");

    /* init vaos */
    glGenVertexArrays(1, &gl_world_vao_simple);
    glGenVertexArrays(1, &gl_world_vao_complex);

    glBindVertexArray(gl_world_vao_simple);
    glVertexAttribIFormat(0, 1, GL_UNSIGNED_SHORT, 0); // x,y,z, 1 bit padding
    glVertexAttribIFormat(1, 1, GL_UNSIGNED_SHORT, 2);  // texture index and data
    glVertexAttribBinding(0, 0);
    glVertexAttribBinding(1, 0);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    glBindVertexArray(gl_world_vao_complex);
    glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0); // position
    glVertexAttribFormat(1, 2, GL_FLOAT, GL_FALSE, sizeof(vec3_t)); // uv
    glVertexAttribIFormat(2, 1, GL_UNSIGNED_SHORT, sizeof(vec3_t) + sizeof(vec2_t)); // texture index, data
    glVertexAttribIFormat(3, 3, GL_UNSIGNED_BYTE, sizeof(vec3_t) + sizeof(vec2_t) + 2 * sizeof(ubyte)); // colormod
    glVertexAttribBinding(0, 0);
    glVertexAttribBinding(1, 0);
    glVertexAttribBinding(2, 0);
    glVertexAttribBinding(3, 0);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glBindVertexArray(0);

    /* load terrain texture */
    terrain_asset = asset_get_image(ASSET_TEXTURE_TERRAIN);

    glGenTextures(1, &gl_world_texture);
    glBindTexture(GL_TEXTURE_2D, gl_world_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, terrain_asset->width, terrain_asset->height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 terrain_asset->data);

    glBindTexture(GL_TEXTURE_2D, 0);

    glGenBuffers(1, &gl_block_selection_vbo);
}

void world_renderer_shutdown(void)
{
}

static void add_block_face(struct vert_complex v, block_face face)
{
    vec3_t offsets[6][4] = {
            [BLOCK_FACE_Y_NEG] = {vec3(0, 0, 0), vec3(0, 0, 1), vec3(1, 0, 0), vec3(1, 0, 1)},
            [BLOCK_FACE_Y_POS] = {vec3(0, 1, 0), vec3(1, 1, 0), vec3(0, 1, 1), vec3(1, 1, 1)},
            [BLOCK_FACE_Z_NEG] = {vec3(1, 1, 0), vec3(0, 1, 0), vec3(1, 0, 0), vec3(0, 0, 0)},
            [BLOCK_FACE_Z_POS] = {vec3(0, 1, 1), vec3(1, 1, 1), vec3(0, 0, 1), vec3(1, 0, 1)},
            [BLOCK_FACE_X_NEG] = {vec3(0, 1, 0), vec3(0, 1, 1), vec3(0, 0, 0), vec3(0, 0, 1)},
            [BLOCK_FACE_X_POS] = {vec3(1, 1, 1), vec3(1, 1, 0), vec3(1, 0, 1), vec3(1, 0, 0)}
    };

    struct vert_complex tl, tr, bl, br;

    tl = v;
    tr = v;
    bl = v;
    br = v;

    tl.pos.x += offsets[face][0].x;
    tl.pos.y += offsets[face][0].y;
    tl.pos.z += offsets[face][0].z;
    tl.uv = vec2(0, 0);

    tr.pos.x += offsets[face][1].x;
    tr.pos.y += offsets[face][1].y;
    tr.pos.z += offsets[face][1].z;
    tr.uv = vec2(1, 0);

    bl.pos.x += offsets[face][2].x;
    bl.pos.y += offsets[face][2].y;
    bl.pos.z += offsets[face][2].z;
    bl.uv = vec2(0, 1);

    br.pos.x += offsets[face][3].x;
    br.pos.y += offsets[face][3].y;
    br.pos.z += offsets[face][3].z;
    br.uv = vec2(1, 1);

    meshbuilder_add_quad(&tl, &tr, &bl, &br);
}

static void add_bottom_face(vec3_t v0, vec3_t v1, vec2_t uv0, vec2_t uv1, int texture, ubyte light)
{
    struct vert_complex tl, tr, bl, br;

    if(texture < 0) {
        swap(uv0.u, uv1.u);
        texture = -texture;
    }

    tl = makevert_complex(vec3(v0.x, v0.y, v0.z), vec2(uv0.u, uv1.v), texture, BLOCK_FACE_Y_NEG, light);
    tr = makevert_complex(vec3(v0.x, v0.y, v1.z), vec2(uv0.u, uv0.v), texture, BLOCK_FACE_Y_NEG, light);
    bl = makevert_complex(vec3(v1.x, v0.y, v0.z), vec2(uv1.u, uv1.v), texture, BLOCK_FACE_Y_NEG, light);
    br = makevert_complex(vec3(v1.x, v0.y, v1.z), vec2(uv1.u, uv0.v), texture, BLOCK_FACE_Y_NEG, light);

    meshbuilder_add_quad(&tl, &tr, &bl, &br);
}

static void add_top_face(vec3_t v0, vec3_t v1, vec2_t uv0, vec2_t uv1, int texture, ubyte light)
{
    struct vert_complex tl, tr, bl, br;

    if(texture < 0) {
        swap(uv0.u, uv1.u);
        texture = -texture;
    }

    tl = makevert_complex(vec3(v0.x, v1.y, v0.z), vec2(uv0.u, uv0.v), texture, BLOCK_FACE_Y_POS, light);
    tr = makevert_complex(vec3(v1.x, v1.y, v0.z), vec2(uv1.u, uv0.v), texture, BLOCK_FACE_Y_POS, light);
    bl = makevert_complex(vec3(v0.x, v1.y, v1.z), vec2(uv0.u, uv1.v), texture, BLOCK_FACE_Y_POS, light);
    br = makevert_complex(vec3(v1.x, v1.y, v1.z), vec2(uv1.u, uv1.v), texture, BLOCK_FACE_Y_POS, light);

    meshbuilder_add_quad(&tl, &tr, &bl, &br);
}

static void add_north_face(vec3_t v0, vec3_t v1, vec2_t uv0, vec2_t uv1, int texture, ubyte light)
{
    struct vert_complex tl, tr, bl, br;

    if(texture < 0) {
        swap(uv0.u, uv1.u);
        texture = -texture;
    }

    tl = makevert_complex(vec3(v1.x, v1.y, v0.z), vec2(uv0.u, uv0.v), texture, BLOCK_FACE_Z_NEG, light);
    tr = makevert_complex(vec3(v0.x, v1.y, v0.z), vec2(uv1.u, uv0.v), texture, BLOCK_FACE_Z_NEG, light);
    bl = makevert_complex(vec3(v1.x, v0.y, v0.z), vec2(uv0.u, uv1.v), texture, BLOCK_FACE_Z_NEG, light);
    br = makevert_complex(vec3(v0.x, v0.y, v0.z), vec2(uv1.u, uv1.v), texture, BLOCK_FACE_Z_NEG, light);

    meshbuilder_add_quad(&tl, &tr, &bl, &br);
}

static void add_south_face(vec3_t v0, vec3_t v1, vec2_t uv0, vec2_t uv1, int texture, ubyte light)
{
    struct vert_complex tl, tr, bl, br;

    if(texture < 0) {
        swap(uv0.u, uv1.u);
        texture = -texture;
    }

    tl = makevert_complex(vec3(v1.x, v0.y, v1.z), vec2(uv1.u, uv1.v), texture, BLOCK_FACE_Z_POS, light);
    tr = makevert_complex(vec3(v0.x, v0.y, v1.z), vec2(uv0.u, uv1.v), texture, BLOCK_FACE_Z_POS, light);
    bl = makevert_complex(vec3(v1.x, v1.y, v1.z), vec2(uv1.u, uv0.v), texture, BLOCK_FACE_Z_POS, light);
    br = makevert_complex(vec3(v0.x, v1.y, v1.z), vec2(uv0.u, uv0.v), texture, BLOCK_FACE_Z_POS, light);

    meshbuilder_add_quad(&tl, &tr, &bl, &br);
}

static void add_west_face(vec3_t v0, vec3_t v1, vec2_t uv0, vec2_t uv1, int texture, ubyte light)
{
    struct vert_complex tl, tr, bl, br;

    if(texture < 0) {
        swap(uv0.u, uv1.u);
        texture = -texture;
    }

    tl = makevert_complex(vec3(v0.x, v1.y, v0.z), vec2(uv1.u, uv0.v), texture, BLOCK_FACE_X_NEG, light);
    tr = makevert_complex(vec3(v0.x, v1.y, v1.z), vec2(uv0.u, uv0.v), texture, BLOCK_FACE_X_NEG, light);
    bl = makevert_complex(vec3(v0.x, v0.y, v0.z), vec2(uv1.u, uv1.v), texture, BLOCK_FACE_X_NEG, light);
    br = makevert_complex(vec3(v0.x, v0.y, v1.z), vec2(uv0.u, uv1.v), texture, BLOCK_FACE_X_NEG, light);

    meshbuilder_add_quad(&tl, &tr, &bl, &br);
}

static void add_east_face(vec3_t v0, vec3_t v1, vec2_t uv0, vec2_t uv1, int texture, ubyte light)
{
    struct vert_complex tl, tr, bl, br;

    if(texture < 0) {
        swap(uv0.u, uv1.u);
        texture = -texture;
    }

    tl = makevert_complex(vec3(v1.x, v1.y, v1.z), vec2(uv0.u, uv0.v), texture, BLOCK_FACE_X_POS, light);
    tr = makevert_complex(vec3(v1.x, v1.y, v0.z), vec2(uv1.u, uv0.v), texture, BLOCK_FACE_X_POS, light);
    bl = makevert_complex(vec3(v1.x, v0.y, v1.z), vec2(uv0.u, uv1.v), texture, BLOCK_FACE_X_POS, light);
    br = makevert_complex(vec3(v1.x, v0.y, v0.z), vec2(uv1.u, uv1.v), texture, BLOCK_FACE_X_POS, light);

    meshbuilder_add_quad(&tl, &tr, &bl, &br);
}

#define SIDE_XP 32
#define SIDE_XN 16
#define SIDE_ZP 8
#define SIDE_ZN 4
#define SIDE_YP 2
#define SIDE_YN 1
#define SIDE_ALL 63

static void render_box(int x, int y, int z, bbox_t box, block_data self, int f)
{
    float uv_x0p = (box.mins.x * 16.0f) / 16.0f;
    float uv_x1p = (box.maxs.x * 16.0f) / 16.0f;
    float uv_z0p = (box.mins.z * 16.0f) / 16.0f;
    float uv_z1p = (box.maxs.z * 16.0f) / 16.0f;
    float uv_y0n = (16.0f - box.mins.y * 16.0f) / 16.0f;
    float uv_y1n = (16.0f - box.maxs.y * 16.0f) / 16.0f;

    float x0 = (float) (x & 15) + box.mins.x;
    float y0 = (float) (y) + box.mins.y;
    float z0 = (float) (z & 15) + box.mins.z;

    float x1 = (float) (x & 15) + box.maxs.x;
    float y1 = (float) (y) + box.maxs.y;
    float z1 = (float) (z & 15) + box.maxs.z;

    vec3_t vec0 = vec3(x0, y0, z0);
    vec3_t vec1 = vec3(x1, y1, z1);

    int tex;
    ubyte light;

    if(box.mins.x < 0.0f || box.maxs.x > 1.0f) {
        uv_x0p = 0.0f;
        uv_x1p = 1.0f;
    }

    if(box.mins.z < 0.0f || box.maxs.z > 1.0f) {
        uv_z0p = 0.0f;
        uv_z1p = 1.0f;
    }

    if((f & SIDE_YN) && block_should_render_face(x, y, z, self, BLOCK_FACE_Y_NEG)) {
        tex = block_get_texture_index(self.id, BLOCK_FACE_Y_NEG, self.metadata, x, y, z);
        light = world_get_block_lighting(x, y - 1, z);
        add_bottom_face(vec0, vec1, vec2(uv_x0p, uv_z0p), vec2(uv_x1p, uv_z1p), tex, light);
    }

    if((f & SIDE_YP) && block_should_render_face(x, y, z, self, BLOCK_FACE_Y_POS)) {
        tex = block_get_texture_index(self.id, BLOCK_FACE_Y_POS, self.metadata, x, y, z);
        light = world_get_block_lighting(x, y + 1, z);
        add_top_face(vec0, vec1, vec2(uv_x0p, uv_z0p), vec2(uv_x1p, uv_z1p), tex, light);
    }

    if((f & SIDE_ZN) && block_should_render_face(x, y, z, self, BLOCK_FACE_Z_NEG)) {
        tex = block_get_texture_index(self.id, BLOCK_FACE_Z_NEG, self.metadata, x, y, z);
        light = world_get_block_lighting(x, y, z - 1);
        add_north_face(vec0, vec1, vec2(uv_x0p, uv_y1n), vec2(uv_x1p, uv_y0n), tex, light);
    }

    if((f & SIDE_ZP) && block_should_render_face(x, y, z, self, BLOCK_FACE_Z_POS)) {
        tex = block_get_texture_index(self.id, BLOCK_FACE_Z_POS, self.metadata, x, y, z);
        light = world_get_block_lighting(x, y, z + 1);
        add_south_face(vec0, vec1, vec2(uv_x0p, uv_y1n), vec2(uv_x1p, uv_y0n), tex, light);
    }

    if((f & SIDE_XN) && block_should_render_face(x, y, z, self, BLOCK_FACE_X_NEG)) {
        tex = block_get_texture_index(self.id, BLOCK_FACE_X_NEG, self.metadata, x, y, z);
        light = world_get_block_lighting(x - 1, y, z);
        add_west_face(vec0, vec1, vec2(uv_z1p, uv_y1n), vec2(uv_z0p, uv_y0n), tex, light);
    }

    if((f & SIDE_XP) && block_should_render_face(x, y, z, self, BLOCK_FACE_X_POS)) {
        tex = block_get_texture_index(self.id, BLOCK_FACE_X_POS, self.metadata, x, y, z);
        light = world_get_block_lighting(x + 1, y, z);
        add_east_face(vec0, vec1, vec2(uv_z0p, uv_y1n), vec2(uv_z1p, uv_y0n), tex, light);
    }
}

void render_fluid(int x, int y, int z, block_data self)
{
    if(block_should_render_face(x, y, z, self, BLOCK_FACE_Y_POS)) {
        float h_00, h_10, h_01, h_11;
        block_properties props = block_get_properties(self.id);
        struct vert_complex v1, v2, v3, v4;
        ubyte light;
        //vec3_t flowdir; todo
        //float flowangle;

        //if(!block_get_fluid_flow_direction(flowdir, x, y, z, self.id)) {
        //	flowangle = 0.0f;
        //} else {
        //	flowangle = RAD2DEG(atan2f(flowdir[2], flowdir[0]) - PI * 0.5f);
        //	con_printf("%f %f %f -> %f\n", flowdir[0], flowdir[1], flowdir[2], flowangle);
        //}

        h_00 = block_fluid_get_height(x, y, z, self.id);
        h_10 = block_fluid_get_height(x + 1, y, z, self.id);
        h_01 = block_fluid_get_height(x, y, z + 1, self.id);
        h_11 = block_fluid_get_height(x + 1, y, z + 1, self.id);

        light = world_get_block_lighting_fast(self, x, y, z);
        v1 = makevert_complex(
                vec3((x & 15), (y) + h_00, (z & 15)), vec2(0, 0),
                props.texture_indices[BLOCK_FACE_Y_POS],
                BLOCK_FACE_Y_POS, light);
        v2 = makevert_complex(
                vec3((x & 15) + 1, (y) + h_10, (z & 15)), vec2(1, 0),
                props.texture_indices[BLOCK_FACE_Y_POS],
                BLOCK_FACE_Y_POS, light);
        v3 = makevert_complex(
                vec3((x & 15), (y) + h_01, (z & 15) + 1), vec2(0, 1),
                props.texture_indices[BLOCK_FACE_Y_POS],
                BLOCK_FACE_Y_POS, light);
        v4 = makevert_complex(
                vec3((x & 15) + 1, (y) + h_11, (z & 15) + 1), vec2(1, 1),
                props.texture_indices[BLOCK_FACE_Y_POS],
                BLOCK_FACE_Y_POS, light);

        //if(flowangle < 0)
        //	flowangle += 360.0f;
        //v1.extradata = (short) flowangle;
        //v2.extradata = (short) flowangle;
        //v3.extradata = (short) flowangle;
        //v4.extradata = (short) flowangle;

        meshbuilder_add_quad(&v1, &v2, &v3, &v4);
    }
}

void render_null(int x attr(unused), int y attr(unused), int z attr(unused), block_data self attr(unused))
{

}

void render_cross(int _x, int _y, int _z, block_data self)
{
    struct vert_complex tl, tr, bl, br;
    float x0 = (float) (_x & 15);
    float y0 = (float) (_y);
    float z0 = (float) (_z & 15);
    float x1 = x0 + 1;
    float y1 = y0 + 1;
    float z1 = z0 + 1;
    ubyte light = world_get_block_lighting_fast(self, _x, _y, _z);
    ubyte texture = block_get_texture_index(self.id, 0, self.metadata, _x, _y, _z);

    tl = makevert_complex(vec3(x1, y1, z0), vec2(0, 0), texture, BLOCK_FACE_Y_POS, light);
    tr = makevert_complex(vec3(x0, y1, z1), vec2(1, 0), texture, BLOCK_FACE_Y_POS, light);
    bl = makevert_complex(vec3(x1, y0, z0), vec2(0, 1), texture, BLOCK_FACE_Y_POS, light);
    br = makevert_complex(vec3(x0, y0, z1), vec2(1, 1), texture, BLOCK_FACE_Y_POS, light);
    meshbuilder_add_quad(&tl, &tr, &bl, &br);

    tl.uv.u = 1;
    tr.uv.u = 0;
    bl.uv.u = 1;
    br.uv.u = 0;
    meshbuilder_add_quad(&tr, &tl, &br, &bl);

    tl = makevert_complex(vec3(x0, y1, z0), vec2(0, 0), texture, BLOCK_FACE_Y_POS, light);
    tr = makevert_complex(vec3(x1, y1, z1), vec2(1, 0), texture, BLOCK_FACE_Y_POS, light);
    bl = makevert_complex(vec3(x0, y0, z0), vec2(0, 1), texture, BLOCK_FACE_Y_POS, light);
    br = makevert_complex(vec3(x1, y0, z1), vec2(1, 1), texture, BLOCK_FACE_Y_POS, light);
    meshbuilder_add_quad(&tl, &tr, &bl, &br);

    tl.uv.u = 1;
    tr.uv.u = 0;
    bl.uv.u = 1;
    br.uv.u = 0;
    meshbuilder_add_quad(&tr, &tl, &br, &bl);
}

void render_torch_impl(int _x, int _y, int _z, float _xf, float _yf, float _zf, block_data self)
{
    float skew_x = 0.0f;
    float skew_z = 0.0f;
    float _1 = 1.0f / 16.0f;
    float _6 = 6.0f / 16.0f;
    float _7 = 7.0f / 16.0f;
    float _8 = 8.0f / 16.0f;
    float _9 = 9.0f / 16.0f;
    float _10 = 10.0f / 16.0f;

    float x = 0.5f;
    float y = 0.2f;
    float z = 0.5f;

    struct vert_complex tl, tr, bl, br;

    float x_mhalf;
    float x_phalf;
    float z_mhalf;
    float z_phalf;

    int t;
    ubyte l;

    switch(self.metadata) {
    case 1:
        skew_x = -0.4f;
        x -= 0.1f;
        break;
    case 2:
        skew_x = 0.4f;
        x += 0.1f;
        break;
    case 3:
        skew_z = -0.4f;
        z -= 0.1f;
        break;
    case 4:
        skew_z = 0.4f;
        z += 0.1f;
        break;
    default:
        y = 0.0f;
    }

    x += (float) (_x & 15);
    y += (float) (_y);
    z += (float) (_z & 15);

    x += _xf;
    y += _yf;
    z += _zf;

    x_mhalf = x - 0.5f;
    x_phalf = x + 0.5f;
    z_mhalf = z - 0.5f;
    z_phalf = z + 0.5f;

    l = world_get_block_lighting_fast(self, _x, _y, _z);

    // +y
    t = block_get_texture_index(self.id, BLOCK_FACE_Y_POS, self.metadata, _x, _y, _z);
    tr = makevert_complex(vec3(x + skew_x * _6 - _1, y + _10, z + skew_z * _6 - _1), vec2(_7, _6), t, BLOCK_FACE_Y_POS, l);
    tl = makevert_complex(vec3(x + skew_x * _6 - _1, y + _10, z + skew_z * _6 + _1), vec2(_7, _8), t, BLOCK_FACE_Y_POS, l);
    br = makevert_complex(vec3(x + skew_x * _6 + _1, y + _10, z + skew_z * _6 - _1), vec2(_9, _6), t, BLOCK_FACE_Y_POS, l);
    bl = makevert_complex(vec3(x + skew_x * _6 + _1, y + _10, z + skew_z * _6 + _1), vec2(_9, _8), t, BLOCK_FACE_Y_POS, l);
    meshbuilder_add_quad(&tl, &tr, &bl, &br);

    // -x
    t = block_get_texture_index(self.id, BLOCK_FACE_X_NEG, self.metadata, _x, _y, _z);
    tr = makevert_complex(vec3(x - _1, y + 1, z_mhalf), vec2(0, 0), t, BLOCK_FACE_Y_POS, l);
    tl = makevert_complex(vec3(x - _1 + skew_x, y + 0, z_mhalf + skew_z), vec2(0, 1), t, BLOCK_FACE_Y_POS, l);
    br = makevert_complex(vec3(x - _1, y + 1, z_phalf), vec2(1, 0), t, BLOCK_FACE_Y_POS, l);
    bl = makevert_complex(vec3(x - _1 + skew_x, y + 0, z_phalf + skew_z), vec2(1, 1), t, BLOCK_FACE_Y_POS, l);
    meshbuilder_add_quad(&tl, &tr, &bl, &br);

    // +x
    t = block_get_texture_index(self.id, BLOCK_FACE_X_POS, self.metadata, _x, _y, _z);
    tr = makevert_complex(vec3(x + _1, y + 1, z_phalf), vec2(0, 0), t, BLOCK_FACE_Y_POS, l);
    tl = makevert_complex(vec3(x + _1 + skew_x, y + 0, z_phalf + skew_z), vec2(0, 1), t, BLOCK_FACE_Y_POS, l);
    br = makevert_complex(vec3(x + _1, y + 1, z_mhalf), vec2(1, 0), t, BLOCK_FACE_Y_POS, l);
    bl = makevert_complex(vec3(x + _1 + skew_x, y + 0, z_mhalf + skew_z), vec2(1, 1), t, BLOCK_FACE_Y_POS, l);
    meshbuilder_add_quad(&tl, &tr, &bl, &br);

    // -z
    t = block_get_texture_index(self.id, BLOCK_FACE_Z_NEG, self.metadata, _x, _y, _z);
    tr = makevert_complex(vec3(x_mhalf + skew_x, y + 0, z - _1 + skew_z), vec2(1, 1), t, BLOCK_FACE_Y_POS, l);
    tl = makevert_complex(vec3(x_mhalf, y + 1, z - _1), vec2(1, 0), t, BLOCK_FACE_Y_POS, l);
    br = makevert_complex(vec3(x_phalf + skew_x, y + 0, z - _1 + skew_z), vec2(0, 1), t, BLOCK_FACE_Y_POS, l);
    bl = makevert_complex(vec3(x_phalf, y + 1, z - _1), vec2(0, 0), t, BLOCK_FACE_Y_POS, l);
    meshbuilder_add_quad(&tl, &tr, &bl, &br);

    // +z
    t = block_get_texture_index(self.id, BLOCK_FACE_Z_POS, self.metadata, _x, _y, _z);
    tr = makevert_complex(vec3(x_phalf + skew_x, y + 0, z + _1 + skew_z), vec2(1, 1), t, BLOCK_FACE_Y_POS, l);
    tl = makevert_complex(vec3(x_phalf, y + 1, z + _1), vec2(1, 0), t, BLOCK_FACE_Y_POS, l);
    br = makevert_complex(vec3(x_mhalf + skew_x, y + 0, z + _1 + skew_z), vec2(0, 1), t, BLOCK_FACE_Y_POS, l);
    bl = makevert_complex(vec3(x_mhalf, y + 1, z + _1), vec2(0, 0), t, BLOCK_FACE_Y_POS, l);
    meshbuilder_add_quad(&tl, &tr, &bl, &br);
}

void render_torch(int x, int y, int z, block_data self)
{
    render_torch_impl(x, y, z, 0, 0, 0, self);
}

void render_fire(int x, int y, int z, block_data self)
{
    block_data b = world_get_block(x, y - 1, z);

    if(!block_is_solid(b) && !block_is_flammable(b.id)) {
        // if we ain't got a block under, render as burning on the sides or ceiling

    } else {
        // render the full flame model

    }
}

static bool can_connect_to_wire(int x, int y, int z, block_face to_face)
{
    block_data b = world_get_block(x, y, z);
    const block_face dirs[] = {BLOCK_FACE_Z_POS, BLOCK_FACE_X_NEG, BLOCK_FACE_Z_NEG, BLOCK_FACE_X_POS};

    switch(b.id) {
    case BLOCK_REDSTONE_DUST:
    case BLOCK_TORCH_REDSTONE_ENABLED:
    case BLOCK_TORCH_REDSTONE_DISABLED:
    case BLOCK_BUTTON:
    case BLOCK_LEVER:
    case BLOCK_PRESSURE_PLATE_STONE:
    case BLOCK_PRESSURE_PLATE_WOOD:
    case BLOCK_RAIL_DETECTOR:
        return true;
    case BLOCK_REDSTONE_REPEATER_DISABLED:
    case BLOCK_REDSTONE_REPEATER_ENABLED:
        return dirs[b.metadata & 3] == to_face;
    }
    return false;
}

static bool wire_passthrough(block_data b)
{
    return block_is_transparent(b);
}

void render_wire(int x, int y, int z, block_data self)
{
    const float _1_64 = 1.0f / 64.0f;
    ubyte light = world_get_block_lighting(x, y, z);
    ubyte r, g, b;
    float power;
    bool xn, xp, zn, zp;
    bool uxn, uxp, uzn, uzp;
    int straight, t;
    vec3_t v0, v1;
    vec2_t uv0, uv1;
    float xf = (float) (x & 15);
    float yf = (float) (y);
    float zf = (float) (z & 15);
    struct vert_complex tl, tr, bl, br;

    power = (float) self.metadata / 15.0f;
    r = (ubyte) ((power * 0.6f + 0.4f) * 255.0f);
    if(self.metadata == 0)
        r = 77;
    g = (ubyte) max(0.0f, (power * power * 0.7f - 0.5f) * 255.0f);
    b = (ubyte) max(0.0f, (power * power * 0.6f - 0.7f) * 255.0f);

    uxn = uxp = uzn = uzp = false;
    xn = can_connect_to_wire(x - 1, y, z, BLOCK_FACE_X_POS);
    xp = can_connect_to_wire(x + 1, y, z, BLOCK_FACE_X_NEG);
    zn = can_connect_to_wire(x, y, z - 1, BLOCK_FACE_Z_POS);
    zp = can_connect_to_wire(x, y, z + 1, BLOCK_FACE_Z_NEG);

    // todo: connect down too....
    if(wire_passthrough(world_get_block(x, y + 1, z))) {
        uxn = can_connect_to_wire(x - 1, y + 1, z, BLOCK_FACE_X_POS);
        uxp = can_connect_to_wire(x + 1, y + 1, z, BLOCK_FACE_X_NEG);
        uzn = can_connect_to_wire(x, y + 1, z - 1, BLOCK_FACE_Z_POS);
        uzp = can_connect_to_wire(x, y + 1, z + 1, BLOCK_FACE_Z_NEG);
        xn |= uxn;
        xp |= uxp;
        zn |= uzn;
        zp |= uzp;
    }

    if((xn || xp) && !zn && !zp) {
        straight = 1;
    } else if((zn || zp) && !xp && !xn) {
        straight = 2;
    } else {
        straight = 0;
    }

    t = block_get_texture_index(self.id, BLOCK_FACE_Y_POS, self.metadata, x, y, z);
    if(straight)
        t++;

    v0 = vec3(xf, yf + _1_64, zf);
    v1 = vec3(xf + 1, yf + _1_64, zf + 1);
    uv0 = vec2(0, 0);
    uv1 = vec2(1, 1);

    if(!straight) {
        if(xn || xp || zn || zp || r_redstone_dot.integer != 0) {
            if(!xn) {
                v0.x += 5.0f / 16.0f;
                uv0.u += 5.0f / 16.0f;
            }
            if(!xp) {
                v1.x -= 5.0f / 16.0f;
                uv1.u -= 5.0f / 16.0f;
            }
            if(!zn) {
                v0.z += 5.0f / 16.0f;
                uv0.v += 5.0f / 16.0f;
            }
            if(!zp) {
                v1.z -= 5.0f / 16.0f;
                uv1.v -= 5.0f / 16.0f;
            }
        }
        tl = makevert_complex(vec3(v0.x, v1.y, v0.z), vec2(uv0.u, uv0.v), t, BLOCK_FACE_Y_POS, light);
        tr = makevert_complex(vec3(v1.x, v1.y, v0.z), vec2(uv1.u, uv0.v), t, BLOCK_FACE_Y_POS, light);
        bl = makevert_complex(vec3(v0.x, v1.y, v1.z), vec2(uv0.u, uv1.v), t, BLOCK_FACE_Y_POS, light);
        br = makevert_complex(vec3(v1.x, v1.y, v1.z), vec2(uv1.u, uv1.v), t, BLOCK_FACE_Y_POS, light);
    } else if(straight == 1) {
        tl = makevert_complex(vec3(v0.x, v1.y, v0.z), vec2(uv0.u, uv0.v), t, BLOCK_FACE_Y_POS, light);
        tr = makevert_complex(vec3(v1.x, v1.y, v0.z), vec2(uv1.u, uv0.v), t, BLOCK_FACE_Y_POS, light);
        bl = makevert_complex(vec3(v0.x, v1.y, v1.z), vec2(uv0.u, uv1.v), t, BLOCK_FACE_Y_POS, light);
        br = makevert_complex(vec3(v1.x, v1.y, v1.z), vec2(uv1.u, uv1.v), t, BLOCK_FACE_Y_POS, light);
    } else {
        tl = makevert_complex(vec3(v0.x, v1.y, v0.z), vec2(uv0.u, uv0.v), t, BLOCK_FACE_Y_POS, light);
        tr = makevert_complex(vec3(v1.x, v1.y, v0.z), vec2(uv0.u, uv1.v), t, BLOCK_FACE_Y_POS, light);
        bl = makevert_complex(vec3(v0.x, v1.y, v1.z), vec2(uv1.u, uv0.v), t, BLOCK_FACE_Y_POS, light);
        br = makevert_complex(vec3(v1.x, v1.y, v1.z), vec2(uv1.u, uv1.v), t, BLOCK_FACE_Y_POS, light);
    }

    tl.r = r;
    tl.g = g;
    tl.b = b;
    tr.r = r;
    tr.g = g;
    tr.b = b;
    bl.r = r;
    bl.g = g;
    bl.b = b;
    br.r = r;
    br.g = g;
    br.b = b;

    meshbuilder_add_quad(&tl, &tr, &bl, &br);
}

static void render_with_side_translation(int x, int y, int z, ubyte light, ubyte texture, float translation, bool diff_faces)
{
    struct vert_complex tl, tr, bl, br;
    float x0, x1, z0, z1;
    block_face face = BLOCK_FACE_Y_POS;

    x &= 15;
    z &= 15;

    x0 = x + translation;
    x1 = x + 1.0f - translation;
    z0 = z;
    z1 = z + 1.0f;

    if(diff_faces)
        face = BLOCK_FACE_X_NEG;
    tl = makevert_complex(vec3(x0, y + 1, z0), vec2(0, 0), texture, face, light);
    tr = makevert_complex(vec3(x0, y + 1, z1), vec2(1, 0), texture, face, light);
    bl = makevert_complex(vec3(x0, y, z0), vec2(0, 1), texture, face, light);
    br = makevert_complex(vec3(x0, y, z1), vec2(1, 1), texture, face, light);
    meshbuilder_add_quad(&tl, &tr, &bl, &br);
    meshbuilder_add_quad(&tr, &tl, &br, &bl);

    if(diff_faces)
        face = BLOCK_FACE_X_POS;
    tl = makevert_complex(vec3(x1, y + 1, z0), vec2(1, 0), texture, face, light);
    tr = makevert_complex(vec3(x1, y + 1, z1), vec2(0, 0), texture, face, light);
    bl = makevert_complex(vec3(x1, y, z0), vec2(1, 1), texture, face, light);
    br = makevert_complex(vec3(x1, y, z1), vec2(0, 1), texture, face, light);
    meshbuilder_add_quad(&tl, &tr, &bl, &br);
    meshbuilder_add_quad(&tr, &tl, &br, &bl);

    x0 = x;
    x1 = x + 1.0f;
    z0 = z + translation;
    z1 = z + 1.0f - translation;

    if(diff_faces)
        face = BLOCK_FACE_Z_NEG;
    tl = makevert_complex(vec3(x0, y + 1, z0), vec2(1, 0), texture, face, light);
    tr = makevert_complex(vec3(x1, y + 1, z0), vec2(0, 0), texture, face, light);
    bl = makevert_complex(vec3(x0, y, z0), vec2(1, 1), texture, face, light);
    br = makevert_complex(vec3(x1, y, z0), vec2(0, 1), texture, face, light);
    meshbuilder_add_quad(&tl, &tr, &bl, &br);
    meshbuilder_add_quad(&tr, &tl, &br, &bl);

    if(diff_faces)
        face = BLOCK_FACE_Z_POS;
    tl = makevert_complex(vec3(x0, y + 1, z1), vec2(0, 0), texture, face, light);
    tr = makevert_complex(vec3(x1, y + 1, z1), vec2(1, 0), texture, face, light);
    bl = makevert_complex(vec3(x0, y, z1), vec2(0, 1), texture, face, light);
    br = makevert_complex(vec3(x1, y, z1), vec2(1, 1), texture, face, light);
    meshbuilder_add_quad(&tl, &tr, &bl, &br);
    meshbuilder_add_quad(&tr, &tl, &br, &bl);
}

void render_crops(int x, int y, int z, block_data self)
{
    ubyte light = world_get_block_lighting_fast(self, x, y, z);
    ubyte texture = block_get_texture_index(self.id, 0, self.metadata, x, y, z);
    render_with_side_translation(x, y, z, light, texture, 0.25f, false);
}

void render_ladder(int x, int y, int z, block_data self)
{
    block_face face;
    struct vert_complex vert;
    ubyte texture, light;
    float xf = (float) (x & 15);
    float yf = (float) (y);
    float zf = (float) (z & 15);

    switch(self.metadata) {
    case 2:
        zf += 0.95f;
        face = BLOCK_FACE_Z_NEG;
        break;
    case 3:
        zf -= 0.95f;
        face = BLOCK_FACE_Z_POS;
        break;
    case 4:
        xf += 0.95f;
        face = BLOCK_FACE_X_NEG;
        break;
    case 5:
        xf -= 0.95f;
        face = BLOCK_FACE_X_POS;
        break;
    default:
        face = BLOCK_FACE_Y_POS;
        break;
    }

    light = world_get_block_lighting_fast(self, x, y, z);
    texture = block_get_texture_index(self.id, face, self.metadata, x, y, z);
    vert = makevert_complex(vec3(xf, yf, zf), vec2(0, 0), texture, face, light);

    add_block_face(vert, face);
}

void render_rail(int x, int y, int z, block_data self)
{
    int texture;
    ubyte light;
    float x0 = (float) (x & 15);
    float y0 = (float) (y);
    float z0 = (float) (z & 15);
    float _1 = 1.0f / 16.0f;
    float x1 = x0 + 1;
    float y1 = y0 + _1;
    float z1 = z0 + 1;

    struct vert_complex tl, tr, bl, br;
    vec3_t v_tl, v_tr, v_bl, v_br;
    vec2_t uv_tl, uv_tr, uv_bl, uv_br;

    v_tl = vec3(x0, y1, z0);
    v_tr = vec3(x1, y1, z0);
    v_bl = vec3(x0, y1, z1);
    v_br = vec3(x1, y1, z1);

    uv_tl = vec2(0, 0);
    uv_tr = vec2(1, 0);
    uv_bl = vec2(0, 1);
    uv_br = vec2(1, 1);

    light = world_get_block_lighting_fast(self, x, y, z);
    texture = block_get_texture_index(self.id, BLOCK_FACE_Y_POS, self.metadata, x, y, z);

    // see file notes/metadata_rails.png
    switch(self.metadata) {
    case 2:
        // go up L to R
        v_tr.y++;
        v_br.y++;
        break;
    case 3:
        // go up R to L
        v_tl.y++;
        v_bl.y++;
        break;
    case 4:
        // go up B to T
        v_tr.y++;
        v_tl.y++;
        break;
    case 5:
        // go up T to B
        v_br.y++;
        v_bl.y++;
        break;
    default:
        break;
    }
    switch(self.metadata) {
    case 1: // rotated rail
    case 2: // rotated rail going up R
    case 3: // rotated rail going up L
    case 7: // corner R to B
        uv_tl = vec2_rotate(uv_tl, 270.0f, vec2(0.5f, 0.5f));
        uv_tr = vec2_rotate(uv_tr, 270.0f, vec2(0.5f, 0.5f));
        uv_bl = vec2_rotate(uv_bl, 270.0f, vec2(0.5f, 0.5f));
        uv_br = vec2_rotate(uv_br, 270.0f, vec2(0.5f, 0.5f));
        break;
    case 8: // corner T to L
        uv_tl = vec2_rotate(uv_tl, 180.0f, vec2(0.5f, 0.5f));
        uv_tr = vec2_rotate(uv_tr, 180.0f, vec2(0.5f, 0.5f));
        uv_bl = vec2_rotate(uv_bl, 180.0f, vec2(0.5f, 0.5f));
        uv_br = vec2_rotate(uv_br, 180.0f, vec2(0.5f, 0.5f));
        break;
    case 9: // corner L to T
        uv_tl = vec2_rotate(uv_tl, 90.0f, vec2(0.5f, 0.5f));
        uv_tr = vec2_rotate(uv_tr, 90.0f, vec2(0.5f, 0.5f));
        uv_bl = vec2_rotate(uv_bl, 90.0f, vec2(0.5f, 0.5f));
        uv_br = vec2_rotate(uv_br, 90.0f, vec2(0.5f, 0.5f));
        break;
    default:
        break;
    }
    tl = makevert_complex(v_tl, uv_tl, texture, BLOCK_FACE_Y_POS, light);
    tr = makevert_complex(v_tr, uv_tr, texture, BLOCK_FACE_Y_POS, light);
    bl = makevert_complex(v_bl, uv_bl, texture, BLOCK_FACE_Y_POS, light);
    br = makevert_complex(v_br, uv_br, texture, BLOCK_FACE_Y_POS, light);
    meshbuilder_add_quad(&tl, &tr, &bl, &br); // top face
    meshbuilder_add_quad(&tr, &tl, &br, &bl); // bottom face
}

void add_stair_bboxes(bbox_t *colliders, size_t *n_colliders, block_data block, int x, int y, int z, bbox_t box);

void render_stairs(int x, int y, int z, block_data self)
{
    bbox_t bboxes[2] = {0};
    size_t n = 0;
    add_stair_bboxes(bboxes, &n, self, 0, 0, 0, (bbox_t){vec3_1(-1), vec3_1(1)});
    render_box(x, y, z, bboxes[0], self, SIDE_ALL);
    render_box(x, y, z, bboxes[1], self, SIDE_ALL);
}

void render_cactus(int x, int y, int z, block_data self)
{
    ubyte light;
    ubyte texture;

    if(block_should_render_face(x, y, z, self, BLOCK_FACE_Y_NEG)) {
        float xf = (float) (x & 15), yf = (float) (y), zf = (float) (z & 15);
        struct vert_complex vert;

        texture = block_get_texture_index(self.id, BLOCK_FACE_Y_NEG, self.metadata, x, y, z);
        light = world_get_block_lighting(x, y - 1, z);

        vert = makevert_complex(vec3(xf, yf, zf), vec2(0, 0), texture, BLOCK_FACE_Y_NEG, light);

        add_block_face(vert, BLOCK_FACE_Y_NEG);
    }

    if(block_should_render_face(x, y, z, self, BLOCK_FACE_Y_POS)) {
        float xf = (float) (x & 15), yf = (float) (y), zf = (float) (z & 15);
        struct vert_complex vert;

        texture = block_get_texture_index(self.id, BLOCK_FACE_Y_POS, self.metadata, x, y, z);
        light = world_get_block_lighting(x, y + 1, z);

        vert = makevert_complex(vec3(xf, yf, zf), vec2(0, 0), texture, BLOCK_FACE_Y_POS, light);

        add_block_face(vert, BLOCK_FACE_Y_POS);
    }

    /* render sides */
    light = world_get_block_lighting_fast(self, x, y, z);
    texture = block_get_texture_index(self.id, BLOCK_FACE_X_POS, self.metadata, x, y, z);
    render_with_side_translation(x, y, z, light, texture, 1.0f / 16.0f, true);
}

void render_bed(int x, int y, int z, block_data self)
{
    bbox_t bbox = block_get_bbox(self, 0, 0, 0, false);

    float x0 = (float) (x & 15);
    float y0 = (float) (y) + 3.0f / 16.0f;
    float z0 = (float) (z & 15);
    float x1 = x0 + 1;
    float y1 = y0 + 6.0f / 16.0f;
    float z1 = z0 + 1;

    ubyte light = world_get_block_lighting(x, y, z);
    int texture;

    struct vert_complex tl, tr, bl, br;
    vec3_t v_tl, v_tr, v_bl, v_br;
    vec2_t uv_tl, uv_tr, uv_bl, uv_br;

    uv_tl = vec2(0, 0);
    uv_tr = vec2(1, 0);
    uv_bl = vec2(0, 1);
    uv_br = vec2(1, 1);

    v_tl = vec3(x0, y0, z0);
    v_tr = vec3(x1, y0, z0);
    v_bl = vec3(x0, y0, z1);
    v_br = vec3(x1, y0, z1);
    texture = block_get_texture_index(self.id, BLOCK_FACE_Y_NEG, self.metadata, x, y, z);
    tl = makevert_complex(v_tl, uv_tl, texture, BLOCK_FACE_Y_NEG, light);
    tr = makevert_complex(v_tr, uv_tr, texture, BLOCK_FACE_Y_NEG, light);
    bl = makevert_complex(v_bl, uv_bl, texture, BLOCK_FACE_Y_NEG, light);
    br = makevert_complex(v_br, uv_br, texture, BLOCK_FACE_Y_NEG, light);
    meshbuilder_add_quad(&tr, &tl, &br, &bl);

    v_tl = vec3(x0, y1, z0);
    v_tr = vec3(x1, y1, z0);
    v_bl = vec3(x0, y1, z1);
    v_br = vec3(x1, y1, z1);

    switch(self.metadata & METADATA_BED_ROT) {
    case 0:
        uv_tl = vec2_rotate(uv_tl, 270.0f, vec2(0.5f, 0.5f));
        uv_tr = vec2_rotate(uv_tr, 270.0f, vec2(0.5f, 0.5f));
        uv_bl = vec2_rotate(uv_bl, 270.0f, vec2(0.5f, 0.5f));
        uv_br = vec2_rotate(uv_br, 270.0f, vec2(0.5f, 0.5f));
        break;
    case 1:
        uv_tl = vec2_rotate(uv_tl, 180.0f, vec2(0.5f, 0.5f));
        uv_tr = vec2_rotate(uv_tr, 180.0f, vec2(0.5f, 0.5f));
        uv_bl = vec2_rotate(uv_bl, 180.0f, vec2(0.5f, 0.5f));
        uv_br = vec2_rotate(uv_br, 180.0f, vec2(0.5f, 0.5f));
        break;
    case 2:
        uv_tl = vec2_rotate(uv_tl, 90.0f, vec2(0.5f, 0.5f));
        uv_tr = vec2_rotate(uv_tr, 90.0f, vec2(0.5f, 0.5f));
        uv_bl = vec2_rotate(uv_bl, 90.0f, vec2(0.5f, 0.5f));
        uv_br = vec2_rotate(uv_br, 90.0f, vec2(0.5f, 0.5f));
        break;
    default:
        break;
    }

    render_box(x, y, z, bbox, self, SIDE_ALL & ~(SIDE_YN | SIDE_YP));

    texture = block_get_texture_index(self.id, BLOCK_FACE_Y_POS, self.metadata, x, y, z);
    tl = makevert_complex(v_tl, uv_tl, texture, BLOCK_FACE_Y_POS, light);
    tr = makevert_complex(v_tr, uv_tr, texture, BLOCK_FACE_Y_POS, light);
    bl = makevert_complex(v_bl, uv_bl, texture, BLOCK_FACE_Y_POS, light);
    br = makevert_complex(v_br, uv_br, texture, BLOCK_FACE_Y_POS, light);
    meshbuilder_add_quad(&tl, &tr, &bl, &br);
}

void render_repeater(int x, int y, int z, block_data self)
{
    block_data torch = {BLOCK_TORCH_REDSTONE_DISABLED, 0, self.skylight, self.blocklight};
    float ox = 0.0f;
    float oz = 0.0f;
    int ticks = (self.metadata & METADATA_REPEATER_TICK) >> 2;
    float offset = -5.0f / 16.0f;

    float x0 = (float) (x & 15);
    float y0 = (float) (y);
    float z0 = (float) (z & 15);
    float x1 = x0 + 1;
    float y1 = y0 + 2.0f / 16.0f;
    float z1 = z0 + 1;

    ubyte light = world_get_block_lighting(x, y, z);
    int texture;

    struct vert_complex tl, tr, bl, br;
    vec3_t v_tl, v_tr, v_bl, v_br;
    vec2_t uv_tl, uv_tr, uv_bl, uv_br;

    if(self.id == BLOCK_REDSTONE_REPEATER_ENABLED)
        torch.id = BLOCK_TORCH_REDSTONE_ENABLED;

    switch(self.metadata & METADATA_REPEATER_ROT) {
    case 0:
        oz = offset;
        break;
    case 1:
        ox = -offset;
        break;
    case 2:
        oz = -offset;
        break;
    case 3:
        ox = offset;
        break;
    }
    render_torch_impl(x, y, z, ox, -0.2f, oz, torch);

    offset = -1.0f / 16.0f + (float) ticks * 2.0f / 16.0f;
    switch(self.metadata & METADATA_REPEATER_ROT) {
    case 0:
        oz = offset;
        break;
    case 1:
        ox = -offset;
        break;
    case 2:
        oz = -offset;
        break;
    case 3:
        ox = offset;
        break;
    }
    render_torch_impl(x, y, z, ox, -0.2f, oz, torch);

    render_box(x, y, z, block_get_bbox(self, 0, 0, 0, false), self, SIDE_ALL & ~(SIDE_YN | SIDE_YP));

    // render top face

    v_tl = vec3(x0, y1, z0);
    v_tr = vec3(x1, y1, z0);
    v_bl = vec3(x0, y1, z1);
    v_br = vec3(x1, y1, z1);

    uv_tl = vec2(0, 0);
    uv_tr = vec2(1, 0);
    uv_bl = vec2(0, 1);
    uv_br = vec2(1, 1);

    switch(self.metadata & METADATA_REPEATER_ROT) {
    case 1:
        uv_tl = vec2_rotate(uv_tl, 270.0f, vec2(0.5f, 0.5f));
        uv_tr = vec2_rotate(uv_tr, 270.0f, vec2(0.5f, 0.5f));
        uv_bl = vec2_rotate(uv_bl, 270.0f, vec2(0.5f, 0.5f));
        uv_br = vec2_rotate(uv_br, 270.0f, vec2(0.5f, 0.5f));
        break;
    case 2:
        uv_tl = vec2_rotate(uv_tl, 180.0f, vec2(0.5f, 0.5f));
        uv_tr = vec2_rotate(uv_tr, 180.0f, vec2(0.5f, 0.5f));
        uv_bl = vec2_rotate(uv_bl, 180.0f, vec2(0.5f, 0.5f));
        uv_br = vec2_rotate(uv_br, 180.0f, vec2(0.5f, 0.5f));
        break;
    case 3:
        uv_tl = vec2_rotate(uv_tl, 90.0f, vec2(0.5f, 0.5f));
        uv_tr = vec2_rotate(uv_tr, 90.0f, vec2(0.5f, 0.5f));
        uv_bl = vec2_rotate(uv_bl, 90.0f, vec2(0.5f, 0.5f));
        uv_br = vec2_rotate(uv_br, 90.0f, vec2(0.5f, 0.5f));
        break;
    default:
        break;
    }

    texture = block_get_texture_index(self.id, BLOCK_FACE_Y_POS, self.metadata, x, y, z);
    tl = makevert_complex(v_tl, uv_tl, texture, BLOCK_FACE_Y_POS, light);
    tr = makevert_complex(v_tr, uv_tr, texture, BLOCK_FACE_Y_POS, light);
    bl = makevert_complex(v_bl, uv_bl, texture, BLOCK_FACE_Y_POS, light);
    br = makevert_complex(v_br, uv_br, texture, BLOCK_FACE_Y_POS, light);
    meshbuilder_add_quad(&tl, &tr, &bl, &br);
}

void render_cube_special(int x, int y, int z, block_data self)
{
    bbox_t box = block_get_bbox(self, x, y, z, true);
    box.mins = vec3_sub(box.mins, vec3(x, y, z));
    box.maxs = vec3_sub(box.maxs, vec3(x, y, z));
    render_box(x, y, z, box, self, SIDE_ALL);
}

void render_grass_side_overlay(int x, int y, int z, block_data self)
{
    self.id = BLOCK_GRASS_OVERLAY;
    render_box(x, y, z, block_get_bbox(self, 0, 0, 0, false), self, SIDE_ALL & ~(SIDE_YN | SIDE_YP));
}

// todo: check whether all of these use the correct lighting like should they use the light at x,y,z or maybe xyz + face offset etc.
void (*render_funcs[RENDER_TYPE_COUNT])(int, int, int, block_data) = {
        [RENDER_CUBE] = render_cube_special, // handled elsewhere tho (not yet tho)
        [RENDER_CROSS] = render_cross,
        [RENDER_TORCH] = render_torch,
        [RENDER_FIRE] = render_fire,
        [RENDER_FLUID] = render_fluid,
        [RENDER_WIRE] = render_wire,
        [RENDER_CROPS] = render_crops,
        [RENDER_DOOR] = render_cube_special,
        [RENDER_LADDER] = render_ladder,
        [RENDER_RAIL] = render_rail,
        [RENDER_STAIRS] = render_stairs,
        [RENDER_FENCE] = render_null,
        [RENDER_LEVER] = render_null,
        [RENDER_CACTUS] = render_cactus,
        [RENDER_BED] = render_bed,
        [RENDER_REPEATER] = render_repeater,
        [RENDER_PISTON_BASE] = render_null,
        [RENDER_PISTON_EXTENSION] = render_null,
        [RENDER_CUBE_SPECIAL] = render_cube_special,
        [RENDER_NONE] = render_null
};

static void remesh_chunk_simple(world_chunk *chunk)
{
    if(!chunk->gl.needs_remesh_simple)
        return;

    chunk->gl.needs_remesh_simple = false;

    /* free old data */
    mem_free(chunk->gl.verts_simple);

    meshbuilder_start(sizeof(*chunk->gl.verts_simple));

    //

    meshbuilder_finish((void **) &chunk->gl.verts_simple, &chunk->gl.n_verts_simple, NULL, NULL);

    glBindBuffer(GL_ARRAY_BUFFER, chunk->gl.vbo_simple);
    glBufferData(GL_ARRAY_BUFFER,
            /*  size */ chunk->gl.n_verts_simple * sizeof(*chunk->gl.verts_simple),
            /*  data */ chunk->gl.verts_simple,
            /* usage */ GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void remesh_chunk_complex(world_chunk *chunk)
{
    if(!chunk->gl.needs_remesh_complex)
        return;

    chunk->gl.needs_remesh_complex = false;

    /* free old data */
    mem_free(chunk->gl.verts_complex);

    meshbuilder_start(sizeof(*chunk->gl.verts_complex));

    for(int x_off = 0; x_off < 16; x_off++) {
        for(int z_off = 0; z_off < 16; z_off++) {
            for(int y_off = 0; y_off < 128; y_off++) {
                int y = y_off;// + glbuf_idx * 16;
                int x = x_off + (chunk->x << 4);
                int z = z_off + (chunk->z << 4);
                block_data block = world_get_block_fast(chunk, x, y, z);
                block_properties props = block_get_properties(block.id);

                if(props.render_type == RENDER_CUBE && block.id == BLOCK_GRASS && r_fancygrass.integer != 0)
                    render_grass_side_overlay(x, y, z, block);

                //if(props.render_type == RENDER_CUBE)
                //    continue;

                render_funcs[props.render_type](x, y, z, block);
            }
        }
    }

    meshbuilder_finish((void **) &chunk->gl.verts_complex, &chunk->gl.n_verts_complex, NULL, NULL);

    glBindBuffer(GL_ARRAY_BUFFER, chunk->gl.vbo_complex);
    glBufferData(GL_ARRAY_BUFFER,
            /*  size */ chunk->gl.n_verts_complex * sizeof(*chunk->gl.verts_complex),
            /*  data */ chunk->gl.verts_complex,
            /* usage */ GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void remesh_chunk(world_chunk *chunk)
{
    ubyte data[128][32][32]; // D H W

    if(num_remeshed >= r_max_remeshes.integer)
        return;

    world_renderer_update_chunk_visibility(chunk);

    if(chunk->gl.needs_remesh_complex || chunk->gl.needs_remesh_simple)
        num_remeshed++;

    remesh_chunk_simple(chunk);
    remesh_chunk_complex(chunk);

    for(int w = 0; w < 18; w++) {
        for(int h = 0; h < 18; h++) {
            for(int d = 0; d < 128; d++) {
                block_data b = world_get_block((chunk->x << 4) + w - 1, d, (chunk->z << 4) + h - 1);
                data[d][h][w] = max(b.blocklight, b.skylight);
            }
        }
    }

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, chunk->gl.light_tex);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, 32, 32, 128, 0, GL_RED, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_3D, 0);
    glActiveTexture(GL_TEXTURE0);
}

void world_renderer_update_chunk_visibility(world_chunk *chunk)
{
    static const float cube_diagonal_half = SQRT_3 * WORLD_CHUNK_SIZE / 2.0f;

    int x_center = chunk->x * WORLD_CHUNK_SIZE + WORLD_CHUNK_SIZE / 2;
    int z_center = chunk->z * WORLD_CHUNK_SIZE + WORLD_CHUNK_SIZE / 2;

    for(int i = 0; i < 8; i++) {
        int y_center = i * WORLD_CHUNK_SIZE + WORLD_CHUNK_SIZE / 2;
        chunk->gl.visible = is_visible_on_frustum(vec3(x_center, y_center, z_center), cube_diagonal_half);
        if(chunk->gl.visible) {
            break;
        }
    }
}

void world_render(void)
{
    size_t i;
    void *it;

    if(cl.state < cl_connected)
        return;

    if(cl.game.rotated || cl.game.moved)
        recalculate_projection_matrix();
    else
        update_view_matrix_and_frustum();

    glLineWidth(1.0f);
    if(!strcasecmp(gl_polygon_mode.string, "GL_LINE")) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glBindTexture(GL_TEXTURE_2D, 0);
    } else if(!strcasecmp(gl_polygon_mode.string, "GL_POINT")) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        glBindTexture(GL_TEXTURE_2D, 0);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindTexture(GL_TEXTURE_2D, gl_world_texture);
    }

    glPointSize(5.0f);

    glUseProgram(gl.shader_blocks_simple);
    glUniformMatrix4fv(loc_view, 1, GL_FALSE, (const GLfloat *) view_mat);
    glUniformMatrix4fv(loc_proj, 1, GL_FALSE, (const GLfloat *) proj_mat);
    glUniform1f(loc_nightlightmod, world_calculate_sky_light_modifier());

    glBindVertexArray(gl_world_vao_simple);

    num_remeshed = 0;

    i = 0;
    while(hashmap_iter(world_chunk_map, &i, &it)) {
        world_chunk *chunk = (world_chunk *) it;
        vec3_t chunk_pos = vec3(chunk->x << 4, 0, chunk->z << 4);

        if(chunk->gl.needs_remesh_simple)
            remesh_chunk(chunk);

        /* go from top to bottom so that the gpu can discard pixels of cave meshes which won't be seen */

        if(!chunk->gl.visible)
            continue;

        if(chunk->gl.n_verts_simple > 0) {
            glUniform3fv(loc_chunkpos, 1, chunk_pos.array);
            glBindVertexBuffer(0, chunk->gl.vbo_simple, 0, sizeof(*chunk->gl.verts_simple));
            glDrawArrays(GL_TRIANGLES, 0, chunk->gl.n_verts_simple);
        }
    }

    glUseProgram(gl.shader_blocks_complex);

    glUniformMatrix4fv(loc_view2, 1, GL_FALSE, (const GLfloat *) view_mat);
    glUniformMatrix4fv(loc_proj2, 1, GL_FALSE, (const GLfloat *) proj_mat);
    glUniform1f(loc_nightlightmod2, world_calculate_sky_light_modifier());

    glActiveTexture(GL_TEXTURE0);
    if(!strcasecmp(gl_polygon_mode.string, "GL_LINE")) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glBindTexture(GL_TEXTURE_2D, 0);
    } else if(!strcasecmp(gl_polygon_mode.string, "GL_POINT")) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        glBindTexture(GL_TEXTURE_2D, 0);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindTexture(GL_TEXTURE_2D, gl_world_texture);
    }

    glUniform1i(loc_terraintex, 0);
    glUniform1i(loc_lighttex, 1);

    glBindVertexArray(gl_world_vao_complex);

    i = 0;
    while(hashmap_iter(world_chunk_map, &i, &it)) {
        world_chunk *chunk = (world_chunk *) it;
        vec3_t chunk_pos = vec3(chunk->x << 4, 0, chunk->z << 4);

        if(chunk->gl.needs_remesh_complex)
            remesh_chunk(chunk);

        /* go from top to bottom so that the gpu can discard pixels of cave meshes which won't be seen */

        if(!chunk->gl.visible)
            continue;

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_3D, chunk->gl.light_tex);

        if(chunk->gl.n_verts_complex > 0) {
            glUniform3fv(loc_chunkpos2, 1, chunk_pos.array);
            glBindVertexBuffer(0, chunk->gl.vbo_complex, 0, sizeof(*chunk->gl.verts_complex));
            glDrawArrays(GL_TRIANGLES, 0, chunk->gl.n_verts_complex);
        }
    }

    glBindTexture(GL_TEXTURE_3D, 0);
    glActiveTexture(GL_TEXTURE0);

    /* draw block selection box */
    if(!cl.game.look_trace.reached_end) {
        vec3_t cp = vec3_1(0);
        glUniform3fv(loc_chunkpos2, 1, cp.array);
        glLineWidth(2.0f);
        glBindVertexBuffer(0, gl_block_selection_vbo, 0, sizeof(struct vert_complex));
        glDrawArrays(GL_LINE_STRIP, 0, 16);
    }

    /* done */
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void world_gen_light_texture(world_chunk *chunk)
{
    GLuint tex;
    ubyte data[128][32][32]; // D H W

    for(int w = 0; w < 17; w++) {
        for(int h = 0; h < 17; h++) {
            for(int d = 0; d < 128; d++) {
                block_data b = world_get_block((chunk->x << 4) + w, d, (chunk->z << 4) + h);
                data[d][h][w] = (b.blocklight);
            }
        }
    }

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_3D, tex);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, 32, 32, 128, 0, GL_RED, GL_UNSIGNED_BYTE, data);

    glBindTexture(GL_TEXTURE_3D, 0);

    chunk->gl.light_tex = tex;
}

void world_init_chunk_glbufs(world_chunk *chunk)
{
    memset(&chunk->gl, 0, sizeof(chunk->gl));
    glGenBuffers(1, &chunk->gl.vbo_complex);
    glGenBuffers(1, &chunk->gl.vbo_simple);
    world_gen_light_texture(chunk);
}

void world_free_chunk_glbufs(world_chunk *chunk)
{
    mem_free(chunk->gl.verts_simple);
    mem_free(chunk->gl.verts_complex);
    glDeleteBuffers(1, &chunk->gl.vbo_simple);
    glDeleteBuffers(1, &chunk->gl.vbo_complex);
    glDeleteTextures(1, &chunk->gl.light_tex);
    memset(&chunk->gl, 0, sizeof(chunk->gl));
}
