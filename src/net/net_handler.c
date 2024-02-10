#include "net_internal.h"
#include "game/world.h"
#include "client/console.h"
#include "client/client.h"
#include "vid/vid.h"
#include "net/packets.h"

#define UNPACK_ANGLE(angle) ((float)(angle * 360) / 256.0f)

#define EMPTY_HANDLER(pkt) void net_handle_ ## pkt (pkt p attr(unused)) {}

void net_handle_pkt_login_request(pkt_login_request pkt)
{
    cl.game.seed = pkt.seed;
    cl.game.our_id = pkt.entity_id_or_protocol_version;
    cl.state = cl_connected;

    vid_unlock_fps();
    con_printf(CON_STYLE_DARK_GREEN"connected!\n");

    net_free_string16(pkt.username);
}

void net_handle_pkt_handshake(pkt_handshake pkt)
{
    // if pkt.conn_hash[0] == '+', auth to mojang or something

	extern cvar cvar_name;
    string16 username = net_make_string16(cvar_name.string);

	cl.state = cl_connecting;
    net_write_pkt_login_request((pkt_login_request) {
        .entity_id_or_protocol_version = PROTOCOL_VERSION,
        .username = username,
        .dimension = 0,
        .seed = 0
    });

	con_printf(CON_STYLE_LIGHT_GRAY "awaiting login approval...\n");

    net_free_string16(pkt.connection_hash_or_username);
    net_free_string16(username);
}

void net_handle_pkt_chat_message(pkt_chat_message pkt)
{
    char *msg = mem_alloc(pkt.message.length + 1);

    for(int i = 0; i < pkt.message.length; i++) {
        msg[i] = (char) pkt.message.data[i];
    }

	con_printf("%s\n", msg);

    mem_free(msg);
    net_free_string16(pkt.message);
}

void net_handle_pkt_time_update(pkt_time_update pkt)
{
	world_set_time(pkt.time);
}

EMPTY_HANDLER(pkt_player_inventory)

void net_handle_pkt_spawn_position(pkt_spawn_position pkt)
{
	cl.game.pos = vec3_from(pkt.x, pkt.y, pkt.z);
}

EMPTY_HANDLER(pkt_use_entity)

void net_handle_pkt_update_health(pkt_update_health pkt)
{
	if(pkt.new_health <= 0) {
		con_printf("u died idiot\n");
	}
}

EMPTY_HANDLER(pkt_respawn)
EMPTY_HANDLER(pkt_flying)
EMPTY_HANDLER(pkt_player_move)
EMPTY_HANDLER(pkt_player_look)

void net_handle_pkt_player_look_move(pkt_player_look_move pkt)
{
	cl.game.pos = vec3_from(pkt.x, pkt.y_or_stance, pkt.z);
	cl.game.stance = pkt.stance_or_y;
	cl.game.rot.yaw = pkt.yaw;
	cl.game.rot.pitch = pkt.pitch;

	// send back or the server gets mad
    pkt.y_or_stance = cl.game.stance;
    pkt.stance_or_y = cl.game.pos.y;
    net_write_pkt_player_look_move(pkt);
}

EMPTY_HANDLER(pkt_block_dig)
EMPTY_HANDLER(pkt_place)
EMPTY_HANDLER(pkt_holding_change)
EMPTY_HANDLER(pkt_multiplayer_sleep)
EMPTY_HANDLER(pkt_animation)
EMPTY_HANDLER(pkt_entity_action)

void net_handle_pkt_named_entity_spawn(pkt_named_entity_spawn pkt)
{
	if(pkt.entity_id == cl.game.our_id) {
		cl.game.pos = vec3_div(vec3_from(pkt.x, pkt.y, pkt.z), 32.0f);
		cl.game.rot.yaw = UNPACK_ANGLE(pkt.yaw);
		cl.game.rot.pitch = UNPACK_ANGLE(pkt.pitch);
	}

    net_free_string16(pkt.name);
}

EMPTY_HANDLER(pkt_item_spawn)
EMPTY_HANDLER(pkt_collect_item)
EMPTY_HANDLER(pkt_vehicle_spawn)
EMPTY_HANDLER(pkt_mob_spawn)

void net_handle_pkt_entity_painting(pkt_entity_painting pkt)
{
    net_free_string16(pkt.title);
}

EMPTY_HANDLER(pkt_stance_update)
EMPTY_HANDLER(pkt_entity_velocity)
EMPTY_HANDLER(pkt_destroy_entity)
EMPTY_HANDLER(pkt_entity)

void net_handle_pkt_entity_move(pkt_entity_move pkt)
{
	if(pkt.entity_id == cl.game.our_id) {
		cl.game.pos = vec3_add(cl.game.pos, vec3_div(vec3_from(pkt.rel_x, pkt.rel_y, pkt.rel_z), 32.0f));
	}
}

