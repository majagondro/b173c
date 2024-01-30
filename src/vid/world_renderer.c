#include "vid.h"
#include "mathlib.h"
#include "common.h"
#include "client/client.h"
#include "glad/glad.h"
#include "game/world.h"
#include "client/console.h"
#include "hashmap.h"

#include "ext/terrain.c"

#define VIEW_HEIGHT 1.62f

static mat4 view_mat = {0};
static mat4 proj_mat = {0};
static uint gl_world_vao;
static u_int gl_world_texture; // fixme
static u_int loc_chunkpos, loc_proj, loc_view, loc_nightlightmod;

struct {
	struct plane {
		vec3 normal;
		float dist;
	} top, bottom, right, left, far, near;
} frustum = {0};

struct block {
	float x, y, z;
	// float data1, data2;
} __attribute__((packed));

static void recalculate_projection_matrix(void);

cvar fov = {"fov", "90", recalculate_projection_matrix};
cvar r_zfar = {"r_zfar", "256", recalculate_projection_matrix};
cvar r_znear = {"r_znear", "0.1", recalculate_projection_matrix};
cvar gl_polygon_mode = {"gl_polygon_mode", "GL_FILL"};

extern cvar vid_width, vid_height;

extern struct gl_state gl;

void world_renderer_init(void)
{
	cvar_register(&fov);
	cvar_register(&r_zfar);
	cvar_register(&r_znear);
	cvar_register(&gl_polygon_mode);

	recalculate_projection_matrix();

	loc_chunkpos = glGetUniformLocation(gl.shader3d, "CHUNK_POS");
	loc_view = glGetUniformLocation(gl.shader3d, "VIEW");
	loc_proj = glGetUniformLocation(gl.shader3d, "PROJECTION");

	loc_nightlightmod = glGetUniformLocation(gl.shader3d, "NIGHTTIME_LIGHT_MODIFIER");

	/* init vao */
	glGenVertexArrays(1, &gl_world_vao);

	glBindVertexArray(gl_world_vao);
		/* modified me? change block_vertex struct in world.h as well */
		/*                  idx sz  type       norm   offs */
		glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribFormat(1, 2, GL_FLOAT, GL_FALSE, 12);

		/*                  idx  bind_idx */
		glVertexAttribBinding(0, 0);
		glVertexAttribBinding(1, 0);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	/* load terrain texture */
	glGenTextures(1, &gl_world_texture);
	glBindTexture(GL_TEXTURE_2D, gl_world_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, terrain_png.width, terrain_png.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, terrain_png.pixel_data);
	glBindTexture(GL_TEXTURE_2D, 0);
}

static float get_dist_to_plane(const struct plane *p, const vec3 point)
{
	return vec3_dot(p->normal, point) - p->dist;
}

static struct plane make_plane(vec3 point, vec3 normal)
{
	struct plane p;
	vec3_copy(p.normal, normal);
	p.dist = vec3_dot(normal, point);
	return p;
}

static bool is_visible_on_frustum(const vec3 point, float radius)
{
	return
		get_dist_to_plane(&frustum.right, point) > -radius &&
		get_dist_to_plane(&frustum.left, point) > -radius &&
		get_dist_to_plane(&frustum.bottom, point) > -radius &&
		get_dist_to_plane(&frustum.top, point) > -radius &&
		get_dist_to_plane(&frustum.far, point) > -radius &&
		get_dist_to_plane(&frustum.near, point) > -radius;
}

