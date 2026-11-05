#include <zgdk/tilemap/flags.h>
/*
 * The ring: native Tiled/ZTM act loading, balloons as layer 1 tiles, the big top
 * backdrop on layer 0 and its sway.
 *
 * Two flags tables share one tilemap: bodies (seal, clowns, fish) stand on the pedestal tops,
 * while the ball flies through them and bounces off balloons, which are TF_SOLID | TF_USER1.
 */
#include <core.h>
#include "game.h"
#include "assets.h"
#include "map_data.h"

TILEFLAGS_DEFINE_CACHE(15);

#define BG_COLS 24   // the sway shows up to 8 px past the right edge

uint8_t arena_start_x;
uint8_t arena_cols[WIDTH];

static uint8_t room_tiles[ROOM_SIZE];
static Tilemap tilemap;
static const uint8_t body_flags[256] = { TILE_FLAGS_INIT [TILE_PED_TOP] = TF_ONEWAY };
static const uint8_t ball_flags[256] = { TILE_FLAGS_INIT };
static uint8_t sway_x = 0xFF;

/* points per balloon, from TILE_BAL_RED; multiplied by the chain */
static const uint8_t POINTS[] = { 10, 20, 30, 50, 80, 50, 100 };

void arena_init(void) {
    // collision only looks at the ring; nothing leaves it
    tileflags_set_outside(TF_SOLID);
    tileflags_register(body_flags);
}

void arena_ball_flags(void) { tileflags_register(ball_flags); }
void arena_body_flags(void) { tileflags_register(body_flags); }

void arena_backdrop(void) {
    uint8_t line[BG_COLS];
    uint8_t x, y, t;
    for (y = 0; y < HEIGHT; y++) {
        for (x = 0; x < BG_COLS; x++) {
            if (y == 0) t = TILE_EMPTY;
            else if (y == 2) t = BG_VALANCE;
            else if (y < 10) t = BG_STRIPE;
            else if (y == 10) t = BG_CROWD1;
            else if (y == 11) t = BG_CROWD2;
            else if (y == 12) t = (x & 1) ? BG_WALL2 : BG_WALL;
            else t = BG_WALL2;
            line[x] = t;
        }
        gfx_tilemap_load(&vctx, line, BG_COLS, BG_LAYER, 0, y);
    }
}

static void read_act(uint8_t act) {
    zar_entry_t file;
    map_objects_t objects;
    uint16_t size = ROOM_SIZE;
    uint8_t i;
    if (zar_entry_from_name(&pack, "acts.ztm", &file) != ERR_SUCCESS ||
        zar_entry_read_at(&pack, &file, (uint16_t)act * ROOM_SIZE, room_tiles, &size) != ERR_SUCCESS || size != ROOM_SIZE)
        mem_set(room_tiles, TILE_EMPTY, ROOM_SIZE);
    size = sizeof(objects);
    if (zar_entry_from_name(&pack, "acts.spn", &file) != ERR_SUCCESS ||
        zar_entry_read_at(&pack, &file, (uint16_t)act * sizeof(objects), &objects, &size) != ERR_SUCCESS ||
        size != sizeof(objects) || objects.count > sizeof(objects.objects) / sizeof(objects.objects[0])) return;
    for (i = 0; i < objects.count; i++) {
        const map_object_t *o = &objects.objects[i];
        if (o->x >= WIDTH || o->y >= HEIGHT) continue;
        if (o->kind == MAP_PLAYER) arena_start_x = o->x;
        else if (o->kind == MAP_CLOWN) actors_spawn_clown(o->x, o->y);
    }
}

void arena_load(uint8_t act) {
    uint16_t i;
    uint8_t x;
    arena_start_x = 9;
    arena_body_flags();
    read_act(act);
    if (tilemap_load(&vctx, &tilemap, room_tiles, WIDTH, HEIGHT, ARENA_LAYER, 0, 0) == ERR_SUCCESS)
        tileflags_refresh();
    game.balloons = 0;
    for (x = 0; x < WIDTH; x++) arena_cols[x] = 0;
    x = 0;
    for (i = 0; i < ROOM_SIZE; i++) {
        if (ball_flags[room_tiles[i]] & TF_USER1) {
            game.balloons++;
            arena_cols[x]++;
        }
        if (++x == WIDTH) x = 0;
    }
}

/* Redraw rows of the act (after a text banner) without re-running the spawns */
void arena_redraw_rows(uint8_t from, uint8_t count) {
    uint8_t y;
    for (y = from; y < from + count; y++) {
        gfx_tilemap_load(&vctx, room_tiles + (y * WIDTH), WIDTH, ARENA_LAYER, 0, y);
    }
}

uint8_t arena_column_balloons(uint8_t tx) {
    return tx < WIDTH ? arena_cols[tx] : 0;
}

static void set_tile(uint8_t tx, uint8_t ty, uint8_t tile) {
    room_tiles[(uint16_t)ty * WIDTH + tx] = tile;
    gfx_tilemap_place(&vctx, tile, ARENA_LAYER, tx, ty);
}

uint8_t arena_pop(uint8_t tx, uint8_t ty) {
    uint8_t tile, chain;
    uint16_t points;
    if (tx >= WIDTH || ty >= HEIGHT) return 0;
    tile = room_tiles[(uint16_t)ty * WIDTH + tx];
    if (!(ball_flags[tile] & TF_USER1)) return 0;

    if (tile == TILE_BAL_SILVER) {
        // tough balloon: the first hit only dents it into a red one
        set_tile(tx, ty, TILE_BAL_RED);
        score_add(POINTS[TILE_BAL_SILVER - TILE_BAL_RED] >> 2);
        game_sfx_tough();
        return 1;
    }
    set_tile(tx, ty, TILE_EMPTY);
    game.balloons--;
    arena_cols[tx]--;
    if (game.chain < 8) game.chain++;
    chain = game.chain;
    points = POINTS[tile - TILE_BAL_RED];
    while (--chain) points += POINTS[tile - TILE_BAL_RED];
    score_add(points);
    actors_spawn_pop(tx << 4, ty << 4);
    if (tile == TILE_BAL_FISH) {
        actors_spawn_fish(tx << 4, ty << 4);
        game_sfx_pop();
    } else if (tile == TILE_BAL_HEART) {
        if (game.balls < BALLS_MAX) game.balls++;
        game.hud_dirty = 1;
        game_sfx_extra();
    } else {
        game_sfx_pop();
    }
    return 1;
}

/* The tent sways a few pixels; called inside the vblank */
void arena_sway(void) {
    uint8_t phase = (frame_count >> 3) & 15;
    uint8_t x = phase < 8 ? phase : 15 - phase;
    if (x == sway_x) return;
    sway_x = x;
    tilemap_scroll_x(BG_LAYER, x);
}
