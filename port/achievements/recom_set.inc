/* The Re:CoM list's own data, shared by the game (port_achievements_recom.inc) and the launcher
 * (achievements_page.inc): the mode's own achievements and which official ones need the whole game on Proud.
 * Include after offline_achievements.h. Kept apart from the official set and never sent online. */
#include <string.h>

enum { RECOM_SORA_BEGINNER, RECOM_SORA_STANDARD, RECOM_SORA_PROUD, RECOM_RIKU_BEGINNER, RECOM_RIKU_STANDARD,
       RECOM_RIKU_PROUD, RECOM_MASTER, RECOM_EXTRA_COUNT };
/* their art in the badge atlas (port/tools/gen_recom_badges.py): row 9, the frame after them */
enum { kRecomBadgeCell = 90, kRecomFrameCell = 97 };

/* en fr de it es ja: title, description */
static const char* const kRecomExtraText[RECOM_EXTRA_COUNT][6][2] = {
    { { "Re:CoM: Sora, Beginner", "Complete Sora's story in Re:CoM on Beginner or higher." },
      { "Re:CoM : Sora, Débutant", "Termine l'histoire de Sora en Re:CoM en Débutant ou plus." },
      { "Re:CoM: Sora, Anfänger", "Schließe Soras Geschichte in Re:CoM auf Anfänger oder höher ab." },
      { "Re:CoM: Sora, Principiante", "Completa la storia di Sora in Re:CoM a Principiante o superiore." },
      { "Re:CoM: Sora, Principiante", "Completa la historia de Sora en Re:CoM en Principiante o superior." },
      { "Re:CoM：ソラ ビギナー", "Re:CoMでソラ編をビギナー以上でクリアする。" } },
    { { "Re:CoM: Sora, Standard", "Complete Sora's story in Re:CoM on Standard or higher." },
      { "Re:CoM : Sora, Normal", "Termine l'histoire de Sora en Re:CoM en Normal ou plus." },
      { "Re:CoM: Sora, Normal", "Schließe Soras Geschichte in Re:CoM auf Normal oder höher ab." },
      { "Re:CoM: Sora, Normale", "Completa la storia di Sora in Re:CoM a Normale o superiore." },
      { "Re:CoM: Sora, Estándar", "Completa la historia de Sora en Re:CoM en Estándar o superior." },
      { "Re:CoM：ソラ スタンダード", "Re:CoMでソラ編をスタンダード以上でクリアする。" } },
    { { "Re:CoM: Sora, Proud", "Complete Sora's story in Re:CoM, the whole game on Proud." },
      { "Re:CoM : Sora, Expert", "Termine l'histoire de Sora en Re:CoM, toute la partie en Expert." },
      { "Re:CoM: Sora, Profi", "Schließe Soras Geschichte in Re:CoM ab, das ganze Spiel auf Profi." },
      { "Re:CoM: Sora, Esperto", "Completa la storia di Sora in Re:CoM, tutta la partita a Esperto." },
      { "Re:CoM: Sora, Experto", "Completa la historia de Sora en Re:CoM, toda la partida en Experto." },
      { "Re:CoM：ソラ プラウド", "Re:CoMでソラ編を最初から最後までプラウドでクリアする。" } },
    { { "Re:CoM: Riku, Beginner", "Complete Reverse/Rebirth in Re:CoM on Beginner or higher." },
      { "Re:CoM : Riku, Débutant", "Termine Reverse/Rebirth en Re:CoM en Débutant ou plus." },
      { "Re:CoM: Riku, Anfänger", "Schließe Reverse/Rebirth in Re:CoM auf Anfänger oder höher ab." },
      { "Re:CoM: Riku, Principiante", "Completa Reverse/Rebirth in Re:CoM a Principiante o superiore." },
      { "Re:CoM: Riku, Principiante", "Completa Reverse/Rebirth en Re:CoM en Principiante o superior." },
      { "Re:CoM：リク ビギナー", "Re:CoMでリク編をビギナー以上でクリアする。" } },
    { { "Re:CoM: Riku, Standard", "Complete Reverse/Rebirth in Re:CoM on Standard or higher." },
      { "Re:CoM : Riku, Normal", "Termine Reverse/Rebirth en Re:CoM en Normal ou plus." },
      { "Re:CoM: Riku, Normal", "Schließe Reverse/Rebirth in Re:CoM auf Normal oder höher ab." },
      { "Re:CoM: Riku, Normale", "Completa Reverse/Rebirth in Re:CoM a Normale o superiore." },
      { "Re:CoM: Riku, Estándar", "Completa Reverse/Rebirth en Re:CoM en Estándar o superior." },
      { "Re:CoM：リク スタンダード", "Re:CoMでリク編をスタンダード以上でクリアする。" } },
    { { "Re:CoM: Riku, Proud", "Complete Reverse/Rebirth in Re:CoM, the whole game on Proud." },
      { "Re:CoM : Riku, Expert", "Termine Reverse/Rebirth en Re:CoM, toute la partie en Expert." },
      { "Re:CoM: Riku, Profi", "Schließe Reverse/Rebirth in Re:CoM ab, das ganze Spiel auf Profi." },
      { "Re:CoM: Riku, Esperto", "Completa Reverse/Rebirth in Re:CoM, tutta la partita a Esperto." },
      { "Re:CoM: Riku, Experto", "Completa Reverse/Rebirth en Re:CoM, toda la partida en Experto." },
      { "Re:CoM：リク プラウド", "Re:CoMでリク編を最初から最後までプラウドでクリアする。" } },
    { { "Master of Memories", "Complete both stories in Re:CoM on Proud." },
      { "Maître des souvenirs", "Termine les deux histoires en Re:CoM en Expert." },
      { "Meister der Erinnerungen", "Schließe beide Geschichten in Re:CoM auf Profi ab." },
      { "Maestro dei ricordi", "Completa entrambe le storie in Re:CoM a Esperto." },
      { "Maestro de los recuerdos", "Completa ambas historias en Re:CoM en Experto." },
      { "記憶のマスター", "Re:CoMで両方の物語をプラウドでクリアする。" } },
};

