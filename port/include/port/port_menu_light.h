#ifndef PORT_MENU_LIGHT_H
#define PORT_MENU_LIGHT_H
#include <stdint.h>

/* All HD menu lamps use Press Start's five authored brightness samples.
   Interpolate each eight-tick leg, then reverse without a hard endpoint. */
static inline void PortMenuLightFrames(uint64_t ticks, int* a, int* b, float* blend) {
    static const unsigned char sequence[8] = {0,1,2,3,4,3,2,1};
    unsigned position = (unsigned)(ticks % 64);
    float t = (position % 8) / 8.0f;
    *a = sequence[position / 8];
    *b = sequence[(position / 8 + 1) % 8];
    *blend = t * t * (3.0f - 2.0f * t);
}
#endif
