/* The scene gallery (Options > Scenes, user 25/10): the story scenes already seen, to watch again in their cartridge
 * version or their Re:CoM HD one. What was seen is the whole ReKHCoM install's (any save, any bank): the furthest any
 * game got. scenes_seen.dat holds one bit per event id. Cheats > Unlock all scenes fills it. */
#include <stdio.h>
#include <string.h>
#include <SDL3/SDL.h>

#include "port/port.h"
#include "game_state.h"
#include "map_api.h"
#include "port/port_state.h"
#include "evt/event_ids.h"
#include "port/port_scene_gallery.h"

extern void PortReplayVisitState(PortStateVisitor visit);
extern int PortSceneGalleryBegin(int eventId);
extern void PortUiGalleryResume(void);

enum { GALLERY_IDLE, GALLERY_SAVE, GALLERY_PLAY, GALLERY_RETURN };
static int sReplay, sReplayEvent, sReplayFrames;
static int sReplayEntered;
static unsigned sReplayStartFrame;
static int sReplayFallback;
static unsigned char *sReplayMemory, *sReplayCursor;
static size_t sReplayBytes;
static char sReplayMode[64];

typedef struct GalleryAudit { uint32_t game, map, config, files; int recom, rank; } GalleryAudit;
static GalleryAudit sAudit;
static uint32_t AuditHash(const void* data, size_t size) {
    const unsigned char* p = data;
    uint32_t hash = 2166136261u;
    while (size--) hash = (hash ^ *p++) * 16777619u;
    return hash;
}
static GalleryAudit AuditStamp(void) {
    static const char* const names[] = { "scenes_seen.dat", "kh_com_recom.ini", "rebel_mode.txt",
        "achievements_official_eu.dat", "achievements_official_us.dat", "achievements_official_jp.dat",
        "achievements_rebel_eu.dat", "achievements_recom_eu.dat", "achievements_recom_extra.dat",
        "achievements_rebel_recom_eu.dat", "achievements_rebel_recom_extra.dat" };
    GalleryAudit stamp = {0};
    unsigned i;
    stamp.game = AuditHash(&gGameState, sizeof(gGameState));
    stamp.map = AuditHash(&gMapFloorState, sizeof(gMapFloorState));
    stamp.config = AuditHash(&gPortConfig, sizeof(gPortConfig));
    stamp.recom = PortRecomActive(); stamp.rank = PortRecomRunRank();
    for (i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        FILE* f = fopen(PortConfigPath(names[i]), "rb");
        int c;
        stamp.files = (stamp.files ^ i) * 16777619u;
        if (f) { while ((c = fgetc(f)) != EOF) stamp.files = (stamp.files ^ c) * 16777619u; fclose(f); }
    }
    return stamp;
}
void PortSceneGalleryAuditBegin(void) { if (SDL_getenv("KHCOM_GALLERY_AUDIT")) sAudit = AuditStamp(); }
void PortSceneGalleryAuditEnd(void) {
    if (SDL_getenv("KHCOM_GALLERY_AUDIT")) {
        GalleryAudit now = AuditStamp();
        PortLog("scene gallery audit: game=%d map=%d config=%d files=%d recom=%d rank=%d",
            sAudit.game == now.game, sAudit.map == now.map, sAudit.config == now.config, sAudit.files == now.files,
            sAudit.recom == now.recom, sAudit.rank == now.rank);
    }
}

/* One small still in memory, never a movie or an unbounded list of decoded frames. Disk copies survive runs. */
#define PREVIEW_W 160
#define PREVIEW_H 90
static uint32_t sPreview[PREVIEW_W * PREVIEW_H];
static int sPreviewEvent = -1, sPreviewHd = -1, sPreviewReady, sPreviewSerial, sPreviewLoadedSerial = -1;
static int sReplayCaptured;
static int sPreviewWait;
static int sHdReplayEvent = -1, sHdCaptured;

