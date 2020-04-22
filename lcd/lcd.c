/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <string.h>
#include <unistd.h>
#include "lcd.h"
#include "nt35310.h"
#include "font.h"
#include "board_config.h"
#include "dmac.h"
static lcd_ctl_t lcd_ctl;
#include "sysctl.h"
#include "iomem.h"
#include "rtc.h"
#include "sdcard.h"
#include "ff.h"
#include "bsp.h"
#include "gbk.h"
#include "index.h"
#include "utils.h"
void lcd_polling_enable(void)
{
    lcd_ctl.mode = 0;
}

void lcd_interrupt_enable(void)
{
    lcd_ctl.mode = 1;
}

void lcd_init(void)
{
    uint8_t data = 0;

    tft_hard_init();
    /*soft reset*/
    tft_write_command(SOFTWARE_RESET);
    usleep(100000);
    /*exit sleep*/
    tft_write_command(SLEEP_OFF);
    usleep(100000);
    /*pixel format*/
    tft_write_command(PIXEL_FORMAT_SET);
    data = 0x55;
    tft_write_byte(&data, 1);
    lcd_set_direction(DIR_XY_LRUD);

    /*display on*/
    tft_write_command(DISPALY_ON);
    lcd_polling_enable();
}

void lcd_set_direction(lcd_dir_t dir)
{
#if !BOARD_LICHEEDAN
    dir |= 0x08;
#endif
    lcd_ctl.dir = dir;
    if (dir & DIR_XY_MASK)
    {
        lcd_ctl.width = LCD_Y_MAX - 1;
        lcd_ctl.height = LCD_X_MAX - 1;
    }
    else
    {
        lcd_ctl.width = LCD_X_MAX - 1;
        lcd_ctl.height = LCD_Y_MAX - 1;
    }

    tft_write_command(MEMORY_ACCESS_CTL);
    tft_write_byte((uint8_t *)&dir, 1);
}

void lcd_set_area(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    uint8_t data[4] = {0};

    data[0] = (uint8_t)(x1 >> 8);
    data[1] = (uint8_t)(x1);
    data[2] = (uint8_t)(x2 >> 8);
    data[3] = (uint8_t)(x2);
    tft_write_command(HORIZONTAL_ADDRESS_SET);
    tft_write_byte(data, 4);

    data[0] = (uint8_t)(y1 >> 8);
    data[1] = (uint8_t)(y1);
    data[2] = (uint8_t)(y2 >> 8);
    data[3] = (uint8_t)(y2);
    tft_write_command(VERTICAL_ADDRESS_SET);
    tft_write_byte(data, 4);

    tft_write_command(MEMORY_WRITE);
}

void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_set_area(x, y, x, y);
    tft_write_half(&color, 1);
}

void lcd_draw_char(uint16_t x, uint16_t y, char c, uint16_t color)
{
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t data = 0;

    for (i = 0; i < 16; i++)
    {
        data = ascii0816[c * 16 + i];
        for (j = 0; j < 8; j++)
        {
            if (data & 0x80)
                lcd_draw_point(x + j, y, color);
            data <<= 1;
        }
        y++;
    }
}

