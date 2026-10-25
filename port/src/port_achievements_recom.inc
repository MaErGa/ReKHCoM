/* Re:CoM achievements (port_achievements.c): a list of their own for games of the Re:CoM version, kept apart from
 * the official one and never sent to RetroAchievements.
 *  - the official set again, evaluated the same way into achievements_recom_eu.dat; the ones about a world or a
 *    boss (completing one, defeating one, its challenges) count only when the whole game was played on Proud;
 *  - the mode's own: each story finished per difficulty (a higher one also earns the lower ones) and both on Proud.
 * Cheats, save states and the debug menu pause this list too. */
#ifdef KH_OFFLINE_EVAL
#include "../achievements/recom_set.inc"
static KhOfflineState sRecom;
static char sRecomPath[1200], sRecomExtraPath[1200];
static rc_runtime_t sRecomRuntime;
static int sRecomReady;
static uint64_t sRecomNotices[(KH_OFFLINE_MAX + 63) / 64];

static uint64_t sRecomExtra[RECOM_EXTRA_COUNT];
static unsigned sRecomExtraNotices;

int PortRecomExtraCount(void) { return RECOM_EXTRA_COUNT; }
const char* PortRecomExtraText(int index, int lang, int field) {
    if (index < 0 || index >= RECOM_EXTRA_COUNT) return "";
    if (lang < 0 || lang > 5) lang = 0;
    return kRecomExtraText[index][lang][field ? 1 : 0];
}

int PortRecomNeedsProud(int index) {
    const KhOfficialAchievement* a = KhOfficialAt(index);
    return a && KhRecomNeedsProud(a);
}

static uint64_t RecomNow(void) {
    uint64_t now = 1;
#ifdef KH_WALL_CLOCK
    now = (uint64_t)time(NULL);
#endif
    return now ? now : 1;
}

static void RecomExtraLoad(void) {
    unsigned long long when[RECOM_EXTRA_COUNT];
    FILE* f = fopen(sRecomExtraPath, "r");
    int i;
    KhRecomExtraParse(f, when);
    if (f) fclose(f);
    for (i = 0; i < RECOM_EXTRA_COUNT; i++) sRecomExtra[i] = when[i];
}

static void RecomExtraSave(void) {
    FILE* f = fopen(sRecomExtraPath, "w");
    int i;
    if (!f) { PortLog("achievements: could not save Re:CoM progress"); return; }
    fprintf(f, "KHREC1\n");
    for (i = 0; i < RECOM_EXTRA_COUNT; i++)
        if (sRecomExtra[i]) fprintf(f, "%d %llu\n", i, (unsigned long long)sRecomExtra[i]);
    fclose(f);
}

static void RecomExtraUnlock(int index) {
    if (sRecomExtra[index]) return;
    sRecomExtra[index] = RecomNow();
    sRecomExtraNotices |= 1u << index;
    PortLog("achievement: Re:CoM %d unlocked", index);
    RecomExtraSave();
}

static void RecomEvent(const rc_runtime_event_t* event) {
    int index;
    const KhOfficialAchievement* a;
    if (event->type != RC_RUNTIME_EVENT_ACHIEVEMENT_TRIGGERED)
        return;
    index = KhOfficialIndex(event->id);
    a = KhOfficialAt(index);
    rc_runtime_deactivate_achievement(&sRecomRuntime, event->id);
    if (!a || sRecom.when[index])
        return;
    if (KhRecomNeedsProud(a) && PortRecomRunRank() < 2) {
        /* not on Proud: armed again, so a Proud game can still earn it */
        rc_runtime_activate_achievement(&sRecomRuntime, a->id, a->definition, NULL, 0);
        return;
    }
    sRecom.when[index] = RecomNow();
    PortLog("achievement: Re:CoM %u unlocked", (unsigned)event->id);
    if (!KhOfflineSave(&sRecom, sRecomPath)) PortLog("achievements: could not save Re:CoM progress");
    sRecomNotices[index / 64] |= 1ull << (index % 64);
}

