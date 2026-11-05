#include <zgdk/physics/platformer.h>
/*
 * The seal and the beach ball.
 *
 * The seal is one native 16x32 sprite running the core platformer physics. The ball runs the
 * game's body_bounce() with its own gravity and the ball flags table (it flies through the
 * pedestal tops and bounces off balloons); every side it hits is probed for balloons to pop.
 *
 * Where the ball lands on the nose sets its sideways speed, and a hop at the moment of contact
 * adds height. Holding catch when it lands balances it on the nose instead: it then slowly rolls
 * off one side (an inverted pendulum) unless the seal shuffles under it, and letting go of catch
 * fires a stronger, aimed shot that keeps the balloon chain going.
 */
#include "game.h"

#define BALL_SIZE      12
#define NOSE_DY        (-4)  // nose tip, relative to the seal hitbox top (the sprite starts 4 px higher)
#define NOSE_REACH     12    // px either side of the nose center that still count as a hit
#define LAUNCH         66    // vy off the nose, 12.4 (reaches row 3 from the floor)
#define HOP_BONUS_MAX  20
#define CLEAN_BONUS    10    // a shot from a balance goes higher
#define AIM            2     // vx per px of nose offset
#define AIM_CLEAN      3
#define BALL_MAX_VX    36
#define BALL_KEEP      12    // a bounce off a balloon or the rigging keeps 12/16 of the speed
#define BALL_MAX_FALL  96
#define BALANCE_DROP   PHYSICS_FP(9)
#define BONK_FRAMES    18
#define SAFE_FRAMES    90    // after a bonk, clowns can't bonk again for this long
#define LOST_FRAMES    100

static const platformer_config_t SEAL_PHYS = {
    .gravity = 3, .max_fall = 64, .jump_vel = -64, .jump_cut = -24,
    .accel = 6, .decel = 8, .max_run = 40, .coyote = 4, .buffer = 6, .air_jumps = 0,
};

static const uint8_t F_IDLE[] = { SPR_SEAL };
static const uint8_t F_WALK[] = { SPR_SEAL_WALK1, SPR_SEAL, SPR_SEAL_WALK2, SPR_SEAL };
static const uint8_t F_HOP[] = { SPR_SEAL_HOP };
static const uint8_t F_BONK[] = { SPR_SEAL_BONK, SPR_SEAL };
static const uint8_t F_SAD[] = { SPR_SEAL_SAD };
static const uint8_t F_CHEER[] = { SPR_SEAL_CHEER, SPR_SEAL };
static const anim_def A_IDLE = { F_IDLE, 1, 60, ANIM_LOOP };
static const anim_def A_WALK = { F_WALK, 4, 6, ANIM_LOOP };
static const anim_def A_HOP = { F_HOP, 1, 60, ANIM_LOOP };
static const anim_def A_BONK = { F_BONK, 2, 4, ANIM_LOOP };
static const anim_def A_SAD = { F_SAD, 1, 60, ANIM_LOOP };
static const anim_def A_CHEER = { F_CHEER, 2, 12, ANIM_LOOP };

static const uint8_t F_SPIN[] = { SPR_BALL0, SPR_BALL1, SPR_BALL2, SPR_BALL3 };
static const uint8_t F_SPIN_BACK[] = { SPR_BALL3, SPR_BALL2, SPR_BALL1, SPR_BALL0 };
static const anim_def A_BALL_STILL = { F_SPIN, 1, 60, ANIM_LOOP };
static const anim_def A_SPIN = { F_SPIN, 4, 4, ANIM_LOOP };
static const anim_def A_SPIN_BACK = { F_SPIN_BACK, 4, 4, ANIM_LOOP };

static game_entity_t *seal;
static game_entity_t *ball;
static uint8_t ball_state;
static uint8_t bonk;
static uint8_t safe;
static uint8_t lost_timer;
static uint8_t cheer;
static int16_t balance_v;     // ball's own sideways speed while balanced, 12.4
static int16_t prev_nose_y;

game_entity_t *seal_entity(void) { return seal; }
game_entity_t *ball_entity(void) { return ball; }
uint8_t seal_ball_state(void) { return ball_state; }
uint8_t seal_lost_done(void) { return ball_state == BALL_LOST && lost_timer == 0; }

static int16_t nose_x(void) {
    // the snout sits a little right of the sprite center, mirrored when facing left
    return PHYSICS_FP_INT(seal->entity.physics.body.x) + (seal->entity.physics.visual.flip ? 5 : 7);
}

static int16_t nose_y(void) {
    return PHYSICS_FP_INT(seal->entity.physics.body.y) + NOSE_DY;
}

