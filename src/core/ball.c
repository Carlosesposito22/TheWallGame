#include "ball.h"
#include "physics.h"
#include "particles.h"
#include "effects.h"
#include "game_common.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h> // Para debug se necessário

static Ball balls[MAX_BALLS];
static int ballCount = 0;
static int lastBallSlot = -1;

// Referências externas necessárias
extern int slotCounts[SLOT_COUNT];
extern int totalBolas;
extern int slotValues[SLOT_COUNT];
extern long long totalScore;

// --- CONFIGURAÇÕES DE ESPAÇO ---
// Devem ser idênticas ao game.c
#define WALL_MARGIN 350.0f
#define WALL_TOP_Y 100.0f 

void InitBalls(void) {
    ballCount = 0;
    for (int i = 0; i < MAX_BALLS; i++) {
        balls[i].active = 0;
        balls[i].scale = 1.0f;
        balls[i].rotation = 0.0f;
        balls[i].rotationSpeed = 0.0f;
        balls[i].slotIndex = -1;
        balls[i].color = COLOR_NEON_GOLD;
    }
    // Não calculamos mais posições fixas aqui para evitar erros de inicialização
}

bool SpawnBall(bool lastAnswerWasCorrect) {
    if (ballCount >= MAX_BALLS) return false;

    // 1. Definição da área jogável (DENTRO das paredes)
    // Se SCREEN_WIDTH for uma macro, ok. Se for função GetScreenWidth(), também ok.
    float leftBound = WALL_MARGIN;
    float rightBound = SCREEN_WIDTH - WALL_MARGIN;
    float playableWidth = rightBound - leftBound;

    // 2. Escolhe um slot (0 a 6)
    int slot = rand() % TOP_SLOTS;

    // 3. Calcula a largura de cada slot superior
    float slotWidth = playableWidth / (float)TOP_SLOTS;

    // 4. Calcula o X central do slot escolhido
    // Lógica: Margem Esquerda + (Slot * Largura) + (Metade da Largura para centralizar)
    float centerX = leftBound + (slot * slotWidth) + (slotWidth * 0.5f);

    // 5. Define posição inicial
    float x = centerX;
    float y = WALL_TOP_Y + BALL_RADIUS + 2.0f; // Nasce logo abaixo do topo da parede

    // Pequena variação aleatória mínima para não parecer artificial, 
    // mas mantendo dentro do slot (slotWidth * 0.2 é seguro)
    x += RandomFloat(-slotWidth * 0.2f, slotWidth * 0.2f);

    // Verifica sobreposição para não "grudar" em bolas paradas no topo
    for (int t = 0; t < 5; t++) {
        bool overlap = false;
        for (int i = 0; i < ballCount; i++) {
            if (!balls[i].active) continue;

            float dx = x - balls[i].x;
            float dy = y - balls[i].y;
            if (sqrtf(dx*dx + dy*dy) < BALL_RADIUS * 2.1f) {
                overlap = true;
                // Se colidir, tenta jogar um pouco mais para o lado (ainda dentro do slot)
                x += RandomFloat(-5.0f, 5.0f); 
                break;
            }
        }
        if (!overlap) break;
    }

    // Configura a nova bola
    Ball *b = &balls[ballCount++];
    b->x = x;
    b->y = y;
    b->vx = 0; // Velocidade zero para cair reto
    b->vy = 0; // Gravidade vai atuar depois
    b->active = 1;
    b->slotIndex = -1;
    b->color = lastAnswerWasCorrect ? COLOR_NEON_GREEN : COLOR_NEON_RED;
    b->scale = 0.1f; // Começa pequena (animação)
    b->rotationSpeed = RandomFloat(-2.0f, 2.0f);
    
    StartScreenShake(2.0f);

    return true;
}

void UpdateBalls(Pin *pins, int pinCount, float baseY, float firstSlotX, float slotWidth, bool lastAnswerWasCorrect, float dt) {
    for (int i = 0; i < ballCount; i++) {
        Ball *b = &balls[i];
        if (!b->active) continue;

        if (UpdateBallPhysics(b, pins, pinCount, baseY, firstSlotX, slotWidth,
                              slotCounts, &totalBolas, slotValues, &totalScore,
                              lastAnswerWasCorrect, dt)) {
            b->active = 0;
            lastBallSlot = b->slotIndex;
        }

        UpdateBallTrail(b->x, b->y, dt);

        // Colisão Bola com Bola
        for (int j = i + 1; j < ballCount; j++) {
            Ball *b2 = &balls[j];
            if (!b2->active) continue;

            float dx = b2->x - b->x;
            float dy = b2->y - b->y;
            float distSq = dx*dx + dy*dy;
            float minDist = 2 * BALL_RADIUS;
            
            if (distSq > 0.001f && distSq < minDist * minDist) {
                float dist = sqrtf(distSq);
                float overlap = 0.5f * (minDist - dist);
                float nx = dx / dist;
                float ny = dy / dist;
                
                b->x -= nx * overlap;
                b->y -= ny * overlap;
                b2->x += nx * overlap;
                b2->y += ny * overlap;

                float dvx = b2->vx - b->vx;
                float dvy = b2->vy - b->vy;
                float dot = dvx * nx + dvy * ny;
                
                if (dot < 0) {
                    float impulse = dot * (1.0f + 0.7f); // Elasticidade hardcoded 0.7f
                    b->vx += impulse * nx;
                    b->vy += impulse * ny;
                    b2->vx -= impulse * nx;
                    b2->vy -= impulse * ny;
                }
            }
        }

        b->rotation += b->rotationSpeed * dt;
        b->scale = MathLerp(b->scale, 1.0f, 10.0f * dt);
    }

    // Remove bolas inativas da lista (limpeza)
    int newCount = 0;
    for (int i = 0; i < ballCount; i++) {
        if (balls[i].active) {
            balls[newCount++] = balls[i];
        }
    }
    ballCount = newCount;
}

void DrawBalls(void) {
    for (int i = 0; i < ballCount; i++) {
        Ball *b = &balls[i];
        if (!b->active) continue;

        DrawCircle((int)b->x, (int)b->y, BALL_RADIUS * b->scale + 3, (Color){255, 255, 255, 80});
        DrawCircle((int)b->x, (int)b->y, BALL_RADIUS * b->scale, b->color);
        DrawCircleLines((int)b->x, (int)b->y, BALL_RADIUS * b->scale, WHITE);
    }
}

bool AnyBallActive(void) {
    return ballCount > 0;
}

int GetLastBallSlot(void) {
    return lastBallSlot;
}

Ball* GetFirstActiveBall(void) {
    if (ballCount > 0) return &balls[0];
    return NULL;
}