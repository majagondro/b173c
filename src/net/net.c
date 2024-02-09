#include <SDL2/SDL_endian.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "common.h"
#include "net.h"
#include "net_internal.h"
#include "client/client.h"
#include "client/console.h"
#include "vid/vid.h"
#include <setjmp.h>

static bool init_ok = false;
static bool cmds_reg = false;
static bool should_disconnect = false;

static bool read_ok = false;
static size_t total_read = 0;
static byte read_buffer[32*1024] = {0};
jmp_buf read_abort;

static int sockfd = -1;

#define perror(desc) con_printf("%s: %s\n", desc, strerror(errno))

static void setblocking(int fd, bool blocking)
{
	int flags;

	/* get old flags */
	if((flags = fcntl(fd, F_GETFL)) < 0) {
		perror("fcntl1");
		return;
	}

	/* add or remove nonblocking flag */
	flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);

	/* apply new flags */
	if(fcntl(fd, F_SETFL, flags) < 0) {
		perror("fcntl2");
		return;
	}
}

void disconnect_f(void);
void connect_f(void);
void say_f(void);
void respawn_f(void);

void net_init(void)
{
	/* do not reinitialize */
	if(init_ok)
		return;

	/* open ipv4 tcp stream socket */
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		net_shutdown(); // deinit
		return;
	}

	/* set socket to nonblocking */
	setblocking(sockfd, false);

	/* register net-related commands */
	if(!cmds_reg) {
		cmd_register("connect", connect_f);
		cmd_register("disconnect", disconnect_f);
		cmd_register("say", say_f);
		cmd_register("respawn", respawn_f);
		cmds_reg = true;
	}

	init_ok = true;
}

void net_connect(struct sockaddr_in *sockaddr, int port)
{
	struct sockaddr_in addr = {0};
	if(sockaddr != NULL) {
		addr = *sockaddr;
		addr.sin_port = htons(port);
	} else {
		// disconnect
		addr.sin_family = AF_UNSPEC;
	}

	if(!init_ok)
		net_init();

	cl.state = cl_disconnected;

	// connect to server
	// set socket to blocking for this because its easier this way
	setblocking(sockfd, true);
	if(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("connect");
		net_shutdown(); // deinit
		return;
	}
	setblocking(sockfd, false);

	if(sockaddr != NULL) {
		cl.state = cl_connecting;
	}
}

static bool net_read_packet(void);

void net_process(void)
{
	// do not do anything if not properly initialized
	if(!init_ok)
		return;

	// read and handle incoming packets
	if(cl.state > cl_disconnected) {
		if(setjmp(read_abort)) {
			// longjmp to here means that recv failed, which usually
			// means we did not receive enough data to read an entire packet

			net_write_packets();
			return;
		}

		while(net_read_packet())
			;

		net_write_packets();
	}

	// disconnect if requested
	if(should_disconnect) {
		net_connect(NULL, 0);
		cl.state = cl_disconnected;
		vid_lock_fps(); // no need to exhaust the cpu by drawing the menu in 1 billion fps
		net_write_packets(); // reset handshake flag
		should_disconnect = false;
	}

}

