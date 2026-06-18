#include "ranking.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void inserir_ordenado(NoJogador** topo, const char* nome, int pontos) {
    NoJogador* novo = (NoJogador*)malloc(sizeof(NoJogador));
    if (novo == NULL) return;
    
    strcpy(novo->nome, nome);
    novo->pontos = pontos;
    novo->proximo = NULL;

    if (*topo == NULL || pontos > (*topo)->pontos) {
        novo->proximo = *topo;
        *topo = novo;
    } 
    else {
        NoJogador* atual = *topo;
       
        while (atual->proximo != NULL && atual->proximo->pontos >= pontos) {
            atual = atual->proximo;
        }
      
        novo->proximo = atual->proximo;
        atual->proximo = novo;
    }
}
NoJogador* carregar_placar_lista() {
    NoJogador* topo = NULL;
    RegistroJogador temp;
    FILE* arquivo = fopen("placar.bin", "rb");
    
    if (arquivo != NULL) {
        while (fread(&temp, sizeof(RegistroJogador), 1, arquivo) == 1) {
            inserir_ordenado(&topo, temp.nome, temp.pontos);
        }
        fclose(arquivo);
    }
    return topo;
}

void salvar_placar_lista(NoJogador* topo) {
    FILE* arquivo = fopen("placar.bin", "wb");
    if (arquivo != NULL) {
        NoJogador* atual = topo;
        RegistroJogador temp;
        while (atual != NULL) {
            strcpy(temp.nome, atual->nome);
            temp.pontos = atual->pontos;
            fwrite(&temp, sizeof(RegistroJogador), 1, arquivo);
            atual = atual->proximo;
        }
        fclose(arquivo);
    }
}

void liberar_lista(NoJogador* topo) {
    NoJogador* atual = topo;
    while (atual != NULL) {
        NoJogador* aux = atual->proximo;
        free(atual);
        atual = aux;
    }
}