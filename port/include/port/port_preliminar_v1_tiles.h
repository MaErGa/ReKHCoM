#ifndef PORT_PRELIMINAR_V1_TILES_H
#define PORT_PRELIMINAR_V1_TILES_H
#include <stdint.h>
#include "port_scene_wide_tiles.h"
#include "port_scene053_plateau.inc"

/* Every profile is attached to map content, including decompressed EU maps.
 * Source rectangles never contain copied ROM pixels or fixed RGB colours. */
typedef struct PortPreliminarRecipe {
    uint32_t hash;
    /* scene, layer, block dimensions, occupied dimensions, left/right
     * source bands, slopes, upper/lower rectangles, repeat, ready. */
    int16_t v[22];
} PortPreliminarRecipe;
static const PortPreliminarRecipe kPreliminarRecipes[] = {
#include "port_preliminar_v1_data.inc"
};

static inline const PortPreliminarRecipe* PortPreliminarFind(uint32_t hash, int bg, int w, int h) {
    unsigned i;
    for (i = 0; i < sizeof(kPreliminarRecipes)/sizeof(kPreliminarRecipes[0]); i++) {
        const PortPreliminarRecipe* p = &kPreliminarRecipes[i];
        if (p->hash == hash && p->v[1] == bg && p->v[2] == w && p->v[3] == h && p->v[21]) return p;
    }
    return 0;
}

