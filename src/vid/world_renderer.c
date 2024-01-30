#include "vid.h"
#include "mathlib.h"
#include "common.h"
#include "client/client.h"
#include "glad/glad.h"
#include "game/world.h"
#include "client/console.h"
#include "hashmap.h"
#include "vtxbuf.h"

#include "ext/terrain.c"

#define VIEW_HEIGHT 1.62f

static mat4 view_mat = {0};
static mat4 proj_mat = {0};
static u_int tex;
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

	/* load terrain texture */
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
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
	vec3 org = {0, 0, 0};

	cam_angles(forward, right, up, cl.game.rot[1], cl.game.rot[0]);
	vec3_mul_scalar(fwdFar, forward, r_zfar.value);
	vec3_mul_scalar(fwdNear, forward, r_znear.value);

	cl.game.pos[1] += VIEW_HEIGHT;

	// update frustum
	vec3_add(tmp, org, fwdFar);
	vec3_invert(normal, forward);
	frustum.far = make_plane(tmp, normal);

	vec3_add(tmp, org, fwdNear);
	vec3_copy(normal, forward);
	frustum.near = make_plane(tmp, normal);

	vec3_mul_scalar(tmp, right, half_w);
	vec3_sub(tmp, fwdFar, tmp);
	vec3_cross(normal, tmp, up);
	frustum.left = make_plane(org, normal);

	vec3_mul_scalar(tmp, right, half_w);
	vec3_add(tmp, fwdFar, tmp);
	vec3_cross(normal, up, tmp);
	frustum.right = make_plane(org, normal);

	vec3_mul_scalar(tmp, up, half_h);
	vec3_sub(tmp, fwdFar, tmp);
	vec3_cross(normal, right, tmp);
	frustum.top = make_plane(org, normal);

	vec3_mul_scalar(tmp, up, half_h);
	vec3_add(tmp, fwdFar, tmp);
	vec3_cross(normal, tmp, right);
	frustum.bottom = make_plane(org, normal);

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

static void build_buffer_for_chunk(struct chunk *chunk)
{
	struct vtxbuf *vtxbuf = &chunk->data->vtxbuf;

	//vtxbuf_free_leave_buffers(vtxbuf);
	// vtxbuf_free(vtxbuf);
	vtxbuf_new(vtxbuf, 0);

	for(int x = 0; x < 16; x++) {
		for(int z = 0; z < 16; z++) {
			for(int y = 0; y < 128; y++) {

				int self_id = getbl(chunk, x, y, z);
				int face_flags = visible_faces(x, y, z, chunk, self_id);

				//if(chunk->x == 0 && chunk->z == 0) {
				//	if(x > 8) {
				//		// printf("%d %d %d %d %d\n", x, y, z, self_id, face_flags);
				//	}
				//}

				if(face_flags == 0)
					continue;

#define CHECK_FACE(face)                                            \
do {                                                                \
	if(face_flags & face) {                                         \
		/* emit 6 verts for a quad */                               \
		vtxbuf_emit_vertex(vtxbuf, vtx_make(x+(chunk->x<<4),y,z+(chunk->z<<4),0,face));         \
		vtxbuf_emit_last(vtxbuf);                                   \
		vtxbuf_emit_last(vtxbuf);                                   \
		vtxbuf_emit_last(vtxbuf);                                   \
		vtxbuf_emit_last(vtxbuf);                                   \
		vtxbuf_emit_last(vtxbuf);                                   \
	}                                                               \
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

	if(!chunk->data->gl_init) {
		glGenVertexArrays(1, &chunk->data->gl_vao);
		glGenBuffers(1, &chunk->data->gl_vbo);
		glGenBuffers(1, &chunk->data->gl_ebo);

		glBindVertexArray(chunk->data->gl_vao);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk->data->gl_ebo);
			glBindBuffer(GL_ARRAY_BUFFER, chunk->data->gl_vbo);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct vtx), 0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct vtx), (void *) 12);
//			glVertexAttribPointer(0, 1, GL_UNSIGNED_INT, GL_FALSE, sizeof(struct vtx), 0); // x,z
//			glVertexAttribPointer(1, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(struct vtx), 1); // y
//			glVertexAttribPointer(2, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(struct vtx), 2); // t
//			glVertexAttribPointer(3, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(struct vtx), 3); // d
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
//			glEnableVertexAttribArray(2);
//			glEnableVertexAttribArray(3);
		glBindVertexArray(0);

		chunk->data->gl_init = true;
	}

	glBindVertexArray(chunk->data->gl_vao);
		/* update vertex buffer */
		glBindBuffer(GL_ARRAY_BUFFER, chunk->data->gl_vbo);
		glBufferData(GL_ARRAY_BUFFER,
			/* size */ chunk->data->vtxbuf.vertex_count * sizeof(struct vtx),
			/* data */ chunk->data->vtxbuf.vertices,
			/* usag */ GL_STATIC_DRAW);

		/* update index buffer */
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk->data->gl_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			/* size */ chunk->data->vtxbuf.index_count * sizeof(u_short),
			/* data */ chunk->data->vtxbuf.indices,
			/* usag */ GL_STATIC_DRAW);
	glBindVertexArray(0);

	con_printf("%d %d rebuilt - %d verts\n", chunk->x, chunk->z, chunk->data->vtxbuf.vertex_count);

	// total_size_vbos += chunk->data->vtxbuf.vertex_count * sizeof(*chunk->data->vtxbuf.vertices);
	// total_size_vbos += chunk->data->vtxbuf.index_count * sizeof(*chunk->data->vtxbuf.indices);

	// vtxbuf_free(vtxbuf);
}