static bool net_read_packet(void)
{
	ubyte packet_id;
	int i;

	// consume the old bytes
	if(read_ok)
		read(sockfd, read_buffer, total_read);
	total_read = 0;

	// try to read a packet
	packet_id = net_read_byte();
	if(!read_ok) {
		return false;
	}

	// read the packet data and call the handler
	// alloca is sometimes used for byte buffers, that is because net_read_buf can longjmp
	// and if malloc were to be used, that would be leaked memory!!
	// i hope that the stack size is big enough lol!!!!
	switch(packet_id) {
		case PKT_KEEP_ALIVE:
			net_handle_0x00();
			break;
		case PKT_LOGIN_REQUEST: {
			int eid = net_read_int();
			string16 unk = net_read_string16();
			long seed = net_read_long();
			byte dim = net_read_byte();
			net_handle_0x01(eid, unk, seed, dim);
			break;
		}
		case PKT_HANDSHAKE:
			net_handle_0x02(net_read_string16());
			break;
		case PKT_CHAT_MESSAGE:
			net_handle_0x03(net_read_string16());
			break;
		case PKT_TIME_UPDATE:
			net_handle_0x04(net_read_long());
			break;
		case PKT_ENTITY_EQUIPMENT: {
			int eid = net_read_int();
			short slot = net_read_short();
			short item_id = net_read_short();
			short item_metadata = net_read_short();
			net_handle_0x05(eid, slot, item_id, item_metadata);
			break;
		}
		case PKT_SPAWN_POSITION: {
			int x = net_read_int(),
				y = net_read_int(),
				z = net_read_int();
			net_handle_0x06(x, y, z);
			break;
		}
		case PKT_USE_ENTITY: {
			int user_id = net_read_int(),
				target_id = net_read_int();
			bool is_attack = net_read_bool();
			net_handle_0x07(user_id, target_id, is_attack);
			break;
		}
		case PKT_UPDATE_HEALTH:
			net_handle_0x08(net_read_short());
			break;
		case PKT_RESPAWN:
			net_handle_0x09(net_read_byte());
			break;
		case PKT_PLAYER_MOVE_AND_LOOK: {
			double x = net_read_double(),
				s = net_read_double(),
				y = net_read_double(),
				z = net_read_double();
			float yaw = net_read_float(),
				pitch = net_read_float();
			bool on_ground = net_read_bool();
			net_handle_0x0D(x, s, y, z, yaw, pitch, on_ground);
			break;
		}
		case PKT_USE_BED: {
			int ent_id = net_read_int();
			byte unused = net_read_byte();
			int x = net_read_int();
			byte y = net_read_byte();
			int z = net_read_int();
			net_handle_0x11(ent_id, unused, x, y, z);
			break;
		}
		case PKT_ANIMATION: {
			int eid = net_read_int();
			byte anim = net_read_byte();
			net_handle_0x12(eid, anim);
			break;
		}
		case PKT_ENTITY_ACTION: {
			int eid = net_read_int();
			byte anim = net_read_byte();
			net_handle_0x13(eid, anim);
			break;
		}
		case PKT_NAMED_ENTITY_SPAWN: {
			int ent_id = net_read_int();
			string16 name = net_read_string16();
			int x = net_read_int(),
				y = net_read_int(),
				z = net_read_int();
			byte yaw = net_read_byte(),
				pitch = net_read_byte();
			short held_item = net_read_short();
			net_handle_0x14(ent_id, name, x, y, z, yaw, pitch, held_item);
			break;
		}
		case PKT_PICKUP_SPAWN: {
			int ent_id = net_read_int();
			short item_id = net_read_short();
			byte cnt = net_read_byte();
			short item_md = net_read_short();
			int x = net_read_int(),
				y = net_read_int(),
				z = net_read_int();
			byte yaw = net_read_byte(),
				pitch = net_read_byte(),
				roll = net_read_byte();
			net_handle_0x15(ent_id, item_id, cnt, item_md, x, y, z, yaw, pitch, roll);
			break;
		}
		case PKT_COLLECT_ITEM: {
			int eid = net_read_int(),
				pid = net_read_int();
			net_handle_0x16(eid, pid);
			break;
		}
		case PKT_ADD_OBJECT_OR_VEHICLE: {
			int ent_id = net_read_int();
			byte type = net_read_byte();
			int x = net_read_int(),
				y = net_read_int(),
				z = net_read_int(),
				owner_id = net_read_int();
			short vx = 0, vy = 0, vz = 0;
			if(owner_id > 0) {
				vx = net_read_short();
				vy = net_read_short();
				vz = net_read_short();
			}
			net_handle_0x17(ent_id, type, x, y, z, owner_id, vx, vy, vz);
			break;
		}
		case PKT_MOB_SPAWN: {
			int eid = net_read_int();
			byte type = net_read_byte();
			int x = net_read_int(),
				y = net_read_int(),
				z = net_read_int();
			byte yaw = net_read_byte(),
				pitch = net_read_byte();
			net_handle_0x18(eid, type, x, y, z, yaw, pitch);
			break;
		}
		case PKT_ENTITY_PAINTING: {
			int eid = net_read_int();
			string16 title = net_read_string16();
			int x = net_read_int(),
				y = net_read_int(),
				z = net_read_int(),
				dir = net_read_int();
			net_handle_0x19(eid, title, x, y, z, dir);
			break;
		}
		case PKT_ENTITY_VELOCITY: {
			int eid = net_read_int();
			short vx = net_read_short(),
				vy = net_read_short(),
				vz = net_read_short();
			net_handle_0x1C(eid, vx, vy, vz);
			break;
		}
		case PKT_DESTROY_ENTITY:
			net_handle_0x1D(net_read_int());
			break;
		case PKT_ENTITY:
			net_handle_0x1E(net_read_int());
			break;
		case PKT_ENTITY_RELATIVE_MOVE: {
			int eid = net_read_int();
			byte x = net_read_byte(),
				y = net_read_byte(),
				z = net_read_byte();
			net_handle_0x1F(eid, x, y, z);
			break;
		}
		case PKT_ENTITY_LOOK: {
			int eid = net_read_int();
			byte yaw = net_read_byte(),
				pitch = net_read_byte();
			net_handle_0x20(eid, yaw, pitch);
			break;
		}
		case PKT_ENTITY_LOOK_AND_RELATIVE_MOVE: {
			int eid = net_read_int();
			byte x = net_read_byte(),
				y = net_read_byte(),
				z = net_read_byte(),
				yaw = net_read_byte(),
				pitch = net_read_byte();
			net_handle_0x21(eid, x, y, z, yaw, pitch);
			break;
		}
		case PKT_ENTITY_TELEPORT: {
			int eid = net_read_int(),
				x = net_read_int(),
				y = net_read_int(),
				z = net_read_int();
			byte yaw = net_read_byte(),
				pitch = net_read_byte();
			net_handle_0x22(eid, x, y, z, yaw, pitch);
			break;
		}
		case PKT_ENTITY_STATUS: {
			int eid = net_read_int();
			byte status = net_read_byte();
			net_handle_0x26(eid, status);
			break;
		}
		case PKT_ATTACH_ENTITY: {
			int eid = net_read_int(),
				vehid = net_read_int();
			net_handle_0x27(eid, vehid);
			break;
		}
		case PKT_ENTITY_METADATA:
			net_handle_0x28(net_read_int());
			break;
		case PKT_PRE_CHUNK: {
			int cx = net_read_int(),
				cy = net_read_int();
			bool load = net_read_bool();
			net_handle_0x32(cx, cy, load);
			break;
		}
		case PKT_MAP_CHUNK: {
			int x, y, z;
			byte sx, sy, sz;
			void *b;

			x = net_read_int();
			y = net_read_short();
			z = net_read_int();
			sx = net_read_byte();
			sy = net_read_byte();
			sz = net_read_byte();

			i = net_read_int();
			b = alloca(i);
			net_read_buf(b, i);

			net_handle_0x33(x, y, z, sx, sy, sz, i, b);

			break;
		}
		case PKT_MULTI_BLOCK_CHANGE: {
			int cx = net_read_int(),
				cz = net_read_int();
			short sz = net_read_short();

			short *b1 = alloca(sz * sizeof(short));
			byte *b2 = alloca(sz);
			byte *b3 = alloca(sz);

			for(i = 0; i < sz; i++)
				b1[i] = net_read_short();
			net_read_buf(b2, sz);
			net_read_buf(b3, sz);

			net_handle_0x34(cx, cz, sz, b1, b2, b3);

			break;
		}
		case PKT_BLOCK_CHANGE: {
			int x = net_read_int();
			byte y = net_read_byte();
			int z = net_read_int();
			byte id = net_read_byte();
			byte md = net_read_byte();
			net_handle_0x35(x, y, z, id, md);
			break;
		}
		case PKT_BLOCK_ACTION: {
			int x = net_read_int();
			short y = net_read_short();
			int z = net_read_int();
			byte d1 = net_read_byte(),
				d2 = net_read_byte();
			net_handle_0x36(x, y, z, d1, d2);
			break;
		}
		case PKT_EXPLOSION: {
			struct ni_off_coord *b;
			double x = net_read_double(),
				y = net_read_double(),
				z = net_read_double();
			float r = net_read_float();
			int n = net_read_int();

			b = alloca(n * sizeof(*b));
			for(i = 0; i < n; i++)
				net_read_buf(&b[i], sizeof(b[i]));

			net_handle_0x3C(x, y, z, r, n, b);
			break;
		}
		case PKT_SOUND_EFFECT: {
			int effid = net_read_int();
			int x = net_read_int();
			byte y = net_read_byte();
			int z = net_read_int();
			int extra = net_read_int();
			net_handle_0x3D(effid, x, y, z, extra);
			break;
		}
		case PKT_NEW_OR_INVALID_STATE:
			net_handle_0x46(net_read_byte());
			break;
		case PKT_THUNDERBOLT: {
			int eid = net_read_int();
			bool unused = net_read_bool();
			int x = net_read_int(),
				y = net_read_int(),
				z = net_read_int();
			net_handle_0x47(eid, unused, x, y, z);
			break;
		}
		case PKT_OPEN_WINDOW: {
			byte gui_id = net_read_byte(),
				gui_type = net_read_byte();
			string8 title = net_read_string8();
			byte num_slots = net_read_byte();
			net_handle_0x64(gui_id, gui_type, title, num_slots);
			break;
		}
		case PKT_CLOSE_WINDOW:
			net_handle_0x65(net_read_byte());
			break;
		case PKT_WINDOW_CLICK: {
			byte b[4] = {0};
			short s[4] = {0};
			b[0] = net_read_byte();
			s[0] = net_read_short();
			b[1] = net_read_byte();
			s[1] = net_read_short();
			b[2] = net_read_bool();
			s[2] = net_read_short();
			if(s[2] != -1) {
				b[3] = net_read_byte();
				s[3] = net_read_short();
			}
			net_handle_0x66(b[0], s[0], b[1], s[1], b[2], s[2], b[3], s[3]);
			break;
		}
		case PKT_SET_SLOT: {
			byte b[2] = {0};
			short s[3] = {0};
			b[0] = net_read_byte();
			s[0] = net_read_short();
			s[1] = net_read_short();
			if(s[1] != -1) {
				b[1] = net_read_byte();
				s[2] = net_read_short();
			}
			net_handle_0x67(b[0], s[0], s[1], b[1], s[2]);
			break;
		}
		case PKT_WINDOW_ITEMS: {
			struct ni_wi_payload *p;
			byte gui = net_read_byte();
			short cnt = net_read_short();
			p = alloca(cnt * sizeof(*p));
			for(i = 0; i < cnt; i++) {
				p[i].item_id = net_read_short();
				if(p[i].item_id != -1) {
					p[i].count = net_read_byte();
					p[i].metadata = net_read_short();
				}
			}
			net_handle_0x68(gui, cnt, p);
			break;
		}
		case PKT_UPDATE_PROGRESS_BAR: {
			byte gui_id = net_read_byte();
			short type = net_read_short(),
				prog = net_read_short();
			net_handle_0x69(gui_id, type, prog);
			break;
		}
		case PKT_TRANSACTION: {
			byte gui_id = net_read_byte();
			short act = net_read_short();
			bool accepted = net_read_bool();
			net_handle_0x6A(gui_id, act, accepted);
			break;
		}
		case PKT_UPDATE_SIGN: {
			int x = net_read_int();
			short y = net_read_short();
			int z = net_read_int();
			string16 l[4];
			l[0] = net_read_string16();
			l[1] = net_read_string16();
			l[2] = net_read_string16();
			l[3] = net_read_string16();
			net_handle_0x82(x, y, z, l[0], l[1], l[2], l[3]);
			break;
		}
		case PKT_ITEM_DATA: {
			short s[2];
			byte n, *b;
			s[0] = net_read_short();
			s[1] = net_read_short();
			n = net_read_byte();
			b = alloca(n);
			net_read_buf(b, n);
			net_handle_0x83(s[0], s[1], n, b);
			break;
		}
		case PKT_INCREMENT_STATISTIC: {
			int stat = net_read_int();
			byte amount = net_read_byte();
			net_handle_0xC8(stat, amount);
			break;
		}
		case PKT_DISCONNECT: {
			net_handle_0xFF(net_read_string16());
			break;
		}
		default: {
			con_printf("unknown packet_id %hhd (0x%hhx)! disconnecting\n", packet_id, packet_id);
			net_shutdown();
			return false;
		}
	}
	return true;
}

