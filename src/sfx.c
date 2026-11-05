/*
 * Sound effects: tables for ZGDK's sequencer (zgdk/sound/sfx.h) on voice 3.
 * Voices 0-2 belong to the ZMT music (music.c), which never writes voice 3.
 * Priorities keep a hop from cutting a pop or the lost-ball slide (sfx_play_prio).
 */
#include "game.h"

static const sfx_seg S_BOOP[]   = { SFX_SEG(WAV_TRIANGLE, 330, 660, 4), SFX_SEG(WAV_TRIANGLE, 660, 880, 3), SFX_END };
static const sfx_seg S_HOP[]    = { SFX_SEG(WAV_SQUARE, 220, 440, 6), SFX_END };
static const sfx_seg S_POP[]    = { SFX_SEG(WAV_NOISE, 4000, 1200, 5), SFX_END };
static const sfx_seg S_TOUGH[]  = { SFX_SEG(WAV_SQUARE, 1400, 1400, 2), SFX_SEG(WAV_NOISE, 2400, 2400, 2), SFX_END };
static const sfx_seg S_WALL[]   = { SFX_SEG(WAV_TRIANGLE, 200, 160, 3), SFX_END };
static const sfx_seg S_CATCH[]  = { SFX_SEG(WAV_TRIANGLE, 880, 660, 4), SFX_SEG(WAV_TRIANGLE, 660, 660, 3), SFX_END };
static const sfx_seg S_LAUNCH[] = { SFX_SEG(WAV_SQUARE, 300, 1200, 8), SFX_END };
static const sfx_seg S_LOST[]   = { SFX_SEG(WAV_SQUARE, 600, 400, 10), SFX_SEG(WAV_SQUARE, 400, 200, 12), SFX_SEG(WAV_NOISE, 600, 200, 10), SFX_END };
static const sfx_seg S_FISH[]   = { SFX_SEG(WAV_SQUARE, 988, 988, 3), SFX_SEG(WAV_SQUARE, 1319, 1319, 3), SFX_SEG(WAV_SQUARE, 1976, 1976, 6), SFX_END };
static const sfx_seg S_HONK[]   = { SFX_SEG(WAV_SAWTOOTH, 260, 240, 6), SFX_SEG(WAV_SAWTOOTH, 330, 300, 8), SFX_END };
static const sfx_seg S_BONK[]   = { SFX_SEG(WAV_SQUARE, 900, 300, 10), SFX_SEG(WAV_NOISE, 900, 300, 6), SFX_END };
static const sfx_seg S_EXTRA[]  = { SFX_SEG(WAV_TRIANGLE, 523, 523, 4), SFX_SEG(WAV_TRIANGLE, 659, 659, 4),
                                    SFX_SEG(WAV_TRIANGLE, 784, 784, 4), SFX_SEG(WAV_TRIANGLE, 1047, 1047, 8), SFX_END };

void game_sfx_init(void) {
    sfx_init(VOICE3, VOL_75);
}

void game_sfx_wall(void)   { sfx_play_prio(S_WALL, 0); }
void game_sfx_hop(void)    { sfx_play_prio(S_HOP, 0); }
void game_sfx_boop(void)   { sfx_play_prio(S_BOOP, 1); }
void game_sfx_launch(void) { sfx_play_prio(S_LAUNCH, 1); }
void game_sfx_catch(void)  { sfx_play_prio(S_CATCH, 1); }
void game_sfx_tough(void)  { sfx_play_prio(S_TOUGH, 1); }
void game_sfx_pop(void)    { sfx_play_prio(S_POP, 1); }
void game_sfx_honk(void)   { sfx_play_prio(S_HONK, 2); }
void game_sfx_bonk(void)   { sfx_play_prio(S_BONK, 2); }
void game_sfx_fish(void)   { sfx_play_prio(S_FISH, 2); }
void game_sfx_extra(void)  { sfx_play_prio(S_EXTRA, 3); }
void game_sfx_lost(void)   { sfx_play_prio(S_LOST, 3); }
