#include "st7789.h"

/*
ST7789 高速绘图库 (V2)
版本 1.0
(C)2023 I.H(@LunaTsukinashi)
*/

/*屏幕尺寸定义*/
#define _LCD_W      240
#define _LCD_H      320

/*下面的外设定义*/
#define _LCD_SPI    spi0
#define _LCD_RES    16
#define _LCD_SCK    18
#define _LCD_DAT    19
#define _LCD_CSN    12
#define _LCD_DCN    17
#define _LCD_BLK    13
/*
使用逻辑分析仪时，请将频率设置为 10MHz 或更低。
否则，数据将会丢失。
*/
#define _LCD_FRQ    1000 * 100 * 625UL 

static uint dma;    /*用于SPI的DMA编号*/

/*设置参数*/
static const uint8_t DATA_PORCTRL[] = {0x0C,0x0C,0x00,0x33,0x33};
static const uint8_t DATA_PWCTRL1[] = {0xA4,0xA1};
static const uint8_t DATA_PVGAMCTRL[] = {0xD0,0x04,0x0D,0x11,0x13,0x2B,0x3F,0x54,0x4C,0x18,0x0D,0x0B,0x1F,0x23};
static const uint8_t DATA_NVGAMCTRL[] = {0xD0,0x04,0x0C,0x11,0x13,0x2C,0x3F,0x44,0x51,0x2F,0x1F,0x1F,0x20,0x23};

/* 单字节 CPU 传输*/
static void singleWriteCPU8(uint8_t addr,uint8_t data,int cmdOnly);

/* 多字节 CPU 传输（8 位）*/
static void burstWriteCPU8(uint8_t addr,uint8_t* data,uint8_t length);

/* 多字节 CPU 传输（16 位）*/
static void burstWriteCPU16(uint8_t addr,uint16_t* data,uint16_t length);

/* 多字节 DMA 传输*/
static void burstWriteDMA16(uint8_t addr,uint16_t* data,uint32_t length,int inc);

/* 设置 VRAM 写入位置*/
static void setMemoryAddress(uint16_t xs,uint16_t ys,uint16_t xe,uint16_t ye);

/* DMA 16 位传输*/
static void xferDMA16(const uint16_t* addr,uint32_t length);

/*LCD初期化*/
void LCD_Init(void){
    
    gpio_init(_LCD_RES);
    gpio_init(_LCD_CSN);
    gpio_init(_LCD_DCN);
    gpio_init(_LCD_BLK);

    gpio_set_dir(_LCD_RES,true);
    gpio_set_dir(_LCD_CSN,true);
    gpio_set_dir(_LCD_DCN,true);
    gpio_set_dir(_LCD_BLK,true);
    gpio_put(_LCD_BLK,true);

    gpio_set_function(_LCD_SCK,GPIO_FUNC_SPI);
    gpio_set_function(_LCD_DAT,GPIO_FUNC_SPI);

    spi_init(_LCD_SPI,_LCD_FRQ);
    
    /*
    初始化控制器
    复位硬件，然后设置参数
    由于某种原因，我没有使用 DMA。我不需要速度，所以没问题。
    */
    gpio_put(_LCD_RES,false);
    sleep_ms(100);
    gpio_put(_LCD_RES,true);

    // 设置颜色模式、扫描方向、显示参数等
    singleWriteCPU8(MADCTL,0x00,0);   // 屏幕方向控制
    singleWriteCPU8(COLMOD,0x05,0);   // 16bit RGB565 模式
    
    burstWriteCPU8(PORCTRL,DATA_PORCTRL,sizeof(DATA_PORCTRL)); // 门控控制参数
    singleWriteCPU8(GCTRL,0x35,0);    // Gate control
    singleWriteCPU8(VCOMS,0x19,0);    // VCOM 设置
    singleWriteCPU8(LCMCTRL,0x2C,0);
    singleWriteCPU8(VDVVRHEN,0x01,0);
    singleWriteCPU8(VRHS,0x12,0);
    singleWriteCPU8(VDVSET,0x20,0);
    singleWriteCPU8(FRCTR2,0x0F,0);
    burstWriteCPU8(PWCTRL1,DATA_PWCTRL1,sizeof(DATA_PWCTRL1));
    burstWriteCPU8(PVGAMCTRL,DATA_PVGAMCTRL,sizeof(DATA_PVGAMCTRL)); // 正伽马校正
    burstWriteCPU8(NVGAMCTRL,DATA_NVGAMCTRL,sizeof(DATA_NVGAMCTRL)); // 负伽马校正

    singleWriteCPU8(INVON,0,1);   // 打开反显模式
    singleWriteCPU8(SLPOUT,0,1);  // 退出睡眠模式
    sleep_ms(120);
    singleWriteCPU8(DISPON,0,1);  // 打开显示

    // 设置初始绘图区域
    setMemoryAddress(0,0,_LCD_W - 1,_LCD_H - 1);

    // 申请一个 DMA 通道
    dma = dma_claim_unused_channel(true);
    return; 
}
/*
矩形填充 x1,y1：待绘制矩形左上顶点坐标
x2,y2：待绘制矩形右下顶点坐标 +1
color：待绘制矩形颜色

*/