static void update_view_matrix(void)
{
	float half_h = r_zfar.value * tanf(DEG2RAD(fov.value) * 0.5f);
	float half_w = half_h * (vid_width.value / vid_height.value);
	vec3 right, up, forward;
	vec3 fwdFar, fwdNear;
	vec3 normal;
	vec3 tmp;

	cam_angles(forward, right, up, cl.game.rot[1], cl.game.rot[0]);
	vec3_mul_scalar(fwdFar, forward, r_zfar.value);
	vec3_mul_scalar(fwdNear, forward, r_znear.value);

	cl.game.pos[1] += VIEW_HEIGHT;

	// update frustum
	vec3_add(tmp, cl.game.pos, fwdFar);
	vec3_invert(normal, forward);
	frustum.far = make_plane(tmp, normal);

	vec3_add(tmp, cl.game.pos, fwdNear);
	vec3_copy(normal, forward);
	frustum.near = make_plane(tmp, normal);

	vec3_mul_scalar(tmp, right, half_w);
	vec3_sub(tmp, fwdFar, tmp);
	vec3_cross(normal, tmp, up);
	frustum.left = make_plane(cl.game.pos, normal);

	vec3_mul_scalar(tmp, right, half_w);
	vec3_add(tmp, fwdFar, tmp);
	vec3_cross(normal, up, tmp);
	frustum.right = make_plane(cl.game.pos, normal);

	vec3_mul_scalar(tmp, up, half_h);
	vec3_sub(tmp, fwdFar, tmp);
	vec3_cross(normal, right, tmp);
	frustum.top = make_plane(cl.game.pos, normal);

	vec3_mul_scalar(tmp, up, half_h);
	vec3_add(tmp, fwdFar, tmp);
	vec3_cross(normal, tmp, right);
	frustum.bottom = make_plane(cl.game.pos, normal);

	// update view matrix
	mat_view(view_mat, cl.game.pos, cl.game.rot);

	// view offset
	cl.game.pos[1] -= VIEW_HEIGHT;
}

static void recalculate_projection_matrix(void)
{
	mat_projection(proj_mat, fov.value, (vid_width.value / vid_height.value), r_znear.value, r_zfar.value);
	update_view_matrix();
}

static bool alpha_blocks[256] = {
	[0] = true,
	[6] = true,
	[8] = true,
	[9] = true,
	[10] = true,
	[11] = true,
	[18] = true,
	[20] = true,
	[26] = true,
	[27] = true,
	[28] = true,
	[30] = true,
	[31] = true,
	[32] = true,
	[37] = true,
	[38] = true,
	[39] = true,
	[40] = true,
	[44] = true,
	[50] = true,
	[52] = true,
	[53] = true,
	[55] = true,
	[59] = true,
	[60] = true,
	[64] = true,
	[65] = true,
	[66] = true,
	[67] = true,
	[69] = true,
	[70] = true,
	[72] = true,
	[75] = true,
	[76] = true,
	[77] = true,
	[78] = true,
	[79] = true,
	[81] = true,
	[83] = true,
	[85] = true,
	[88] = true,
	[90] = true,
	[92] = true,
	[96] = true
};

static bool is_water(u_byte block_id)
{
	// todo: nether portal block too?
	return block_id == 8 || block_id == 9;
}

static u_byte getbl(struct chunk *c, int x, int y, int z)
{
	if(x < 0 || x > 15 || z < 0 || z > 15)
		return world_get_block(x + (c->x << 4), y, z + (c->z << 4));
	return c->data->blocks[IDX_FROM_COORDS(x, y, z)];
}

static int check_side(struct chunk *c, int x, int y, int z, u_byte idself, int side)
{
	int id = getbl(c, x, y, z);
	if(alpha_blocks[id] && ((idself == 18) || (id != idself)))
		return side;
	return 0;
}

#define FACE_Y_POS 1
#define FACE_Y_NEG 2
#define FACE_X_POS 4
#define FACE_X_NEG 8
#define FACE_Z_POS 16
#define FACE_Z_NEG 32

static int visible_faces(int x, int y, int z, struct chunk *c, int block_id_self)
{
	if(c->data->blocks[IDX_FROM_COORDS(x, y, z)] == 0)
		return 0; // air (id 0) isnt drawn

	return check_side(c, x, y + 1, z, block_id_self, FACE_Y_POS) |
		   check_side(c, x, y - 1, z, block_id_self, FACE_Y_NEG) |
		   check_side(c, x + 1, y, z, block_id_self, FACE_X_POS) |
		   check_side(c, x - 1, y, z, block_id_self, FACE_X_NEG) |
		   check_side(c, x, y, z + 1, block_id_self, FACE_Z_POS) |
		   check_side(c, x, y, z - 1, block_id_self, FACE_Z_NEG);
}

