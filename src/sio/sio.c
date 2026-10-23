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
enum { NET_HELLO = 1, NET_START, NET_SLOTS };
typedef struct {
    u16 words[4];
    u8 pause;
} NetSlot;
static struct {
    u32 mine;   /* slots of ours made */
    u32 paired; /* slots given to the game */
    u32 acked;  /* ours the other side has */
    u32 theirs; /* theirs we have, all of them below */
    NetSlot my[NET_RING], peer[NET_RING];
    u8 peerKnown[NET_RING];
    u8 pauseFrames;
    u8 startFrames; /* the parent repeats START this many frames */
    /* the battle's smoothing: after a frame without the other player's packet, the next pairs are held until
       `target` of them are in hand, so an uneven connection makes few short waits instead of many hitches;
       target grows with repeated waits and shrinks after a calm while (the only cost: that many frames of delay) */
    u8 target, refill;
    u32 frames, waited, lastWait, calm;
} sNet;

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

static void NetSendControl(u8 type) {
    u8 d[2] = { type, (u8)PortNetlinkRole() };
    PortNetlinkSend(d, 2);
}

/* ours from the oldest the other side lacks: type, first slot, count, what we have of theirs, then 9 bytes a slot */
static void NetSendSlots(void) {
    u8 d[10 + NET_RESEND * 9];
    u32 first = sNet.acked, i, n;
    n = sNet.mine - first; /* from the oldest the other side lacks: after an outage nothing is skipped */
    if (n > NET_RESEND)
        n = NET_RESEND;
    d[0] = NET_SLOTS;
    NetPut32(d + 1, first);
    d[5] = (u8)n;
    NetPut32(d + 6, sNet.theirs);
    for (i = 0; i < n; i++) {
        const NetSlot* s = &sNet.my[(first + i) % NET_RING];
        u8* p = d + 10 + i * 9;
        NetPut16(p, s->words[0]);
        NetPut16(p + 2, s->words[1]);
        NetPut16(p + 4, s->words[2]);
        NetPut16(p + 6, s->words[3]);
        p[8] = s->pause;
    }
    PortNetlinkSend(d, (int)(10 + n * 9));
}

