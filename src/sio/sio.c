/**
 * sio.c
 * Serial Communication Driver
 */

#include "macros.h"
#include "intr.h"
#include "sio.h"
#include "gba/io_reg.h"
#include "system_state.h"
#include "gba/macro.h"
#include "sio_types.h"
#include <stddef.h>
#include "types.h"
#include "main.h"
#include "sio_api.h"

u8 gSioLastSendCount EWRAM_COMMON(4);
s16 gSioErrorFrameCount EWRAM_COMMON(4);
u16 gSioRecvFrame[4][2] EWRAM_COMMON(16);
u32 gSioErrorStatus EWRAM_COMMON(4);
s32 (*gSioLinkRecvCallback)() EWRAM_COMMON(8);
u8 gSioPlayerCount EWRAM_COMMON(4);
u8 gSioLastRecvCount EWRAM_COMMON(4);
s32 (*gSioLinkSendCallback)() EWRAM_COMMON(4);
u16 gSioCommandRecv[4][2] EWRAM_COMMON(16);
u32 gSioStatus EWRAM_COMMON(4);
u8 gSioHandshake EWRAM_COMMON(4);
u32 gSioPlayerId EWRAM_COMMON(4);
u8 gSioHandshakeRequest EWRAM_COMMON(4);
SioWork gSioWork EWRAM_COMMON(16);
u16 gSioCommandSend[4] EWRAM_COMMON(8);
u8 gSioLinkResult EWRAM_COMMON(4);
u16 gSioSendFrame[4] EWRAM_COMMON(8);

static u8 sSioChecksumReady;
static u16 sSioSavedIme;
static u8 sSioIdleVBlanks;
static u8 sSioSendEmpty;
static u8 sSioPauseTimer;
static u8 sSioPrevPlayerCount;
static s8 sSioAutoStartDone;
static u16 sSioSendNonzero;
static u16 sSioRecvNonzero;

enum SioState {
    SIO_STATE_STOP,
    SIO_STATE_INIT,
    SIO_STATE_HANDSHAKE,
    SIO_STATE_INIT_TIMER,
    SIO_STATE_CONNECTED
};

enum SioRequest {
    SIO_REQUEST_NONE,
    SIO_REQUEST_START,
    SIO_REQUEST_RESET
};

enum SioRole {
    SIO_ROLE_CHILD,
    SIO_ROLE_PARENT = 8
};

enum SioQueueFull {
    SIO_QUEUE_FULL_SEND = 1,
    SIO_QUEUE_FULL_RECV = 2
};

enum SioTimeout {
    SIO_TIMEOUT_PARENT = 1,
    SIO_TIMEOUT_CHILD = 2
};

enum SioHandshakeWord {
    SIO_HANDSHAKE_START = 0x8FFF,
    SIO_HANDSHAKE_IDLE = 0xD5E0
};

#define SIO_MULTI_EMPTY 0xFFFF
#define SIO_PAUSE_REQUEST 0x1000

#ifdef PLATFORM_PC
/* The cable over the network (port/src/port_netlink.c, port/progress/ONLINE_ESTUDIO.md): the game's packet queue
   stays as it is; instead of the registers, each game frame is a slot both players fill with one packet (or none)
   and send by UDP; slot n of player 1 and slot n of player 2 make the pair the cable would have carried, given to
   both games in the same order. The game's own empty state and pause request absorb the network's delay. */
#include "port/port.h"
#include "port/port_netlink.h"
#include "port/port_ui.h"

#define NET_RING 64
#define NET_RESEND 24 /* slots sent again in every datagram, until the other side has them */
#define NET_VERSION 2 /* the slots' rules: both games must play by the same ones (HELLO carries it) */
enum { NET_HELLO = 1, NET_START, NET_SLOTS, NET_HASH };
#define NET_HASH_EVERY 120 /* battle frames between desync checks */
typedef struct {
    u16 words[4];
    u8 pause;
} NetSlot;
static struct {
    u32 mine;   /* slots of ours made: the game's link frames since the connection */
    u32 paired; /* pairs given to the game */
    u32 acked;  /* ours the other side has */
    u32 theirs; /* theirs we have, all of them below */
    NetSlot my[NET_RING], peer[NET_RING];
    u8 peerKnown[NET_RING];
    u8 pauseFrames;
    u8 startFrames; /* the parent repeats START this many frames */
    /* the lockstep, as the cable's: link frame n of both games gets pair n - delay, the same pair on both sides
       whatever their speed; a frame whose pair has not arrived waits for it (the game does not run without it).
       The parent picks the delay from the round trip measured in the handshake and sends it with START. */
    u8 delay;
    u32 frames, waited, waitStart;
    /* the desync check: our hashes and theirs by battle frame (the groups the game consumed) */
    u32 groups, hashAt[2][8], hash[2][8], checks, mismatches;
} sNet;
/* the handshake's measures, kept apart from sNet (cleared on connection) */
static u32 sNetPeerTicks, sNetRttMin;
static u8 sNetPeerVersion, sNetVersionWarned;

