/*

*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include "lcd.h"
#include "dmac.h"
#include "sysctl.h"
#include "iomem.h"
#include "rtc.h"
#include "sdcard.h"
#include "ff.h"
#include "bsp.h"
#include "gbk.h"
#include "index.h"
#include "utils.h"
#include "nt35310.h"
#include "sd.h"
#include "jiantou.h"
#include  "clock.h"
#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3
#define SURE 4
#define UNFIX 5
struct
{   
    uint8_t wifi;
    uint8_t index;
    uint8_t key;
    uint8_t main_index;
    uint8_t last_index;
    void (*index_fun)();
}sys_info;
typedef struct _clock_time_
{   
    uint16_t middle_x;
    uint16_t middle_y;
    uint16_t hour_start[6];
    uint16_t hour_end[6];
    uint16_t minute_start[4];
    uint16_t minute_end[4];
    uint16_t second_start[2];
    uint16_t second_end[2];
}clock_info_t;
  clock_info_t clock_info={
        120,120,{99,100,100,100,101,100},{100,60,100,60,100,60},
        {100,100,101,100},{100,50,100,50},{100,100},{100,40}

    };
double_t angle_covert(uint16_t angle)
{
    double_t pi=3.14159265;
    return (pi*angle/180);
}
void rotate(int16_t mx,int16_t my, int16_t  x1,int16_t y1, int16_t * x,int16_t * y, uint16_t angle)
{

    *x=(int16_t)(mx+(x1-mx)*cos(angle_covert(angle))-(y1-mx)*sin(angle_covert(angle))) ;
    *y=(int16_t)(my+(x1-mx)*sin(angle_covert(angle))+(y1-mx)*cos(angle_covert(angle)));

}
void draw_line_ram(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint16_t *str,uint16_t w,uint16_t color)
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
        //lcd_draw_point(uRow, uCol, color); //画点
        *(str+uRow+uCol*w)=color;
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
void write_hour(uint16_t ho,uint16_t color,uint32_t *mubiao)
{
            uint16_t hour_start[6];
            uint16_t hour_end[6];
      /*时针变换*/
            for(uint16_t i =0;i<3;i++)//时针 360/12=30 每次旋转30度
            {
                ho=ho%12;
                uint16_t angle=ho*30;
                rotate(100,100,clock_info.hour_start[i*2],clock_info.hour_start[i*2+1],hour_start+i*2,hour_start+i*2+1,angle);
                rotate(100,100,clock_info.hour_end[i*2],clock_info.hour_end[i*2+1],hour_end+i*2,hour_end+i*2+1,angle);
            }
              for(uint16_t i =0;i<3;i++)
            draw_line_ram(hour_end[i*2],hour_end[i*2+1],hour_start[i*2],hour_start[i*2+1],mubiao,200,color);

}
void write_minute(uint16_t mi,uint16_t color,uint32_t *mubiao)
{
            uint16_t minute_start[4];
            uint16_t minute_end[4];
               /*分针变换*/
            for(uint16_t i =0;i<2;i++)//分针 360/60=6 每次旋转6度
            {
                uint16_t angle=mi*6;
                rotate(100,100,clock_info.minute_start[i*2],clock_info.minute_start[i*2+1],minute_start+i*2,minute_start+i*2+1,angle);
                rotate(100,100,clock_info.minute_end[i*2],clock_info.minute_end[i*2+1],minute_end+i*2,minute_end+i*2+1,angle);
            }
              for(uint16_t i =0;i<2;i++)
            draw_line_ram(minute_start[i*2],minute_start[i*2+1],minute_end[i*2],minute_end[i*2+1],mubiao,200,color);
}
void write_second(uint16_t sc,uint16_t color,uint32_t *mubiao)
{
     uint16_t second_start[2];
     uint16_t second_end[2];
     uint16_t angle=sc*6;
    rotate(100,100,clock_info.second_end[0],clock_info.second_end[1],second_end,second_end+1,angle);
      draw_line_ram(second_end[0],second_end[1],100,100,mubiao,200,color);
}

