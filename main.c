#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include "sysctl.h"
#include "plic.h"
#include "utils.h"
#include "gpiohs.h"
#include "fpioa.h"
#include "lcd.h"
#include "nt35310.h"
#include "dvp.h"
#include "ov2640.h"
#include "uarths.h"
#include "kpu.h"
#include "region_layer.h"
#include "image_process.h"
#include "board_config.h"
#include "w25qxx.h"
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#define INCBIN_PREFIX
#include "incbin.h"
#include "iomem.h"
#include "wifi.h"
#include "apple.h"
#include "wifioff.h"
#include "rtc.h"
#include "sdcard.h"
#include "ff.h"
#include "bsp.h"
#include "gbk.h"
#include "sd.h"
#include "jinclude.h"
#include "jcapi.h"
#include "index.h"
#define RGB565_RED      0xf800
#define RGB565_GREEN    0x07e0
#define RGB565_BLUE     0x001f
#define PLL0_OUTPUT_FREQ 800000000UL
#define PLL1_OUTPUT_FREQ 400000000UL
#define ANCHOR_NUM 5
#define  LOAD_KMODEL_FROM_FLASH  1
#if LOAD_KMODEL_FROM_FLASH
#define KMODEL_SIZE (3100 * 1024)
uint8_t *model_data;
#else
INCBIN(model, "test.kmodel");
#endif

void get_date_time();

volatile uint32_t g_ai_done_flag;
volatile uint8_t g_dvp_finish_flag;
static image_t kpu_image;
static uint32_t *g_lcd_gram0,*g_lcd_gram1;
volatile uint8_t g_ram_mux;
kpu_model_context_t face_detect_task;
static region_layer_t face_detect_rl;
static yn_info_t apple_yn;
static obj_info_t face_detect_info;
static float anchor[ANCHOR_NUM * 2] = {1.16,1.56, 1.47,2.19, 2.08,2.85, 2.79,3.84, 4.21,4.63};
unsigned char lcdown=0;//

/***rgb2jpg***/
unsigned char JPG_enc_buf[20000];//jpeg ÃŠÃ¤Â³Ã¶Â»ÂºÂ³Ã¥
unsigned int pt_buf;//Â»ÂºÂ³Ã¥Ã‡Ã¸Ã–Â¸Ã•Ã«
jpeg_compress_info info1;
JQUANT_TBL  JQUANT_TBL_2[2];
JHUFF_TBL  JHUFF_TBL_4[4];
unsigned char dcttab[3][512];//
unsigned char hang[672];//jpeg ÃŠÃ¤Â³Ã¶Â»ÂºÂ³Ã¥
volatile unsigned char inbuf_buf[224*224*3];
////ÃŠÃ¤ÃˆÃ«Ã‡Ã¸Â»ÂºÂ³Ã¥,Ã•Ã¢Â¸Ã¶ÃŠÃ‡ÃŽÂªÂ¿Ã­Â¶Ãˆ240ÂµÃ„ÃÂ¼Ã†Â¬Â´Ã³ÃÂ¡Ã‰Ã¨Ã–ÃƒÂµÃ„Â£Â¬ÃˆÃ§Â¹Ã»Ã’ÂªÂ¸Ã¼Â´Ã³ÂµÃ„ÃÂ¼Ã†Â¬Â£Â¬Â¾ÃÃÃ¨Ã’ÂªÂ¸Ã¼Â´Ã³ÂµÃ„Â»ÂºÂ³Ã¥11520 = 240x16x3


typedef struct _time_conut
{
    uint64_t time_last;
    uint64_t time_now;
    int time_count;
    int fs;
} time_conut_t;
time_conut_t fs_count;

static void ai_done(void *ctx)
{
    g_ai_done_flag = 1;
}

