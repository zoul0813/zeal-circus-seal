#include <zgdk/physics/platformer.h>
/*
 * Everything else in the ring: clowns walking the floor, fish dropped by fish balloons,
 * balloon pops and the star over a caught fish. The seal and the ball live in seal.c.
 */
#include "game.h"

#define CLOWN_SPEED   12
#define HONK_FRAMES   20
#define HEAD_DEPTH    10    // the top px of a clown that bounce the ball
#define HEAD_BOUNCE   (-52)
#define FISH_POINTS   500
#define FISH_LIFE     150   // frames flopping on the floor before it slips away
#define POP_FRAMES    10
#define STAR_FRAMES   30

static const platformer_config_t CLOWN_PHYS = {
    .gravity = 3, .max_fall = 64, .jump_vel = 0, .jump_cut = 0,
    .accel = 0, .decel = 0, .max_run = 0, .coyote = 0, .buffer = 0, .air_jumps = 0,
};
static const platformer_config_t FISH_PHYS = {
    .gravity = 1, .max_fall = 20, .jump_vel = 0, .jump_cut = 0,
    .accel = 0, .decel = 0, .max_run = 0, .coyote = 0, .buffer = 0, .air_jumps = 0,
};

static const uint8_t F_CLOWN[] = { SPR_CLOWN1, SPR_CLOWN2 };
static const uint8_t F_HONK[] = { SPR_CLOWN_HONK };
static const uint8_t F_FISH[] = { SPR_FISH1, SPR_FISH2 };
static const uint8_t F_POP[] = { SPR_POP1, SPR_POP2 };
static const uint8_t F_STAR[] = { SPR_STAR };
static const anim_def A_CLOWN = { F_CLOWN, 2, 10, ANIM_LOOP };
static const anim_def A_HONK = { F_HONK, 1, 60, ANIM_LOOP };
static const anim_def A_FISH = { F_FISH, 2, 8, ANIM_LOOP };
static const anim_def A_FLOP = { F_FISH, 2, 3, ANIM_LOOP };
static const anim_def A_POP = { F_POP, 2, 5, 0 };
static const anim_def A_STAR = { F_STAR, 1, 60, ANIM_LOOP };

static gfx_sprite sprite_arena[ARENA_SIZE];
static game_entity_t pool[POOL_SIZE];

void actors_init(void) {
    body_set_touch_mask(0);   // no hazards or ladders in the ring
    sprites_register_arena(sprite_arena, ARENA_SIZE);
    sprite_entities_init(&game_entities, pool, POOL_SIZE, sizeof(*pool));
}

void actors_spawn_clown(uint8_t tx, uint8_t ty) {
    game_entity_t *e = physics_entities_spawn(&game_entities, E_CLOWN, (tx << 4) + 2, ((ty + 1) << 4) - 28, 12, 28);
    if (e == NULL) return;
    e->entity.physics.visual.tall = 1;
    e->entity.physics.visual.ox = -2;
    e->entity.physics.visual.oy = -4;
    e->entity.physics.body.vx = (tx < WIDTH / 2) ? CLOWN_SPEED : -CLOWN_SPEED;
    if (game.loop) e->entity.physics.body.vx += e->entity.physics.body.vx >> 1;
    e->data = (uint16_t)e->entity.physics.body.vx;
    animated_physics_entity_play(&e->entity, &A_CLOWN);
}

void actors_spawn_fish(int16_t px, int16_t py) {
    game_entity_t *e = physics_entities_spawn(&game_entities, E_FISH, px + 2, py + 4, 12, 8);
    if (e == NULL) return;
    e->entity.physics.visual.ox = -2;
    e->entity.physics.visual.oy = -5;
    e->entity.physics.body.vx = (px < 160) ? 6 : -6;
    e->entity.physics.visual.flip = e->entity.physics.body.vx < 0 ? SPRITE_FLIP_X : 0;
    animated_physics_entity_play(&e->entity, &A_FISH);
}

void actors_spawn_pop(int16_t px, int16_t py) {
    game_entity_t *e = physics_entities_spawn(&game_entities, E_POP, px, py, 16, 16);
    if (e == NULL) return;
    e->timer = POP_FRAMES;
    animated_physics_entity_play(&e->entity, &A_POP);
}

