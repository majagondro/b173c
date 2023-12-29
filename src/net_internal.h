#ifndef B173C_NET_INTERNAL_H
#define B173C_NET_INTERNAL_H

#include "common.h"

#define PROTOCOL_VERSION 14

/*********************************************************************
                GENERAL DATA READING/WRITING FUNCTIONS
*********************************************************************/

void net_read_buf(void *dest, size_t n);
byte net_read_byte(void);
short net_read_short(void);
int net_read_int(void);
long net_read_long(void);
float net_read_float(void);
double net_read_double(void);
bool net_read_bool(void);
string8 net_read_string8(void);
string16 net_read_string16(void);

void net_write_buf(const void *buf, size_t n);
void net_write_byte(u_byte v);
void net_write_short(short v);
void net_write_int(int v);
void net_write_long(long v);
void net_write_float(float v);
void net_write_double(double v);
void net_write_bool(bool v);
void net_write_string8(string8 v);
void net_write_string16(string16 v);

/*********************************************************************
                              PACKET IDS
*********************************************************************/

#define PKT_KEEP_ALIVE 0x00
#define PKT_LOGIN_REQUEST 0x01
#define PKT_HANDSHAKE 0x02
#define PKT_CHAT_MESSAGE 0x03
#define PKT_TIME_UPDATE 0x04
#define PKT_ENTITY_EQUIPMENT 0x05
#define PKT_SPAWN_POSITION 0x06
#define PKT_USE_ENTITY 0x07
#define PKT_UPDATE_HEALTH 0x08
#define PKT_RESPAWN 0x09
#define PKT_PLAYER 0x0A
#define PKT_PLAYER_MOVE 0x0B
#define PKT_PLAYER_LOOK 0x0C
#define PKT_PLAYER_MOVE_AND_LOOK 0x0D
#define PKT_PLAYER_MINE 0x0E
#define PKT_PLAYER_PLACE 0x0F
#define PKT_HOLDING_CHANGE 0x10
#define PKT_USE_BED 0x11
#define PKT_ANIMATION 0x12
#define PKT_ENTITY_ACTION 0x13
#define PKT_NAMED_ENTITY_SPAWN 0x14
#define PKT_PICKUP_SPAWN 0x15
#define PKT_COLLECT_ITEM 0x16
#define PKT_ADD_OBJECT_OR_VEHICLE 0x17
#define PKT_MOB_SPAWN 0x18
#define PKT_ENTITY_PAINTING 0x19
#define PKT_STANCE_UPDATE 0x1B
#define PKT_ENTITY_VELOCITY 0x1C
#define PKT_DESTROY_ENTITY 0x1D
#define PKT_ENTITY 0x1E
#define PKT_ENTITY_RELATIVE_MOVE 0x1F
#define PKT_ENTITY_LOOK 0x20
#define PKT_ENTITY_LOOK_AND_RELATIVE_MOVE 0x21
#define PKT_ENTITY_TELEPORT 0x22
#define PKT_ENTITY_STATUS 0x26
#define PKT_ATTACH_ENTITY 0x27
#define PKT_ENTITY_METADATA 0x28
#define PKT_PRE_CHUNK 0x32
#define PKT_MAP_CHUNK 0x33
#define PKT_MULTI_BLOCK_CHANGE 0x34
#define PKT_BLOCK_CHANGE 0x35
#define PKT_BLOCK_ACTION 0x36
#define PKT_EXPLOSION 0x3C
#define PKT_SOUND_EFFECT 0x3D
#define PKT_NEW_OR_INVALID_STATE 0x46
#define PKT_THUNDERBOLT 0x47
#define PKT_OPEN_WINDOW 0x64
#define PKT_CLOSE_WINDOW 0x65
#define PKT_WINDOW_CLICK 0x66
#define PKT_SET_SLOT 0x67
#define PKT_WINDOW_ITEMS 0x68
#define PKT_UPDATE_PROGRESS_BAR 0x69
#define PKT_TRANSACTION 0x6A
#define PKT_UPDATE_SIGN 0x82
#define PKT_ITEM_DATA 0x83
#define PKT_INCREMENT_STATISTIC 0xC8
#define PKT_DISCONNECT 0xFF

/*********************************************************************
                       INCOMING PACKET HANDLERS
*********************************************************************/


