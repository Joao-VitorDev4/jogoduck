#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h> 
#include <time.h> 
#include <math.h> 

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h> 
#include <allegro5/allegro_font.h> 
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

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
#define MAX_TEXTOS_FLUTUANTES 10 

// Controle global do jogo
float multiplicador_velocidade = 1.0f;
int vidas = 5; 

// Controle Global de Áudio
float volume_atual = 0.5f;   
bool mutado = false;
bool arrastando_slider = false;

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
    bool atingido;      
    float angulo;       
    int tempo_renascer;
    int tipo; // 0 = Normal, 1 = Verde, 2 = Vermelho
} Pato;

typedef struct {
    char nome[50];
    int pontos;
} RegistroJogador;

typedef struct {
    float x, y;
    char texto[15];
    ALLEGRO_COLOR cor;
    int vida; 
} TextoFlutuante;

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

void desenhar_coracao(float x, float y, float tamanho) {
    float raio = tamanho / 4.0;
    al_draw_filled_circle(x - raio, y, raio, al_map_rgb(230, 30, 40));
    al_draw_filled_circle(x + raio, y, raio, al_map_rgb(230, 30, 40));
    al_draw_filled_triangle(x - tamanho/2.0, y + raio/2.0, 
                            x + tamanho/2.0, y + raio/2.0, 
                            x, y + tamanho, al_map_rgb(230, 30, 40));
}