/* every datagram waiting: their slots into the ring; START moves a waiting child on */
static void NetReceive(void) {
    u8 d[PORT_NETLINK_MAX];
    int n;
    while ((n = PortNetlinkRecv(d, sizeof(d))) > 0) {
        if (d[0] == NET_HELLO)
            continue;
        if ((d[0] == NET_START || d[0] == NET_SLOTS) && gSioWork.state == SIO_STATE_HANDSHAKE && !gSioWork.isParent) {
            gSioWork.state = SIO_STATE_CONNECTED;
            gSioWork.playerCount = 2; /* the parent was heard: two on the cable */
            gSioWork.handshake = 1;
            CpuFill32(0, &sNet, sizeof(sNet));
            PortLog("netlink: connected (player 2)");
        }
        if (d[0] == NET_SLOTS && n >= 10 && gSioWork.state == SIO_STATE_CONNECTED) {
            u32 first = NetGet32(d + 1), ack = NetGet32(d + 6), i, count = d[5];
            if (ack > sNet.acked && ack <= sNet.mine)
                sNet.acked = ack;
            for (i = 0; i < count && 10 + (int)(i + 1) * 9 <= n; i++) {
                u32 slot = first + i;
                const u8* p = d + 10 + i * 9;
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

/* the frame's slot: one packet of the game's send queue, the pairs both sides have into the receive queue */
static void NetSlotsFrame(u16* sendFrame, u16 (*recvFrame)[2]) {
    u32 i;
    /* the network never cuts the link: while the other player is away the game waits (its packets stop being
       taken once the queue is nearly full, instead of the cable's "queue full" error), no timeout is reported */
    if (!gSioWork.paused && gSioWork.sendCount < 28)
        SioQueueSendFrame(sendFrame);
    NetReceive();
    if (sNet.mine - sNet.paired < NET_RING && sNet.mine - sNet.acked < NET_RING) {
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
        /* the cable's flow control (a pause request past 3 waiting packets) is not used: the slots already keep
           both sides in step, and a pause would send empty packets that the lockstep battle reads as missing */
        s->pause = 0;
        sNet.mine++;
    }
    {
        u32 ready = (sNet.theirs < sNet.mine ? sNet.theirs : sNet.mine) - sNet.paired;
        if (sNet.target == 0)
            sNet.target = 2;
        if (sNet.refill && gSioWork.recvCount == 0 && ready < sNet.target && (gSystemFlags & SYSTEM_FLAG_LINK_ACTIVE))
            goto held;
        sNet.refill = 0;
    }
    while (sNet.paired < sNet.mine && sNet.peerKnown[sNet.paired % NET_RING] && gSioWork.recvCount < 30) {
        const NetSlot* mine = &sNet.my[sNet.paired % NET_RING];
        NetSlot* theirs = &sNet.peer[sNet.paired % NET_RING];
        const NetSlot* by[2];
        u16 nonzero = 0;
        by[gSioWork.playerId] = mine;
        by[gSioWork.playerId ^ 1] = theirs;
        for (i = 0; i < 4; i++)
            nonzero |= by[0]->words[i] | by[1]->words[i];
        if (nonzero && PortNetlinkTrace())
            PortLog("netlink: pair %u  %04x %04x %04x %04x | %04x %04x %04x %04x", (unsigned)sNet.paired,
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
        sNet.peerKnown[sNet.paired % NET_RING] = 0;
        sNet.paired++;
    }
held:
    if (sNet.mine % 600 == 0)
        PortLog("netlink: slot %u, %u paired, %u packets waiting", (unsigned)sNet.mine, (unsigned)sNet.paired,
                (unsigned)gSioWork.recvCount);
    gSioWork.paused = sNet.pauseFrames > 0;
    if (sNet.pauseFrames > 0)
        sNet.pauseFrames--;
    NetSendSlots();
    if (sNet.startFrames > 0) {
        sNet.startFrames--;
        NetSendControl(NET_START);
    }
    gSioWork.queueFull = 0;
    gSioWork.timeout = 0;
    gSioWork.hardwareError = 0;
    gSioWork.checksumError = 0;
    {
        int silent = PortNetlinkSilentMs();
        static u8 noticeTick;
        if (silent > 1000 && silent < (1 << 29) && ++noticeTick % 30 == 0)
            PortUiLinkWaitingNotice(silent / 1000);
    }
    SioReadRecvFrame(recvFrame);
    if (gSystemFlags & SYSTEM_FLAG_LINK_ACTIVE) { /* the lockstep battle: a frame without a pair is a frame waited */
        sNet.frames++;
        if (gSioWork.recvEmpty) {
            sNet.waited++;
            if (!sNet.refill) {
                sNet.refill = 1;
                if (sNet.frames - sNet.lastWait < 180 && sNet.target < 8) {
                    sNet.target++;
                    PortLog("netlink: uneven connection, buffer %u frames", (unsigned)sNet.target);
                }
                sNet.lastWait = sNet.frames;
            }
            sNet.calm = 0;
        } else if (++sNet.calm > 900 && sNet.target > 2) {
            sNet.target--;
            sNet.calm = 0;
            PortLog("netlink: steady connection, buffer %u frames", (unsigned)sNet.target);
        }
        if (sNet.frames % 600 == 0)
            PortLog("netlink: battle %u frames, %u waited, buffer %u", (unsigned)sNet.frames, (unsigned)sNet.waited,
                    (unsigned)sNet.target);
    }
    if (PortNetlinkTrace() && !gSioWork.recvEmpty)
        PortLog("netlink: read %04x %04x | %04x %04x", recvFrame[0][0], recvFrame[1][0], recvFrame[0][1], recvFrame[1][1]);
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
    peer = PortNetlinkSilentMs() < 1000;
    gSioWork.playerCount = peer ? 2 : 1;
    gSioWork.handshake = peer ? 1 : 0;
    if ((*request == 1 || (sSioAutoStartDone == 0 && peer)) && gSioWork.isParent && peer) {
        gSioWork.state = SIO_STATE_CONNECTED;
        CpuFill32(0, &sNet, sizeof(sNet));
        sNet.startFrames = 30;
        sSioAutoStartDone = -1;
        PortLog("netlink: connected (player 1)");
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