struct ni_off_coord { byte off_x, off_y, off_z; };
struct ni_wi_payload { short item_id, metadata; byte count; };

// macro trickery so the packet IDS get properly expanded
// so we get
//    void net_handle_0x00(void)
// instead of
//    void net_handle_PKT_KEEP_ALIVE(void)
#define HANDLER2(id, ...) void net_handle_ ## id (__VA_ARGS__)
#define HANDLER(id, ...) HANDLER2(id, __VA_ARGS__)

HANDLER(PKT_KEEP_ALIVE, void);
HANDLER(PKT_LOGIN_REQUEST, int ent_id, string16 unused, long seed, byte dimension);
HANDLER(PKT_HANDSHAKE, string16 conn_hash);
HANDLER(PKT_CHAT_MESSAGE, string16 message);
HANDLER(PKT_TIME_UPDATE, long time);
HANDLER(PKT_ENTITY_EQUIPMENT, int ent_id, short slot, short item_id, short metadata);
HANDLER(PKT_SPAWN_POSITION, int x, int y, int z);
HANDLER(PKT_USE_ENTITY, int user_id, int target_id, bool attack);
HANDLER(PKT_UPDATE_HEALTH, short health);
HANDLER(PKT_RESPAWN, byte dimension);
// 0x0A, 0x0B, 0x0C are not sent by the server
HANDLER(PKT_PLAYER_MOVE_AND_LOOK, double x, double stance, double y, double z, float yaw, float pitch, bool on_ground);
// 0x0E, 0x0F, 0x10, are not sent by the server
HANDLER(PKT_USE_BED, int ent_id, bool unused, int bed_x, byte bed_y, int bed_z);
HANDLER(PKT_ANIMATION, int ent_id, byte animation_type);
HANDLER(PKT_ENTITY_ACTION, int ent_id, byte action);
HANDLER(PKT_NAMED_ENTITY_SPAWN, int ent_id, string16 name, int x, int y, int z, byte yaw, byte pitch, short held_item);
HANDLER(PKT_PICKUP_SPAWN, int ent_id, short item_id, byte count, short metadata, int x, int y, int z, byte yaw, byte pitch, byte roll);
HANDLER(PKT_COLLECT_ITEM, int pickup_id, int collector_id);
HANDLER(PKT_ADD_OBJECT_OR_VEHICLE, int ent_id, byte type, int x, int y, int z, int owner_id, short vel_x, short vel_y, short vel_z);
HANDLER(PKT_MOB_SPAWN, int ent_id, byte type, int x, int y, int z, byte yaw, byte pitch, ...);
HANDLER(PKT_ENTITY_PAINTING, int ent_id, string16 title, int x, int y, int z, int direction);
// 0x1B is not handled or sent by the vanilla client, but is handled by the server, todo: investigate possible uses?
HANDLER(PKT_ENTITY_VELOCITY, int ent_id, short vel_x, short vel_y, short vel_z);
HANDLER(PKT_DESTROY_ENTITY, int ent_id);
HANDLER(PKT_ENTITY, int ent_id);
HANDLER(PKT_ENTITY_RELATIVE_MOVE, int ent_id, byte rel_x, byte rel_y, byte rel_z);
HANDLER(PKT_ENTITY_LOOK, int ent_id, byte yaw, byte pitch);
HANDLER(PKT_ENTITY_LOOK_AND_RELATIVE_MOVE, int ent_id, byte rel_x, byte rel_y, byte rel_z, byte yaw, byte pitch);
HANDLER(PKT_ENTITY_TELEPORT, int ent_id, int x, int y, int z, byte yaw, byte pitch);
HANDLER(PKT_ENTITY_STATUS, int ent_id, byte status);
HANDLER(PKT_ATTACH_ENTITY, int ent_id, int vehicle_id);
HANDLER(PKT_ENTITY_METADATA, int ent_id, ...);
HANDLER(PKT_PRE_CHUNK, int chunk_x, int chunk_z, bool load);
HANDLER(PKT_MAP_CHUNK, int x, short y, int z, byte size_x, byte size_y, byte size_z, int data_size, byte *data);
HANDLER(PKT_MULTI_BLOCK_CHANGE, int chunk_x, int chunk_z, short array_size, short *coord_array, byte *id_array, byte *metadata_array);
HANDLER(PKT_BLOCK_CHANGE, int x, byte y, int z, byte block_id, byte metadata);
HANDLER(PKT_BLOCK_ACTION, int x, short y, int z, byte data1, byte data2); // noteblock: data1 = instrument data2 = pitch      piston: data1 = state     data2 = direction
HANDLER(PKT_EXPLOSION, double x, double y, double z, float radius, int num_affected_blocks, struct ni_off_coord *affected_blocks);
HANDLER(PKT_SOUND_EFFECT, int effect_id, int x, byte y, int z, int extra_data);
HANDLER(PKT_NEW_OR_INVALID_STATE, byte something);
HANDLER(PKT_THUNDERBOLT, int ent_id, bool unused, int x, int y, int z);
HANDLER(PKT_OPEN_WINDOW, byte gui_id, byte gui_type, string8 gui_title, byte num_slots);
HANDLER(PKT_CLOSE_WINDOW, byte gui_id);
HANDLER(PKT_WINDOW_CLICK, byte gui_id, short slot, byte mouse_buton, short action_num, bool shift_held, short item_id, byte item_count, short item_metadata);
HANDLER(PKT_SET_SLOT, byte gui_id, short slot, short item_id, byte item_count, short item_metadata);
HANDLER(PKT_WINDOW_ITEMS, byte gui_id, short count, struct ni_wi_payload *payload);
HANDLER(PKT_UPDATE_PROGRESS_BAR, byte gui_id, short type, short progress);
HANDLER(PKT_TRANSACTION, byte gui_id, short action, bool accepted);
HANDLER(PKT_UPDATE_SIGN, int x, short y, int z, string16 line1, string16 line2, string16 line3, string16 line4);
HANDLER(PKT_ITEM_DATA, short item_type, short item_id, u_byte data_size, byte *data);
HANDLER(PKT_INCREMENT_STATISTIC, int stat_id, byte amount);
HANDLER(PKT_DISCONNECT, string16 reason);

