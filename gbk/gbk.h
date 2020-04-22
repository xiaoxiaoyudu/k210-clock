#ifndef __GBK_H__
#define __GBK_H__	 
#include <stdio.h>
#define GBK 0xA00000	
#define GB32 0xB00000	
#define u8 unsigned char
#define u16 uint32_t	     
void Get_HzMat(unsigned char *code,unsigned char *mat,u8 size);			//�õ����ֵĵ�����
void Show_Font(u16 x,u16 y,u8 *font,u8 size,u8 mode);					//��ָ��λ����ʾһ������
void Show_Str(u16 x,u16 y,u16 width,u16 height,u8*str,u8 size,u8 mode);	//��ָ��λ����ʾһ���ַ��� 
void Show_Str_Mid(u16 x,u16 y,u8*str,u8 size,u8 len);
void Show_Font_CL(u16 x,u16 y,u8 *font,u8 size,uint16_t color);
void Show_Str_Mid_CL(u16 x,u16 y,u8*str,u8 size,u8 len,uint16_t color);
void Show_Str_CL(u16 x,u16 y,u16 width,u16 height,u8*str,u8 size,uint16_t color);
#endif
