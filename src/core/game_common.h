// game_common.h
#pragma once
#include "raylib.h"

typedef struct {
    float x, y, vx, vy;
    int active;
    int slotIndex;
    Color color;
    float scale;
    float rotation;
    float rotationSpeed;
} Ball;

typedef struct {
    float x, y;
    Color color;
    int visible;
} Pin;

typedef struct {
    float x, y, vx, vy;
    float life, maxLife;
    Color color;
    float size;
} Particle;

// Funções utilitárias
float MathLerp(float a, float b, float t);
float RandomFloat(float min, float max);
long long factorial(int n);
long long combinations(int n, int k);

// Constantes físicas comuns
#define GRAVITY 1200.0f
#define AIR_RESISTANCE 0.995f
#define FRICTION 0.88f
#define ELASTICITY 0.75f
#define BALL_RADIUS 15