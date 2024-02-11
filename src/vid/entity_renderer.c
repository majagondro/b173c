#include "vid.h"
#include "mathlib.h"
#include "common.h"
#include "client/client.h"
#include "glad/glad.h"
#include "game/world.h"
#include "game/entity.h"
#include "client/console.h"
#include "hashmap.h"
#include "meshbuilder.h"
#include "assets.h"

vec3 entity_model_verts[] = {
    vec3_from(0,0,0),
    vec3_from(1,0,0),
    vec3_from(0,1,0)
};

uint gl_vao, gl_vbo;
uint gl_uniform_model, gl_uniform_view, gl_uniform_projection;
extern struct gl_state gl;
extern mat4 view_mat, proj_mat;

void entity_renderer_init(void)
{
    glGenVertexArrays(1, &gl_vao);
    glGenBuffers(1, &gl_vbo);

    glBindVertexArray(gl_vao);
        glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(entity_model_verts), entity_model_verts, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), 0);
        glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    gl_uniform_model = glGetUniformLocation(gl.shader_model, "MODEL");
    gl_uniform_view = glGetUniformLocation(gl.shader_model, "VIEW");
    gl_uniform_projection = glGetUniformLocation(gl.shader_model, "PROJECTION");

}

void entity_renderer_shutdown(void)
{

}

void entity_renderer_render(void)
{
    size_t i;
    void *it;

    glUniformMatrix4fv(gl_uniform_view, 1, GL_FALSE, (const GLfloat *) view_mat);
    glUniformMatrix4fv(gl_uniform_projection, 1, GL_FALSE, (const GLfloat *) proj_mat);

    glBindVertexArray(gl_vao);

    glDisable(GL_CULL_FACE);

    i = 0;
    while(hashmap_iter(world_entity_map, &i, &it)) {
        entity *ent = it;
        mat4 model, t, r;

        mat_identity(t);
        mat_identity(r);
        mat_translate(t, ent->position);
        mat_rotate(r, ent->rotation);
        mat_multiply(model, r, t);

        glUniformMatrix4fv(gl_uniform_model, 1, GL_FALSE, (const GLfloat *) model);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
    }
    glEnable(GL_CULL_FACE);

    glBindVertexArray(0);
}
