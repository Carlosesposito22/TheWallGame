#pragma once
#include "raylib.h"
#include "game_common.h"
#include <stdbool.h>

#define MAX_BALLS 20
#define TOP_SLOTS 7

void InitBalls(void);
bool SpawnBall(bool lastAnswerWasCorrect, int selectedSlot);
void UpdateBalls(Pin *pins, int pinCount, float baseY, float firstSlotX, float slotWidth, bool lastAnswerWasCorrect, float dt);
void DrawBalls(void);
bool AnyBallActive(void);
int GetLastBallSlot(void);

Ball* GetFirstActiveBall(void);