static void RecomInit(void) {
    int i;
    snprintf(sRecomPath, sizeof(sRecomPath), "%s", PortConfigPath("achievements_recom_eu.dat"));
    snprintf(sRecomExtraPath, sizeof(sRecomExtraPath), "%s", PortConfigPath("achievements_recom_extra.dat"));
    KhOfflineLoad(&sRecom, sRecomPath);
    RecomExtraLoad();
    rc_runtime_init(&sRecomRuntime);
    for (i = 0; i < KhOfficialCount(); ++i)
        if (!sRecom.when[i] &&
            rc_runtime_activate_achievement(&sRecomRuntime, KhOfficialAt(i)->id, KhOfficialAt(i)->definition, NULL, 0) == RC_OK)
            sRecomReady++;
}

/* the credits movie (mode_movie.c: 4 Sora's story, 5 Reverse/Rebirth): each difficulty up to the one played
   earns its achievement */
static int sRecomCredits; /* 0 none, else the movie */

void PortAchievementsMovie(int movieId) {
    if (movieId == 4 || movieId == 5) sRecomCredits = movieId;
}

static void RecomStoryFrame(void) {
    int rank, i, base;
    if (!sRecomCredits) return;
    base = sRecomCredits == 4 ? RECOM_SORA_BEGINNER : RECOM_RIKU_BEGINNER;
    sRecomCredits = 0;
    rank = PortRecomRunRank();
    for (i = 0; i <= rank; i++) RecomExtraUnlock(base + i);
    if (sRecomExtra[RECOM_SORA_PROUD] && sRecomExtra[RECOM_RIKU_PROUD]) RecomExtraUnlock(RECOM_MASTER);
}

static void RecomFrame(int evaluate) {
    if (!evaluate) { sRecomCredits = 0; return; }
    if (sRecomReady) rc_runtime_do_frame(&sRecomRuntime, RecomEvent, OfflinePeek, NULL, NULL);
    RecomStoryFrame();
}

static void RecomNoticesFrame(void) {
    int i, l = ACHIEVEMENT_TEXT_LANGUAGE;
    static const char* const kLabel[6] = { "RE:COM ACHIEVEMENT", "SUCCÈS RE:COM", "RE:COM-ERFOLG", "OBIETTIVO RE:COM",
        "LOGRO RE:COM", "Re:CoM 実績" };
    if (l < 0 || l > 5) l = 0;
    for (i = 0; i < KhOfficialCount(); ++i)
        if (sRecomNotices[i / 64] & (1ull << (i % 64))) {
            sRecomNotices[i / 64] &= ~(1ull << (i % 64));
            PortAchievementToast(KhOfficialText(KhOfficialAt(i), l, 0), KhOfficialText(KhOfficialAt(i), l, 1), kLabel[l], i | 0x10000 /* TOAST_FRAMED */);
        }
    for (i = 0; i < RECOM_EXTRA_COUNT; ++i)
        if (sRecomExtraNotices & (1u << i)) {
            sRecomExtraNotices &= ~(1u << i);
            PortAchievementToast(kRecomExtraText[i][l][0], kRecomExtraText[i][l][1], kLabel[l], (kRecomBadgeCell + i) | 0x10000);
        }
}

/* the Re:CoM list as the Options menu reads it */
int PortRecomUnlocked(int index) { return index >= 0 && index < KH_OFFLINE_MAX && sRecom.when[index] != 0; }
int PortRecomExtraUnlocked(int index) { return index >= 0 && index < RECOM_EXTRA_COUNT && sRecomExtra[index] != 0; }
#endif
#ifndef KH_OFFLINE_EVAL
void PortAchievementsMovie(int movieId) { (void)movieId; } /* the Re:CoM list is evaluated with the European set */
#endif
