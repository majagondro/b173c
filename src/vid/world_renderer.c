#include "vid.h"
#include "mathlib.h"
#include "common.h"
#include "client/client.h"
#include "glad/glad.h"
#include "game/world.h"
#include "client/console.h"

#define Z_NEAR 0.1f
#define Z_FAR 1024.0f

static mat4 view_mat = {0};
static mat4 proj_mat = {0};
static u_int vao;
static u_int vbo;
static u_int loc_mode, loc_proj, loc_view;

struct {
	struct plane {
		vec3 normal;
		float dist;
	} top, bottom, right, left, far, near;
} frustum = {0};

struct world_renderer {
	int x, y, z;
	bool updated;
};

static void recalculate_projection_matrix(void);
cvar fov = {"fov", "90.0f", recalculate_projection_matrix};
extern cvar vid_width, vid_height;

extern struct gl_state gl;

void world_renderer_init(void)
{
	cvar_register(&fov);

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), (void *) 0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);

	recalculate_projection_matrix();

	loc_view = glGetUniformLocation(gl.shader3d, "VIEW");
	loc_proj = glGetUniformLocation(gl.shader3d, "PROJECTION");
	loc_mode = glGetUniformLocation(gl.shader3d, "mode");
}

extern struct chunk *chunks;

static float get_dist_to_plane(const struct plane *p, const vec3 point)
{
	return vec3_dot(p->normal, point) - p->dist;
}

static struct plane make_plane(vec3 point, vec3 normal)
{
	struct plane p;
	vec3_copy(p.normal, normal);
	p.dist = vec3_dot(normal, point);
	//p.dist = vec3_len(point);
	//con_printf("dist = %f\n", p.dist);
	return p;
}

static bool is_visible_on_frustum(const vec3 point, float radius)
{
	return
		//get_dist_to_plane(&frustum.near, point) > -radius &&
		//get_dist_to_plane(&frustum.far, point) > -radius &&
		get_dist_to_plane(&frustum.right, point) > -radius &&
		get_dist_to_plane(&frustum.left, point) > -radius &&
		get_dist_to_plane(&frustum.top, point) > -radius &&
		get_dist_to_plane(&frustum.bottom, point) > -radius;
}

static void update_view_matrix(void)
{
	float half_h = Z_FAR * tanf(DEG2RAD(fov.value) * 0.5f);
	float half_w = half_h * (vid_width.value / vid_height.value);
	vec3 right, up, forward;
	vec3 fwdFar;
	vec3 normal;
	vec3 tmp;
	vec3 org = {0,0,0};

	cam_angles(forward, right, up, cl.game.rot[1], cl.game.rot[0]);
	vec3_mul_scalar(fwdFar, forward, Z_FAR);

	cl.game.pos[0] -= 0.5;
	cl.game.pos[1] += 0.62f;
	cl.game.pos[2] -= 0.5;

	// update frustum

	// broken
	vec3_mul_scalar(tmp, forward, Z_NEAR);
	frustum.near = make_plane(tmp, forward);

	// broken
	vec3_add(tmp, org, fwdFar);
	vec3_invert(normal, forward);
	frustum.far = make_plane(tmp, normal);

	vec3_mul_scalar(tmp, right, half_w);
	vec3_sub(tmp, fwdFar, tmp);
	vec3_cross(normal, tmp, up);
	frustum.right = make_plane(org, normal);

	vec3_mul_scalar(tmp, right, half_w);
	vec3_add(tmp, fwdFar, tmp);
	vec3_cross(normal, up, tmp);
	frustum.left = make_plane(org, normal);

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
	cl.game.pos[0] += 0.5;
	cl.game.pos[1] -= 0.62f;
	cl.game.pos[2] += 0.5;
}

static void recalculate_projection_matrix(void)
{
	mat_projection(proj_mat, fov.value, (vid_width.value / vid_height.value), Z_NEAR, Z_FAR);
	update_view_matrix();
}

#define make_idx(x,y,z) ((((x) & 15) << 11) | (((z) & 15) << 7) | ((y) & 127))

static byte getbl(byte *blocks, int x, int y, int z)
{
	if((x & 15) != x || (z & 15) != z || (y & 127) != y)
		return 1;
	return blocks[make_idx(x,y,z)];
}

#define is_transparent(blockid) (blockid == 0 || blockid == 8 || blockid == 9)

static int num_visible_faces(int x, int y, int z, byte *blocks)
{
#define check(xx,yy,zz) (is_transparent(getbl(blocks,xx,yy,zz)) && getbl(blocks,xx,yy,zz) != getbl(blocks,x,y,z) ? 1 : 0)
	if(blocks[make_idx(x,y,z)] <= 0)
		return 0;
	//if(x == 0 || z == 0)//if(x == 0 || x == 15 || z == 0 || z == 15)
	//	return false;
	return check(x,y+1,z) + check(x,y-1,z) + check(x+1,y,z) + check(x-1,y,z) + check(x,y,z+1) + check(x,y,z-1);
}

