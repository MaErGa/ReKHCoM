/* The scene gallery (Options > Scenes, user 25/10): the story scenes already seen, to watch again in their cartridge
 * version or their Re:CoM HD one. What was seen is the whole ReKHCoM install's (any save, any bank): the furthest any
 * game got. scenes_seen.dat holds one bit per event id. Cheats > Unlock all scenes fills it. */
#include <stdio.h>
#include <string.h>

#include "port/port.h"
#include "game_state.h"
#include "map_api.h"

#define SCENE_BITS 1024

static unsigned char sSeen[SCENE_BITS / 8];
static int sLoaded;

static void Save(void);

/* The scenes a game already went through, from its progress: the gallery's scenes are the story's (the floors' halls),
   each floor's run of event ids in order (event_ids.h): Sora's 13 floors, Riku's 12 (B12F first). Saves from before
   the gallery (scenes_seen.dat only counts scenes played since) get theirs back this way, with nothing past where
   they are. floor: the floor the game is on; flags: that floor's (GameFloor), -1 unknown (a file's summary). */
static const unsigned char kSoraFloors[14] = { 0, 12, 15, 18, 21, 24, 29, 32, 36, 39, 43, 52, 61, 74 };
static const unsigned char kRikuFloors[13] = { 149, 158, 160, 162, 163, 166, 168, 169, 170, 176, 185, 191, 197 };

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
    if (cleared)
        return Mark(at[0], at[floors]);
    if (floor < 0)
        return 0;
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