void net_handle_pkt_entity_look(pkt_entity_look pkt)
{
	if(pkt.entity_id == cl.game.our_id) {
		cl.game.rot.yaw = UNPACK_ANGLE(pkt.yaw);
		cl.game.rot.pitch = UNPACK_ANGLE(pkt.pitch);
	}
}

void net_handle_pkt_entity_look_move(pkt_entity_look_move pkt)
{
    net_handle_pkt_entity_look((pkt_entity_look) {
        .entity_id = pkt.entity_id,
        .pitch = pkt.pitch,
        .yaw = pkt.yaw
    });
    net_handle_pkt_entity_move((pkt_entity_move) {
       .entity_id = pkt.entity_id,
       .rel_x = pkt.rel_x,
       .rel_y = pkt.rel_y,
       .rel_z = pkt.rel_z
    });
}

void net_handle_pkt_entity_teleport(pkt_entity_teleport pkt)
{
    net_handle_pkt_entity_look((pkt_entity_look) {
        .entity_id = pkt.entity_id,
        .pitch = pkt.pitch,
        .yaw = pkt.yaw
    });

	if(pkt.entity_id == cl.game.our_id) {
		cl.game.pos = vec3_div(vec3_from(pkt.x, pkt.y, pkt.z), 32.0f);
	}
}

EMPTY_HANDLER(pkt_entity_status)
EMPTY_HANDLER(pkt_attach_entity)
EMPTY_HANDLER(pkt_entity_metadata)

void net_handle_pkt_pre_chunk(pkt_pre_chunk pkt)
{
	if(pkt.load) {
		world_alloc_chunk(pkt.x, pkt.z);
	} else {
		world_free_chunk(pkt.x, pkt.z);
	}
}

void net_handle_pkt_map_chunk(pkt_map_chunk pkt)
{
	world_load_compressed_chunk_data(pkt.x, pkt.y, pkt.z, (int)pkt.size_x + 1, (int)pkt.size_y + 1, (int)pkt.size_z + 1, (size_t) pkt.data_size, pkt.data);
}

void net_handle_pkt_multi_block_change(pkt_multi_block_change pkt)
{
	if(!world_chunk_exists(pkt.chunk_x, pkt.chunk_z))
		return;

	for(int i = 0; i < pkt.size; i++) {
		int x, y, z;
		int c = pkt.coords[i];
		block_id id = pkt.block_ids[i];
		ubyte metadata = pkt.metadatas[i];

		/* unpack */
		x = ((c >> 12) & 15) + pkt.chunk_x * WORLD_CHUNK_SIZE;
		z = ((c >> 8) & 15) + pkt.chunk_z * WORLD_CHUNK_SIZE;
		y = c & 255;

		world_set_block_id(x, y, z, id);
		world_set_block_metadata(x, y, z, metadata);
	}
}

void net_handle_pkt_block_change(pkt_block_change pkt)
{
	world_set_block_id(pkt.x, pkt.y, pkt.z, pkt.block_id);
	world_set_block_metadata(pkt.x, pkt.y, pkt.z, pkt.metadata);
}

// noteblock: data1 = instrument  data2 = pitch
// piston:    data1 = state       data2 = direction
EMPTY_HANDLER(pkt_block_action)
EMPTY_HANDLER(pkt_explosion)
EMPTY_HANDLER(pkt_sound_effect)
EMPTY_HANDLER(pkt_rain_or_bed_message)
EMPTY_HANDLER(pkt_weather_event)

void net_handle_pkt_open_window(pkt_open_window pkt)
{
    net_free_string8(pkt.window_title);
}

EMPTY_HANDLER(pkt_close_window)
EMPTY_HANDLER(pkt_window_click)
EMPTY_HANDLER(pkt_set_slot)
EMPTY_HANDLER(pkt_window_items)
EMPTY_HANDLER(pkt_update_progress_bar)
EMPTY_HANDLER(pkt_transaction)

void net_handle_pkt_update_sign(pkt_update_sign pkt)
{
    net_free_string16(pkt.line1);
    net_free_string16(pkt.line2);
    net_free_string16(pkt.line3);
    net_free_string16(pkt.line4);
}

EMPTY_HANDLER(pkt_item_data)
EMPTY_HANDLER(pkt_increment_statistic)

void net_handle_pkt_disconnect(pkt_disconnect pkt)
{
    char *msg = mem_alloc(pkt.reason.length + 1);

    for(int i = 0; i < pkt.reason.length; i++) {
        msg[i] = (char) pkt.reason.data[i];
    }

	con_printf("you got kicked: %s\n", msg);
	cmd_exec("disconnect");
	cl.state = cl_disconnected;

    mem_free(msg);
    net_free_string16(pkt.reason);
}
