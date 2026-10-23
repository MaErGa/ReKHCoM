/* The link cable over the network (alpha, port/progress/ONLINE_ESTUDIO.md): a UDP socket to the other player.
 * src/sio/sio.c keeps the game's own packet queue and asks here for the datagrams; this file only moves bytes.
 *
 * KHCOM_NETLINK=steam:host         open a Steam room (port_steamlink.c): the cable's parent, player 1
 * KHCOM_NETLINK=steam:join         enter a friend's Steam room: the child, player 2
 * KHCOM_NETLINK=host:<port>        wait for the other player on that UDP port (player 1)
 * KHCOM_NETLINK=join:<ip>:<port>   connect to a host by address or name (player 2)
 * KHCOM_NETLINK=relay:<server>:<port>:<code>:host|join   meet through a relay (port/tools/netlink_relay.py on a
 *                                  VPS) by a room code: no ports to open, any platform with UDP (crossplay)
 * Without the variable, netlink.cfg in the data folder (the launcher writes it) holds the same text.
 * KHCOM_NETLINK_LAG=<frames>       tests: hold every datagram that many frames before it goes out
 * KHCOM_NETLINK_JITTER=<frames>    tests: and up to that many more, at random (an uneven connection)
 * KHCOM_NETLINK_LOSS=<percent>     tests: drop that share of the datagrams sent
 * KHCOM_NETLINK_DROP=<from>-<to>   tests: an outage, nothing sent between those game frames
 */
#include "port/port.h"
#include "port/port_netlink.h"

#if __STDC_HOSTED__ && !defined(__EMSCRIPTEN__)
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET Sock;
#define BAD_SOCK INVALID_SOCKET
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int Sock;
#define BAD_SOCK (-1)
#define closesocket close
#endif

static int sChecked, sRole, sSteam;
static Sock sSock = BAD_SOCK;
static struct sockaddr_in sPeer;
static int sHavePeer;
static uint64_t sLastHeard;
static int sLag, sLoss, sJitter;
static uint32_t sDropFrom, sDropTo;
static int sRelay, sRelayPlayers;
static char sRoom[17];

/* datagrams held back by KHCOM_NETLINK_LAG */
typedef struct {
    uint32_t due;
    int size;
    uint8_t data[PORT_NETLINK_MAX];
} Held;
static Held sHeld[256];
static int sHeldCount;
static uint32_t sFrame;

/* host (an address or a name) and port into to */
static int Resolve(const char* host, int port, struct sockaddr_in* to) {
    struct addrinfo hints, *res = NULL;
    SDL_memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(host, NULL, &hints, &res) != 0 || !res)
        return 0;
    SDL_memcpy(to, res->ai_addr, sizeof(*to));
    to->sin_port = htons((unsigned short)port);
    freeaddrinfo(res);
    return 1;
}

static void RelayHello(void) {
    uint8_t d[2 + sizeof(sRoom)];
    size_t n = SDL_strlen(sRoom);
    d[0] = 'J';
    d[1] = (uint8_t)sRole;
    SDL_memcpy(d + 2, sRoom, n);
    sendto(sSock, (const char*)d, (int)(2 + n), 0, (struct sockaddr*)&sPeer, sizeof(sPeer));
}

static int Open(const char* spec) {
    char host[128] = "";
    int port = 0;
    struct sockaddr_in me;
#ifdef _WIN32
    WSADATA wsa;
    u_long nb = 1;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 0;
#endif
    if (SDL_strncmp(spec, "host:", 5) == 0) {
        port = SDL_atoi(spec + 5);
        sRole = PORT_NETLINK_HOST;
    } else if (SDL_strncmp(spec, "join:", 5) == 0) {
        const char* colon = SDL_strrchr(spec + 5, ':');
        if (!colon || colon - (spec + 5) >= (int)sizeof(host))
            return 0;
        SDL_memcpy(host, spec + 5, (size_t)(colon - (spec + 5)));
        host[colon - (spec + 5)] = 0;
        port = SDL_atoi(colon + 1);
        sRole = PORT_NETLINK_JOIN;
    } else if (SDL_strncmp(spec, "relay:", 6) == 0) { /* relay:<server>:<port>:<code>:host|join */
        char rest[160], *f[4];
        int i;
        SDL_strlcpy(rest, spec + 6, sizeof(rest));
        f[0] = rest;
        for (i = 1; i < 4; i++) {
            f[i] = f[i - 1] ? SDL_strchr(f[i - 1], ':') : NULL;
            if (f[i])
                *f[i]++ = 0;
        }
        if (!f[3])
            return 0;
        SDL_strlcpy(host, f[0], sizeof(host));
        port = SDL_atoi(f[1]);
        SDL_strlcpy(sRoom, f[2], sizeof(sRoom));
        sRole = SDL_strcmp(f[3], "host") == 0 ? PORT_NETLINK_HOST : PORT_NETLINK_JOIN;
        sRelay = 1;
    }
    if (port <= 0 || port > 65535 || !sRole)
        return 0;
    sSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sSock == BAD_SOCK)
        return 0;
