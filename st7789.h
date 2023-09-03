#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"

/*
    ST7789(V2)用高速描画ライブラリ
        Ver1.0
    (C)2023 I.H(@LunaTsukinashi)
*/

void LCD_Init(void);

void Fill(uint x1,uint y1,uint x2,uint y2,uint16_t color);

void Draw(uint x,uint y,uint16_t* graph,uint width,uint height);

/*以下レジスタ定義*/
#define SLPOUT      0x11    /*Sleep out*/
#define INVON       0x21    /*Display inversion on*/
#define DISPON      0x29    /*Display on*/
#define CASET       0x2A    /*Set X address*/
#define RASET       0x2B    /*Set Y address*/
#define RAMWR       0x2C    /*Write data*/
#define MADCTL      0x36    /*Memory data access control*/
#define COLMOD      0x3A    /*Interface pixel format*/
#define PORCTRL     0xB2    /*Porch control*/
#define GCTRL       0xB7    /*Gate control*/
#define VCOMS       0xBB    /*VCOM setting*/
#define LCMCTRL     0xC0    /*LCM control*/
#define VDVVRHEN    0xC2    /*VDV and VRH Command Enable*/
#define VRHS        0xC3    /*VRH Set*/
#define VDVSET      0xC4    /*VDV Setting*/
#define FRCTR2      0xC6    /*FR Control2*/
#define PWCTRL1     0xD0    /*Power Control 1*/
#define PVGAMCTRL   0xE0    /*Positive Voltage Gamma Control*/
#define NVGAMCTRL   0xE1    /*Negative Voltage Gamma Control*/
