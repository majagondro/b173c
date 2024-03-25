#include "player.h"
#include "client/cvar.h"
#include "client/client.h"
#include "net/packets.h"
#include "input.h"

float break_progress = 0.0f;

static void update_block_break(entity *player)
{

}

static void on_click(entity *ent, int button)
{
    break_progress = 0.0f;

}

static void on_release(entity *ent, int button)
{
    break_progress = 0.0f;

}

void player_update(entity *ent, gamekeys_t input)
{
    if(cl_freecamera.integer == 0) {
        vec3_t fwd, side, up;

        cam_angles(&fwd, &side, &up, ent->rotation.yaw, 0.0f);

        fwd.y = 0.0f;
        side.y = 0.0f;

        ent->move_forward = 0;
        if(input.forward.pressed)
            ent->move_forward++;
        if(input.back.pressed)
            ent->move_forward--;

        ent->move_side = 0;
        if(input.right.pressed)
            ent->move_side++;
        if(input.left.pressed)
            ent->move_side--;

        ent->sneaking = input.sneak.pressed; // && !sleeping
        if(ent->sneaking) {
            ent->move_forward *= 0.3f;
            ent->move_side *= 0.3f;
            if(ent->height_offset < 0.2f) {
                ent->height_offset = 0.2f;
            }
        }

        if(input.sneak.just_pressed) {
            net_write_pkt_entity_action((pkt_entity_action) {
               .entity_id = ent->id,
               .action = 1
            });
        } else if(input.sneak.just_released) {
            net_write_pkt_entity_action((pkt_entity_action) {
                .entity_id = ent->id,
                .action = 2
            });
        }

        if(input.jump.pressed) {
            if(entity_in_water(ent) || entity_in_lava(ent)) {
                ent->velocity.y += 0.04f * cl.frametime * 20.0f;
            } else if(ent->onground) {
                ent->velocity.y = 0.42f;
            }
        }

        if(input.attack.pressed)
            update_block_break(ent);

        if(input.attack.just_pressed) {
            on_click(ent, KEY_MOUSE1);
        } else if(input.attack2.just_pressed) {
            on_click(ent, KEY_MOUSE2);
        }

        if(input.attack.just_released) {
            on_release(ent, KEY_MOUSE1);
        } else if(input.attack2.just_released) {
            on_release(ent, KEY_MOUSE2);
        }
    } else {
        vec3_t fwd, side, up;
        float m_fwd, m_side, m_up;
        float spd = 20.0f;

        cam_angles(&fwd, &side, &up, ent->rotation.yaw, 0.0f);

        fwd.y = 0.0f;
        side.y = 0.0f;

        m_fwd = 0;
        if(input.forward.pressed)
            m_fwd++;
        if(input.back.pressed)
            m_fwd--;

        m_side = 0;
        if(input.right.pressed)
            m_side++;
        if(input.left.pressed)
            m_side--;

        m_up = 0;
        if(input.jump.pressed)
            m_up++;
        if(input.sneak.pressed)
            m_up--;

        cl.game.cam_pos = vec3_add(cl.game.cam_pos, vec3_mul(fwd, m_fwd * cl.frametime * spd));
        cl.game.cam_pos = vec3_add(cl.game.cam_pos, vec3_mul(side, m_side * cl.frametime * spd));
        cl.game.cam_pos = vec3_add(cl.game.cam_pos, vec3_mul(vec3(0,1,0), m_up * cl.frametime * spd));
    }
}
