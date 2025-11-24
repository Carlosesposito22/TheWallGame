#include "ball.h"
#include "physics.h"
#include "particles.h"
#include "effects.h"
#include "game_common.h"
#include <math.h>
#include <stdlib.h>

void InitBall(Ball *ball) {
    ball->active = 0;
    ball->x = SCREEN_WIDTH * 0.5f;
    ball->y = 60;
    ball->vx = 0;
    ball->vy = 0;
    ball->scale = 1.0f;
    ball->rotation = 0.0f;
    ball->rotationSpeed = 0.0f;
    ball->slotIndex = -1;
    ball->color = COLOR_NEON_GOLD;
}

bool TrySpawnBall(Ball *ball, bool lastAnswerWasCorrect) {
    if (ball->active) return false;

    ball->x = SCREEN_WIDTH * 0.5f;
    ball->y = 60;
    ball->vx = RandomFloat(-100, 100);
    ball->vy = 0;
    ball->active = 1;
    ball->slotIndex = -1;
    ball->color = lastAnswerWasCorrect ? COLOR_NEON_GREEN : COLOR_NEON_RED;
    ball->scale = 1.3f;
    ball->rotationSpeed = ball->vx * 0.02f;
    StartScreenShake(3.0f);
    return true;
}

void DrawBall(const Ball *ball) {
    if (!ball->active && ball->slotIndex < 0) return;

    DrawCircle((int)ball->x, (int)ball->y, BALL_RADIUS * ball->scale + 3,
               (Color){255, 255, 255, 80});

    DrawCircle((int)ball->x, (int)ball->y, BALL_RADIUS * ball->scale, ball->color);
    DrawCircleLines((int)ball->x, (int)ball->y, BALL_RADIUS * ball->scale, WHITE);

    float markerX = ball->x + cosf(ball->rotation) * BALL_RADIUS * 0.6f;
    float markerY = ball->y + sinf(ball->rotation) * BALL_RADIUS * 0.6f;
    DrawCircle((int)markerX, (int)markerY, BALL_RADIUS * 0.2f,
               (Color){255, 255, 255, 180});
}