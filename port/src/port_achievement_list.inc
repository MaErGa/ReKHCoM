/* The official set as the Options menu lists it: this game's own unlocks
   (achievements_official_eu.dat) and, when the launcher has read the account,
   the account's (cache/achievement-unlocks.dat: public ids only). */
#include "port/port_achievement_list.h"
static unsigned char sAccountUnlocked[KH_OFFLINE_MAX];
static int AchievementReadU32(FILE* file, uint32_t* value) {
    unsigned char b[4];
    if (fread(b, 1, 4, file) != 4) return 0;
    *value = b[0] | (uint32_t)b[1]<<8 | (uint32_t)b[2]<<16 | (uint32_t)b[3]<<24;
    return 1;
}
void PortAchievementsListRefresh(void) {
    FILE* file;
    char signature[8];
    uint32_t count, known, id;
    int i, index;
    memset(sAccountUnlocked, 0, sizeof(sAccountUnlocked));
    file = fopen(PortConfigPath("cache/achievement-unlocks.dat"), "rb");
    if (!file) return;
    if (fread(signature, 1, 8, file) == 8 && !memcmp(signature, "KHRAPRG1", 8) &&
        AchievementReadU32(file, &known) && known == 1 && AchievementReadU32(file, &count) && count <= 2000)
        for (i = 0; i < (int)count && AchievementReadU32(file, &id); ++i)
            if ((index = KhOfficialIndex(id)) >= 0) sAccountUnlocked[index] = 1;
    fclose(file);
}
int PortAchievementsListCount(void) { return KhOfficialCount(); }
int PortAchievementsListGet(int index, PortAchievementInfo* out) {
    const KhOfficialAchievement* a = KhOfficialAt(index);
    if (!out || !a) return 0;
    SDL_strlcpy(out->title, KhOfficialText(a, ACHIEVEMENT_TEXT_LANGUAGE, 0), sizeof(out->title));
    SDL_strlcpy(out->description, KhOfficialText(a, ACHIEVEMENT_TEXT_LANGUAGE, 1), sizeof(out->description));
    if (SDL_strlen(a->badge) < sizeof(out->badge)) SDL_strlcpy(out->badge, a->badge, sizeof(out->badge));
    else out->badge[0] = 0;
    out->points = a->points;
    out->state = sAccountUnlocked[index];
#ifdef KH_OFFLINE_EVAL
    out->state |= sOffline.when[index] != 0;
#endif
    return 1;
}
/* The Re:CoM list: the official set as earned in Re:CoM games (Proud noted where it is needed), then the mode's own */
int PortRecomListCount(void) {
#ifdef KH_OFFLINE_EVAL
    return KhOfficialCount() + PortRecomExtraCount();
#else
    return 0;
#endif
}
int PortRecomListGet(int index, PortAchievementInfo* out, int* badgeIndex) {
#ifdef KH_OFFLINE_EVAL
    int l = ACHIEVEMENT_TEXT_LANGUAGE;
    if (!out || index < 0 || index >= PortRecomListCount()) return 0;
    if (l < 0 || l > 5) l = 0;
    memset(out, 0, sizeof(*out));
    if (index < KhOfficialCount()) {
        const KhOfficialAchievement* a = KhOfficialAt(index);
        SDL_strlcpy(out->title, KhOfficialText(a, l, 0), sizeof(out->title));
        SDL_snprintf(out->description, sizeof(out->description), "%s%s", KhOfficialText(a, l, 1),
                     PortRecomNeedsProud(index) ? kRecomProudNote[l] : "");
        out->points = a->points;
        out->state = PortRecomUnlocked(index);
        if (badgeIndex) *badgeIndex = index;
    } else {
        int e = index - KhOfficialCount();
        SDL_strlcpy(out->title, PortRecomExtraText(e, l, 0), sizeof(out->title));
        SDL_strlcpy(out->description, PortRecomExtraText(e, l, 1), sizeof(out->description));
        out->state = PortRecomExtraUnlocked(e);
        if (badgeIndex) *badgeIndex = kRecomBadgeCell + e; /* the mode's own art, row 9 of the atlas */
    }
    return 1;
#else
    (void)index; (void)out; (void)badgeIndex;
    return 0;
#endif
}