void net_shutdown(void)
{
	init_ok = false;
	con_printf("net shutting down...\n");

	// close socket
	setblocking(sockfd, true);
	if(sockfd != -1) {
		if(close(sockfd) < 0) {
			perror("close");
			// bruh :|
		}
		sockfd = -1;
	}

	cl.state = cl_disconnected;
	vid_lock_fps();
}

void net_read_buf(void *dest, size_t n)
{
	ssize_t n_read;

	if(!init_ok || !dest || n == 0)
		return;

	// peek does not consume the bytes
	// we save them in a buffer in case we are missing some for an entire packet
	// if we are missing some then we can try to read them during the next frame
	n_read = recv(sockfd, read_buffer, total_read+n, MSG_PEEK);

	if(n_read == 0) {
		// EOF - todo: disonnect here? read recv man page and see return value 0
		read_ok = false;
		longjmp(read_abort, 1);
	} else if(n_read == -1) {
		read_ok = false;
		// EAGAIN is a common when using non-blocking sockets
		// it just means no data is currently available
		if(errno != EAGAIN)
			con_printf("net_read_buf: %s (error %d)\n", strerror(errno), errno);
		longjmp(read_abort, 1);
	} else if((size_t) n_read - total_read != n) {
		// read some, but not everything
		// delay until next frame
		read_ok = false;
		total_read = n_read;
		longjmp(read_abort, 1);
	} else {
		read_ok = true;
		memcpy(dest, read_buffer + total_read, n);
		total_read = n_read;
	}
}