static u_byte getbl_light(struct chunk *c, int x, int y, int z)
{
	if(y < 0 || y > 127)
		return 0;
	if(x < 0 || x > 15 || z < 0 || z > 15)
		return world_get_block_lighting(x + (c->x << 4), y, z + (c->z << 4));
	return chunk_get_block_lighting(c, x, y, z);
}

static int calc_light_data(int sides, int x, int y, int z, struct chunk *c)
{
	u_byte l[6] = {0};
	if(sides & 1)
		l[0] = getbl_light(c, x, y + 1, z);
	if(sides & 2)
		l[1] = getbl_light(c, x, y - 1, z);
	if(sides & 4)
		l[2] = getbl_light(c, x + 1, y, z);
	if(sides & 8)
		l[3] = getbl_light(c, x - 1, y, z);
	if(sides & 16)
		l[4] = getbl_light(c, x, y, z + 1);
	if(sides & 32)
		l[5] = getbl_light(c, x, y, z - 1);
	return (l[0] & 15) |
		   (l[1] & 15) << 4 |
		   (l[2] & 15) << 8 |
		   (l[3] & 15) << 12 |
		   (l[4] & 15) << 16 |
		   (l[5] & 15) << 20;
}

struct render_buf {
	byte dirty;
	bool visible;
	int num_opaq_faces, num_alpha_faces;
	u_int gl_buf_opaq, gl_buf_alpha;
	struct block *opaque_faces, *alpha_faces;
};

extern struct hashmap *chunk_map;

static u_byte construct_fence_metadata(int x, int y, int z)
{
	u_byte nx = world_get_block(x - 1, y, z) == 85 ? 1 : 0;
	u_byte px = world_get_block(x + 1, y, z) == 85 ? 2 : 0;
	u_byte nz = world_get_block(x, y, z - 1) == 85 ? 4 : 0;
	u_byte pz = world_get_block(x, y, z + 1) == 85 ? 8 : 0;
	return nx | px | nz | pz;
}

/*static void emit_vertex(struct side **vbo_p, size_t *size_vbo, int x, int y, int z, int texture, int data)
{
	(*size_vbo)++;
	(*vbo_p)->x = x;
	(*vbo_p)->y = y;
	(*vbo_p)->z = z;
	(*vbo_p)->texture_index = texture;
	(*vbo_p)->data = data;
	(*vbo_p)++;
}

#define emit_face(...)    \
emit_vertex(__VA_ARGS__), \
emit_vertex(__VA_ARGS__), \
emit_vertex(__VA_ARGS__), \
emit_vertex(__VA_ARGS__), \
emit_vertex(__VA_ARGS__), \
emit_vertex(__VA_ARGS__)
*/

size_t total_size_vbos = 0;

static void add_vertex(struct chunk *c, struct block_vertex v)
{
	c->data->verts[c->data->num_verts++] = v;
}

