#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOMINMAX

#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

// Definições do Jogo
#define NUM_PINS_X 12
#define NUM_PINS_Y 9
#define PIN_SPACING 60
#define PIN_RADIUS 5
#define BALL_RADIUS 11
#define GRAVITY 500.0f
#define SLOT_COUNT (NUM_PINS_X + 1)
#define NUM_ETAPAS 5 // Define 5 etapas

// Structs
typedef struct {
    float x, y;
    float vx, vy;
    int   active;
    int   slotIndex;
} Ball;

typedef struct {
    float x, y;
} Pin;


// --- NOVO (BLOCO A): Lógica de Perguntas e Estados ---

// 1. Define os estados do jogo
typedef enum {
    STATE_START_SCREEN,
    STATE_ASKING_QUESTION,
    STATE_WAITING_FOR_BALL,
    STATE_BALL_FALLING,
    STATE_BALL_LANDED,
    STATE_GAME_OVER
} GameState;

// 2. Define a estrutura da Pergunta
typedef struct {
    const char* texto;
    const char* opcoes[3];
    int resposta_correta; // 0, 1, ou 2
} Pergunta;

// 3. Banco de Perguntas (5 etapas)
Pergunta perguntas[NUM_ETAPAS] = {
    {
        "Qual a capital da Franca?",
        {"1. Londres", "2. Paris", "3. Berlim"},
        1 // Resposta é a opção 1 (índice 1)
    },
    {
        "Quem pintou a Mona Lisa?",
        {"1. Van Gogh", "2. Picasso", "3. Da Vinci"},
        2 // Resposta é a opção 2 (índice 2)
    },
    {
        "Quanto e 5 x 8?",
        {"1. 40", "2. 45", "3. 35"},
        0 // Resposta é a opção 0 (índice 0)
    },
    {
        "Qual o maior planeta do Sistema Solar?",
        {"1. Terra", "2. Marte", "3. Jupiter"},
        2 // Resposta é a opção 2 (índice 2)
    },
    {
        "Em que ano o homem pisou na Lua?",
        {"1. 1969", "2. 1975", "3. 1982"},
        0 // Resposta é a opção 0 (índice 0)
    }
};
// --- FIM DO BLOCO A ---


// --- BLOCO 1: Funções de "Matemática" (Fatorial, Combinações) ---
long long factorial(int n) {
    if (n < 0) return 0;
    if (n == 0) return 1;
    long long f = 1;
    for (int i = 2; i <= n; i++) f *= i;
    return f;
}

long long combinations(int n, int k) {
    if (k < 0 || k > n) return 0;
    long long denom = factorial(k) * factorial(n - k);
    if (denom == 0) return 0;
    return factorial(n) / denom;
}
// --- FIM DO BLOCO 1 ---