byte net_read_byte(void)
{
	byte b;
	net_read_buf(&b, 1);
	return b;
}

short net_read_short(void)
{
	short s;
	net_read_buf(&s, 2);
	return SDL_SwapBE16(s);
}

int net_read_int(void)
{
	int i;
	net_read_buf(&i, 4);
	return SDL_SwapBE32(i);
}

long net_read_long(void)
{
	long l;
	net_read_buf(&l, 8);
	return SDL_SwapBE64(l);
}

float net_read_float(void)
{
	union {
		float f;
		int i;
	} u;
	u.i = net_read_int();
	return u.f;
}

double net_read_double(void)
{
	union {
		double d;
		long l;
	} u;
	u.l = net_read_long();
	return u.d;
}

bool net_read_bool(void)
{
	return net_read_byte() != 0;
}

string8 net_read_string8(void)
{
	string8 s;
	short len;

	len = net_read_short();
	s = mem_alloc(len + 1);
	net_read_buf(s, len);
	s[len] = 0;

	return s; // you take care of freeing me now ;-)
}

string16 net_read_string16(void)
{
	string16 s;
	short size, len;
	int i;

	len = net_read_short();
	size = (len + 1) * sizeof(char16);
	s = mem_alloc(size);
	for(i = 0; i < len; i++) {
		// swap twice becauseeeee... java? yeah, it's all java's fault
		s[i] = SDL_SwapBE16(net_read_short());
	}
	s[len] = 0;

	return s; // you take care of freeing me now ;-)
}

