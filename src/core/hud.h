#ifndef HUD_H
#define HUD_H

#include "raylib.h"
#include "game_common.h"

void DrawStartScreenHUD(void);
void DrawWaitingForBallHUD(bool lastAnswerWasCorrect);
void DrawBallLandedHUD(bool lastAnswerWasCorrect, int currentStage);
void DrawNameInputHUD(const char* playerName, int letterCount, long long totalScore);
void DrawGameOverHUD(long long totalScore);

#endif