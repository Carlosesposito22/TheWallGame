#include "particles.h"
#include "game_common.h"
#include <math.h>
#include <stdlib.h>

static Particle particles[PARTICLE_COUNT];
static TrailPoint ballTrail[TRAIL_LENGTH];
static int trailIndex = 0;

void InitParticles(void) {
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        particles[i].life = 0.0f;
    }
    for (int i = 0; i < TRAIL_LENGTH; i++) {
        ballTrail[i].alpha = 0.0f;
        ballTrail[i].scale = 0.3f;
    }
    trailIndex = 0;
}

void CreateParticles(float x, float y, Color color, int count) {
    for (int i = 0; i < PARTICLE_COUNT && count > 0; i++) {
        if (particles[i].life <= 0.0f) {
            particles[i].x = x;
            particles[i].y = y;
            particles[i].vx = RandomFloat(-400, 400);
            particles[i].vy = RandomFloat(-400, 400);
            particles[i].life = particles[i].maxLife = RandomFloat(0.3f, 1.0f);
            particles[i].color = color;
            particles[i].size = RandomFloat(2, 6);
            count--;
        }
    }
}

void UpdateParticles(float dt) {
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        if (particles[i].life > 0.0f) {
            particles[i].x += particles[i].vx * dt;
            particles[i].y += particles[i].vy * dt;
            particles[i].vy += GRAVITY * 0.3f * dt;

            particles[i].life -= dt;

            if (particles[i].life < 0.0f) particles[i].life = 0.0f;

            particles[i].color.a = (unsigned char)
                (255 * (particles[i].life / particles[i].maxLife));
        }
    }
}

void UpdateBallTrail(float ballX, float ballY, float dt) {
    for (int i = 0; i < TRAIL_LENGTH; i++) {
        ballTrail[i].alpha = MathLerp(ballTrail[i].alpha, 0.0f, 8.0f * dt);
        ballTrail[i].scale = MathLerp(ballTrail[i].scale, 0.3f, 6.0f * dt);
    }
    ballTrail[trailIndex].x = ballX;
    ballTrail[trailIndex].y = ballY;
    ballTrail[trailIndex].alpha = 1.0f;
    ballTrail[trailIndex].scale = 1.0f;

    trailIndex = (trailIndex + 1) % TRAIL_LENGTH;
}

void DrawParticles(void) {
    // Desenha trilha
    for (int i = 0; i < TRAIL_LENGTH; i++) {
        if (ballTrail[i].alpha > 0.01f) {
            Color c = WHITE;
            c.a = (unsigned char)(150 * ballTrail[i].alpha);
            DrawCircle((int)ballTrail[i].x, (int)ballTrail[i].y,
                      BALL_RADIUS * 0.7f * ballTrail[i].scale, c);
        }
    }

    // Desenha partículas
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        if (particles[i].life > 0.0f) {
            DrawCircle((int)particles[i].x, (int)particles[i].y,
                       particles[i].size, particles[i].color);
        }
    }
}