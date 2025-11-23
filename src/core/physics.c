// physics.c
#include "physics.h"
#include "particles.h"
#include "effects.h"

void UpdateBallPhysics(Ball* ball, float dt, Pin* pins, int pinCount, float screenWidth, float baseY, float firstSlotX, float slotWidth, void (*OnSlotLanded)(int)) {
    if (!ball->active) return;

    ball->vy += GRAVITY * dt;
    ball->vx *= AIR_RESISTANCE;
    ball->vy *= AIR_RESISTANCE;
    ball->x += ball->vx * dt;
    ball->y += ball->vy * dt;
    // (... resto das colisões com pinos e paredes ...)
}