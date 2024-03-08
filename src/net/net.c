#include <SDL2/SDL_endian.h>
#include "common.h"
#include <SDL2/SDL_net.h>
#include <errno.h>
#include "net.h"
#include "net_internal.h"
#include "client/client.h"
#include "client/console.h"
#include "vid/vid.h"
#include <uchar.h>
#include "packets.h"
#include <SDL_timer.h>

#define ERR(desc1, desc2) do {                            \
    con_printf("(%s) network error: %s\n", desc1, desc2); \
    net_shutdown();                                       \
} while(0)

#define PACKET(id, name, stuff) [id] = #name,
const char *packet_names[256] = {
#include "packets_def.h"
};
#undef PACKET

static bool init_ok = false;
static bool should_disconnect = false;

static TCPsocket socket = NULL;
static SDLNet_SocketSet socket_set = NULL;

void disconnect_f(void);
void connect_f(void);
void say_f(void);
void respawn_f(void);

errcode net_init(void)
{
    if(SDLNet_Init() != 0) {
        ERR("0", SDLNet_GetError());
        return ERR_NETWORK;
    }
    init_ok = true;
    return ERR_OK;
}

void net_connect(IPaddress *addr)
{
    cl.state = cl_disconnected;

    if(addr != NULL) {
        socket = SDLNet_TCP_Open(addr);
        if(socket == NULL) {
            ERR("1", SDLNet_GetError());
            return;
        }

        socket_set = SDLNet_AllocSocketSet(1);
        if(socket_set == NULL) {
            ERR("2", SDLNet_GetError());
            SDLNet_TCP_Close(socket);
            return;
        }

        // should not error unless REALLY really bad stuff happens
        SDLNet_TCP_AddSocket(socket_set, socket);

        cl.state = cl_connecting;
    } else {
        if(socket != NULL) {
            SDLNet_FreeSocketSet(socket_set);
            SDLNet_TCP_Close(socket);
        }
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
        while(net_read_packet());

        net_write_packets();
    }

    // disconnect if requested
    if(should_disconnect) {
        cl.state = cl_disconnected;

        // reset handshake flag and send disconnect packet
        net_write_packets();

        // wait a bit before closing the socket, didn't find a way to wait
        // for the writes to finish :/
        // fixme!
        // SDL_Delay(250);

        net_connect(NULL);
        should_disconnect = false;
    }

}

