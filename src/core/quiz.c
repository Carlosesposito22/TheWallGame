#include "quiz.h"
#include "effects.h"
#include "particles.h"
#include "commons.h"
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h> // Necessário para a função rand()
#include <time.h>   // Necessário para inicializar a semente de rand()

#define MAX_PERGUNTAS 30

static Pergunta all_perguntas[MAX_PERGUNTAS] = {
    // 0. História Antiga
    { "Das Sete Maravilhas do Mundo Antigo, qual é a única estrutura que ainda existe hoje?",
      {"1. Templo de Ártemis", "2. Farol de Alexandria", "3. Grande Pirâmide de Gizé"}, 2, COLOR_NEON_BLUE },
      
    // 1. Astronomia
    { "Qual planeta do nosso Sistema Solar é famoso por possuir a\n'Grande Mancha Vermelha', uma tempestade gigante\nque é maior do que a Terra?",
      {"1. Vênus", "2. Júpiter", "3. Saturno"}, 1, COLOR_NEON_GOLD },
      
    // 2. Biologia
    { "O morcego é o único mamífero que consegue voar ativamente.\nQual das seguintes categorias ele não pertence?",
      {"1. Um mamífero placentário", "2. Um primata", "3. Um animal noturno"}, 1, COLOR_NEON_GREEN },
      
    // 3. Geografia
    { "Qual país insular tem uma população de ovelhas\nque supera a de humanos em mais de 5 vezes?",
      {"1. Islândia", "2. Nova Zelândia", "3. Austrália"}, 1, COLOR_NEON_PURPLE },
      
    // 4. Tecnologia
    { "Qual é o nome da primeira criptomoeda descentralizada,\ncriada em 2009 sob o pseudônimo de Satoshi Nakamoto?",
      {"1. Ethereum", "2. Litecoin", "3. Bitcoin"}, 2, COLOR_NEON_RED },
      
    // 5. Química
    { "Qual elemento químico tem o símbolo 'Fe' na Tabela Periódica?",
      {"1. Flúor", "2. Ferro", "3. Fósforo"}, 1, (Color){150, 150, 255, 255} }, // Azul claro
      
    // 6. Artes
    { "Quem pintou a obra 'Mona Lisa'?",
      {"1. Vincent van Gogh", "2. Claude Monet", "3. Leonardo da Vinci"}, 2, (Color){255, 100, 100, 255} }, // Vermelho claro
      
    // 7. Geografia
    { "Qual é a capital do Canadá?",
      {"1. Toronto", "2. Vancouver", "3. Ottawa"}, 2, (Color){100, 255, 100, 255} }, // Verde claro
      
    // 8. Biologia
    { "Qual é a unidade básica da hereditariedade?",
      {"1. Proteína", "2. Gene", "3. Célula"}, 1, (Color){255, 200, 150, 255} }, // Laranja
      
    // 9. Matemática
    { "Quantos lados tem um heptágono?",
      {"1. Seis", "2. Sete", "3. Oito"}, 1, (Color){255, 150, 255, 255} }, // Roxo claro

    // 10. História
    { "Em que ano o homem pisou na Lua pela primeira vez?",
      {"1. 1965", "2. 1969", "3. 1971"}, 1, COLOR_NEON_GOLD },

    // 11. Geografia
    { "Qual deserto é o maior do mundo em termos de área?",
      {"1. Deserto do Saara", "2. Deserto da Antártida", "3. Deserto de Gobi"}, 1, COLOR_NEON_BLUE },

    // 12. Química
    { "Qual gás é responsável pela cor azul do céu?",
      {"1. Oxigênio", "2. Nitrogênio", "3. Dióxido de Carbono"}, 1, COLOR_NEON_GREEN },

    // 13. Literatura
    { "Qual autor escreveu a obra 'Dom Quixote'?",
      {"1. Gabriel García Márquez", "2. Miguel de Cervantes", "3. William Shakespeare"}, 1, COLOR_NEON_PURPLE },

    // 14. Biologia
    { "Qual o nome do processo pelo qual as plantas produzem seu próprio alimento?",
      {"1. Respiração", "2. Fotossíntese", "3. Transpiração"}, 1, COLOR_NEON_RED },

    // 15. Tecnologia
    { "Qual linguagem de programação foi criada por Guido van Rossum?",
      {"1. Java", "2. C++", "3. Python"}, 2, (Color){255, 165, 0, 255} }, // Laranja Vivo

    // 16. Artes
    { "Qual movimento artístico surgiu na França no final\ndo século XIX, caracterizado por pinceladas soltas?",
      {"1. Cubismo", "2. Impressionismo", "3. Surrealismo"}, 1, (Color){173, 216, 230, 255} }, // Azul Claro (light blue)

    // 17. Esportes
    { "Quantos jogadores um time de futebol de campo tem em jogo?",
      {"1. Dez", "2. Onze", "3. Doze"}, 1, (Color){50, 205, 50, 255} }, // Verde Limão

    // 18. Música
    { "Qual instrumento musical é geralmente considerado o 'Rei dos Instrumentos'?",
      {"1. Piano", "2. Violino", "3. Órgão de tubos"}, 2, (Color){255, 255, 100, 255} }, // Amarelo Claro

    // 19. História Antiga
    { "Qual imperador romano fez do Cristianismo a religião oficial do Império?",
      {"1. Augusto", "2. Constantino", "3. Nero"}, 1, (Color){139, 0, 0, 255} }, // Vermelho Escuro

    // 20. Astronomia
    { "O que é um 'buraco negro'?",
      {"1. Uma estrela em colapso", "2. Uma galáxia escura", "3. Uma região do espaço-tempo com gravidade extrema"}, 2, (Color){75, 0, 130, 255} }, // Índigo

    // 21. Geografia
    { "Qual é o rio mais longo da América do Sul?",
      {"1. Rio Paraná", "2. Rio Orinoco", "3. Rio Amazonas"}, 2, (Color){0, 191, 255, 255} }, // Azul Profundo

    // 22. Linguagem
    { "Quantas letras há no alfabeto português?",
      {"1. 24", "2. 26", "3. 27"}, 1, (Color){255, 223, 0, 255} }, // Dourado

    // 23. Cinema
    { "Qual filme ganhou o primeiro Oscar de 'Melhor Filme' em 1929?",
      {"1. O Cantor de Jazz", "2. Asas (Wings)", "3. Ben-Hur"}, 1, (Color){192, 192, 192, 255} }, // Prata

    // 24. Mitologia
    { "Na mitologia grega, quem era o deus do mar?",
      {"1. Zeus", "2. Hades", "3. Poseidon"}, 2, COLOR_NEON_BLUE },

    // 25. Biologia
    { "Qual o maior órgão do corpo humano?",
      {"1. Coração", "2. Fígado", "3. Pele"}, 2, COLOR_NEON_GREEN },

    // 26. Química
    { "Qual o único metal líquido em temperatura ambiente?",
      {"1. Alumínio", "2. Mercúrio", "3. Bromo"}, 1, COLOR_NEON_RED },

    // 27. Culinária
    { "De onde é originário o prato 'sushi'?",
      {"1. Coreia", "2. China", "3. Japão"}, 2, COLOR_NEON_GOLD },

    // 28. Física
    { "Qual é a velocidade da luz no vácuo (aproximadamente)?",
      {"1. 30.000 km/s", "2. 300.000 km/s", "3. 3.000.000 km/s"}, 1, COLOR_NEON_PURPLE },

    // 29. Lógica/Matemática
    { "Um número ímpar menos um número par resulta em um número...",
      {"1. Par", "2. Ímpar", "3. Primo"}, 1, (Color){255, 105, 180, 255} } // Rosa Choque
};



