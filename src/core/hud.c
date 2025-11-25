#include "hud.h"
#include "prediction.h"
#include "commons.h"
#include "raylib.h"
#include <stdio.h>
#include <math.h>

void DrawStartScreenHUD(void)
{
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.85f));
    float titleGlow = sinf(GetTime() * 2.5f) * 0.4f + 0.6f;

    DrawText("THE WALL",
             (SCREEN_WIDTH - MeasureText("THE WALL", 120)) / 2, 250, 120,
             (Color){COLOR_NEON_GOLD.r, COLOR_NEON_GOLD.g, COLOR_NEON_GOLD.b, (int)(255 * titleGlow)});

    DrawText("Simulador de Distribuição Binomial",
             (SCREEN_WIDTH - MeasureText("Simulador de Distribuição Binomial", 36)) / 2, 400, 36, RAYWHITE);

    DrawText("COM ANÁLISE PREDITIVA EM TEMPO REAL",
             (SCREEN_WIDTH - MeasureText("COM ANÁLISE PREDITIVA EM TEMPO REAL", 24)) / 2, 470, 24, COLOR_NEON_BLUE);

    if (((int)(GetTime() * 2) % 2) == 0) {
        DrawText("Pressione ENTER para começar",
                 (SCREEN_WIDTH - MeasureText("Pressione ENTER para começar", 28)) / 2, 550, 28, COLOR_NEON_GREEN);
    }

    DrawText("Trabalho de Estatística - Probabilidade e Combinatória",
             (SCREEN_WIDTH - MeasureText("Trabalho de Estatística - Probabilidade e Combinatória", 20)) / 2,
             SCREEN_HEIGHT - 80, 20, GRAY);
}

void DrawWaitingForBallHUD(bool lastAnswerWasCorrect)
{
    DrawRectangle(0, 0, SCREEN_WIDTH, 220, Fade(BLACK, 0.8f));

    if (lastAnswerWasCorrect) {
        DrawText("✓ RESPOSTA CORRETA!",
                 (SCREEN_WIDTH - MeasureText("✓ RESPOSTA CORRETA!", 40)) / 2, 80, 40, COLOR_NEON_GREEN);
        DrawText("O valor da bola será SOMADO à sua pontuação",
                 (SCREEN_WIDTH - MeasureText("O valor da bola será SOMADO à sua pontuação", 24)) / 2, 130, 24, LIME);
    } else {
        DrawText("✗ RESPOSTA INCORRETA",
                 (SCREEN_WIDTH - MeasureText("✗ RESPOSTA INCORRETA", 40)) / 2, 80, 40, COLOR_NEON_RED);
        DrawText("O valor da bola será SUBTRAÍDO da sua pontuação",
                 (SCREEN_WIDTH - MeasureText("O valor da bola será SUBTRAÍDO da sua pontuação", 24)) / 2, 130, 24, ORANGE);
    }

    float pulse = sinf(GetTime() * 4.0f) * 0.5f + 0.5f;
    DrawText("PRESSIONE ESPAÇO PARA SOLTAR A BOLA",
             (SCREEN_WIDTH - MeasureText("PRESSIONE ESPAÇO PARA SOLTAR A BOLA", 28)) / 2, 180, 28,
             (Color){255, 255, 100, (int)(255 * pulse)});
}

void DrawBallLandedHUD(bool lastAnswerWasCorrect, int currentStage)
{
    DrawRectangle(0, 0, SCREEN_WIDTH, 240, Fade(BLACK, 0.8f));
    char resultadoStr[100];
    Color resultadoCor;

    if (lastAnswerWasCorrect) {
        snprintf(resultadoStr, sizeof(resultadoStr), "✓ Rodada concluída! Confira sua pontuação!");
        resultadoCor = COLOR_NEON_GREEN;
    } else {
        snprintf(resultadoStr, sizeof(resultadoStr), "✗ Rodada concluída! Pontos reduzidos!");
        resultadoCor = COLOR_NEON_RED;
    }

    DrawText(resultadoStr,
             (SCREEN_WIDTH - MeasureText(resultadoStr, 30)) / 2, 130, 30, resultadoCor);

    if (((int)(GetTime() * 2) % 2) == 0) {
        if (currentStage < NUM_ETAPAS - 1) {
            DrawText("Pressione ENTER para a próxima etapa...",
                     (SCREEN_WIDTH - MeasureText("Pressione ENTER para a próxima etapa...", 22)) / 2, 230, 22, YELLOW);
        } else {
            DrawText("Pressione ENTER para ver seu resultado...",
                     (SCREEN_WIDTH - MeasureText("Pressione ENTER para ver seu resultado...", 22)) / 2, 230, 22, YELLOW);
        }
    }
}