static inline int PortPreliminarTileSource(const PortPreliminarRecipe* p, int* x, int* y) {
    const int16_t* v = p->v;
    int ox=*x, oy=*y, sx=ox, sy=oy, w=v[4], h=v[5];
    if (v[0]==45) {
        if (v[1]==2) {
            if (ox>=0 && ox<64 && oy>=0 && oy<64) return 1;
            if (oy>=22 && oy<27) { *x=10+PortSceneTileMod(ox-10,4); *y=oy; return 1; }
            return 0;
        }
        if (oy<0) { *x=24; *y=0; return 1; }
        if (oy<27 && (ox<10 || ox>=54)) {
            *x=oy<15?24:10+PortSceneTileMod(ox-10,12); *y=oy; return 1;
        }
        if (ox>=54 && ox<64 && oy>=43 && oy<64) { /* the sunlit corner, shaded as the rest */
            *x=PortSceneTileMod(ox,8); *y=44+PortSceneTileMod(oy-44,4); return 1;
        }
        if (ox>=0 && ox<64 && oy<64) return 1;
        if (oy>=64) {
            sy=62+PortSceneTileMod(oy-62,2); sx=ox+oy-sy;
            if (sx>=0 && sx<64) { *x=sx; *y=sy; return 1; }
        }
        if (oy<43) { *x=0; *y=oy; return 1; }
        /* the dark green in the trees' shade, as inside (columns 0..7 under
         * the wall), not the sunlit corner's (user 24/10) */
        *x=PortSceneTileMod(ox,8); *y=44+PortSceneTileMod(oy-44,4); return 1;
    }
    if (v[0]==54) {
        /* Keep the island and pier unique. BG3's leftmost 128px is clear
         * sky/sea and safe as a water donor at horizontal map edges. */
        if (v[1]==2 && ox<0) {
            /* the pier goes on to the lower left along 2y+x, two posts
             * (eight columns) at a time (user 24/10) */
            sx=4+PortSceneTileMod(ox-4,8); sy=oy+(ox-sx)/2;
            if (sy<0 || sy>=64) return 0;
            *x=sx; *y=sy; return 1;
        }
        if (v[1]==2) return ox>=0 && ox<64 && oy>=0 && oy<64;
        if (ox>=0 && ox<64 && oy>=0 && oy<64) return 1;
        if (oy<0) {
            *x=PortSceneTileMod(ox,8); *y=PortSceneTileMod(oy,8); return 1;
        }
        if (oy>=64) {
            *x=PortSceneTileMod(ox,16); *y=48+PortSceneTileMod(oy-48,8); return 1;
        }
        /* Above the island, the full row carries the clouds and horizon;
         * below it, restrict donors to the eight-tile open-water margin. */
        *x=PortSceneTileMod(ox,oy<24?64:16); *y=oy; return 1;
    }
    if (v[0]==55) {
        /* The palm, island and debris live on BG2. Keep them unique; continue
         * BG3's cloud band at the sides and use its palette background as sea. */
        if (v[1]==2) return ox>=0 && ox<64 && oy>=0 && oy<64;
        if (oy<0) { *x=PortSceneTileMod(ox,64); *y=PortSceneTileMod(oy,4); return 1; }
        if (ox>=0 && ox<64 && oy>=0 && oy<32) return 1;
        if (oy>=0 && oy<8) { *x=PortSceneTileMod(ox,64); *y=oy; return 1; }
        return 0;
    }
    if (v[0]==7 && (ox<0 || ox>=64 || oy<0 || oy>=64)) {
        if (v[1]==2) {
            if (ox>=64) {
                if (oy<12 || oy>=20) return 0;
                *x=57+PortSceneTileMod(ox-57,7); *y=oy; return 1;
            }
            if (ox<0) {
                *x=PortSceneTileMod(ox,6); *y=oy+(ox-*x)/2;
                return *y>=0 && *y<64;
            }
            return 0;
        }
        if (ox<0) {
            if (oy<0) { *x=4; *y=0; return 1; }
            /* The first house is cropped 32 px into a complete 104 px bay.
             * Align the intact bay at x=104, including its preceding gap. */
            if (oy>=0 && oy<16) { *x=9+PortSceneTileMod(ox-9,17); *y=oy; return 1; }
            if (2*oy+ox<56) { *x=4; *y=20; return 1; }
            sx=PortSceneTileMod(ox,12); sy=oy+(ox-sx)/2;
            if (sy>=0 && sy<64) { *x=sx; *y=sy; return 1; }
        }
    }
    if (v[0]==44) {
        int diagonal=2*oy+ox;
        if (v[1]==2) {
            /* the town behind (user 24/10): to the left its low houses
             * (columns 12..23), to the right the arcade (37..48) repeat;
             * the facade's top piece at 15..19 floated over the red wall */
            if (oy>=23 && oy<26 && ox>=12 && ox<20) return 0;
            if (oy>=16 && oy<23 && ox<12) { *x=12+PortSceneTileMod(ox-12,12); *y=oy; return 1; }
            if (oy>=16 && oy<24 && ox>=49) { *x=37+PortSceneTileMod(ox-37,12); *y=oy; return 1; }
        }
        if (v[1]==3 && oy<21 && (ox<11 || ox>=56 || oy<0)) {
            *x=11+PortSceneTileMod(ox-11,8); *y=oy<0?0:oy; return 1;
        }
        if (v[1]==3 && ((ox>=8 && ox<64 && oy>=64) ||
            (ox<8 && diagonal>=116) || (ox>=64 && oy>=48))) {
            /* Sunlight crosses the ground in the opposite diagonal from
             * the yellow facade. Continue that band independently. */
            int light=2*oy-ox;
            if (light<64 || light>114) {
                *x=48+PortSceneTileMod(ox-48,8); *y=48+PortSceneTileMod(oy-48,4); return 1;
            }
            sx=24+PortSceneTileMod(ox-24,4); sy=oy+(sx-ox)/2;
            while (sy>=64) { sx-=4; sy-=2; }
            if (2*sy+sx<124) { sx+=8; sy+=4; }
            *x=sx; *y=sy; return sy>=0 && sy<64;
        }
        if (ox<8) {
            if (v[1]==2) {
                if (diagonal>=68 && diagonal<80) sx=8+PortSceneTileMod(ox-8,4);
                else return 0;
                *x=sx; *y=oy+(ox-sx)/2;
                return *y>=0 && *y<64;
            }
            if (oy>=21 && diagonal<64) { *x=0; *y=22; return 1; }
            sx=12+PortSceneTileMod(ox-12,4); sy=oy+(ox-sx)/2;
            if (sy>=0 && sy<64) { *x=sx; *y=sy; return 1; }
            *x=PortSceneTileMod(ox,8); *y=56+PortSceneTileMod(sy-56,8); return 1;
        }
        if (ox>=64 && oy>=21 && oy<24 && v[1]==3) { *x=0; *y=22; return 1; }
    }
    if (v[0]==87 && v[1]==3 &&
        (ox<0 || ox>=w || oy<0 || oy>=h)) {
        /* The chamber floor is repeatable, but BG3 also contains both room
         * walls. Extend only its clean interior swath; keep BG2's statues and
         * the authored perimeter strictly inside the original room. */
        int chunk=ox>=0?ox/w:(ox-(w-1))/w;
        int phase=PortSceneTileMod(chunk*3,8);
        /* the room's own structures go on too (user 24/10): the brick wall
         * on the right (columns 24..31 from row 18), the stone step at the
         * bottom along 2y+x=56, the stone landing at the top along 3y+x=12 */
        if (ox>=24 && oy>=18) {
            *x=24+PortSceneTileMod(ox-24,8); *y=oy<32?oy:22+PortSceneTileMod(oy-22,10); return 1;
        }
        if (ox<24 && 2*oy+ox>=56) {
            sx=8+PortSceneTileMod(ox-8,8); sy=oy+(ox-sx)/2;
            if (sy>=32) sy=28+PortSceneTileMod(sy-28,4);
            *x=sx; *y=sy; return 1;
        }
        if (3*oy+ox<12) { /* only the landing's stone (rows 0..1, columns 0..5) */
            *x=PortSceneTileMod(ox,6); *y=PortSceneTileMod(oy,2); return 1;
        }
        *x=PortSceneTileMod(ox,w);
        *y=8+PortSceneTileMod(oy-8+phase,8);
        return 1;
    }
    if (v[0]==90 && (ox<0 || ox>=w || oy<0 || oy>=h)) {
        /* BG3 merges scenery with unique props. Continue skyline, wall and
         * floor as separate diagonal bands. The wall donor is the prop-free
         * stretch x8..23, avoiding the central gate and the graves; BG1
         * overlays remain unique. */
        if (v[1]!=3) return 0;
        {
            int diagonal=2*oy+ox;
            if (ox>=w && oy>=16 && oy<28) {
                /* right: the wall and its whole railing (posts down to the
                 * masonry's foot, row 27) repeat unmirrored (user 24/10) */
                sx=60+PortSceneTileMod(ox-60,4);
                sy=oy<27?oy:26; /* row 27 holds a grave's top */
            } else if (ox<0 && oy>=16 && diagonal<64) {
                /* left, under the hills and above the railing: one solid
                 * colour of the night band, no stretched gradient (user 24/10) */
                sx=PortSceneTileMod(ox,8);
                sy=20;
            } else if (oy<16 || (ox>=w && oy<24)) {
                /* the distant hills only: the band below them carries the
                 * railing's tips, which mirrored into blocks (user 24/10) */
                /* the hills come every ~10.6 tiles: 32 columns are three of
                   them, so columns 0..31 (no moon) repeat seamlessly both
                   ways; mirroring without flipping the tiles stepped them */
                sx=PortSceneTileMod(ox,32);
                sy=oy<0?PortSceneTileMod(oy,8):oy;
            } else if (diagonal>=64 && diagonal<104) {
                /* the railing over the wall and the wall: the railing's posts
                 * reach up to 2y+x=64 (they were cut at 84, user 24/10) */
                sx=8+PortSceneTileMod(ox-8,16);
                sy=(diagonal-sx)/2;
                if (sy<0 || sy>=h) return 0;
            } else if (diagonal>=104) {
                if (ox<0) sx=PortSceneTileMod(ox,16);
                else if (ox>=w) sx=48+PortSceneTileMod(ox-48,16);
                else sx=48+PortSceneTileMod(ox-48,16);
                sy=48+PortSceneTileMod((diagonal-sx)/2-48,16);
            } else {
                /* Clear night backdrop below the distant hills, before the
                 * diagonal masonry starts; this eight-tile donor has no fence. */
                sx=PortSceneTileMod(ox,8);
                sy=24+PortSceneTileMod(oy-24,8);
            }
        }
        if (sx<0 || sx>=w || sy<0 || sy>=h) return 0;
        *x=sx; *y=sy;
        return 1;
    }
    if (v[0]==95 && (ox<0 || ox>=w || oy<0 || oy>=h)) {
        /* BG3 merges the maze, throne and garden. Continue a throne-free
         * skyline, a short flower-hedge module, then open lawn. The red path,
         * throne and BG1 trees/arch/stairs/chairs remain unique. */
        if (v[1]!=3) return 0;
        if (oy<8) {
            *x=PortSceneTileMod(ox,64); *y=PortSceneTileMod(oy,6); return 1;
        }
        if (oy<12) {
            int phase=PortSceneTileMod(ox,64);
            *x=phase<32?phase:63-phase; *y=8+PortSceneTileMod(oy-8,4); return 1;
        }
        /* the maze behind each hedge and the hedge itself go on along their
         * diagonals (user 24/10): left hedge from 2y+x=54 to 72, right hedge
         * from 2y-x=-16 to 0; the lawn in front */
        if (ox<0 && 2*oy+ox<72) {
            if (2*oy+ox<54) sx=PortSceneTileMod(ox,12);    /* the maze */
            else sx=8+PortSceneTileMod(ox-8,12);           /* the hedge and its topiaries */
            sy=oy+(ox-sx)/2;
            if (sy<12) sy=12+PortSceneTileMod(sy-12,8); /* the maze goes on upwards too */
            if (sy<64) { *x=sx; *y=sy; return 1; }
        }
        if (ox>=w && 2*oy-ox<0) {
            sx=58+PortSceneTileMod(ox-58,6); /* clear of the throne's cabinet */
            sy=oy+(sx-ox)/2;
            if (sy<12) sy=12+PortSceneTileMod(sy-12,8);
            if (sy<64) { *x=sx; *y=sy; return 1; }
        }
        /* BG3 is transparent below y=44. Fill the distant park with a broad,
         * mirrored sample of its clean grass margin, never the maze or path. */
        {
            int phase=PortSceneTileMod(ox,32);
            *x=phase<16?48+phase:63-(phase-16);
            *y=40+PortSceneTileMod(oy-40,4);
        }
        return 1;
    }
    if (v[0]==98 && (ox<0 || ox>=w || oy<0 || oy>=h)) {
        /* Queen of Hearts room: its wallpaper and checker floor are one
         * isometric room separated by the back-wall diagonal. Repeat only
         * clean texture cells; BG1's table and BG3's fireplace/door stay put. */
        if (v[1]!=3) return 0;
        {
            int diagonal=oy<0?0:oy>=h?128:ox<0?2*oy+ox:2*oy-(ox-w);
            if (diagonal<80) {
                /* Wide clean wallpaper panel above the red cornice. */
                sx=32+PortSceneTileMod(ox-32,16);
                sy=PortSceneTileMod(oy,5);
            } else {
                sx=40+PortSceneTileMod(ox-40,8);
                sy=48+PortSceneTileMod(oy-48,8);
            }
        }
        *x=sx; *y=sy; return 1;
    }
    if (v[0]==99 && (ox<0 || ox>=w || oy<0 || oy>=h)) {
        /* Queen's chamber: mirror only the side wallpaper modules and the
         * checker floor. The centered fireplace, lamp, arch and vase stay
         * unique; the clean panel supplies the unseen top/bottom rows. */
        if (v[1]!=3) return 0;
        if (ox<0 || ox>=w) {
            int distance=ox<0?-ox-1:ox-w;
            if (oy<3) {
                sx=PortSceneTileMod(ox,w);
            } else if (distance<8) {
                sx=ox<0?distance:w-1-distance;
            } else {
                /* Past one edge module, continue with the clean pink panel. */
                sx=11;
            }
        }
        if ((ox<0 || ox>=w) && oy>=16 && oy<21) {
            /* Continue clean wall trim without the candle motifs or vase. */
            sx=ox<0?4+PortSceneTileMod(-ox-1,3):29+PortSceneTileMod(ox-w,3);
        } else if ((ox<0 || ox>=w) && oy>=21) {
            /* Beyond the authored edges, continue the floor with solid
             * palette-matched checker cells; repeating the perspective map
             * would introduce extra vanishing points in the wide margins. */
            int distance=ox<0?-ox-1:ox-w;
            int cellWidth=oy<24?2:oy<26?3:oy<31?6:8;
            int baseDark;
            int dark;
            if (oy<h) baseDark=oy<24 || (oy>=29 && oy<31);
            else baseDark=PortSceneTileMod((oy-h)/2,2)!=0;
            dark=baseDark ^ ((distance/cellWidth)&1);
            if (dark) { sx=5; sy=31; }
            else if (oy>=24 && oy<26) { sx=5; sy=25; }
            else { sx=2; sy=31; }
            if (oy>=h) sy=h-1;
        }
        if (oy<0) {
            sy=PortSceneTileMod(oy,3);
        } else if (oy>=h && ox>=0 && ox<w) {
            /* Hold the nearest clean checker row past the room map. */
            sy=31;
        }
        *x=sx; *y=sy; return 1;
    }
    if (v[0] == 102) {
        /* The authored BG3 already contains its diagonal corridor. Do not
         * repeat that silhouette into the wide margins; the renderer fills
         * them from floor/water swatches outside the unchanged GBA viewport. */
        return ox >= 0 && ox < w && oy >= 0 && oy < h;
    }
    if (v[0] == 97) {
        if (ox >= 8 && ox < 48 && oy >= 0 && oy < 40) return 1;
        if (!v[20]) return 0;
        *x=8+PortSceneTileMod(ox-8,40); *y=PortSceneTileMod(oy,40); return 1;
    }
    if (ox >= 0 && ox < w && oy >= 0 && oy < h) return 1;
    if (v[0]==53 && v[1]==3 && (ox<0 || ox>=w || oy<0) && 2*oy-ox<-1 &&
        ox>=SCENE053_PLATEAU_X0 && ox<SCENE053_PLATEAU_X0+SCENE053_PLATEAU_W &&
        oy>=SCENE053_PLATEAU_Y0 && oy<SCENE053_PLATEAU_Y0+SCENE053_PLATEAU_H) {
        /* the plateau laid out with whole tufts (gen_scene053_plateau.py, user 24/10) */
        const uint8_t* t=kScene053Plateau[oy-SCENE053_PLATEAU_Y0][ox-SCENE053_PLATEAU_X0];
        *x=t[0]; *y=t[1]; return 1;
    }
    if (!v[20]) return 0; /* characters, furniture and unique landmarks */
    if (v[0]==112 && v[1]==2 && ox<0) return 0;
    if (ox < 0 || ox >= w) {
        int start=ox<0?v[6]:v[8], period=ox<0?v[7]:v[9];
        sx=start+PortSceneTileMod(ox-start,period);
        sy=oy+(ox<0?v[10]:v[11])*(ox-sx)/2;
    }
    if (sy >= 0 && sy < h) { *x=sx; *y=sy; return 1; }
    if (v[0]==53 && v[1]==2 && sy>=h) {
        *x=PortSceneTileMod(ox,8); *y=56+PortSceneTileMod(oy,8); return 1;
    }
    {
        int offset=sy<0?12:16;
        if (!v[offset+2] || !v[offset+3]) return 0;
        *x=v[offset]+PortSceneTileMod(ox-v[offset],v[offset+2]);
        *y=v[offset+1]+PortSceneTileMod(sy-v[offset+1],v[offset+3]);
        return 1;
    }
}

