#pragma once
#include "raylib.h"
#include "game_common.h"

void InitBall(Ball *ball);
bool TrySpawnBall(Ball *ball, bool lastAnswerWasCorrect);
void DrawBall(const Ball *ball);