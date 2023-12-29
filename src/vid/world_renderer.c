#include "vid.h"
#include "mathlib.h"
#include "common.h"
#include "client/client.h"
#include "glad/glad.h"
#include "game/world.h"
#include "console.h"

static mat4 view_mat = {0};
static mat4 proj_mat = {0};
static u_int vao;
static u_int vbo;
static u_int loc_model, loc_proj, loc_view;

extern struct gl_state gl;

void world_renderer_init(void)
{

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *) 0);
	//glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void *) sizeof(vec3));
	glEnableVertexAttribArray(0);
	//glEnableVertexAttribArray(1);

	glBindVertexArray(0);

	/* setup projection matrix */
	mat_projection(proj_mat, 105.0f, 16.0f / 9.0f, 0.1f, 1024.0f);

	loc_model = glGetUniformLocation(gl.shader3d, "MODEL");
	loc_view = glGetUniformLocation(gl.shader3d, "VIEW");
	loc_proj = glGetUniformLocation(gl.shader3d, "PROJECTION");
}

extern struct chunk *chunks;

void world_render(void)
{
	u_long num_blox = 0;
	mat4 model_mat;
	struct chunk *c;
	int i;
	struct thing { vec3 pos; /*vec2 data;*/ } *blox;

	if(cl.state < cl_connected)
		return;

	blox = B_malloc(999999 * sizeof(struct thing));

	/* construct buffer data */

	c = world_get_chunk((int)cl.game.pos[0] >> 4, (int)cl.game.pos[2] >> 4);
	c = chunks;
	while(c) {
		for(i = 0; i < 16 * 16 * 128; i++) {
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
		}
		c = c->next;
	}
	done:



	/* update view matrix */
	cl.game.pos[1] += .6f;
	cl.game.pos[0] -= 0.5;
	cl.game.pos[2] -= 0.5;
	mat_view(view_mat, cl.game.pos, cl.game.rot);
	cl.game.pos[0] += 0.5;
	cl.game.pos[2] += 0.5;
	cl.game.pos[1] -= .6f;
	mat_identity(model_mat);

	glUniformMatrix4fv(loc_view, 1, GL_FALSE, (const GLfloat *) view_mat);
	glUniformMatrix4fv(loc_proj, 1, GL_FALSE, (const GLfloat *) proj_mat);
	glUniformMatrix4fv(loc_model, 1, GL_FALSE, (const GLfloat *) model_mat);


	glPointSize(5.0f);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, num_blox * sizeof(struct thing), blox, GL_DYNAMIC_DRAW);

	glDrawArrays(GL_POINTS, 0, num_blox);

	glBindVertexArray(0);


	B_free(blox);

}
