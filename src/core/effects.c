#include "effects.h"
#include "raylib.h"
#include "game_common.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SHAKE_DURATION 0.4f

static float shakeTimer = 0.0f;
static float shakeIntensity = 0.0f;
static float cameraOffsetX = 0.0f;
static float cameraOffsetY = 0.0f;
static int slowMoActive = 0;
static float slowMoFactor = 0.20f;

void StartScreenShake(float intensity) {
    shakeTimer = SHAKE_DURATION;
    shakeIntensity = intensity;
}

void UpdateScreenShake(float dt) {
    if (shakeTimer > 0.0f) {
        shakeTimer -= dt;
        cameraOffsetX = RandomFloat(-shakeIntensity, shakeIntensity) * (shakeTimer / SHAKE_DURATION);
        cameraOffsetY = RandomFloat(-shakeIntensity, shakeIntensity) * (shakeTimer / SHAKE_DURATION);
    } else {
        cameraOffsetX = 0.0f;
        cameraOffsetY = 0.0f;
    }
}

float GetCameraOffsetX(void) { return cameraOffsetX; }
float GetCameraOffsetY(void) { return cameraOffsetY; }

void UpdateSlowMotion(void) {
    slowMoActive = IsKeyDown(KEY_S);
}

float ApplyTimeScale(float dt) {
    return slowMoActive ? (dt * slowMoFactor) : dt;
}

bool IsSlowMotionActive(void) {
    return slowMoActive;
}

void DrawSlowMotionIndicator(void) {
    float pulse = sinf(GetTime() * 7.0f) * 0.5f + 0.5f;
    unsigned char alpha = (unsigned char)(80 + 120 * pulse);

    int cx = SCREEN_WIDTH / 2;
    int cy = SCREEN_HEIGHT / 2;

    float size = 26.0f;      
    float width = size * 1.4f; 

    Vector2 v1 = (Vector2){ cx - width, cy };        
    Vector2 v2 = (Vector2){ cx + width * 0.6f, cy - size };
    Vector2 v3 = (Vector2){ cx + width * 0.6f, cy + size };

    Color fill = (Color){ COLOR_NEON_BLUE.r, COLOR_NEON_BLUE.g, COLOR_NEON_BLUE.b, alpha };
    Color border = (Color){ COLOR_NEON_GOLD.r, COLOR_NEON_GOLD.g, COLOR_NEON_GOLD.b, (unsigned char)(180 * (0.6f + 0.4f * pulse)) };

    Color glow = (Color){ COLOR_NEON_BLUE.r, COLOR_NEON_BLUE.g, COLOR_NEON_BLUE.b, (unsigned char)(50 + 60 * pulse) };
    DrawCircle(cx, cy, 60, glow);

    Vector2 sv1 = (Vector2){ v1.x + 2, v1.y + 2 };
    Vector2 sv2 = (Vector2){ v2.x + 2, v2.y + 2 };
    Vector2 sv3 = (Vector2){ v3.x + 2, v3.y + 2 };
    DrawTriangle(sv1, sv2, sv3, (Color){ 0, 0, 0, 50 });

    DrawTriangle(v1, v2, v3, fill);
    DrawTriangleLines(v1, v2, v3, border);

    const char* txt = "Camera lenta";
    DrawText(txt, cx - MeasureText(txt, 16)/2, cy + 50, 16, (Color){ 200, 220, 255, 180 });
}