void net_write_buf(const void *buf, size_t n)
{
	ssize_t n_written;

	if(!init_ok || !buf || n == 0 || cl.state == cl_disconnected)
		return;

	n_written = send(sockfd, buf, n, MSG_NOSIGNAL);
	if(n_written == -1) {
		if(errno == EAGAIN || errno == EWOULDBLOCK) {
			// is this right?
			con_printf("net_write_buf: operation WILL block\n");
			setblocking(sockfd, true);
			net_write_buf(buf, n);
			setblocking(sockfd, false);
		} else {
			perror("net_write_buf");
			net_shutdown();
			return;
		}
	} else if((size_t) n_written != n) {
		// is this right?
		con_printf("net_write_buf: wrote only %ld/%lu bytes, gonna block\n", n_written, n);
		setblocking(sockfd, true);
		write(sockfd, (byte *)buf + n_written, n - n_written);
		setblocking(sockfd, false);
	}
}

void net_write_byte(ubyte v)
{
	net_write_buf(&v, 1);
}

void net_write_short(short v)
{
	v = SDL_SwapBE16(v);
	net_write_buf(&v, 2);
}

void net_write_int(int v)
{
	v = SDL_SwapBE32(v);
	net_write_buf(&v, 4);
}

void net_write_long(long v)
{
	v = SDL_SwapBE64(v);
	net_write_buf(&v, 8);
}