/* both hashes of one battle frame known: compare */
static void NetHashCompare(u32 at) {
    int i, j;
    for (i = 0; i < 8; i++)
        if (sNet.hash[0][i] && sNet.hashAt[0][i] == at)
            for (j = 0; j < 8; j++)
                if (sNet.hash[1][j] && sNet.hashAt[1][j] == at) {
                    sNet.checks++;
                    if (sNet.hash[0][i] != sNet.hash[1][j]) {
                        if (sNet.mismatches++ == 0)
                            PortUiLinkNotice("Link: the games differ (desync); reported in the log",
                                             "Enlace: las partidas difieren (desincronizacion); queda en el registro", NULL);
                        PortLog("netlink: DESYNC at battle frame %u: %08x here, %08x there", (unsigned)at,
                                (unsigned)sNet.hash[0][i], (unsigned)sNet.hash[1][j]);
                    } else if (sNet.checks % 10 == 0) {
                        PortLog("netlink: %u state checks, %u desync", (unsigned)sNet.checks, (unsigned)sNet.mismatches);
                    }
                    sNet.hash[0][i] = sNet.hash[1][j] = 0;
                    return;
                }
}

static int NetActive(void) {
    return PortNetlinkRole() != 0;
}

static void NetPut16(u8* p, u16 v) {
    p[0] = (u8)v;
    p[1] = (u8)(v >> 8);
}
static u16 NetGet16(const u8* p) {
    return (u16)(p[0] | p[1] << 8);
}
static void NetPut32(u8* p, u32 v) {
    NetPut16(p, (u16)v);
    NetPut16(p + 2, (u16)(v >> 16));
}
static u32 NetGet32(const u8* p) {
    return NetGet16(p) | (u32)NetGet16(p + 2) << 16;
}

/* HELLO: role, version, our clock and the other's last one echoed (the round trip); START: role, the delay */
static void NetSendControl(u8 type) {
    u8 d[11] = { type, (u8)PortNetlinkRole(), NET_VERSION };
    if (type == NET_START)
        d[2] = sNet.delay;
    NetPut32(d + 3, PortNetlinkMs() | 1);
    NetPut32(d + 7, sNetPeerTicks);
    PortNetlinkSend(d, type == NET_HELLO ? 11 : 3);
}

/* ours from the oldest the other side lacks: type, first slot, count, what we have of theirs, the delay, then 9 bytes
   a slot */
static void NetSendSlots(void) {
    u8 d[11 + NET_RESEND * 9];
    u32 first = sNet.acked, i, n;
    n = sNet.mine - first; /* from the oldest the other side lacks: after an outage nothing is skipped */
    if (n > NET_RESEND)
        n = NET_RESEND;
    d[0] = NET_SLOTS;
    NetPut32(d + 1, first);
    d[5] = (u8)n;
    NetPut32(d + 6, sNet.theirs);
    d[10] = sNet.delay;
    for (i = 0; i < n; i++) {
        const NetSlot* s = &sNet.my[(first + i) % NET_RING];
        u8* p = d + 11 + i * 9;
        NetPut16(p, s->words[0]);
        NetPut16(p + 2, s->words[1]);
        NetPut16(p + 4, s->words[2]);
        NetPut16(p + 6, s->words[3]);
        p[8] = s->pause;
    }
    PortNetlinkSend(d, (int)(11 + n * 9));
}

/* every datagram waiting: their slots into the ring; START (or their first slots) moves a waiting child on */
static void NetReceive(void) {
    u8 d[PORT_NETLINK_MAX];
    int n;
    while ((n = PortNetlinkRecv(d, sizeof(d))) > 0) {
        if (d[0] == NET_HELLO) {
            sNetPeerVersion = n >= 11 ? d[2] : 1;
            if (n >= 11) {
                u32 echo = NetGet32(d + 7);
                sNetPeerTicks = NetGet32(d + 3);
                if (echo) {
                    u32 rtt = (PortNetlinkMs() | 1) - echo;
                    if (rtt < 5000 && (sNetRttMin == 0 || rtt < sNetRttMin))
                        sNetRttMin = rtt ? rtt : 1;
                }
            }
            if (sNetPeerVersion != NET_VERSION && !sNetVersionWarned) {
                sNetVersionWarned = 1;
                PortLog("netlink: the other game plays link version %d, this one %d: update both", sNetPeerVersion,
                        NET_VERSION);
                PortUiLinkNotice("Link: the other player's ReKHCoM is a different version",
                                 "Enlace: el ReKHCoM del otro jugador es de otra version", NULL);
            }
            continue;
        }
        if (d[0] == NET_HASH && n >= 9) {
            u32 at = NetGet32(d + 1), slot = (at / NET_HASH_EVERY) % 8;
            sNet.hashAt[1][slot] = at;
            sNet.hash[1][slot] = NetGet32(d + 5);
            NetHashCompare(at);
            continue;
        }
        if (((d[0] == NET_START && n >= 3) || (d[0] == NET_SLOTS && n >= 11)) && gSioWork.state == SIO_STATE_HANDSHAKE &&
            !gSioWork.isParent && sNetPeerVersion == NET_VERSION) {
            gSioWork.state = SIO_STATE_CONNECTED;
            gSioWork.playerCount = 2; /* the parent was heard: two on the cable */
            gSioWork.handshake = 1;
            CpuFill32(0, &sNet, sizeof(sNet));
            sNet.delay = d[0] == NET_START ? d[2] : d[10];
            PortLog("netlink: connected (player 2), %d frames of delay", sNet.delay);
        }
        if (d[0] == NET_SLOTS && n >= 11 && gSioWork.state == SIO_STATE_CONNECTED) {
            u32 first = NetGet32(d + 1), ack = NetGet32(d + 6), i, count = d[5];
            if (ack > sNet.acked && ack <= sNet.mine)
                sNet.acked = ack;
            for (i = 0; i < count && 11 + (int)(i + 1) * 9 <= n; i++) {
                u32 slot = first + i;
                const u8* p = d + 11 + i * 9;
                NetSlot* s;
                if (slot < sNet.paired || slot >= sNet.paired + NET_RING)
                    continue;
                s = &sNet.peer[slot % NET_RING];
                s->words[0] = NetGet16(p);
                s->words[1] = NetGet16(p + 2);
                s->words[2] = NetGet16(p + 4);
                s->words[3] = NetGet16(p + 6);
                s->pause = p[8];
                sNet.peerKnown[slot % NET_RING] = 1;
            }
            while (sNet.theirs < sNet.paired + NET_RING && sNet.peerKnown[sNet.theirs % NET_RING] &&
                   sNet.theirs >= sNet.paired)
                sNet.theirs++;
        }
    }
}

