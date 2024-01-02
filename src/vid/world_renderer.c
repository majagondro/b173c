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
static u_int vao, tex;
static u_int loc_mode, loc_proj, loc_view;

struct {
	struct plane {
		vec3 normal;
		float dist;
	} top, bottom, right, left, far, near;
} frustum = {0};

struct face {
	float x, y, z;
	float data;
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

	/* init vao */
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glVertexAttribFormat(0, 4, GL_FLOAT, GL_FALSE, 0);
	glVertexAttribBinding(0, 0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	loc_view = glGetUniformLocation(gl.shader3d, "VIEW");
	loc_proj = glGetUniformLocation(gl.shader3d, "PROJECTION");
	loc_mode = glGetUniformLocation(gl.shader3d, "mode");

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
	[8] = true,
	[9] = true,
	[18] = true,
	[31] = true,
	[32] = true,
	[37] = true,
	[38] = true,
	[39] = true,
	[40] = true,
	[44] = true,
	[70] = true,
	[72] = true,
	[81] = true,
	[83] = true
};

static int render_type(byte blockid)
{
	switch(blockid) {
		case 31:
		case 32:
		case 37:
		case 38:
		case 39:
		case 40:
		case 83:
			return 1; /* X */
		case 81:
			return 2; /* cactus */
		case 70:
		case 72:
		case 44:
			return 3; /* slab/pressure plate */
	}
	return 0;
}

#define make_idx(x, y, z) ((((x) & 15) << 11) | (((z) & 15) << 7) | ((y) & 127))

static u_byte getbl(struct chunk *c, int x, int y, int z)
{
	if(x < 0 || x > 15 || z < 0 || z > 15)
		return world_get_block(x + (c->x << 4), y, z + (c->z << 4));
	return c->data->blocks[make_idx(x, y, z)];
}

static int check_side(struct chunk *c, int x, int y, int z, u_byte idself, int side)
{
	int id = getbl(c, x, y, z);
	if(alpha_blocks[id] && ((idself == 18) || (id != idself)))
		return side;
	return 0;
}

static int visible_faces(int x, int y, int z, struct chunk *c, int block_id_self)
{
	if(c->data->blocks[make_idx(x, y, z)] == 0)
		return 0; // air (id 0) isnt drawn

	return check_side(c, x, y + 1, z, block_id_self, 1) |
		   check_side(c, x, y - 1, z, block_id_self, 2) |
		   check_side(c, x + 1, y, z, block_id_self, 4) |
		   check_side(c, x - 1, y, z, block_id_self, 8) |
		   check_side(c, x, y, z + 1, block_id_self, 16) |
		   check_side(c, x, y, z - 1, block_id_self, 32);
}


struct render_buf {
	byte dirty;
	bool visible;
	int num_opaq_faces, num_alpha_faces;
	u_int gl_buf_opaq, gl_buf_alpha;
	struct face *opaque_faces, *alpha_faces;
};

extern struct hashmap *chunk_map;

static void build_buffers(void)
{
	// fixme: +zoom, load new chunks, unzoom, some old chunks get rebuilt?
	size_t i = 0;
	void *it;

	/* construct buffer data */
	while(hashmap_iter(chunk_map, &i, &it)) {
		struct chunk *c = (struct chunk *) it;
		c->visible = false;

		if(!c->dirty)
			continue;

		for(int rb = 0; rb < 8; rb++) {
			vec3 center;

			if(c->data->render_bufs[rb] == NULL) {
				c->data->render_bufs[rb] = B_malloc(sizeof(struct render_buf));
				*c->data->render_bufs[rb] = 1; // new buf, mark dirty
			}

			center[0] = (c->x << 4) - cl.game.pos[0] + 8;
			center[1] = (rb << 4) - cl.game.pos[1] + 8;
			center[2] = (c->z << 4) - cl.game.pos[2] + 8;

			if(!is_visible_on_frustum(center, 14.0f)) {
				((struct render_buf *) c->data->render_bufs[rb])->visible = false;
				continue;
			} else {
				struct render_buf *r_buf = (struct render_buf *) c->data->render_bufs[rb];
				int o_face;
				int t_face;
				int *face;

				// mark chunk and this piece as visible
				c->visible = true;
				r_buf->visible = true;

				// not changed, no need to rebuild the mesh
				if(!r_buf->dirty)
					continue;

				r_buf->dirty = false;
				r_buf->num_opaq_faces = 0;
				r_buf->num_alpha_faces = 0;

				if(!r_buf->gl_buf_opaq)
					glGenBuffers(1, &r_buf->gl_buf_opaq);
				if(!r_buf->gl_buf_alpha)
					glGenBuffers(1, &r_buf->gl_buf_alpha);

				/* count visible faces */
				for(int x = 0; x < 16; x++) {
					for(int z = 0; z < 16; z++) {
						for(int y = 0; y < 16; y++) {
							int block_y = (rb << 4) + y;
							int block_id = c->data->blocks[make_idx(x, block_y, z)];
							if(block_id > 0) {
								int n = visible_faces(x, block_y, z, c, block_id) > 0 ? 1 : 0;
								if(alpha_blocks[block_id]) {
									r_buf->num_alpha_faces += n;
								} else {
									r_buf->num_opaq_faces += n;
								}
							}
						}
					}
				}

				/* no faces, don't build any buffers */
				if(r_buf->num_opaq_faces <= 0 && r_buf->num_alpha_faces <= 0) {
					r_buf->visible = false;
					return; // return, dont stall the cpu too much by building
					        // all rbufs or we get lag spikes
					continue;
				}

				/* build buffers */
				o_face = t_face = 0;
				r_buf->opaque_faces = B_malloc(sizeof(struct face) * r_buf->num_opaq_faces);
				r_buf->alpha_faces = B_malloc(sizeof(struct face) * r_buf->num_alpha_faces);
				for(int x = 0; x < 16; x++) {
					for(int z = 0; z < 16; z++) {
						for(int y = 0; y < 16; y++) {
							int block_y = (rb << 4) + y;
							int block_id = c->data->blocks[make_idx(x, block_y, z)];
							if(block_id > 0) {
								struct face *buf;
								int sides;

								if(alpha_blocks[block_id]) {
									buf = r_buf->alpha_faces;
									face = &t_face;
								} else {
									buf = r_buf->opaque_faces;
									face = &o_face;
								}

								sides = visible_faces(x, block_y, z, c, block_id);
								if(sides != 0) {
									buf[*face].x = (float) x + (float) (c->x << 4);
									buf[*face].z = (float) z + (float) (c->z << 4);
									buf[*face].y = (float) block_y;
									buf[*face].data = (float) (sides + (block_id << 6) + (render_type(block_id) << 14));
									(*face)++;
								}
							}
						}
					}
				}

				if(r_buf->num_opaq_faces > 0) {
					glBindBuffer(GL_ARRAY_BUFFER, r_buf->gl_buf_opaq);
					glBufferData(GL_ARRAY_BUFFER, r_buf->num_opaq_faces * sizeof(struct face), r_buf->opaque_faces, GL_DYNAMIC_DRAW);
					glBindBuffer(GL_ARRAY_BUFFER, 0);
				}
				if(r_buf->num_alpha_faces > 0) {
					glBindBuffer(GL_ARRAY_BUFFER, r_buf->gl_buf_alpha);
					glBufferData(GL_ARRAY_BUFFER, r_buf->num_alpha_faces * sizeof(struct face), r_buf->alpha_faces, GL_DYNAMIC_DRAW);
					glBindBuffer(GL_ARRAY_BUFFER, 0);
				}
				return;
			}
		}
	}
}

float dist_to_player(struct chunk *c)
{
	float xd = cl.game.pos[0] - ((c->x << 4) + 8);
	float zd = cl.game.pos[2] - ((c->z << 4) + 8);
	return xd * xd + zd * zd;
}

int wr_total_faces;
int wr_draw_calls;

void world_render(void)
{
	size_t i = 0;
	void *it;

	wr_total_faces = 0;
	wr_draw_calls = 0;

	if(cl.state < cl_connected)
		return;

	if(cl.game.rotated || cl.game.moved)
		recalculate_projection_matrix();

	build_buffers();

	if(!strcasecmp(gl_polygon_mode.string, "GL_LINE")) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else if(!strcasecmp(gl_polygon_mode.string, "GL_POINT")) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	glPointSize(5.0f);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glUniformMatrix4fv(loc_view, 1, GL_FALSE, (const GLfloat *) view_mat);
	glUniformMatrix4fv(loc_proj, 1, GL_FALSE, (const GLfloat *) proj_mat);

	glBindTexture(GL_TEXTURE_2D, tex);

	glBindVertexArray(vao);

	while(hashmap_iter(chunk_map, &i, &it)) {
		struct chunk *c = (struct chunk *) it;

		if(!c->visible)
			continue;

		for(int rb = 0; rb < 8; rb++) {
			struct render_buf *r_buf = (struct render_buf *) c->data->render_bufs[rb];
			if(!r_buf || !r_buf->visible)
				continue;

			if(r_buf->num_opaq_faces > 0) {
				glUniform1i(loc_mode, 0);
				glBindVertexBuffer(0, r_buf->gl_buf_opaq, 0, sizeof(vec4));
				glDrawArrays(GL_POINTS, 0, r_buf->num_opaq_faces);
				wr_total_faces += r_buf->num_opaq_faces;
				wr_draw_calls++;
			}
		}
	}

	i = 0;
	while(hashmap_iter(chunk_map, &i, &it)) {
		struct chunk *c = (struct chunk *) it;

		if(!c->visible)
			continue;

		for(int rb = 0; rb < 8; rb++) {
			struct render_buf *r_buf = (struct render_buf *) c->data->render_bufs[rb];
			if(!r_buf || !r_buf->visible)
				continue;

			if(r_buf->num_alpha_faces > 0) {
				glUniform1i(loc_mode, 1);
				glBindVertexBuffer(0, r_buf->gl_buf_alpha, 0, sizeof(vec4));
				glDrawArrays(GL_POINTS, 0, r_buf->num_alpha_faces);
				wr_total_faces += r_buf->num_alpha_faces;
				wr_draw_calls++;
			}
		}
	}

	/* done */
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_CULL_FACE);
}

void world_renderer_free_chunk_rbufs(struct chunk *c)
{
	for(int rb = 0; rb < 8; rb++) {
		struct render_buf *r_buf = (struct render_buf *) c->data->render_bufs[rb];
		if(!r_buf)
			continue;
		glDeleteBuffers(1, &r_buf->gl_buf_opaq);
		glDeleteBuffers(1, &r_buf->gl_buf_alpha);
		B_free(r_buf->opaque_faces);
		B_free(r_buf->alpha_faces);
		B_free(r_buf);
	}
}

void world_renderer_shutdown(void)
{
	glDeleteBuffers(1, &vao);
}
