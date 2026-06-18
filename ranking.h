#ifndef RANKING_H
#define RANKING_H

// Estrutura para salvar no arquivo binário
typedef struct {
    char nome[50];
    int pontos;
} RegistroJogador;

// Estrutura da lista encadeada dinâmica
typedef struct NoJogador {
    char nome[50];
    int pontos;
    struct NoJogador* proximo;
} NoJogador;

// Declaração das funções
void inserir_ordenado(NoJogador** topo, const char* nome, int pontos);
NoJogador* carregar_placar_lista();
void salvar_placar_lista(NoJogador* topo);
void liberar_lista(NoJogador* topo);

#endif