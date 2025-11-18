#include "ranking.h"
#include "commons.h" // Para SCREEN_WIDTH, SCREEN_HEIGHT, currentScreen
#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static HighScore highScores[MAX_HIGH_SCORES];
static const char* SCORE_FILE = "scores.dat";

// Carrega do arquivo
static void LoadScores() {
    FILE *file = fopen(SCORE_FILE, "rb");
    if (file) {
        fread(highScores, sizeof(HighScore), MAX_HIGH_SCORES, file);
        fclose(file);
    } else {
        // Se não existir, zera tudo
        for (int i = 0; i < MAX_HIGH_SCORES; i++) {
            strcpy(highScores[i].name, "---");
            highScores[i].score = 0;
        }
    }
}

// Salva no arquivo
static void SaveScoresToFile() {
    FILE *file = fopen(SCORE_FILE, "wb");
    if (file) {
        fwrite(highScores, sizeof(HighScore), MAX_HIGH_SCORES, file);
        fclose(file);
    }
}

// Adiciona score, ordena e salva
void AddHighScore(const char* playerName, long long score) {
    LoadScores(); // Garante que temos a versão mais recente

    // 1. Insere o novo score na última posição temporariamente ou desloca
    // Vamos usar um algoritmo simples: Insere no fim e ordena
    
    // Verifica se entra no ranking (é maior que o menor score?)
    if (score > highScores[MAX_HIGH_SCORES - 1].score) {
        // Substitui o último
        strcpy(highScores[MAX_HIGH_SCORES - 1].name, playerName);
        highScores[MAX_HIGH_SCORES - 1].score = score;

        // Bubble sort simples para reordenar (Maior para o Menor)
        for (int i = 0; i < MAX_HIGH_SCORES - 1; i++) {
            for (int j = 0; j < MAX_HIGH_SCORES - i - 1; j++) {
                if (highScores[j].score < highScores[j + 1].score) {
                    HighScore temp = highScores[j];
                    highScores[j] = highScores[j + 1];
                    highScores[j + 1] = temp;
                }
            }
        }
        SaveScoresToFile();
    }
}

void InitRanking(void) {
    LoadScores();
}

void UpdateRanking(void) {
    // Pressione ESC ou ENTER para voltar ao Menu
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) {
        extern GameScreen currentScreen; // Referencia a global
        currentScreen = SCREEN_MENU;
    }
}

void DrawRanking(void) {
    // Fundo Azul The Wall
    ClearBackground((Color){5, 15, 40, 255});
    DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){10, 20, 60, 255}, (Color){5, 10, 30, 255});

    // Título
    const char* title = "HALL DA FAMA";
    DrawText(title, (SCREEN_WIDTH - MeasureText(title, 50)) / 2, 80, 50, GOLD);
    DrawLine(200, 140, SCREEN_WIDTH - 200, 140, GOLD);

    // Cabeçalho da Tabela
    int startY = 200;
    DrawText("RANK", 150, startY, 20, LIGHTGRAY);
    DrawText("NOME", 300, startY, 20, LIGHTGRAY);
    DrawText("PONTUACAO", 550, startY, 20, LIGHTGRAY);

    // Lista
    for (int i = 0; i < MAX_HIGH_SCORES; i++) {
        int y = startY + 40 + (i * 50);
        Color rowColor = WHITE;
        
        // Cores especiais para top 3
        if (i == 0) rowColor = GOLD;
        else if (i == 1) rowColor = (Color){192, 192, 192, 255}; // Prata
        else if (i == 2) rowColor = (Color){205, 127, 50, 255};  // Bronze

        // Desenha Rank #
        DrawText(TextFormat("#%d", i + 1), 160, y, 30, rowColor);
        
        // Desenha Nome
        DrawText(highScores[i].name, 300, y, 30, rowColor);
        
        // Desenha Score (formatado R$)
        DrawText(TextFormat("R$ %lld", highScores[i].score), 550, y, 30, rowColor);
        
        // Linha separadora sutil
        DrawLine(150, y + 35, 750, y + 35, (Color){255, 255, 255, 30});
    }

    DrawText("Pressione [ENTER] para voltar ao MENU", (SCREEN_WIDTH - MeasureText("Pressione [ENTER] para voltar ao MENU", 20)) / 2, SCREEN_HEIGHT - 50, 20, YELLOW);
}