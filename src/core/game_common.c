#include "raylib.h"
#include <stdlib.h>
#include "game_common.h"

float RandomFloat(float min, float max) {
    return min + ((float)rand() / RAND_MAX) * (max - min);
}

float MathLerp(float a, float b, float t) {
    return a + t * (b - a);
}

long long factorial(int n) {
    if (n < 0) return 0;
    long long f = 1;
    for (int i = 2; i <= n; i++) f *= i;
    return f;
}

long long combinations(int n, int k) {
    if (k < 0 || k > n) return 0;
    long long denom = factorial(k) * factorial(n - k);
    return denom ? factorial(n) / denom : 0;
}