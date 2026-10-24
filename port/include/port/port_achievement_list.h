#ifndef PORT_ACHIEVEMENT_LIST_H
#define PORT_ACHIEVEMENT_LIST_H
/* Read-only menu view of the official set: earned in this game (kept in the
 * data folder) or on the RetroAchievements account the launcher last read.
 * No credentials, networking or award submission belongs to this interface. */
typedef struct PortAchievementInfo {
    char title[192], description[768];
    char badge[8]; /* validated public cache basename */
    unsigned points;
    int state; /* 0 locked, 1 unlocked */
} PortAchievementInfo;
void PortAchievementsListRefresh(void);
int PortAchievementsListCount(void);
int PortAchievementsListGet(int index, PortAchievementInfo* out);
/* The Re:CoM list (port_achievements_recom.inc): the official set again, then the mode's own; never online.
   badgeIndex: the official badge, -1 for the mode's own. */
int PortRecomListCount(void);
int PortRecomListGet(int index, PortAchievementInfo* out, int* badgeIndex);
#endif