static void build_buffer_for_chunk(struct chunk *chunk)
{
	mem_free(chunk->data->verts);
	chunk->data->verts = mem_alloc(16*16*128*6*sizeof(*chunk->data->verts));

	for(int x = 0; x < 16; x++) {
		for(int z = 0; z < 16; z++) {
			for(int y = 0; y < 128; y++) {

				int self_id = getbl(chunk, x, y, z);
				int face_flags = visible_faces(x, y, z, chunk, self_id);

				if(face_flags == 0)
					continue;

#define CHECK_FACE(face)                                                   \
do {                                                                       \
	if(face_flags & face) {                                                \
		/* emit 6 verts for a quad */                                      \
		add_vertex(chunk, (struct block_vertex){x,y,z,1,face});            \
		add_vertex(chunk, chunk->data->verts[chunk->data->num_verts - 1]); \
		add_vertex(chunk, chunk->data->verts[chunk->data->num_verts - 1]); \
		add_vertex(chunk, chunk->data->verts[chunk->data->num_verts - 1]); \
		add_vertex(chunk, chunk->data->verts[chunk->data->num_verts - 1]); \
		add_vertex(chunk, chunk->data->verts[chunk->data->num_verts - 1]); \
	}                                                                      \
} while(0)

				CHECK_FACE(FACE_Y_POS);
				CHECK_FACE(FACE_Y_NEG);
				CHECK_FACE(FACE_X_POS);
				CHECK_FACE(FACE_X_NEG);
				CHECK_FACE(FACE_Z_POS);
				CHECK_FACE(FACE_Z_NEG);
			}
		}
	}

	if(chunk->data->num_verts <= 0)
		return; // :-|

	// fixme realloc(): invalid next size for some reason (that was before i changed == 0 to <= 0 above)
	chunk->data->verts = reallocarray(chunk->data->verts, chunk->data->num_verts, sizeof(*chunk->data->verts));

	if(!chunk->data->gl_init) {
		glGenBuffers(1, &chunk->data->gl_vbo);
		chunk->data->gl_init = true;
	}

	/* update vertex buffer */
	glBindBuffer(GL_ARRAY_BUFFER, chunk->data->gl_vbo);
		glBufferData(GL_ARRAY_BUFFER,
			/* size */ chunk->data->num_verts * sizeof(*chunk->data->verts),
			/* data */ chunk->data->verts,
			/* usag */ GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static bool frustum_chunk_check(int x, int z)
{
	static const float cube_diagonal = 1.7321f /* sqrt(3) */ * 16.0f * 0.5f;
	// convert to block coordinates
	x <<= 4;
	z <<= 4;
	// move to center of chunk
	x += 8;
	z += 8;
	for(int y = 8; y < 128; y += 8) {
		if(is_visible_on_frustum((vec3){x, y, z}, cube_diagonal)) {
			return true;
		}
	}
	return false;
}

void build_buffers(void)
{
	size_t i = 0;
	void *it;
	bool built = false;

	while(hashmap_iter(chunk_map, &i, &it)) {
		struct chunk *c = (struct chunk *) it;

		c->visible = frustum_chunk_check(c->x, c->z);

		if(!c->dirty)
			continue;

		if(!built) {
			build_buffer_for_chunk(c);
			c->dirty = false;
			// only build 1 chunk per frame to avoid lag spikes
			built = true;
		}
	}
}

int wr_total_vertices;
int wr_draw_calls;

void world_render(void)
{
	size_t i = 0;
	void *it;

	wr_total_vertices = 0;
	wr_draw_calls = 0;

	if(cl.state < cl_connected)
		return;

	if(cl.game.rotated || cl.game.moved)
		recalculate_projection_matrix();

	build_buffers();

	if(!strcasecmp(gl_polygon_mode.string, "GL_LINE"))
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else if(!strcasecmp(gl_polygon_mode.string, "GL_POINT"))
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glPointSize(5.0f);
	glLineWidth(2.0f);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glUniformMatrix4fv(loc_view, 1, GL_FALSE, (const GLfloat *) view_mat);
	glUniformMatrix4fv(loc_proj, 1, GL_FALSE, (const GLfloat *) proj_mat);
	glUniform1f(loc_nightlightmod, world_get_light_modifier());

	glBindTexture(GL_TEXTURE_2D, gl_world_texture);

	glBindVertexArray(gl_world_vao);

	while(hashmap_iter(chunk_map, &i, &it)) {
		struct chunk *c = (struct chunk *) it;

		if(c->data->gl_init && c->visible) {
			vec2 chunk_pos = {c->x << 4, c->z << 4};
			glUniform2fv(loc_chunkpos, 1, chunk_pos);

			glBindVertexBuffer(0, c->data->gl_vbo, 0, sizeof(*c->data->verts));
			glDrawArrays(GL_TRIANGLES, 0, c->data->num_verts);

			wr_total_vertices += c->data->num_verts;
			wr_draw_calls++;
		}
	}

	/* done */
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_CULL_FACE);
}

void world_renderer_free_chunk_render_data(struct chunk *c)
{
	if(c->data->gl_init) {
		glDeleteBuffers(1, &c->data->gl_vbo);
	}
	mem_free(c->data->verts);
}

void world_renderer_shutdown(void)
{
}
