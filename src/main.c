#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOMINMAX

#include "raylib.h"
#include "commons.h"
#include "menu.h"
#include "game.h"
#include "ranking.h"
#include <stdlib.h>
#include <time.h>

GameScreen currentScreen = SCREEN_MENU;
GameScreen lastScreen = -1; 

void TransitionScreen() {
    if (currentScreen != lastScreen) {
        switch (currentScreen) {
            case SCREEN_MENU: InitMenu(); break;
            case SCREEN_GAME: InitGame(); break;
            case SCREEN_RANKING: InitRanking(); break;
        }
        lastScreen = currentScreen;
    }
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "The Wall");
    SetTargetFPS(60);
    srand((unsigned)time(NULL));

    while (!WindowShouldClose()) {
        
        TransitionScreen();

        switch (currentScreen) {
            case SCREEN_MENU:    UpdateMenu(); break;
            case SCREEN_GAME:    UpdateGame(); break;
            case SCREEN_RANKING: UpdateRanking(); break;
        }

        BeginDrawing();
        switch (currentScreen) {
            case SCREEN_MENU:    DrawMenu(); break;
            case SCREEN_GAME:    DrawGame(); break;
            case SCREEN_RANKING: DrawRanking(); break;
        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}