static inline int PortPreliminarPixelSource(const PortPreliminarRecipe* p, int wx, int wy, int* sx, int* sy) {
    if (p && p->v[0]==45 && p->v[1]==2 && wx<0 && wx>=-24 && wy>=54*8 && wy<64*8) {
        /* the pine cut by the map's left edge (its axis at x=20): its missing
         * half is its right half reflected, below row 54 where the next pine
         * ends; its tip above is narrow enough to be whole (user 24/10) */
        *sx=39-wx; *sy=wy;
        return 1;
    }
    if (p && p->v[0]==124 && (wx<0 || wx>=480 || wy<0 || wy>=440)) {
        int L=2*wy+wx, R=2*wy-wx;
        if (p->v[1]==3) {
            /* The golden field surrounds finite platforms. The two arms
             * which cross the source edges keep their own borders; the
             * upper-left point is an authored corner, not a repeated bay. */
            *sx=320+PortSceneTileMod(wx-320,64);
            *sy=400+PortSceneTileMod(wy-400,32);
            if (L<496) {
                *sx=PortSceneTileMod(wx,64);
                *sy=wy+(wx-*sx)/2;
                if (*sy<0) *sy=PortSceneTileMod(*sy,32);
            } else if (wx<0 && L>=600 && L<880) {
                *sx=8+PortSceneTileMod(wx-8,16);
                *sy=wy+(wx-*sx)/2;
            } else if (wx>=480 && L>=600 && L<880) {
                *sx=448+PortSceneTileMod(wx-448,16);
                *sy=wy+(wx-*sx)/2;
            } else if (wx>=480 && wx<520 && R>=16 && L<=1054) {
                *sx=448+PortSceneTileMod(wx-448,16);
                *sy=wy+(wx-*sx)/2;
            }
        } else if (p->v[1]==2) {
            *sx=504; *sy=504; /* no extra columns */
            if (wx<0 && L>=608 && L<656) {
                *sx=8+PortSceneTileMod(wx-8,16);
                *sy=wy+(wx-*sx)/2;
            } else if (wx<0 && L>=816 && L<896) {
                *sx=64+PortSceneTileMod(wx-64,16);
                *sy=wy+(wx-*sx)/2;
            } else if (wx>=480 && ((L>=608 && L<656) || (L>=816 && L<896))) {
                *sx=448+PortSceneTileMod(wx-448,16);
                *sy=wy+(wx-*sx)/2;
            } else if (wx>=480 && wx<520 && R>=0 && R<48 && L<=1054) {
                *sx=448+PortSceneTileMod(wx-448,16);
                *sy=wy-(wx-*sx)/2;
            } else if (wx>=480 && wx<520 && L>=1040 && L<1120 && R>=16) {
                *sx=448+PortSceneTileMod(wx-448,16);
                *sy=wy+(wx-*sx)/2;
            }
        } else return 0;
        return 1;
    }
    if (p && p->v[0]==121 && (wx<0 || wx>=384 || wy<0 || wy>=256)) {
        if (p->v[1]==2) {
            *sx=504; *sy=248; /* empty map cell, not the emblem */
            if (wx<0) {
                int x=PortSceneTileMod(wx,16), y=wy+(wx-x)/2;
                if (y>=128 && y<160) { *sx=x; *sy=y; }
            } else if (wx>=224) {
                int x=240+PortSceneTileMod(wx-240,16), y=wy+(wx-x)/2;
                if (y>=40 && y<64) { *sx=x; *sy=y; }
            }
            /* The original corner shaft crosses y=0. Continue that shaft,
             * and complete the two existing windows from the intact first
             * window; do not place windows at any new positions. */
            if (wy<0 && wx>=214 && wx<234) {
                *sx=wx; *sy=PortSceneTileMod(wy,32);
            } else if (wy<0 && wx>=284 && wx<292 && wy>=-10) {
                *sx=wx-20; *sy=wy+10;
            } else if (wy<0 && wx>=304 && wx<312 && wy>=-20) {
                *sx=wx-40; *sy=wy+20;
            }
        } else if (p->v[1]==3) {
            int floor;
            if (wx<192) {
                *sx=PortSceneTileMod(wx,32);
                *sy=wy+(wx-*sx)/2;
                if (*sy<0) *sy=PortSceneTileMod(*sy,64);
                floor=2*wy+wx>=272;
            } else if (wx>=224) {
                *sx=224+PortSceneTileMod(wx-224,32);
                *sy=wy+(wx-*sx)/2;
                if (*sy<0) *sy=PortSceneTileMod(*sy,32);
                floor=2*wy+wx>=336;
            } else {
                *sx=wx; *sy=PortSceneTileMod(wy,32);
                floor=2*wy-wx>=-112;
            }
            if (floor) {
                *sx=PortSceneTileMod(wx,128);
                *sy=160+PortSceneTileMod(wy-160,64);
            }
        } else return 0;
        return 1;
    }
    /* Coliseum 122, approved with honours (user 24/10, Codex c62c7d8ee) */
    if (p && p->v[0]==122 && (wx<0 || wx>=440 || wy<0 || wy>=336)) {
        /* Three original wall planes: the central face spans x=184..311.
         * Side walls follow 2*y-x, with their own intercepts. Translate in
         * pixels to retain the four-pixel phase of each 8-pixel column. */
        if (p->v[1]==2) {
            *sx=504; *sy=504; /* transparent: never clone the heraldic reliefs */
            if (wx<0 || (wx<184 && wy<0)) {
                int x=24+PortSceneTileMod(wx-24,16), y=wy+(x-wx)/2;
                if (y>=0 && y<32) { *sx=x; *sy=y; }
            } else if (wx>=440) {
                int x=384+PortSceneTileMod(wx-384,16), y=wy+(x-wx)/2;
                if (y>=112 && y<144) { *sx=x; *sy=y; }
            }
            /* Complete only the original panels cut by y=0 or x=440.
             * The intact diagonal panel at (312,16)..(376,112) is the donor;
             * no new heraldry is placed in the empty extended walls. */
            if (wy<0 && wx>=56 && wx<120) { *sx=wx+256; *sy=wy+64; }
            else if (wy<0 && wx>=120 && wx<184) { *sx=wx+192; *sy=wy+32; }
            else if (wx>=440 && wx<472 && wy-48>=16 && wy-48<112) {
                *sx=wx-96; *sy=wy-48;
            }
            if (*sy<0) { *sx=504; *sy=504; }
            if ((wx<184 && 2*wy-wx>=32) ||
                (wx>=184 && wx<312 && wy>=112) ||
                (wx>=312 && 2*wy-wx>=-96)) {
                *sx=PortSceneTileMod(wx,128);
                *sy=160+PortSceneTileMod(wy-160,128);
            }
        } else if (p->v[1]==3) {
            if (wx<184) {
                *sx=128+PortSceneTileMod(wx-128,32);
                *sy=wy+(*sx-wx)/2;
                if (*sy<0) *sy=PortSceneTileMod(*sy,32);
            } else if (wx>=312) {
                *sx=320+PortSceneTileMod(wx-320,32);
                *sy=wy+(*sx-wx)/2;
                if (*sy<0) *sy=PortSceneTileMod(*sy,32);
            } else {
                *sx=wx; *sy=PortSceneTileMod(wy,64);
            }
            /* Past the wall contact, extend an intact pavement module.
             * Original BG2 accents stay within the authored floor. */
            if ((wx<184 && 2*wy-wx>=-24) ||
                (wx>=184 && wx<312 && wy>=96) ||
                (wx>=312 && 2*wy-wx>=-152)) {
                *sx=PortSceneTileMod(wx,64);
                *sy=160+PortSceneTileMod(wy-160,64);
            }
        } else return 0;
        return 1;
    }
    /* Coliseum 121, approved with honours (user 24/10, Codex 1ee40e4e6): only this scene of Preliminar V1 */
    if (p && p->v[0]==121 && (wx<0 || wx>=384 || wy<0 || wy>=256)) {
        if (p->v[1]==2) {
            *sx=504; *sy=248; /* empty map cell, not the emblem */
            if (wx<0) {
                int x=PortSceneTileMod(wx,16), y=wy+(wx-x)/2;
                if (y>=128 && y<160) { *sx=x; *sy=y; }
            } else if (wx>=224) {
                int x=240+PortSceneTileMod(wx-240,16), y=wy+(wx-x)/2;
                if (y>=40 && y<64) { *sx=x; *sy=y; }
            }
            /* The original corner shaft crosses y=0. Continue that shaft,
             * and complete the two existing windows from the intact first
             * window; do not place windows at any new positions. */
            if (wy<0 && wx>=214 && wx<234) {
                *sx=wx; *sy=PortSceneTileMod(wy,32);
            } else if (wy<0 && wx>=284 && wx<292 && wy>=-10) {
                *sx=wx-20; *sy=wy+10;
            } else if (wy<0 && wx>=304 && wx<312 && wy>=-20) {
                *sx=wx-40; *sy=wy+20;
            }
        } else if (p->v[1]==3) {
            int floor;
            if (wx<192) {
                *sx=PortSceneTileMod(wx,32);
                *sy=wy+(wx-*sx)/2;
                if (*sy<0) *sy=PortSceneTileMod(*sy,64);
                floor=2*wy+wx>=272;
            } else if (wx>=224) {
                *sx=224+PortSceneTileMod(wx-224,32);
                *sy=wy+(wx-*sx)/2;
                if (*sy<0) *sy=PortSceneTileMod(*sy,32);
                floor=2*wy+wx>=336;
            } else {
                *sx=wx; *sy=PortSceneTileMod(wy,32);
                floor=2*wy-wx>=-112;
            }
            if (floor) {
                *sx=PortSceneTileMod(wx,128);
                *sy=160+PortSceneTileMod(wy-160,64);
            }
        } else return 0;
        return 1;
    }
    if (p && p->v[0]==44 && p->v[1]==2 && wx<64 &&
        2*wy+wx>=114*8 && 2*wy+wx<124*8) {
        /* Clean curb at x=120..127, between the original flower boxes.
         * Alternating columns need a FOUR PIXEL vertical offset: retaining
         * the pixel phase is essential, rather than clipping half a tile. */
        *sx=120+PortSceneTileMod(wx-120,8); *sy=wy+(wx-*sx)/2;
        return *sy>=0 && *sy<512;
    }
    return 0;
}
#endif
