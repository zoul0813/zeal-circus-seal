#include <zgdk/physics/platformer.h>
#include <stdint.h>
#include "bounce.h"

static int16_t scale(int16_t v, uint8_t keep) {
    // keep/16 without a 16-bit multiply for the common cases
    if (keep >= 16) return v;
    if (keep == 12) return v - (v >> 2);
    if (keep == 8) return v >> 1;
    return (int16_t)(((int32_t)v * keep) >> 4);
}

void body_bounce(body_t *body, uint8_t keep, int16_t rest) {
    int16_t vx = body->vx, vy = body->vy;
    body_step(body);
    if (body->state & BODY_HIT_WALL) body->vx = -scale(vx, keep);
    if ((body->state & BODY_HIT_CEIL) && vy < 0) body->vy = -scale(vy, keep);
    if ((body->state & BODY_ON_GROUND) && vy > 0) {
        body->vy = vy > rest ? -scale(vy, keep) : 0;
    }
}