static void spawn_star(int16_t px, int16_t py) {
    game_entity_t *e = physics_entities_spawn(&game_entities, E_STAR, px, py, 16, 16);
    if (e == NULL) return;
    e->timer = STAR_FRAMES;
    animated_physics_entity_play(&e->entity, &A_STAR);
}

static void clown_update(game_entity_t *e, game_entity_t *seal, game_entity_t *ball) {
    int16_t bx, by, cx, cy;
    platformer_move(&e->entity.physics.body, &CLOWN_PHYS);
    if (e->entity.physics.body.state & BODY_HIT_WALL) e->data = (uint16_t)(-(int16_t)e->data);
    e->entity.physics.body.vx = (int16_t)e->data;
    e->entity.physics.visual.flip = e->entity.physics.body.vx < 0 ? SPRITE_FLIP_X : 0;

    if (e->timer) {
        if (--e->timer == 0) animated_physics_entity_play(&e->entity, &A_CLOWN);
    }
    if (body_overlap(&e->entity.physics.body, &seal->entity.physics.body) && seal_bonk(PHYSICS_FP_INT(e->entity.physics.body.x))) {
        // honk and walk away
        e->data = (uint16_t)(-(int16_t)e->data);
        e->timer = HONK_FRAMES;
        animated_physics_entity_play(&e->entity, &A_HONK);
    }

    // the ball coming down on the clown's head bounces back up (and honks)
    if (seal_ball_state() == BALL_FREE && ball->entity.physics.body.vy > 0) {
        bx = PHYSICS_FP_INT(ball->entity.physics.body.x);
        by = PHYSICS_FP_INT(ball->entity.physics.body.y) + 12;
        cx = PHYSICS_FP_INT(e->entity.physics.body.x);
        cy = PHYSICS_FP_INT(e->entity.physics.body.y) - 4;
        if (by >= cy && by < cy + HEAD_DEPTH && bx + 12 > cx && bx < cx + 12) {
            ball_bounce_up(HEAD_BOUNCE);
            e->timer = HONK_FRAMES;
            animated_physics_entity_play(&e->entity, &A_HONK);
            game_sfx_honk();
        }
    }
}

static void fish_update(game_entity_t *e, game_entity_t *seal) {
    platformer_move(&e->entity.physics.body, &FISH_PHYS);
    if (e->entity.physics.body.state & BODY_HIT_WALL) {
        e->entity.physics.body.vx = (e->entity.physics.visual.flip ? 6 : -6);
        e->entity.physics.visual.flip ^= SPRITE_FLIP_X;
    }
    if (e->entity.physics.body.state & BODY_ON_GROUND) {
        // flopping on the floor
        if (e->timer == 0) {
            e->timer = FISH_LIFE;
            e->entity.physics.body.vx = 0;
            animated_physics_entity_play(&e->entity, &A_FLOP);
        }
        if (--e->timer == 0) {
            entity_pool_free(&game_entities, e);
            return;
        }
        e->entity.physics.visual.hidden = e->timer < 40 && (e->timer & 4);
    }
    if (body_overlap(&e->entity.physics.body, &seal->entity.physics.body)) {
        score_add(FISH_POINTS);
        game_sfx_fish();
        spawn_star(PHYSICS_FP_INT(e->entity.physics.body.x) - 2, PHYSICS_FP_INT(e->entity.physics.body.y) - 12);
        entity_pool_free(&game_entities, e);
    }
}

void actors_update(void) {
    game_entity_t *e;
    game_entity_t *seal = seal_entity();
    game_entity_t *ball = ball_entity();
    ENTITY_POOL_FOR_EACH(&game_entities, e, entity.physics.visual.type) {
        switch (e->entity.physics.visual.type) {
            case E_CLOWN:
                clown_update(e, seal, ball);
                break;
            case E_FISH:
                fish_update(e, seal);
                break;
            case E_POP:
                if (--e->timer == 0) entity_pool_free(&game_entities, e);
                break;
            case E_STAR:
                e->entity.physics.body.y -= PHYSICS_FP(1) >> 1;
                if (--e->timer == 0) entity_pool_free(&game_entities, e);
                break;
            default:
                continue;
        }
        if (e->entity.physics.visual.type != ENTITY_FREE) e->entity.physics.visual.sprite->tile = anim_tick(&e->entity.anim);
    }
}
