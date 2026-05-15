#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>

const int LARGURA = 800;
const int ALTURA = 600;
// NOME DA EQUIPE ALERRANDRO, JOÃO VITOR E KAUALY

//////////////// STRUCT //////////////////
typedef struct {
    float x, y;
    float vel;
    bool vivo;
} Pato;

//////////////// FUNÇÃO //////////////////
void atualizar(Pato* p) { // ponteiro
    p->x += p->vel;

    if (p->x > LARGURA) {
        p->x = -80;
        p->vivo = true;
    }
}

int main() {

    ////// INICIALIZAÇÃO ////// 
    if (!al_init()) return -1;
    if (!al_install_mouse()) return -1;
    if (!al_init_image_addon()) return -1;

    ALLEGRO_DISPLAY* display = al_create_display(LARGURA, ALTURA);
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60);
    ALLEGRO_EVENT_QUEUE* fila = al_create_event_queue();

    al_register_event_source(fila, al_get_display_event_source(display));
    al_register_event_source(fila, al_get_timer_event_source(timer));
    al_register_event_source(fila, al_get_mouse_event_source());

    ////// IMAGENS ////// 
    ALLEGRO_BITMAP* fundo = al_load_bitmap("fundo.jpg");
    ALLEGRO_BITMAP* pato_img = al_load_bitmap("pato.jpg");
    ALLEGRO_BITMAP* arma = al_load_bitmap("arma.jpg");

    if (!fundo || !pato_img || !arma) {
        printf("Erro ao carregar imagens\n");
        return -1;
    }

    ////// VETOR DE STRUCT ////// 
    Pato patos[3];

    for (int i = 0; i < 3; i++) {
        patos[i].x = -100 * i;
        patos[i].y = 150 + i * 100;
        patos[i].vel = 2 + i;
        patos[i].vivo = true;
    }

    float mx = 0, my = 0;

    al_start_timer(timer);

    ////// LOOP ////// 
    while (true) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(fila, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            break;

        if (ev.type == ALLEGRO_EVENT_MOUSE_AXES) {
            mx = ev.mouse.x;
            my = ev.mouse.y;
        }

        if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            for (int i = 0; i < 3; i++) {
                if (patos[i].vivo &&
                    mx >= patos[i].x && mx <= patos[i].x + 80 &&
                    my >= patos[i].y && my <= patos[i].y + 80) {

                    patos[i].vivo = false;
                }
            }
        }

        if (ev.type == ALLEGRO_EVENT_TIMER) {

            ////// ATUALIZA ////// 
            for (int i = 0; i < 3; i++) {
                atualizar(&patos[i]); // ponteiro
            }

            ////// DESENHO ////// 
            al_clear_to_color(al_map_rgb(0, 0, 0));

            al_draw_scaled_bitmap(fundo, 0, 0,
                al_get_bitmap_width(fundo), al_get_bitmap_height(fundo),
                0, 0, LARGURA, ALTURA, 0);

            for (int i = 0; i < 3; i++) {
                if (patos[i].vivo) {
                    al_draw_scaled_bitmap(pato_img, 0, 0,
                        al_get_bitmap_width(pato_img), al_get_bitmap_height(pato_img),
                        patos[i].x, patos[i].y, 80, 80, 0);
                }
            }

            al_draw_scaled_bitmap(arma, 0, 0,
                al_get_bitmap_width(arma), al_get_bitmap_height(arma),
                mx - 100, ALTURA - 150, 200, 150, 0);

            al_flip_display();
        }
    }

    ////// FINALIZA ////// 
    al_destroy_bitmap(fundo);
    al_destroy_bitmap(pato_img);
    al_destroy_bitmap(arma);

    al_destroy_timer(timer);
    al_destroy_display(display);
    al_destroy_event_queue(fila);

    return 0;
}