static int16_t clamp16(int16_t v, int16_t lo, int16_t hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

/* Ball centered on the nose */
static void ball_to_nose(void) {
    ball->entity.physics.body.x = PHYSICS_FP(nose_x() - (BALL_SIZE / 2));
    ball->entity.physics.body.y = PHYSICS_FP(nose_y() - BALL_SIZE);
}

void seal_init(void) {
    seal = NULL;
    ball = NULL;
}

void seal_serve(void) {
    ball_state = BALL_SERVE;
    ball->entity.physics.visual.hidden = 0;
    ball->entity.physics.body.vx = 0;
    ball->entity.physics.body.vy = 0;
    animated_physics_entity_play(&ball->entity, &A_BALL_STILL);
    ball_to_nose();
    game.chain = 0;
}

/* After entity_pool_clear(&game_entities): spawn both on the act's start column */
void seal_act_start(void) {
    seal = physics_entities_spawn(&game_entities, E_SEAL, arena_start_x << 4, FLOOR_Y - 28, 12, 28);
    seal->entity.physics.visual.tall = 1;
    seal->entity.physics.visual.ox = -2;
    seal->entity.physics.visual.oy = -4;
    animated_physics_entity_play(&seal->entity, &A_IDLE);
    ball = physics_entities_spawn(&game_entities, E_BALL, 0, 0, BALL_SIZE, BALL_SIZE);
    ball->entity.physics.visual.ox = -2;
    ball->entity.physics.visual.oy = -2;
    bonk = 0;
    safe = 0;
    cheer = 0;
    lost_timer = 0;
    prev_nose_y = nose_y();
    seal_serve();
}

uint8_t seal_bonk(int16_t from_x) {
    if (bonk || safe || cheer || ball_state == BALL_LOST) return 0;
    bonk = BONK_FRAMES;
    safe = BONK_FRAMES + SAFE_FRAMES;
    seal->entity.physics.body.vx = from_x < PHYSICS_FP_INT(seal->entity.physics.body.x) ? 40 : -40;
    if (ball_state == BALL_BALANCE) {
        ball_state = BALL_FREE;
        ball->entity.physics.body.vx = balance_v;
    }
    game_sfx_bonk();
    return 1;
}

void seal_cheer(void) {
    cheer = 1;
    ball->entity.physics.visual.hidden = 1;
    ball_state = BALL_SERVE;
}

/* A clown's head knocks a falling ball back up */
void ball_bounce_up(int16_t vy) {
    if (ball_state != BALL_FREE || ball->entity.physics.body.vy <= 0) return;
    ball->entity.physics.body.vy = vy;
}

static void launch(int16_t vy, int16_t vx) {
    ball_state = BALL_FREE;
    ball->entity.physics.body.vy = vy;
    ball->entity.physics.body.vx = clamp16(vx, -BALL_MAX_VX, BALL_MAX_VX);
    game_sfx_launch();
}

static int16_t hop_bonus(void) {
    // rising with the hop adds up to HOP_BONUS_MAX: hop just before the ball lands
    int16_t v = seal->entity.physics.body.vy;
    if (v >= 0) return 0;
    v = (-v) >> 1;
    return v > HOP_BONUS_MAX ? HOP_BONUS_MAX : v;
}

static void pop_row(int16_t py, int16_t px) {
    uint8_t ty = (uint8_t)(py >> 4);
    uint8_t t0 = (uint8_t)(px >> 4), t1 = (uint8_t)((px + BALL_SIZE - 1) >> 4);
    arena_pop(t0, ty);
    if (t1 != t0) arena_pop(t1, ty);
}

static void ball_free(void) {
    body_t *b = &ball->entity.physics.body;
    int16_t vx0, vy0, px, py, prev_bottom, ny, off;
    uint8_t st;

    prev_bottom = PHYSICS_FP_INT(b->y) + BALL_SIZE;
    b->vy += 1;
    if (b->vy > BALL_MAX_FALL) b->vy = BALL_MAX_FALL;
    vx0 = b->vx;
    vy0 = b->vy;
    arena_ball_flags();
    body_bounce(b, BALL_KEEP, 0);
    arena_body_flags();
    st = b->state;
    px = PHYSICS_FP_INT(b->x);
    py = PHYSICS_FP_INT(b->y);

    if (st & BODY_HIT_WALL) {
        int16_t wx = vx0 > 0 ? px + BALL_SIZE : px - 1;
        uint8_t tx = (uint8_t)(wx >> 4);
        uint8_t ty0 = (uint8_t)(py >> 4), ty1 = (uint8_t)((py + BALL_SIZE - 1) >> 4);
        uint8_t hit = arena_pop(tx, ty0);
        if (ty1 != ty0) hit |= arena_pop(tx, ty1);
        if (!hit) game_sfx_wall();
    }
    if (st & BODY_HIT_CEIL) pop_row(py - 1, px);
    if ((st & BODY_ON_GROUND) && vy0 > 0) {
        if (py + BALL_SIZE >= FLOOR_Y) {
            ball_state = BALL_LOST;
            lost_timer = LOST_FRAMES;
            b->vx = 0;
            b->vy = 0;
            animated_physics_entity_play(&ball->entity, &A_BALL_STILL);
            if (game.balls) game.balls--;
            game.hud_dirty = 1;
            game_sfx_lost();
            return;
        }
        pop_row(py + BALL_SIZE, px);
    }

    // the nose: the ball's bottom crossed the nose tip this frame, coming down
    ny = nose_y();
    if (vy0 > 0 && !bonk && !cheer && prev_bottom <= prev_nose_y + 2 && py + BALL_SIZE >= ny) {
        off = (px + BALL_SIZE / 2) - nose_x();
        if (off >= -NOSE_REACH && off <= NOSE_REACH) {
            b->y = PHYSICS_FP(ny - BALL_SIZE);
            if (input_now & IN_CATCH) {
                ball_state = BALL_BALANCE;
                balance_v = b->vx >> 2;
                b->vx = 0;
                b->vy = 0;
                game_sfx_catch();
            } else {
                game.chain = 0;
                b->vy = -(LAUNCH + hop_bonus());
                b->vx = clamp16(off * AIM + (seal->entity.physics.body.vx >> 1), -BALL_MAX_VX, BALL_MAX_VX);
                game_sfx_boop();
            }
        }
    }
    animated_physics_entity_play(&ball->entity, b->vx < 0 ? &A_SPIN_BACK : &A_SPIN);
}

static void ball_balance(void) {
    body_t *b = &ball->entity.physics.body;
    int16_t off = (b->x + PHYSICS_FP(BALL_SIZE / 2)) - PHYSICS_FP(nose_x());
    // inverted pendulum: the further off center, the faster it rolls away
    balance_v += off >> 4;
    balance_v -= balance_v >> 4;
    if ((frame_count & 31) == 0) balance_v += (int16_t)(rand8() & 3) - 1;
    b->x += balance_v;
    if (b->x < PHYSICS_FP(16)) { b->x = PHYSICS_FP(16); balance_v = -balance_v; }
    if (b->x > PHYSICS_FP(WIDTH * 16 - 16 - BALL_SIZE)) { b->x = PHYSICS_FP(WIDTH * 16 - 16 - BALL_SIZE); balance_v = -balance_v; }
    b->y = PHYSICS_FP(nose_y() - BALL_SIZE);
    off = (b->x + PHYSICS_FP(BALL_SIZE / 2)) - PHYSICS_FP(nose_x());

    if (off > BALANCE_DROP || off < -BALANCE_DROP) {
        // rolled off the side
        ball_state = BALL_FREE;
        b->vx = balance_v;
        b->vy = 0;
        return;
    }
    if (input_pressed & IN_HOP) {
        launch(-(LAUNCH + CLEAN_BONUS + HOP_BONUS_MAX), PHYSICS_FP_INT(off) * AIM_CLEAN + balance_v);
    } else if (!(input_now & IN_CATCH)) {
        launch(-(LAUNCH + CLEAN_BONUS), PHYSICS_FP_INT(off) * AIM_CLEAN + balance_v);
    }
}

static void ball_serve(void) {
    ball_to_nose();
    if (cheer || bonk) return;
    if (input_pressed & (IN_HOP | IN_CATCH)) {
        int16_t vx = (seal->entity.physics.body.vx >> 1) + (int16_t)(rand8() & 15) - 8;
        launch(-(LAUNCH + CLEAN_BONUS + ((input_pressed & IN_HOP) ? HOP_BONUS_MAX : 0)), vx);
    }
}

static void seal_move(void) {
    body_t *s = &seal->entity.physics.body;
    int8_t dir = DIRECTION_NONE;
    uint8_t held = 0, pressed = 0;

    if (safe) safe--;
    if (bonk) {
        bonk--;
    } else if (ball_state != BALL_LOST && !cheer) {
        if (input_now & IN_LEFT) dir = DIRECTION_LEFT;
        else if (input_now & IN_RIGHT) dir = DIRECTION_RIGHT;
        held = (input_now & IN_HOP) != 0;
        pressed = (input_pressed & IN_HOP) != 0;
    }
    if (platformer_update(s, &SEAL_PHYS, &seal->platformer, dir, held, pressed)) game_sfx_hop();
    platformer_move(s, &SEAL_PHYS);

    if (s->vx < 0) seal->entity.physics.visual.flip = SPRITE_FLIP_X;
    else if (s->vx > 0) seal->entity.physics.visual.flip = 0;

    if (ball_state == BALL_LOST) animated_physics_entity_play(&seal->entity, &A_SAD);
    else if (cheer) animated_physics_entity_play(&seal->entity, &A_CHEER);
    else if (bonk) animated_physics_entity_play(&seal->entity, &A_BONK);
    else if (!(s->state & BODY_ON_GROUND)) animated_physics_entity_play(&seal->entity, &A_HOP);
    else if (s->vx > 4 || s->vx < -4) animated_physics_entity_play(&seal->entity, &A_WALK);
    else animated_physics_entity_play(&seal->entity, &A_IDLE);
    seal->entity.physics.visual.sprite->tile = anim_tick(&seal->entity.anim);
}

void seal_update(void) {
    seal_move();
    switch (ball_state) {
        case BALL_SERVE: ball_serve(); break;
        case BALL_FREE: ball_free(); break;
        case BALL_BALANCE: ball_balance(); break;
        case BALL_LOST:
            if (lost_timer) lost_timer--;
            ball->entity.physics.visual.hidden = (lost_timer < 40) && (lost_timer & 4);
            break;
    }
    prev_nose_y = nose_y();
    ball->entity.physics.visual.sprite->tile = anim_tick(&ball->entity.anim);
}
