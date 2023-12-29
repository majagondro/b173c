#include "net_internal.h"
#include "game/world.h"
#include "vid/console.h"
#include "client/client.h"

void skip_metadata(void);

// macro trickery so the packet IDS get properly expanded
// so we get
//    void net_handle_0x00(void)
// instead of
//    void net_handle_PKT_KEEP_ALIVE(void)
#define HANDLER2(id, ...) void net_handle_ ## id (__VA_ARGS__)
#define HANDLER(id, ...) HANDLER2(id, __VA_ARGS__)

#define NW2(id, ...) net_write_ ## id (__VA_ARGS__)
#define NET_WRITE(id, ...) NW2(id, __VA_ARGS__)

HANDLER(PKT_KEEP_ALIVE, void)
{
	net_write_0x00(); // ping pong :-)
}

HANDLER(PKT_LOGIN_REQUEST, int ent_id, string16 unused, long seed, byte dimension)
{
	cl.state = cl_connected;
	con_printf("connected!\n");
	con_hide();
	cl.game.our_id = ent_id;
	B_free(unused);
}

HANDLER(PKT_HANDSHAKE, string16 conn_hash)
{
	cl.state = cl_connecting;
	// if conn_hash[0] == '+', auth to mojang or something
	// nah
	NET_WRITE(PKT_LOGIN_REQUEST, PROTOCOL_VERSION, c16("player"), 0, 0);
	con_printf("awaiting login approval...\n");
	B_free(conn_hash);
}

HANDLER(PKT_CHAT_MESSAGE, string16 message)
{
	con_printf("%s\n", c8(message));
	B_free(message);
}

HANDLER(PKT_TIME_UPDATE, long time)
{
	world_set_time(time);
}

HANDLER(PKT_ENTITY_EQUIPMENT, int ent_id, short slot, short item_id, short metadata)
{

}

HANDLER(PKT_SPAWN_POSITION, int x, int y, int z)
{
	cl.game.pos[0] = (float) x;
	cl.game.pos[1] = (float) y;
	cl.game.pos[2] = (float) z;
}

HANDLER(PKT_USE_ENTITY, int user_id, int target_id, bool attack)
{

}

HANDLER(PKT_UPDATE_HEALTH, short health)
{

}

HANDLER(PKT_RESPAWN, byte dimension)
{

}

// 0x0A, 0x0B, 0x0C are not sent by the server
HANDLER(PKT_PLAYER_MOVE_AND_LOOK, double x, double stance, double y, double z, float yaw, float pitch, bool on_ground)
{
	cl.game.pos[0] = x;
	cl.game.pos[1] = y;
	cl.game.pos[2] = z;
	cl.game.stance = stance;
	cl.game.rot[1] = yaw;
	cl.game.rot[0] = pitch;

	// send back
	net_write_0x0D(cl.game.pos[0], cl.game.pos[1], cl.game.stance, cl.game.pos[2], cl.game.rot[1], cl.game.rot[0], false);
}

// 0x0E, 0x0F, 0x10, are not sent by the server
HANDLER(PKT_USE_BED, int ent_id, bool unused, int bed_x, byte bed_y, int bed_z)
{

}

HANDLER(PKT_ANIMATION, int ent_id, byte animation_type)
{

}

HANDLER(PKT_ENTITY_ACTION, int ent_id, byte action)
{

}

HANDLER(PKT_NAMED_ENTITY_SPAWN, int ent_id, string16 name, int x, int y, int z, byte yaw, byte pitch, short held_item)
{
	if(ent_id == cl.game.our_id) {
		cl.game.pos[0] = (float)(x / 32);
		cl.game.pos[1] = (float)(y / 32);
		cl.game.pos[2] = (float)(z / 32);
		cl.game.rot[1] = (float)(yaw * 360) / 256.0f;
		cl.game.rot[0] = (float)(pitch * 360) / 256.0f;
	}

	B_free(name);
}

HANDLER(PKT_PICKUP_SPAWN, int ent_id, short item_id, byte count, short metadata, int x, int y, int z, byte yaw, byte pitch, byte roll)
{

}

HANDLER(PKT_COLLECT_ITEM, int pickup_id, int collector_id)
{

}

HANDLER(PKT_ADD_OBJECT_OR_VEHICLE, int ent_id, byte type, int x, int y, int z, int owner_id, short vel_x, short vel_y, short vel_z)
{

}

HANDLER(PKT_MOB_SPAWN, int ent_id, byte type, int x, int y, int z, byte yaw, byte pitch, ...)
{
	skip_metadata();
}

HANDLER(PKT_ENTITY_PAINTING, int ent_id, string16 title, int x, int y, int z, int direction)
{
	B_free(title);
}

HANDLER(PKT_ENTITY_VELOCITY, int ent_id, short vel_x, short vel_y, short vel_z)
{

}

HANDLER(PKT_DESTROY_ENTITY, int ent_id)
{

}

HANDLER(PKT_ENTITY, int ent_id)
{

}

HANDLER(PKT_ENTITY_RELATIVE_MOVE, int ent_id, byte rel_x, byte rel_y, byte rel_z)
{
	if(ent_id == cl.game.our_id) {
		cl.game.pos[0] += (float)(rel_x) / 32.0f;
		cl.game.pos[1] += (float)(rel_y) / 32.0f;
		cl.game.pos[2] += (float)(rel_z) / 32.0f;
	}
}

HANDLER(PKT_ENTITY_LOOK, int ent_id, byte yaw, byte pitch)
{
	if(ent_id == cl.game.our_id) {
		cl.game.rot[1] = (float)(yaw * 360) / 256.0f;
		cl.game.rot[0] = (float)(pitch * 360) / 256.0f;
	}
}