void Fill(uint x1,uint y1,uint x2,uint y2,uint16_t color){
    int width = x2 - x1;
    int height = y2 - y1;
    setMemoryAddress(x1,y1,x2 - 1,y2 -1);
    burstWriteDMA16(RAMWR,&color,(uint32_t)width * (uint32_t)height,0);
    return;
}

/*
绘制加载到内存中的图形

x,y：待绘制图形左上顶点坐标
graph：指向待绘制图形起始位置的指针
width：图形宽度
height：图形高度

*/
void Draw(uint x,uint y,uint16_t* graph,uint width,uint height){
    setMemoryAddress(x,y,(x+width-1),(y+height-1));
    burstWriteDMA16(RAMWR,graph,(uint32_t)width * (uint32_t)height,1);
    return;
}

/*
设置 VRAM 写入位置

xs,ys：起始坐标
xe,ye：结束坐标

包含每个坐标的范围
*/
static void setMemoryAddress(uint16_t xs,uint16_t ys,uint16_t xe,uint16_t ye){
    /*
    CASET(2Ah) 设置列地址
    RASET(2Bh) 设置行地址
    XS[7:0]、XE[7:0]、YS[7:0]、YE[7:0] 均包含
    示例：
    如果 XS = 0 且 XE = 10，则宽度为 11 像素。
    YS 和 YE 也同样适用。
    */
    uint16_t DATA_CASET[] = {
        xs,
        xe,
    };

    uint16_t DATA_RASET[] = {
        ys,
        ye
    };

    burstWriteDMA16(0x2A,DATA_CASET,2,1);  // CASET 命令
    burstWriteDMA16(0x2B,DATA_RASET,2,1);  // RASET 命令
}

/*
1 字节 CPU 传输

addr：目标寄存器地址
data：要设置的数据
cmdOnly：发送不带数据的命令时（例如软件复位）为非零值
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
多字节 CPU 传输（8 位）

addr：目标寄存器地址
data：指向待设置数据的第一个指针
length：数据元素的数量
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
多字节 CPU 传输（16 位）

addr：目标寄存器地址
data：待设置数据的起始指针
length：数据元素数量
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
多字节 DMA 传输（16 位）

addr：目标寄存器地址
data：待设置数据的起始指针
length：数据元素数量
inc：是否递增待发送数据的地址
（例如，连续发送相同数据时，例如填充数据块时，则为非零值）
*/
static void burstWriteDMA16(uint8_t addr,uint16_t* data,uint32_t length,int inc){
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
                          (void*)length,
                          true);
    dma_channel_wait_for_finish_blocking(dma);
    while(spi_is_busy(_LCD_SPI)){}; /*即使DMA传输完成，也不一定意味着SPI传输完成，所以要等待*/
    gpio_put(_LCD_CSN,true);
    return;
}