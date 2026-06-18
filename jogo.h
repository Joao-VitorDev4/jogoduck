#ifndef JOGO_H
#define JOGO_H

#include <allegro5/allegro.h>
#include <stdbool.h>

// Definições de constantes do jogo
extern const int LARGURA_BASE;
extern const int ALTURA_BASE;
extern const int PATO_LARGURA;
extern const int PATO_ALTURA;
extern const int ARMA_LARGURA;
extern const int ARMA_ALTURA;

#define MAX_FAISCAS 30
#define MAX_TEXTOS_FLUTUANTES 10

typedef struct {
    float x;
    float y;
    float vel;             
    bool vivo;
    bool atingido; 
    float angulo;     
    int tempo_renascer;
    int tipo; 
} Pato;

typedef struct {
    float x, y;
    float vel_x, vel_y;
    int vida;
    int cor_g;
} Faisca;

typedef struct {
    float x, y;
    char texto[30];
    ALLEGRO_COLOR cor;
    int vida;
} TextoFlutuante;

// Variáveis globais compartilhadas
extern float multiplicador_velocidade;
extern int vidas; 
extern float volume_atual;   
extern bool mutado;
extern bool arrastando_slider;
extern Pato patos[5];

// Protótipos das funções
void resetar_pato(Pato* p, int index);
void atualizar(Pato* p, int index, int* pontos);
void desenhar_coracao(int x, int y, int tamanho); 
void criar_texto_flutuante(TextoFlutuante* lista, float x, float y, const char* texto, ALLEGRO_COLOR cor);

#endif