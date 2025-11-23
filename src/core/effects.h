// effects.h
#pragma once
#include "raylib.h"
#include "game_common.h"

void StartScreenShake(float intensity);
void UpdateScreenShake(float dt);
void UpdateSlowMotion(void);
float ApplyTimeScale(float dt);
void DrawSlowMotionIndicator(void);