void DrawNameInputHUD(const char* playerName, int letterCount, long long totalScore)
{
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade((Color){10, 20, 40, 255}, 0.95f));

    int centerX = SCREEN_WIDTH / 2;
    int centerY = SCREEN_HEIGHT / 2;

    DrawText("FIM DE JOGO!",
             centerX - MeasureText("FIM DE JOGO!", 60)/2, centerY - 200, 60, COLOR_NEON_GOLD);

    char scoreText[50];
    snprintf(scoreText, sizeof(scoreText), "Pontuação Final: %lld", totalScore);
    DrawText(scoreText, centerX - MeasureText(scoreText, 40)/2, centerY - 120, 40, COLOR_NEON_GREEN);

    DrawText(TextFormat("Precisão da Análise: %.1f%%", CalculatePredictionAccuracy()),
             centerX - MeasureText(TextFormat("Precisão da Análise: %.1f%%", CalculatePredictionAccuracy()), 24)/2,
             centerY - 70, 24, COLOR_NEON_BLUE);

    DrawText("Digite seu nome para o ranking:",
             centerX - MeasureText("Digite seu nome para o ranking:", 28)/2, centerY - 30, 28, WHITE);

    DrawRectangle(centerX - 250, centerY, 500, 70, (Color){30, 30, 50, 255});
    DrawRectangleLines(centerX - 250, centerY, 500, 70, COLOR_NEON_GOLD);
    DrawText(playerName, centerX - 240, centerY + 18, 36, YELLOW);

    if (((int)(GetTime() * 2) % 2) == 0) {
        int textWidth = MeasureText(playerName, 36);
        DrawText("_", centerX - 240 + textWidth, centerY + 18, 36, COLOR_NEON_GOLD);
    }

    DrawText("Pressione ENTER para confirmar",
             centerX - MeasureText("Pressione ENTER para confirmar", 20)/2, centerY + 90, 20, LIGHTGRAY);

    if (letterCount == 0) {
        DrawText("Digite pelo menos um caractere",
                 centerX - MeasureText("Digite pelo menos um caractere", 18)/2, centerY + 120, 18, ORANGE);
    }
}

void DrawGameOverHUD(long long totalScore)
{
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.9f));

    DrawText("FIM DE JOGO",
             (SCREEN_WIDTH - MeasureText("FIM DE JOGO", 80)) / 2,
             150, 80, COLOR_NEON_GOLD);

    DrawText(TextFormat("Pontuação Final: %lld", totalScore),
             (SCREEN_WIDTH - MeasureText(TextFormat("Pontuação Final: %lld", totalScore), 40)) / 2,
             250, 40, YELLOW);

    DrawText("Pressione R para reiniciar",
             (SCREEN_WIDTH - MeasureText("Pressione R para reiniciar", 28)) / 2,
             350, 28, RAYWHITE);

    DrawText("Pressione Q para voltar ao menu",
             (SCREEN_WIDTH - MeasureText("Pressione Q para voltar ao menu", 28)) / 2,
             390, 28, LIGHTGRAY);
}

void DrawChoosingSlotHUD(void) {
    DrawText("ESCOLHA O SLOT DE LANÇAMENTO", 
             SCREEN_WIDTH / 2 - MeasureText("ESCOLHA O SLOT DE LANÇAMENTO", 30) / 2, 
             SCREEN_HEIGHT / 2, 30, WHITE);
    DrawText("Use SETAS ou A/D para mover. ENTER para confirmar.", 
             SCREEN_WIDTH / 2 - MeasureText("Use SETAS ou A/D para mover. ENTER para confirmar.", 20) / 2, 
             SCREEN_HEIGHT / 2 + 40, 20, COLOR_NEON_BLUE);
}