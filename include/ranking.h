#ifndef RANKING_H
#define RANKING_H

// Aumentamos para 50 para justificar a paginação
#define MAX_HIGH_SCORES 50 
#define SCORES_PER_PAGE 10
#define MAX_NAME_LENGTH 15

typedef struct {
    char name[MAX_NAME_LENGTH + 1];
    long long score;
} HighScore;

void InitRanking(void);
void UpdateRanking(void);
void DrawRanking(void);
void AddHighScore(const char* playerName, long long score);

#endif