#ifdef _WIN32
    ioctlsocket(sSock, FIONBIO, &nb);
#else
    fcntl(sSock, F_SETFL, fcntl(sSock, F_GETFL, 0) | O_NONBLOCK);
#endif
    SDL_memset(&me, 0, sizeof(me));
    me.sin_family = AF_INET;
    me.sin_addr.s_addr = htonl(INADDR_ANY);
    me.sin_port = htons((unsigned short)(sRole == PORT_NETLINK_HOST && !sRelay ? port : 0));
    if (bind(sSock, (struct sockaddr*)&me, sizeof(me)) != 0) {
        PortLog("netlink: could not bind port %d", port);
        closesocket(sSock);
        sSock = BAD_SOCK;
        return 0;
    }
    if (sRole == PORT_NETLINK_JOIN || sRelay) { /* the other player, or the relay, by address or name */
        if (!Resolve(host, port, &sPeer)) {
            PortLog("netlink: %s not found", host);
            return 0;
        }
        sHavePeer = 1;
    }
    if (SDL_getenv("KHCOM_NETLINK_LAG"))
        sLag = SDL_atoi(SDL_getenv("KHCOM_NETLINK_LAG"));
    if (SDL_getenv("KHCOM_NETLINK_DROP")) {
        unsigned a = 0, b = 0;
        if (SDL_sscanf(SDL_getenv("KHCOM_NETLINK_DROP"), "%u-%u", &a, &b) == 2) {
            sDropFrom = a;
            sDropTo = b;
        }
    }
    if (SDL_getenv("KHCOM_NETLINK_JITTER"))
        sJitter = SDL_atoi(SDL_getenv("KHCOM_NETLINK_JITTER"));
    if (SDL_getenv("KHCOM_NETLINK_LOSS"))
        sLoss = SDL_atoi(SDL_getenv("KHCOM_NETLINK_LOSS"));
    PortLog("netlink: %s, port %d%s%s%s%s", sRole == PORT_NETLINK_HOST ? "hosting" : "joining", port,
            sRole == PORT_NETLINK_JOIN || sRelay ? " at " : "", host, sRelay ? ", room " : "", sRelay ? sRoom : "");
    if (sRelay)
        RelayHello();
    return 1;
}

/* the link's setting: KHCOM_NETLINK, else the first line of netlink.cfg */
static const char* Spec(void) {
    static char line[160];
    const char* env = SDL_getenv("KHCOM_NETLINK");
    FILE* f;
    if (env)
        return env;
    f = fopen(PortConfigPath("netlink.cfg"), "r");
    if (!f)
        return NULL;
    if (!fgets(line, sizeof(line), f))
        line[0] = 0;
    fclose(f);
    line[strcspn(line, "\r\n")] = 0;
    return line[0] && SDL_strcmp(line, "off") != 0 ? line : NULL;
}

int PortNetlinkRole(void) {
    if (!sChecked) {
        const char* spec = Spec();
        sChecked = 1;
        if (spec && SDL_strncmp(spec, "steam:", 6) == 0) {
            sRole = SDL_strcmp(spec + 6, "host") == 0 ? PORT_NETLINK_HOST : PORT_NETLINK_JOIN;
            sSteam = PortSteamlinkStart(sRole);
            if (!sSteam)
                sRole = 0;
            else
                PortLog("netlink: through Steam, %s", sRole == PORT_NETLINK_HOST ? "hosting" : "joining");
        } else if (spec && !Open(spec)) {
            sRole = 0;
        }
    }
    return sSock != BAD_SOCK || sSteam ? sRole : 0;
}

static void SendNow(const uint8_t* data, int size) {
    if (sDropTo && sFrame >= sDropFrom && sFrame < sDropTo)
        return;
    if (sLoss > 0 && (int)(SDL_rand(100)) < sLoss)
        return;
    if (sSteam) {
        PortSteamlinkSend(data, size);
        return;
    }
    if (sRelay) {
        uint8_t d[PORT_NETLINK_MAX + 1];
        d[0] = 'D';
        SDL_memcpy(d + 1, data, (size_t)size);
        sendto(sSock, (const char*)d, size + 1, 0, (struct sockaddr*)&sPeer, sizeof(sPeer));
        return;
    }
    sendto(sSock, (const char*)data, size, 0, (struct sockaddr*)&sPeer, sizeof(sPeer));
}

