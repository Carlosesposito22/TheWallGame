// physics.h
#pragma once
#include "raylib.h"
#include "game_common.h"
#include "particles.h"

int UpdateBallPhysics(
    Ball* ball,
    Pin* pins, int pinCount,
    float baseY, float firstSlotX, float slotWidth,
    int slotCounts[], int* totalBolas,
    int slotValues[], long long* totalScore,
    int lastAnswerWasCorrect,
    float dt
);