static int on_irq_dvp(void* ctx)
{
    if (dvp_get_interrupt(DVP_STS_FRAME_FINISH))
    {
        /* switch gram */
        dvp_set_display_addr(g_ram_mux ? (uint32_t)g_lcd_gram0 : (uint32_t)g_lcd_gram1);

        dvp_clear_interrupt(DVP_STS_FRAME_FINISH);
        g_dvp_finish_flag = 1;
    }
    else
    {
        if(g_dvp_finish_flag == 0)
            dvp_start_convert();
        dvp_clear_interrupt(DVP_STS_FRAME_START);
    }

    return 0;
}

static void io_mux_init(void)
{

    /* SD card */
    fpioa_set_function(27, FUNC_SPI1_SCLK);
    fpioa_set_function(28, FUNC_SPI1_D0);
    fpioa_set_function(26, FUNC_SPI1_D1);
    fpioa_set_function(29, FUNC_GPIOHS7);

    /* Init DVP IO map and function settings */
    fpioa_set_function(42, FUNC_CMOS_RST);
    fpioa_set_function(44, FUNC_CMOS_PWDN);
    fpioa_set_function(46, FUNC_CMOS_XCLK);
    fpioa_set_function(43, FUNC_CMOS_VSYNC);
    fpioa_set_function(45, FUNC_CMOS_HREF);
    fpioa_set_function(47, FUNC_CMOS_PCLK);
    fpioa_set_function(41, FUNC_SCCB_SCLK);
    fpioa_set_function(40, FUNC_SCCB_SDA);

    /* Init SPI IO map and function settings */
    fpioa_set_function(38, FUNC_GPIOHS0 + DCX_GPIONUM);
    fpioa_set_function(36, FUNC_SPI0_SS3);
    fpioa_set_function(39, FUNC_SPI0_SCLK);
    fpioa_set_function(37, FUNC_GPIOHS0 + RST_GPIONUM);

    sysctl_set_spi0_dvp_data(1);

}
/*Â»Ã±ÃˆÂ¡ÃŠÂ±Â¼Ã¤*/
void get_date_time()
{
    if(!lcdown)
    {
    char time[25];
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    rtc_timer_get(&year, &month, &day, &hour, &minute, &second);
    sprintf(time,"%02d:%02d:%02d", hour, minute, second);
    dmac_wait_idle ( SYSCTL_DMA_CHANNEL_0 );
    lcd_show_strings(120,0,time,BLACK ,WHITE);
    uint32_t i= get_free_heap_size();
    float nouse=i/1024.0/1024;
    sprintf(time,"%0.1fm", nouse);
    dmac_wait_idle ( SYSCTL_DMA_CHANNEL_0 );
    lcd_show_strings(230,0,time,BLUE ,WHITE);

    }

 

}
/*ÃŠÂ±Â¼Ã¤Ã–ÃÂ¶Ã*/
int on_timer_interrupt()
{
 
    get_date_time();
   
    return 0;
}



static void io_set_power(void)
{

        /* Set dvp and spi pin to 1.8V */
        sysctl_set_power_mode(SYSCTL_POWER_BANK6, SYSCTL_POWER_V18);
        sysctl_set_power_mode(SYSCTL_POWER_BANK7, SYSCTL_POWER_V18);

}