// TODO
void read_entity_metadata(void)
{
    byte field_id;
    while((field_id = net_read_byte()) != 0x7F) {
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
            net_free_string16(net_read_string16());
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

static struct ni_wi_payload read_window_items_payload(void)
{
    struct ni_wi_payload item;
    item.item_id = net_read_short();
    if(item.item_id != -1) {
        item.count = net_read_byte();
        item.metadata = net_read_short();
    }
    return item;
}

static bool net_read_packet(void)
{
    ubyte packet_id;

    if(!SDLNet_CheckSockets(socket_set, 0))
        return false; // no data available

    // try to read a packet
    packet_id = net_read_byte();

#define UBYTE(name)                this.name = (ubyte) net_read_byte();
#define BYTE(name)                 this.name = net_read_byte();
#define SHORT(name)                this.name = net_read_short();
#define INT(name)                  this.name = net_read_int();
#define LONG(name)                 this.name = net_read_long();
#define FLOAT(name)                this.name = net_read_float();
#define DOUBLE(name)               this.name = net_read_double();
#define STRING8(name)              this.name = net_read_string8();
#define STRING16(name)             this.name = net_read_string16();
#define BOOL(name)                 this.name = net_read_byte();
#define METADATA(name)             /*this.name = */read_entity_metadata();
#define WINDOW_ITEMS_PAYLOAD(name) this.name = read_window_items_payload();

#define OPT(cond, stuff) if(cond) { stuff }
// todo: wrapper for alloca in case of bad size
#define BUF(type, name, size) this.name = alloca((size) * sizeof(*this.name)); for(size_t i = 0; i < (size_t) (size); i++) { type(name[i]) }

#define PACKET(id, name, stuff) case id: { pkt_ ## name this = {0}; stuff; net_handle_pkt_ ## name(this); return true; }

    switch(packet_id) {

#include "packets_def.h"
    case 0x00: { // keep alive packet
        net_write_byte(0x00);
        return true;
    }
    default: {
        con_printf("unknown packet_id %hhd (0x%hhx)! disconnecting\n", packet_id, packet_id);
        net_shutdown();
        return false;
    }
    }
}

void net_shutdown(void)
{
    if(!init_ok)
        return;

    init_ok = false;

    net_connect(NULL);

    SDLNet_Quit();
}

void net_read_buf(void *dest, int n)
{
    int n_read;

    if(!init_ok || !dest || n == 0)
        return;

    n_read = SDLNet_TCP_Recv(socket, dest, n);

    if(n_read <= 0) {
        ERR("3", SDLNet_GetError());
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

int64_t net_read_long(void)
{
    int64_t l;
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
        int64_t l;
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

    s.length = net_read_short();
    s.data = mem_alloc(s.length + 1);
    net_read_buf(s.data, s.length);
    s.data[s.length] = 0;

    return s;
}

string16 net_read_string16(void)
{
    string16 s;
    short size;
    int i;

    s.length = net_read_short();
    size = (short)((s.length + 1) * sizeof(*s.data));
    s.data = mem_alloc(size);
    for(i = 0; i < s.length; i++)
        s.data[i] = net_read_short();
    s.data[s.length] = 0;

    return s;
}

void net_free_string8(string8 v)
{
    mem_free(v.data);
}

void net_free_string16(string16 v)
{
    mem_free(v.data);
}

string8 net_make_string8(const char *text)
{
    string8 str;
    str.length = (short)strlen(text);
    str.data = mem_alloc(str.length + 1);
    strlcpy(str.data, text, str.length);
    return str;
}

string16 net_make_string16(const char *text)
{
    string16 str;
    int i;
    str.length = (short)strlen(text);
    str.data = mem_alloc((str.length + 1) * sizeof(*str.data));
    for(i = 0; i < str.length; i++) {
        str.data[i] = (char16_t) text[i];
    }
    str.data[i] = u'\0';
    return str;
}

void net_write_buf(const void *buf, int n)
{
    int n_written;

    if(!init_ok || !buf || n == 0 || (cl.state == cl_disconnected && !should_disconnect))
        return;

    n_written = SDLNet_TCP_Send(socket, buf, n);
    if(n_written == -1) {
        // invalid usage
        // prevent further writes?
        ERR("5", "invalid usage");
        init_ok = false;
        return;
    } else if(n_written < n) {
        ERR("4", SDLNet_GetError());
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

void net_write_long(int64_t v)
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
        int64_t l;
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
    net_write_short(v.length);
    net_write_buf(v.data, v.length);
}

void net_write_string16(string16 v)
{
    short len = v.length;
    net_write_short(len);
    for(int i = 0; i < len; i++) {
        net_write_short((short) v.data[i]);
    }
}

void connect_f(void)
{
    char *addrstr, *p;
    int port, err;
    IPaddress addr;

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
            con_printf("invalid ip address port\n");
            return;
        }
        *p = 0; // set to 0 for ip address parsing (idk if needed)
    }

    err = SDLNet_ResolveHost(&addr, addrstr, port);

    if(err != 0) {
        ERR("6", SDLNet_GetError());
        return;
    }

    net_init();
    net_connect(&addr);
}

void disconnect_f(void)
{
    if(cl.state != cl_disconnected) {
        vid_lock_fps();
        cl_end_game();

        // net_update will actually disconnect next update
        should_disconnect = true;
        cl.state = cl_disconnected;

        con_show();
    }
}

void say_f(void)
{
    char *msg;
    string16 message;

    if(cmd_argc() == 1) {
        con_printf("usage: %s <message>\n", cmd_argv(0));
    }

    if(cl.state != cl_connected) {
        con_printf("can't \"%s\", not connected\n", cmd_argv(0));
        return;
    }

    msg = cmd_args(1, cmd_argc());
    message = net_make_string16(msg);

    net_write_pkt_chat_message((pkt_chat_message) {
        .message = message
    });

    net_free_string16(message);
}

// fixme: this is temporary (or maybe leave it for dat sweet customizability and just make the respawn button execute this command)
void respawn_f(void)
{
    if(cl.state != cl_connected) {
        con_printf("can't \"%s\", not connected\n", cmd_argv(0));
        return;
    }
    net_write_pkt_respawn((pkt_respawn) {
        .dimension = 0
    });
}