index_t fun[FUNNUM]=
{
    {2,1,3,0,3,0,(*fun_index)},//
    {0,2,4,1,4,1,(*fun_index)},
    {1,0,5,2,5,2,(*fun_index)},
    {3,3,3,0,3,0,(*fun_second)},
    {4,4,4,0,4,1,(*fun_second)},
    {5,5,5,0,5,2,(*fun_second)},
};
main_index_t index_ico[3]=
{
    {"bin/ico/wifiico.bin", "bin/ico/connetico.bin","网络设置",244,26,60,40,56,56,120,120},
    {"bin/ico/photoico.bin","bin/ico/fruit.bin",    "水果识别", 240,90,60,40,64,64,120,120},
    {"bin/ico/sysico.bin",  "bin/ico/bigsz.bin",   "系统设置",    240,160,60,40,56,56,120,120},

};
side_t side_xy[3]=
{
    {308,46},{308,116},{308,180},
};
void fun1_1()
{
     lcd_clear_slow(0, 16,240, 240,WHITE,0);
    Show_Str_Mid(60,50,"网络未连接",16,140);
    Show_Str_Mid_CL(60,120,"软件配网",16,140,BLUE);
    Show_Str_Mid(60,150,"小程序配网",16,140);
    usleep(1000000);
    fun1_2();
    usleep(1000000);
   fun1_1_1();
}
void fun1_1_1()
{
          lcd_clear_slow(0, 16,240, 240,WHITE,0);
      lcd_ico_appear(20, 239, 20,20,200,200,"bin/ico/erico.bin");

}



void fun1_2()
{

    Show_Str_Mid(60,120,"软件配网",16,140);
    Show_Str_Mid_CL(60,150,"小程序配网",16,140,BLUE);

}

void ico_dis(uint8_t index)
{
    uint8_t clock_flag=0;
    for(uint8_t i=0;i<3;i++)
    {
        if(i!=index)
        {
            ico_disappear_rl(index_ico[i].index_x, index_ico[i].index_y, 320, 
            index_ico[i].index_y, index_ico[i].index_w,index_ico[i].index_h,index_ico[i].index_ico,1);

        }
    }
    if(index==4)
    {
         index=0;
         clock_flag=1;
    }
   

    lcd_clear_slow(0,180,240,204,WHITE,1);
     ico_disappear_rl(index_ico[index].big_x, index_ico[index].big_y, 0, 
     index_ico[index].big_y, index_ico[index].big_w,index_ico[index].big_h,index_ico[index].big_ico,0);
     
    uint16_t *p = (uint16_t *)iomem_malloc(index_ico[index].index_w*index_ico[index].index_h*2);
    rsdwbuff(index_ico[index].index_ico,p);
     if(index)
    {
    lcd_ico_move(index_ico[index].index_x, index_ico[index].index_y, index_ico[index].index_x, 
     26,  index_ico[index].index_w, index_ico[index].index_h,p,1);//
    }
     iomem_free(p);
     lcd_clear_xy(308,0,312,239,WHITE);
     if(!clock_flag)
     Show_Str_Mid(240,100,index_ico[index].text,16,60);
}

void fun_second()
{
    uint8_t index=fun[ sys_info.index].main_index;//

    ico_dis(index);
    switch (index)
    {
        case 0:
        fun1_1();
        break;
        case 1:
   
        break;
        case 2:
          
        break;
    }

}
void fun_main(uint8_t index)
{
	usleep(10000);
    lcd_clear_slow(0, 16,320, 240,WHITE,0);
    //rsdwlcd(244,26,56,56,"bin/ico/wifiico.bin");
    lcd_ico_appear(240, 239, 244,26,56,56,"bin/ico/wifiico.bin");
    lcd_ico_appear(240, 239, 240,90,64,64,"bin/ico/photoico.bin");
    lcd_ico_appear(240, 239, 244,160,56,56,"bin/ico/sysico.bin");
    //rsdwlcd(240,90,64,64,"bin/ico/photoico.bin");
    //rsdwlcd(244,160,56,56,"bin/ico/sysico.bin");
    lcd_draw_line(0, 15, 319,15, GREENYELLOW);
    //rsdwlcd(20,16,200,200,"bin/ico/erico.bin");
    draw_side_cartoon(308,220,side_xy[index].x,side_xy[index].y,DARKGREY);//
    /**/
    lcd_ico_appear(60, 239, index_ico[index].big_x,index_ico[index].big_y,index_ico[index].big_w,index_ico[index].big_h,index_ico[index].big_ico);
    Show_Str_Mid(0,180,index_ico[index].text,24,240);
    // Show_Str(0,180,240,32,index_ico[index].text,32,1);
}
void switch_key()
{
    sys_info.last_index=sys_info.index;
    sys_info.main_index=fun[sys_info.index].main_index;
    printf("%d\n",sys_info.index);
    switch(sys_info.key)
    {
        case UP:        
        sys_info.index=fun[sys_info.index].up;
        break;
        case DOWN:       
        sys_info.index=fun[sys_info.index].down;      
        break;
        case LEFT:
        sys_info.index=fun[sys_info.index].left;       
        break;
        case RIGHT:
        sys_info.index=fun[sys_info.index].right;      
        break;
        case SURE:    
        sys_info.index=fun[sys_info.index].sure;
        break;

    }
    sys_info.key=UNFIX;
    sys_info.index_fun=fun[sys_info.index].indexing;
    (*sys_info.index_fun)();
}
void fun_index()
{
    
   if(sys_info.last_index<3)
   {
     draw_side_cartoon(side_xy[sys_info.last_index].x,side_xy[sys_info.last_index].y,
    side_xy[sys_info.index].x,side_xy[sys_info.index].y,DARKGREY);//
    lcd_clear_slow(0,180,240,204,WHITE,1);
    ico_disappear(60, 40,index_ico[sys_info.last_index].big_x,index_ico[sys_info.last_index].big_y,120,120,"bin/ico/connetico.bin");
    
     lcd_ico_appear(60, 239, index_ico[sys_info.index].big_x,index_ico[sys_info.index].big_y,index_ico[sys_info.index].big_w,index_ico[sys_info.index].big_h,index_ico[sys_info.index].big_ico);
    Show_Str_Mid(0,180,index_ico[sys_info.index].text,24,240);
   }
   else
   {
       fun_main(sys_info.main_index);
   }
   
 

}