void PortSceneGalleryHdBegin(int eventId) { sHdReplayEvent = eventId; sHdCaptured = 0; }
void PortSceneGalleryHdEnd(void) { sHdReplayEvent = -1; }

static const char* PreviewPath(int eventId, int hd) {
    char name[64];
    SDL_snprintf(name, sizeof(name), "scene-preview-%s-%03d.rgb", hd ? "hd" : "gba", eventId);
    return PortConfigPath(name);
}

void PortSceneGalleryRemember(int eventId, int hd, const uint32_t* frame, int w, int h) {
    uint32_t still[PREVIEW_W * PREVIEW_H];
    FILE* f;
    int x, y;
    if (!frame || w <= 0 || h <= 0)
        return;
    for (y = 0; y < PREVIEW_H; y++)
        for (x = 0; x < PREVIEW_W; x++)
            still[y * PREVIEW_W + x] = frame[(y * h / PREVIEW_H) * w + x * w / PREVIEW_W] | 0xFF000000u;
    f = fopen(PreviewPath(eventId, hd), "wb");
    if (f) {
        fwrite(still, 1, sizeof(still), f);
        fclose(f);
        sPreviewSerial++;
    }
}

const uint32_t* PortSceneGalleryPreview(int eventId, int hd) {
    FILE* f;
    if (eventId != sPreviewEvent || hd != sPreviewHd || sPreviewLoadedSerial != sPreviewSerial) {
        sPreviewEvent = eventId;
        sPreviewHd = hd;
        sPreviewLoadedSerial = sPreviewSerial;
        sPreviewReady = 0;
        sPreviewWait = 0;
        f = fopen(PreviewPath(eventId, hd), "rb");
        if (f) {
            sPreviewReady = fread(sPreview, 1, sizeof(sPreview), f) == sizeof(sPreview);
            fclose(f);
        }
        if (!sPreviewReady && !hd) {
            extern int PortSceneBackgroundPreview(int eventId, uint32_t* out, int width, int height);
            sPreviewReady = PortSceneBackgroundPreview(eventId, sPreview, PREVIEW_W, PREVIEW_H);
            if (sPreviewReady) PortLog("scene gallery preview: GBA event %d from cartridge background", eventId);
        }
    }
#if defined(_WIN32) || defined(PORT_LINUX)
    /* Debounce the cursor: no decoder work while scrolling, no startup prefetch. One frame, one selected scene. */
    if (!sPreviewReady && hd && ++sPreviewWait == 12) {
        extern int PortHdScenePreview(const char* movie, uint32_t* out, int w, int h, int second);
        const char* movie = PortHdGalleryMovieName(eventId);
        if (movie) {
                sPreviewReady = PortHdScenePreview(movie, sPreview, PREVIEW_W, PREVIEW_H, PortHdGalleryMovieOffset(eventId) + 1);
                if (sPreviewReady) {
                    PortLog("scene gallery preview: HD event %d from one decoded frame", eventId);
                    PortSceneGalleryRemember(eventId, 1, sPreview, PREVIEW_W, PREVIEW_H);
                }
        }
    }
#endif
    return sPreviewReady ? sPreview : NULL;
}

void PortSceneGalleryCapture(void) {
    if (sReplay == GALLERY_PLAY && !sReplayCaptured && PortFrameCount() - sReplayStartFrame >= 90) {
        PortSceneGalleryRemember(sReplayEvent, 0, gPortFrame, gPortScreen.width, gPortScreen.height);
        sReplayCaptured = 1;
    }
}

void PortSceneGalleryMovieCapture(SDL_Renderer* renderer) {
#if defined(_WIN32) || defined(PORT_LINUX)
    if (sHdReplayEvent >= 0 && !sHdCaptured) {
        SDL_Surface* raw = SDL_RenderReadPixels(renderer, NULL);
        SDL_Surface* rgb = raw ? SDL_ConvertSurface(raw, SDL_PIXELFORMAT_XRGB8888) : NULL;
        if (rgb && rgb->pitch == rgb->w * 4) {
            PortSceneGalleryRemember(sHdReplayEvent, 1, rgb->pixels, rgb->w, rgb->h);
            sHdCaptured = 1;
        }
        SDL_DestroySurface(rgb);
        SDL_DestroySurface(raw);
    }
#else
    (void)renderer;
#endif
}