/* waiting for the other player inside a link frame: our slots out again, the picture and the window alive */
static void NetWait(void) {
    static u32 noticed;
    u32 now = PortNetlinkMs();
    if (!sNet.waitStart)
        sNet.waitStart = now | 1;
    else if (now - sNet.waitStart > 1000 && now - noticed > 500) {
        noticed = now;
        PortUiLinkWaitingNotice((int)((now - sNet.waitStart) / 1000));
    }
    NetSendSlots();
    PortLinkIdle();
    NetReceive();
}

/* the frame's slot: one packet of the game's send queue; the pair of `delay` link frames ago into the receive
   queue, waited for if it has not arrived */
static void NetSlotsFrame(u16* sendFrame, u16 (*recvFrame)[2]) {
    u32 i;
    int waited = 0;
    /* the network never cuts the link: while the other player is away the game waits (its packets stop being
       taken once the queue is nearly full, instead of the cable's "queue full" error), no timeout is reported */
    if (!gSioWork.paused && gSioWork.sendCount < 28)
        SioQueueSendFrame(sendFrame);
    NetReceive();
    while (sNet.mine - sNet.acked >= NET_RING && gSioWork.state == SIO_STATE_CONNECTED) { /* our ring full */
        NetWait();
        waited = 1;
    }
    {
        NetSlot* s = &sNet.my[sNet.mine % NET_RING];
        if (gSioWork.sendCount > 0) {
            for (i = 0; i < 4; i++)
                s->words[i] = gSioWork.sendBuf[i][gSioWork.sendReadIdx];
            gSioWork.sendCount--;
            if (++gSioWork.sendReadIdx > 31)
                gSioWork.sendReadIdx = 0;
        } else {
            CpuFill16(0, s->words, sizeof(s->words));
        }
        /* the cable's flow control (a pause request past 3 waiting packets) is not used: the lockstep already
           keeps both sides in step */
        s->pause = 0;
        sNet.mine++;
    }
    NetSendSlots();
    if (sNet.mine > sNet.delay) {
        u32 want = sNet.mine - 1 - sNet.delay;
        while (!sNet.peerKnown[want % NET_RING] && gSioWork.state == SIO_STATE_CONNECTED) {
            NetWait();
            waited = 1;
        }
        if (sNet.peerKnown[want % NET_RING]) {
            const NetSlot* mine = &sNet.my[want % NET_RING];
            NetSlot* theirs = &sNet.peer[want % NET_RING];
            const NetSlot* by[2];
            u16 nonzero = 0;
            by[gSioWork.playerId] = mine;
            by[gSioWork.playerId ^ 1] = theirs;
            for (i = 0; i < 4; i++)
                nonzero |= by[0]->words[i] | by[1]->words[i];
            if (nonzero && PortNetlinkTrace())
                PortLog("netlink: pair %u  %04x %04x %04x %04x | %04x %04x %04x %04x", (unsigned)want,
                        by[0]->words[0], by[0]->words[1], by[0]->words[2], by[0]->words[3], by[1]->words[0],
                        by[1]->words[1], by[1]->words[2], by[1]->words[3]);
            if (nonzero && gSioWork.recvCount < 32) {
                u8 idx = gSioWork.recvReadIdx + gSioWork.recvCount;
                if (idx > 31)
                    idx -= 32;
                for (i = 0; i < 4; i++) {
                    gSioWork.recvBuf[0][i][idx] = by[0]->words[i];
                    gSioWork.recvBuf[1][i][idx] = by[1]->words[i];
                }
                gSioWork.recvCount++;
            }
            sNet.peerKnown[want % NET_RING] = 0;
            sNet.paired = want + 1;
        }
    }
    sNet.waitStart = 0;
    if (sNet.mine % 600 == 0)
        PortLog("netlink: slot %u, %u paired, %u packets waiting", (unsigned)sNet.mine, (unsigned)sNet.paired,
                (unsigned)gSioWork.recvCount);
    gSioWork.paused = sNet.pauseFrames > 0;
    if (sNet.pauseFrames > 0)
        sNet.pauseFrames--;
    if (sNet.startFrames > 0) {
        sNet.startFrames--;
        NetSendControl(NET_START);
    }
    gSioWork.queueFull = 0;
    gSioWork.timeout = 0;
    gSioWork.hardwareError = 0;
    gSioWork.checksumError = 0;
    SioReadRecvFrame(recvFrame);
    if ((gSystemFlags & SYSTEM_FLAG_LINK_ACTIVE) && !gSioWork.recvEmpty) {
        /* the game runs one battle frame with this group: every NET_HASH_EVERY of them, the state before it */
        extern u32 VsBattleStateHash(void);
        if (sNet.groups % NET_HASH_EVERY == 0) {
            u32 h = VsBattleStateHash(), slot = (sNet.groups / NET_HASH_EVERY) % 8;
            if (h && PortNetlinkFakeDesync() && sNet.groups >= 1200) /* tests: the check must catch a difference */
                h ^= 1;
            if (h) {
                u8 d[9];
                sNet.hashAt[0][slot] = sNet.groups;
                sNet.hash[0][slot] = h;
                d[0] = NET_HASH;
                NetPut32(d + 1, sNet.groups);
                NetPut32(d + 5, h);
                PortNetlinkSend(d, 9);
                PortNetlinkSend(d, 9); /* twice: a lost check is only a skipped check */
                NetHashCompare(sNet.groups);
            }
        }
        sNet.groups++;
    }
    if (gSystemFlags & SYSTEM_FLAG_LINK_ACTIVE) { /* the battle: how often a frame had to wait for the other player */
        sNet.frames++;
        sNet.waited += waited;
        if (sNet.frames % 600 == 0)
            PortLog("netlink: battle %u frames, %u waited, %d frames of delay", (unsigned)sNet.frames,
                    (unsigned)sNet.waited, sNet.delay);
    }
    if (PortNetlinkTrace() && !gSioWork.recvEmpty)
        PortLog("netlink: read %04x %04x | %04x %04x", recvFrame[0][0], recvFrame[1][0], recvFrame[0][1], recvFrame[1][1]);
}