void PortNetlinkSend(const uint8_t* data, int size) {
    if (!PortNetlinkRole() || !(sHavePeer || sSteam) || size <= 0 || size > PORT_NETLINK_MAX)
        return;
    if ((sLag > 0 || sJitter > 0) && sHeldCount < (int)SDL_arraysize(sHeld)) {
        Held* h = &sHeld[sHeldCount++];
        h->due = sFrame + (uint32_t)sLag + (sJitter > 0 ? (uint32_t)SDL_rand(sJitter + 1) : 0);
        h->size = size;
        SDL_memcpy(h->data, data, (size_t)size);
        return;
    }
    SendNow(data, size);
}

int PortNetlinkRecv(uint8_t* data, int cap) {
    struct sockaddr_in from;
#ifdef _WIN32
    int len = sizeof(from);
#else
    socklen_t len = sizeof(from);
#endif
    int n;
    if (!PortNetlinkRole())
        return 0;
    if (sSteam) {
        n = PortSteamlinkRecv(data, cap);
        if (n > 0)
            sLastHeard = SDL_GetTicks();
        return n;
    }
    n = (int)recvfrom(sSock, (char*)data, cap, 0, (struct sockaddr*)&from, &len);
    if (n <= 0)
        return 0;
    if (sRelay) { /* only the relay writes: 'O' its room count, 'D' the other player's datagram */
        if (from.sin_addr.s_addr != sPeer.sin_addr.s_addr || from.sin_port != sPeer.sin_port)
            return 0;
        if (data[0] == 'O' && n >= 2) {
            if (data[1] != sRelayPlayers)
                PortLog("netlink: relay room %s, %d player(s)", sRoom, data[1]);
            sRelayPlayers = data[1];
            return 0;
        }
        if (data[0] != 'D')
            return 0;
        SDL_memmove(data, data + 1, (size_t)--n);
    } else if (sRole == PORT_NETLINK_HOST && !sHavePeer) { /* the first player to write is the other end of the cable */
        sPeer = from;
        sHavePeer = 1;
        PortLog("netlink: player 2 at %s:%d", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
    } else if (from.sin_addr.s_addr != sPeer.sin_addr.s_addr || from.sin_port != sPeer.sin_port) {
        /* player 2 back from another address (a new connection after a cut): taken once the old one went quiet */
        if (sRole != PORT_NETLINK_HOST || PortNetlinkSilentMs() < 2000)
            return 0;
        sPeer = from;
        PortLog("netlink: player 2 back at %s:%d", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
    }
    sLastHeard = SDL_GetTicks();
    return n;
}

void PortNetlinkFrame(void) {
    int i, kept = 0;
    sFrame++;
    if (sSteam)
        PortSteamlinkFrame();
    if (sRelay && sSock != BAD_SOCK && sFrame % 60 == 0) /* the room again: the relay finds a changed address */
        RelayHello();
    for (i = 0; i < sHeldCount; i++) {
        if (sHeld[i].due <= sFrame)
            SendNow(sHeld[i].data, sHeld[i].size);
        else
            sHeld[kept++] = sHeld[i];
    }
    sHeldCount = kept;
}

int PortNetlinkSilentMs(void) {
    return sLastHeard ? (int)(SDL_GetTicks() - sLastHeard) : 1 << 30;
}

int PortBtlCamTrace(void) {
    static int trace = -1;
    if (trace < 0)
        trace = SDL_getenv("KHCOM_BTLCAM") != NULL;
    return trace;
}

int PortNetlinkTrace(void) {
    static int trace = -1;
    if (trace < 0)
        trace = SDL_getenv("KHCOM_NETLINK_TRACE") != NULL;
    return trace;
}
#else
int PortNetlinkTrace(void) {
    return 0;
}
int PortBtlCamTrace(void) {
    return 0;
}
int PortNetlinkRole(void) {
    return 0;
}
void PortNetlinkSend(const uint8_t* data, int size) {
    (void)data;
    (void)size;
}
int PortNetlinkRecv(uint8_t* data, int cap) {
    (void)data;
    (void)cap;
    return 0;
}
void PortNetlinkFrame(void) {
}
int PortNetlinkSilentMs(void) {
    return 1 << 30;
}
#endif