int PortSceneGalleryActive(void) { return sReplay != GALLERY_IDLE || sHdReplayEvent >= 0; }
int PortSceneGalleryAvailable(void) {
    char *data, *bss;
    size_t dataSize, bssSize;
    extern int PortDebugModeAvailable(void);
    extern int NetRollbackSession(void);
    PortGameSections(&data, &dataSize, &bss, &bssSize);
    return !NetRollbackSession() && ((dataSize && bssSize) || PortDebugModeAvailable());
}
unsigned PortSceneGalleryAge(void) { return PortFrameCount() - sReplayStartFrame; }
static void ReplaySize(void* p, size_t n) { (void)p; sReplayBytes += n; }
static void ReplaySave(void* p, size_t n) { memcpy(sReplayCursor, p, n); sReplayCursor += n; }
static void ReplayLoad(void* p, size_t n) { memcpy(p, sReplayCursor, n); sReplayCursor += n; }
static void ReplayVisit(PortStateVisitor visit) {
    PortReplayVisitState(visit);
    visit((void*)(uintptr_t)GBA_SRAM, GBA_SRAM_SIZE); /* a replay cannot change the player's cartridge save */
}

int PortSceneGalleryPlay(int eventId) {
    char *data, *bss;
    size_t dataSize, bssSize;
    extern int NetRollbackSession(void);
    if (sReplay || NetRollbackSession() || eventId < 0 || (eventId >= EVENT_COUNT && !PORT_SCENE_IS_MOVIE(eventId)))
        return 0;
    PortGameSections(&data, &dataSize, &bss, &bssSize);
    if (!dataSize || !bssSize) {
        extern int PortDebugModeAvailable(void);
        if (!PortDebugModeAvailable()) return 0; /* no running-game snapshot on this backend */
        PortSramSyncIfDirty();
        sReplayEvent = eventId;
        sReplayFallback = 1;
        sReplay = GALLERY_PLAY;
        sReplayFrames = 0;
        sReplayCaptured = 0;
        sReplayEntered = 0;
        sReplayStartFrame = PortFrameCount();
        PortSceneGalleryBegin(eventId);
        return 1;
    }
    PortSramSyncIfDirty(); /* commit any already completed user save before temporarily blocking replay writes */
    sReplayFallback = 0;
    sReplayEvent = eventId;
    sReplay = GALLERY_SAVE;
    return 1;
}

void PortSceneGalleryReturn(void) {
    if (sReplay == GALLERY_PLAY)
        sReplay = GALLERY_RETURN;
}

void PortSceneGalleryShutdown(void) {
    SDL_free(sReplayMemory);
    sReplayMemory = NULL;
    sReplay = GALLERY_IDLE;
    sHdReplayEvent = -1;
    sReplayFallback = 0;
}

