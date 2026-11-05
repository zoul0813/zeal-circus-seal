/* Game-owned physics checks for circus-seal; moved from common host_motion. */
#include <stdio.h>
#include <stdint.h>
#include <zgdk/tilemap/flags.h>
#include <zgdk/physics/body.h>
#include "bounce.h"

static const char *MAP[15] = {
    "####################",
    "#..................#",
    "#..................#",
    "#~~~~~~~~~~~~~~~~~~#",
    "#~~~~~~~~~~~~~~~~~~#",
    "#~~~~~~~~~~~~~~~~~~#",
    "#~~~~~~~~~~~~~~~~~~#",
    "#~~~~~~~~~~~~~~~~~~#",
    "#~~~~~~~~~~~~~~~~~~#",
    "#~~~~~~~~~~~~~~~~~~#",
    "#~~~~~~~~~~~~~~~~~~#",
    "#~~~~~~~~~~~~~~~~~~#",
    "#~~~~~~~~~~~~~~~~~~#",
    "####################",
    "####################",
};

#include "zgdk/tilemap.h"

TILEFLAGS_DEFINE_CACHE(40);
Tilemap *_TILEMAP;
static Tilemap tilemap;
static uint8_t tiles[20 * 15];
static uint8_t flags[256];
static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)
#define WATER TF_USER1

int main(void) {
    body_t b;
    int x, y, i, peak;
    for (y = 0; y < 15; y++) for (x = 0; x < 20; x++) tiles[y * 20 + x] = (uint8_t)MAP[y][x];
    tilemap.tiles = tiles; tilemap.rect.w = 20; tilemap.rect.h = 15; _TILEMAP = &tilemap;
    flags['#'] = TF_SOLID; flags['~'] = WATER;
    tileflags_register(flags);

    // bounce: drop a ball, it bounces lower each time and comes to rest
    body_init(&b, 100, 40, 12, 12);
    peak = 999;
    for (i = 0; i < 600; i++) {
        b.vy += 6;
        body_bounce(&b, 12, 20);
        if (b.state & BODY_ON_GROUND) { if (b.vy < 0 && -b.vy < peak) peak = -b.vy; }
    }
    CHECK(b.vy == 0 && body_py(&b) == 13 * 16 - 12, "ball rests on the floor (%d, vy %d)", body_py(&b), b.vy);
    CHECK(peak < 999, "ball bounced");
    // wall bounce reverses vx
    body_init(&b, 280, 100, 12, 12);
    b.vx = 32;
    for (i = 0; i < 10; i++) body_bounce(&b, 16, 0);
    CHECK(b.vx == -32, "wall bounce vx %d", b.vx);
    // ceiling bounce
    body_init(&b, 100, 20, 12, 12);
    b.vy = -48;
    for (i = 0; i < 3; i++) body_bounce(&b, 8, 0);
    CHECK(b.vy == 24, "ceiling bounce vy %d", b.vy);

    printf(fails ? "host_bounce: %d FAILED\n" : "host_bounce: all passed\n", fails);
    return fails != 0;
}
