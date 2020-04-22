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
#ifndef _INDEX_H_
#define _INDEX_H_

#include <stdint.h>
//#include "main.c"
#define FUNNUM 6
typedef struct _caidan
{   
    uint8_t up;
    uint8_t down;
    uint8_t left;
    uint8_t right;
    uint8_t sure;
    uint8_t main_index;//当前主索引
    void (*indexing);
}index_t;
typedef struct _caidan_mian
{   
	char * index_ico;
	char * big_ico;
	char * text;
	uint16_t index_x;
	uint16_t index_y;
	uint16_t big_x;
	uint16_t big_y;
	uint16_t index_w;
	uint16_t index_h;
	uint16_t big_w;
	uint16_t big_h;

}main_index_t;
typedef struct _side
{   

	uint16_t x;
	uint16_t y;

}side_t;
extern void draw_side_cartoon(uint16_t x1, uint16_t y1,uint16_t x2, uint16_t y2,uint16_t color);
extern void get_date_time();
extern unsigned char lcdown;//

void fun_index();
void fun_main(uint8_t index);
void fun1(void);
void fun_second(void);//次级菜单
void fun1_1();
void fun1_2();
void fun1_1_1();
#endif

