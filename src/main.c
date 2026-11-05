/*
 * Circus Seal: a single-screen ball game for Zeal 8-bit OS.
 * Keep the beach ball in the air off the seal's nose and pop every balloon under the big top.
 */
#include <zos_vfs.h>
#include <stdint.h>
#include <zos_errors.h>
#include <zos_sys.h>
#include <zos_video.h>
#include "game.h"
#include "controls.h"
#include "entities.h"
#include "assets.h"
#include <zgdk/sprites.h>

#define ACT_INTRO_FRAMES   130   // the drum roll jingle is 128 frames
#define ACT_CLEAR_FRAMES   170
#define ACT_CLEAR_BONUS    500
#define GAME_OVER_FRAMES   260   // the sad trombone is 256 frames
#ifndef START_ACT
#define START_ACT          0
#endif

gfx_context vctx;
game_t game;
entity_pool_t game_entities;
uint16_t input_now;
uint16_t input_pressed;
uint8_t frame_count;
static uint16_t input_last;
#ifdef AUTOPILOT
static uint8_t autopilot_on;
#endif

static const highscore_t DEFAULT_SCORES[HISCORES_COUNT] = {
    { { 'Z', 'E', 'L' }, 20000 }, { { 'S', 'E', 'A' }, 15000 }, { { 'L', 'I', 'O' }, 12000 },
    { { 'B', 'A', 'L' }, 10000 }, { { 'P', 'O', 'P' }, 8000 },  { { 'Z', 'G', 'D' }, 6000 },
    { { 'Z', 'V', 'B' }, 4000 },  { { 'Z', 'O', 'S' }, 3000 },  { { 'Z', 'D', 'E' }, 2000 },
    { { 'H', 'O', 'N' }, 1000 },
};

static const hiscore_config_t HISCORE_CONFIG = {
    .context = &vctx,
    .width = WIDTH,
    .height = HEIGHT,
    .layer = ARENA_LAYER,
    .empty_tile = TILE_EMPTY,
    .default_scores = DEFAULT_SCORES,
    .path = HISCORE_PATH,
};

static void fatal(zos_err_t err, const char *what) {
    // printf would pull in ~2.6 KB of formatting code
    uint16_t len = 0;
    char code[4] = { ' ', '0' + (err / 10) % 10, '0' + err % 10, '\n' };
    ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
    while (what[len]) len++;
    write(DEV_STDOUT, what, &len);
    len = sizeof(code);
    write(DEV_STDOUT, code, &len);
    exit(err);
}

static void read_input(void) {
    input_now = input_read();
#ifdef AUTOPILOT
    if (autopilot_on) input_now = (input_now & IN_START) | autopilot_input();
#endif
    input_pressed = input_now & ~input_last;
    input_last = input_now;
}

static void frame_end(void) {
    physics_entities_sync(&game_entities);
    gfx_wait_vblank(&vctx);
    sprites_render(&vctx);
    arena_sway();
    if (game.hud_dirty) hud_draw();
    gfx_wait_end_vblank(&vctx);
    bgm_tick();
    sfx_tick();
    frame_count++;
}

static void wait_frames(uint8_t frames) {
    while (frames--) {
        read_input();
        frame_end();
    }
}

static void init(void) {
    zos_err_t err;

    err = input_init(1);
    if (err) fatal(err, "input");

    gfx_enable_screen(0);
    err = gfx_initialize(ZVB_CTRL_VID_MODE_GFX_320_8BIT, &vctx);
    if (err) fatal(err, "gfx");
    err = sprites_clear(&vctx);   // a previous program may have left sprites in the table
    if (err) fatal(err, "sprites");
    err = zar_file_open(PACK_FILE, &pack);   // circusseal.zar, next to the .bin
    if (err) fatal(err, PACK_FILE);
    err = load_palette(&vctx);
    if (err) fatal(err, "palette");
    err = load_tileset(&vctx);
    if (err) fatal(err, "tileset");
    hud_init();
    tilemap_fill(&vctx, BG_LAYER, TILE_EMPTY, 0, 0, 80, 40);
    tilemap_fill(&vctx, ARENA_LAYER, TILE_EMPTY, 0, 0, 80, 40);
    arena_backdrop();

    actors_init();
    arena_init();
    sound_init_hardware();       // resets every voice, so it goes before the music and the effects
    err = bgm_init();
    if (err) fatal(err, "music");
    game_sfx_init();
    hiscore_init(&HISCORE_CONFIG);
    rand8_seed(0x5EA1);

    gfx_enable_screen(1);
}

static void deinit(void) {
    bgm_stop();
    sound_deinit();
    zar_file_close(&pack);
    tilemap_scroll_x(BG_LAYER, 0);
    (void)sprites_clear(&vctx);   // best-effort cleanup before returning to ZOS
    ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
}

