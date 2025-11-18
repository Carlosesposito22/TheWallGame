#ifndef RANKING_H
#define RANKING_H

// Define quantos scores vamos guardar
#define MAX_HIGH_SCORES 5 
#define MAX_NAME_LENGTH 15

typedef struct {
    char name[MAX_NAME_LENGTH + 1];
    long long score;
} HighScore;

// Funções de ciclo de vida da tela
void InitRanking(void);
void UpdateRanking(void);
void DrawRanking(void);

// Função para salvar um novo score (chamada pelo game.c)
void AddHighScore(const char* playerName, long long score);

#endif