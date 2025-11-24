#ifndef QUIZ_H
#define QUIZ_H

#include "raylib.h"
#include "commons.h"
#include "game_common.h"

#define NUM_OPCOES 3

typedef struct {
    const char* texto;
    const char* opcoes[NUM_OPCOES];
    int resposta_correta;
    Color corTema;
} Pergunta;

void InitQuiz(void);
int UpdateQuiz(void);
void DrawQuiz(int stage);
Color GetQuizThemeColor(int stage);
bool QuizHasNext(int stage);
const Pergunta* GetQuestion(int stage);

#endif