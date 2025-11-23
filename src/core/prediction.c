#include "prediction.h"
#include "game_common.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    Vector2 start;
    Vector2 end;
    float alpha;
    float probability;
} PredictionPath;

static PredictionPath predictionPaths[MAX_PREDICTION_PATHS];
float currentProbabilities[SLOT_COUNT];
static char analysisText[3][64];
static float uncertaintyLevel = 1.0f;
static int pinsRemaining = NUM_PINS_Y;
int mostProbableSlot = -1;
float highestProbability = 0.0f;

void CalculateRealTimeProbabilities(float ballX, float ballY) {
    float firstPinY = 120.0f;
    float baseY = 750.0f;
    float firstSlotX = (SCREEN_WIDTH - SLOT_COUNT * PIN_SPACING) * 0.5f;
    float slotWidth = PIN_SPACING;

    float progress = (ballY - firstPinY) / ((baseY - BALL_RADIUS) - firstPinY);
    progress = fmaxf(0.0f, fminf(1.0f, progress));

    pinsRemaining = (int)((1.0f - progress) * NUM_PINS_Y);
    pinsRemaining = fmax(1, pinsRemaining);

    int centerSlot = (int)((ballX - firstSlotX) / slotWidth);
    centerSlot = fmax(0, fmin(SLOT_COUNT - 1, centerSlot));

    highestProbability = 0.0f;
    mostProbableSlot = -1;

    for (int targetSlot = 0; targetSlot < SLOT_COUNT; targetSlot++) {
        int distance = abs(targetSlot - centerSlot);
        if (distance > pinsRemaining) {
            currentProbabilities[targetSlot] = 0.0f;
            continue;
        }

        double prob = (double)combinations(pinsRemaining, distance) * pow(0.5, pinsRemaining);
        currentProbabilities[targetSlot] = (float)prob;

        if (prob > highestProbability) {
            highestProbability = (float)prob;
            mostProbableSlot = targetSlot;
        }
    }

    // Entropia (nível de incerteza)
    uncertaintyLevel = 0.0f;
    for (int i = 0; i < SLOT_COUNT; i++) {
        if (currentProbabilities[i] > 0.0f) {
            uncertaintyLevel -= currentProbabilities[i] * log2f(currentProbabilities[i]);
        }
    }
    uncertaintyLevel /= log2f(SLOT_COUNT);

    // Atualiza mensagens visuais da análise
    if (pinsRemaining == NUM_PINS_Y) {
        snprintf(analysisText[0], sizeof(analysisText[0]), "Incerteza Máxima (50/50)");
        snprintf(analysisText[1], sizeof(analysisText[1]), "Todos os caminhos possíveis");
        snprintf(analysisText[2], sizeof(analysisText[2]), "Distribuição Uniforme");
    } else if (pinsRemaining > NUM_PINS_Y / 2) {
        snprintf(analysisText[0], sizeof(analysisText[0]), "Alta Incerteza");
        snprintf(analysisText[1], sizeof(analysisText[1]), "Múltiplos caminhos prováveis");
        snprintf(analysisText[2], sizeof(analysisText[2]), "Convergindo gradualmente");
    } else if (pinsRemaining > 2) {
        snprintf(analysisText[0], sizeof(analysisText[0]), "Incerteza Moderada");
        snprintf(analysisText[1], sizeof(analysisText[1]), "Focando no slot %d", mostProbableSlot + 1);
        snprintf(analysisText[2], sizeof(analysisText[2]), "Curva se estreitando");
    } else {
        snprintf(analysisText[0], sizeof(analysisText[0]), "Baixa Incerteza");
        snprintf(analysisText[1], sizeof(analysisText[1]), "Destino quase definido");
        snprintf(analysisText[2], sizeof(analysisText[2]), "Slot %d (%.1f%%)",
                 mostProbableSlot + 1, highestProbability * 100);
    }
}

void UpdatePredictionPaths(float ballX, float ballY) {
    float firstSlotX = (SCREEN_WIDTH - SLOT_COUNT * PIN_SPACING) * 0.5f;
    float slotWidth = PIN_SPACING;
    float baseY = 750.0f;

    for (int i = 0; i < MAX_PREDICTION_PATHS; i++) {
        predictionPaths[i].alpha = MathLerp(predictionPaths[i].alpha, 0.0f, 0.3f);
    }

    int pathIndex = 0;
    for (int slot = 0; slot < SLOT_COUNT && pathIndex < MAX_PREDICTION_PATHS; slot++) {
        if (currentProbabilities[slot] > 0.05f) {
            float slotCenterX = firstSlotX + slot * slotWidth + slotWidth / 2;
            predictionPaths[pathIndex].start = (Vector2){ ballX, ballY };
            predictionPaths[pathIndex].end   = (Vector2){ slotCenterX, baseY - BALL_RADIUS };
            predictionPaths[pathIndex].alpha = currentProbabilities[slot] * 0.8f;
            predictionPaths[pathIndex].probability = currentProbabilities[slot];
            pathIndex++;
        }
    }
}

