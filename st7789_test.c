#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"

#include "resources.h"
#include "st7789.h"

#define SPRITE_W    32
#define SPRITE_H    32

#define TEST_SCREEN_ITER    600
#define TEST_SPRITE_ITER    24000

/*
    ST7789(V2)高速描画ライブラリテスト用プログラム
        Ver1.0
    (C)2023 I.H(@LunaTsukinashi)
*/

static float screenTest(int iter);
static float spriteTest(int iter);

static uint64_t get_time(void) {
    // Reading low latches the high value
    uint32_t lo = timer_hw->timelr;
    uint32_t hi = timer_hw->timehr;
    return ((uint64_t) hi << 32u) | lo;
}

int main(void){
    stdio_init_all();
    sleep_ms(500);
    printf("=========================================\r\n");
    printf("ST7789 Graphics Library Test Programme   \r\n");
    printf("             (C)2023 I.H(@LunaTsukinashi)\r\n");
    printf("=========================================\r\n");
    printf("\r\n");
    printf("LCD Init...");
    LCD_Init();
    printf("OK!\r\n\r\n");
    printf("-----------------------------------------\r\n");
    float fps = screenTest(TEST_SCREEN_ITER);
    printf("\r\n");
    printf("-----------------------------------------\r\n");
    float sps = spriteTest(TEST_SPRITE_ITER);
    printf("\r\nAll Test Finished.\r\n");
    printf("-----------------------------------------\r\n");
    printf("SUMMARY:\r\n");
    printf("\t%4.2f frames/sec\r\n",fps);
    printf("\t%7.2f sprites/sec\r\n",sps);
    printf("=========================================\r\n");
    while(1);    
}

static float screenTest(int iter){
    printf("1.Screen Refreshrate Test Begin.\r\n");
    printf("Iteration:%04d frames.\r\n\r\n",iter);
    uint64_t st = get_time();
    for(int f = 0;f < iter;f++){
        Draw(0,0,icon_large,240,240);
        if(f % 150 == 0 && f){
            uint64_t elapsed = get_time() - st;
            printf("%04d frames:%5.3f sec\r\n",f,(elapsed / (float)1e6));
        }
    }
    uint64_t et = get_time() - st;
    float fps = iter / (et / (float)1e6);
    printf("Total Time:%5.3f sec.,Framerate: %4.2f fps\r\n",et / (float)1e6,fps);
    return fps;
}

static float spriteTest(int iter){
    printf("2.Sprite Rendering Test Begin.\r\n");
    printf("Iteration:%04d sprites.\r\n\r\n",iter);
    uint64_t st = get_time();
    for(int s = 0;s < iter;s++){
        int x = rand() % (239 - SPRITE_W);
        int y = rand() % (239 - SPRITE_H);

        Draw(x,y,icon_small,SPRITE_W,SPRITE_H);

        if(s % 3000 == 0 && s){
            uint64_t elapsed = get_time() - st;
            printf("%05d sprites:%5.3f sec\r\n",s,(elapsed / (float)1e6));
        }
    }
    uint64_t et = get_time() - st;
    float sps = iter / (et / (float)1e6);
    printf("Total Time:%5.3f sec.,%7.2f sprites/sec\r\n",et / (float)1e6,sps);    
    
    return sps;
}