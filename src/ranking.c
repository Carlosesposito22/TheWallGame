#include "ranking.h"
#include "commons.h" 
#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Variáveis Estáticas
static HighScore highScores[MAX_HIGH_SCORES];
static const char* SCORE_FILE = "scores.txt"; // Mudado para .txt
static int currentPage = 0;

// Variáveis para a Logo
static Texture2D logoTexture;
static bool isLogoLoaded = false;

// --- MANIPULAÇÃO DE ARQUIVO .TXT ---

static void LoadScores() {
    // Zera o array primeiro
    for (int i = 0; i < MAX_HIGH_SCORES; i++) {
        strcpy(highScores[i].name, "---");
        highScores[i].score = 0;
    }

    FILE *file = fopen(SCORE_FILE, "r"); // Modo leitura texto
    if (file) {
        int i = 0;
        // Lê: string até a vírgula, depois um long long
        while (i < MAX_HIGH_SCORES && fscanf(file, "%[^,],%lld\n", highScores[i].name, &highScores[i].score) == 2) {
            i++;
        }
        fclose(file);
    }
}

static void SaveScoresToFile() {
    FILE *file = fopen(SCORE_FILE, "w"); // Modo escrita texto
    if (file) {
        for (int i = 0; i < MAX_HIGH_SCORES; i++) {
            // Só salva se tiver pontuação ou nome válido para não encher o txt de lixo, 
            // mas aqui salvaremos tudo para manter a estrutura fixa se preferir.
            // Vamos salvar no formato: Nome,Pontuacao
            fprintf(file, "%s,%lld\n", highScores[i].name, highScores[i].score);
        }
        fclose(file);
    }
}

// --- LÓGICA DO RANKING ---

void AddHighScore(const char* playerName, long long score) {
    LoadScores(); 

    // Verifica se entra no ranking (é maior que o último da lista de 50)
    if (score > highScores[MAX_HIGH_SCORES - 1].score) {
        // Substitui o último
        strcpy(highScores[MAX_HIGH_SCORES - 1].name, playerName);
        highScores[MAX_HIGH_SCORES - 1].score = score;

        // Bubble sort (Maior -> Menor)
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
    currentPage = 0;

    // Carrega a Logo (apenas se ainda não estiver carregada para evitar recargas desnecessárias)
    if (!isLogoLoaded) {
        logoTexture = LoadTexture("assets/logo-pequena.png");
        if (logoTexture.id != 0) { // Verifica se carregou com sucesso
            isLogoLoaded = true;
            // Opcional: SetTextureFilter(logoTexture, TEXTURE_FILTER_BILINEAR);
        }
    }
}

void UpdateRanking(void) {
    // Paginação
    if (IsKeyPressed(KEY_RIGHT)) {
        currentPage++;
        int maxPages = (MAX_HIGH_SCORES + SCORES_PER_PAGE - 1) / SCORES_PER_PAGE;
        if (currentPage >= maxPages) currentPage = maxPages - 1;
    }
    if (IsKeyPressed(KEY_LEFT)) {
        currentPage--;
        if (currentPage < 0) currentPage = 0;
    }

    // Voltar ao Menu
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) {
        // Descarrega a textura ao sair para limpar memória
        if (isLogoLoaded) {
            UnloadTexture(logoTexture);
            isLogoLoaded = false;
        }
        
        extern GameScreen currentScreen;
        currentScreen = SCREEN_MENU;
    }
}

