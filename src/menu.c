#include "menu.h"
#include "commons.h"
#include "raylib.h"
#include "raymedia.h"

extern GameScreen currentScreen;

static int menuSelection = 0;

typedef enum {
    MENU_STATE_IDLE = 0,
    MENU_STATE_FADE_TO_BLACK,        // 0 -> 1 (escurece)
    MENU_STATE_FADE_FROM_BLACK_VIDEO,// 1 -> 0 (vídeo já rodando)
    MENU_STATE_PLAY_VIDEO,           // vídeo tocando (visível)
    MENU_STATE_FADE_TO_BLACK_END     // 0 -> 1 e troca de tela
} MenuState;

static MenuState menuState = MENU_STATE_IDLE;
static float fadeAlpha = 0.0f;           // 0.0 = sem overlay; 1.0 = preto total
static float fadeSpeed = 2.0f;           // velocidade do fade (ajuste a gosto)

// Config da intro
static const char* INTRO_VIDEO_PATH = "assets/thewall-intro.mp4";
static const char* INTRO_AUDIO_PATH = "assets/thewall-intro.mp3";
static const float INTRO_DURATION   = 7.0f; // segundos

// Vídeo (raymedia)
static MediaStream introMedia;
static bool introVideoLoaded = false;

// Áudio (raylib Music)
static Music introMusic;
static bool introMusicLoaded = false;

static float videoTimer = 0.0f;

// Helpers
static void StartIntroMedia(void) {
    // VIDEO
    if (introVideoLoaded) {
        UnloadMedia(&introMedia);
        introVideoLoaded = false;
    }
    introMedia = LoadMedia(INTRO_VIDEO_PATH);
    introVideoLoaded = IsMediaValid(introMedia);
    if (introVideoLoaded) {
        SetMediaLooping(introMedia, false);
    }

    // AUDIO
    if (!introMusicLoaded) {
        if (!IsAudioDeviceReady()) InitAudioDevice();
        introMusic = LoadMusicStream(INTRO_AUDIO_PATH);
        introMusicLoaded = true; // assumimos sucesso; se o arquivo faltar, raylib loga erro
        SetMusicVolume(introMusic, 1.0f);
    } else {
        StopMusicStream(introMusic);
    }
    // Garante começo no t=0 quando formos dar Play
    if (introMusicLoaded) SeekMusicStream(introMusic, 0.0f);

    videoTimer = 0.0f;
}

static void StopAndUnloadIntro(void) {
    if (introVideoLoaded) {
        UnloadMedia(&introMedia);
        introVideoLoaded = false;
    }
    if (introMusicLoaded) {
        StopMusicStream(introMusic);
        UnloadMusicStream(introMusic);
        introMusicLoaded = false;
    }
}

void InitMenu(void) {
    menuSelection = 0;
    menuState = MENU_STATE_IDLE;
    fadeAlpha = 0.0f;
    videoTimer = 0.0f;
    // Não carregamos nada aqui; carregamos ao disparar a sequência para evitar custo inicial
}

