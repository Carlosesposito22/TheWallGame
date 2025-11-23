#pragma once
#include "raylib.h"
#include "commons.h"

#define COLOR_BG           (Color){  8,  12,  32, 255 }
#define COLOR_NEON_BLUE    (Color){  0, 200, 255, 255 }
#define COLOR_NEON_GOLD    (Color){255, 215,   0, 255 }
#define COLOR_NEON_PURPLE  (Color){180,  70, 255, 255 }
#define COLOR_NEON_GREEN   (Color){ 50, 255, 150, 255 }
#define COLOR_NEON_RED     (Color){255,  50, 100, 255 }
#define COLOR_UI_BG        (Color){ 20,  25,  45, 220 }
#define COLOR_UI_BORDER    (Color){ 70, 130, 230, 255 }
#define COLOR_PREDICTION   (Color){100, 255, 255, 120 }
#define COLOR_PATH         (Color){255, 255, 100,  80 }

// typedef struct {
//     float x, y, vx, vy;
//     int active;
//     int slotIndex;
//     Color color;
//     float scale;
//     float rotation;
//     float rotationSpeed;
// } Ball;

// typedef struct {
//     float x, y;
//     Color color;
//     int visible;
// } Pin;

// typedef struct {
//     float x, y, vx, vy;
//     float life, maxLife;
//     Color color;
//     float size;
// } Particle;

// // Funções utilitárias
// float MathLerp(float a, float b, float t);
float RandomFloat(float min, float max);
// long long factorial(int n);
// long long combinations(int n, int k);

// // Constantes físicas comuns
// #define GRAVITY 1200.0f
// #define AIR_RESISTANCE 0.995f
// #define FRICTION 0.88f
// #define ELASTICITY 0.75f
// #define BALL_RADIUS 15