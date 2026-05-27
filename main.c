#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h> 
#include <time.h> 
#include <math.h> // Necessário para a constante M_PI da rotação

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h> 
#include <allegro5/allegro_font.h> 

// Dimensões base para referência lógica de proporção (800x600)
const int LARGURA_BASE = 800;
const int ALTURA_BASE = 600;

// Dimensoes dos Elementos
const int PATO_LARGURA = 70;
const int PATO_ALTURA = 70;
const int ARMA_LARGURA = 200;
const int ARMA_ALTURA = 150;

#define MAX_FAISCAS 15
#define MAX_JOGADORES 100 

typedef struct {
    float x, y;
    float vel_x, vel_y;
    int vida; 
    int cor_g;
} Faisca;

typedef struct {
    float x, y;
    float vel;
    bool vivo;
    bool atingido;      // NOVO: Controla se o pato está na animação de queda
    float angulo;       // NOVO: Guarda o ângulo atual do tombamento
    int tempo_renascer;
} Pato;

typedef struct {
    char nome[50];
    int pontos;
} RegistroJogador;

void resetar_pato(Pato* p, int index) {
    p->x = 110; 
    p->vel = 2 + (rand() % 3);             
    p->vivo = true;
    p->atingido = false; // NOVO: Reseta o estado de queda
    p->angulo = 0.0;     // NOVO: Reseta o ângulo
    p->tempo_renascer = -(rand() % 120); 

    if (index == 0) {
        p->y = 175; 
    } else if (index == 1) {
        p->y = 280; 
    } else {
        p->y = 405; 
    }
}

void atualizar(Pato* p, int index) {
    if (p->tempo_renascer < 0) {
        p->tempo_renascer++;
        return; 
    }

    if (p->vivo) {
        p->x += p->vel;
        if (p->x > 620) {
            resetar_pato(p, index);
        }
    } 
    // NOVO: Lógica da animação de tombamento caso tenha sido atingido
    else if (p->atingido) {
        // Faz o pato girar para trás (sentido anti-horário se voa para a direita)
        p->angulo -= 0.08; 
        // Faz ele cair levemente enquanto gira
        p->y += 1.5;       

        // Quando o giro passar de ~90 graus (1.57 radianos), termina a queda e inicia a contagem para renascer
        if (p->angulo <= -1.57) {
            p->atingido = false;
            p->tempo_renascer = 0; 
        }
    } 
    else {
        p->tempo_renascer++;
        if (p->tempo_renascer > 60) {
            resetar_pato(p, index);
        }
    }
}

int carregar_placar(RegistroJogador* lista) {
    int qtd = 0;
    FILE* arquivo = fopen("placar.txt", "r");
    if (arquivo != NULL) {
        while (fscanf(arquivo, "Jogador: %49s | Pontos: %d\n", lista[qtd].nome, &lista[qtd].pontos) == 2) {
            qtd++;
            if (qtd >= MAX_JOGADORES - 1) break;
        }
        fclose(arquivo);
    }
    
    for (int i = 0; i < qtd - 1; i++) {
        for (int j = 0; j < qtd - i - 1; j++) {
            if (lista[j].pontos < lista[j + 1].pontos) {
                RegistroJogador temp = lista[j];
                lista[j] = lista[j + 1];
                lista[j + 1] = temp;
            }
        }
    }
    return qtd;
}

