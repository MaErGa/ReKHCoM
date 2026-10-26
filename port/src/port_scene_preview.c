/* Static gallery stills from the cartridge's background definition. No game mode, task, palette or VRAM changes.
 * This gives old saves a preview immediately; after replay the gallery keeps a real presented scene capture. */
#include <SDL3/SDL.h>
#include <string.h>
#include "port/port.h"
#include "evt/event_background_types.h"
#include "evt/event_index_data.h"
#include "evt/event_ids.h"
#include "port/port_scene_gallery.h"

void LZ77UnCompWram(const void* src, void* dst);

static unsigned char* Unpack(const void* source, size_t size, int compressed, size_t limit, size_t* unpacked) {
    const unsigned char* src = source;
    unsigned char* out;
    if (!src) return NULL;
    if (compressed) {
        if (src[0] != 0x10) return NULL;
        size = src[1] | (size_t)src[2] << 8 | (size_t)src[3] << 16;
    }
    if (!size || size > limit) return NULL;
    out = SDL_malloc(size);
    if (out) {
        if (compressed) LZ77UnCompWram(source, out); else memcpy(out, source, size);
        *unpacked = size;
    }
    return out;
}

static uint32_t Color(unsigned color) {
    unsigned r = color & 31, g = (color >> 5) & 31, b = (color >> 10) & 31;
    return 0xFF000000u | ((r << 3 | r >> 2) << 16) | ((g << 3 | g >> 2) << 8) | (b << 3 | b >> 2);
}

int PortSceneBackgroundPreview(int eventId, uint32_t* out, int width, int height) {
    const EventBackgroundDef* bg;
    unsigned char *tiles = NULL, *tiles2 = NULL;
    size_t ntiles = 0, ntiles2 = 0;
    const unsigned short* palette;
    int x, y, layer, ok = 0;
    if (PORT_SCENE_IS_MOVIE(eventId)) {
        /* Its cartridge backdrop, tied to the same story segment; replaced by a live still after viewing. */
        static const int backdrop[] = {0, EVENT_000_1F_ENTRANCE_PART1, EVENT_025_6F_GOAL_1,
            EVENT_056_12F_DESTINY_ISLAND_E2_2, EVENT_073_13F_CAPSULE_ROOM_ENDING, EVENT_195_RIKU_ENDING};
        eventId = backdrop[eventId - PORT_SCENE_MOVIE_BASE];
    }
    if (eventId < 0 || eventId >= EVENT_COUNT || width <= 0 || height <= 0)
        return 0;
    bg = gEventBackgroundDefs[eventId];
    if (!bg) {
        /* This event's ROM definition is intentionally a black backdrop (e.g. 070), not a missing asset. */
        for (y = 0; y < height; y++) for (x = 0; x < width; x++) out[y * width + x] = 0xFF000000u;
        return 1;
    }
    if (!bg->palette || !bg->mapWidth || !bg->mapHeight || bg->mapWidth > 16 || bg->mapHeight > 16)
        return 0;
    palette = bg->palette;
    tiles = Unpack(bg->tiles, bg->tilesSize, bg->compression[0] & 1, 65536, &ntiles);
    if (!tiles) goto done;
    if (bg->tiles2)
        tiles2 = Unpack(bg->tiles2, bg->tilesSize2, bg->compression[0] & 1, 65536, &ntiles2);
    for (y = 0; y < height; y++) for (x = 0; x < width; x++) out[y * width + x] = Color(palette[0]);
    for (layer = 0; layer < (bg->isAffine ? 1 : 3); layer++) {
        const void** maps = layer == 0 ? bg->maps : layer == 1 ? bg->maps2 : bg->maps3;
        const unsigned char* td = layer == 2 && tiles2 ? tiles2 : tiles;
        size_t nt = layer == 2 && tiles2 ? ntiles2 : ntiles;
        unsigned char* map;
        size_t mapBytes, blockBytes = bg->isAffine ? 4096 : 2048;
        int b, blocks = bg->isAffine ? 1 : bg->mapWidth * bg->mapHeight;
        int mw = bg->isAffine ? 512 : bg->mapWidth * 256, mh = bg->isAffine ? 512 : bg->mapHeight * 256;
        if (!maps) continue;
        map = SDL_malloc((size_t)blocks * blockBytes);
        if (!map) goto done;
        for (b = 0; b < blocks; b++) {
            unsigned char* decoded = Unpack(maps[b], blockBytes, bg->compression[0] & 2, blockBytes, &mapBytes);
            if (!decoded || mapBytes != blockBytes) { SDL_free(decoded); break; }
            memcpy(map + (size_t)b * blockBytes, decoded, blockBytes);
            SDL_free(decoded);
        }
        if (b == blocks) {
            for (y = 0; y < height; y++) for (x = 0; x < width; x++) {
                int sx = x * mw / width, sy = y * mh / height, entry, tx, ty, ci;
                size_t index;
                if (bg->isAffine) {
                    entry = map[(sy / 8) * 64 + sx / 8];
                    index = (size_t)entry * 64 + (sy & 7) * 8 + (sx & 7);
                    ci = index < nt ? td[index] : 0;
                } else {
                    int block = (sy / 256) * bg->mapWidth + sx / 256;
                    entry = ((unsigned short*)(map + (size_t)block * blockBytes))[((sy & 255) / 8) * 32 + (sx & 255) / 8];
                    tx = entry & 1024 ? 7 - (sx & 7) : sx & 7;
                    ty = entry & 2048 ? 7 - (sy & 7) : sy & 7;
                    index = (size_t)(entry & 1023) * 32 + ty * 4 + tx / 2;
                    ci = index < nt ? (td[index] >> ((tx & 1) * 4)) & 15 : 0;
                    if (!ci) continue;
                    ci += (entry >> 12) * 16;
                }
                if (ci * 2 < bg->paletteSize) out[y * width + x] = Color(palette[ci]);
            }
            ok = 1;
        }
        SDL_free(map);
    }
done:
    SDL_free(tiles2);
    SDL_free(tiles);
    return ok;
}
