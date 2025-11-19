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

Music bgMusic; 

GameScreen currentScreen = SCREEN_MENU;
GameScreen lastScreen = -1; 

void TransitionScreen() {
    if (currentScreen != lastScreen) {
        switch (currentScreen) {
            case SCREEN_MENU: 
                InitMenu(); 
                if (!IsMusicStreamPlaying(bgMusic)) PlayMusicStream(bgMusic);
                break;
            case SCREEN_GAME: 
                InitGame(); 
                StopMusicStream(bgMusic);
                break;
            case SCREEN_RANKING: 
                InitRanking();
                if (!IsMusicStreamPlaying(bgMusic)) PlayMusicStream(bgMusic);
                break;
        }
        lastScreen = currentScreen;
    }
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "The Wall");
    InitAudioDevice();
    SetTargetFPS(60);
    srand((unsigned)time(NULL));

    bgMusic = LoadMusicStream("assets/musica-principal.mp3");
    SetMusicVolume(bgMusic, 1.0f);
    PlayMusicStream(bgMusic);

    while (!WindowShouldClose()) {

        if (IsMusicStreamPlaying(bgMusic)) {
            UpdateMusicStream(bgMusic);
        }

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

    // Limpeza
    UnloadMusicStream(bgMusic);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}