#include "quiz.h"
#include "effects.h"
#include "particles.h"
#include "commons.h"
#include "raylib.h"
#include <stdio.h>

static Pergunta perguntas[NUM_ETAPAS] = {
    { "Qual a capital da Franca?",
      {"1. Londres", "2. Paris", "3. Berlim"}, 1, COLOR_NEON_BLUE },
    { "Quem pintou a Mona Lisa?",
      {"1. Van Gogh", "2. Picasso", "3. Da Vinci"}, 2, COLOR_NEON_GOLD },
    { "Quanto e 5 x 8?",
      {"1. 40", "2. 45", "3. 35"}, 0, COLOR_NEON_GREEN },
    { "Qual o maior planeta do Sistema Solar?",
      {"1. Terra", "2. Marte", "3. Jupiter"}, 2, COLOR_NEON_PURPLE },
    { "Em que ano o homem pisou na Lua?",
      {"1. 1969", "2. 1975", "3. 1982"}, 0, COLOR_NEON_RED }
};

void InitQuiz(void) {
    // Poderia resetar estatísticas se houver
}

int UpdateQuiz(void) {
    int resposta = -1;
    if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) resposta = 0;
    if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) resposta = 1;
    if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) resposta = 2;

    if (resposta == -1) return -1;

    extern int currentStage;
    if (resposta == perguntas[currentStage].resposta_correta)
        return 1;
    else
        return 0;
}

void DrawQuiz(int stage) {
    if (stage < 0 || stage >= NUM_ETAPAS) return;

    Color bgColor = perguntas[stage].corTema;
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(bgColor, 0.15f));
    DrawRectangle(SCREEN_WIDTH * 0.1f, SCREEN_HEIGHT * 0.2f, 
                  SCREEN_WIDTH * 0.8f, SCREEN_HEIGHT * 0.5f, COLOR_UI_BG);
    DrawRectangleLines(SCREEN_WIDTH * 0.1f, SCREEN_HEIGHT * 0.2f, 
                       SCREEN_WIDTH * 0.8f, SCREEN_HEIGHT * 0.5f, bgColor);

    Pergunta q = perguntas[stage];
    DrawText("PERGUNTA:", 
            (SCREEN_WIDTH - MeasureText("PERGUNTA:", 28)) / 2,
             SCREEN_HEIGHT * 0.23f, 28, WHITE);
    DrawText(q.texto, 
            (SCREEN_WIDTH - MeasureText(q.texto, 26)) / 2, 
             SCREEN_HEIGHT * 0.32f, 26, RAYWHITE);

    for (int i = 0; i < NUM_OPCOES; i++) {
        int yPos = SCREEN_HEIGHT * 0.45f + i * 60;
        DrawRectangle(SCREEN_WIDTH * 0.2f, yPos, SCREEN_WIDTH * 0.6f, 50, Fade(WHITE, 0.1f));
        DrawRectangleLines(SCREEN_WIDTH * 0.2f, yPos, SCREEN_WIDTH * 0.6f, 50, GRAY);
        DrawText(q.opcoes[i], SCREEN_WIDTH * 0.22f, yPos + 12, 22, RAYWHITE);
        DrawText(TextFormat("[%d]", i + 1), SCREEN_WIDTH * 0.7f, yPos + 12, 22, YELLOW);
    }

    DrawText("Use as teclas 1, 2 ou 3 para responder",
            (SCREEN_WIDTH - MeasureText("Use as teclas 1, 2 ou 3 para responder", 20)) / 2,
            SCREEN_HEIGHT * 0.7f, 20, COLOR_NEON_GREEN);
}

Color GetQuizThemeColor(int stage) {
    if (stage < 0 || stage >= NUM_ETAPAS) return COLOR_NEON_BLUE;
    return perguntas[stage].corTema;
}

bool QuizHasNext(int stage) {
    return stage < NUM_ETAPAS - 1;
}

const Pergunta* GetQuestion(int stage) {
    if (stage < 0 || stage >= NUM_ETAPAS) return NULL;
    return &perguntas[stage];
}