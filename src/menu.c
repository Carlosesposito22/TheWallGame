#include "menu.h"
#include "commons.h"

extern GameScreen currentScreen; 

static int menuSelection = 0;

void InitMenu(void) {
    menuSelection = 0;
}

void UpdateMenu(void) {
    if (IsKeyPressed(KEY_DOWN)) menuSelection = 1;
    if (IsKeyPressed(KEY_UP)) menuSelection = 0;

    if (IsKeyPressed(KEY_ENTER)) {
        if (menuSelection == 0) {
            currentScreen = SCREEN_GAME;
        } else {
            currentScreen = SCREEN_RANKING;
        }
    }
    
    Vector2 mousePoint = GetMousePosition();
    Rectangle btnJogar = { SCREEN_WIDTH/2 - 100, 400, 200, 50 };
    Rectangle btnRank = { SCREEN_WIDTH/2 - 100, 480, 200, 50 };
    
    if (CheckCollisionPointRec(mousePoint, btnJogar)) {
        menuSelection = 0;
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) currentScreen = SCREEN_GAME;
    }
    if (CheckCollisionPointRec(mousePoint, btnRank)) {
        menuSelection = 1;
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) currentScreen = SCREEN_RANKING;
    }
}

void DrawMenu(void) {
    ClearBackground((Color){10, 10, 20, 255});

    const char* title = "THE WALL GAME";
    DrawText(title, (SCREEN_WIDTH - MeasureText(title, 60))/2, 150, 60, GOLD);

    Color colorJogar = (menuSelection == 0) ? SKYBLUE : DARKBLUE;
    DrawRectangle(SCREEN_WIDTH/2 - 100, 400, 200, 50, colorJogar);
    DrawText("JOGAR", SCREEN_WIDTH/2 - MeasureText("JOGAR", 20)/2, 415, 20, WHITE);

    Color colorRank = (menuSelection == 1) ? SKYBLUE : DARKBLUE;
    DrawRectangle(SCREEN_WIDTH/2 - 100, 480, 200, 50, colorRank);
    DrawText("RANKING", SCREEN_WIDTH/2 - MeasureText("RANKING", 20)/2, 495, 20, WHITE);

    DrawText("Use SETAS e ENTER ou MOUSE", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT - 50, 15, GRAY);
}