static Pergunta perguntas_da_partida[NUM_ETAPAS];

void swap(Pergunta *a, Pergunta *b) {
    Pergunta temp = *a;
    *a = *b;
    *b = temp;
}

void InitQuiz(void) {
    srand((unsigned int)time(NULL));

    Pergunta temp_perguntas[MAX_PERGUNTAS];
    for (int i = 0; i < MAX_PERGUNTAS; i++) {
        temp_perguntas[i] = all_perguntas[i];
    }

    for (int i = MAX_PERGUNTAS - 1; i > 0; i--) {
        int j = rand() % (i + 1); 
        swap(&temp_perguntas[i], &temp_perguntas[j]);
    }

    for (int i = 0; i < NUM_ETAPAS; i++) {
        perguntas_da_partida[i] = temp_perguntas[i];
    }
}

int UpdateQuiz(void) {
    int resposta = -1;
    if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) resposta = 0;
    if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) resposta = 1;
    if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) resposta = 2;

    if (resposta == -1) return -1;

    extern int currentStage;
    
    if (resposta == perguntas_da_partida[currentStage].resposta_correta)
        return 1;
    else
        return 0;
}

void DrawQuiz(int stage) {
    if (stage < 0 || stage >= NUM_ETAPAS) return;

    Color bgColor = perguntas_da_partida[stage].corTema; 
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(bgColor, 0.15f));
    DrawRectangle(SCREEN_WIDTH * 0.1f, SCREEN_HEIGHT * 0.2f, 
                  SCREEN_WIDTH * 0.8f, SCREEN_HEIGHT * 0.5f, COLOR_UI_BG);
    DrawRectangleLines(SCREEN_WIDTH * 0.1f, SCREEN_HEIGHT * 0.2f, 
                      SCREEN_WIDTH * 0.8f, SCREEN_HEIGHT * 0.5f, bgColor);

    Pergunta q = perguntas_da_partida[stage];
    
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
    return perguntas_da_partida[stage].corTema;
}

const Pergunta* GetQuestion(int stage) {
    if (stage < 0 || stage >= NUM_ETAPAS) return NULL;
    return &perguntas_da_partida[stage];
}

bool QuizHasNext(int stage) {
    return stage < NUM_ETAPAS - 1;
}