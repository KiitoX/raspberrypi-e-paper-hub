/***************************************************
//Web: http://www.buydisplay.com
EastRising Technology Co.,LTD
****************************************************/

#ifndef __EPD_0583_1_H_
#define __EPD_0583_1_H_

#include "DEV_Config.h"

// Display resolution
#define EPD_0583_1_WIDTH       648
#define EPD_0583_1_HEIGHT      480

UBYTE EPD_0583_1_Init(void);
void EPD_0583_1_Clear(void);
void EPD_0583_1_Display(const UBYTE *blackimage, const UBYTE *ryimage);
void EPD_0583_1_Sleep(void);

void EPD_0583_1_DisplayFast(const UBYTE *blackimage, const UBYTE *ryimage, const UDOUBLE x, const UDOUBLE w);
void EPD_0583_1_PartialDisplay(const UBYTE *blackimage, const UBYTE *ryimage, const UDOUBLE x, const UDOUBLE y, const UDOUBLE w, const UDOUBLE h);

#endif
