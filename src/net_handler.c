#include "net_internal.h"

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
	printf("HELLO WORLD!!! i am %d of world %ld in dimension %hhd!!\n", ent_id, seed, dimension);
	B_free(unused);
}

HANDLER(PKT_HANDSHAKE, string16 conn_hash)
{
	// if conn_hash[0] == '+', auth to mojang or something
	// nah
	NET_WRITE(PKT_LOGIN_REQUEST, PROTOCOL_VERSION, c16("player"), 0, 0);
	B_free(conn_hash);
}

HANDLER(PKT_CHAT_MESSAGE, string16 message)
{
	B_free(message);
}

HANDLER(PKT_TIME_UPDATE, long time)
{

}

HANDLER(PKT_ENTITY_EQUIPMENT, int ent_id, short slot, short item_id, short metadata)
{

}

HANDLER(PKT_SPAWN_POSITION, int x, int y, int z)
{

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
	// send back
	// note: client sends X Y STANCE Z
	//       server sends X STANCE Y Z
	NET_WRITE(PKT_PLAYER_MOVE_AND_LOOK, x, y, stance, z, yaw, pitch, on_ground);
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

}

HANDLER(PKT_ENTITY_LOOK, int ent_id, byte yaw, byte pitch)
{

}

HANDLER(PKT_ENTITY_LOOK_AND_RELATIVE_MOVE, int ent_id, byte rel_x, byte rel_y, byte rel_z, byte yaw, byte pitch)
{

}

HANDLER(PKT_ENTITY_TELEPORT, int ent_id, int x, int y, int z, byte yaw, byte pitch)
{

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

}

HANDLER(PKT_MAP_CHUNK, int x, short y, int z, byte size_x, byte size_y, byte size_z, int data_size, byte *data)
{

}

HANDLER(PKT_MULTI_BLOCK_CHANGE, int chunk_x, int chunk_z, short array_size, short *coord_array, byte *id_array, byte *metadata_array)
{

}

HANDLER(PKT_BLOCK_CHANGE, int x, byte y, int z, byte block_id, byte metadata)
{

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
	B_free(reason);
}


#undef HANDLER2
#undef HANDLER