#undef check

static int visible_faces(int x, int y, int z, byte *blocks)
{
#define check(xx,yy,zz,side) (is_transparent(getbl(blocks,xx,yy,zz)) && getbl(blocks,xx,yy,zz) != getbl(blocks,x,y,z) ? side : 0)
	if(blocks[make_idx(x,y,z)] <= 0)
		return 0;
	//if(x == 0 || z == 0)
	//	return false; ;//return check2(x,y+1,z,1) * 100000 + check2(x,y-1,z,2) * 10000 + check2(x+1,y,z,3) * 1000 + check2(x-1,y,z,4) * 100 + check2(x,y,z+1,5) * 10 + check2(x,y,z-1,6);
	return check(x,y+1,z,1) * 100000 + check(x,y-1,z,2) * 10000 + check(x+1,y,z,3) * 1000 + check(x-1,y,z,4) * 100 + check(x,y,z+1,5) * 10 + check(x,y,z-1,6);
}


struct render_buf {
	byte dirty;
	bool visible;
	int num_vis_opaque_faces;
	int num_vis_transparent_faces;
	struct face {
		float x, y, z;
		float data;
	} *opaque_faces, *transparent_faces;
};

static int powi(int b, int e)
{
	int r = 1;
	while(e-- > 0)
		r *= b;
	return r;
}

static void build_buffers(void)
{
	struct chunk *c;

	/* construct buffer data */
	for(c = chunks; c != NULL; c = c->next) {
		c->visible = false;

		if(!c->dirty)
			continue;

		for(int rb = 0; rb < 8; rb ++) {
			vec3 center;

			if(c->data->render_bufs[rb] == NULL) {
				c->data->render_bufs[rb] = B_malloc(sizeof(struct render_buf));
				*c->data->render_bufs[rb] = 1; // new buf, mark dirty
			}

			center[0] = (c->x << 4) - cl.game.pos[0] + 8;
			center[1] = (  rb << 4) - cl.game.pos[1] + 8;
			center[2] = (c->z << 4) - cl.game.pos[2] + 8;

			if(!is_visible_on_frustum(center, 14.0f)) {
				((struct render_buf *)c->data->render_bufs[rb])->visible = false;
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
				r_buf->num_vis_opaque_faces = 0;
				r_buf->num_vis_transparent_faces = 0;

				/* count visible faces */
				for(int x = 0; x < 16; x++) {
					for(int z = 0; z < 16; z++) {
						for(int y = 0; y < 16; y++) {
							int block_y = (rb << 4) + y;
							int block_id = c->data->blocks[make_idx(x, block_y, z)];
							if(block_id > 0) {
								if(is_transparent(block_id)) {
									r_buf->num_vis_transparent_faces += num_visible_faces(x, block_y, z, c->data->blocks);
								} else {
									r_buf->num_vis_opaque_faces += num_visible_faces(x, block_y, z, c->data->blocks);
								}
							}
						}
					}
				}

				if(r_buf->num_vis_opaque_faces <= 0 && r_buf->num_vis_transparent_faces <= 0) {
					continue;
				}

				/* build buffer */
				o_face = t_face = 0;
				r_buf->opaque_faces = B_malloc(sizeof(struct face) * r_buf->num_vis_opaque_faces);
				r_buf->transparent_faces = B_malloc(sizeof(struct face) * r_buf->num_vis_transparent_faces);
				for(int x = 0; x < 16; x++) {
					for(int z = 0; z < 16; z++) {
						for(int y = 0; y < 16; y++) {
							int block_y = (rb << 4) + y;
							int block_id = c->data->blocks[make_idx(x, block_y, z)];
							if(block_id > 0) {
								struct face *buf;

								if(is_transparent(block_id)) {
									buf = r_buf->transparent_faces;
									face = &t_face;
								} else {
									buf = r_buf->opaque_faces;
									face = &o_face;
								}

								int sides = visible_faces(x, block_y, z, c->data->blocks);
#define getsid(sides,i) (sides / powi(10,i) % 10)
								if(getsid(sides, 0)) {
									buf[*face].x = (float) x + (float)(c->x << 4);
									buf[*face].z = (float) z + (float)(c->z << 4);
									buf[*face].y = (float) block_y;
									buf[*face].data = 0.0f + (10.0f * block_id);
									(*face)++;
								}
								if(getsid(sides, 1)) {
									buf[*face].x = (float) x + (float)(c->x << 4);
									buf[*face].z = (float) z + (float)(c->z << 4);
									buf[*face].y = (float) block_y;
									buf[*face].data = 1.0f + (10.0f * block_id);
									(*face)++;
								}
								if(getsid(sides, 2)) {
									buf[*face].x = (float) x + (float)(c->x << 4);
									buf[*face].z = (float) z + (float)(c->z << 4);
									buf[*face].y = (float) block_y;
									buf[*face].data = 2.0f + (10.0f * block_id);
									(*face)++;
								}
								if(getsid(sides, 3)) {
									buf[*face].x = (float) x + (float)(c->x << 4);
									buf[*face].z = (float) z + (float)(c->z << 4);
									buf[*face].y = (float) block_y;
									buf[*face].data = 3.0f + (10.0f * block_id);
									(*face)++;
								}
								if(getsid(sides, 4)) {
									buf[*face].x = (float) x + (float)(c->x << 4);
									buf[*face].z = (float) z + (float)(c->z << 4);
									buf[*face].y = (float) block_y;
									buf[*face].data = 4.0f + (10.0f * block_id);
									(*face)++;
								}
								if(getsid(sides, 5)) {
									buf[*face].x = (float) x + (float)(c->x << 4);
									buf[*face].z = (float) z + (float)(c->z << 4);
									buf[*face].y = (float) block_y;
									buf[*face].data = 5.0f + (10.0f * block_id);
									(*face)++;
								}
							}
						}
					}
				}
			}
		}
	}
}