/* the delay the parent picks: the other player's packets must be in hand `delay` link frames after they were made,
   so one way of the round trip plus a margin for jitter, in frames of 16.7 ms (KHCOM_NETLINK_DELAY: fixed, tests) */
static u8 NetPickDelay(void) {
    int d = PortNetlinkDelayOverride();
    if (d <= 0)
        d = ((int)sNetRttMin / 2 + 25 + 16) / 17;
    return (u8)(d < 2 ? 2 : d > 12 ? 12 : d);
}

/* the handshake: the host is the parent (player 1), whoever joins the child; both see two players once the other
   is heard; the parent's start request starts the slots on both sides */
static void NetHandshake(u8* request) {
    int peer;
    gSioWork.isParent = PortNetlinkRole() == PORT_NETLINK_HOST ? 8 : 0;
    gSioWork.playerId = gSioWork.isParent ? 0 : 1;
    NetSendControl(NET_HELLO);
    NetReceive();
    if (gSioWork.state != SIO_STATE_HANDSHAKE)
        return; /* START arrived */
    peer = PortNetlinkSilentMs() < 1000 && sNetPeerVersion == NET_VERSION;
    gSioWork.playerCount = peer ? 2 : 1;
    gSioWork.handshake = peer ? 1 : 0;
    if ((*request == 1 || (sSioAutoStartDone == 0 && peer)) && gSioWork.isParent && peer) {
        if (!sNetRttMin)
            return; /* the round trip is measured in a frame or two: the start waits for it */
        u8 delay = NetPickDelay();
        gSioWork.state = SIO_STATE_CONNECTED;
        CpuFill32(0, &sNet, sizeof(sNet));
        sNet.delay = delay;
        sNet.startFrames = 30;
        sSioAutoStartDone = -1;
        PortLog("netlink: connected (player 1), round trip %u ms, %d frames of delay", (unsigned)sNetRttMin, delay);
    } else if (*request == 1) {
        sSioAutoStartDone = -1;
    }
}
#endif