static void draw_edge(uint32_t *gram, obj_info_t *obj_info, uint32_t index, uint16_t color)
{
    uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;
    uint32_t *addr1, *addr2, *addr3, *addr4, x1, y1, x2, y2;
    uint8_t x,y;
    uint16_t *addr5;
    x1 = obj_info->obj[index].x1;
    y1 = obj_info->obj[index].y1;
    x2 = obj_info->obj[index].x2;
    y2 = obj_info->obj[index].y2;

    if (x1 <= 0)
        x1 = 1;
    if (x2 >= 223)
        x2 = 223;
    if (y1 <= 0)
        y1 = 1;
    if (y2 >= 223)
        y2 = 223;
    //lcd_draw_string(x1,y1, "apple", GREEN);
    addr1 = gram + (224 * y1 + x1) / 2;
    addr2 = gram + (224 * y1 + x2 - 8) / 2;
    addr3 = gram + (224 * (y2 - 1) + x1) / 2;
    addr4 = gram + (224 * (y2 - 1) + x2 - 8) / 2;
    for (uint32_t i = 0; i < 4; i++)
    {
        *addr1 = data;
        *(addr1 + 112) = data;
        *addr2 = data;
        *(addr2 + 112) = data;
        *addr3 = data;
        *(addr3 + 112) = data;
        *addr4 = data;
        *(addr4 + 112) = data;
        addr1++;
        addr2++;
        addr3++;
        addr4++;
    }
    addr1 = gram + (224 * y1 + x1) / 2;
    addr2 = gram + (224 * y1 + x2 - 2) / 2;
    addr3 = gram + (224 * (y2 - 8) + x1) / 2;
    addr4 = gram + (224 * (y2 - 8) + x2 - 2) / 2;
    for (uint32_t i = 0; i < 8; i++)
    {
        *addr1 = data;
        *addr2 = data;
        *addr3 = data;
        *addr4 = data;
        addr1 += 112;
        addr2 += 112;
        addr3 += 112;
        addr4 += 112;
    }
/*Â»Â­Ã–ÃÃÃ„Ã‡Ã¸Ã“Ã²*/
    addr1 = gram + (224 * 112 + 100) / 2;
    for (uint32_t i = 0; i < 13; i++)
    {
        *addr1 = data;
        *(addr1 + 112) = data;
        addr1 ++;

    }
       addr1 = gram + (224 * 100 + 112) / 2;
    for (uint32_t i = 0; i < 25; i++)
    {
        *addr1 = data;   
         addr1 += 112;
       
    }
    /*Â»Â­Ã†Â»Â¹Ã»Ã–ÃÃÃ„*/
    x=(x1+x2)/2;
    y=(y1+y2)/2;
 addr5 = gram + (224 * (y-3)+ (x-3)) / 2;
    for (uint32_t i = 0; i < 7; i++)
    {
        *addr5 = data; 
        *(addr5 + 1) = data;
         addr5 += 225;
       
    }
    addr5 = gram + (224 * (y-3)+ (x+3)) / 2;
    for (uint32_t i = 0; i < 7; i++)
    {
        *addr5 = data;  
        *(addr5 + 1) = data; 
         addr5 += 223;
       
    }

}

void lcd_show_init()
{
     /* LCD init */
    printf("LCD init\n");
    lcd_init();
    lcd_set_direction(DIR_YX_RLDU);
    lcd_clear(WHITE);
    //lcd_draw_ico(0,0,16,16,gImage_wifi,BLACK);
    lcd_draw_ico(0,0,16,16,gImage_wifioff,BLACK);
   // lcd_draw_ico(240,40,64,64,gImage_link,BLACK);
    lcd_draw_ico(240,16,64,64,gImage_apple,RED);

}

uint16_t app_count=0;//Ã†Â»Â¹Ã»ÃŠÃ½Ã„Â¿