int main() {
    srand(time(NULL));

    char nome_jogador[50] = ""; 
    int pontuacao = 0;
    
    int tempo_restante = 60; 
    int frames_contador = 0;  
    
    int estado_atual = 0; 

    ////// INICIALIZACAO ALLEGRO ////// 
    if (!al_init()) return -1;
    if (!al_init_primitives_addon()) return -1; 
    if (!al_install_mouse()) return -1;
    if (!al_install_keyboard()) return -1; 
    if (!al_init_image_addon()) return -1;
    
    al_init_font_addon(); 

    ALLEGRO_MONITOR_INFO info;
    al_get_monitor_info(0, &info);
    int largura_tela = info.x2 - info.x1;
    int altura_tela = info.y2 - info.y1;

    al_set_new_display_flags(ALLEGRO_FULLSCREEN_WINDOW);
    ALLEGRO_DISPLAY* display = al_create_display(largura_tela, altura_tela);
    
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60);
    ALLEGRO_EVENT_QUEUE* fila = al_create_event_queue();

    if (!display || !timer || !fila) return -1;

    al_register_event_source(fila, al_get_display_event_source(display));
    al_register_event_source(fila, al_get_timer_event_source(timer));
    al_register_event_source(fila, al_get_mouse_event_source());
    al_register_event_source(fila, al_get_keyboard_event_source()); 

    ALLEGRO_FONT* fonte = al_create_builtin_font();

    ////// IMAGENS ////// 
    ALLEGRO_BITMAP* fundo = al_load_bitmap("fundo.jpg");
    ALLEGRO_BITMAP* pato_img = al_load_bitmap("pato.jpg");
    ALLEGRO_BITMAP* arma = al_load_bitmap("arma.jpg");

    if (!fundo || !pato_img || !arma) {
        printf("Erro ao carregar imagens.\n");
        return -1;
    }
    
    Pato patos[3];
    for (int i = 0; i < 3; i++) {
        resetar_pato(&patos[i], i);
    }

    Faisca faiscas[MAX_FAISCAS];
    for(int i = 0; i < MAX_FAISCAS; i++) {
        faiscas[i].vida = 0; 
    }

    float mx = 0, my = 0;
    int frame_tiro = 0; 
    bool atirou = false;

    float impacto_x = 0;
    float impacto_y = 0;

    RegistroJogador lista_jogadores[MAX_JOGADORES];
    int qtd_jogadores = 0;

    int btn_jogar_x1 = LARGURA_BASE / 2 - 100, btn_jogar_y1 = 230;
    int btn_jogar_x2 = LARGURA_BASE / 2 + 100, btn_jogar_y2 = 280;
    
    int btn_pontos_x1 = LARGURA_BASE / 2 - 100, btn_pontos_y1 = 300;
    int btn_pontos_x2 = LARGURA_BASE / 2 + 100, btn_pontos_y2 = 350;
    
    int btn_sair_x1 = LARGURA_BASE / 2 - 100, btn_sair_y1 = 370;
    int btn_sair_x2 = LARGURA_BASE / 2 + 100, btn_sair_y2 = 420;

    int btn_reiniciar_x1 = LARGURA_BASE / 2 - 140, btn_reiniciar_y1 = 430;
    int btn_reiniciar_x2 = LARGURA_BASE / 2 - 10,  btn_reiniciar_y2 = 480;
    int btn_fim_sair_x1  = LARGURA_BASE / 2 + 10,  btn_fim_sair_y1  = 430;
    int btn_fim_sair_x2  = LARGURA_BASE / 2 + 140, btn_fim_sair_y2  = 480;

    int btn_x_fechar_x1 = LARGURA_BASE - 50;
    int btn_x_fechar_y1 = 20;
    int btn_x_fechar_x2 = LARGURA_BASE - 20;
    int btn_x_fechar_y2 = 50;

    int btn_voltar_x1 = LARGURA_BASE / 2 - 80, btn_voltar_y1 = 450;
    int btn_voltar_x2 = LARGURA_BASE / 2 + 80, btn_voltar_y2 = 495;

    al_start_timer(timer);

    ////// LOOP PRINCIPAL ////// 
    while (true) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(fila, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            break;

        if (ev.type == ALLEGRO_EVENT_MOUSE_AXES) {
            mx = ev.mouse.x * ((float)LARGURA_BASE / largura_tela);
            my = ev.mouse.y * ((float)ALTURA_BASE / altura_tela);
        }

        if (estado_atual == 1 && ev.type == ALLEGRO_EVENT_KEY_CHAR) {
            int unichar = ev.keyboard.unichar;
            int len = strlen(nome_jogador);

            if (unichar >= 32 && unichar <= 126 && len < 15) {
                nome_jogador[len] = (char)unichar;
                nome_jogador[len + 1] = '\0';
            }
            else if (ev.keyboard.keycode == ALLEGRO_KEY_BACKSPACE && len > 0) {
                nome_jogador[len - 1] = '\0';
            }
            else if (ev.keyboard.keycode == ALLEGRO_KEY_ENTER && len > 0) {
                estado_atual = 2; 
            }
        }

        if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            if (ev.mouse.button & 1) { 
                
                if ((estado_atual == 1 || estado_atual == 2) && 
                    mx >= btn_x_fechar_x1 && mx <= btn_x_fechar_x2 && my >= btn_x_fechar_y1 && my <= btn_x_fechar_y2) {
                    
                    qtd_jogadores = carregar_placar(lista_jogadores); 
                    estado_atual = 4; 
                    continue; 
                }

                if (estado_atual == 0) {
                    if (mx >= btn_jogar_x1 && mx <= btn_jogar_x2 && my >= btn_jogar_y1 && my <= btn_jogar_y2) {
                        estado_atual = 1; 
                    }
                    else if (mx >= btn_pontos_x1 && mx <= btn_pontos_x2 && my >= btn_pontos_y1 && my <= btn_pontos_y2) {
                        qtd_jogadores = carregar_placar(lista_jogadores); 
                        estado_atual = 4; 
                    }
                    else if (mx >= btn_sair_x1 && mx <= btn_sair_x2 && my >= btn_sair_y1 && my <= btn_sair_y2) {
                        break; 
                    }
                }
                else if (estado_atual == 2) {
                    if (!(mx >= btn_x_fechar_x1 && mx <= btn_x_fechar_x2 && my >= btn_x_fechar_y1 && my <= btn_x_fechar_y2)) {
                        atirou = true;
                        frame_tiro = 6; 
                        impacto_x = mx;
                        impacto_y = my;

                        float cano_x = mx;
                        float cano_y = (ALTURA_BASE - ARMA_ALTURA) - 25 + 20; 

                        for (int i = 0; i < MAX_FAISCAS; i++) {
                            faiscas[i].x = cano_x;
                            faiscas[i].y = cano_y;
                            faiscas[i].vel_x = ((rand() % 100) - 50) / 10.0; 
                            faiscas[i].vel_y = -((rand() % 50) + 30) / 10.0; 
                            faiscas[i].vida = 10 + (rand() % 15); 
                            faiscas[i].cor_g = 100 + (rand() % 155);
                        }

                        bool acertou_algum = false;
                        for (int i = 0; i < 3; i++) {
                            // MODIFICADO: Só pode acertar o pato se ele estiver vivo E não estiver caindo
                            if (patos[i].vivo && !patos[i].atingido && patos[i].tempo_renascer >= 0 &&
                                mx >= patos[i].x && mx <= patos[i].x + PATO_LARGURA &&
                                my >= patos[i].y && my <= patos[i].y + PATO_ALTURA) {

                                patos[i].vivo = false;
                                patos[i].atingido = true; // Ativa a animação de tombar
                                patos[i].angulo = 0.0;     // Garante o início do ângulo estável
                                acertou_algum = true;
                            }
                        }

                        if (acertou_algum) {
                            pontuacao += 10; 
                        } else {
                            pontuacao -= 5;  
                            if (pontuacao < 0) pontuacao = 0; 
                        }
                    }
                }
                else if (estado_atual == 3) {
                    if (mx >= btn_reiniciar_x1 && mx <= btn_reiniciar_x2 && my >= btn_reiniciar_y1 && my <= btn_reiniciar_y2) {
                        pontuacao = 0;
                        tempo_restante = 60;
                        frames_contador = 0;
                        atirou = false;
                        strcpy(nome_jogador, ""); 
                        
                        for (int i = 0; i < 3; i++) {
                            resetar_pato(&patos[i], i);
                        }
                        for(int i = 0; i < MAX_FAISCAS; i++) {
                            faiscas[i].vida = 0; 
                        }
                        estado_atual = 1; 
                    }
                    else if (mx >= btn_fim_sair_x1 && mx <= btn_fim_sair_x2 && my >= btn_fim_sair_y1 && my <= btn_fim_sair_y2) {
                        break; 
                    }
                }
                else if (estado_atual == 4) {
                    if (mx >= btn_voltar_x1 && mx <= btn_voltar_x2 && my >= btn_voltar_y1 && my <= btn_voltar_y2) {
                        estado_atual = 0; 
                    }
                }
            }
        }

        if (ev.type == ALLEGRO_EVENT_TIMER) {

            if (estado_atual == 2) {
                frames_contador++;
                if (frames_contador >= 60) {
                    if (tempo_restante > 0) {
                        tempo_restante--;
                    }
                    frames_contador = 0;
                }

                if (tempo_restante <= 0) {
                    estado_atual = 3; 

                    qtd_jogadores = 0;
                    FILE* arquivo_leitura = fopen("placar.txt", "r");
                    if (arquivo_leitura != NULL) {
                        while (fscanf(arquivo_leitura, "Jogador: %49s | Pontos: %d\n", 
                               lista_jogadores[qtd_jogadores].nome, &lista_jogadores[qtd_jogadores].pontos) == 2) {
                            qtd_jogadores++;
                            if (qtd_jogadores >= MAX_JOGADORES - 1) break;
                        }
                        fclose(arquivo_leitura);
                    }

                    int indice_encontrado = -1;
                    for (int i = 0; i < qtd_jogadores; i++) {
                        if (strcmp(lista_jogadores[i].nome, nome_jogador) == 0) {
                            indice_encontrado = i;
                            break; 
                        }
                    }

                    if (indice_encontrado != -1) {
                        if(pontuacao > lista_jogadores[indice_encontrado].pontos)
                            lista_jogadores[indice_encontrado].pontos = pontuacao; 
                    } else {
                        strcpy(lista_jogadores[qtd_jogadores].nome, nome_jogador);
                        lista_jogadores[qtd_jogadores].pontos = pontuacao;
                        qtd_jogadores++;
                    }

                    for (int i = 0; i < qtd_jogadores - 1; i++) {
                        for (int j = 0; j < qtd_jogadores - i - 1; j++) {
                            if (lista_jogadores[j].pontos < lista_jogadores[j + 1].pontos) {
                                RegistroJogador temp = lista_jogadores[j];
                                lista_jogadores[j] = lista_jogadores[j + 1];
                                lista_jogadores[j + 1] = temp;
                            }
                        }
                    }

                    FILE* arquivo_escrita = fopen("placar.txt", "w"); 
                    if (arquivo_escrita != NULL) {
                        for (int i = 0; i < qtd_jogadores; i++) {
                            fprintf(arquivo_escrita, "Jogador: %s | Pontos: %d\n", lista_jogadores[i].nome, lista_jogadores[i].pontos);
                        }
                        fclose(arquivo_escrita);
                    }
                }

                for (int i = 0; i < 3; i++) {
                    atualizar(&patos[i], i);
                }

                for (int i = 0; i < MAX_FAISCAS; i++) {
                    if (faiscas[i].vida > 0) {
                        faiscas[i].x += faiscas[i].vel_x;
                        faiscas[i].y += faiscas[i].vel_y;
                        faiscas[i].vel_y += 0.2; 
                        faiscas[i].vida--;
                    }
                }

                if (atirou) {
                    frame_tiro--;
                    if (frame_tiro <= 0) {
                        atirou = false;
                    }
                }
            }

            ALLEGRO_TRANSFORM transformar;
            al_identity_transform(&transformar);
            al_scale_transform(&transformar, (float)largura_tela / LARGURA_BASE, (float)altura_tela / ALTURA_BASE);
            al_use_transform(&transformar);

            al_clear_to_color(al_map_rgb(0, 0, 0));

            al_draw_scaled_bitmap(fundo, 0, 0,
                al_get_bitmap_width(fundo), al_get_bitmap_height(fundo),
                0, 0, LARGURA_BASE, ALTURA_BASE, 0);

            // ---------------- ESTADO 0: MENU PRINCIPAL ----------------
            if (estado_atual == 0) {
                al_draw_filled_rectangle(LARGURA_BASE / 2 - 200, 100, LARGURA_BASE / 2 + 200, 480, al_map_rgba(0, 0, 0, 200));
                al_draw_rectangle(LARGURA_BASE / 2 - 200, 100, LARGURA_BASE / 2 + 200, 480, al_map_rgb(255, 215, 0), 2);

                al_draw_text(fonte, al_map_rgb(255, 50, 50), LARGURA_BASE / 2, 140, ALLEGRO_ALIGN_CENTER, "DUCK SHOOT CIRCO");
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, 170, ALLEGRO_ALIGN_CENTER, "Menu Principal");
                al_draw_text(fonte, al_map_rgb(150, 150, 150), LARGURA_BASE / 2, 195, ALLEGRO_ALIGN_CENTER, "--------------------");

                al_draw_filled_rectangle(btn_jogar_x1, btn_jogar_y1, btn_jogar_x2, btn_jogar_y2, al_map_rgb(0, 160, 0));
                al_draw_rectangle(btn_jogar_x1, btn_jogar_y1, btn_jogar_x2, btn_jogar_y2, al_map_rgb(255, 255, 255), 1);
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, btn_jogar_y1 + 18, ALLEGRO_ALIGN_CENTER, "JOGAR");

                al_draw_filled_rectangle(btn_pontos_x1, btn_pontos_y1, btn_pontos_x2, btn_pontos_y2, al_map_rgb(0, 90, 180));
                al_draw_rectangle(btn_pontos_x1, btn_pontos_y1, btn_pontos_x2, btn_pontos_y2, al_map_rgb(255, 255, 255), 1);
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, btn_pontos_y1 + 18, ALLEGRO_ALIGN_CENTER, "VER PONTOS");

                al_draw_filled_rectangle(btn_sair_x1, btn_sair_y1, btn_sair_x2, btn_sair_y2, al_map_rgb(180, 0, 0));
                al_draw_rectangle(btn_sair_x1, btn_sair_y1, btn_sair_x2, btn_sair_y2, al_map_rgb(255, 255, 255), 1);
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, btn_sair_y1 + 18, ALLEGRO_ALIGN_CENTER, "SAIR");
            } 
            // ---------------- ESTADO 1: TELA DE DIGITAÇÃO ----------------
            else if (estado_atual == 1) {
                al_draw_filled_rectangle(LARGURA_BASE / 2 - 200, 180, LARGURA_BASE / 2 + 200, 420, al_map_rgba(0, 0, 0, 220));
                al_draw_rectangle(LARGURA_BASE / 2 - 200, 180, LARGURA_BASE / 2 + 200, 420, al_map_rgb(255, 215, 0), 2);

                al_draw_text(fonte, al_map_rgb(255, 215, 0), LARGURA_BASE / 2, 210, ALLEGRO_ALIGN_CENTER, "QUEM ESTA JOGANDO?");
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, 240, ALLEGRO_ALIGN_CENTER, "Digite o seu Nickname:");

                al_draw_filled_rectangle(LARGURA_BASE / 2 - 150, 280, LARGURA_BASE / 2 + 150, 320, al_map_rgb(255, 255, 255));
                al_draw_text(fonte, al_map_rgb(0, 0, 0), LARGURA_BASE / 2, 295, ALLEGRO_ALIGN_CENTER, nome_jogador);

                al_draw_text(fonte, al_map_rgb(150, 150, 150), LARGURA_BASE / 2, 350, ALLEGRO_ALIGN_CENTER, "Pressione [ENTER] para iniciar");
            }
            // ---------------- ESTADO 2: JOGO ROLANDO ----------------
            else if (estado_atual == 2) {
                // MODIFICADO: Renderização dos patos usando rotação acumulada (Tombamento)
                for (int i = 0; i < 3; i++) {
                    if ((patos[i].vivo && patos[i].tempo_renascer >= 0) || patos[i].atingido) {
                        int img_w = al_get_bitmap_width(pato_img);
                        int img_h = al_get_bitmap_height(pato_img);

                        // Define o ponto pivô de rotação no centro da base horizontal inferior do pato
                        float pivo_x = img_w / 2.0;
                        float pivo_y = (float)img_h;

                        // Ajusta o destino na tela considerando a escala
                        float dest_x = patos[i].x + (PATO_LARGURA / 2.0);
                        float dest_y = patos[i].y + PATO_ALTURA;

                        float escala_x = (float)PATO_LARGURA / img_w;
                        float escala_y = (float)PATO_ALTURA / img_h;

                        al_draw_scaled_rotated_bitmap(pato_img, pivo_x, pivo_y, 
                                                      dest_x, dest_y, escala_x, escala_y, 
                                                      patos[i].angulo, 0);
                    }
                }

                float arma_x = mx - (ARMA_LARGURA / 2);
                float arma_y = ALTURA_BASE - ARMA_ALTURA;

                if (atirou) {
                    arma_y -= 25; 
                    al_draw_filled_circle(impacto_x, impacto_y, 4, al_map_rgb(255, 255, 255));
                    al_draw_filled_circle(mx, arma_y + 20, 15, al_map_rgb(255, 200, 0)); 
                }

                for (int i = 0; i < MAX_FAISCAS; i++) {
                    if (faiscas[i].vida > 0) {
                        float tamanho = 1.0 + (faiscas[i].vida / 6.0);
                        al_draw_filled_circle(faiscas[i].x, faiscas[i].y, tamanho, 
                            al_map_rgb(255, faiscas[i].cor_g, 0));
                    }
                }

                al_draw_scaled_bitmap(arma, 0, 0,
                    al_get_bitmap_width(arma), al_get_bitmap_height(arma),
                    arma_x, arma_y, ARMA_LARGURA, ARMA_ALTURA, 0);

                al_draw_filled_rectangle(LARGURA_BASE / 2 - 180, ALTURA_BASE - 40, LARGURA_BASE / 2 + 180, ALTURA_BASE - 10, al_map_rgba(0, 0, 0, 180));
                al_draw_rectangle(LARGURA_BASE / 2 - 180, ALTURA_BASE - 40, LARGURA_BASE / 2 + 180, ALTURA_BASE - 10, al_map_rgb(255, 215, 0), 1); 

                al_draw_textf(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, ALTURA_BASE - 30, ALLEGRO_ALIGN_CENTER,
                              "JOGADOR: %s  |  PONTOS: %d  |  TEMPO: %ds", nome_jogador, pontuacao, tempo_restante);
            } 
            // ---------------- ESTADO 3: FIM DE JOGO ----------------
            else if (estado_atual == 3) {
                al_draw_filled_rectangle(LARGURA_BASE / 2 - 250, 80, LARGURA_BASE / 2 + 250, 520, al_map_rgba(0, 0, 0, 220));
                al_draw_rectangle(LARGURA_BASE / 2 - 250, 80, LARGURA_BASE / 2 + 250, 520, al_map_rgb(255, 215, 0), 3); 

                al_draw_text(fonte, al_map_rgb(255, 50, 50), LARGURA_BASE / 2, 110, ALLEGRO_ALIGN_CENTER, "FIM DE JOGO!");
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, 140, ALLEGRO_ALIGN_CENTER, "RANKING GERAL (TOP 5)");
                al_draw_text(fonte, al_map_rgb(150, 150, 150), LARGURA_BASE / 2, 170, ALLEGRO_ALIGN_CENTER, "------------------------");

                int limite_exibicao = (qtd_jogadores < 5) ? qtd_jogadores : 5;
                int pos_y = 200;

                for (int i = 0; i < limite_exibicao; i++) {
                    ALLEGRO_COLOR cor_linha = al_map_rgb(255, 255, 255);
                    if (strcmp(lista_jogadores[i].nome, nome_jogador) == 0) {
                        cor_linha = al_map_rgb(255, 215, 0); 
                    }

                    al_draw_textf(fonte, cor_linha, LARGURA_BASE / 2, pos_y, ALLEGRO_ALIGN_CENTER,
                                  "%d. %-15s ...... %d pts", i + 1, lista_jogadores[i].nome, lista_jogadores[i].pontos);
                    pos_y += 40; 
                }

                al_draw_text(fonte, al_map_rgb(150, 150, 150), LARGURA_BASE / 2, 400, ALLEGRO_ALIGN_CENTER, "------------------------");

                al_draw_filled_rectangle(btn_reiniciar_x1, btn_reiniciar_y1, btn_reiniciar_x2, btn_reiniciar_y2, al_map_rgb(0, 160, 0));
                al_draw_rectangle(btn_reiniciar_x1, btn_reiniciar_y1, btn_reiniciar_x2, btn_reiniciar_y2, al_map_rgb(255, 255, 255), 1);
                al_draw_text(fonte, al_map_rgb(255, 255, 255), (btn_reiniciar_x1 + btn_reiniciar_x2) / 2, btn_reiniciar_y1 + 18, ALLEGRO_ALIGN_CENTER, "NOVO JOGO");

                al_draw_filled_rectangle(btn_fim_sair_x1, btn_fim_sair_y1, btn_fim_sair_x2, btn_fim_sair_y2, al_map_rgb(180, 0, 0));
                al_draw_rectangle(btn_fim_sair_x1, btn_fim_sair_y1, btn_fim_sair_x2, btn_fim_sair_y2, al_map_rgb(255, 255, 255), 1);
                al_draw_text(fonte, al_map_rgb(255, 255, 255), (btn_fim_sair_x1 + btn_fim_sair_x2) / 2, btn_fim_sair_y1 + 18, ALLEGRO_ALIGN_CENTER, "SAIR");
            }
            // ---------------- ESTADO 4: APENAS VISUALIZAR PONTOS ----------------
            else if (estado_atual == 4) {
                al_draw_filled_rectangle(LARGURA_BASE / 2 - 250, 80, LARGURA_BASE / 2 + 250, 520, al_map_rgba(0, 0, 0, 220));
                al_draw_rectangle(LARGURA_BASE / 2 - 250, 80, LARGURA_BASE / 2 + 250, 520, al_map_rgb(0, 120, 255), 3); 

                al_draw_text(fonte, al_map_rgb(0, 160, 255), LARGURA_BASE / 2, 110, ALLEGRO_ALIGN_CENTER, "QUADRO DE RECORDES");
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, 140, ALLEGRO_ALIGN_CENTER, "RANKING GERAL (TOP 5)");
                al_draw_text(fonte, al_map_rgb(150, 150, 150), LARGURA_BASE / 2, 170, ALLEGRO_ALIGN_CENTER, "------------------------");

                int limite_exibicao = (qtd_jogadores < 5) ? qtd_jogadores : 5;
                int pos_y = 200;

                for (int i = 0; i < limite_exibicao; i++) {
                    al_draw_textf(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, pos_y, ALLEGRO_ALIGN_CENTER,
                                  "%d. %-15s ...... %d pts", i + 1, lista_jogadores[i].nome, lista_jogadores[i].pontos);
                    pos_y += 40; 
                }

                al_draw_text(fonte, al_map_rgb(150, 150, 150), LARGURA_BASE / 2, 410, ALLEGRO_ALIGN_CENTER, "------------------------");

                al_draw_filled_rectangle(btn_voltar_x1, btn_voltar_y1, btn_voltar_x2, btn_voltar_y2, al_map_rgb(70, 80, 95));
                al_draw_rectangle(btn_voltar_x1, btn_voltar_y1, btn_voltar_x2, btn_voltar_y2, al_map_rgb(255, 255, 255), 1);
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, btn_voltar_y1 + 17, ALLEGRO_ALIGN_CENTER, "VOLTAR");
            }

            // Desenho do Botão "X" 
            if (estado_atual == 1 || estado_atual == 2) {
                al_draw_filled_rectangle(btn_x_fechar_x1, btn_x_fechar_y1, btn_x_fechar_x2, btn_x_fechar_y2, al_map_rgb(180, 0, 0));
                al_draw_rectangle(btn_x_fechar_x1, btn_x_fechar_y1, btn_x_fechar_x2, btn_x_fechar_y2, al_map_rgb(255, 255, 255), 1);
                al_draw_text(fonte, al_map_rgb(255, 255, 255), (btn_x_fechar_x1 + btn_x_fechar_x2) / 2, btn_x_fechar_y1 + 10, ALLEGRO_ALIGN_CENTER, "X");
            }

            al_flip_display();
        }
    }

    ////// FINALIZACAO ////// 
    al_destroy_font(fonte); 
    al_destroy_bitmap(fundo);
    al_destroy_bitmap(pato_img);
    al_destroy_bitmap(arma);
    al_destroy_timer(timer);
    al_destroy_event_queue(fila);
    al_destroy_display(display);

    return 0;
}