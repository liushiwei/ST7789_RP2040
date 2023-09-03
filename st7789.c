#include "st7789.h"

/*
    ST7789(V2)用高速描画ライブラリ
        Ver1.0
    (C)2023 I.H(@LunaTsukinashi)
*/

/*画面サイズ定義*/
#define _LCD_W      240
#define _LCD_H      240

/*以下ペリフェラル定義*/
#define _LCD_SPI    spi0
#define _LCD_RES    1
#define _LCD_SCK    2
#define _LCD_DAT    3
#define _LCD_CSN    4
#define _LCD_DCN    5
/*
    ロジックアナライザ使用時は10MHz以下にセットすること
    さもなくばデータを取りこぼす
*/
#define _LCD_FRQ    1000 * 100 * 625UL 

static uint dma;    /*SPIに用いるDMA番号*/

/*設定パラメータ類*/
static const uint8_t DATA_PORCTRL[] = {0x0C,0x0C,0x00,0x33,0x33};
static const uint8_t DATA_PWCTRL1[] = {0xA4,0xA1};
static const uint8_t DATA_PVGAMCTRL[] = {0xD0,0x04,0x0D,0x11,0x13,0x2B,0x3F,0x54,0x4C,0x18,0x0D,0x0B,0x1F,0x23};
static const uint8_t DATA_NVGAMCTRL[] = {0xD0,0x04,0x0C,0x11,0x13,0x2C,0x3F,0x44,0x51,0x2F,0x1F,0x1F,0x20,0x23};

/*1バイトCPU転送*/
static void singleWriteCPU8(uint8_t addr,uint8_t data,int cmdOnly);

/*複数バイトCPU転送(8bit)*/
static void burstWriteCPU8(uint8_t addr,uint8_t* data,uint8_t length);

/*複数バイトCPU転送(16bit)*/
static void burstWriteCPU16(uint8_t addr,uint16_t* data,uint16_t length);

/*複数バイトDMA転送*/
static void burstWriteDMA16(uint8_t addr,uint16_t* data,uint16_t length,int inc);

/*VRAM書き込み位置セット*/
static void setMemoryAddress(uint16_t xs,uint16_t ys,uint16_t xe,uint16_t ye);

/*DMA16bit転送*/
static void xferDMA16(const uint16_t* addr,uint32_t length);

/*LCD初期化*/
void LCD_Init(void){
    /*
        ペリフェラル初期化
    */
    gpio_init(_LCD_RES);
    gpio_init(_LCD_CSN);
    gpio_init(_LCD_DCN);

    gpio_set_dir(_LCD_RES,true);
    gpio_set_dir(_LCD_CSN,true);
    gpio_set_dir(_LCD_DCN,true);

    gpio_set_function(_LCD_SCK,GPIO_FUNC_SPI);
    gpio_set_function(_LCD_DAT,GPIO_FUNC_SPI);

    spi_init(_LCD_SPI,_LCD_FRQ);
    
    /*
        コントローラの初期化
        ハードウェアリセットしてからパラメータをセット
        なんとなくDMAを使っていない。速度要らないし別にいっか
    */
    gpio_put(_LCD_RES,false);
    sleep_ms(100);
    gpio_put(_LCD_RES,true);

    singleWriteCPU8(MADCTL,0x00,0);
    singleWriteCPU8(COLMOD,0x05,0);

    
    burstWriteCPU8(PORCTRL,DATA_PORCTRL,sizeof(DATA_PORCTRL));

    singleWriteCPU8(GCTRL,0x35,0);
    singleWriteCPU8(VCOMS,0x19,0);
    singleWriteCPU8(LCMCTRL,0x2C,0);
    singleWriteCPU8(VDVVRHEN,0x01,0);
    singleWriteCPU8(VRHS,0x12,0);
    singleWriteCPU8(VDVSET,0x20,0);
    singleWriteCPU8(FRCTR2,0x0F,0);
    
    
    burstWriteCPU8(PWCTRL1,DATA_PWCTRL1,sizeof(DATA_PWCTRL1));

    
    burstWriteCPU8(PVGAMCTRL,DATA_PVGAMCTRL,sizeof(DATA_PVGAMCTRL));

    
    burstWriteCPU8(NVGAMCTRL,DATA_NVGAMCTRL,sizeof(DATA_NVGAMCTRL));

    singleWriteCPU8(INVON,0,1);

    singleWriteCPU8(SLPOUT,0,1);

    sleep_ms(120);

    singleWriteCPU8(DISPON,0,1);

    setMemoryAddress(0,0,_LCD_W - 1,_LCD_H - 1);

    dma = dma_claim_unused_channel(true);
    return; 
}
/*
    矩形塗りつぶし

    x1,y1   :   描画する四角形の左上の頂点座標
    x2,y2   :   描画する四角形の右下+1の頂点座標
    color   :   描画する四角形の色

*/

void Fill(uint x1,uint y1,uint x2,uint y2,uint16_t color){
    int width = x2 - x1;
    int height = y2 - y1;
    setMemoryAddress(x1,y1,x2 - 1,y2 -1);
    burstWriteDMA16(RAMWR,&color,width * height,0);
    return;
}

