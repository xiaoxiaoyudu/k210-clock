#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include "w25qxx.h"
#include "lcd.h"
#include "gbk.h"
void Get_HzMat(unsigned char *code,unsigned char *mat,u8 size)
{		    
	unsigned char qh,ql;
	unsigned char i;					  
	unsigned long foffset; 
	u8 csize=(size/8+((size%8)?1:0))*(size);//得到字体一个字符对应点阵集所占的字节数	 
	if(size==16)
	{
			qh=*code;
		ql=*(++code);
		if(qh<0x81||ql<0x40||ql==0xff||qh==0xff)//非 常用汉字
		{   		    
	 	   for(i=0;i<csize;i++)*mat++=0x00;//填充满格
	   		 return; //结束访问
		}          
		if(ql<0x7f)ql-=0x40;//注意!
			else ql-=0x41;
			qh-=0x81;   
	
	foffset=((unsigned long)190*qh+ql)*csize;	//得到字库中的字节偏移量  	

			w25qxx_read_data(GBK+foffset, mat, csize, W25QXX_QUAD_FAST);	
  
	}
	else
	{
		qh=*code;
		ql=*(++code);
		ql -= 0xa1;
		qh -= 0xa1;
        foffset = ((unsigned long)94*qh + ql)*csize;
	//w25qxx_read_data(GB32+foffset, mat, csize, W25QXX_QUAD_FAST);	 
	w25qxx_read_data(GB32+foffset, mat, csize, W25QXX_QUAD_FAST);
	
	}
	
   												    
}  
//显示一个指定大小的汉字
//x,y :汉字的坐标
//font:汉字GBK码
//size:字体大小
//mode:0慢速,1,快速	   gb32只支持快速
void Show_Font(u16 x,u16 y,u8 *font,u8 size,u8 mode)
{
	
	u8 temp,t,t1;
	u16 y0=y;
	u8 dzk[128];   
	u8 csize=(size/8+((size%8)?1:0))*(size);//得到字体一个字符对应点阵集所占的字节数	 

	if(size!=16&&size!=24)return;	//不支持的size
	
	Get_HzMat(font,dzk,size);	//得到相应大小的点阵数据 
	if(mode)
	{
		if(size==16)
		lcd_draw_ico(x,y, 16, 16,dzk, BLACK);
		else
		{
		
			lcd_draw_ico(x,y, 24, 24,dzk, BLACK);
		}
		
	}
	else
	{
			uint8_t i = 0;
		    uint8_t j = 0;
		    uint8_t z = 0;
		    uint8_t data = 0;
		    for (z = 0; z < 16; z++){
		         for (i = 0; i < 2; i++)
		    {
		        data = dzk[z*2+i];
		        for (j = 0; j < 8; j++)
		        {
		            if (data & 0x80)
		                lcd_draw_point(x + i*8+j, y+z, BLACK);
		            data <<= 1;
		        }

		    }
		    }

	// 	for(t=0;t<csize;t++)
	// {   												   
	// 	temp=dzk[t];			//得到点阵数据                          
	// 	for(t1=0;t1<8;t1++)
	// 	{
	// 		if(temp&0x80)lcd_draw_point(x, y,BLACK);
	// 		else 
	// 		temp<<=1;
	// 		y++;
	// 		if((y-y0)==size)
	// 		{
	// 			y=y0;
	// 			x++;
	// 			break;
	// 		}
	// 	}  	 
	// }  
	}
	
}
//在指定位置开始显示一个字符串	    
//支持自动换行
//(x,y):起始坐标
//width,height:区域
//str  :字符串
//size :字体大小
//mode:0,慢速;1,快速   	   		   
void Show_Str(u16 x,u16 y,u16 width,u16 height,u8*str,u8 size,u8 mode)
{					
	u16 x0=x;
	u16 y0=y;							  	  
    u8 bHz=0;     //字符或者中文 1中文 0英语 	    				    				  	  
    while(*str!=0)//数据未结束
    { 
        if(!bHz)//英语
        {
	        if(*str>0x80)bHz=1;//中文 
	        else              //字符
	        {      
                if(x>(x0+width-size/2))//换行
				{				   
					y+=size;
					x=x0;	   
				}							    
		        if(y>(y0+height-size))break;//越界返回      
		        if(*str==13)//换行符号
		        {         
		            y+=size;
					x=x0;
		            str++; 
		        }  
		        else 
		   
		        	
		      lcd_draw_char(x, y, *str, BLACK) ;
		      	
		        
				str++; 
		        x+=size/2; //字符,为全字的一半 
	        }
        }else//中文 
        {     
            bHz=0;//有汉字库    
            if(x>(x0+width-size))//换行
			{	    
				y+=size;
				x=x0;		  
			}
	        if(y>(y0+height-size))break;//越界返回  				     
	        Show_Font(x,y,str,size,mode); //显示这个汉字,空心显示 
	        str+=2; 
	        x+=size;//下一个汉字偏移	    
        }						 
    }   
}  			 		 
//在指定宽度的中间显示字符串
//如果字符长度超过了len,则用Show_Str显示
//len:指定要显示的宽度			  
void Show_Str_Mid(u16 x,u16 y,u8*str,u8 size,u8 len)
{
	u16 strlenth=0;
   	strlenth=strlen((const char*)str);
	strlenth*=size/2;
	if(strlenth>len)Show_Str(x,y,320,240,str,size,1);
	else
	{
		strlenth=(len-strlenth)/2;
	    Show_Str(strlenth+x,y,320,240,str,size,1);
	}
}   
//指定颜色
void Show_Str_Mid_CL(u16 x,u16 y,u8*str,u8 size,u8 len,uint16_t color)
{
	u16 strlenth=0;
   	strlenth=strlen((const char*)str);
	strlenth*=size/2;
	if(strlenth>len)Show_Str(x,y,320,240,str,size,1);
	else
	{
		strlenth=(len-strlenth)/2;
	    Show_Str_CL(strlenth+x,y,320,240,str,size,color);
	}
}   
void Show_Str_CL(u16 x,u16 y,u16 width,u16 height,u8*str,u8 size,uint16_t color)
{					
	u16 x0=x;
	u16 y0=y;							  	  
    u8 bHz=0;     //字符或者中文 1中文 0英语 	    				    				  	  
    while(*str!=0)//数据未结束
    { 
        if(!bHz)//英语
        {
	        if(*str>0x80)bHz=1;//中文 
	        else              //字符
	        {      
                if(x>(x0+width-size/2))//换行
				{				   
					y+=size;
					x=x0;	   
				}							    
		        if(y>(y0+height-size))break;//越界返回      
		        if(*str==13)//换行符号
		        {         
		            y+=size;
					x=x0;
		            str++; 
		        }  
		        else 
		   
		        	
		      lcd_draw_char(x, y, *str, BLACK) ;
		      	
		        
				str++; 
		        x+=size/2; //字符,为全字的一半 
	        }
        }else//中文 
        {     
            bHz=0;//有汉字库    
            if(x>(x0+width-size))//换行
			{	    
				y+=size;
				x=x0;		  
			}
	        if(y>(y0+height-size))break;//越界返回  				     
	        Show_Font_CL(x,y,str,size,color); //显示这个汉字,空心显示 
	        str+=2; 
	        x+=size;//下一个汉字偏移	    
        }						 
    }   
}  		
//显示一个指定大小的汉字
//x,y :汉字的坐标
//font:汉字GBK码
//size:字体大小
void Show_Font_CL(u16 x,u16 y,u8 *font,u8 size,uint16_t color)
{
	
	u8 temp,t,t1;
	u16 y0=y;
	u8 dzk[128];   
	u8 csize=(size/8+((size%8)?1:0))*(size);//得到字体一个字符对应点阵集所占的字节数	 

	if(size!=16&&size!=24)return;	//不支持的size
	
	Get_HzMat(font,dzk,size);	//得到相应大小的点阵数据 

	
		if(size==16)
		lcd_draw_ico(x,y, 16, 16,dzk, color);
		else
		{
		
			lcd_draw_ico(x,y, 24, 24,dzk, color);
		}
		
	

	
}























		  