/*{
struct side *vbo_p;
struct hashmap *vert_hashmap;


B_free(chunk->data->vbo);
chunk->data->vbo = B_malloc(sizeof(struct side) * 16 * 16 * 128 * 6);
chunk->data->size_vbo = 0;

vbo_p = chunk->data->vbo;

for(int x = 0; x < 16; x++) {
	for(int z = 0; z < 16; z++) {
		for(int y = 0; y < 128; y++) {
			int self_id = getbl(chunk, x, y, z);
			int face_flags = visible_faces(x, y, z, chunk, self_id);

			if(face_flags == 0)
				continue;

#define CHECK_FACE(face)                                             \
do {                                                                 \
if(face_flags & face) {                                          \
	emit_face(&vbo_p, &chunk->data->size_vbo, x, y, z, 1, face); \
}                                                                \
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

chunk->dirty = false;

// con_printf("%d\n", chunk->data->size_vbo);

glGenBuffers(1, &chunk->data->gl_vbo);
glGenBuffers(1, &chunk->data->gl_ebo);

glBindBuffer(GL_ARRAY_BUFFER, chunk->data->gl_vbo);
glBufferData(GL_ARRAY_BUFFER, chunk->data->size_vbo * sizeof(struct side), chunk->data->vbo, GL_STATIC_DRAW);
glBindBuffer(GL_ARRAY_BUFFER, 0);

glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk->data->gl_ebo);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, chunk->data->size_ebo * sizeof(uint), chunk->data->ebo, GL_STATIC_DRAW);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

total_size_vbos += chunk->data->size_vbo * sizeof(struct side);
//con_printf("size = %d bytes\n", chunk->data->size_vbo * sizeof(struct side));

// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk->data->gl_ebo);



}*/

void build_buffers(void)
{
	size_t i = 0;
	void *it;

	while(hashmap_iter(chunk_map, &i, &it)) {
		struct chunk *c = (struct chunk *) it;

		if(!c->dirty)
			continue;
		build_buffer_for_chunk(c);
		c->dirty = false;
	}
	//con_printf("VBO total size: %zu (%zu on avg)\n", total_size_vbos, total_size_vbos / hashmap_count(chunk_map));
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

	glPointSize(2.0f);

	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);

	glUniformMatrix4fv(loc_view, 1, GL_FALSE, (const GLfloat *) view_mat);
	glUniformMatrix4fv(loc_proj, 1, GL_FALSE, (const GLfloat *) proj_mat);
	glUniform1f(loc_nightlightmod, world_get_light_modifier());

	glBindTexture(GL_TEXTURE_2D, tex);

	while(hashmap_iter(chunk_map, &i, &it)) {
		struct chunk *c = (struct chunk *) it;

		if(c->data->gl_init) {
			vec2 chunk_pos = {c->x, c->z};
			glUniform2fv(loc_chunkpos, 1, chunk_pos);

			glBindVertexArray(c->data->gl_vao);
			//glDrawElements(GL_POINTS, c->data->vtxbuf.index_count, GL_UNSIGNED_SHORT, c->data->vtxbuf.indices);
			glDrawArrays(GL_TRIANGLES, 0, c->data->vtxbuf.vertex_count);

			wr_total_vertices += c->data->vtxbuf.vertex_count;
			wr_draw_calls++;
		}
	}

	/* done */
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	//glDisable(GL_CULL_FACE);
}

void world_renderer_free_chunk_render_data(struct chunk *c)
{
	if(c->data->gl_init) {
		glDeleteBuffers(1, &c->data->gl_ebo);
		glDeleteBuffers(1, &c->data->gl_vbo);
		glDeleteVertexArrays(1, &c->data->gl_vao);
	}
	vtxbuf_free(&c->data->vtxbuf);
}

void world_renderer_shutdown(void)
{
}