void get_target(obj_info_t *obj_info, uint32_t index)
{
    char sfs[15];
    uint8_t x1,x2,y1,y2,w,h,mx,my;
    uint32_t area;
    apple_yn.apple[index].an_count=0;
    x1=obj_info->obj[index].x1;
    x2=obj_info->obj[index].x2;
    y1=obj_info->obj[index].y1;
    y2=obj_info->obj[index].y2;
             if(x2>223)
             {
                x2=223;
             }
                
             if(y2>223)
             {
                y2=223;  
             }
               
                w = x2-x1;
                h = y2-y1;
                /*Â¼Ã†Ã‹Ã£Ã–ÃÂµÃ£*/
                mx = (x2+x1)/2;
                my= (y2+y1)/2;
                if(index<2)
                {
                sprintf(sfs,"W:%3d h:%3d", w,h);
                lcd_show_strings(230,90+32*index,sfs,BLACK ,WHITE);
                /*Â¼Ã†Ã‹Ã£ÃƒÃ¦Â»Ã½*/
                 area=h*w;
                sprintf(sfs,"area:%d", area);
                lcd_show_strings(230,106+32*index,sfs,BLACK ,WHITE);
                }

                /*Ã†Â»Â¹Ã»Â¼Ã†ÃŠÃ½*/
                if(abs(mx-112)<12&&abs(my-112)<12)
                {
                    apple_yn.apple[index].app_disappear=0;
                    apple_yn.apple[index].ay_count++;
                    if(apple_yn.apple[index].ay_count==3&&apple_yn.apple[index].app_exist==0)//Â¼Ã¬Â²Ã¢ÂµÂ½Ã†Â»Â¹Ã»
                    {
                        sysctl_disable_irq();//Â½Ã»Ã“ÃƒÃÂµÃÂ³Ã–ÃÂ¶Ã
                        app_count++;
                        apple_yn.apple[index].app_exist=1;
                        lcd_clear_xy(0,16,223,239,WHITE);
                        lcdown=1;
                        for(uint8_t i=y1;i<=y2;i++)
                        {

                           lcd_draw_picture(x1,i+16, w+1, 1, 
                            (uint32_t *)(g_ram_mux ? g_lcd_gram0 : g_lcd_gram1)+(224*i+x1)/2);

                        }
                        lcdown=0;
                       //  mubiao = (uint32_t *)iomem_malloc((w+1)*(h+1)*2);//Ã‰ÃªÃ‡Ã«Â¿Ã•Â¼Ã¤                  
                       //  for(uint8_t i=0;i<=h;i++)
                       //  {
                       //  dmac_wait_idle ( SYSCTL_DMA_CHANNEL_1 );//Ã‘Â¡Ã”Ã±ÃÂ¨ÂµÃ€Ã’Â»
                       //  dmac_set_single_mode ( SYSCTL_DMA_CHANNEL_1 , (uint32_t *)(g_ram_mux ? g_lcd_gram0 : g_lcd_gram1)
                       //   +(224*(y1+i)+x1)/2 ,
                       //  mubiao+(w+1)*i/2,DMAC_ADDR_INCREMENT , DMAC_ADDR_INCREMENT , DMAC_MSIZE_4 , DMAC_TRANS_WIDTH_32 , (w+1)/2);
                       // }

                       //  lcd_draw_picture(x1,y1+16, w+1, h+1, 
                       //      mubiao);

                       //   iomem_free(mubiao);


                       msleep(500);
                        sysctl_enable_irq();//Â¿ÂªÃ†Ã´Ã–ÃÂ¶Ã
                    }

                }
                else
                {
                    apple_yn.apple[index].app_disappear++;
                    if(apple_yn.apple[index].app_disappear==3){
                    apple_yn.apple[index].ay_count=0;
                    apple_yn.apple[index].app_exist=0;
                    }
                  
                }
            sprintf(sfs,"number:%4d", app_count);
            lcd_show_strings(230,220,sfs,BLACK ,WHITE);
}


