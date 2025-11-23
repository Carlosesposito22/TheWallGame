#include "raylib.h"
#include <stdlib.h>
#include "game_common.h"

float RandomFloat(float min, float max) {
    return min + ((float)rand() / RAND_MAX) * (max - min);
}