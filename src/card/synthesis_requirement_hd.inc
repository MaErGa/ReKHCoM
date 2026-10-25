/* PC-only synthesis metadata. The real OAM frame, VRAM and palette stay intact.
 * Only these task-local entry copies receive an HD lookup identity. */
#include "sprite.h"
#include "port/port_hdsprites.h"
#include "port/synthesis_requirement_hd.h"

const unsigned char gSynthesisHdIdentities[SYNTHESIS_HD_COUNT] = {0};
static EventKey sSynthesisHdKeys[4];
static ObjTiles sSynthesisHdEntryTiles[4][2];

static void SynthesisHdRemember(int index, const EventKey* key) {
    if ((unsigned)index < 4)
        sSynthesisHdKeys[index] = *key;
}

static void SynthesisHdTag(SelmapEventKeyWork* work, int index, int first) {
    const EventKey* key;
    EventKeyCard* card;
    unsigned value;
    const unsigned char *body, *requirement = NULL;
    int j;

    if ((unsigned)index >= 4)
        return;
    key = &sSynthesisHdKeys[index];
    card = &work->cards[index];
    value = key->rule == EVENT_KEY_RULE_TOTAL ? card->drawnTotal : key->value;

    /* Specific key-card illustrations retain their existing card renderer.
     * Unsupported values/rules keep the complete original, never a partial HD card. */
    if (key->kind != MAP_CARD_NONE || key->color > MAP_CARD_COLOR_GOLD ||
        key->rule > EVENT_KEY_RULE_TOTAL || card->sprite.tiles2 == NULL)
        return;
    body = &gSynthesisHdIdentities[key->color];
    if (!PortHdSpriteFind(body, NULL))
        return; /* includes HD off and assets not staged */
    if (key->rule != EVENT_KEY_RULE_NONE) {
        if (value >= SYNTHESIS_HD_VALUES || card->sprite.tiles3 == NULL)
            return;
        requirement = &gSynthesisHdIdentities[SYNTHESIS_HD_BODY_COUNT +
            (key->rule - EVENT_KEY_RULE_AT_LEAST) * SYNTHESIS_HD_VALUES + value];
        if (!PortHdSpriteFind(requirement, NULL))
            return;
    }
    for (j = first; j < gSpriteWork->entryCount; j++) {
        SpriteEntry* entry = &gSpriteWork->entries[j];
        int part = entry->tiles == card->sprite.tiles2 ? 0 :
            (entry->tiles == card->sprite.tiles3 && requirement != NULL ? 1 : -1);
        ObjTiles* copy;
        if (part < 0)
            continue;
        copy = &sSynthesisHdEntryTiles[index][part];
        /* DrawSprite has already submitted the original DMA. EmitOam reads
         * index/allocated from this copy and src only for PortOamSetHdPart.
         * These copies live through sorting/emission; never free or load them. */
        *copy = *(ObjTiles*)entry->tiles;
        copy->src = part == 0 ? body : requirement;
        entry->tiles = copy;
    }
}