int carregar_placar(RegistroJogador* lista) {
    int qtd = 0;
    FILE* arquivo = fopen("placar.bin", "rb"); 
    if (arquivo != NULL) {
        while (fread(&lista[qtd], sizeof(RegistroJogador), 1, arquivo) == 1) {
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

void criar_texto_flutuante(TextoFlutuante* lista, float x, float y, const char* texto, ALLEGRO_COLOR cor) {
    for (int i = 0; i < MAX_TEXTOS_FLUTUANTES; i++) {
        if (lista[i].vida <= 0) { 
            lista[i].x = x;
            lista[i].y = y - 15; 
            strcpy(lista[i].texto, texto);
            lista[i].cor = cor;
            lista[i].vida = 50; // Quantidade de frames que o texto existirá
            break;
        }
    }
}

int main() {
    srand(time(NULL));

    char nome_jogador[50] = ""; 
    int pontuacao = 0;
    
    int tempo_restante = 60; 
    int frames_contador = 0;  
    
    int estado_atual = 0; // 0=Menu, 1=Nick, 2=Jogo, 3=Fim, 4=Ranking, 5=Opções, 6=Créditos

    ////// INICIALIZACAO ALLEGRO ////// 
    if (!al_init()) return -1;
    if (!al_init_primitives_addon()) return -1; 
    if (!al_install_mouse()) return -1;
    if (!al_install_keyboard()) return -1; 
    if (!al_init_image_addon()) return -1;
    
    if (!al_install_audio()) {
        printf("Erro ao inicializar o audio.\n");
        return -1;
    }
    if (!al_init_acodec_addon()) {
        printf("Erro ao inicializar os codecs de audio.\n");
        return -1;
    }
    if (!al_reserve_samples(4)) { 
        printf("Erro ao reservar canais de audio.\n");
        return -1;
    }

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
    ALLEGRO_BITMAP* pato_verde_img = al_load_bitmap("pato_verde.jpg"); 
    ALLEGRO_BITMAP* pato_vermelho_img = al_load_bitmap("pato_vermelho.jpg"); 
    ALLEGRO_BITMAP* arma = al_load_bitmap("arma.jpg");
    ALLEGRO_BITMAP* img_creditos = al_load_bitmap("creditos.jpg"); 

    if (!fundo || !pato_img || !pato_verde_img || !pato_vermelho_img || !arma) {
        printf("Erro ao carregar imagens.\n");
        return -1;
    }

    ////// CARREGAMENTO DA MÚSICA //////
    ALLEGRO_SAMPLE* musica_fundo = al_load_sample("musica_fundo.ogg"); 
    ALLEGRO_SAMPLE_INSTANCE* instancia_musica = NULL;

    if (!musica_fundo) {
        printf("Aviso: Arquivo 'musica_fundo.ogg' nao encontrado.\n");
    } else {
        instancia_musica = al_create_sample_instance(musica_fundo);
        al_set_sample_instance_playmode(instancia_musica, ALLEGRO_PLAYMODE_LOOP);
        al_attach_sample_instance_to_mixer(instancia_musica, al_get_default_mixer());
        al_set_sample_instance_gain(instancia_musica, volume_atual);
    }
    
    for (int i = 0; i < 5; i++) {
        resetar_pato(&patos[i], i);
    }

    Faisca faiscas[MAX_FAISCAS];
    for(int i = 0; i < MAX_FAISCAS; i++) {
        faiscas[i].vida = 0; 
    }

    TextoFlutuante textos_flutuantes[MAX_TEXTOS_FLUTUANTES];
    for (int i = 0; i < MAX_TEXTOS_FLUTUANTES; i++) {
        textos_flutuantes[i].vida = 0;
    }

    float mx = 0, my = 0;
    int frame_tiro = 0; 
    bool atirou = false;

    float impacto_x = 0;
    float impacto_y = 0;

    RegistroJogador lista_jogadores[MAX_JOGADORES];
    int qtd_jogadores = 0;

    // Organização e alinhamento dos botões (Menu)
    int largura_btn = 200;
    int altura_btn = 40;
    int escoramento_x1 = LARGURA_BASE / 2 - (largura_btn / 2);
    int escoramento_x2 = LARGURA_BASE / 2 + (largura_btn / 2);
    int y_inicial = 200;
    int espacamento = 55; 

    int btn_jogar_x1 = escoramento_x1,    btn_jogar_y1 = y_inicial;
    int btn_jogar_x2 = escoramento_x2,    btn_jogar_y2 = y_inicial + altura_btn;

    int btn_opcoes_x1 = escoramento_x1,   btn_opcoes_y1 = y_inicial + espacamento;
    int btn_opcoes_x2 = escoramento_x2,   btn_opcoes_y2 = y_inicial + espacamento + altura_btn;

    int btn_pontos_x1 = escoramento_x1,   btn_pontos_y1 = y_inicial + (espacamento * 2);
    int btn_pontos_x2 = escoramento_x2,   btn_pontos_y2 = y_inicial + (espacamento * 2) + altura_btn;

    int btn_creditos_x1 = escoramento_x1, btn_creditos_y1 = y_inicial + (espacamento * 3);
    int btn_creditos_x2 = escoramento_x2, btn_creditos_y2 = y_inicial + (espacamento * 3) + altura_btn;

    int btn_sair_x1 = escoramento_x1,     btn_sair_y1 = y_inicial + (espacamento * 4);
    int btn_sair_x2 = escoramento_x2,     btn_sair_y2 = y_inicial + (espacamento * 4) + altura_btn;

    // Coordenadas das telas internas
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

    // Elementos do Slider
    int slider_x = LARGURA_BASE / 2 - 150;
    int slider_y = 260;
    int slider_largura = 300;
    int btn_mute_x1 = LARGURA_BASE / 2 - 80, btn_mute_y1 = 330;
    int btn_mute_x2 = LARGURA_BASE / 2 + 80, btn_mute_y2 = 375;

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

            if (estado_atual == 5 && arrastando_slider) {
                if (mx < slider_x) volume_atual = 0.0f;
                else if (mx > slider_x + slider_largura) volume_atual = 1.0f;
                else volume_atual = (mx - slider_x) / (float)slider_largura;

                if (instancia_musica) {
                    al_set_sample_instance_gain(instancia_musica, mutado ? 0.0f : volume_atual);
                }
            }
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
                if (instancia_musica && !mutado) {
                    al_play_sample_instance(instancia_musica);
                }
            }
        }

        if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
            if (ev.mouse.button & 1) {
                if (estado_atual == 5 && arrastando_slider) {
                    arrastando_slider = false;
                    if (instancia_musica) {
                        al_stop_sample_instance(instancia_musica);
                    }
                }
            }
        }

        if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            if (ev.mouse.button & 1) { 
                
                if ((estado_atual == 1 || estado_atual == 2) && 
                    mx >= btn_x_fechar_x1 && mx <= btn_x_fechar_x2 && my >= btn_x_fechar_y1 && my <= btn_x_fechar_y2) {
                    
                    if (instancia_musica) al_stop_sample_instance(instancia_musica);
                    qtd_jogadores = carregar_placar(lista_jogadores); 
                    estado_atual = 4; 
                    continue; 
                }

                if (estado_atual == 0) {
                    if (mx >= btn_jogar_x1 && mx <= btn_jogar_x2 && my >= btn_jogar_y1 && my <= btn_jogar_y2) {
                        estado_atual = 1; 
                    }
                    else if (mx >= btn_opcoes_x1 && mx <= btn_opcoes_x2 && my >= btn_opcoes_y1 && my <= btn_opcoes_y2) {
                        estado_atual = 5; 
                    }
                    else if (mx >= btn_pontos_x1 && mx <= btn_pontos_x2 && my >= btn_pontos_y1 && my <= btn_pontos_y2) {
                        qtd_jogadores = carregar_placar(lista_jogadores); 
                        estado_atual = 4; 
                    }
                    else if (mx >= btn_creditos_x1 && mx <= btn_creditos_x2 && my >= btn_creditos_y1 && my <= btn_creditos_y2) {
                        estado_atual = 6; 
                    }
                    else if (mx >= btn_sair_x1 && mx <= btn_sair_x2 && my >= btn_sair_y1 && my <= btn_sair_y2) {
                        break; 
                    }
                }
                else if (estado_atual == 5) { 
                    if (mx >= slider_x && mx <= slider_x + slider_largura && my >= slider_y - 10 && my <= slider_y + 10) {
                        arrastando_slider = true;
                        volume_atual = (mx - slider_x) / (float)slider_largura;
                        
                        if (instancia_musica) {
                            al_set_sample_instance_gain(instancia_musica, mutado ? 0.0f : volume_atual);
                            if (!mutado) al_play_sample_instance(instancia_musica); 
                        }
                    }
                    else if (mx >= btn_mute_x1 && mx <= btn_mute_x2 && my >= btn_mute_y1 && my <= btn_mute_y2) {
                        mutado = !mutado;
                        if (instancia_musica) {
                            al_set_sample_instance_gain(instancia_musica, mutado ? 0.0f : volume_atual);
                            if (mutado) al_stop_sample_instance(instancia_musica);
                        }
                    }
                    else if (mx >= btn_voltar_x1 && mx <= btn_voltar_x2 && my >= btn_voltar_y1 && my <= btn_voltar_y2) {
                        estado_atual = 0; 
                    }
                }
                else if (estado_atual == 4 || estado_atual == 6) { 
                    if (mx >= btn_voltar_x1 && mx <= btn_voltar_x2 && my >= btn_voltar_y1 && my <= btn_voltar_y2) {
                        estado_atual = 0; 
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

                        int tipo_acerto = -1; 

                        for (int i = 0; i < 5; i++) {
                            if (patos[i].vivo && !patos[i].atingido && patos[i].tempo_renascer >= 0 &&
                                mx >= patos[i].x && mx <= patos[i].x + PATO_LARGURA &&
                                my >= patos[i].y && my <= patos[i].y + PATO_ALTURA) {

                                patos[i].vivo = false;
                                patos[i].atingido = true; 
                                patos[i].angulo = 0.0;     
                                tipo_acerto = patos[i].tipo;
                                break; 
                            }
                        }

                        if (tipo_acerto == 1) { 
                            pontuacao += 20;
                            multiplicador_velocidade += 0.2f; 
                            criar_texto_flutuante(textos_flutuantes, impacto_x, impacto_y, "+20 SPEED!", al_map_rgb(50, 255, 50));
                        } 
                        else if (tipo_acerto == 2) { 
                            pontuacao -= 15;
                            if (vidas > 0) vidas--; 
                            criar_texto_flutuante(textos_flutuantes, impacto_x, impacto_y, "-15 & -1 VIDA!", al_map_rgb(255, 0, 0));
                        }
                        else if (tipo_acerto == 0) { 
                            pontuacao += 10; 
                            criar_texto_flutuante(textos_flutuantes, impacto_x, impacto_y, "+10", al_map_rgb(0, 230, 0));
                        } 
                        else { 
                            pontuacao -= 5;  
                            criar_texto_flutuante(textos_flutuantes, impacto_x, impacto_y, "-5", al_map_rgb(230, 0, 0));
                        }
                        
                        if (pontuacao < 0) pontuacao = 0; 
                    }
                }
                else if (estado_atual == 3) {
                    if (mx >= btn_reiniciar_x1 && mx <= btn_reiniciar_x2 && my >= btn_reiniciar_y1 && my <= btn_reiniciar_y2) {
                        pontuacao = 0;
                        tempo_restante = 60;
                        frames_contador = 0;
                        atirou = false;
                        strcpy(nome_jogador, ""); 
                        multiplicador_velocidade = 1.0f; 
                        vidas = 5; 
                        
                        for (int i = 0; i < 5; i++) {
                            resetar_pato(&patos[i], i);
                        }
                        for(int i = 0; i < MAX_FAISCAS; i++) {
                            faiscas[i].vida = 0; 
                        }
                        for (int i = 0; i < MAX_TEXTOS_FLUTUANTES; i++) {
                            textos_flutuantes[i].vida = 0;
                        }
                        estado_atual = 1; 
                    }
                    else if (mx >= btn_fim_sair_x1 && mx <= btn_fim_sair_x2 && my >= btn_fim_sair_y1 && my <= btn_fim_sair_y2) {
                        break; 
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

                if (tempo_restante <= 0 || vidas <= 0) {
                    estado_atual = 3; 

                    if (instancia_musica) {
                        al_stop_sample_instance(instancia_musica);
                    }

                    qtd_jogadores = 0;
                    FILE* arquivo_leitura = fopen("placar.bin", "rb");
                    if (arquivo_leitura != NULL) {
                        while (fread(&lista_jogadores[qtd_jogadores], sizeof(RegistroJogador), 1, arquivo_leitura) == 1) {
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

                    FILE* arquivo_escrita = fopen("placar.bin", "wb"); 
                    if (arquivo_escrita != NULL) {
                        fwrite(lista_jogadores, sizeof(RegistroJogador), qtd_jogadores, arquivo_escrita);
                        fclose(arquivo_escrita);
                    }
                }

                for (int i = 0; i < 5; i++) {
                    atualizar(&patos[i], i, &pontuacao);
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

            // Menu Principal
            if (estado_atual == 0) {
                al_draw_filled_rectangle(LARGURA_BASE / 2 - 200, 80, LARGURA_BASE / 2 + 200, 520, al_map_rgba(0, 0, 0, 200));
                al_draw_rectangle(LARGURA_BASE / 2 - 200, 80, LARGURA_BASE / 2 + 200, 520, al_map_rgb(255, 215, 0), 2);

                al_draw_text(fonte, al_map_rgb(255, 50, 50), LARGURA_BASE / 2, 110, ALLEGRO_ALIGN_CENTER, "DUCK SHOOT CIRCO");
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, 140, ALLEGRO_ALIGN_CENTER, "Menu Principal");
                al_draw_text(fonte, al_map_rgb(150, 150, 150), LARGURA_BASE / 2, 165, ALLEGRO_ALIGN_CENTER, "--------------------");

                al_draw_filled_rectangle(btn_jogar_x1, btn_jogar_y1, btn_jogar_x2, btn_jogar_y2, al_map_rgb(0, 160, 0));
                al_draw_rectangle(btn_jogar_x1, btn_jogar_y1, btn_jogar_x2, btn_jogar_y2, al_map_rgb(255, 255, 255), 1);
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, btn_jogar_y1 + 15, ALLEGRO_ALIGN_CENTER, "JOGAR");

                al_draw_filled_rectangle(btn_opcoes_x1, btn_opcoes_y1, btn_opcoes_x2, btn_opcoes_y2, al_map_rgb(200, 120, 0));
                al_draw_rectangle(btn_opcoes_x1, btn_opcoes_y1, btn_opcoes_x2, btn_opcoes_y2, al_map_rgb(255, 255, 255), 1);
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, btn_opcoes_y1 + 15, ALLEGRO_ALIGN_CENTER, "OPCOES");

                al_draw_filled_rectangle(btn_pontos_x1, btn_pontos_y1, btn_pontos_x2, btn_pontos_y2, al_map_rgb(0, 90, 180));
                al_draw_rectangle(btn_pontos_x1, btn_pontos_y1, btn_pontos_x2, btn_pontos_y2, al_map_rgb(255, 255, 255), 1);
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, btn_pontos_y1 + 15, ALLEGRO_ALIGN_CENTER, "VER PONTOS");

                al_draw_filled_rectangle(btn_creditos_x1, btn_creditos_y1, btn_creditos_x2, btn_creditos_y2, al_map_rgb(120, 50, 150));
                al_draw_rectangle(btn_creditos_x1, btn_creditos_y1, btn_creditos_x2, btn_creditos_y2, al_map_rgb(255, 255, 255), 1);
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, btn_creditos_y1 + 15, ALLEGRO_ALIGN_CENTER, "CREDITOS");

                al_draw_filled_rectangle(btn_sair_x1, btn_sair_y1, btn_sair_x2, btn_sair_y2, al_map_rgb(180, 0, 0));
                al_draw_rectangle(btn_sair_x1, btn_sair_y1, btn_sair_x2, btn_sair_y2, al_map_rgb(255, 255, 255), 1);
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, btn_sair_y1 + 15, ALLEGRO_ALIGN_CENTER, "SAIR");
            } 
            // Tela de Opções (ESTADO 5)
            else if (estado_atual == 5) {
                al_draw_filled_rectangle(LARGURA_BASE / 2 - 200, 100, LARGURA_BASE / 2 + 200, 520, al_map_rgba(0, 0, 0, 220));
                al_draw_rectangle(LARGURA_BASE / 2 - 200, 100, LARGURA_BASE / 2 + 200, 520, al_map_rgb(255, 215, 0), 2);

                al_draw_text(fonte, al_map_rgb(255, 215, 0), LARGURA_BASE / 2, 130, ALLEGRO_ALIGN_CENTER, "CONFIGURACOES");
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, 160, ALLEGRO_ALIGN_CENTER, "Ajuste de Som");
                al_draw_text(fonte, al_map_rgb(150, 150, 150), LARGURA_BASE / 2, 185, ALLEGRO_ALIGN_CENTER, "--------------------");

                al_draw_textf(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, 220, ALLEGRO_ALIGN_CENTER, "Volume: %d%%", (int)(volume_atual * 100));
                al_draw_filled_rectangle(slider_x, slider_y - 4, slider_x + slider_largura, slider_y + 4, al_map_rgb(80, 80, 80));
                al_draw_filled_rectangle(slider_x, slider_y - 4, slider_x + (volume_atual * slider_largura), slider_y + 4, al_map_rgb(255, 215, 0));
                al_draw_filled_circle(slider_x + (volume_atual * slider_largura), slider_y, 10, al_map_rgb(255, 255, 255));
                al_draw_circle(slider_x + (volume_atual * slider_largura), slider_y, 10, al_map_rgb(0, 0, 0), 1);

                ALLEGRO_COLOR cor_mute = mutado ? al_map_rgb(180, 0, 0) : al_map_rgb(40, 40, 40);
                al_draw_filled_rectangle(btn_mute_x1, btn_mute_y1, btn_mute_x2, btn_mute_y2, cor_mute);
                al_draw_rectangle(btn_mute_x1, btn_mute_y1, btn_mute_x2, btn_mute_y2, al_map_rgb(255, 255, 255), 1);
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, btn_mute_y1 + 15, ALLEGRO_ALIGN_CENTER, mutado ? "MUTADO" : "SILENCIAR");

                al_draw_filled_rectangle(btn_voltar_x1, btn_voltar_y1, btn_voltar_x2, btn_voltar_y2, al_map_rgb(70, 80, 95));
                al_draw_rectangle(btn_voltar_x1, btn_voltar_y1, btn_voltar_x2, btn_voltar_y2, al_map_rgb(255, 255, 255), 1);
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, btn_voltar_y1 + 17, ALLEGRO_ALIGN_CENTER, "VOLTAR");
            }
            // Tela de Créditos (ESTADO 6)
            else if (estado_atual == 6) {
                al_draw_filled_rectangle(LARGURA_BASE / 2 - 250, 60, LARGURA_BASE / 2 + 250, 520, al_map_rgba(0, 0, 0, 220));
                al_draw_rectangle(LARGURA_BASE / 2 - 250, 60, LARGURA_BASE / 2 + 250, 520, al_map_rgb(120, 50, 150), 3); 

                al_draw_text(fonte, al_map_rgb(180, 100, 220), LARGURA_BASE / 2, 85, ALLEGRO_ALIGN_CENTER, "CREDITOS DO JOGO");

                if (img_creditos) {
                    float img_w = al_get_bitmap_width(img_creditos);
                    float img_h = al_get_bitmap_height(img_creditos);
                    float max_w = 400;
                    float max_h = 300;
                    al_draw_scaled_bitmap(img_creditos, 0, 0, img_w, img_h, 
                                          LARGURA_BASE / 2 - (max_w / 2), 125, max_w, max_h, 0);
                } else {
                    al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, 240, ALLEGRO_ALIGN_CENTER, "[Imagem creditos.jpg nao encontrada]");
                }

                al_draw_filled_rectangle(btn_voltar_x1, btn_voltar_y1, btn_voltar_x2, btn_voltar_y2, al_map_rgb(70, 80, 95));
                al_draw_rectangle(btn_voltar_x1, btn_voltar_y1, btn_voltar_x2, btn_voltar_y2, al_map_rgb(255, 255, 255), 1);
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, btn_voltar_y1 + 17, ALLEGRO_ALIGN_CENTER, "VOLTAR");
            }
            // Tela de Digitação
            else if (estado_atual == 1) {
                al_draw_filled_rectangle(LARGURA_BASE / 2 - 200, 180, LARGURA_BASE / 2 + 200, 420, al_map_rgba(0, 0, 0, 220));
                al_draw_rectangle(LARGURA_BASE / 2 - 200, 180, LARGURA_BASE / 2 + 200, 420, al_map_rgb(255, 215, 0), 2);

                al_draw_text(fonte, al_map_rgb(255, 215, 0), LARGURA_BASE / 2, 210, ALLEGRO_ALIGN_CENTER, "QUEM ESTA JOGANDO?");
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, 240, ALLEGRO_ALIGN_CENTER, "Digite o seu Nickname:");

                al_draw_filled_rectangle(LARGURA_BASE / 2 - 150, 280, LARGURA_BASE / 2 + 150, 320, al_map_rgb(255, 255, 255));
                al_draw_text(fonte, al_map_rgb(0, 0, 0), LARGURA_BASE / 2, 295, ALLEGRO_ALIGN_CENTER, nome_jogador);

                al_draw_text(fonte, al_map_rgb(150, 150, 150), LARGURA_BASE / 2, 350, ALLEGRO_ALIGN_CENTER, "Pressione [ENTER] para iniciar");
            }
            // Jogo Rolando
            else if (estado_atual == 2) {
                al_draw_filled_rounded_rectangle(15, 15, 160, 47, 6, 6, al_map_rgba(40, 40, 40, 160));

                for (int i = 0; i < vidas; i++) {
                    desenhar_coracao(33 + (i * 26), 23, 16);
                }

                for (int i = 0; i < 5; i++) {
                    if ((patos[i].vivo && patos[i].tempo_renascer >= 0) || patos[i].atingido) {
                        
                        ALLEGRO_BITMAP* img_atual = pato_img;
                        if (patos[i].tipo == 1) img_atual = pato_verde_img;
                        else if (patos[i].tipo == 2) img_atual = pato_vermelho_img;

                        int img_w = al_get_bitmap_width(img_atual);
                        int img_h = al_get_bitmap_height(img_atual);

                        float pivo_x = img_w / 2.0;
                        float pivo_y = (float)img_h;

                        float dest_x = patos[i].x + (PATO_LARGURA / 2.0);
                        float dest_y = patos[i].y + PATO_ALTURA;

                        float escala_x = (float)PATO_LARGURA / img_w;
                        float escala_y = (float)PATO_ALTURA / img_h;

                        al_draw_scaled_rotated_bitmap(img_atual, pivo_x, pivo_y, 
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

                // =======================================================
                // ATUALIZAÇÃO E DESENHO DOS TEXTOS FLUTUANTES (CORRIGIDO)
                // =======================================================
                for (int i = 0; i < MAX_TEXTOS_FLUTUANTES; i++) {
                    if (textos_flutuantes[i].vida > 0) {
                        // Desenha o texto flutuante na tela
                        al_draw_text(fonte, textos_flutuantes[i].cor, textos_flutuantes[i].x, textos_flutuantes[i].y, ALLEGRO_ALIGN_CENTER, textos_flutuantes[i].texto);
                        
                        // Faz o texto subir e reduz seu tempo de vida a cada frame
                        textos_flutuantes[i].y -= 0.8f; 
                        textos_flutuantes[i].vida--;
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
            // Fim de Jogo
            else if (estado_atual == 3) {
                al_draw_filled_rectangle(LARGURA_BASE / 2 - 250, 80, LARGURA_BASE / 2 + 250, 520, al_map_rgba(0, 0, 0, 220));
                al_draw_rectangle(LARGURA_BASE / 2 - 250, 80, LARGURA_BASE / 2 + 250, 520, al_map_rgb(255, 215, 0), 3); 

                if(vidas <= 0) {
                    al_draw_text(fonte, al_map_rgb(255, 50, 50), LARGURA_BASE / 2, 95, ALLEGRO_ALIGN_CENTER, "GAME OVER!");
                } else {
                    al_draw_text(fonte, al_map_rgb(255, 50, 50), LARGURA_BASE / 2, 95, ALLEGRO_ALIGN_CENTER, "O TEMPO ESGOTOU!");
                }

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
            // Quadro de Recordes
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

            if (estado_atual == 1 || estado_atual == 2) {
                al_draw_filled_rectangle(btn_x_fechar_x1, btn_x_fechar_y1, btn_x_fechar_x2, btn_x_fechar_y2, al_map_rgb(180, 0, 0));
                al_draw_rectangle(btn_x_fechar_x1, btn_x_fechar_y1, btn_x_fechar_x2, btn_x_fechar_y2, al_map_rgb(255, 255, 255), 1);
                al_draw_text(fonte, al_map_rgb(255, 255, 255), (btn_x_fechar_x1 + btn_x_fechar_x2) / 2, btn_x_fechar_y1 + 10, ALLEGRO_ALIGN_CENTER, "X");
            }

            al_flip_display();
        }
    }

    ////// FINALIZACAO ////// 
    if (instancia_musica) al_destroy_sample_instance(instancia_musica);
    if (musica_fundo) al_destroy_sample(musica_fundo);

    al_destroy_font(fonte); 
    al_destroy_bitmap(fundo);
    al_destroy_bitmap(pato_img);
    al_destroy_bitmap(pato_verde_img); 
    al_destroy_bitmap(pato_vermelho_img); 
    al_destroy_bitmap(arma);
    if (img_creditos) al_destroy_bitmap(img_creditos);
    al_destroy_timer(timer);
    al_destroy_event_queue(fila);
    al_destroy_display(display);

    return 0;
}