/* the note on official achievements that need Proud in the Re:CoM list */
static const char* const kRecomProudNote[6] = { " (whole game on Proud)", " (toute la partie en Expert)",
    " (ganzes Spiel auf Profi)", " (tutta la partita a Esperto)", " (toda la partida en Experto)",
    "(最初から最後までプラウド)" };

/* A world, the story or a boss (and its challenges): needs the whole game on Proud. Collections, sleights, the
   journal, Pooh's rewards and the first Heartless do not. Read from the set's English description, so a later set
   sorts itself. */
static int KhRecomNeedsProud(const KhOfficialAchievement* a) {
    const char* d = KhOfficialText(a, 0, 1);
    if (!strncmp(d, "Complete ", 9))
        return strstr(d, "story") != NULL; /* a world or the story; not "the Map Cards section" */
    if (!strncmp(d, "Defeat your first", 17))
        return 0;
    return !strncmp(d, "Defeat", 6) || !strncmp(d, "Finish off", 10) || !strncmp(d, "Without", 7) ||
           !strncmp(d, "Escape", 6);
}

/* achievements_recom_extra.dat: "KHREC1", then "<index> <time>" per unlock (index into the list above) */
static void KhRecomExtraParse(FILE* f, unsigned long long* when) {
    char line[64];
    memset(when, 0, sizeof(*when) * RECOM_EXTRA_COUNT);
    if (!f) return;
    if (fgets(line, sizeof(line), f) && !strncmp(line, "KHREC1", 6))
        while (fgets(line, sizeof(line), f)) {
            unsigned index; unsigned long long t;
            if (sscanf(line, "%u %llu", &index, &t) == 2 && index < RECOM_EXTRA_COUNT)
                when[index] = t ? t : 1;
        }
}
