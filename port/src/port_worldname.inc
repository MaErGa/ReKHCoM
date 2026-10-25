/* World select (mode_worldselect.c): the world's name in HD text (art review, 24/10: "ver si podemos pasar textos a
 * hd, ya que estan sobre fondo plano"). The cartridge draws the name as a picture, 18 x 3 tiles at character block 0 +
 * 1024 (WorldselectDrawName; BG1 shows it); with HD text on, its letters give way to the panel's colour and the same name is
 * laid out in the HD font, centred where the picture centres it, on the picture's baseline. The names are the ones
 * the pictures show (EU: English, French, German, Italian, Spanish; US: English). Japanese keeps its picture.
 * Included by port_ppu.c. */
#define WORLDNAME_TILE0 32  /* (GetBgCharBase(0) + 1024) / 32: character block 0 */
#define WORLDNAME_TILES 54  /* 18 x 3 */
#define WORLDNAME_W 144

/* by WorldId (1..13), then English, French, German, Italian, Spanish (cp1252) */
static const char* const kWorldNames[14][5] = {
    { 0 },
    { "Agrabah", "Agrabah", "Agrabah", "Agrabah", "Agrabah" },
    { "Atlantica", "Atlantica", "Atlantica", "Atlantica", "Atl\xE1ntica" },
    { "Olympus Coliseum", "Colis\xE9" "e de l\x92Olympe", "Arena des Olymps", "Il Monte Olimpo", "Coliseo del Olimpo" },
    { "Wonderland", "Pays des Merveilles", "Wunderland", "Il Paese delle Meraviglie", "Pa\xEDs de las Maravillas" },
    { "Monstro", "Monstro", "Monstro", "La balena", "Monstruo" },
    { "Halloween Town", "Ville d\x92Halloween", "Halloween Town", "La Citt\xE0 di Halloween", "Ciudad de Halloween" },
    { "Never Land", "Pays Imaginaire", "Nimmerland", "L\x92Isola che non c\x92\xE8", "Pa\xEDs de Nunca Jam\xE1s" },
    { "Hollow Bastion", "Forteresse Oubli\xE9" "e", "Hollow Bastion", "La Fortezza Oscura", "Basti\xF3n Hueco" },
    { "Destiny Islands", "\xCEle du Destin", "Insel des Schicksals", "L\x92Isola del destino", "Islas del Destino" },
    { "Traverse Town", "Ville de Traverse", "Die Stadt Traverse", "La Citt\xE0 di mezzo", "Ciudad de Paso" },
    { "Twilight Town", "Cit\xE9 du cr\xE9puscule", "Dimmerstadt", "Crepuscopoli", "Villa Crep\xFAsculo" },
    { "Castle Oblivion", "Manoir Oblivion", "Schloss des Entfallens", "Il Castello dell\x92Oblio", "Castillo del Olvido" },
    { "100 Acre Wood", "For\xEAt des R\xEAves Bleus", "100-Morgen-Wald", "Il Bosco dei 100 Acri", "Bosque de los 100 Acres" },
};

static struct {
    const char* text;               /* NULL: no name shown in HD */
    uint8_t image[WORLDNAME_TILES * 32]; /* the picture as the game copies it to VRAM */
    uint8_t panel, face, shadow;    /* palette indices: the panel, the letters' two colours */
    uint8_t top;                    /* the letters' first ink row in the picture */
    uint32_t emitted;               /* frame + 1 the glyphs were laid out on */
} sWorldName;
static int sWorldNameOn; /* this frame: VRAM holds the picture, HD text is on */

