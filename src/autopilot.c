/*
 * AUTOPILOT builds only (-DAUTOPILOT=ON): a bot that plays the real game, used to record gameplay
 * videos and to time frames in the emulator. It returns the buttons a player would hold this
 * frame; everything else (physics, rules, sound) is the normal game.
 *
 * It works out where the ball comes down (a coarse simulation that bounces off the poles and the
 * balloons), stands so the nose offset sends the ball toward a column that still has balloons,
 * hops into every other bounce for height, and now and then catches and balances the ball.
 */
#include "game.h"

#define BALL           12
#define HOP_LEAD       10    // frames before contact to start a hop
#define BALANCE_HOLD   50
#define AIM_MAX        6     // px of nose offset the bot aims with

static uint8_t boops;
static uint8_t serve_wait;
static uint8_t hold;
static uint8_t last_state = 0xFF;
static uint8_t catching;
static int16_t prev_vy, prev_vx;
static int16_t plan_land, plan_frames;
static int16_t sim_elapsed;
int16_t ap_dbg[8];   // land, want, nose, frames, ball x, ball y, bonks, - (read by trace.py)

/*
 * Where the ball's center comes down to the nose height: a coarse forward simulation in steps of
 * 2 frames that reflects off the poles and off anything solid the ball's leading edge enters
 * (balloons, rigging). It runs SIM_PER_FRAME steps a frame, so the bot's own CPU time stays small
 * (one whole run in a frame took most of a frame and spoiled the frame timing), and restarts from
 * the real ball after every bounce. Until it lands, the last plan stands.
 */
#define SIM_DT         2
#define SIM_STEPS      110
#define SIM_PER_FRAME  10
static int16_t sim_x, sim_y, sim_vx, sim_vy, sim_bottom;
static uint8_t sim_steps, sim_active;

static void sim_start(const body_t *b, int16_t ny) {
    sim_x = b->x;
    sim_y = b->y;
    sim_vx = b->vx;
    sim_vy = b->vy;
    sim_bottom = PHYSICS_FP(ny - BALL);
    sim_steps = 0;
    sim_active = 1;
}

static void sim_run(void) {
    const int16_t lo = PHYSICS_FP(16), hi = PHYSICS_FP(WIDTH * 16 - 16 - BALL);
    uint8_t n, f;
    arena_ball_flags();
    for (n = 0; n < SIM_PER_FRAME; n++) {
        sim_vy += SIM_DT;
        sim_x += sim_vx * SIM_DT;
        sim_y += sim_vy * SIM_DT;
        if (sim_x < lo) { sim_x = lo + lo - sim_x; sim_vx = -sim_vx; }
        if (sim_x > hi) { sim_x = hi + hi - sim_x; sim_vx = -sim_vx; }
        if ((sim_vy > 0 && sim_y >= sim_bottom) || ++sim_steps >= SIM_STEPS) {
            plan_land = PHYSICS_FP_INT(sim_x) + BALL / 2;
            // the simulation started `elapsed` frames ago
            plan_frames = (int16_t)sim_steps * SIM_DT - sim_elapsed;
            if (plan_frames < 0) plan_frames = 0;
            sim_active = 0;
            break;
        }
        if (sim_y < PHYSICS_FP(16)) { sim_y = PHYSICS_FP(16); sim_vy = -sim_vy; continue; }
        f = tileflags_fast((uint16_t)PHYSICS_FP_INT(sim_x) + BALL / 2, (uint16_t)PHYSICS_FP_INT(sim_y) + (sim_vy < 0 ? 0 : BALL));
        if (f & TF_SOLID) {
            // reflect, losing a quarter like the real bounce
            sim_y -= sim_vy * SIM_DT;
            sim_vy = -(sim_vy - (sim_vy >> 2));
        }
    }
    arena_body_flags();
}

/* Column with balloons nearest to px */
static int16_t target_x(int16_t px) {
    int8_t best = -1;
    uint8_t c, bestd = 0xFF, d;
    uint8_t col = (uint8_t)(px >> 4);
    for (c = 1; c < WIDTH - 1; c++) {
        if (!arena_cols[c]) continue;
        d = c > col ? c - col : col - c;
        if (d < bestd) { bestd = d; best = (int8_t)c; }
    }
    return best < 0 ? px : (best << 4) + 8;
}

