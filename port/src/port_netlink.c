/* The link cable over the network (alpha, port/progress/ONLINE_ESTUDIO.md): a UDP socket to the other player.
 * src/sio/sio.c keeps the game's own packet queue and asks here for the datagrams; this file only moves bytes.
 *
 * KHCOM_NETLINK=host:<port>        wait for the other player on that port (the cable's parent, player 1)
 * KHCOM_NETLINK=join:<ip>:<port>   connect to a host (the child, player 2)
 * KHCOM_NETLINK_LAG=<frames>       tests: hold every datagram that many frames before it goes out
 * KHCOM_NETLINK_LOSS=<percent>     tests: drop that share of the datagrams sent
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
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int Sock;
#define BAD_SOCK (-1)
#define closesocket close
#endif

static int sChecked, sRole;
static Sock sSock = BAD_SOCK;
static struct sockaddr_in sPeer;
static int sHavePeer;
static uint64_t sLastHeard;
static int sLag, sLoss;

/* datagrams held back by KHCOM_NETLINK_LAG */
typedef struct {
    uint32_t due;
    int size;
    uint8_t data[PORT_NETLINK_MAX];
} Held;
static Held sHeld[64];
static int sHeldCount;
static uint32_t sFrame;

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
    me.sin_port = htons((unsigned short)(sRole == PORT_NETLINK_HOST ? port : 0));
    if (bind(sSock, (struct sockaddr*)&me, sizeof(me)) != 0) {
        PortLog("netlink: could not bind port %d", port);
        closesocket(sSock);
        sSock = BAD_SOCK;
        return 0;
    }
    if (sRole == PORT_NETLINK_JOIN) {
        SDL_memset(&sPeer, 0, sizeof(sPeer));
        sPeer.sin_family = AF_INET;
        sPeer.sin_port = htons((unsigned short)port);
        if (inet_pton(AF_INET, host, &sPeer.sin_addr) != 1) {
            PortLog("netlink: bad address %s", host);
            return 0;
        }
        sHavePeer = 1;
    }
    if (SDL_getenv("KHCOM_NETLINK_LAG"))
        sLag = SDL_atoi(SDL_getenv("KHCOM_NETLINK_LAG"));
    if (SDL_getenv("KHCOM_NETLINK_LOSS"))
        sLoss = SDL_atoi(SDL_getenv("KHCOM_NETLINK_LOSS"));
    PortLog("netlink: %s, port %d%s%s", sRole == PORT_NETLINK_HOST ? "hosting" : "joining", port,
            sRole == PORT_NETLINK_JOIN ? " at " : "", host);
    return 1;
}

int PortNetlinkRole(void) {
    if (!sChecked) {
        const char* spec = SDL_getenv("KHCOM_NETLINK");
        sChecked = 1;
        if (spec && !Open(spec))
            sRole = 0;
    }
    return sSock != BAD_SOCK ? sRole : 0;
}

static void SendNow(const uint8_t* data, int size) {
    if (sLoss > 0 && (int)(SDL_rand(100)) < sLoss)
        return;
    sendto(sSock, (const char*)data, size, 0, (struct sockaddr*)&sPeer, sizeof(sPeer));
}

void PortNetlinkSend(const uint8_t* data, int size) {
    if (!PortNetlinkRole() || !sHavePeer || size <= 0 || size > PORT_NETLINK_MAX)
        return;
    if (sLag > 0 && sHeldCount < (int)SDL_arraysize(sHeld)) {
        Held* h = &sHeld[sHeldCount++];
        h->due = sFrame + (uint32_t)sLag;
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
    n = (int)recvfrom(sSock, (char*)data, cap, 0, (struct sockaddr*)&from, &len);
    if (n <= 0)
        return 0;
    if (sRole == PORT_NETLINK_HOST && !sHavePeer) { /* the first player to write is the other end of the cable */
        sPeer = from;
        sHavePeer = 1;
        PortLog("netlink: player 2 at %s:%d", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
    } else if (from.sin_addr.s_addr != sPeer.sin_addr.s_addr || from.sin_port != sPeer.sin_port) {
        return 0; /* someone else */
    }
    sLastHeard = SDL_GetTicks();
    return n;
}

void PortNetlinkFrame(void) {
    int i, kept = 0;
    sFrame++;
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
