#include "ranking.h"
#include "commons.h"

extern GameScreen currentScreen;

void InitRanking(void) {
    // Aqui você carregaria dados de um arquivo, se tivesse
}

void UpdateRanking(void) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) {
        currentScreen = SCREEN_MENU;
    }
}

void DrawRanking(void) {
    ClearBackground(BLACK);
    DrawText("TOP JOGADORES", 250, 100, 40, YELLOW);

    DrawText("1. Joao - 5000 pts", 280, 200, 30, WHITE);
    DrawText("2. Maria - 3500 pts", 280, 250, 30, GRAY);
    DrawText("3. Pedro - 1200 pts", 280, 300, 30, BROWN);

    DrawText("Pressione [ENTER] para voltar", 240, 800, 20, LIGHTGRAY);
}