#ifndef GUARD_PORT_BATTLE_BACKDROP_H
#define GUARD_PORT_BATTLE_BACKDROP_H

#include <stdint.h>

/* Presentation-only continuation of Guard Armor's authored patio.
 * Called only outside the hardware picture. No game/collision map changes. */
int PortGuardArmorBackdropMatches(const void* const* maps);
int PortGuardArmorBackdropPixel(int x, int y, uint16_t* color);

/* Trickmaster's room (port_trickmaster.c): its chessboard goes on below the map, in its perspective */
int PortTrickmasterBackdropMatches(const void* const* maps);
int PortTrickmasterBackdropPixel(int x, int y, uint16_t* color);

/* Maleficent's dragon arena (port_maleficent.c): the hall goes on by its bays, the dragon drawn once */
int PortMaleficentLayer(const void* const* maps);
int PortMaleficentDragonLayer(const void* const* maps);
int PortMaleficentHallPixel(int x, int y, int cx, int cy, uint16_t* color);
int PortMaleficentDragonVisible(int x, int y, int cx, int cy);

/* the boss arenas' Preliminar V2 (port_ppu.c): on in the review copy only (KHCOM_ARENAS_V2=1) until approved */
int PortArenasV2(void);

/* Ursula's cave (port_ursula.c): 1 the cave (its floor goes on below), 2 her own layer (drawn once) */
int PortUrsulaLayer(const void* const* maps);
int PortUrsulaFloorPixel(int x, int y, int cy, uint16_t* color);

/* Darkside's beach (port_darkside.c): 1 the beach layer, 2 Darkside's body, 0 neither. */
int PortDarksideLayer(const void* const* maps);
/* cx, cy: the map tile at the middle of the original picture (the body's single copy is the one nearest it) */
int PortDarksideTileSource(int layer, int* tx, int* ty, int cx, int cy);
int PortDarksideShorePixel(int x, int y, uint16_t* color);
int PortDarksideBodyPixel(const void* const* maps, int x, int y, int cx, int cy, uint16_t* color);

#endif
