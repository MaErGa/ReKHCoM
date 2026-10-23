#!/usr/bin/env python3
"""The link battle's relay (port/progress/ONLINE_ESTUDIO.md): for a VPS, so two players on any platform (PC, Android,
Switch... anything with UDP) meet by a room code without opening ports. Standard library only.

    python3 netlink_relay.py [--port 47800]

Datagrams (the game's port_netlink.c, KHCOM_NETLINK / netlink.cfg = relay:<server>:<port>:<code>:host|join):
    'J' role code      a player in a room (role 1 host, 2 join; code up to 16 characters). Sent again every second:
                       a player whose address changed (Wi-Fi to mobile data, a new router) is found again.
    'O' n              the relay's answer: n players in the room.
    'D' payload        the game's datagram, passed to the other player of the room as it came.
A room nobody has written to for 10 minutes is forgotten. Nothing is stored or read beyond the room's two addresses.
"""
import argparse
import asyncio
import time

IDLE = 600


class Relay(asyncio.DatagramProtocol):
    def __init__(self):
        self.rooms = {}  # code -> {role: (address, last time)}
        self.where = {}  # address -> (code, role)

    def connection_made(self, transport):
        self.transport = transport

    def datagram_received(self, data, addr):
        now = time.time()
        if not data:
            return
        kind = data[:1]
        if kind == b'J' and len(data) >= 3:
            role, code = data[1], data[2:18].decode('ascii', 'replace')
            if role not in (1, 2):
                return
            room = self.rooms.setdefault(code, {})
            old = room.get(role)
            if old and old[0] != addr:
                self.where.pop(old[0], None)
            room[role] = (addr, now)
            self.where[addr] = (code, role)
            self.transport.sendto(b'O' + bytes([len(room)]), addr)
        elif kind == b'D':
            place = self.where.get(addr)
            if not place:
                return
            code, role = place
            room = self.rooms.get(code, {})
            room[role] = (addr, now)
            other = room.get(3 - role)
            if other:
                self.transport.sendto(data, other[0])

    def sweep(self):
        now = time.time()
        for code in [c for c, r in self.rooms.items() if all(now - t > IDLE for _, t in r.values())]:
            for addr, _ in self.rooms[code].values():
                self.where.pop(addr, None)
            del self.rooms[code]


async def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--port', type=int, default=47800)
    args = ap.parse_args()
    loop = asyncio.get_running_loop()
    _, relay = await loop.create_datagram_endpoint(Relay, local_addr=('0.0.0.0', args.port))
    print(f'netlink relay on UDP {args.port}', flush=True)
    while True:
        await asyncio.sleep(60)
        relay.sweep()


if __name__ == '__main__':
    asyncio.run(main())
