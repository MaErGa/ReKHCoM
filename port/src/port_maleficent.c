/* Maleficent's dragon arena in the wide view (Preliminar V2 of the boss arenas, waits for the user's approval).
 * BATTLE_SCENERY_AUDIT.md #5: the hall ends in cuts and the dragon's body shows again beside the picture.
 *
 * The hall (gBosMdBgMap0..3, a 512x512 BG) is drawn once, x 64..447 and y 120..511 (its stone curb goes on in the
 * map's top 16 rows); the rest of the map is empty. Its bays repeat every 72 pixels (an arch, a pillar with its
 * gargoyle), so beyond the drawn part each row goes on with a whole bay of itself; above it the plain night sky, below
 * it the stone curb. The dragon (gBosMdFrame*Map, the same BG's last block) is one creature: outside the picture only
 * the copy nearest the picture is drawn. Source tiles stay in the user's ROM; palette indices resolved live. */
#include "port/port.h"
#include "port/port_battle_backdrop.h"

extern const uint16_t gBosMdBgMap0[], gBosMdBgMap1[], gBosMdBgMap2[], gBosMdBgMap3[];
extern const uint8_t gBosMdBgTiles[];
extern const uint16_t gDefaultBgMap[];

#define HALL_LEFT 64
#define HALL_RIGHT 448
#define HALL_TOP 120
#define BAY 72
#define CURB_TOP 496 /* the curb's 32 rows: 496..511 and the map's 0..15 */

static int Repeat(int position, int start, int width) {
    int offset = (position - start) % width;
    return start + (offset < 0 ? offset + width : offset);
}

static int FloorDiv(int a, int b) {
    return a >= 0 ? a / b : -((-a + b - 1) / b);
}

static int SourceIndex(int x, int y) {
    static const uint16_t* const maps[4] = { gBosMdBgMap0, gBosMdBgMap1, gBosMdBgMap2, gBosMdBgMap3 };
    const uint16_t* block;
    unsigned entry;
    int px, py;
    x &= 511;
    y &= 511;
    block = maps[(y >> 8) * 2 + (x >> 8)];
    entry = block[((y & 255) >> 3) * 32 + ((x & 255) >> 3)];
    px = x & 7;
    py = y & 7;
    if (entry & 0x400) px = 7 - px;
    if (entry & 0x800) py = 7 - py;
    px = (gBosMdBgTiles[(entry & 1023) * 32 + py * 4 + px / 2] >> ((px & 1) * 4)) & 15;
    return px ? (int)(entry >> 12) * 16 + px : 0;
}

/* 1 the hall, 2 the dragon, 0 neither */
int PortMaleficentLayer(const void* const* maps) {
    if (!maps)
        return 0;
    if (maps[0] == gBosMdBgMap0 && maps[1] == gBosMdBgMap1 && maps[2] == gBosMdBgMap2 && maps[3] == gBosMdBgMap3)
        return 1;
    return 0;
}

/* the dragon's layer: the three empty blocks and one of its frames (called only while the hall is on screen) */
int PortMaleficentDragonLayer(const void* const* maps) {
    return maps && maps[0] == gDefaultBgMap && maps[1] == gDefaultBgMap && maps[2] == gDefaultBgMap &&
           maps[3] != gDefaultBgMap;
}

/* x, y: the layer's world position; cx, cy: the world position of the picture's middle (its copy is the one drawn) */
int PortMaleficentHallPixel(int x, int y, int cx, int cy, uint16_t* color) {
    int mx = x - FloorDiv(cx, 512) * 512, my = y - FloorDiv(cy, 512) * 512, sx, sy, index;
    int inX = mx >= HALL_LEFT && mx < HALL_RIGHT, inY = my >= HALL_TOP && my < 512;
    if (inX && inY)
        return 0; /* the hall as the cartridge draws it */
    sx = inX ? mx : mx < HALL_LEFT ? Repeat(mx, HALL_LEFT + 12, BAY) : Repeat(mx, HALL_RIGHT - 12 - BAY, BAY);
    if (my < HALL_TOP)
        sy = HALL_TOP + 8; /* the plain sky */
    else if (my >= CURB_TOP)
        sy = CURB_TOP + Repeat(my, CURB_TOP, 32) - CURB_TOP; /* the curb, its 32 rows again (the last 16 wrap to 0) */
    else
        sy = my;
    index = SourceIndex(sx, sy);
    *color = index ? ((const uint16_t*)(uintptr_t)GBA_PLTT)[index] & 0x7FFF : 0x8000;
    return 1;
}