static void title_page(void) {
    // dark sky behind the title text, the big top only from the crowd down
    tilemap_fill(&vctx, BG_LAYER, TILE_EMPTY, 0, 0, 24, BANNER_ROW);
    text_clear();
    tilemap_fill(&vctx, ARENA_LAYER, TILE_FLOOR, 0, FLOOR_ROW, WIDTH, 1);
    text_center("CIRCUS SEAL", 2);
    text_center("BOUNCE THE BALL", 4);
    text_center("POP THE BALLOONS", 5);
    text_center(CONTROL_PROMPT("Z HOP  X CATCH", PAD_B " HOP  " PAD_A " CATCH"), 7);
    text_center(PROMPT_START, 10);
    seal_entity()->entity.physics.visual.hidden = 0;
    ball_entity()->entity.physics.visual.hidden = 0;
}

/* Title screen, alternating with the hiscore table. Returns 0 to quit. */
static uint8_t title(void) {
    uint16_t t = 0;
    uint8_t page = 0xFF;
    uint16_t seed = 0;

    entity_pool_clear(&game_entities);
    arena_start_x = 9;
    seal_act_start();
    game.hud_dirty = 0;
    input_flush();
    controls_menu_begin();
    bgm_play(BGM_TITLE);

    while (1) {
        uint8_t want = (t / 300) & 1;
        if (want != page) {
            page = want;
            if (page == 0) {
                title_page();
            } else {
                seal_entity()->entity.physics.visual.hidden = 1;
                ball_entity()->entity.physics.visual.hidden = 1;
                tilemap_fill(&vctx, BG_LAYER, TILE_EMPTY, 0, 0, 24, HEIGHT);
                hiscore_show();
                text_center("HIGH SCORES", 1);
                text_center(PROMPT_START, 14);
            }
        }
        read_input();
        if (controls_menu_update()) page = 0xFF;
        seed++;
        if (input_pressed & IN_START) break;
        if (input_pressed & IN_QUIT) return 0;
#ifdef AUTOPILOT
        if (t == 120) break;   // the bot starts on its own
#endif
        frame_end();
        t++;
    }
    rand8_seed(seed | 1);
    return 1;
}

static void act_start(void) {
    char text[] = "ACT 00";
    uint8_t act = game.act + 1;

    entity_pool_clear(&game_entities);
    text_clear();
    arena_backdrop();
    arena_load(game.act);
    seal_act_start();
    game.hud_dirty = 1;

    text[4] = '0' + (act / 10);
    text[5] = '0' + (act % 10);
    text_center(text, BANNER_ROW);
    text_center(ACT_NAMES[game.act], BANNER_ROW + 1);
    bgm_play(BGM_READY);
    wait_frames(ACT_INTRO_FRAMES);
    bgm_play(BGM_MARCH);
    arena_redraw_rows(BANNER_ROW, 2);
}

static void pause(void) {
    text_center("PAUSED", BANNER_ROW);
    bgm_pause();
    input_flush();
    input_last = 0;
    do {
        read_input();
        frame_end();
    } while (!(input_pressed & IN_START));
    arena_redraw_rows(BANNER_ROW, 1);
    bgm_resume();
}

static void play(void) {
    uint8_t clear_timer = 0;
    uint16_t over_timer = 0;

    game.score = 0;
    game.balls = BALLS_START;
    game.next_extra = EXTRA_BALL_EVERY;
    game.act = START_ACT;
    game.loop = 0;
    game.chain = 0;
#ifdef AUTOPILOT
    autopilot_on = 1;
#endif
    {
        uint16_t top = hiscore_get(0)->score;
        game.hi = top ? top : DEFAULT_SCORES[0].score;
    }
    act_start();

    while (1) {
        read_input();
        if (input_pressed & IN_START) pause();
        if (input_pressed & IN_QUIT) break;

        seal_update();
        actors_update();

        if (over_timer) {
            if (++over_timer > GAME_OVER_FRAMES) break;
        } else if (clear_timer) {
            if (++clear_timer >= ACT_CLEAR_FRAMES) {
                clear_timer = 0;
                game.act++;
                if (game.act >= ACT_COUNT) {
                    game.act = 0;
                    game.loop++;
                }
                act_start();
            }
        } else if (game.balloons == 0) {
            seal_cheer();
            score_add(ACT_CLEAR_BONUS);
            text_center("BRAVO!", BANNER_ROW);
            bgm_play(BGM_CLEAR);
            clear_timer = 1;
        } else if (seal_lost_done()) {
            if (game.balls == 0) {
                text_center("GAME OVER", BANNER_ROW);
                bgm_play(BGM_OVER);
                over_timer = 1;
            } else {
                seal_serve();
            }
        }
        frame_end();
    }

    entity_pool_clear(&game_entities);
    physics_entities_sync(&game_entities);
    bgm_stop();
#ifdef AUTOPILOT
    autopilot_on = 0;
#endif
    frame_end();
    input_flush();
    hiscore_add(game.score);
    input_flush();
}

int main(void) {
    init();
    while (title()) play();
    deinit();
    return 0;
}