void DrawPredictionCurve(void) {
    if (pinsRemaining <= 0) return;

    int pointCount = 50;
    Vector2 points[pointCount];

    float firstSlotX = (SCREEN_WIDTH - SLOT_COUNT * PIN_SPACING) * 0.5f;
    float slotWidth  = PIN_SPACING;
    float baseY      = 750.0f;

    float maxProb = 0.0f;
    for (int i = 0; i < SLOT_COUNT; i++) {
        if (currentProbabilities[i] > maxProb) {
            maxProb = currentProbabilities[i];
        }
    }
    if (maxProb <= 0.0f) return;

    for (int i = 0; i < pointCount; i++) {
        float t = (float)i / (pointCount - 1);
        int slot = (int)(t * (SLOT_COUNT - 1));
        float x = firstSlotX + slot * slotWidth + slotWidth / 2;
        float y = baseY - (currentProbabilities[slot] / maxProb) * 150.0f;
        points[i] = (Vector2){ x, y };
    }

    for (int i = 0; i < pointCount - 1; i++) {
        float alpha = 150.0f * (0.3f + 0.7f * uncertaintyLevel);
        Color curveColor = (Color){
            COLOR_PREDICTION.r,
            COLOR_PREDICTION.g,
            COLOR_PREDICTION.b,
            (unsigned char)alpha
        };
        DrawLineEx(points[i], points[i + 1], 3.0f, curveColor);
    }

    for (int i = 0; i < pointCount - 1; i++) {
        Vector2 quad[4] = {
            points[i],
            points[i + 1],
            {points[i + 1].x, baseY},
            {points[i].x, baseY}
        };
        Color fillColor = (Color){
            COLOR_PREDICTION.r,
            COLOR_PREDICTION.g,
            COLOR_PREDICTION.b,
            30
        };
        DrawTriangle(quad[0], quad[1], quad[2], fillColor);
        DrawTriangle(quad[0], quad[2], quad[3], fillColor);
    }
}

void DrawPredictionPaths(void) {
    for (int i = 0; i < MAX_PREDICTION_PATHS; i++) {
        if (predictionPaths[i].alpha > 0.01f) {
            Color c = COLOR_PATH;
            c.a = (unsigned char)(predictionPaths[i].alpha * 255);
            DrawLineEx(predictionPaths[i].start, predictionPaths[i].end,
                       2.0f * predictionPaths[i].probability, c);
            float pulse = sinf(GetTime() * 4.0f) * 0.3f + 0.7f;
            DrawCircleV(predictionPaths[i].end,
                        6.0f * predictionPaths[i].probability * pulse, c);
        }
    }
}


float GetUncertaintyLevel(void) { 
    return uncertaintyLevel; 
}

int GetPinsRemaining(void) { 
    return pinsRemaining; 
}

const char* GetAnalysisLine(int index) {
    if (index < 0 || index >= 3) 
        return "";
    return analysisText[index];
}


float CalculatePredictionAccuracy(void) {
    return 75.0f + (float)(rand() % 20); // Simula 75-95% de precisão
}

void InitPredictionSystem(void) {
    for (int i = 0; i < SLOT_COUNT; i++) {
        currentProbabilities[i] = 0.0f;
    }
    for (int i = 0; i < MAX_PREDICTION_PATHS; i++) {
        predictionPaths[i].alpha = 0.0f;
    }
    strcpy(analysisText[0], "Aguardando início...");
    strcpy(analysisText[1], "Preparando análise...");
    strcpy(analysisText[2], "Sistema pronto");
    uncertaintyLevel = 1.0f;
    pinsRemaining = NUM_PINS_Y;
    mostProbableSlot = -1;
    highestProbability = 0.0f;
}

void ResetPredictionText(void) {
    strcpy(analysisText[0], "Pronto para análise");
    strcpy(analysisText[1], "Aguardando lançamento");
    strcpy(analysisText[2], "Incerteza máxima");
    uncertaintyLevel = 1.0f;
    pinsRemaining = NUM_PINS_Y;
    mostProbableSlot  = -1;
    highestProbability = 0.0f;
}