int main(void) {
    // Configurações da Janela
    const int screenWidth = 800;
    const int screenHeight = 1000;
    const int gameAreaHeight = 700; 

    InitWindow(screenWidth, screenHeight, "The Wall (Trabalho de Estatística)");
    srand((unsigned)time(NULL));
    SetTargetFPS(60);

    // ----- Cria Pinos -----
    Pin pins[NUM_PINS_X * NUM_PINS_Y];
    int pinCount = 0;
    float firstPinY = 100.0f;
    float totalPinsWidth = (NUM_PINS_X - 1) * PIN_SPACING + PIN_RADIUS * 2; 
    // Calcula o ponto inicial X para que o tabuleiro fique centralizado
    float initialPinX = (screenWidth - totalPinsWidth) / 2.0f;
    for (int y = 0; y < NUM_PINS_Y; y++) {
        for (int x = 0; x < NUM_PINS_X; x++) {
            float offset = (y % 2 == 0) ? 0 : PIN_SPACING / 2.0f;
            pins[pinCount].x = screenWidth / 2.0f - NUM_PINS_X * PIN_SPACING / 2 + offset + x * PIN_SPACING;
            pins[pinCount].y = firstPinY + y * PIN_SPACING;
            pinCount++;
        }
    }
    
    // ----- Cria Slots -----
    float slotWidth = PIN_SPACING;
    float baseY = 640; 
    float totalSlotsWidth = SLOT_COUNT * slotWidth; 
    // Calcula o ponto inicial X para que os slots fiquem centralizados
    float firstSlotX = (screenWidth - totalSlotsWidth) / 2.0f;
    
    // --- MUDANÇA: Define valores dos slots (apenas positivos) ---
    int slotValues[SLOT_COUNT]; // SLOT_COUNT é 12
    
    // Usando os 12 valores da sua lista, na ordem
    int valoresBase[SLOT_COUNT] = {1, 10, 1000, 100, 500, 100, 1000, 100, 500, 100, 1000, 10, 1}; 
    
    for (int i = 0; i < SLOT_COUNT; i++) {
        slotValues[i] = valoresBase[i];
    }
    // --- FIM DA MUDANÇA ---


    // --- BLOCO 2: Variáveis de Estatística e Jogo ---
    int slotCounts[SLOT_COUNT] = {0}; 
    int totalBolas = 0;
    Ball ball = {0};
    ball.active = 0;

    // --- NOVO (BLOCO B): Variáveis de Estado do Jogo ---
    GameState currentState = STATE_START_SCREEN;
    int currentStage = 0; // Etapa atual (de 0 a 4)
    long long totalScore = 0;
    int lastAnswerWasCorrect = 0; // 1 se acertou, 0 se errou
    int lastValue = 0; // Salva o valor da última bola
    
    Color slotColor = BLUE; // Começa AZUL
    // --- FIM DO BLOCO B ---

    // ----- Loop principal -----
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        

        // ##### 1. ATUALIZAÇÃO (LÓGICA DO JOGO) #####
        
        switch (currentState) {
            case STATE_START_SCREEN: {
                // Espera o jogador pressionar ENTER para começar
                if (IsKeyPressed(KEY_ENTER)) {
                    currentState = STATE_ASKING_QUESTION;
                }
            } break;
            case STATE_ASKING_QUESTION: {
                // Espera o jogador responder (teclas 1, 2, 3)
                int resposta = -1; // <--- Variável 'resposta' criada aqui
                if (IsKeyPressed(KEY_ONE))   resposta = 0;
                if (IsKeyPressed(KEY_TWO))   resposta = 1;
                if (IsKeyPressed(KEY_THREE)) resposta = 2;

                if (resposta != -1) {
                    // Verificou a resposta
                    if (resposta == perguntas[currentStage].resposta_correta) {
                        lastAnswerWasCorrect = 1;
                        slotColor = GREEN; // <--- MUDANÇA: fica VERDE
                    } else {
                        lastAnswerWasCorrect = 0;
                        slotColor = RED;   // <--- MUDANÇA: fica VERMELHO
                    }
                    currentState = STATE_WAITING_FOR_BALL;
                }
            } break;

            case STATE_WAITING_FOR_BALL: {
                // Espera o jogador pressionar ESPAÇO para soltar a bola
                if (IsKeyPressed(KEY_SPACE) && !ball.active) {
                    ball.x = screenWidth / 2.0f;
                    ball.y = 40;
                    ball.vx = 0;
                    ball.vy = 0;
                    ball.active = 1;
                    ball.slotIndex = -1;
                    currentState = STATE_BALL_FALLING;
                }
            } break;

            case STATE_BALL_FALLING: {
                // Física da bola
                if (ball.active) {
                    ball.vy += GRAVITY * dt;
                    ball.x += ball.vx * dt;
                    ball.y += ball.vy * dt;
                    for (int i = 0; i < pinCount; i++) {
                        float dx = ball.x - pins[i].x;
                        float dy = ball.y - pins[i].y;
                        float dist = sqrtf(dx*dx + dy*dy);
                        float minDist = BALL_RADIUS + PIN_RADIUS;
                        if (dist < minDist && dist > 0.0001f) {
                            float nx = dx / dist;
                            float ny = dy / dist;
                            float pen = minDist - dist;
                            ball.x += nx * pen;
                            ball.y += ny * pen;
                            float vDotN = ball.vx * nx + ball.vy * ny;
                            ball.vx = ball.vx - 2.0f * vDotN * nx;
                            ball.vy = ball.vy - 2.0f * vDotN * ny;
                            ball.vx *= 0.6f;
                            ball.vy *= 0.6f;
                            ball.vx += ((rand() % 200) - 100) / 100.0f * 40.0f;
                            break;
                        }
                    }
                    if (ball.x < BALL_RADIUS) { ball.x = BALL_RADIUS; ball.vx *= -0.6f; }
                    if (ball.x > screenWidth - BALL_RADIUS) { ball.x = screenWidth - BALL_RADIUS; ball.vx *= -0.6f; }

                    // Bola Aterrissou!
                    if (ball.y > baseY - BALL_RADIUS) {
                        ball.y = baseY - BALL_RADIUS;
                        int idx = (int)((ball.x - firstSlotX) / slotWidth);
                        if (idx < 0) idx = 0;
                        if (idx >= SLOT_COUNT) idx = SLOT_COUNT - 1;
                        
                        ball.slotIndex = idx;
                        ball.active = 0;

                        // ATUALIZA ESTATÍSTICA (BLOCO 3)
                        slotCounts[idx]++; 
                        totalBolas++;
                        
                        // --- NOVO (BLOCO C): ATUALIZA PONTUAÇÃO ---
                        lastValue = slotValues[ball.slotIndex];
                        if (lastAnswerWasCorrect) {
                            totalScore += lastValue; // Acertou: SOMA
                        } else {
                            totalScore -= lastValue; // Errou: SUBTRAI
                        }
                        // --- FIM DO BLOCO C ---
                        
                        currentState = STATE_BALL_LANDED;
                    }
                }
            } break;

            case STATE_BALL_LANDED: {
                // Espera o jogador pressionar ENTER para ir para a próxima etapa
                if (IsKeyPressed(KEY_ENTER)) {
                    currentStage++;
                    if (currentStage >= NUM_ETAPAS) {
                        currentState = STATE_GAME_OVER;
                    } else {
                        currentState = STATE_ASKING_QUESTION;
                        slotColor = BLUE; // <--- MUDANÇA: Reseta cor
                    }
                }
            } break;

            case STATE_GAME_OVER: {
                // Espera o jogador pressionar R para reiniciar
                if (IsKeyPressed(KEY_R)) {
                    // Reseta o jogo
                    totalScore = 0;
                    currentStage = 0;
                    currentState = STATE_ASKING_QUESTION;
                    slotColor = BLUE; // <--- MUDANÇA: Reseta cor
                    totalBolas = 0;
                    for (int i = 0; i < SLOT_COUNT; i++) slotCounts[i] = 0;
                }
            } break;
        }


        // ##### 2. DESENHO (GRÁFICOS) #####
        
        BeginDrawing();
        ClearBackground((Color){5,15,40,255});

        // --- Desenha o Jogo (Pinos, Slots, Fundo) ---
        for (int i = 0; i < pinCount; i++)
            DrawCircle((int)pins[i].x, (int)pins[i].y, PIN_RADIUS, RAYWHITE);
        
        DrawRectangle(0, baseY, screenWidth, gameAreaHeight - baseY, DARKGRAY);
        for (int i = 0; i < SLOT_COUNT; i++) {
            float x = firstSlotX + i * slotWidth;
            DrawLine(x, baseY, x, gameAreaHeight, RAYWHITE);
            char txt[16];
            sprintf(txt, "%d", slotValues[i]);
            
            // --- MUDANÇA (Usa a variável de cor 'slotColor') ---
            DrawText(txt, x + slotWidth/2 - 20, baseY + 15, 16, slotColor);
        }

        // --- Desenha a Bola (se estiver caindo) ---
        if (ball.active) {
            DrawCircle((int)ball.x, (int)ball.y, BALL_RADIUS, GOLD);
        }

        // --- Desenha a Área de Estatística (sempre visível) ---
        DrawRectangle(0, gameAreaHeight, screenWidth, screenHeight - gameAreaHeight, (Color){10, 20, 50, 255});
        DrawLine(0, gameAreaHeight, screenWidth, gameAreaHeight, RAYWHITE);
        DrawText("DISTRIBUIÇÃO DE PROBABILIDADE (EM TEMPO REAL)", 110, gameAreaHeight + 15, 20, YELLOW);
        
        // BLOCO 4: Gráficos (Empírico e Teórico)
        float graphBaseY = screenHeight - 30; 
        float graphAreaWidth = 600;           
        float graphStartX = (screenWidth - graphAreaWidth) / 2;
        float graphBarWidth = graphAreaWidth / SLOT_COUNT;
        int scaleFactor = 20; 
        // 4a. Empírico (AZUL)
        for (int i = 0; i < SLOT_COUNT; i++) {
            float x = graphStartX + i * graphBarWidth;
            int height = slotCounts[i] * scaleFactor;
            DrawRectangle(x + 2, graphBaseY - height, graphBarWidth - 4, height, BLUE);
            DrawText(TextFormat("%d", slotCounts[i]), x + graphBarWidth/2 - 5, graphBaseY - height - 15, 14, RAYWHITE);
        }
        // 4b. Teórico (VERMELHO)
        if (totalBolas > 0) {
            int n_rows = NUM_PINS_Y;
            for (int k = 0; k <= n_rows; k++) {
                int slotIndex = k + 1; 
                double prob = (double)combinations(n_rows, k) * pow(0.5, n_rows);
                int expectedHeight = (int)(prob * totalBolas * scaleFactor);
                float x = graphStartX + slotIndex * graphBarWidth;
                DrawRectangleLines(x + 2, graphBaseY - expectedHeight, graphBarWidth - 4, expectedHeight, RED);
            }
        }
        
        // BLOCO 5: Previsão Dinâmica (VERDE)
        if (ball.active) {
            float progress = (ball.y - firstPinY) / (baseY - firstPinY);
            if (progress < 0.0f) progress = 0.0f;
            if (progress > 1.0f) progress = 1.0f;
            int n_remaining = (int)((1.0f - progress) * NUM_PINS_Y);
            if (n_remaining < 1) n_remaining = 1;
            float predictedX = ball.x;
            int center_slot_idx = (int)((predictedX - firstSlotX) / slotWidth);
            int k_peak = n_remaining / 2;
            int slot_offset = center_slot_idx - k_peak;
            int graphHeight = (screenHeight - gameAreaHeight - 60);
            for (int k = 0; k <= n_remaining; k++) {
                double prob = (double)combinations(n_remaining, k) * pow(0.5, n_remaining);
                int probHeight = (int)(prob * (graphHeight * 2.0));
                int target_slot = k + slot_offset;
                if (target_slot >= 0 && target_slot < SLOT_COUNT) {
                    float x = graphStartX + target_slot * graphBarWidth;
                    DrawRectangle(x + 2, graphBaseY - probHeight, graphBarWidth - 4, probHeight, Fade(GREEN, 0.4f));
                }
            }
        }

        // --- NOVO (BLOCO D): Desenha a Interface do Jogo (UI) ---
        // Esta parte desenha o texto de acordo com o estado do jogo
        
        // Desenha a Pontuação Total e a Etapa (quase sempre visível)
        if (currentState != STATE_GAME_OVER && currentState != STATE_START_SCREEN) {
            DrawText(TextFormat("PONTUACAO: %lld", totalScore), 20, 20, 24, YELLOW);
            DrawText(TextFormat("ETAPA: %d / %d", currentStage + 1, NUM_ETAPAS), 620, 20, 24, YELLOW);
        }

        switch (currentState) {
            case STATE_START_SCREEN: {
                // Desenha uma camada escura para focar o texto
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));
                
                // Título
                DrawText("THE WALL", (screenWidth - MeasureText("THE WALL", 80)) / 2, 250, 80, GOLD);
                DrawText("Trabalho de Estatistica", (screenWidth - MeasureText("Trabalho de Estatistica", 30)) / 2, 340, 30, RAYWHITE);
                
                // Instrução
                DrawText("Pressione [ENTER] para comecar", (screenWidth - MeasureText("Pressione [ENTER] para comecar", 20)) / 2, 450, 20, YELLOW);
                
            } break;

            case STATE_ASKING_QUESTION: {
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));
                // Desenha a pergunta e as opções
                Pergunta q = perguntas[currentStage];
                DrawText(q.texto, (screenWidth - MeasureText(q.texto, 20)) / 2, 60, 20, WHITE);
                DrawText(q.opcoes[0], (screenWidth - MeasureText(q.opcoes[0], 20)) / 2, 90, 20, RAYWHITE);
                DrawText(q.opcoes[1], (screenWidth - MeasureText(q.opcoes[1], 20)) / 2, 120, 20, RAYWHITE);
                DrawText(q.opcoes[2], (screenWidth - MeasureText(q.opcoes[2], 20)) / 2, 150, 20, RAYWHITE);
                DrawText("Pressione [1], [2] ou [3] para responder", 210, 190, 20, LIME);
            } break;

            case STATE_WAITING_FOR_BALL: {
                // Mostra se o jogador acertou ou errou
                if (lastAnswerWasCorrect) {
                    DrawText("RESPOSTA CORRETA!", 280, 80, 24, GREEN);
                    DrawText("O valor da bola sera SOMADO a sua pontuacao.", 150, 110, 20, WHITE);
                } else {
                    DrawText("RESPOSTA ERRADA!", 290, 80, 24, RED);
                    DrawText("O valor da bola sera SUBTRAIDO da sua pontuacao.", 140, 110, 20, WHITE);
                }
                DrawText("Pressione [ESPACO] para soltar a bola", 200, 160, 20, YELLOW);
            } break;

            case STATE_BALL_FALLING: {
                // Não desenha nada extra
            } break;

            case STATE_BALL_LANDED: {
                // Mostra o resultado da jogada
                char* resultadoTexto;
                Color resultadoCor;
                
                if (lastAnswerWasCorrect) {
                    resultadoTexto = TextFormat("Voce GANHOU %d pontos!", lastValue);
                    resultadoCor = GREEN;
                } else {
                    resultadoTexto = TextFormat("Voce PERDEU %d pontos!", lastValue);
                    resultadoCor = RED;
                }
                DrawText(TextFormat("A bola caiu no valor: %d", lastValue), 220, 80, 20, RAYWHITE);
                DrawText(resultadoTexto, (screenWidth - MeasureText(resultadoTexto, 24)) / 2, 110, 24, resultadoCor);
                DrawText("Pressione [ENTER] para a proxima etapa...", 170, 160, 20, YELLOW);
            } break;

            case STATE_GAME_OVER: {
                // Tela de Fim de Jogo
                DrawText("FIM DE JOGO!", 280, 100, 40, GOLD);
                DrawText(TextFormat("PONTUACAO FINAL: %lld", totalScore), (screenWidth - MeasureText(TextFormat("PONTUACAO FINAL: %lld", totalScore), 30)) / 2, 160, 30, YELLOW);
                DrawText("Pressione [R] para reiniciar o jogo", 210, 220, 20, RAYWHITE);
            } break;
        }
        // --- FIM DO BLOCO D ---

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