void UpdateMenu(void) {
    float dt = GetFrameTime();

    switch (menuState) {
    case MENU_STATE_IDLE: {
        // Navegação por teclado
        if (IsKeyPressed(KEY_DOWN)) menuSelection = 1;
        if (IsKeyPressed(KEY_UP))   menuSelection = 0;

        if (IsKeyPressed(KEY_ENTER)) {
            if (menuSelection == 0) {
                StartIntroMedia();
                menuState = MENU_STATE_FADE_TO_BLACK;
                fadeAlpha = 0.0f;
            } else {
                currentScreen = SCREEN_RANKING;
            }
        }

        // Mouse
        Vector2 mousePoint = GetMousePosition();
        Rectangle btnJogar = (Rectangle){ SCREEN_WIDTH/2 - 100, 400, 200, 50 };
        Rectangle btnRank  = (Rectangle){ SCREEN_WIDTH/2 - 100, 480, 200, 50 };

        if (CheckCollisionPointRec(mousePoint, btnJogar)) {
            menuSelection = 0;
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                StartIntroMedia();
                menuState = MENU_STATE_FADE_TO_BLACK;
                fadeAlpha = 0.0f;
            }
        }
        if (CheckCollisionPointRec(mousePoint, btnRank)) {
            menuSelection = 1;
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                currentScreen = SCREEN_RANKING;
            }
        }
    } break;

    case MENU_STATE_FADE_TO_BLACK: {
        fadeAlpha += fadeSpeed * dt;
        if (fadeAlpha >= 1.0f) {
            fadeAlpha = 1.0f;
            if (introMusicLoaded) {
                SeekMusicStream(introMusic, 0.0f);
                PlayMusicStream(introMusic);
            }
            videoTimer = 0.0f;
            menuState = MENU_STATE_FADE_FROM_BLACK_VIDEO;
        }
    } break;

    case MENU_STATE_FADE_FROM_BLACK_VIDEO: {
        // Atualiza vídeo e áudio
        if (introVideoLoaded) UpdateMedia(&introMedia);
        if (introMusicLoaded) UpdateMusicStream(introMusic);

        videoTimer += dt;

        // Revela a tela
        fadeAlpha -= fadeSpeed * dt;
        if (fadeAlpha <= 0.0f) {
            fadeAlpha = 0.0f;
            menuState = MENU_STATE_PLAY_VIDEO;
        }

        // Permitir pular
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
            menuState = MENU_STATE_FADE_TO_BLACK_END;
        }
    } break;

    case MENU_STATE_PLAY_VIDEO: {
        if (introVideoLoaded) UpdateMedia(&introMedia);
        if (introMusicLoaded) UpdateMusicStream(introMusic);

        videoTimer += dt;

        // Pular com Enter/Esc
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
            menuState = MENU_STATE_FADE_TO_BLACK_END;
            break;
        }

        // Termina após ~6s (ou você pode checar fim de música/vídeo)
        if (videoTimer >= INTRO_DURATION) {
            menuState = MENU_STATE_FADE_TO_BLACK_END;
        }
    } break;

    case MENU_STATE_FADE_TO_BLACK_END: {
        if (introVideoLoaded) UpdateMedia(&introMedia);
        if (introMusicLoaded) UpdateMusicStream(introMusic);

        fadeAlpha += fadeSpeed * dt;
        if (fadeAlpha >= 1.0f) {
            fadeAlpha = 1.0f;
            // Limpa antes de trocar de tela
            StopAndUnloadIntro();
            currentScreen = SCREEN_GAME;
        }
    } break;
    }
}

void DrawMenu(void) {
    ClearBackground((Color){10, 10, 20, 255});

    // Se estamos no trecho com vídeo, desenhe o frame
    if ((menuState == MENU_STATE_FADE_FROM_BLACK_VIDEO || menuState == MENU_STATE_PLAY_VIDEO || menuState == MENU_STATE_FADE_TO_BLACK_END) && introVideoLoaded) {
        float zoomFactor = 0.9f;

        // Tamanho final ajustado
        float destWidth  = introMedia.videoTexture.width  * zoomFactor;
        float destHeight = introMedia.videoTexture.height * zoomFactor;

        // Coordenadas centradas na janela
        float destX = (SCREEN_WIDTH  - destWidth)  / 2.0f;
        float destY = (SCREEN_HEIGHT - destHeight) / 2.0f;

        // Retângulos fonte e destino
        Rectangle src  = { 0, 0, (float)introMedia.videoTexture.width, (float)introMedia.videoTexture.height };
        Rectangle dest = { destX, destY, destWidth, destHeight };

        // Desenha o vídeo ajustado
        DrawTexturePro(introMedia.videoTexture, src, dest, (Vector2){ 0, 0 }, 0.0f, WHITE);
    } else {
        // Tela de Menu
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

    // Overlay do fade (preto)
    if (fadeAlpha > 0.0f) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, (unsigned char)(fadeAlpha * 255)});
    }
}