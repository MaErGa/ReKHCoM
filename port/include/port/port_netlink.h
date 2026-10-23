/* The link cable over the network (alpha): port/src/port_netlink.c, used by src/sio/sio.c. */
#ifndef PORT_NETLINK_H
#define PORT_NETLINK_H
#include <stdint.h>

#define PORT_NETLINK_HOST 1 /* the cable's parent (player 1) */
#define PORT_NETLINK_JOIN 2 /* the child (player 2) */
#define PORT_NETLINK_MAX 512

int PortNetlinkRole(void); /* 0: no network link (the cartridge's cable, as always) */
void PortNetlinkSend(const uint8_t* data, int size);
int PortNetlinkRecv(uint8_t* data, int cap); /* one datagram from the other player, 0 when none */
void PortNetlinkFrame(void);                 /* once a game frame (the test lag) */
int PortNetlinkSilentMs(void);               /* since the other player was last heard */
int PortNetlinkTrace(void);                  /* KHCOM_NETLINK_TRACE: every packet pair in the log */


/* port_steamlink.c: the same datagrams through a Steam room and Steam's P2P messages (Windows) */
int PortSteamlinkStart(int role);
void PortSteamlinkFrame(void);
int PortSteamlinkHasPeer(void);
void PortSteamlinkSend(const uint8_t* data, int size);
int PortSteamlinkRecv(uint8_t* data, int cap);
const char* PortSteamlinkStatus(void); /* what the Steam link is doing, for the screen */

#endif