void DrawPredictionPanel(int totalBolas, int currentStage, int comboCount, int slotCounts[], int currentSlotIndex, int isBallActive, int slotIndex) {
    int statsPanelWidth = 400;
    int statsPanelX = SCREEN_WIDTH - statsPanelWidth - 20;

    // Painel principal
    DrawRectangle(statsPanelX, 20, statsPanelWidth, 300, COLOR_UI_BG);
    DrawRectangleLines(statsPanelX, 20, statsPanelWidth, 300, COLOR_UI_BORDER);

    DrawText("HUB DE ANÁLISE", statsPanelX + 15, 35, 22, COLOR_NEON_GOLD);

    // Informações básicas
    DrawText(TextFormat("Bolas: %d", totalBolas), statsPanelX + 20, 70, 18, LIGHTGRAY);
    DrawText(TextFormat("Stage: %d/%d", currentStage + 1, NUM_ETAPAS),
             statsPanelX + 20, 95, 18, LIGHTGRAY);
    DrawText(TextFormat("Combo: x%d", comboCount),
             statsPanelX + statsPanelWidth - 100, 95, 18, COLOR_NEON_GREEN);

    // Barra de incerteza
    int uncertaintyBarWidth = statsPanelWidth - 40;
    DrawRectangle(statsPanelX + 20, 125, uncertaintyBarWidth, 12, (Color){50, 50, 70, 255});
    DrawRectangle(statsPanelX + 20, 125,
                  (int)(uncertaintyBarWidth * GetUncertaintyLevel()), 12,
                  (Color){255, (int)(255 * GetUncertaintyLevel()),
                          (int)(100 * (1.0f - GetUncertaintyLevel())), 255});
    DrawText(TextFormat("Incerteza: %.0f%%", GetUncertaintyLevel() * 100),
             statsPanelX + 20, 140, 14, LIGHTGRAY);

    // Se bola está ativa
    DrawText("ANÁLISE PREDITIVA:", statsPanelX + 20, 165, 16, COLOR_NEON_BLUE);

    if (isBallActive && slotIndex == -1) {
        DrawText(GetAnalysisLine(0), statsPanelX + 25, 190, 16, WHITE);
        DrawText(GetAnalysisLine(1), statsPanelX + 25, 215, 14, LIGHTGRAY);
        DrawText(GetAnalysisLine(2), statsPanelX + 25, 235, 14, LIGHTGRAY);

        // Slot mais provável
        if (mostProbableSlot >= 0) {
            DrawText(TextFormat("Mais provável: Slot %d (%.1f%%)", 
                                mostProbableSlot + 1, highestProbability * 100),
                     statsPanelX + 25, 260, 14, COLOR_NEON_GREEN);
        }

        // Pinos restantes
        DrawText(TextFormat("Pinos restantes: %d/%d", GetPinsRemaining(), NUM_PINS_Y),
                 statsPanelX + 25, 285, 14, COLOR_NEON_PURPLE);
    } else {
        DrawText("Aguardando lançamento...", statsPanelX + 25, 190, 16, GRAY);
        DrawText("A análise começará quando", statsPanelX + 25, 215, 14, GRAY);
        DrawText("a bola for solta", statsPanelX + 25, 235, 14, GRAY);
    }
}

void DrawPredictionChart(int totalBolas, int slotCounts[], int currentSlotIndex, float baseY) {
    int graphHeight = 150;
    int graphY = baseY + 50;
    int graphWidth = SCREEN_WIDTH - 40;
    int graphX = 20;

    // Fundo
    DrawRectangle(graphX, graphY, graphWidth, graphHeight, (Color){15, 20, 35, 220});
    DrawRectangleLines(graphX, graphY, graphWidth, graphHeight, COLOR_UI_BORDER);

    DrawText("DISTRIBUIÇÃO EMPÍRICA - HISTÓRICO DE BOLAS",
             graphX + 10, graphY + 5, 16, COLOR_NEON_GOLD);

    // Gráfico de barras
    int maxCount = 0;
    for (int i = 0; i < SLOT_COUNT; i++) {
        if (slotCounts[i] > maxCount) maxCount = slotCounts[i];
    }

    float barWidth = (float)graphWidth / SLOT_COUNT;

    for (int i = 0; i < SLOT_COUNT; i++) {
        float x = graphX + i * barWidth;
        DrawRectangle(x + 2, graphY + graphHeight - 20, barWidth - 4, 15, (Color){40, 40, 60, 255});

        if (slotCounts[i] > 0) {
            float barHeight = maxCount > 0 
                ? (float)slotCounts[i] / maxCount * (graphHeight - 50) : 0;
            Color barColor = (i == currentSlotIndex) ? COLOR_NEON_GOLD : COLOR_NEON_BLUE;
            DrawRectangle(x + 2, graphY + graphHeight - 20 - barHeight, barWidth - 4, barHeight, barColor);
            DrawRectangleLines(x + 2, graphY + graphHeight - 20 - barHeight, barWidth - 4, barHeight, WHITE);

            if (barHeight > 25) {
                char countText[16];
                sprintf(countText, "%d", slotCounts[i]);
                int textWidth = MeasureText(countText, 12);
                DrawText(countText, x + barWidth/2 - textWidth/2,
                         graphY + graphHeight - 25 - barHeight, 12, WHITE);
            }

            if (totalBolas > 0) {
                float percentage = (float)slotCounts[i] / totalBolas * 100.0f;
                char percentText[16];
                sprintf(percentText, "%.1f%%", percentage);
                int textWidth = MeasureText(percentText, 10);
                DrawText(percentText, x + barWidth/2 - textWidth/2,
                         graphY + graphHeight - 5, 10, LIGHTGRAY);
            }
        }

        char slotText[8];
        sprintf(slotText, "%d", i + 1);
        int slotTextWidth = MeasureText(slotText, 12);
        DrawText(slotText, x + barWidth/2 - slotTextWidth/2,
                 graphY + graphHeight + 5, 12, WHITE);
    }

    if (totalBolas > 0) {
        DrawText(TextFormat("Total de bolas: %d | Distribuição baseada no histórico", totalBolas),
                 graphX + 10, graphY + graphHeight + 25, 12, LIGHTGRAY);
    }
}