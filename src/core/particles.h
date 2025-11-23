#pragma once
#include "raylib.h"
#include "game_common.h"

void InitParticles(void);
void CreateParticles(float x, float y, Color color, int count);
void UpdateParticles(float dt);
void UpdateBallTrail(float ballX, float ballY, float dt);
void DrawParticles(void);