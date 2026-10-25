#ifndef PORT_SYNTHESIS_REQUIREMENT_HD_H
#define PORT_SYNTHESIS_REQUIREMENT_HD_H

/* No ROM symbols: these are lookup identities for synthesis entry copies. */
#define SYNTHESIS_HD_BODY_COUNT 5
#define SYNTHESIS_HD_VALUES 100
#define SYNTHESIS_HD_COUNT (SYNTHESIS_HD_BODY_COUNT + 4 * SYNTHESIS_HD_VALUES)
extern const unsigned char gSynthesisHdIdentities[SYNTHESIS_HD_COUNT];

#endif