void clock_strat()
{
    ico_dis(4);
    rtc_tick_irq_unregister();
    /*时钟出现*/


        uint32_t mubiao = (uint32_t *)iomem_malloc(200*200*2);//申请空间              
            dmac_wait_idle ( SYSCTL_DMA_CHANNEL_1 );//选择通道一
           dmac_set_single_mode ( SYSCTL_DMA_CHANNEL_1 , (uint32_t)gImage_clock,  mubiao , DMAC_ADDR_INCREMENT,DMAC_ADDR_INCREMENT , DMAC_MSIZE_4 , DMAC_TRANS_WIDTH_32 , 80000/4);             
            dmac_wait_done ( SYSCTL_DMA_CHANNEL_1);
        
            for(uint16_t j=1;j<201;j++)
    {
        lcd_draw_picture_by_half(20,239-j,200,j,(uint16_t *)mubiao);
        usleep(300);

    }
    lcd_ico_move(20, 39, 20, 20,  200, 200,mubiao,1);

         uint8_t h=0,m=0,s=0;
          while(1)
           {
            dmac_wait_idle ( SYSCTL_DMA_CHANNEL_1 );//选择通道一
           dmac_set_single_mode ( SYSCTL_DMA_CHANNEL_1 , (uint32_t)gImage_clock,  mubiao , DMAC_ADDR_INCREMENT,DMAC_ADDR_INCREMENT , DMAC_MSIZE_4 , DMAC_TRANS_WIDTH_32 , 80000/4);             
            dmac_wait_done ( SYSCTL_DMA_CHANNEL_1);
            s++;
            if(s==60)
            {
                m++;
                if(m==60)
                {
                    h++;
                    m=0;
                    if(h==24)
                        h=0;
                }
                s=0;
            }  
              get_date_time();
            write_hour(h,RED,mubiao);
            write_minute(m,GREEN,mubiao);
            write_second(s,BLUE,mubiao);
             lcd_draw_picture_by_half(20,20,200, 200,  mubiao);
           
           }         
            /*旋转变换*/
              iomem_free(mubiao);
}
void fun1()
{	
    uint8_t rom=0;
    sys_info.index=0;
    sys_info.key=5;
    // ico_disappear_rl(index_ico[0].index_x, index_ico[0].index_y, 320, 
       // index_ico[0].index_y, index_ico[0].index_w,index_ico[0].index_h,index_ico[0].index_ico,1);
    // ico_dis(1);
    // Show_Str(0,180,240,64,"啊你好",24,1);
    //fun1_1();
  //  show_time();
    clock_strat();
    // rotate_test();
	while(1)
    {
        // rom++;
        // sys_info.key=rom%5;
        // lcdown=1;
        // switch_key();
        // lcdown=0;
        // usleep(1000*1000);
    }
	

    usleep(1000*1000);

}