void All_init()//Â³ÃµÃŠÂ¼Â»Â¯Ã’Â»Ã‡ÃÃ‚Ã’Ã†ÃŸÂ°Ã‹Ã”Ã£ÂµÃ„
{
        /* ÃŠÂ±Ã–Ã“Â³ÃµÃŠÂ¼Â»Â¯ */
    sysctl_pll_set_freq(SYSCTL_PLL0, PLL0_OUTPUT_FREQ);
    sysctl_pll_set_freq(SYSCTL_PLL1, PLL1_OUTPUT_FREQ);
    sysctl_clock_enable(SYSCTL_CLOCK_AI);
    uarths_init();
    io_set_power();
    io_mux_init();
    plic_init();
    rtc_init();
    lcd_show_init();
    
    /* flash init */
    printf("flash init\n");
    lcd_show_strings(0,32,"flash init",BLACK,WHITE);
    msleep(100);
    w25qxx_init(3, 0);
    w25qxx_enable_quad_mode();
#if LOAD_KMODEL_FROM_FLASH
    model_data = (uint8_t *)malloc(KMODEL_SIZE + 255);
    uint8_t *model_data_align = (uint8_t *)(((uintptr_t)model_data+255)&(~255));
    w25qxx_read_data(0x300000, model_data_align, KMODEL_SIZE, W25QXX_QUAD_FAST);
#else
    uint8_t *model_data_align = model_data;
#endif
 
    //lcd_draw_ico(30,30,192,192,gImage_erweima,RED);   
    g_lcd_gram0 = (uint32_t *)iomem_malloc(224*224*2);
    g_lcd_gram1 = (uint32_t *)iomem_malloc(224*224*2);
    /* DVP init */
    printf("DVP init\n");
    lcd_show_strings(0,48,"DVP init" ,BLACK,WHITE);
    msleep(100);
    dvp_init(8);
    dvp_set_xclk_rate(24000000);
    dvp_enable_burst();
    dvp_set_output_enable(0, 1);
    dvp_set_output_enable(1, 1);
    dvp_set_image_format(DVP_CFG_RGB_FORMAT);
    dvp_set_image_size(224, 224);
    ov2640_init();
    kpu_image.pixel = 3;
    kpu_image.width = 224;
    kpu_image.height = 224;
    image_init(&kpu_image);
    dvp_set_ai_addr((uint32_t)kpu_image.addr, (uint32_t)(kpu_image.addr + 224 * 224), (uint32_t)(kpu_image.addr + 224 * 224 * 2));
    dvp_set_display_addr((uint32_t)g_lcd_gram0);
    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
    dvp_disable_auto();
    /* DVP interrupt config */
    printf("DVP interrupt config\n");
    lcd_show_strings(0,64,"DVP interrupt config" ,BLACK,WHITE);
    msleep(100);
    plic_set_priority(IRQN_DVP_INTERRUPT, 1);
    plic_irq_register(IRQN_DVP_INTERRUPT, on_irq_dvp, NULL);
    plic_irq_enable(IRQN_DVP_INTERRUPT);


    if(sdcard_init())
    {
        printf("SD card err\n");
        lcd_show_strings(0,80,"fat32 init error" ,BLACK,WHITE);
        while(1);
    }
    lcd_show_strings(0,80,"fat32 init" ,BLACK,WHITE);
        if(fs_init())
    {
        printf("FAT32 err\n");

        while(1);
    }
    /* init face detect model */
    if (kpu_load_kmodel(&face_detect_task, model_data_align) != 0)
    {
        printf("\nmodel init error\n");
        lcd_show_strings(0,80,"model init error" ,BLACK,WHITE);
        while (1);
    }
    face_detect_rl.anchor_number = ANCHOR_NUM;
    face_detect_rl.anchor = anchor;
    face_detect_rl.threshold = 0.7;
    face_detect_rl.nms_value = 0.3;
    region_layer_init(&face_detect_rl, 7, 7, 30, kpu_image.width, kpu_image.height);
    fs_count.time_last = sysctl_get_time_us();
    fs_count.time_now = sysctl_get_time_us();
    fs_count.time_count = 0;
    fs_count.fs=0;

}
void fs_show()
{   
    char sfs[15];
    fs_count.time_count ++;
    if(fs_count.time_count % 10 == 0)
    {
        fs_count.time_now = sysctl_get_time_us();
        fs_count.fs=(int)10000000/(fs_count.time_now - fs_count.time_last);
        sprintf(sfs,"FS:%d", fs_count.fs);
        fs_count.time_last = fs_count.time_now;
       get_date_time();//Ã‹Â¢ÃÃ‚ÃŠÂ±Â¼Ã¤
        lcd_show_strings(280,0,sfs,BLACK ,WHITE);

        }


}

void menu(void)
{
    lcdown=1;
    fun_main(0);
    lcdown=0;
    fun1();

 
  
    while (1);
}
int main(void)
{    
    All_init();

    sysctl_enable_irq();
    rtc_timer_set(2020, 4, 13, 19, 60, 50);
    rtc_tick_irq_register(
        false,
        RTC_INT_SECOND,
        on_timer_interrupt,
        NULL,
        1
    );

    printf("System start\n");
    lcd_show_strings(0,96,"System start" ,BLACK,WHITE);
    msleep(300);
    menu();

   
}
