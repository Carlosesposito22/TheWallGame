// physics.h
#pragma once
#include "game_common.h"

void UpdateBallPhysics(Ball* ball, float dt, Pin* pins, int pinCount, float screenWidth, float baseY, float firstSlotX, float slotWidth, void (*OnSlotLanded)(int slotIndex));