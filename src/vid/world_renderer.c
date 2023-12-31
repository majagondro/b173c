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
static u_int loc_model, loc_proj, loc_view;

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

void recalculate_projection_matrix(void);
cvar fov = {"fov", "90.0f", recalculate_projection_matrix};
extern cvar vid_width, vid_height;

void recalculate_projection_matrix(void)
{
	mat_projection(proj_mat, fov.value, vid_width.value / vid_height.value, Z_NEAR, Z_FAR);
}

extern struct gl_state gl;

void world_renderer_init(void)
{
	cvar_register(&fov);

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), (void *) 0);
	//glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void *) sizeof(vec3));
	glEnableVertexAttribArray(0);
	//glEnableVertexAttribArray(1);

	glBindVertexArray(0);

	recalculate_projection_matrix();

	loc_view = glGetUniformLocation(gl.shader3d, "VIEW");
	loc_proj = glGetUniformLocation(gl.shader3d, "PROJECTION");
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
	p.dist = vec3_dot(p.normal, point);
	return p;
}

static bool is_visible_on_frustum(const vec3 point, float radius)
{
	return
		get_dist_to_plane(&frustum.near, point) > -radius &&
		get_dist_to_plane(&frustum.far, point) > -radius &&
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

	cam_angles(forward, right, up, cl.game.rot[1], cl.game.rot[0]);
	vec3_mul_scalar(fwdFar, forward, Z_FAR);

	cl.game.pos[0] -= 0.5;
	cl.game.pos[1] += 0.62f;
	cl.game.pos[2] -= 0.5;

	// update frustum
	vec3_mul_scalar(tmp, forward, Z_NEAR);
	frustum.near = make_plane(tmp, forward);

	vec3_add(tmp, cl.game.pos, fwdFar);
	vec3_invert(normal, forward);
	frustum.far = make_plane(tmp, normal);

	vec3_mul_scalar(tmp, right, half_w);
	vec3_sub(tmp, fwdFar, tmp);
	vec3_cross(normal, tmp, up);
	frustum.right = make_plane(cl.game.pos, normal);

	vec3_mul_scalar(tmp, right, half_w);
	vec3_add(tmp, fwdFar, tmp);
	vec3_cross(normal, up, tmp);
	frustum.left = make_plane(cl.game.pos, normal);

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
	cl.game.pos[0] += 0.5;
	cl.game.pos[1] -= 0.62f;
	cl.game.pos[2] += 0.5;

}

static bool has_visible_face(int x, int y, int z, byte *blocks)
{
#define make_idx(x,y,z) ((((x) & 15) << 11) | (((z) & 15) << 7) | ((y) & 127))
	int idx = make_idx(x,y,z);
	int idx2;
	if(blocks[idx] <= 0)
		return false;

	idx2 = make_idx(x,y+1,z);
	if(blocks[idx2] == 0)
		return true;

	idx2 = make_idx(x,y-1,z);
	if(blocks[idx2] == 0)
		return true;

	idx2 = make_idx(x+1,y,z);
	if(blocks[idx2] == 0)
		return true;

	idx2 = make_idx(x-1,y,z);
	if(blocks[idx2] == 0)
		return true;

	idx2 = make_idx(x,y,z+1);
	if(blocks[idx2] == 0)
		return true;

	idx2 = make_idx(x,y,z-1);
	if(blocks[idx2] == 0)
		return true;

	return false;
}

u_long num_blox = 0;
struct thing { vec4 pos; /*vec2 data;*/ } *blox = NULL;

static void build_buffers(void)
{
	struct chunk *c;
	int i;
	num_blox = 0;

	if(blox == NULL) {
		blox = B_malloc(999999 * sizeof(struct thing));
	}

	/* construct buffer data */

	c = chunks;
	while(c) {
		int y;
		int minY = -1;
		int maxY = -1;

		if(!c->dirty)
			return;

		for(y = 0; y < 128; y += 16) {
			if(is_visible_on_frustum((vec3) {c->x << 4, y, c->z << 4}, 64.0f)) {
				if(minY == -1) {
					minY = y;
				} else {
					maxY = y;
				}
			}
		}

		if(minY == maxY) {
			c = c->next;
			continue;
		}

		for(int x = 0; x < 16; x++) {
			for(int z = 0; z < 16; z++) {
				for(y = minY; y <= maxY; y++) {
					if(has_visible_face(x, y, z, c->data->blocks)) {
						blox[num_blox].pos[0] = (float) x + (float) (c->x << 4);// - cl.game.pos[0];
						blox[num_blox].pos[1] = (float) y ;//- cl.game.pos[1];
						blox[num_blox].pos[2] = (float) z + (float) (c->z << 4) ;//- cl.game.pos[2];
						i = make_idx(x,y,z);
						blox[num_blox].pos[3] = (float) c->data->blocks[i];
						num_blox++;
						if(num_blox >= 999990) {
							goto done;
						}
					}
				}
			}
		}

		/*for(i = 0; i < 16 * 16 * 128; i++) {
			if(c->data->blocks[i] > 0) {
				int x, y, z;

				x = (i >> 11) & 15;
				z = (i >> 7) & 15;
				y = (i & 127);
				if(y < 60)
					continue;
				blox[num_blox].pos[0] = (float) x + (float) (c->x << 4);// - cl.game.pos[0];
				blox[num_blox].pos[1] = (float) y ;//- cl.game.pos[1];
				blox[num_blox].pos[2] = (float) z + (float) (c->z << 4) ;//- cl.game.pos[2];
				//blox[num_blox].data[0] = (float) c->data->blocks[i];
				num_blox++;
				if(num_blox >= 999990) {
					goto done;
				}
			}
		}*/
		c = c->next;
	}
	done:
	return;
}

void world_render(void)
{
	static bool frustrum_updatd = false;

	if(cl.state < cl_connected)
		return;

	if(!frustrum_updatd) {
		update_view_matrix();
		frustrum_updatd = true;
	}

	build_buffers();

	//if(cl.game.rotated || cl.game.moved)
	cl.game.pos[0] -= 0.5;
	cl.game.pos[1] += 0.62f;
	cl.game.pos[2] -= 0.5;
	// update view matrix
	mat_view(view_mat, cl.game.pos, cl.game.rot);
	cl.game.pos[0] += 0.5;
	cl.game.pos[1] -= 0.62f;
	cl.game.pos[2] += 0.5;

	glUniformMatrix4fv(loc_view, 1, GL_FALSE, (const GLfloat *) view_mat);
	glUniformMatrix4fv(loc_proj, 1, GL_FALSE, (const GLfloat *) proj_mat);


	glPointSize(5.0f);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, num_blox * sizeof(struct thing), blox, GL_DYNAMIC_DRAW);

	glDrawArrays(GL_POINTS, 0, num_blox);

	glBindVertexArray(0);
}

void world_renderer_shutdown(void)
{
	glDeleteBuffers(1, &vao);
	glDeleteBuffers(1, &vbo);
}