/* the dragon: only its copy nearest the picture (no second dragon beside it) */
int PortMaleficentDragonVisible(int x, int y, int cx, int cy) {
    return FloorDiv(x - cx + 256, 512) == 0 && FloorDiv(y - cy + 256, 512) == 0;
}

/* Atlas42f2b038/608f0f55/71dbfe50: only the missing body, all41 animation slots. */
extern const uint16_t gBosMdFrame0Map[];
extern const uint16_t gBosMdFrame10Map[];
extern const uint16_t gBosMdFrame11Map[];
extern const uint16_t gBosMdFrame12Map[];
extern const uint16_t gBosMdFrame13Map[];
extern const uint16_t gBosMdFrame14Map[];
extern const uint16_t gBosMdFrame18Map[];
extern const uint16_t gBosMdFrame19Map[];
extern const uint16_t gBosMdFrame20Map[];
extern const uint16_t gBosMdFrame21Map[];
extern const uint16_t gBosMdFrame22Map[];
extern const uint16_t gBosMdFrame24Map[];
extern const uint16_t gBosMdFrame26Map[];
extern const uint16_t gBosMdFrame2Map[];
extern const uint16_t gBosMdFrame32Map[];
extern const uint16_t gBosMdFrame34Map[];
extern const uint16_t gBosMdFrame35Map[];
extern const uint16_t gBosMdFrame36Map[];
extern const uint16_t gBosMdFrame37Map[];
extern const uint16_t gBosMdFrame38Map[];
extern const uint16_t gBosMdFrame3Map[];
extern const uint16_t gBosMdFrame4Map[];
extern const uint16_t gBosMdFrame5Map[];
extern const uint16_t gBosMdFrame6Map[];
extern const uint16_t gBosMdFrame7Map[];
extern const uint16_t gBosMdFrame8Map[];
extern const uint16_t gBosMdFrame9Map[];
#include "port/maleficent_body_approved.inc"

int PortMaleficentDragonPixel(const void* map, int x, int y, int cx, int cy, uint16_t* color) {
    const uint8_t* pixels = NULL;
    int x0=0,y0=0,w=0,h=0,n,index;
    x -= 256 + FloorDiv(cx - 128,512)*512;
    y -= 256 + FloorDiv(cy - 128,512)*512;
    if (x < 256) return 0;
    if (map == gBosMdFrame0Map || map == gBosMdFrame7Map || map == gBosMdFrame9Map || map == gBosMdFrame10Map || map == gBosMdFrame11Map || map == gBosMdFrame12Map || map == gBosMdFrame18Map || map == gBosMdFrame22Map || map == gBosMdFrame24Map || map == gBosMdFrame26Map || map == gBosMdFrame32Map || map == gBosMdFrame34Map || map == gBosMdFrame38Map) { pixels=kMdCompletion0;x0=256;y0=50;w=162;h=169; }
    else if (map == gBosMdFrame2Map || map == gBosMdFrame5Map || map == gBosMdFrame6Map || map == gBosMdFrame8Map || map == gBosMdFrame13Map || map == gBosMdFrame20Map || map == gBosMdFrame21Map || map == gBosMdFrame35Map) { pixels=kMdCompletion1;x0=256;y0=48;w=162;h=172; }
    else if (map == gBosMdFrame3Map || map == gBosMdFrame4Map || map == gBosMdFrame14Map || map == gBosMdFrame19Map || map == gBosMdFrame36Map || map == gBosMdFrame37Map) { pixels=kMdCompletion2;x0=256;y0=50;w=162;h=166; }
    if (!pixels || x<x0 || x>=x0+w || y<y0 || y>=y0+h) return 0;
    n=(y-y0)*w+x-x0;index=(pixels[n/2]>>((n&1)*4))&15;
    if (!index) return 0;
    *color=((const uint16_t*)(uintptr_t)GBA_PLTT)[index]&0x7fff;
    return 1;
}