static void sort_chunks(void)
{

}

int wr_total_faces;
int wr_draw_calls;

void world_render(void)
{
	struct chunk *c;

	wr_total_faces = 0;
	wr_draw_calls = 0;

	if(cl.state < cl_connected)
		return;

	if(cl.game.rotated || cl.game.moved)
		recalculate_projection_matrix();

	build_buffers();

	glEnable(GL_CULL_FACE);

	glUniformMatrix4fv(loc_view, 1, GL_FALSE, (const GLfloat *) view_mat);
	glUniformMatrix4fv(loc_proj, 1, GL_FALSE, (const GLfloat *) proj_mat);

	glBindVertexArray(vao);

	/* render opaque objects */

	for(c = chunks; c != NULL; c = c->next) {
		if(!c->visible)
			continue;

		for(int rb = 0; rb < 8; rb ++) {
			struct render_buf *r_buf = (struct render_buf *) c->data->render_bufs[rb];
			if(!r_buf || !r_buf->visible || r_buf->num_vis_opaque_faces <= 0)
				continue;

			glBindBuffer(GL_ARRAY_BUFFER, vbo);

			if(r_buf->num_vis_opaque_faces > 0) {
				glUniform1i(loc_mode, 0);
				glBufferData(GL_ARRAY_BUFFER, r_buf->num_vis_opaque_faces * sizeof(struct face), r_buf->opaque_faces, GL_DYNAMIC_DRAW);
				glDrawArrays(GL_POINTS, 0, r_buf->num_vis_opaque_faces);
				wr_total_faces += r_buf->num_vis_opaque_faces;
				wr_draw_calls++;
			}

			if(r_buf->num_vis_transparent_faces > 0) {
				glUniform1i(loc_mode, 1);
				glBufferData(GL_ARRAY_BUFFER, r_buf->num_vis_transparent_faces * sizeof(struct face), r_buf->transparent_faces, GL_DYNAMIC_DRAW);
				glDrawArrays(GL_POINTS, 0, r_buf->num_vis_transparent_faces);
				wr_total_faces += r_buf->num_vis_transparent_faces;
				wr_draw_calls++;
			}
		}
	}

	/* render transparent objects */
	/*glUniform1i(loc_mode, 1);
	for(c = chunks; c != NULL; c = c->next) {
		if(!c->visible)
			continue;

		for(int rb = 0; rb < 8; rb ++) {
			struct render_buf *r_buf = (struct render_buf *) c->data->render_bufs[rb];
			if(!r_buf || !r_buf->visible || r_buf->num_vis_faces <= 0 || !r_buf->has_transparent)
				continue;

			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER,  r_buf->num_vis_faces * sizeof(struct face), r_buf->faces, GL_DYNAMIC_DRAW);
			glDrawArrays(GL_POINTS, 0, r_buf->num_vis_faces);

			wr_total_faces += r_buf->num_vis_faces;
			wr_draw_calls++;
		}
	}*/

	/* done */
	glBindVertexArray(0);
	glDisable(GL_CULL_FACE);
}

void world_renderer_shutdown(void)
{
	glDeleteBuffers(1, &vao);
	glDeleteBuffers(1, &vbo);
}