void DrawRanking(void) {
    ClearBackground((Color){5, 15, 40, 255});
    DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){10, 20, 60, 255}, (Color){5, 10, 30, 255});

    // Variáveis de layout
    int startY = 180;
    int listHeight = SCORES_PER_PAGE * 45; // Altura total da lista (~450px)
    int listCenterY = startY + 40 + (listHeight / 2); // Centro vertical da lista

    // ====================================================================
    // LADO DIREITO: LOGO GRANDE E CENTRALIZADA
    // ====================================================================
    if (isLogoLoaded) {
        float logoSize = 380.0f; // Tamanho aumentado conforme pedido
        
        // Centraliza na metade DIREITA da tela (X: 400 a 800 -> Centro 600)
        float destX = (SCREEN_WIDTH * 0.75f) - (logoSize / 2.0f);
        
        // Centraliza na mesma altura dos jogadores
        float destY = listCenterY - (logoSize / 2.0f); 
        
        Rectangle destRect = { destX, destY, logoSize, logoSize };
        Rectangle srcRect = { 0, 0, (float)logoTexture.width, (float)logoTexture.height };
        
        DrawTexturePro(logoTexture, srcRect, destRect, (Vector2){0, 0}, 0.0f, WHITE);
    } else {
        // Fallback (Quadrado amarelo)
        int size = 300;
        int x = (SCREEN_WIDTH * 0.75f) - (size / 2);
        int y = listCenterY - (size / 2);
        DrawRectangleLines(x, y, size, size, GOLD);
        DrawText("LOGO", x + 100, y + 130, 40, GOLD);
    }

    // ====================================================================
    // LADO ESQUERDO: TABELA DE PONTUAÇÃO
    // ====================================================================

    // Ajusta coordenadas X para ficarem na metade esquerda (0 a 400)
    int titleCenterX = SCREEN_WIDTH / 4; // 200
    
    DrawText("HALL DA FAMA", titleCenterX - MeasureText("HALL DA FAMA", 40)/2, 60, 40, GOLD);
    DrawText("Use < SETAS > para mudar", titleCenterX - MeasureText("Use < SETAS > para mudar", 20)/2, 115, 20, SKYBLUE);
    DrawLine(30, 150, (SCREEN_WIDTH/2) - 30, 150, GOLD);

    // Colunas compactas
    int xPos = 30;
    int xName = 100;
    int xScore = 280;

    DrawText("POS", xPos, startY, 20, YELLOW);
    DrawText("NOME", xName, startY, 20, YELLOW);
    DrawText("PONTUACAO", xScore, startY, 20, YELLOW);

    int startIndex = currentPage * SCORES_PER_PAGE;
    int endIndex = startIndex + SCORES_PER_PAGE;
    if (endIndex > MAX_HIGH_SCORES) endIndex = MAX_HIGH_SCORES;

    for (int i = startIndex; i < endIndex; i++) {
        int relIndex = i - startIndex; 
        int y = startY + 40 + (relIndex * 45);
        
        Color rowColor = WHITE;
        if (currentPage == 0) {
            if (i == 0) rowColor = GOLD;
            else if (i == 1) rowColor = (Color){220, 220, 220, 255};
            else if (i == 2) rowColor = (Color){205, 127, 50, 255};
        }

        // Fundo alternado (apenas metade esquerda)
        if (relIndex % 2 == 0) {
            DrawRectangle(20, y - 5, (SCREEN_WIDTH/2) - 40, 40, (Color){255, 255, 255, 10});
        }

        DrawText(TextFormat("#%02d", i + 1), xPos, y, 24, rowColor);
        
        // Limita visualmente o nome se for muito grande
        char displayBuffer[20];
        if (strlen(highScores[i].name) > 13) {
             strncpy(displayBuffer, highScores[i].name, 10);
             displayBuffer[10] = '\0';
             strcat(displayBuffer, "...");
             DrawText(displayBuffer, xName, y, 24, rowColor);
        } else {
             DrawText(highScores[i].name, xName, y, 24, rowColor);
        }
        
        DrawText(TextFormat("R$ %lld", highScores[i].score), xScore, y, 24, rowColor);
    }

    // Rodapé
    int totalPages = (MAX_HIGH_SCORES + SCORES_PER_PAGE - 1) / SCORES_PER_PAGE;
    DrawText(TextFormat("Pagina %d / %d", currentPage + 1, totalPages), 
             titleCenterX - MeasureText(TextFormat("Pagina %d / %d", currentPage + 1, totalPages), 20) / 2, 
             SCREEN_HEIGHT - 85, 20, LIGHTGRAY);

    DrawText("Pressione [ENTER] para voltar ao MENU", (SCREEN_WIDTH - MeasureText("Pressione [ENTER] para voltar ao MENU", 20)) / 2, SCREEN_HEIGHT - 50, 20, YELLOW);
}