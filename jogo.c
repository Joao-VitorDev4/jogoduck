#include "jogo.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <allegro5/allegro_primitives.h>

const int LARGURA_BASE = 800;
const int ALTURA_BASE = 600;
const int PATO_LARGURA = 70;
const int PATO_ALTURA = 70;
const int ARMA_LARGURA = 200;
const int ARMA_ALTURA = 150;

float multiplicador_velocidade = 1.0f;
int vidas = 5; 
float volume_atual = 0.5f;   
bool mutado = false;
bool arrastando_slider = false;

Pato patos[5];

void resetar_pato(Pato* p, int index) {
    p->x = 110; 
    p->vel = (2 + (rand() % 3));             
    p->vivo = true;
    p->atingido = false; 
    p->angulo = 0.0;     

    int fileiras_possiveis[3] = {175, 280, 405};
    bool fileira_ocupada[3] = {false, false, false};

    for (int i = 0; i < 5; i++) {
        if (i != index && patos[i].vivo && patos[i].tempo_renascer >= 0) {
            if ((int)patos[i].y == 175) fileira_ocupada[0] = true;
            if ((int)patos[i].y == 280) fileira_ocupada[1] = true;
            if ((int)patos[i].y == 405) fileira_ocupada[2] = true;
        }
    }

    int livres[3];
    int qtd_livres = 0;
    for (int i = 0; i < 3; i++) {
        if (!fileira_ocupada[i]) {
            livres[qtd_livres] = fileiras_possiveis[i];
            qtd_livres++;
        }
    }

    if (qtd_livres > 0) {
        p->y = livres[rand() % qtd_livres];
    } else {
        int r = rand() % 3;
        if (r == 0) p->y = 175;
        else if (r == 1) p->y = 280;
        else p->y = 405;
    }

    if (index == 3) {
        p->tipo = 1; 
        p->tempo_renascer = -(180 + (rand() % 240)); 
        p->vel += 1.5; 
    } 
    else if (index == 4) {
        p->tipo = 2; 
        p->tempo_renascer = -(240 + (rand() % 300)); 
        p->vel += 1.0; 
    } 
    else {
        p->tipo = 0; 
        p->tempo_renascer = -(rand() % 120); 
    }
}

void atualizar(Pato* p, int index, int* pontos) {
    if (p->tempo_renascer < 0) {
        p->tempo_renascer++;
        return; 
    }

    if (p->vivo) {
        p->x += p->vel * multiplicador_velocidade;
        if (p->x > 620) {
            resetar_pato(p, index);
        }
    } 
    else if (p->atingido) {
        p->angulo -= 0.08; 
        p->y += 1.5;       

        if (p->angulo <= -1.57) {
            p->atingido = false;
            p->tempo_renascer = 0; 
        }
    } 
    else {
        p->tempo_renascer++;
        int limite_respawn = (p->tipo != 0) ? 120 : 60;
        if (p->tempo_renascer > limite_respawn) {
            resetar_pato(p, index);
        }
    }
}

// Renderização dos corações milimetricamente ajustados ao seu HUD cinza
void desenhar_coracao(int x, int y, int tamanho) {
    tamanho = 16;
    float raio = tamanho / 4.0f;
    ALLEGRO_COLOR cor = al_map_rgb(230, 30, 40);
    
    // Coordenada Y travada centralizadamente na área de HUD superior demarcada
    float centro_y = 31.0f;

    al_draw_filled_circle((float)x - raio, centro_y - (raio / 2.0f), raio, cor);
    al_draw_filled_circle((float)x + raio, centro_y - (raio / 2.0f), raio, cor);
    al_draw_filled_triangle((float)x - tamanho / 2.0f, centro_y - (raio / 2.0f), 
                            (float)x + tamanho / 2.0f, centro_y - (raio / 2.0f), 
                            (float)x, centro_y + (tamanho / 2.0f), cor);
}

void criar_texto_flutuante(TextoFlutuante* lista, float x, float y, const char* texto, ALLEGRO_COLOR cor) {
    for (int i = 0; i < MAX_TEXTOS_FLUTUANTES; i++) {
        if (lista[i].vida <= 0) { 
            lista[i].x = x;
            lista[i].y = y - 15; 
            strcpy(lista[i].texto, texto);
            lista[i].cor = cor;
            lista[i].vida = 50; 
            break;
        }
    }
}