/*
    メモリに読み込んだグラフィックの描画

    x,y     :   グラフィックを描画する左上頂点の座標  
    graph   :   描画するグラフィックの先頭ポインタ
    width   :   グラフィックの幅
    height  :   グラフィックの高さ

*/
void Draw(uint x,uint y,uint16_t* graph,uint width,uint height){
    setMemoryAddress(x,y,(x+width-1),(y+height-1));
    burstWriteDMA16(RAMWR,graph,width*height,1);
    return;
}

/*
    VRAM書き込み位置セット

    xs,ys   :   始点座標
    xe,ye   :   終点座標

    各々の座標を「含む」範囲
*/
static void setMemoryAddress(uint16_t xs,uint16_t ys,uint16_t xe,uint16_t ye){
    /*
        CASET(2Ah)  Column Address Set
        RASET(2Bh)  Row Address Set
        XS[7:0],XE[7:0],YS[7:0],YE[7:0]はそれぞれを含む
        例:
            XS = 0,XE = 10の場合幅は11ピクセルとなる
            YS,YEについても同様
    */
    uint16_t DATA_CASET[] = {
        xs,
        xe,
    };

    uint16_t DATA_RASET[] = {
        ys,
        ye
    };

    burstWriteDMA16(0x2A,DATA_CASET,2,1);
    burstWriteDMA16(0x2B,DATA_RASET,2,1);
}

/*
    1バイトCPU転送

    addr    :   対象のレジスタアドレス
    data    :   セットするデータ
    cmdOnly :   データのないコマンドを送信時に非ゼロ(例:ソフトウェアリセット)
*/
static void singleWriteCPU8(uint8_t addr,uint8_t data,int cmdOnly){
    spi_set_format(_LCD_SPI,8,true,true,SPI_MSB_FIRST);
    gpio_put(_LCD_CSN,false);
    gpio_put(_LCD_DCN,false);
    spi_write_blocking(_LCD_SPI,&addr,1);
    if(cmdOnly){
        goto ret;
    }
    gpio_put(_LCD_DCN,true);
    spi_write_blocking(_LCD_SPI,&data,1);
ret:
    gpio_put(_LCD_CSN,true);
    return;
}

/*
    複数バイトCPU転送(8bit)
    
    addr    :   対象のレジスタアドレス
    data    :   セットするデータの先頭ポインタ
    length  :   データの要素数
*/
static void burstWriteCPU8(uint8_t addr,uint8_t* data,uint8_t length){
    spi_set_format(_LCD_SPI,8,true,true,SPI_MSB_FIRST);
    gpio_put(_LCD_CSN,false);
    gpio_put(_LCD_DCN,false);
    spi_write_blocking(_LCD_SPI,&addr,1);
    gpio_put(_LCD_DCN,true);
    spi_write_blocking(_LCD_SPI,data,length);
    gpio_put(_LCD_CSN,true);
    return;
}

/*
    複数バイトCPU転送(16bit)
    
    addr    :   対象のレジスタアドレス
    data    :   セットするデータの先頭ポインタ
    length  :   データの「要素数」
*/
static void burstWriteCPU16(uint8_t addr,uint16_t* data,uint16_t length){
    spi_set_format(_LCD_SPI,8,true,true,SPI_MSB_FIRST);
    gpio_put(_LCD_CSN,false);
    gpio_put(_LCD_DCN,false);
    spi_write_blocking(_LCD_SPI,&addr,1);
    gpio_put(_LCD_DCN,true);
    spi_set_format(_LCD_SPI,16,true,true,SPI_MSB_FIRST);
    spi_write16_blocking(_LCD_SPI,data,length);
    gpio_put(_LCD_CSN,true);
    return;
}

/*
    複数バイトDMA転送(16bit)
    
    addr    :   対象のレジスタアドレス
    data    :   セットするデータの先頭ポインタ
    length  :   データの「要素数」
    inc     :   送信対象データのアドレスをインクリメントするか
                (例:塗りつぶしなど常に同じデータを送り続ける際に非ゼロ)
*/
static void burstWriteDMA16(uint8_t addr,uint16_t* data,uint16_t length,int inc){
    dma_channel_config c = dma_channel_get_default_config(dma);
    singleWriteCPU8(addr,0x00,1);
    spi_set_format(_LCD_SPI,16,true,true,SPI_MSB_FIRST);
    gpio_put(_LCD_CSN,false);
    gpio_put(_LCD_DCN,true);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_dreq(&c, spi_get_dreq(_LCD_SPI, true));
    channel_config_set_read_increment(&c, !(inc == 0));
    dma_channel_configure(dma, &c,
                          &spi_get_hw(_LCD_SPI)->dr,
                          data,
                          length,
                          true);
    dma_channel_wait_for_finish_blocking(dma);
    while(spi_is_busy(_LCD_SPI)){}; /*DMA転送が終わっていてもSPI転送が終わっているとは限らないので待つ*/
    gpio_put(_LCD_CSN,true);
    return;
}