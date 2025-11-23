#pragma once
#include "raylib.h"
#include "game_common.h"

void CalculateRealTimeProbabilities(float ballX, float ballY);
void UpdatePredictionPaths(float ballX, float ballY);
void DrawPredictionCurve(void);
void DrawPredictionPaths(void);

float GetUncertaintyLevel(void);
const char* GetAnalysisLine(int index);
int GetPinsRemaining(void);

float CalculatePredictionAccuracy(void);
void InitPredictionSystem(void);
void ResetPredictionText(void);

extern int mostProbableSlot;
extern float highestProbability;
extern float currentProbabilities[SLOT_COUNT];