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

#include "jogo.h"
#include "ranking.h"

int main() {
    srand(time(NULL));

    char nome_jogador[50] = ""; 
    int pontuacao = 0;
    
    int tempo_restante = 120; 
    int frames_contador = 0;  
    
    int estado_atual = 0; // 0=Menu, 1=Nick, 2=Jogo, 3=Fim, 4=Ranking, 5=Opções, 6=Créditos
    
    bool perdeu_por_vidas = false;

    ////// INICIALIZACAO ALLEGRO ////// 
    if (!al_init()) return -1;
    if (!al_init_primitives_addon()) return -1; 
    if (!al_install_mouse()) return -1;
    if (!al_install_keyboard()) return -1; 
    if (!al_init_image_addon()) return -1;
    
    if (!al_install_audio()) return -1;
    if (!al_init_acodec_addon()) return -1;
    if (!al_reserve_samples(4)) return -1;

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

    ////// MUSICA //////
    ALLEGRO_SAMPLE* musica_fundo = al_load_sample("musica_fundo.ogg"); 
    ALLEGRO_SAMPLE_INSTANCE* instancia_musica = NULL;

    if (musica_fundo) {
        instancia_musica = al_create_sample_instance(musica_fundo);
        al_set_sample_instance_playmode(instancia_musica, ALLEGRO_PLAYMODE_LOOP);
        al_attach_sample_instance_to_mixer(instancia_musica, al_get_default_mixer());
        al_set_sample_instance_gain(instancia_musica, volume_atual);
    }
    
    for (int i = 0; i < 5; i++) {
        resetar_pato(&patos[i], i);
    }

    Faisca faiscas[MAX_FAISCAS];
    for(int i = 0; i < MAX_FAISCAS; i++) faiscas[i].vida = 0; 

    TextoFlutuante textos_flutuantes[MAX_TEXTOS_FLUTUANTES];
    for (int i = 0; i < MAX_TEXTOS_FLUTUANTES; i++) textos_flutuantes[i].vida = 0;

    NoJogador* lista_placar = NULL;

    float mx = 0, my = 0;
    int frame_tiro = 0; 
    bool atirou = false;
    float impacto_x = 0, impacto_y = 0;

    // Alinhamentos dos botões do Menu
    int largura_btn = 200, altura_btn = 40;
    int escoramento_x1 = LARGURA_BASE / 2 - (largura_btn / 2);
    int escoramento_x2 = LARGURA_BASE / 2 + (largura_btn / 2);
    int y_inicial = 200, espacamento = 55; 

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

    int btn_reiniciar_x1 = LARGURA_BASE / 2 - 140, btn_reiniciar_y1 = 430;
    int btn_reiniciar_x2 = LARGURA_BASE / 2 - 10,  btn_reiniciar_y2 = 480;
    int btn_fim_sair_x1  = LARGURA_BASE / 2 + 10,  btn_fim_sair_y1  = 430;
    int btn_fim_sair_x2  = LARGURA_BASE / 2 + 140, btn_fim_sair_y2  = 480;

    int btn_x_fechar_x1 = LARGURA_BASE - 50, btn_x_fechar_y1 = 20;
    int btn_x_fechar_x2 = LARGURA_BASE - 20, btn_x_fechar_y2 = 50;
    int btn_voltar_x1 = LARGURA_BASE / 2 - 80, btn_voltar_y1 = 450;
    int btn_voltar_x2 = LARGURA_BASE / 2 + 80, btn_voltar_y2 = 495;

    int slider_x = LARGURA_BASE / 2 - 150, slider_y = 260, slider_largura = 300;
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
                perdeu_por_vidas = false; 
                if (instancia_musica && !mutado) al_play_sample_instance(instancia_musica);
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
                    liberar_lista(lista_placar);
                    lista_placar = carregar_placar_lista(); 
                    estado_atual = 0; 
                    continue; 
                }

                if (estado_atual == 0) {
                    if (mx >= btn_jogar_x1 && mx <= btn_jogar_x2 && my >= btn_jogar_y1 && my <= btn_jogar_y2) estado_atual = 1; 
                    else if (mx >= btn_opcoes_x1 && mx <= btn_opcoes_x2 && my >= btn_opcoes_y1 && my <= btn_opcoes_y2) estado_atual = 5; 
                    else if (mx >= btn_pontos_x1 && mx <= btn_pontos_x2 && my >= btn_pontos_y1 && my <= btn_pontos_y2) {
                        liberar_lista(lista_placar);
                        lista_placar = carregar_placar_lista(); 
                        estado_atual = 4; 
                    }
                    else if (mx >= btn_creditos_x1 && mx <= btn_creditos_x2 && my >= btn_creditos_y1 && my <= btn_creditos_y2) estado_atual = 6; 
                    else if (mx >= btn_sair_x1 && mx <= btn_sair_x2 && my >= btn_sair_y1 && my <= btn_sair_y2) break; 
                }
                else if (estado_atual == 5) { 
                    if (mx >= slider_x && mx <= slider_x + slider_largura && my >= slider_y - 15 && my <= slider_y + 15) {
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
                    else if (mx >= btn_voltar_x1 && mx <= btn_voltar_x2 && my >= btn_voltar_y1 && my <= btn_voltar_y2) estado_atual = 0; 
                }
                else if (estado_atual == 4 || estado_atual == 6) { 
                    // No estado 6 (créditos), o botão voltar foi deslocado 10px para baixo na renderização, mas a colisão herda o clique padrão com folga
                    if (mx >= btn_voltar_x1 && mx <= btn_voltar_x2 && my >= btn_voltar_y1 && my <= btn_voltar_y2 + 20) estado_atual = 0; 
                }
                else if (estado_atual == 2) {
                    if (!(mx >= btn_x_fechar_x1 && mx <= btn_x_fechar_x2 && my >= btn_x_fechar_y1 && my <= btn_x_fechar_y2)) {
                        atirou = true; frame_tiro = 6; impacto_x = mx; impacto_y = my;
                        float cano_x = mx, cano_y = (ALTURA_BASE - ARMA_ALTURA) - 25 + 20; 

                        for (int i = 0; i < MAX_FAISCAS; i++) {
                            faiscas[i].x = cano_x; faiscas[i].y = cano_y;
                            faiscas[i].vel_x = ((rand() % 100) - 50) / 10.0; 
                            faiscas[i].vel_y = -((rand() % 50) + 30) / 10.0; 
                            faiscas[i].vida = 10 + (rand() % 15); faiscas[i].cor_g = 100 + (rand() % 155);
                        }

                        int tipo_acerto = -1; 
                        for (int i = 0; i < 5; i++) {
                            if (patos[i].vivo && !patos[i].atingido && patos[i].tempo_renascer >= 0 &&
                                mx >= patos[i].x && mx <= patos[i].x + PATO_LARGURA &&
                                my >= patos[i].y && my <= patos[i].y + PATO_ALTURA) {
                                patos[i].vivo = false; patos[i].atingido = true; patos[i].angulo = 0.0;     
                                tipo_acerto = patos[i].tipo; break; 
                            }
                        }

                        if (tipo_acerto == 1) { 
                            pontuacao += 20; multiplicador_velocidade += 0.2f; 
                            criar_texto_flutuante(textos_flutuantes, impacto_x, impacto_y, "+20 SPEED!", al_map_rgb(50, 255, 50));
                        } else if (tipo_acerto == 2) { 
                            pontuacao -= 15; if (vidas > 0) vidas--; 
                            criar_texto_flutuante(textos_flutuantes, impacto_x, impacto_y, "-15 & -1 VIDA!", al_map_rgb(255, 0, 0));
                        } else if (tipo_acerto == 0) { 
                            pontuacao += 10; criar_texto_flutuante(textos_flutuantes, impacto_x, impacto_y, "+10", al_map_rgb(0, 230, 0));
                        } else { 
                            pontuacao -= 5; criar_texto_flutuante(textos_flutuantes, impacto_x, impacto_y, "-5", al_map_rgb(230, 0, 0));
                        }
                        if (pontuacao < 0) pontuacao = 0; 
                    }
                }
                else if (estado_atual == 3) {
                    if (mx >= btn_reiniciar_x1 && mx <= btn_reiniciar_x2 && my >= btn_reiniciar_y1 && my <= btn_reiniciar_y2) {
                        pontuacao = 0; tempo_restante = 120; frames_contador = 0; atirou = false;
                        strcpy(nome_jogador, ""); multiplicador_velocidade = 1.0f; vidas = 5; 
                        perdeu_por_vidas = false; 
                        for (int i = 0; i < 5; i++) resetar_pato(&patos[i], i);
                        for(int i = 0; i < MAX_FAISCAS; i++) faiscas[i].vida = 0; 
                        for (int i = 0; i < MAX_TEXTOS_FLUTUANTES; i++) textos_flutuantes[i].vida = 0;
                        estado_atual = 1; 
                    }
                    else if (mx >= btn_fim_sair_x1 && mx <= btn_fim_sair_x2 && my >= btn_fim_sair_y1 && my <= btn_fim_sair_y2) break; 
                }
            }
        }

        if (ev.type == ALLEGRO_EVENT_TIMER) {
            if (estado_atual == 2) {
                frames_contador++;
                if (frames_contador >= 60) {
                    if (tempo_restante > 0) tempo_restante--;
                    frames_contador = 0;
                }

                if (tempo_restante <= 0 || vidas <= 0) {
                    if (vidas <= 0) {
                        perdeu_por_vidas = true;
                        pontuacao = 0; 
                    }
                    
                    estado_atual = 3; 
                    if (instancia_musica) al_stop_sample_instance(instancia_musica);

                    liberar_lista(lista_placar);
                    lista_placar = carregar_placar_lista();
                    inserir_ordenado(&lista_placar, nome_jogador, pontuacao);
                    salvar_placar_lista(lista_placar);
                }

                for (int i = 0; i < 5; i++) atualizar(&patos[i], i, &pontuacao);

                for (int i = 0; i < MAX_FAISCAS; i++) {
                    if (faiscas[i].vida > 0) {
                        faiscas[i].x += faiscas[i].vel_x; faiscas[i].y += faiscas[i].vel_y;
                        faiscas[i].vel_y += 0.2; faiscas[i].vida--;
                    }
                }

                if (atirou) {
                    frame_tiro--;
                    if (frame_tiro <= 0) atirou = false;
                }
            }

            ALLEGRO_TRANSFORM transformar;
            al_identity_transform(&transformar);
            al_scale_transform(&transformar, (float)largura_tela / LARGURA_BASE, (float)altura_tela / ALTURA_BASE);
            al_use_transform(&transformar);

            al_clear_to_color(al_map_rgb(0, 0, 0));
            al_draw_scaled_bitmap(fundo, 0, 0, al_get_bitmap_width(fundo), al_get_bitmap_height(fundo), 0, 0, LARGURA_BASE, ALTURA_BASE, 0);

            if (estado_atual == 2) {
                al_draw_filled_rounded_rectangle(15, 15, 160, 47, 6, 6, al_map_rgba(40, 40, 40, 160));
                for (int i = 0; i < vidas; i++) {
                    if (i >= 0) desenhar_coracao(33 + (i * 26), 23, 16);
                }

                for (int i = 0; i < 5; i++) {
                    if ((patos[i].vivo && patos[i].tempo_renascer >= 0) || patos[i].atingido) {
                        ALLEGRO_BITMAP* img_atual = (patos[i].tipo == 1) ? pato_verde_img : ((patos[i].tipo == 2) ? pato_vermelho_img : pato_img);
                        al_draw_scaled_rotated_bitmap(img_atual, al_get_bitmap_width(img_atual)/2.0, al_get_bitmap_height(img_atual), patos[i].x + (PATO_LARGURA/2.0), patos[i].y + PATO_ALTURA, (float)PATO_LARGURA/al_get_bitmap_width(img_atual), (float)PATO_ALTURA/al_get_bitmap_height(img_atual), patos[i].angulo, 0);
                    }
                }

                float arma_x = mx - (ARMA_LARGURA / 2), arma_y = ALTURA_BASE - ARMA_ALTURA;
                if (atirou) { 
                    arma_y -= 25; 
                    al_draw_filled_circle(impacto_x, impacto_y, 4, al_map_rgb(255, 255, 255)); 
                }

                for (int i = 0; i < MAX_FAISCAS; i++) if (faiscas[i].vida > 0) al_draw_filled_circle(faiscas[i].x, faiscas[i].y, 1.0 + (faiscas[i].vida / 6.0), al_map_rgb(255, faiscas[i].cor_g, 0));

                for (int i = 0; i < MAX_TEXTOS_FLUTUANTES; i++) {
                    if (textos_flutuantes[i].vida > 0) { 
                        al_draw_text(fonte, textos_flutuantes[i].cor, textos_flutuantes[i].x, textos_flutuantes[i].y, ALLEGRO_ALIGN_CENTER, textos_flutuantes[i].texto);
                        textos_flutuantes[i].y -= 0.8f; textos_flutuantes[i].vida--;
                    }
                }

                al_draw_scaled_bitmap(arma, 0, 0, al_get_bitmap_width(arma), al_get_bitmap_height(arma), arma_x, arma_y, ARMA_LARGURA, ARMA_ALTURA, 0);
                
                al_draw_filled_rectangle(LARGURA_BASE / 2 - 180, ALTURA_BASE - 40, LARGURA_BASE / 2 + 180, ALTURA_BASE - 10, al_map_rgba(0, 0, 0, 180));
                al_draw_textf(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, ALTURA_BASE - 30, ALLEGRO_ALIGN_CENTER, "JOGADOR: %s  |  PONTOS: %d  |  TEMPO: %ds", nome_jogador, pontuacao, tempo_restante);
            } 
            else if (estado_atual == 0) {
                al_draw_filled_rectangle(LARGURA_BASE / 2 - 200, 80, LARGURA_BASE / 2 + 200, 520, al_map_rgba(0, 0, 0, 200));
                al_draw_rectangle(LARGURA_BASE / 2 - 200, 80, LARGURA_BASE / 2 + 200, 520, al_map_rgb(255, 215, 0), 2);
                al_draw_text(fonte, al_map_rgb(255, 50, 50), LARGURA_BASE / 2, 110, ALLEGRO_ALIGN_CENTER, "DUCK SHOOT CIRCO");
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, 140, ALLEGRO_ALIGN_CENTER, "Menu Principal");
                al_draw_text(fonte, al_map_rgb(150, 150, 150), LARGURA_BASE / 2, 165, ALLEGRO_ALIGN_CENTER, "--------------------");

                al_draw_filled_rectangle(btn_jogar_x1, btn_jogar_y1, btn_jogar_x2, btn_jogar_y2, al_map_rgb(0, 160, 0));
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, btn_jogar_y1 + 15, ALLEGRO_ALIGN_CENTER, "JOGAR");
                al_draw_filled_rectangle(btn_opcoes_x1, btn_opcoes_y1, btn_opcoes_x2, btn_opcoes_y2, al_map_rgb(200, 120, 0));
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, btn_opcoes_y1 + 15, ALLEGRO_ALIGN_CENTER, "OPCOES");
                al_draw_filled_rectangle(btn_pontos_x1, btn_pontos_y1, btn_pontos_x2, btn_pontos_y2, al_map_rgb(0, 90, 180));
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, btn_pontos_y1 + 15, ALLEGRO_ALIGN_CENTER, "VER PONTOS");
                al_draw_filled_rectangle(btn_creditos_x1, btn_creditos_y1, btn_creditos_x2, btn_creditos_y2, al_map_rgb(120, 50, 150));
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, btn_creditos_y1 + 15, ALLEGRO_ALIGN_CENTER, "CREDITOS");
                al_draw_filled_rectangle(btn_sair_x1, btn_sair_y1, btn_sair_x2, btn_sair_y2, al_map_rgb(180, 0, 0));
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, btn_sair_y1 + 15, ALLEGRO_ALIGN_CENTER, "SAIR");
            } 
            else if (estado_atual == 5) {
                al_draw_filled_rectangle(LARGURA_BASE / 2 - 200, 100, LARGURA_BASE / 2 + 200, 520, al_map_rgba(0, 0, 0, 220));
                al_draw_textf(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, 220, ALLEGRO_ALIGN_CENTER, "Volume: %d%%", (int)(volume_atual * 100));
                al_draw_filled_rectangle(slider_x, slider_y - 4, slider_x + slider_largura, slider_y + 4, al_map_rgb(80, 80, 80));
                al_draw_filled_rectangle(slider_x, slider_y - 4, slider_x + (volume_atual * slider_largura), slider_y + 4, al_map_rgb(255, 215, 0));
                al_draw_filled_circle(slider_x + (volume_atual * slider_largura), slider_y, 10, al_map_rgb(255, 255, 255));
                al_draw_filled_rectangle(btn_mute_x1, btn_mute_y1, btn_mute_x2, btn_mute_y2, mutado ? al_map_rgb(180, 0, 0) : al_map_rgb(40, 40, 40));
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, btn_mute_y1 + 15, ALLEGRO_ALIGN_CENTER, mutado ? "MUTADO" : "SILENCIAR");
                al_draw_filled_rectangle(btn_voltar_x1, btn_voltar_y1, btn_voltar_x2, btn_voltar_y2, al_map_rgb(70, 80, 95));
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, btn_voltar_y1 + 17, ALLEGRO_ALIGN_CENTER, "VOLTAR");
            }
            else if (estado_atual == 6) {
                // TELA DE CRÉDITOS EXPANDIDA (Largura de 500 para 700 pixels)
                al_draw_filled_rectangle(LARGURA_BASE / 2 - 350, 40, LARGURA_BASE / 2 + 350, 530, al_map_rgba(0, 0, 0, 220));
                
                // Imagem redimensionada de 400x300 para 640x380 de forma centralizada
                if (img_creditos) {
                    al_draw_scaled_bitmap(img_creditos, 0, 0, al_get_bitmap_width(img_creditos), al_get_bitmap_height(img_creditos), LARGURA_BASE / 2 - 320, 70, 640, 380, 0);
                }
                
                // Botão Voltar renderizado com leve ajuste vertical para encaixar no novo design
                al_draw_filled_rectangle(btn_voltar_x1, btn_voltar_y1 + 10, btn_voltar_x2, btn_voltar_y2 + 10, al_map_rgb(70, 80, 95));
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, btn_voltar_y1 + 27, ALLEGRO_ALIGN_CENTER, "VOLTAR");
            }
            else if (estado_atual == 1) {
                al_draw_filled_rectangle(LARGURA_BASE / 2 - 200, 180, LARGURA_BASE / 2 + 200, 420, al_map_rgba(0, 0, 0, 220));
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, 220, ALLEGRO_ALIGN_CENTER, "DIGITE SEU NICKNAME:");
                al_draw_filled_rectangle(LARGURA_BASE / 2 - 150, 280, LARGURA_BASE / 2 + 150, 320, al_map_rgb(255, 255, 255));
                al_draw_text(fonte, al_map_rgb(0, 0, 0), LARGURA_BASE / 2, 295, ALLEGRO_ALIGN_CENTER, nome_jogador);
            }
            else if (estado_atual == 3) {
                al_draw_filled_rectangle(LARGURA_BASE / 2 - 250, 80, LARGURA_BASE / 2 + 250, 520, al_map_rgba(0, 0, 0, 220));
                
                if (perdeu_por_vidas) {
                    al_draw_text(fonte, al_map_rgb(255, 50, 50), LARGURA_BASE / 2, 120, ALLEGRO_ALIGN_CENTER, "VOCE MORREU! PONTUACAO ZERADA");
                } else {
                    al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, 140, ALLEGRO_ALIGN_CENTER, "RANKING GERAL (TOP 5)");
                }

                NoJogador* atual = lista_placar;
                int pos_y = 200, cont = 0;
                while (atual != NULL && cont < 5) {
                    ALLEGRO_COLOR cor_linha = (strcmp(atual->nome, nome_jogador) == 0) ? al_map_rgb(255, 215, 0) : al_map_rgb(255, 255, 255);
                    al_draw_textf(fonte, cor_linha, LARGURA_BASE / 2, pos_y, ALLEGRO_ALIGN_CENTER, "%d. %-15s ...... %d pts", cont + 1, atual->nome, atual->pontos);
                    pos_y += 40; cont++; atual = atual->proximo;
                }

                al_draw_filled_rectangle(btn_reiniciar_x1, btn_reiniciar_y1, btn_reiniciar_x2, btn_reiniciar_y2, al_map_rgb(0, 160, 0));
                al_draw_text(fonte, al_map_rgb(255, 255, 255), (btn_reiniciar_x1 + btn_reiniciar_x2) / 2, btn_reiniciar_y1 + 18, ALLEGRO_ALIGN_CENTER, "REINICIAR");

                al_draw_filled_rectangle(btn_fim_sair_x1, btn_fim_sair_y1, btn_fim_sair_x2, btn_fim_sair_y2, al_map_rgb(180, 0, 0));
                al_draw_text(fonte, al_map_rgb(255, 255, 255), (btn_fim_sair_x1 + btn_fim_sair_x2) / 2, btn_fim_sair_y1 + 18, ALLEGRO_ALIGN_CENTER, "SAIR");
            }
            else if (estado_atual == 4) {
                al_draw_filled_rectangle(LARGURA_BASE / 2 - 250, 80, LARGURA_BASE / 2 + 250, 520, al_map_rgba(0, 0, 0, 220));
                al_draw_text(fonte, al_map_rgb(0, 160, 255), LARGURA_BASE / 2, 110, ALLEGRO_ALIGN_CENTER, "QUADRO DE RECORDES");

                NoJogador* atual = lista_placar;
                int pos_y = 200, cont = 0;
                while (atual != NULL && cont < 5) {
                    al_draw_textf(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, pos_y, ALLEGRO_ALIGN_CENTER, "%d. %-15s ...... %d pts", cont + 1, atual->nome, atual->pontos);
                    pos_y += 40; cont++; atual = atual->proximo;
                }

                al_draw_filled_rectangle(btn_voltar_x1, btn_voltar_y1, btn_voltar_x2, btn_voltar_y2, al_map_rgb(70, 80, 95));
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_BASE / 2, btn_voltar_y1 + 17, ALLEGRO_ALIGN_CENTER, "VOLTAR");
            }

            if (estado_atual == 1 || estado_atual == 2) {
                al_draw_filled_rectangle(btn_x_fechar_x1, btn_x_fechar_y1, btn_x_fechar_x2, btn_x_fechar_y2, al_map_rgb(180, 0, 0));
                al_draw_text(fonte, al_map_rgb(255, 255, 255), (btn_x_fechar_x1 + btn_x_fechar_x2) / 2, btn_x_fechar_y1 + 8, ALLEGRO_ALIGN_CENTER, "X");
            }

            al_flip_display();
        }
    }

    ////// FINALIZACAO ////// 
    liberar_lista(lista_placar);

    if (instancia_musica) al_destroy_sample_instance(instancia_musica);
    if (musica_fundo) al_destroy_sample(musica_fundo);
    al_destroy_font(fonte); al_destroy_bitmap(fundo);
    al_destroy_bitmap(pato_img); al_destroy_bitmap(pato_verde_img); al_destroy_bitmap(pato_vermelho_img); al_destroy_bitmap(arma);
    if (img_creditos) al_destroy_bitmap(img_creditos);
    al_destroy_timer(timer); al_destroy_event_queue(fila); al_destroy_display(display);

    return 0;
}