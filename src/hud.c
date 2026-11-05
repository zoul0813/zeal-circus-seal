/*
 * HUD text and icons use this game's shared ASCII_MAP.
 */
#include <core.h>
#include "game.h"
#include <zgdk/ascii.h>

void hud_init(void) {
    // This game's font occupies tiles 32-95; unsupported characters are blank.
    mem_set(ASCII_MAP, 32, sizeof(ASCII_MAP));
    ascii_map(' ', 64, 32);
    ascii_map(HUD_BALL_ICON, 1, TILE_ICON_BALL);
}

void text_print(const char *str, uint8_t x, uint8_t y) {
    uint8_t len = 0;
    while (str[len]) len++;
    hud_tilemap_nput_s_layer(&vctx, str, len, ARENA_LAYER, x, y);
}

void text_center(const char *str, uint8_t y) {
    uint8_t len = 0;
    while (str[len]) len++;
    hud_tilemap_nput_s_layer(&vctx, str, len, ARENA_LAYER, (WIDTH - len) >> 1, y);
}

void text_clear(void) {
    tilemap_fill(&vctx, ARENA_LAYER, TILE_EMPTY, 0, 0, WIDTH, HEIGHT);
}

/* 5 digit, zero padded. zgdk_itoa takes an int, so scores above 32767 would print negative.
 * Subtracting powers of ten: % and / call __moduint/__divuint, ~4% of a frame when the score changes. */
static const uint16_t POW10[4] = { 10000, 1000, 100, 10 };
static void format5(uint16_t value, char *out) {
    uint8_t i;
    for (i = 0; i < 4; i++) {
        char c = '0';
        while (value >= POW10[i]) { value -= POW10[i]; c++; }
        out[i] = c;
    }
    out[4] = '0' + (uint8_t)value;
    out[5] = 0;
}

void score_add(uint16_t points) {
    // hiscore_t.score is 16-bit, so the whole game caps at 65535
    if ((uint16_t)(game.score + points) < game.score) game.score = 0xFFFF;
    else game.score += points;
    // an extra ball every EXTRA_BALL_EVERY points
    while (game.next_extra && game.score >= game.next_extra) {
        if (game.balls < BALLS_MAX) game.balls++;
        game_sfx_extra();
        if ((uint16_t)(game.next_extra + EXTRA_BALL_EVERY) < game.next_extra) {
            game.next_extra = 0;
            break;
        }
        game.next_extra += EXTRA_BALL_EVERY;
    }
    if (game.score > game.hi) game.hi = game.score;
    game.hud_dirty = 1;
}

void hud_draw(void) {
    // "SSSSS @B A01 HIHHHHH": score, balls, act, hiscore
    char line[WIDTH + 1];
    uint8_t i;
    uint8_t act = game.act + 1;
    for (i = 0; i < WIDTH; i++) line[i] = ' ';
    line[WIDTH] = 0;
    format5(game.score, line);
    line[5] = ' ';
    line[6] = HUD_BALL_ICON;
    line[7] = '0' + (game.balls > 9 ? 9 : game.balls);
    line[9] = 'A';
    line[10] = '0' + (act / 10);
    line[11] = '0' + (act % 10);
    line[13] = 'H';
    line[14] = 'I';
    format5(game.hi, &line[15]);
    hud_tilemap_nput_s_layer(&vctx, line, WIDTH, ARENA_LAYER, 0, HUD_ROW);
    game.hud_dirty = 0;
}
