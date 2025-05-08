#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

#include "include/ssd1306.h"
#include "include/ssd1306_i2c.h"
#include "include/ssd1306_font.h"

 /* ---- Botões ---- */
 #define BUTTON_A   5
 #define BUTTON_B   6
 
 /* ---- Parâmetros do tabuleiro ---- */
 #define OLED_W          128
 #define OLED_H           64
 #define NUM_BINS         16
 #define BIN_W           (OLED_W / NUM_BINS)
 
 #define PIN_SPACING_Y     4         /* distância vertical entre linhas */
 #define DX                4         /* passo lateral em cada pino      */
 #define MAX_BALLS         8
 #define TICK_MS          80
 #define SPAWN_INTERVAL   10
 #define HIST_SCALE       50
 #define BIAS             0.5f
 
 /* ---- Derivados para o desenho do board ---- */
 #define PIN_ROWS       ((OLED_H - 10) / PIN_SPACING_Y)      /* ≈13 linhas */
 #define BOARD_HALF     (PIN_ROWS * DX)                      /* deslocamento máx. */
 
 typedef struct { int x, y; bool alive; } Ball;
 
 /* ---- Globais ---- */
 static ssd1306_t disp;
 static Ball      balls[MAX_BALLS];
 static uint16_t  hist[NUM_BINS];
 static uint32_t  total_balls = 0;
 
 /* Flags */
 static volatile bool restart_requested = false;
 static volatile bool toggle_view       = false;   /* setado pelo BTN B */
 static bool show_histogram             = false;
 
 /* -------------------------------------------------------------------------- */
 /* Utilidades                                                                 */
 /* -------------------------------------------------------------------------- */
 static inline bool rand_dir(float bias) {
     return ((float)rand() / (float)RAND_MAX) < bias;
 }
 
 static inline void ball_spawn(Ball *b) {
     b->x = OLED_W / 2;
     b->y = 0;
     b->alive = true;
 }
 
 static void clear_simulation(void) {
     for (int i = 0; i < NUM_BINS;  i++) hist[i] = 0;
     for (int i = 0; i < MAX_BALLS; i++) balls[i].alive = false;
     total_balls = 0;
 }
 
 /* Barra vertical sólida */
 static void draw_bar(int bin, int height, int y0) {
     if (height <= 0) return;
     int x0 = bin * BIN_W;
     for (int dx = 0; dx < BIN_W - 1; dx++)
         for (int dy = 0; dy < height; dy++)
             ssd1306_draw_pixel(&disp, x0 + dx, y0 - dy);
 }
 
 /* Desenha triângulo de pinos limitando à largura real */
 static void draw_pins(void) {
     const int CX = OLED_W / 2;                     /* centro X */
     for (int row = 1; row <= PIN_ROWS; row++) {
         int y = row * PIN_SPACING_Y;
         int half = row * DX;                       /* alcance lateral desta linha */
         for (int col = -row; col <= row; col += 2) {
             int x = CX + col * DX;
             ssd1306_draw_pixel(&disp, x, y);
         }
     }
 }
 
 /* -------------------------------------------------------------------------- */
 /* OLED setup                                                                 */
 /* -------------------------------------------------------------------------- */
 static void oled_setup(void) {
     i2c_init(i2c1, 400 * 1000);
     gpio_set_function(14, GPIO_FUNC_I2C);
     gpio_set_function(15, GPIO_FUNC_I2C);
     gpio_pull_up(14); gpio_pull_up(15);
 
     disp.external_vcc = false;
     if (!ssd1306_init(&disp, OLED_W, OLED_H, 0x3C, i2c1)) {
         printf("OLED init falhou\n");
         while (1) tight_loop_contents();
     }
     ssd1306_clear(&disp);
     ssd1306_show(&disp);
 }
 
 /* -------------------------------------------------------------------------- */
 /* IRQ de botões                                                              */
 /* -------------------------------------------------------------------------- */
 static void button_irq_callback(uint gpio, uint32_t events) {
     static absolute_time_t last_a = {0}, last_b = {0};
     absolute_time_t now = get_absolute_time();
 
     if (gpio == BUTTON_A &&
         absolute_time_diff_us(last_a, now) > 200000) {
         last_a = now;
         restart_requested = true;
     }
     else if (gpio == BUTTON_B &&
              absolute_time_diff_us(last_b, now) > 200000) {
         last_b = now;
         toggle_view = true;
     }
 }
 
 /* -------------------------------------------------------------------------- */
 /* main                                                                       */
 /* -------------------------------------------------------------------------- */
 int main(void) {
     stdio_init_all();
     sleep_ms(500);
 
     srand((unsigned)time_us_64());
     oled_setup();
     clear_simulation();
 
     /* Configura botões */
     gpio_init(BUTTON_A); gpio_set_dir(BUTTON_A, GPIO_IN); gpio_pull_up(BUTTON_A);
     gpio_init(BUTTON_B); gpio_set_dir(BUTTON_B, GPIO_IN); gpio_pull_up(BUTTON_B);
 
     gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &button_irq_callback);
     gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, true);
 
     const int CX = OLED_W / 2;
     uint32_t tick = 0;
 
     while (1) {
 
         /* ---- Flags de controle ---- */
         if (restart_requested) {
             restart_requested = false;
             clear_simulation();
         }
 
         if (toggle_view) {
             toggle_view = false;
             show_histogram = !show_histogram;
         }
 
         /* ---- Atualiza física ---- */
         if (tick % SPAWN_INTERVAL == 0) {
             for (int i = 0; i < MAX_BALLS; i++)
                 if (!balls[i].alive) { ball_spawn(&balls[i]); break; }
         }
 
         for (int i = 0; i < MAX_BALLS; i++) {
             Ball *b = &balls[i];
             if (!b->alive) continue;
 
             if (b->y % PIN_SPACING_Y == 0) {
                 b->x += rand_dir(BIAS) ? DX : -DX;
                 if (b->x < CX - BOARD_HALF) b->x = CX - BOARD_HALF;
                 if (b->x > CX + BOARD_HALF) b->x = CX + BOARD_HALF;
             }
             b->y++;
 
             if (b->y >= OLED_H - 1) {
                 int bin = b->x / BIN_W;
                 hist[bin]++; total_balls++;
                 ball_spawn(b);
             }
         }
 
         /* ---- Desenho ---- */
         ssd1306_clear(&disp);
 
         if (!show_histogram) {
             /* -------- Tela 1: bolinhas + contador -------- */
             draw_pins();
 
             for (int i = 0; i < MAX_BALLS; i++)
                 if (balls[i].alive)
                     ssd1306_draw_pixel(&disp, balls[i].x, balls[i].y);
 
             char buf[16];
             snprintf(buf, sizeof buf, "%lu", total_balls);
             ssd1306_draw_string(&disp, 0, 0, 1, buf);
 
         } else {
             /* -------- Tela 2: histograma + contador -------- */
             for (int b = 0; b < NUM_BINS; b++) {
                 int h = (hist[b] * (OLED_H - 8)) / HIST_SCALE;
                 if (h > OLED_H - 8) h = OLED_H - 8;
                 draw_bar(b, h, OLED_H - 1);
             }
             char buf[16];
             snprintf(buf, sizeof buf, "%lu", total_balls);
             ssd1306_draw_string(&disp, 0, 0, 1, buf);
         }
 
         ssd1306_show(&disp);
 
         sleep_ms(TICK_MS);
         tick++;
     }
 }