void net_write_float(float v)
{
	union {
		float f;
		int i;
	} u;
	u.f = v;
	net_write_int(u.i);
}

void net_write_double(double v)
{
	union {
		double d;
		long l;
	} u;
	u.d = v;
	net_write_long(u.l);
}

void net_write_bool(bool v)
{
	net_write_byte(v);
}

void net_write_string8(string8 v)
{
	short len = strlen(v);
	net_write_short(len);
	net_write_buf(v, len);
}

void net_write_string16(string16 v)
{
	short len = c16strlen(v);
	int i;
	net_write_short(len);
	for(i = 0; i < len; i++) {
		net_write_short(v[i]);
	}
}

void skip_entity_metadata(void)
{
	byte field_id;
	while((field_id = net_read_byte()) != 0x7F && read_ok) {
		switch((field_id >> 5) & 7) {
			case 0:
				net_read_byte();
				break;
			case 1:
				net_read_short();
				break;
			case 2:
				net_read_int();
				break;
			case 3:
				net_read_float();
				break;
			case 4:
				net_read_string16();
				break;
			case 5:
				net_read_short();
				net_read_byte();
				net_read_short();
				break;
			case 6:
				net_read_int();
				net_read_int();
				net_read_int();
				break;
		}
	}
}

void connect_f(void)
{
	struct addrinfo hints = {0}, *info;
	char *addrstr, *p;
	int port, err;

	if(cmd_argc() != 2) {
		con_printf("usage: %s <ip>[:<port>]\n", cmd_argv(0));
	}

	addrstr = cmd_argv(1);

	if(cl.state != cl_disconnected) {
		con_printf("disconnect first\n");
		return;
	}

	p = addrstr;
	while(*p != '\0' && *p != ':')
		p++;

	if(*p == '\0') {
		// no port specified
		port = 25565;
	} else {
		port = strtol(p, NULL, 10);
		if(errno == EINVAL) {
			con_printf("invalid ip\n");
			return;
		}
		*p = 0; // set to 0 for ip address parsing (probably needed but idk)
	}

	// todo: ipv6 support if you can even use that with a beta server
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if((err = getaddrinfo(addrstr, "http", &hints, &info))) {
		con_printf("error: %s\n", gai_strerror(err));
		return;
	}

	if(info != NULL) {
		net_init();
		net_connect((struct sockaddr_in *) info->ai_addr, port);
	}

	freeaddrinfo(info);
	return;
}

void disconnect_f(void)
{
	if(cl.state != cl_disconnected) {
		if(cl.state == cl_connected) {
			// set to blocking so the disconnect packet gets sent
			// before the socket is closed
			setblocking(sockfd, true);
			net_write_0xFF(c16("Quitting"));
			setblocking(sockfd, false);
		}

		// todo: world_cleanup() or something
		// or just move world vars to cl.game
		memset(&cl.game, 0, sizeof(cl.game));

		// net_update will disconnect next update
		cl.state = cl_disconnected;
		vid_lock_fps();
		should_disconnect = true;

		con_show();
	}
}

void say_f(void)
{
	if(cmd_argc() == 1) {
		con_printf("usage: %s <message>\n", cmd_argv(0));
	}

	if(cl.state != cl_connected) {
		con_printf("can't \"%s\", not connected\n", cmd_argv(0));
		return;
	}

	net_write_0x03(c16(cmd_args(1, cmd_argc())));
}

// fixme: this is temporary (or maybe leave it for dat sweet customizability and just make the respawn button execute this command)
void respawn_f(void)
{
	if(cl.state != cl_connected) {
		con_printf("can't \"%s\", not connected\n", cmd_argv(0));
		return;
	}
	net_write_0x09(0);
}