void lcd_draw_string(uint16_t x, uint16_t y, char *str, uint16_t color)
{
    while (*str)
    {
        lcd_draw_char(x, y, *str, color);
        str++;
        x += 8;
    }
}
void lcd_show_strings(uint16_t x1, uint16_t y1,char *str,uint16_t font_color, uint16_t bg_color)
{

    uint32_t lcd_string[40 * 16 * 8 / 2];//最多显示一行
    lcd_ram_draw_string(str,lcd_string,font_color ,bg_color );
    lcd_draw_picture(x1,y1, strlen(str)*8, 15, lcd_string);
    
}
void lcd_draw_ico(uint16_t x, uint16_t y, uint16_t w, uint16_t h,unsigned char *str, uint16_t color)
{


    // uint8_t i = 0;
    // uint8_t j = 0;
    // uint8_t z = 0;
    // uint8_t data = 0;
    // for (z = 0; z < h; z++){
    //      for (i = 0; i < w/8; i++)
    // {
    //     data = str[z*w/8+i];
    //     for (j = 0; j < 8; j++)
    //     {
    //         if (data & 0x80)
    //             lcd_draw_point(x + i*8+j, y+z, color);
    //         data <<= 1;
    //     }

    // }
    // }
    uint32_t lcd_ico= (uint32_t *)iomem_malloc(w*h*2);//申请空间  
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t z = 0;
    uint8_t data = 0;
    uint16_t *lcd_ico2=lcd_ico;
    for (z = 0; z < h; z++){
         for (i = 0; i < w/8; i++)
    {
        data = str[z*w/8+i];
        for (j = 0; j < 8; j++)
        {
            if(j%2==0)
            {
                if (data & 0x80)
                *(lcd_ico2+1)=color;
            else
                *(lcd_ico2+1)=WHITE;

            }else
            {
                if (data & 0x80)
                *(lcd_ico2-1)=color;
            else
                *(lcd_ico2-1)=WHITE;

            }
            data <<= 1;
            lcd_ico2++;
        }

    }


    }

 lcd_draw_picture(x,y, w, h, lcd_ico);
   iomem_free(lcd_ico);
}
void lcd_draw_16ico(uint16_t x, uint16_t y, uint16_t w, uint16_t h,unsigned char *str)
{
    uint8_t i = 0;
    uint8_t j = 0;
    uint16_t *st=str;
    for (i = 0; i < h; i++)
    {
        for (j = 0; j < w; j++)
         {
            lcd_draw_point(x+j, y+i, *st);
            st++;

         }

    }


}
void lcd_ram_draw_string(char *str, uint32_t *ptr, uint16_t font_color, uint16_t bg_color)
{
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t data = 0;
    uint8_t *pdata = NULL;
    uint16_t width = 0;
    uint32_t *pixel = NULL;

    width = 4 * strlen(str);
    while (*str)
    {
        pdata = (uint8_t *)&ascii0816[(*str) * 16];
        for (i = 0; i < 16; i++)
        {
            data = *pdata++;
            pixel = ptr + i * width;
            for (j = 0; j < 4; j++)
            {
                switch (data >> 6)
                {
                    case 0:
                        *pixel = ((uint32_t)bg_color << 16) | bg_color;
                        break;
                    case 1:
                        *pixel = ((uint32_t)bg_color << 16) | font_color;
                        break;
                    case 2:
                        *pixel = ((uint32_t)font_color << 16) | bg_color;
                        break;
                    case 3:
                        *pixel = ((uint32_t)font_color << 16) | font_color;
                        break;
                    default:
                        *pixel = 0;
                        break;
                }
                data <<= 2;
                pixel++;
            }
        }
        str++;
        ptr += 4;
    }
}

void lcd_clear(uint16_t color)
{
    uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;

    lcd_set_area(0, 0, lcd_ctl.width, lcd_ctl.height);
    tft_fill_data(&data, LCD_X_MAX * LCD_Y_MAX / 2);
}
void lcd_clear_xy(uint16_t x1, uint16_t y1,uint16_t x2, uint16_t y2,uint16_t color)
{
    uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;
    if(x2>=320)
        x2=319;
    if(y2>=240)
        y2=239;

    lcd_set_area(x1, y1, x2, y2);
    tft_fill_data(&data, (y2-y1+1) * (x2-x1+1) / 2);
}

void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t width, uint16_t color)
{
    uint32_t data_buf[640] = {0};
    uint32_t *p = data_buf;
    uint32_t data = color;
    uint32_t index = 0;

    data = (data << 16) | data;
    for (index = 0; index < 160 * width; index++)
        *p++ = data;

    lcd_set_area(x1, y1, x2, y1 + width - 1);
    tft_write_word(data_buf, ((x2 - x1 + 1) * width + 1) / 2, 0);
    lcd_set_area(x1, y2 - width + 1, x2, y2);
    tft_write_word(data_buf, ((x2 - x1 + 1) * width + 1) / 2, 0);
    lcd_set_area(x1, y1, x1 + width - 1, y2);
    tft_write_word(data_buf, ((y2 - y1 + 1) * width + 1) / 2, 0);
    lcd_set_area(x2 - width + 1, y1, x2, y2);
    tft_write_word(data_buf, ((y2 - y1 + 1) * width + 1) / 2, 0);
}