/* Same native call stack as quick states/rollback; never restore from an event task's own wait. */
void PortSceneGalleryFrame(void) {
    extern const char* PortHarnessModeName(void);
    if (sReplayFallback) {
        if (sReplay == GALLERY_RETURN) {
            extern void PortSceneGalleryGoTitle(void);
            PortSceneGalleryGoTitle();
            sReplay = GALLERY_SAVE;
        } else if (sReplay == GALLERY_SAVE && !strcmp(PortHarnessModeName(), "mode_title")) {
            PortSceneGalleryShutdown();
            PortUiGalleryResume();
        }
        return;
    }
    if (!gPortInMainLoop)
        return;
    if (sReplay == GALLERY_SAVE) {
        sReplayBytes = 0;
        ReplayVisit(ReplaySize);
        if (sReplayBytes > 64u * 1024u * 1024u || !(sReplayMemory = SDL_malloc(sReplayBytes))) {
            PortLog("scene gallery: checkpoint unavailable; playback cancelled");
            sReplay = GALLERY_IDLE;
            PortUiGalleryResume();
            return;
        }
        sReplayCursor = sReplayMemory;
        ReplayVisit(ReplaySave);
        PortSceneGalleryAuditBegin();
        SDL_strlcpy(sReplayMode, PortHarnessModeName(), sizeof(sReplayMode));
        sReplayFrames = 0;
        sReplayEntered = 0;
        sReplayStartFrame = PortFrameCount();
        sReplayCaptured = 0;
        sReplay = GALLERY_PLAY;
        PortLog("scene gallery: kept %u bytes from %s; playing %d", (unsigned)sReplayBytes, sReplayMode, sReplayEvent);
        PortSceneGalleryBegin(sReplayEvent);
    } else if (sReplay == GALLERY_RETURN) {
        sReplayCursor = sReplayMemory;
        ReplayVisit(ReplayLoad);
        PortSceneGalleryAuditEnd();
        PortAudioReset();
        PortSceneGalleryShutdown();
        PortUiGalleryResume();
        PortLog("scene gallery: restored original state; returning to the list (%s)", PortHarnessModeName());
    } else if (sReplay == GALLERY_PLAY) {
        /* A scene which never entered its mode returns safely rather than stranding the player. */
        ++sReplayFrames;
        const char* expected = PORT_SCENE_IS_MOVIE(sReplayEvent) ? "mode_movie" : "Event";
        if (!strcmp(PortHarnessModeName(), expected)) sReplayEntered = 1;
        if ((sReplayEntered || sReplayFrames > 600) && strcmp(PortHarnessModeName(), expected))
            sReplay = GALLERY_RETURN;
    }
}

#define SCENE_BITS 1024

static unsigned char sSeen[SCENE_BITS / 8];
static int sLoaded;

static void Save(void);

/* The scenes a game already went through, from its progress: the gallery's scenes are the story's (the floors' halls),
   each floor's run of event ids in order (event_ids.h): Sora's 13 floors, Riku's 12 (B12F first). Saves from before
   the gallery (scenes_seen.dat only counts scenes played since) get theirs back this way, with nothing past where
   they are. floor: the floor the game is on; flags: that floor's (GameFloor), -1 unknown (a file's summary). */
static const unsigned char kSoraFloors[14] = { 0, 12, 15, 18, 21, 24, 29, 32, 36, 39, 43, 52, 61, 74 };
static const unsigned char kRikuFloors[13] = { EVENT_149_RIKU_B12F_OPNING, EVENT_158_RIKU_B11F_ENTRANCE,
    EVENT_160_RIKU_B10F_GOAL, EVENT_162_RIKU_B9F_DEMO, EVENT_163_RIKU_B8F_GOAL, EVENT_166_RIKU_B7F_ENTRANCE,
    EVENT_168_RIKU_B6F_DEMO, EVENT_169_RIKU_B5F_DEMO, EVENT_170_RIKU_B4F_ENTRANCE, EVENT_176_RIKU_B3F_ENTRANCE,
    EVENT_185_RIKU_B2F_ENTRANCE, EVENT_191_RIKU_B1F_ENTRANCE, EVENT_COUNT };

static int Mark(int from, int to) {
    int e, added = 0;
    for (e = from; e < to && e < SCENE_BITS; e++)
        if (!(sSeen[e >> 3] & (1 << (e & 7)))) {
            sSeen[e >> 3] |= (unsigned char)(1 << (e & 7));
            added = 1;
        }
    return added;
}

