/*
 * Galton Board OLED – 3 telas (bolas, histograma, contagem por bin)
 * com 10 bins (10 colunas de 12 px cada)
 */

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
 
 /* ---- Constantes ---- */
 #define BUTTON_A 5
 #define BUTTON_B 6
 
 #define OLED_W   128
 #define OLED_H    64
 
 #define NUM_BINS 10
 #define BIN_W    12                     /* 10 × 12 = 120 px usados   */
 #define LEFT_MARGIN 4                   /* (128‑120)/2               */
 
 #define PIN_SPACING_Y 4
 #define DX            4
 #define MAX_BALLS     8
 #define TICK_MS       80
 #define SPAWN_INTERVAL 10
 #define HIST_SCALE    50
 #define BIAS          0.5f
 
 #define PIN_ROWS   ((OLED_H - 10) / PIN_SPACING_Y)
 #define BOARD_HALF (PIN_ROWS * DX)       /* 52 px – cabe nos 60 px de half */
 
 typedef struct { int x,y; bool alive; } Ball;
 
 /* ---- Globais ---- */
 static ssd1306_t disp;
 static Ball      balls[MAX_BALLS];
 static uint16_t  hist[NUM_BINS];
 static uint32_t  total_balls = 0;
 
 static volatile bool restart_flag = false;
 static volatile bool next_view   = false;
 static uint8_t       view_mode   = 0;   /* 0=bolas 1=hist 2=tabela */
 
 /* ---- Helpers ---- */
 static inline bool rand_dir(float bias){
     return ((float)rand()/RAND_MAX) < bias;
 }
 static inline void ball_spawn(Ball *b){
     b->x = OLED_W/2;
     b->y = 0;
     b->alive = true;
 }
 static void clear_sim(void){
     for(int i=0;i<NUM_BINS;i++) hist[i]=0;
     for(int i=0;i<MAX_BALLS;i++) balls[i].alive=false;
     total_balls=0;
 }
 
 /* barras verticais */
 static void draw_bar(int bin,int h,int y0){
     if(h<=0) return;
     int x0 = LEFT_MARGIN + bin*BIN_W;
     for(int dx_=0;dx_<BIN_W-1;dx_++)
         for(int dy=0;dy<h;dy++)
             ssd1306_draw_pixel(&disp,x0+dx_,y0-dy);
 }
 
 /* triângulo de pinos */
 static void draw_pins(void){
     const int CX = OLED_W/2;
     for(int row=1;row<=PIN_ROWS;row++){
         int y = row*PIN_SPACING_Y;
         for(int col=-row;col<=row;col+=2){
             int x = CX + col*DX;
             if(x>=LEFT_MARGIN && x<=LEFT_MARGIN+NUM_BINS*BIN_W-1)
                 ssd1306_draw_pixel(&disp,x,y);
         }
     }
 }
 
 /* página 3 – contagens */
 static void draw_counts_page(void){
     char line[64];
     for(int row=0;row<2;row++){
         int off=0;
         for(int i=0;i<5;i++){
             int bin=row*5+i;
             off+=snprintf(&line[off],sizeof(line)-off,"|%3u",hist[bin]);
         }
         line[off++]='|'; line[off]='\0';
         ssd1306_draw_string(&disp,0,row*16,1,line);
     }
     char buf[20];
     snprintf(buf,sizeof buf,"Total:%lu",total_balls);
     ssd1306_draw_string(&disp,0,OLED_H-10,1,buf);
 }
 
 /* OLED init */
 static void oled_setup(void){
     i2c_init(i2c1,400000);
     gpio_set_function(14,GPIO_FUNC_I2C);
     gpio_set_function(15,GPIO_FUNC_I2C);
     gpio_pull_up(14); gpio_pull_up(15);
     disp.external_vcc=false;
     if(!ssd1306_init(&disp,OLED_W,OLED_H,0x3C,i2c1)){
         while(1);
     }
     ssd1306_clear(&disp); ssd1306_show(&disp);
 }
 
 /* Botões */
 static void btn_cb(uint gpio,uint32_t events){
     static absolute_time_t lastA={0},lastB={0};
     absolute_time_t now=get_absolute_time();
     if(gpio==BUTTON_A && absolute_time_diff_us(lastA,now)>200000){
         lastA=now; restart_flag=true;
     }
     if(gpio==BUTTON_B && absolute_time_diff_us(lastB,now)>200000){
         lastB=now; next_view=true;
     }
 }
 
 /* ---- main ---- */
 int main(void){
     stdio_init_all(); sleep_ms(500);
     srand((unsigned)time_us_64());
     oled_setup(); clear_sim();
 
     gpio_init(BUTTON_A); gpio_set_dir(BUTTON_A,GPIO_IN); gpio_pull_up(BUTTON_A);
     gpio_init(BUTTON_B); gpio_set_dir(BUTTON_B,GPIO_IN); gpio_pull_up(BUTTON_B);
     gpio_set_irq_enabled_with_callback(BUTTON_A,GPIO_IRQ_EDGE_FALL,true,&btn_cb);
     gpio_set_irq_enabled(BUTTON_B,GPIO_IRQ_EDGE_FALL,true);
 
     const int CX=OLED_W/2;
     const int MIN_X=LEFT_MARGIN;                    /* 4 px  */
     const int MAX_X=LEFT_MARGIN+NUM_BINS*BIN_W-1;   /* 123 px */
     uint32_t tick=0;
 
     while(true){
 
         if(restart_flag){ restart_flag=false; clear_sim(); }
         if(next_view){ next_view=false; view_mode=(view_mode+1)%3; }
 
         /* física */
         if(tick%SPAWN_INTERVAL==0){
             for(int i=0;i<MAX_BALLS;i++)
                 if(!balls[i].alive){ ball_spawn(&balls[i]); break; }
         }
         for(int i=0;i<MAX_BALLS;i++){
             Ball *b=&balls[i]; if(!b->alive) continue;
             if(b->y%PIN_SPACING_Y==0){
                 b->x += rand_dir(BIAS)? DX:-DX;
                 if(b->x<MIN_X) b->x=MIN_X;
                 if(b->x>MAX_X) b->x=MAX_X;
             }
             b->y++;
             if(b->y>=OLED_H-1){
                 int bin=(b->x-LEFT_MARGIN)/BIN_W;
                 hist[bin]++; total_balls++;
                 ball_spawn(b);
             }
         }
 
         /* desenho */
         ssd1306_clear(&disp);
         if(view_mode==0){
             draw_pins();
             for(int i=0;i<MAX_BALLS;i++)
                 if(balls[i].alive)
                     ssd1306_draw_pixel(&disp,balls[i].x,balls[i].y);
             char buf[16]; snprintf(buf,sizeof buf,"%lu",total_balls);
             ssd1306_draw_string(&disp,0,0,1,buf);
         }else if(view_mode==1){
             for(int b=0;b<NUM_BINS;b++){
                 int h=(hist[b]*(OLED_H-8))/HIST_SCALE;
                 if(h>OLED_H-8) h=OLED_H-8;
                 draw_bar(b,h,OLED_H-1);
             }
             char buf[16]; snprintf(buf,sizeof buf,"%lu",total_balls);
             ssd1306_draw_string(&disp,0,0,1,buf);
         }else{
             draw_counts_page();
         }
         ssd1306_show(&disp);
         sleep_ms(TICK_MS); tick++;
     }
 }
 