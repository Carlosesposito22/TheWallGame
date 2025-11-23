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

void DrawPredictionPanel(int totalBolas, int currentStage, int comboCount, int slotCounts[], int currentSlotIndex, int isBallActive, int slotIndex);
void DrawPredictionChart(int totalBolas, int slotCounts[], int currentSlotIndex, float baseY);

extern int mostProbableSlot;
extern float highestProbability;
extern float currentProbabilities[SLOT_COUNT];