/* mode_worldselect.c: the name picture just built (width: its half-width revealed so far, 9 = whole) */
void PortWorldNameShow(int world, int width, const void* image) {
#if defined(VERSION_JP)
    (void)world; (void)width; (void)image;
    sWorldName.text = NULL;
#else
#ifdef VERSION_EU
    extern uint32_t gLanguage;
    static const int kColumn[5] = { 0, 1, 2, 3, 4 }; /* LANGUAGE_ENGLISH..LANGUAGE_SPANISH */
    int lang = gLanguage < 5 ? kColumn[gLanguage] : 0;
#else
    int lang = 0;
#endif
    int count[16] = { 0 }, fills[16] = { 0 }, i, x, y, panel = -1, ink[2] = { -1, -1 };
    const uint8_t* p = (const uint8_t*)image;
    sWorldName.text = NULL;
    if (width < 9 || world < 1 || world > 13 || !p)
        return; /* while it opens from the middle, the picture as on the cartridge */
    memcpy(sWorldName.image, p, sizeof(sWorldName.image));
    /* the panel and its rims fill whole rows of the picture (but for its rounded ends); the letters' face and shadow never do */
    for (y = 0; y < 24; y++) {
        int row[16] = { 0 };
        for (x = 0; x < WORLDNAME_W; x++) {
            const uint8_t* q = p + ((y / 8) * 18 + x / 8) * 32 + (y & 7) * 4 + (x & 7) / 2;
            int c = (x & 1) ? *q >> 4 : *q & 15;
            count[c]++;
            row[c]++;
        }
        for (i = 1; i < 16; i++)
            if (row[i] * 10 >= WORLDNAME_W * 9) fills[i] = 1; /* nine tenths of the row: the rounded ends are clear */
    }
    for (i = 1; i < 16; i++) {
        if (!count[i]) continue;
        if (fills[i]) { if (panel < 0 || count[i] > count[panel]) panel = i; }
        /* the letters: the two commonest of the rest (a few pixels of other colours round the panel's ends) */
        else if (ink[0] < 0 || count[i] > count[ink[0]]) { ink[1] = ink[0]; ink[0] = i; }
        else if (ink[1] < 0 || count[i] > count[ink[1]]) ink[1] = i;
    }
    if (panel < 0 || ink[0] < 0)
        return;
    sWorldName.panel = (uint8_t)panel;
    sWorldName.face = (uint8_t)ink[0];
    sWorldName.shadow = (uint8_t)(ink[1] >= 0 ? ink[1] : ink[0]);
    /* first row with letter ink */
    sWorldName.top = 0;
    for (y = 0; y < 24; y++) {
        int found = 0;
        for (x = 0; x < WORLDNAME_W && !found; x++) {
            const uint8_t* q = p + ((y / 8) * 18 + x / 8) * 32 + (y & 7) * 4 + (x & 7) / 2;
            int c = (x & 1) ? *q >> 4 : *q & 15;
            found = c == ink[0] || c == ink[1];
        }
        if (found) { sWorldName.top = (uint8_t)y; break; }
    }
    sWorldName.text = kWorldNames[world][lang];
#endif
}

/* frame start: on only while VRAM holds the picture registered (the game copies it at the next vertical blank) */
static void WorldNameFrame(void) {
    sWorldNameOn = 0;
    if (!sHdText || !sWorldName.text)
        return;
    {
        unsigned base = WORLDNAME_TILE0 * 32;
        if (!memcmp(VRAM8 + base, sWorldName.image, sizeof(sWorldName.image)))
            sWorldNameOn = 1;
    }
}

/* the name's HD glyphs, laid out when a layer draws the picture's first tile at (x, gy) of the line */
static void WorldNameEmit(int bg, int x, int gy, int pal, int priority) {
    const uint16_t* widths = PortLatinGlyphWidths();
    const unsigned char* s = (const unsigned char*)sWorldName.text;
    int total = 0, pen, i;
    uint16_t a = PLTT16[pal * 16 + sWorldName.face] & 0x7FFF, b = PLTT16[pal * 16 + sWorldName.shadow] & 0x7FFF;
    int la = (a & 31) * 2 + ((a >> 5) & 31) * 5 + ((a >> 10) & 31);
    int lb = (b & 31) * 2 + ((b >> 5) & 31) * 5 + ((b >> 10) & 31);
    if (sWorldName.emitted == PortFrameCount() + 1)
        return;
    sWorldName.emitted = PortFrameCount() + 1;
    for (i = 0; s[i]; i++)
        total += widths[s[i]];
    pen = x + (WORLDNAME_W - total) / 2;
    for (i = 0; s[i] && BgTextRoom(); i++) {
        BgTextGlyph* glyph;
        int xx = pen, yy = gy + sWorldName.top - 4; /* the font's capitals start on row 4 of their 16-pixel cell */
        pen += widths[s[i]];
        if (s[i] == ' ')
            continue;
        glyph = BgTextNew();
        glyph->bg = (uint8_t)bg;
        glyph->priority = (uint8_t)priority;
        glyph->screen = sOverlay && sScreenBg[bg];
        if (glyph->screen) {
            xx -= gPortScreen.originX;
            if (sBgSplit[bg]) xx += xx < sBgSplit[bg] ? -PortScreenMarginX() : PortScreenMarginX();
            xx += (gPortOverlayW - GBA_SCREEN_W) / 2;
        } else yy += gPortScreen.originY;
        memset(&glyph->glyph, 0, sizeof(glyph->glyph));
        glyph->glyph.x = xx; glyph->glyph.y = yy;
        glyph->glyph.code = s[i];
        glyph->glyph.style = 0;
        glyph->glyph.face = la >= lb ? a : b;
        glyph->glyph.shadow = la >= lb ? b : a;
        memset(glyph->ink, 0xFF, sizeof(glyph->ink)); /* the occlusion test looks at the letter's middle */
    }
}