static int FromProgress(int riku, int cleared, int floor, int flags) {
    const unsigned char* at = riku ? kRikuFloors : kSoraFloors;
    int floors = riku ? 12 : 13, f, added = 0;
    if (!riku && (floor >= 0 || cleared)) added |= Mark(PORT_SCENE_MOVIE(1), PORT_SCENE_MOVIE(1) + 1);
    if (!riku && (floor >= 6 || cleared)) added |= Mark(PORT_SCENE_MOVIE(2), PORT_SCENE_MOVIE(2) + 1);
    if (!riku && (floor >= 12 || cleared)) added |= Mark(PORT_SCENE_MOVIE(3), PORT_SCENE_MOVIE(3) + 1);
    if (cleared) added |= Mark(PORT_SCENE_MOVIE(riku ? 5 : 4), PORT_SCENE_MOVIE(riku ? 5 : 4) + 1);
    if (cleared)
        return added | Mark(at[0], at[floors]);
    if (floor < 0)
        return added;
    for (f = 0; f < floor && f < floors; f++)
        added |= Mark(at[f], at[f + 1]);
    if (floor < floors && flags >= 0) {
        if (flags & (FLOOR_FLAG_CLEARED | FLOOR_FLAG_EXIT_EVENT_DONE))
            added |= Mark(at[floor], at[floor + 1]);
        else if (flags & FLOOR_FLAG_ENTRY_EVENT_DONE)
            added |= Mark(at[floor], at[floor] + 1); /* the floor's entrance */
    }
    return added;
}

static void Load(void) {
    FILE* f;
    if (sLoaded)
        return;
    sLoaded = 1;
    f = fopen(PortConfigPath("scenes_seen.dat"), "rb");
    if (f) {
        if (fread(sSeen, 1, sizeof(sSeen), f) != sizeof(sSeen))
            memset(sSeen, 0, sizeof(sSeen)); /* an older or broken file: start again */
        fclose(f);
    }
}

/* the saves' own progress, as the title read their summaries (they change as the player saves: looked at whenever the
   gallery asks): files 1-2 Sora's, 3-4 Riku's; a cleared story, all of it */
static void FromSaves(void) {
    int i, added = 0;
    added |= FromProgress(0, (gGameState.flags & GAME_FLAG_SORA_CLEAR) != 0, -1, -1);
    added |= FromProgress(1, (gGameState.flags & GAME_FLAG_RIKU_CLEAR) != 0, -1, -1);
    for (i = 0; i < 4; i++)
        if (gGameState.fileSummaries[i].playTime)
            added |= FromProgress(i >= 2, 0, gGameState.fileSummaries[i].floor, -1);
    if (added) {
        PortLog("scene gallery: scenes seen added from the saves' progress");
        Save();
    }
}

/* save.c: a game was loaded: its floors' own marks too (the current floor's entrance, its end) */
void PortScenesFromGame(void) {
    int floor = gGameState.floor;
    if (PortSceneGalleryActive()) return;
    Load();
    /* the floor being played keeps its marks in the map's state (saved with it); floors[] has them once left */
    if (FromProgress((gGameState.flags & GAME_FLAG_RIKU) != 0, 0, floor,
                     floor >= 0 && floor < 13 ? gMapFloorState.flags | gGameState.floors[floor].flags : -1))
        Save();
}

static void Save(void) {
    FILE* f = fopen(PortConfigPath("scenes_seen.dat"), "wb");
    if (!f)
        return;
    fwrite(sSeen, 1, sizeof(sSeen), f);
    fclose(f);
}

void PortSceneSeen(int eventId) {
    if (PortSceneGalleryActive()) return;
    eventId &= 0x7FFF;
    if (eventId < 0 || eventId >= SCENE_BITS)
        return;
    Load();
    if (sSeen[eventId >> 3] & (1 << (eventId & 7)))
        return;
    sSeen[eventId >> 3] |= (unsigned char)(1 << (eventId & 7));
    Save();
}

int PortSceneIsSeen(int eventId) {
    eventId &= 0x7FFF;
    if (eventId < 0 || eventId >= SCENE_BITS)
        return 0;
    Load();
    FromSaves();
    return (sSeen[eventId >> 3] >> (eventId & 7)) & 1;
}

void PortScenesUnlockAll(void) {
    Load();
    memset(sSeen, 0xFF, sizeof(sSeen));
    Save();
}