/* A clown walking into the seal: hop over it (a bonk freezes the seal for most of a second) */
static uint8_t clown_near(game_entity_t *seal) {
    game_entity_t *e;
    int16_t sx = PHYSICS_FP_INT(seal->entity.physics.body.x), dx;
    if (!(seal->entity.physics.body.state & BODY_ON_GROUND)) return 0;
    ENTITY_POOL_FOR_EACH(&game_entities, e, entity.physics.visual.type) {
        if (e->entity.physics.visual.type != E_CLOWN) continue;
        dx = PHYSICS_FP_INT(e->entity.physics.body.x) - sx;
        if (dx > 0 && dx < 36) return 1;
        if (dx < 0 && dx > -36) return 1;
    }
    return 0;
}

uint16_t autopilot_input(void) {
    game_entity_t *seal = seal_entity();
    game_entity_t *ball = ball_entity();
    uint8_t state = seal_ball_state();
    uint16_t in = 0;
    int16_t sx, nose, want, land, frames, off;

    if (seal == NULL || ball == NULL) return 0;
    sx = PHYSICS_FP_INT(seal->entity.physics.body.x);
    {
        static uint8_t was_bonk;
        uint8_t bonk = seal->entity.physics.visual.sprite->tile == SPR_SEAL_BONK;
        if (bonk && !was_bonk) ap_dbg[6]++;
        if (bonk) was_bonk = 8;   // the bonk animation blinks with the plain frame
        else if (was_bonk) was_bonk--;
    }
    nose = sx + (seal->entity.physics.visual.flip ? 5 : 7);

    if (state != last_state) {
        if (state == BALL_SERVE) serve_wait = 30 + (rand8() & 31);
        if (state == BALL_BALANCE) hold = BALANCE_HOLD;
        last_state = state;
    }

    switch (state) {
        case BALL_SERVE:
            // the act intro also reads input, so keep re-arming until the serve takes
            if (serve_wait == 0) serve_wait = 30 + (rand8() & 31);
            if (--serve_wait == 0) in |= IN_HOP;
            break;

        case BALL_BALANCE: {
            // keep the nose under the ball, then let go
            int16_t bx = PHYSICS_FP_INT(ball->entity.physics.body.x) + BALL / 2;
            if (bx > nose + 1) in |= IN_RIGHT;
            else if (bx < nose - 1) in |= IN_LEFT;
            if (hold) {
                hold--;
                in |= IN_CATCH;
            }
            catching = 0;
            break;
        }

        case BALL_FREE:
            // a bounce off the nose (or a clown) just happened
            if (prev_vy > 0 && ball->entity.physics.body.vy < 0 && PHYSICS_FP_INT(ball->entity.physics.body.y) > PHYSICS_FP_INT(seal->entity.physics.body.y) - 40) boops++;
            // re-plan from the real ball after any bounce (or when the last plan ran out)
            if ((prev_vy ^ ball->entity.physics.body.vy) < 0 || prev_vx != ball->entity.physics.body.vx || (!sim_active && plan_frames == 0)) {
                sim_start(&ball->entity.physics.body, PHYSICS_FP_INT(seal->entity.physics.body.y) - 4);
                sim_elapsed = 0;
            }
            if (plan_frames) plan_frames--;
            if (sim_active) {
                sim_elapsed++;
                sim_run();
            }
            land = plan_land;
            frames = plan_frames;
            want = target_x(land);
            // nose offset that sends the ball from land toward want over one flight
            // (kept well inside the nose's reach: the plan can be a few px off)
            off = (want - land) / 12;
            if (off > AIM_MAX) off = AIM_MAX;
            if (off < -AIM_MAX) off = -AIM_MAX;
            want = land - off;
            ap_dbg[0] = land; ap_dbg[1] = want; ap_dbg[2] = nose; ap_dbg[3] = frames;
            ap_dbg[4] = PHYSICS_FP_INT(ball->entity.physics.body.x); ap_dbg[5] = PHYSICS_FP_INT(ball->entity.physics.body.y);
            if (nose < want - 2) in |= IN_RIGHT;
            else if (nose > want + 2) in |= IN_LEFT;
            if (ball->entity.physics.body.vy > 0 && frames <= HOP_LEAD && (seal->entity.physics.body.state & BODY_ON_GROUND)) {
                if (boops & 1) in |= IN_HOP;
            }
            if (clown_near(seal)) in |= IN_HOP;
            // every few bounces, catch and balance instead
            if (ball->entity.physics.body.vy > 0 && frames < 20 && (boops & 7) == 5) catching = 1;
            if (catching) in |= IN_CATCH;
            break;

        default:
            catching = 0;
            break;
    }
    prev_vy = ball->entity.physics.body.vy;
    prev_vx = ball->entity.physics.body.vx;
    return in;
}
