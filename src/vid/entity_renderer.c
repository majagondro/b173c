#include "vid.h"
#include "mathlib.h"
#include "common.h"
#include "client/client.h"
#include "glad/glad.h"
#include "game/entity.h"
#include "hashmap.c/hashmap.h"
#include "ui.h"
#include "client/cvar.h"

uint gl_vao, gl_vbo;
GLint gl_uniform_model, gl_uniform_view, gl_uniform_projection;
extern struct gl_state gl;
extern mat4_t view_mat, proj_mat;

errcode entity_renderer_init(void)
{
    vec3_t entity_model_verts[] = {
            vec3(0, 0, 0),
            vec3(1, 0, 0),
            vec3(0, 1, 0)
    };

    glGenVertexArrays(1, &gl_vao);
    glGenBuffers(1, &gl_vbo);

    glBindVertexArray(gl_vao);
        glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(entity_model_verts), entity_model_verts, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3_t), 0);
        glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    gl_uniform_model = glGetUniformLocation(gl.shader_model, "MODEL");
    gl_uniform_view = glGetUniformLocation(gl.shader_model, "VIEW");
    gl_uniform_projection = glGetUniformLocation(gl.shader_model, "PROJECTION");

    if(glGetError() != GL_NO_ERROR)
        return ERR_FATAL;
    return ERR_OK;
}

void entity_renderer_shutdown(void)
{
    glDeleteVertexArrays(1, &gl_vao);
    glDeleteBuffers(1, &gl_vbo);
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
        mat4_t model, t, r;
        vec3_t text_pos;

        if(ent == cl.game.our_ent && cl_freecamera.integer == 0)
            continue;

        mat4_identity(t);
        mat4_identity(r);
        mat4_translation(t, ent->position);
        mat4_rotation(r, ent->rotation);
        mat4_multiply(model, r, t);

        if(developer.integer >= 2) {
            text_pos = cam_project_3d_to_2d(ent->position, proj_mat, view_mat, vec2(ui_w, ui_h));
            if(text_pos.z < 1) {
                ui_printf(text_pos.x, text_pos.y, "%d", ent->type);
            }
        }

        glUniformMatrix4fv(gl_uniform_model, 1, GL_FALSE, (const GLfloat *) model);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
    }
    glEnable(GL_CULL_FACE);

    glBindVertexArray(0);
}
