#include "net_internal.h"
#include "client/client.h"
#include "vid/console.h"

void net_write_packets(void)
{
	static bool sent_handshake = false;

	if(cl.state == cl_disconnected && sent_handshake) {
		sent_handshake = false;
		return;
	}

	if(cl.state == cl_connecting && !sent_handshake) {
		// send handshake once
		con_printf(COLOR_DGRAY "awaiting handshake...\n");
		net_write_0x02(c16("player"));
		sent_handshake = true;
	}

	if(cl.state != cl_connected)
		return;

	net_write_0x0D(cl.game.pos[0], cl.game.pos[1], cl.game.pos[1] + 0.2f, cl.game.pos[2], cl.game.rot[1]+180.0f, cl.game.rot[0], false);
	//net_write_0x0C(cl.game.rot[1], cl.game.rot[0], false);
}

// macro trickery so the packet IDS get properly expanded
// so we get
//    void net_write_0x00(void)
// instead of
//    void net_write_PKT_KEEP_ALIVE(void)
#define WRITER2(id, ...) void net_write_ ## id (__VA_ARGS__) { net_write_byte(id);
#define WRITER(id, ...) WRITER2(id, __VA_ARGS__)

WRITER(PKT_KEEP_ALIVE, void)

}

WRITER(PKT_LOGIN_REQUEST, int protocol_version, string16 username, long unused1, byte unused2)
	net_write_int(protocol_version);
	net_write_string16(username);
	net_write_long(unused1);
	net_write_byte(unused2);
}

WRITER(PKT_HANDSHAKE, string16 username)
	net_write_string16(username);
}

WRITER(PKT_CHAT_MESSAGE, string16 message)
	net_write_string16(message);
}

WRITER(PKT_USE_ENTITY, int user_id, int target_id, bool left_click)
	net_write_int(user_id);
	net_write_int(target_id);
	net_write_bool(left_click);
}

WRITER(PKT_RESPAWN, byte dimension)
	net_write_byte(dimension);
}

WRITER(PKT_PLAYER, bool on_ground)
	net_write_bool(on_ground);
}

WRITER(PKT_PLAYER_MOVE, double x, double y, double stance, double z, bool on_ground)
	net_write_double(x);
	net_write_double(y);
	net_write_double(stance);
	net_write_double(z);
	net_write_bool(on_ground);
}

WRITER(PKT_PLAYER_LOOK, float yaw, float pitch, bool on_ground)
	net_write_float(yaw);
	net_write_float(pitch);
	net_write_bool(on_ground);
}

WRITER(PKT_PLAYER_MOVE_AND_LOOK, double x, double y, double stance, double z, float yaw, float pitch, bool on_ground)
	net_write_double(x);
	net_write_double(y);
	net_write_double(stance);
	net_write_double(z);
	net_write_float(yaw);
	net_write_float(pitch);
	net_write_bool(on_ground);
}

WRITER(PKT_PLAYER_MINE, byte action, int x, byte y, int z, byte face)
	net_write_byte(action);
	net_write_int(x);
	net_write_byte(y);
	net_write_int(z);
	net_write_byte(face);
}

WRITER(PKT_PLAYER_PLACE, int x, byte y, int z, byte face, short block_or_item_id, byte item_count, short item_metadata)
	net_write_int(x);
	net_write_byte(y);
	net_write_int(z);
	net_write_byte(face);
	net_write_short(block_or_item_id);
	net_write_byte(item_count);
	net_write_short(item_metadata);
}

WRITER(PKT_ANIMATION, int ent_id, byte animation)
	net_write_int(ent_id);
	net_write_byte(animation);
}

WRITER(PKT_HOLDING_CHANGE, short slot)
	net_write_short(slot);
}

WRITER(PKT_ENTITY_ACTION, int ent_id, byte action)
	net_write_int(ent_id);
	net_write_byte(action);
}

WRITER(PKT_CLOSE_WINDOW, int gui_id)
	net_write_int(gui_id);
}

WRITER(PKT_WINDOW_CLICK, int gui_id, short slot, byte mouse_button, short action, bool shift, short item_id, byte item_count, short item_metadata)
	net_write_int(gui_id);
	net_write_short(slot);
	net_write_byte(mouse_button);
	net_write_short(action);
	net_write_bool(shift);
	net_write_short(item_id);
	net_write_byte(item_count);
	net_write_short(item_metadata);
}

WRITER(PKT_TRANSACTION, int gui_id, short action, bool accepted)
	net_write_int(gui_id);
	net_write_short(action);
	net_write_bool(accepted);
}

WRITER(PKT_UPDATE_SIGN, int x, short y, int z, string16 line1, string16 line2, string16 line3, string16 line4)
	net_write_int(x);
	net_write_short(y);
	net_write_int(z);
	net_write_string16(line1);
	net_write_string16(line2);
	net_write_string16(line3);
	net_write_string16(line4);
}

WRITER(PKT_DISCONNECT, string16 reason)
	net_write_string16(reason);
}

#undef WRITER2
#undef WRITER