HANDLER(PKT_ENTITY_LOOK_AND_RELATIVE_MOVE, int ent_id, byte rel_x, byte rel_y, byte rel_z, byte yaw, byte pitch)
{
	if(ent_id == cl.game.our_id) {
		cl.game.pos[0] += (float)(rel_x) / 32.0f;
		cl.game.pos[1] += (float)(rel_y) / 32.0f;
		cl.game.pos[2] += (float)(rel_z) / 32.0f;
		cl.game.rot[1] = (float)(yaw * 360) / 256.0f;
		cl.game.rot[0] = (float)(pitch * 360) / 256.0f;
	}
}

HANDLER(PKT_ENTITY_TELEPORT, int ent_id, int x, int y, int z, byte yaw, byte pitch)
{
	if(ent_id == cl.game.our_id) {
		cl.game.pos[0] = (float)(x) / 32.0f;
		cl.game.pos[1] = (float)(y) / 32.0f;
		cl.game.pos[2] = (float)(z) / 32.0f;
		cl.game.rot[1] = (float)(yaw * 360) / 256.0f;
		cl.game.rot[0] = (float)(pitch * 360) / 256.0f;
	}
}

HANDLER(PKT_ENTITY_STATUS, int ent_id, byte status)
{

}

HANDLER(PKT_ATTACH_ENTITY, int ent_id, int vehicle_id)
{

}

HANDLER(PKT_ENTITY_METADATA, int ent_id, ...)
{
	skip_metadata();
}

HANDLER(PKT_PRE_CHUNK, int chunk_x, int chunk_z, bool load)
{
	if(load) {
		world_prepare_chunk(chunk_x, chunk_z);
	} else {
		world_delete_chunk(chunk_x, chunk_z);
	}
}

HANDLER(PKT_MAP_CHUNK, int x, short y, int z, byte size_x, byte size_y, byte size_z, int data_size, byte *data)
{
	static int num_paks = 0;
	int sx, sy, sz;
	sx = size_x + 1;
	sy = size_y + 1;
	sz = size_z + 1;
	world_load_region_data(x, y, z, sx, sy, sz, data_size, (u_byte *) data);
	if(sx == 16 && sz == 16 && sy == 128) {
		num_paks++;
	}
}

HANDLER(PKT_MULTI_BLOCK_CHANGE, int chunk_x, int chunk_z, short array_size, short *coord_array, byte *id_array, byte *metadata_array)
{
	struct chunk *chunk;
	int i;
	chunk = world_get_chunk(chunk_x, chunk_z);
	if(!chunk)
		return;
	for(i = 0; i < array_size; i++) {
		byte id = id_array[i];
		int x, y, z, c = coord_array[i];
		x = (c >> 12) & 4;
		z = (c >> 8) & 4;
		y = c & 255;
		if(y > 127)
			continue;

		chunk->data->blocks[IDX_FROM_COORDS(x,y,z)] = id;
	}
}

HANDLER(PKT_BLOCK_CHANGE, int x, byte y, int z, byte block_id, byte metadata)
{
	world_set_block(x, y, z, block_id);
}

HANDLER(PKT_BLOCK_ACTION, int x, short y, int z, byte data1, byte data2)
{

}
// noteblock: data1 = instrument data2 = pitch      piston: data1 = state     data2 = direction
HANDLER(PKT_EXPLOSION, double x, double y, double z, float radius, int num_affected_blocks, struct ni_off_coord *affected_blocks)
{

}

HANDLER(PKT_SOUND_EFFECT, int effect_id, int x, byte y, int z, int extra_data)
{

}

HANDLER(PKT_NEW_OR_INVALID_STATE, byte something)
{

}

HANDLER(PKT_THUNDERBOLT, int ent_id, bool unused, int x, int y, int z)
{

}

HANDLER(PKT_OPEN_WINDOW, byte gui_id, byte gui_type, string8 gui_title, byte num_slots)
{
	B_free(gui_title);
}

HANDLER(PKT_CLOSE_WINDOW, byte gui_id)
{

}

HANDLER(PKT_WINDOW_CLICK, byte gui_id, short slot, byte mouse_buton, short action_num, bool shift_held, short item_id, byte item_count, short item_metadata)
{

}

HANDLER(PKT_SET_SLOT, byte gui_id, short slot, short item_id, byte item_count, short item_metadata)
{

}

HANDLER(PKT_WINDOW_ITEMS, byte gui_id, short count, struct ni_wi_payload *payload)
{

}

HANDLER(PKT_UPDATE_PROGRESS_BAR, byte gui_id, short type, short progress)
{

}

HANDLER(PKT_TRANSACTION, byte gui_id, short action, bool accepted)
{

}

HANDLER(PKT_UPDATE_SIGN, int x, short y, int z, string16 line1, string16 line2, string16 line3, string16 line4)
{
	B_free(line1);
	B_free(line2);
	B_free(line3);
	B_free(line4);
}

HANDLER(PKT_ITEM_DATA, short item_type, short item_id, u_byte data_size, byte *data)
{

}

HANDLER(PKT_INCREMENT_STATISTIC, int stat_id, byte amount)
{

}

HANDLER(PKT_DISCONNECT, string16 reason)
{
	printf("'");
	c16puts(reason);
	printf("'\n");
	con_printf("you got kicked: %s\n", c8(reason));
	cmd_exec("disconnect", false);
	cl.state = cl_disconnected;
	B_free(reason);
}


#undef HANDLER2
#undef HANDLER