u16 IsVBlankIntrLive() {
    if (REG_IME & 1) {
        if (REG_DISPSTAT & DISPSTAT_VBLANK_INTR) {
            if (REG_IE & INTR_FLAG_VBLANK) {
                if (!(REG_DISPCNT & DISPCNT_FORCED_BLANK)) {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

void SetVBlankCallback(IntrFunc fn) {
    gVBlankCallback = fn;

    if (!IsVBlankIntrLive()) {
        *gIntrTableVBlank = fn;
    }
}

void ResetVBlankCallback() {
    *gIntrTableVBlank = VBlankIntr;
    gVBlankCallback = VBlankIntr;
}

void SetVCountCallback(IntrFunc fn) {
    gVCountCallback = fn;

    if (!IsVBlankIntrLive()) {
        *gIntrTableVCount = fn;
    }
}

void ResetVCountCallback() {
    *gIntrTableVCount = VCountIntrDummy;
    gVCountCallback = VCountIntrDummy;
}

void SetHBlankCallback(IntrFunc fn) {
    gHBlankCallback = fn;

    if (!IsVBlankIntrLive()) {
        *gIntrTableHBlank = fn;
    }
}

void ResetHBlankCallback() {
    *gIntrTableHBlank = HBlankIntrDummy;
    gHBlankCallback = HBlankIntrDummy;
}

void SetSerialCallback(IntrFunc fn) {
    *gIntrTableSerial = fn;
}

void ResetSerialCallback() {
    *gIntrTableSerial = SerialIntrDummy;
}

void SetTimer3Callback(IntrFunc fn) {
    *gIntrTableTimer3 = fn;
}

void ResetTimer3Callback() {
    *gIntrTableTimer3 = SerialIntrDummy;
}

void SioInit() {
    u16* savedIme;
    u16 ime;

    savedIme = &sSioSavedIme;
    ime = REG_IME;
    REG_IME = 0;
    REG_IE &= ~(INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL);
    REG_IME = ime;
    REG_RCNT = 0;
    REG_SIOCNT = SIO_MULTI_MODE;
    REG_SIOCNT |= (SIO_INTR_ENABLE | SIO_115200_BPS);
    *savedIme = REG_IME;
    SetVBlankCallback(VBlankIntrSio);
    SetSerialCallback(SioSerialIntr);
    SetTimer3Callback(SioTimer3Intr);
    REG_IME = 0;
    REG_IE |= INTR_FLAG_SERIAL;
    REG_IME = *savedIme;
    REG_SIOMLT_SEND = 0;
    *(u64*)REG_ADDR_SIOMULTI0 = 0;
    CpuFill32(0, &gSioWork, sizeof(SioWork));
    sSioIdleVBlanks = 0;
    sSioSendEmpty = FALSE;
    sSioPrevPlayerCount = 0;
    gSioLastSendCount = 0;
    gSioLastRecvCount = 0;
    gSioStatus = 0;
    gSioErrorFrameCount = 0;
    gSioErrorStatus = 0;
    gSioPlayerId = 0;
    gSioPlayerCount = 0;
    gSioHandshake = 0;
    gSioHandshakeRequest = SIO_REQUEST_NONE;
    gSioLinkResult = SIO_LINK_RESULT_NONE;
    sSioAutoStartDone = 0;
    sSioChecksumReady = FALSE;
    sSioSendNonzero = 0;
    sSioRecvNonzero = 0;
    gSioLinkSendCallback = NULL;
    gSioLinkRecvCallback = NULL;
}

void SioReset() {
    SioInit();
    SioStop();
}

void func_08006E70() {
}

void SioStop() {
    sSioSavedIme = REG_IME;
    REG_IME = 0;
    REG_IE &= ~(INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL);
    REG_IME = sSioSavedIme;
    REG_SIOCNT = 0;
    REG_TM3CNT_H = 0;
    REG_IF = (INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL);
    CpuFill32(0, &gSioWork, sizeof(SioWork));
}

u32 SioRunStateMachine(u8* request, u16* sendFrame, u16 (*recvFrame)[2]) {
    u32 playerBits;
    u32 status;
    u32 result;
    u32 recvEmpty, handshake, hardwareError, checksumError, queueFull, timeout;

#ifdef PLATFORM_PC
    PortNetlinkFrame();
    if (NetActive() && gSioWork.state == SIO_STATE_HANDSHAKE && *request != SIO_REQUEST_RESET) {
        NetHandshake(request);
    } else if (NetActive() && (gSioWork.state == SIO_STATE_CONNECTED || gSioWork.state == SIO_STATE_INIT_TIMER)) {
        gSioWork.state = SIO_STATE_CONNECTED;
        NetSlotsFrame(sendFrame, recvFrame);
    } else
#endif
    switch (gSioWork.state) {
    case SIO_STATE_STOP:
        SioStop();
        gSioWork.state = SIO_STATE_INIT;
        break;
    case SIO_STATE_INIT:
        SioInit();
        gSioWork.state = SIO_STATE_HANDSHAKE;
        break;
    case SIO_STATE_HANDSHAKE:
        switch (*request) {
        default:
            SioCheckParent();

            if (sSioAutoStartDone == 0) {
                if (gSioWork.isParent != 0 && gSioWork.playerCount == 2) {
                    gSioWork.startPending = TRUE;
                    sSioAutoStartDone = -1;
                }
            }

            break;
        case SIO_REQUEST_START:
            if (gSioWork.isParent != 0 && gSioWork.playerCount == 2) {
                gSioWork.startPending = TRUE;
            }

            sSioAutoStartDone = -1;
            break;
        case SIO_REQUEST_RESET:
            gSioWork.state = SIO_STATE_STOP;
            REG_SIOMLT_SEND = 0;
            break;
        }

        break;
    case SIO_STATE_INIT_TIMER:
        SioInitTimer();
        gSioWork.state = SIO_STATE_CONNECTED;
    case SIO_STATE_CONNECTED:
        if (!gSioWork.paused) {
            SioQueueSendFrame(sendFrame);
        }

        SioReadRecvFrame(recvFrame);
        break;
    }

    *request = SIO_REQUEST_NONE;
    playerBits = gSioWork.playerId | (gSioWork.playerCount << SIO_STAT_PLAYER_COUNT_SHIFT);

    if (gSioWork.isParent == SIO_ROLE_PARENT) {
        playerBits |= SIO_STAT_PARENT;
    }

    recvEmpty = gSioWork.recvEmpty << SIO_STAT_RECV_EMPTY_SHIFT;
    handshake = gSioWork.handshake << SIO_STAT_HANDSHAKE_SHIFT;
    hardwareError = gSioWork.hardwareError << SIO_STAT_ERROR_HARDWARE_SHIFT;
    checksumError = gSioWork.checksumError << SIO_STAT_ERROR_CHECKSUM_SHIFT;
    queueFull = gSioWork.queueFull << SIO_STAT_ERROR_QUEUE_FULL_SHIFT;
    timeout = gSioWork.timeout << SIO_STAT_ERROR_TIMEOUT_SHIFT;

    if (gSioWork.state == SIO_STATE_CONNECTED) {
        status = playerBits | SIO_STAT_CONNECTED | recvEmpty | handshake | hardwareError | checksumError | queueFull | timeout;
    } else {
        status = playerBits | recvEmpty | handshake | hardwareError | checksumError | queueFull | timeout;
    }

    result = status;

    if (gSioWork.playerId > 1) {
        result |= SIO_STAT_ERROR_INVALID_ID;
    }

    return result;
}

u32 SioTransferFrames(u8* request, u16* sendFrame, u16 (*recvFrame)[2]) {
    u32 playerBits;
    u32 status;
    u32 result;
    u32 recvEmpty, handshake, hardwareError, checksumError, queueFull, timeout;

#ifdef PLATFORM_PC
    PortNetlinkFrame();
    if (NetActive() && gSioWork.state == SIO_STATE_CONNECTED)
        NetSlotsFrame(sendFrame, recvFrame);
    else
#endif
    if (gSioWork.state == SIO_STATE_CONNECTED) {
        if (!gSioWork.paused) {
            SioQueueSendFrame(sendFrame);
        }

        SioReadRecvFrame(recvFrame);
    }

    playerBits = gSioWork.playerId | (gSioWork.playerCount << SIO_STAT_PLAYER_COUNT_SHIFT);

    if (gSioWork.isParent == SIO_ROLE_PARENT) {
        playerBits |= SIO_STAT_PARENT;
    }

    recvEmpty = gSioWork.recvEmpty << SIO_STAT_RECV_EMPTY_SHIFT;
    handshake = gSioWork.handshake << SIO_STAT_HANDSHAKE_SHIFT;
    hardwareError = gSioWork.hardwareError << SIO_STAT_ERROR_HARDWARE_SHIFT;
    checksumError = gSioWork.checksumError << SIO_STAT_ERROR_CHECKSUM_SHIFT;
    queueFull = gSioWork.queueFull << SIO_STAT_ERROR_QUEUE_FULL_SHIFT;
    timeout = gSioWork.timeout << SIO_STAT_ERROR_TIMEOUT_SHIFT;

    if (gSioWork.state == SIO_STATE_CONNECTED) {
        status = playerBits | SIO_STAT_CONNECTED | recvEmpty | handshake | hardwareError | checksumError | queueFull | timeout;
    } else {
        status = playerBits | recvEmpty | handshake | hardwareError | checksumError | queueFull | timeout;
    }

    result = status;

    if (gSioWork.playerId > 1) {
        result |= SIO_STAT_ERROR_INVALID_ID;
    }

    return result;
}

void SioCheckParent() {
    if (((*(vu32*)REG_ADDR_SIOCNT) & (SIO_MULTI_SI | SIO_MULTI_SD)) == SIO_MULTI_SD && gSioWork.playerId == 0) {
        gSioWork.isParent = SIO_ROLE_PARENT;
    } else {
        gSioWork.isParent = SIO_ROLE_CHILD;
    }
}

void SioInitTimer() {
    if (gSioWork.isParent != 0) {
        REG_TM3CNT_L = 0xFF2D;
        REG_TM3CNT_H = (TIMER_INTR_ENABLE | TIMER_64CLK);
        sSioSavedIme = REG_IME;
        REG_IME = 0;
        REG_IE |= INTR_FLAG_TIMER3;
        REG_IME = sSioSavedIme;
    }
}

void SioQueueSendFrame(u16* frame) {
    u8 idx;
    u8 i;

    sSioSavedIme = REG_IME;
    REG_IME = 0;

    if (gSioWork.sendCount < 32) {
        idx = gSioWork.sendCount + gSioWork.sendReadIdx;

        if (idx > 31) {
            idx -= 32;
        }

        for (i = 0; i < ARRAY_COUNT(gSioWork.sendBuf); i++) {
            sSioSendNonzero |= *frame;
            gSioWork.sendBuf[i][idx] = *frame;
            *frame = 0;
            frame++;
        }
    } else {
        gSioWork.queueFull |= SIO_QUEUE_FULL_SEND;
    }

    if (sSioSendNonzero != 0) {
        gSioWork.sendCount++;
        sSioSendNonzero = 0;
    }

    REG_IME = sSioSavedIme;
    gSioLastSendCount = gSioWork.sendCount;
}

void SioReadRecvFrame(u16 (*frame)[2]) {
    u8 i;
    u8 j;

    sSioSavedIme = REG_IME;
    REG_IME = 0;

    if (gSioWork.recvCount == 0) {
        for (i = 0; i < 4; i++) {
            for (j = 0; j < gSioWork.playerCount; j++) {
                frame[i][j] = 0;
            }
        }

        gSioWork.recvEmpty = 1;
    } else {
        for (i = 0; i < ARRAY_COUNT(gSioWork.recvBuf[0]); i++) {
            for (j = 0; j < gSioWork.playerCount; j++) {
                frame[i][j] = gSioWork.recvBuf[j][i][gSioWork.recvReadIdx];
            }
        }

        gSioWork.recvCount--;
        gSioWork.recvReadIdx++;

        if (gSioWork.recvReadIdx > 31) {
            gSioWork.recvReadIdx = 0;
        }

        gSioWork.recvEmpty = 0;
    }

    REG_IME = sSioSavedIme;
}

void SioVBlankUpdate() {
#ifdef PLATFORM_PC
    if (NetActive())
        return; /* no transfers to count: the network's timeout and pause are NetSlotsFrame's */
#endif
    if (gSioWork.paused) {
        sSioPauseTimer--;

        if (sSioPauseTimer != 0) {
            return;
        }

        gSioWork.paused = FALSE;
    }

    if (gSioWork.isParent != 0) {
        if (gSioWork.state == SIO_STATE_HANDSHAKE) {
            SioStartTransfer();
        } else if (gSioWork.state == SIO_STATE_CONNECTED) {
            if (gSioWork.transferCount <= 4) {
                if (gSioWork.hardwareError != 0) {
                    SioStartTransfer();
                } else {
                    gSioWork.timeout = SIO_TIMEOUT_PARENT;
                }
            } else if (gSioWork.timeout == 0) {
                gSioWork.transferCount = 0;
                SioStartTransfer();
            }
        }
    } else if (gSioWork.state == SIO_STATE_CONNECTED || gSioWork.state == SIO_STATE_HANDSHAKE) {
        sSioIdleVBlanks++;

        if (sSioIdleVBlanks > 6) {
            if (gSioWork.state == SIO_STATE_CONNECTED) {
                gSioWork.timeout = SIO_TIMEOUT_CHILD;
            }

            if (gSioWork.state == SIO_STATE_HANDSHAKE) {
                gSioWork.playerId = 0;
                gSioWork.playerCount = 0;
                gSioWork.handshake = 0;
            }
        }
    }
}

void SioTimer3Intr() {
    SioStopTimer();
    SioStartTransfer();
}

void SioSerialIntr() {
    u32 cnt;

    cnt = (*(vu32*)REG_ADDR_SIOCNT);
    gSioWork.playerId = (cnt << 26) >> 30;

    switch (gSioWork.state) {
    case SIO_STATE_CONNECTED:
        if (cnt & SIO_ERROR) {
            gSioWork.hardwareError = 1;
        }

        SioRecvWord();
        SioSendWord();
        SioFinishTransfer();
        break;
    case SIO_STATE_HANDSHAKE:
        if (SioHandshake()) {
            if (gSioWork.isParent != 0) {
                gSioWork.state = SIO_STATE_INIT_TIMER;
                gSioWork.transferCount = 4;
            } else {
                gSioWork.state = SIO_STATE_CONNECTED;
            }
        }

        break;
    }

    gSioWork.transferCount++;
    sSioIdleVBlanks = 0;

    if (gSioWork.transferCount == 4) {
        gSioLastRecvCount = gSioWork.recvCount;
    }
}

void SioStartTransfer() {
    REG_SIOCNT |= SIO_START;
}

u8 SioHandshake() {
    u8 count;
    u16 min;
    u8 i;

    count = 0;
    min = 0xFFFF;

    if (gSioWork.startPending == TRUE) {
        REG_SIOMLT_SEND = SIO_HANDSHAKE_START;
    } else {
        REG_SIOMLT_SEND = SIO_HANDSHAKE_IDLE;
    }

    gSioWork.startPending = FALSE;
    *(u64*)gSioWork.recv = *(u64*)REG_ADDR_SIOMULTI0;

    for (i = 0; i < 2; i++) {
        if ((gSioWork.recv[i] & ~3) == SIO_HANDSHAKE_IDLE || gSioWork.recv[i] == SIO_HANDSHAKE_START) {
            count++;

            if (min > gSioWork.recv[i] && gSioWork.recv[i] != 0) {
                min = gSioWork.recv[i];
            }
        } else if (gSioWork.recv[i] == SIO_MULTI_EMPTY) {
            if (i == gSioWork.playerId) {
                count = 0;
            }
        } else {
            count = 0;
            break;
        }
    }

    gSioWork.playerCount = count;

    if (gSioWork.playerCount == 2) {
        if (gSioWork.playerCount == sSioPrevPlayerCount && gSioWork.recv[0] == SIO_HANDSHAKE_START) {
            return TRUE;
        }

        if (gSioWork.playerCount == 2) {
            gSioWork.handshake = (min & 3) + 1;
        } else {
            gSioWork.handshake = 0;
        }
    } else {
        gSioWork.handshake = 0;
    }

    sSioPrevPlayerCount = gSioWork.playerCount;
    return FALSE;
}

void SioRecvWord() {
    u16 recv[4];
    u8 i;
    u8 idx;

    *(u64*)recv = *(u64*)REG_ADDR_SIOMULTI0;

    if (gSioWork.sendWordIdx == 0) {
        for (i = 0; i < gSioWork.playerCount; i++) {
            if (gSioWork.checksum != recv[i] && sSioChecksumReady) {
                gSioWork.checksumError = 1;
            }
        }

        gSioWork.checksum = 0;
        sSioChecksumReady = TRUE;
    } else {
        idx = gSioWork.recvReadIdx + gSioWork.recvCount;

        if (idx > 31) {
            idx -= 32;
        }

        if (gSioWork.recvCount < 32) {
            for (i = 0; i < gSioWork.playerCount; i++) {
                gSioWork.checksum += recv[i];
                sSioRecvNonzero |= recv[i];
                gSioWork.recvBuf[i][gSioWork.recvWordIdx][idx] = recv[i];

                if (gSioWork.sendWordIdx == 1 && !gSioWork.paused && (recv[i] & SIO_PAUSE_REQUEST)) {
                    gSioWork.paused = TRUE;
                    sSioPauseTimer = 5;
                }
            }
        } else {
            gSioWork.queueFull |= SIO_QUEUE_FULL_RECV;
        }

        gSioWork.recvWordIdx++;

        if (gSioWork.recvWordIdx == 4 && sSioRecvNonzero != 0) {
            gSioWork.recvCount++;
            sSioRecvNonzero = 0;
        }
    }
}

void SioSendWord() {
    if (gSioWork.sendWordIdx == 4) {
        REG_SIOMLT_SEND = gSioWork.checksum;

        if (!sSioSendEmpty) {
            gSioWork.sendCount--;
            gSioWork.sendReadIdx++;

            if (gSioWork.sendReadIdx > 31) {
                gSioWork.sendReadIdx = 0;
            }
        } else {
            sSioSendEmpty = FALSE;
        }
    } else {
        if (gSioWork.sendWordIdx == 0 && gSioWork.sendCount == 0) {
            sSioSendEmpty = TRUE;
        }

        if (sSioSendEmpty) {
            REG_SIOMLT_SEND = 0;
        } else {
            REG_SIOMLT_SEND = gSioWork.sendBuf[gSioWork.sendWordIdx][gSioWork.sendReadIdx];
        }

        if (!gSioWork.paused && gSioWork.sendWordIdx == 0 && gSioWork.recvCount > 3) {
            REG_SIOMLT_SEND |= SIO_PAUSE_REQUEST;
        }

        gSioWork.sendWordIdx++;
    }
}

void SioStopTimer() {
    if (gSioWork.isParent != 0) {
        REG_TM3CNT_H &= ~TIMER_ENABLE;
        REG_TM3CNT_L = 0xFF2D;
    }
}

void SioFinishTransfer() {
    if (gSioWork.recvWordIdx == 4) {
        gSioWork.sendWordIdx = 0;
        gSioWork.recvWordIdx = 0;
    } else if (gSioWork.isParent != 0) {
        REG_TM3CNT_H |= TIMER_ENABLE;
    }
}

void SioResetSendQueue() {
    u8 i;
    u8 j;

    gSioWork.sendCount = 0;
    gSioWork.sendReadIdx = 0;

    for (i = 0; i < ARRAY_COUNT(gSioWork.sendBuf); i++) {
        for (j = 0; j < ARRAY_COUNT(gSioWork.sendBuf[0]); j++) {
            gSioWork.sendBuf[i][j] = 0xEFFF;
        }
    }
}

void SioResetRecvQueue() {
    u8 i;
    u8 j;
    u8 k;

    gSioWork.recvCount = 0;
    gSioWork.recvReadIdx = 0;

    for (i = 0; i < ARRAY_COUNT(gSioWork.recvBuf); i++) {
        for (j = 0; j < ARRAY_COUNT(gSioWork.recvBuf[0]); j++) {
            for (k = 0; k < ARRAY_COUNT(gSioWork.recvBuf[0][0]); k++) {
                gSioWork.recvBuf[i][j][k] = 0xEFFF;
            }
        }
    }
}

void SioClearRegs() {
    REG_RCNT = 0;
    REG_SIOCNT = 0;
    REG_SIODATA8 = 0;
    *(vu16*)REG_ADDR_SIODATA32 = 0;
    REG_SIOMLT_SEND = 0;
    REG_SIOMLT_RECV = 0;
    REG_SIOMULTI0 = 0;
    REG_SIOMULTI1 = 0;
    REG_SIOMULTI2 = 0;
    REG_SIOMULTI3 = 0;
}

void SioShutdown() {
    SioClearRegs();
    REG_IME = 0;
    ResetVBlankCallback();
    ResetSerialCallback();
    ResetTimer3Callback();
    REG_IE = (INTR_FLAG_VBLANK | INTR_FLAG_GAMEPAK);
    REG_DISPSTAT = DISPSTAT_VBLANK_INTR;
    REG_IME = 1;
    SioStop();
}

u8 SioIsConnected() {
    if (gSioWork.state == SIO_STATE_CONNECTED) {
        return TRUE;
    }

    return FALSE;
}
