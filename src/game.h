#include <stdint.h>
#include <zos_errors.h>
#include <zvb_gfx.h>
/* only the ZGDK headers in use: <zgdk.h> also pulls rect.h, ascii.h and music.h, whose
 * non-inline static functions SDCC copies into every file that includes them */
#include <zgdk/types.h>
#include <zgdk/input.h>
#include <zgdk/sprites.h>
#include <zgdk/tilemap.h>
#include <zgdk/tilemap/scroll.h>
#include <zgdk/hiscore.h>
#include <zgdk/utils/rand.h>
#include <zgdk/hud/tilemap.h>
#include <zgdk/sound/sounds.h>
#include <zgdk/sound/tracker.h>
#include <zgdk/physics/body.h>
#include <zgdk/tilemap/flags.h>
#include <zgdk/sound/sfx.h>
#include "bounce.h"
#include <zgdk/input.h>
#include "gfx.h"
#include "entities.h"

#ifndef GAME_H
#define GAME_H

#define WIDTH          20
#define HEIGHT         15
#define ROOM_SIZE      (WIDTH * HEIGHT)

#define BG_LAYER       0   // the big top (stripes, crowd, ring wall), sways sideways
#define ARENA_LAYER    1   // rigging, poles, balloons, pedestals, ring floor + HUD on row 0
#define HUD_ROW        0
#define BANNER_ROW     10  // rows 10-11 are always clear of balloons and pedestals

#define FLOOR_ROW      14
#define FLOOR_Y        (FLOOR_ROW * 16)

#define POOL_SIZE      16
#define ARENA_SIZE     POOL_SIZE

/* entity types */
#define E_SEAL    0
#define E_BALL    1
#define E_CLOWN   2
#define E_FISH    3
#define E_POP     4
#define E_STAR    5

/* ball states */
#define BALL_SERVE    0   // sitting still on the nose, waiting for a button
#define BALL_FREE     1   // flying
#define BALL_BALANCE  2   // caught: wobbling on the nose while the catch button is held
#define BALL_LOST     3   // touched the ring floor

/* input */
#define IN_LEFT   BUTTON_LEFT
#define IN_RIGHT  BUTTON_RIGHT
#define IN_HOP    BUTTON_B                // Z / Space, SNES B
#define IN_CATCH  (BUTTON_Y | BUTTON_A)   // X or A key, SNES Y or A
#define IN_START  BUTTON_START            // Enter, SNES Start
#define IN_QUIT   BUTTON_SELECT           // Right Shift, SNES Select

#ifndef HISCORE_PATH
#ifdef EMULATOR
#define HISCORE_PATH "H:/cseal.zhs"   // HostFS: the game folder
#else
#define HISCORE_PATH "B:/cseal.zhs"
#endif
#endif

#define ACT_COUNT    12
#define BALLS_START  3
#define BALLS_MAX    9
#define EXTRA_BALL_EVERY 10000

typedef struct {
    uint16_t score;
    uint16_t hi;
    uint16_t next_extra; // score that gives the next extra ball
    uint8_t balls;
    uint8_t act;        // 0 based
    uint8_t loop;       // times every act was cleared
    uint8_t balloons;   // balloons left in the act (a silver one counts once)
    uint8_t chain;      // balloons popped since the ball last touched the nose
    uint8_t hud_dirty;
} game_t;

extern gfx_context vctx;
extern game_t game;
extern uint16_t input_now;
extern uint16_t input_pressed;
extern uint8_t frame_count;

/* acts.c */
extern const char *const ACT_NAMES[ACT_COUNT];

/* arena.c */
void arena_init(void);
void arena_load(uint8_t act);
void arena_redraw_rows(uint8_t from, uint8_t count);
void arena_backdrop(void);
void arena_sway(void);                  // in the vblank
uint8_t arena_pop(uint8_t tx, uint8_t ty);   // ball hit tile tx,ty; 1 if it was a balloon
uint8_t arena_column_balloons(uint8_t tx);
void arena_ball_flags(void);
void arena_body_flags(void);
extern uint8_t arena_start_x;
extern uint8_t arena_cols[WIDTH];        // balloons left per column (autopilot aims with it)

/* seal.c: the seal and the ball */
void seal_init(void);
void seal_act_start(void);
void seal_update(void);
uint8_t seal_bonk(int16_t from_x);     // 1 if the seal got bonked
void seal_cheer(void);
uint8_t seal_ball_state(void);
uint8_t seal_lost_done(void);
void seal_serve(void);
game_entity_t *seal_entity(void);
game_entity_t *ball_entity(void);
void ball_bounce_up(int16_t vy);

/* actors.c: clowns, fish, effects */
void actors_init(void);
void actors_spawn_clown(uint8_t tx, uint8_t ty);
void actors_spawn_fish(int16_t px, int16_t py);
void actors_spawn_pop(int16_t px, int16_t py);
void actors_update(void);

/* hud.c */
void hud_draw(void);
/* Character glyphs mapped to icon tiles by hud_init(). */
#define HUD_BALL_ICON '@'

void hud_init(void);
void text_print(const char *str, uint8_t x, uint8_t y);
void text_center(const char *str, uint8_t y);
void text_clear(void);
void score_add(uint16_t points);

/* sfx.c (on ZGDK sound/sfx: sfx_tick() comes from there) */
void game_sfx_init(void);
void game_sfx_boop(void);
void game_sfx_hop(void);
void game_sfx_pop(void);
void game_sfx_tough(void);
void game_sfx_wall(void);
void game_sfx_catch(void);
void game_sfx_launch(void);
void game_sfx_lost(void);
void game_sfx_fish(void);
void game_sfx_honk(void);
void game_sfx_bonk(void);
void game_sfx_extra(void);

/* music.c */
#define BGM_TITLE    0
#define BGM_MARCH    1
#define BGM_READY    2
#define BGM_CLEAR    3
#define BGM_OVER     4
#define BGM_NONE     0xFF
zos_err_t bgm_init(void);
void *bgm_scratch(uint16_t *size);   // the pattern buffers, free until the first bgm_play()
void bgm_play(uint8_t song);
void bgm_stop(void);
void bgm_pause(void);
void bgm_resume(void);
uint8_t bgm_busy(void);
void bgm_tick(void);

/* autopilot.c (AUTOPILOT builds, for recording videos and frame timing) */
#ifdef AUTOPILOT
uint16_t autopilot_input(void);
#endif

#endif
