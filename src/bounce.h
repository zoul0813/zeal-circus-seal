#include <zgdk/physics/platformer.h>
#include <stdint.h>
#include <zgdk/physics/body.h>

#ifndef CIRCUS_BOUNCE_H
#define CIRCUS_BOUNCE_H

/**
 * @brief body_step() that bounces off solid tiles instead of stopping (balls, debris)
 *
 * Circus Seal ball physics. On a wall the horizontal speed is reversed,
 * on a floor or ceiling the vertical speed, each scaled by `keep` / 16 (16 = no loss).
 * The body_t.state bits are set like body_step() (BODY_HIT_WALL, BODY_ON_GROUND, BODY_HIT_CEIL),
 * so the game can still tell what was hit. Add gravity to vy yourself before the call.
 * Floor bounces slower than `rest` stop dead (vy = 0), so a ball can come to rest.
 */
void body_bounce(body_t *body, uint8_t keep, int16_t rest);

#endif