void lcd_draw_picture(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint32_t *ptr)
{
  
    if(width<=320&&height<=240)
    {
     lcd_set_area(x1, y1, x1 + width - 1, y1 + height - 1);

    tft_write_word(ptr, width * height / 2, lcd_ctl.mode ? 2 : 0);
    }

}

//画线
//x1,y1:起点坐标
//x2,y2:终点坐标
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint16_t t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, uRow, uCol;
    delta_x = x2 - x1; //计算坐标增量
    delta_y = y2 - y1;
    uRow = x1;
    uCol = y1;
    if (delta_x > 0)
        incx = 1; //设置单步方向
    else if (delta_x == 0)
        incx = 0; //垂直线
    else
    {
        incx = -1;
        delta_x = -delta_x;
    }
    if (delta_y > 0)
        incy = 1;
    else if (delta_y == 0)
        incy = 0; //水平线
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }
    if (delta_x > delta_y)
        distance = delta_x; //选取基本增量坐标轴
    else
        distance = delta_y;
    for (t = 0; t <= distance + 1; t++) //画线输出
    {
        lcd_draw_point(uRow, uCol, color); //画点
        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance)
        {
            xerr -= distance;
            uRow += incx;
        }
        if (yerr > distance)
        {
            yerr -= distance;
            uCol += incy;
        }
    }
}

//在指定位置画一个指定大小的圆
//(x,y):中心点
//r    :半径
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
    int a, b;
    int di;
    a = 0;
    b = r;
    di = 3 - (r << 1); //判断下个点位置的标志
    while (a <= b)
    {
        lcd_draw_point(x0 + a, y0 - b, color); //5
        lcd_draw_point(x0 + b, y0 - a, color); //0
        lcd_draw_point(x0 + b, y0 + a, color); //4
        lcd_draw_point(x0 + a, y0 + b, color); //6
        lcd_draw_point(x0 - a, y0 + b, color); //1
        lcd_draw_point(x0 - b, y0 + a, color);
        lcd_draw_point(x0 - a, y0 - b, color); //2
        lcd_draw_point(x0 - b, y0 - a, color); //7
        a++;
        //使用Bresenham算法画圆
        if (di < 0)
            di += 4 * a + 6;
        else
        {
            di += 10 + 4 * (a - b);
            b--;
        }
    }
}
void lcd_draw_picture_by_half(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint16_t *ptr)
{
    lcd_set_area(x1, y1, x1 + width - 1, y1 + height - 1);
    tft_write_half(ptr, width * height);
}
void draw_side(uint16_t x, uint16_t y,uint16_t color)
{
    uint16_t p[88];
    for(uint16_t i=2;i<80;i++)
    p[i]=color;
    p[0]=WHITE;
    p[1]=WHITE;
    p[2]=WHITE;
    p[3]=WHITE;
    p[84]=WHITE;
    p[85]=WHITE;
    p[86]=WHITE;
    p[87]=WHITE;
    lcd_draw_picture(x, y, 4, 22, p);

}
void draw_side_cartoon(uint16_t x1, uint16_t y1,uint16_t x2, uint16_t y2,uint16_t color)
{
    uint16_t x,y;
    x=x1;
   for(y=y1;y!=y2;y1>y2?y--:y++)
        {
            dmac_wait_idle ( SYSCTL_DMA_CHANNEL_0 );
            draw_side(x,y,DARKGREY);
            usleep(2000);
        }
    
}
void lcd_clear_slow(uint16_t x1, uint16_t y1,uint16_t x2, uint16_t y2,uint16_t color,uint16_t mode)
{
    float w=x2-x1,h=y2-y1;
    float k=w/h;
    uint16_t i;
    uint32_t data = ((uint32_t)color << 16)|(uint32_t)color;
    if(mode)
    {
        for(i=0;i<=(uint16_t)h;i++)
        {
        usleep(2*1000);
        lcd_clear_xy(x1,y1,(uint16_t)(k*i)+x1,y1+i,color);
        }

    }else
    {
        for(i=0;i<=(uint16_t)h/2;i++)
        {
        usleep(2000);
        lcd_clear_xy(x1,y1,(uint16_t)(k*i)+x1+1,y1+i+1,color);
        usleep(2000);
        lcd_clear_xy(x2-(uint16_t)(k*i)-1,y1+1,x2,y1+i,color);
        usleep(2000);
        lcd_clear_xy(x1,y2-i-1,(uint16_t)(k*i)+x1+1,y2,color);
        usleep(2000);
        lcd_clear_xy(x2-(uint16_t)(k*i)-1,y2-i-1,x2,y2,color);

        }
        
    }
    
    // lcd_set_area(x1, y1, x2, y2);
    // tft_fill_data(&data, (y2-y1+1) * (x2-x1+1) / 2);

}