#undef HANDLER2
#undef HANDLER

/*********************************************************************
                            PACKET WRITERS
*********************************************************************/

// macro trickery so the packet IDS get properly expanded
// so we get
//    void net_write_0x00(void)
// instead of
//    void net_write_PKT_KEEP_ALIVE(void)
#define WRITER2(id, ...) void net_write_ ## id (__VA_ARGS__)
#define WRITER(id, ...) WRITER2(id, __VA_ARGS__)

WRITER(PKT_KEEP_ALIVE, void);
WRITER(PKT_LOGIN_REQUEST, int protocol_version, string16 username, long unused1, byte unused2);
WRITER(PKT_HANDSHAKE, string16 username);
WRITER(PKT_CHAT_MESSAGE, string16 message);
WRITER(PKT_USE_ENTITY, int user_id, int target_id, bool left_click);
WRITER(PKT_RESPAWN, byte dimension);
WRITER(PKT_PLAYER, bool on_ground);
WRITER(PKT_PLAYER_MOVE, double x, double y, double stance, double z, bool on_ground);
WRITER(PKT_PLAYER_LOOK, float yaw, float pitch, bool on_ground);
WRITER(PKT_PLAYER_MOVE_AND_LOOK, double x, double y, double stance, double z, float yaw, float pitch, bool on_ground);
WRITER(PKT_PLAYER_MINE, byte action, int x, byte y, int z, byte face);
WRITER(PKT_PLAYER_PLACE, int x, byte y, int z, byte face, short block_or_item_id, byte item_count, short item_metadata);
WRITER(PKT_ANIMATION, int ent_id, byte animation);
WRITER(PKT_HOLDING_CHANGE, short slot);
WRITER(PKT_ENTITY_ACTION, int ent_id, byte action);
WRITER(PKT_CLOSE_WINDOW, int gui_id);
WRITER(PKT_WINDOW_CLICK, int gui_id, short slot, byte mouse_button, short action, bool shift, short item_id, byte item_count, short item_metadata);
WRITER(PKT_TRANSACTION, int gui_id, short action, bool accepted);
WRITER(PKT_UPDATE_SIGN, int x, short y, int z, string16 line1, string16 line2, string16 line3, string16 line4);
WRITER(PKT_DISCONNECT, string16 reason);

#undef WRITER2
#undef WRITER

#endif