void lcd_ico_move(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t w, uint16_t h,char *str,uint16_t mode)//1上下移动0左右移动
{
    uint16_t x,y;
    lcd_clear_xy(x1,y1,x1+w-1, y1+h-1,WHITE);
    if(mode)//上下移动
    {
        for(y=y1;y!=y2;y1>y2?y--:y++)
        {
            dmac_wait_idle ( SYSCTL_DMA_CHANNEL_0 );
            lcd_draw_picture(x1,y,w,h,(uint32_t *)str);    
            usleep(600);
            if(y1>y2)
            lcd_clear_xy(x1,y+h-2,x1+w,y+h,WHITE);
            else
            lcd_clear_xy(x1,y,x1+w,y-2,WHITE);
           
        }

    }
    else
    {
        for(x=x1;x!=x2;x1>x2?x--:x++)
        {
            dmac_wait_idle ( SYSCTL_DMA_CHANNEL_0 );
            lcd_draw_picture(x,y1,w,h,(uint32_t *)str);
           // draw_side(x,y,DARKGREY);
            usleep(600);
            if(x1>x2)
            lcd_clear_xy(x,y1,x+4,y1+h,WHITE);
            else
            lcd_clear_xy(x-4,y1,x,y1+h,WHITE);
          
        }

    }


}
void lcd_ico_appear(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t w, uint16_t h,char *str)
{
    
    uint16_t *p = (uint16_t *)iomem_malloc(w*h*2);
    rsdwbuff(str,p);
    uint16_t j;
    for(j=1;j<(h+1);j++)
    {
        lcd_draw_picture(x1,y1-j,w,j,(uint32_t *)p);
        usleep(300);

    }
    lcd_ico_move(x1, y1-j, x2, y2,  w, h,p,1);
    iomem_free(p);

}
void rsdwlcd(uint16_t x, uint16_t y, uint16_t w, uint16_t h,char *str)
{
    uint32_t *mubiao;
    FIL file;
    FRESULT ret = FR_OK;
    //char *path = "bin/ico/photoico.bin";
    char *path = str;
    printf("/*******sd read sz.bin*******/\n");
    FILINFO v_fileinfo;
    uint32_t v_ret_len = 0;
    if((ret = f_stat(path, &v_fileinfo)) == FR_OK)
    {
        printf("%s length is %lld\n", path, v_fileinfo.fsize);
    }
    else
    {
        printf("%s fstat err [%d]\n", path, ret);
    }
    if ((ret = f_open(&file, path, FA_READ)) != FR_OK) {
        printf("open file %s err[%d]\n", path, ret);
        lcd_show_strings(100,100,"open file error",BLACK,WHITE);
        while(1);
    }
    else
    {
        printf("Open %s ok\n", path);
         mubiao = (uint32_t *)iomem_malloc(v_fileinfo.fsize);//脙鈥懊兟冣∶兟偮棵冣⒚偮济兟                                 

                     
        ret = f_read(&file, (void *)mubiao, v_fileinfo.fsize, &v_ret_len);

        if(ret != FR_OK)
        {
            printf("Read %s err[%d]\n", path, ret);
             lcd_show_strings(100,100,"read file error",BLACK,WHITE);
            while(1);
        }
        else
        {
           // printf("Read :> %s %d bytes lenth\n", (char *)mubiao, v_ret_len);
             lcd_draw_picture(x,y, w, h,mubiao);
           
            // lcd_ico_appear(160, 239, x, y, 56, 56,mubiao);
 
        }
          iomem_free(mubiao);

    }
    f_close(&file);

}
void rsdwbuff(char *str,uint16_t *buff)
{
    FIL file;
    FRESULT ret = FR_OK;
    //char *path = "bin/ico/photoico.bin";
    char *path = str;
    printf("/*******sd read sz.bin*******/\n");
    FILINFO v_fileinfo;
    uint32_t v_ret_len = 0;
    if((ret = f_stat(path, &v_fileinfo)) == FR_OK)
    {
        printf("%s length is %lld\n", path, v_fileinfo.fsize);
    }
    else
    {
        printf("%s fstat err [%d]\n", path, ret);
    }
    if ((ret = f_open(&file, path, FA_READ)) != FR_OK) {
        printf("open file %s err[%d]\n", path, ret);
        lcd_show_strings(100,100,"open file error",BLACK,WHITE);
        while(1);
    }
    else
    {
        printf("Open %s ok\n", path);
        ret = f_read(&file, (void *)buff, v_fileinfo.fsize, &v_ret_len);

        if(ret != FR_OK)
        {
            printf("Read %s err[%d]\n", path, ret);
            lcd_show_strings(100,100,"read file error",BLACK,WHITE);
            while(1);
        }

    }
    f_close(&file);


}
void ico_disappear(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t w, uint16_t h,char *str)
{
    uint16_t *p = (uint16_t *)iomem_malloc(w*h*2);
    rsdwbuff(str,p);
    uint16_t i,j,z;
    lcd_ico_move(x1, y1, x2, y2,  w, h,p,1);

    for(j=0;j<h;j+=2)
    {
        lcd_draw_picture(x2,y2,w,h-j,(uint32_t *)(p+w*j));
        usleep(1500);

    }
    iomem_free(p);
}
void ico_disappear_rl(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t w, uint16_t h,char *str,uint8_t mode)
{
    uint16_t *p = (uint16_t *)iomem_malloc(w*h*2);
    rsdwbuff(str,p);
    uint16_t *c;
    int16_t i,j,z;
    
    if(mode)//右移动
    {
        lcd_ico_move(x1, y1, x2-w, y2,  w, h,p,0);//移动
        for(i=w;i>0;i-=2)
        {
            uint16_t *q = (uint16_t *)iomem_malloc(i*h*2);
            c=q;
            for(j=0;j<h;j++)
                for(z=0;z<i;z++)
                {
                    *c=p[z+j*w];
                    c++;
                }
            lcd_draw_picture(x2-i,y2,i,h,q);
            usleep(1000);
            lcd_clear_xy(x2-i-2,y2,x2-i+1,y2+h,WHITE);
            iomem_free(q);
        }
    }
    else
    {
        lcd_ico_move(x1, y1, x2, y2,  w, h,p,0);//移动
        for(i=w;i>0;i-=2)
        {
            uint16_t *q = (uint16_t *)iomem_malloc(i*h*2);
            c=q;
            for(j=0;j<h;j++)
                for(z=w-i;z<w;z++)
                {
                    *c=p[z+j*w];
                    c++;
                }
            lcd_draw_picture(x2,y2,i,h,q);
            usleep(1000);
            iomem_free(q);
        }
    }
    // for(j=0;j<h;j+=2)
    // {
    //     lcd_draw_picture(x2,y2,w,h-j,(uint32_t *)(p+w*j));
    //     usleep(6000);

    // }
    iomem_free(p);
}