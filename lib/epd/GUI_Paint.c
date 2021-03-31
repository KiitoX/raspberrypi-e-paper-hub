/***************************************************
//Web: http://www.buydisplay.com
EastRising Technology Co.,LTD
****************************************************/
#include "GUI_Paint.h"
#include "DEV_Config.h"
#include "Debug.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h> //memset()
#include <math.h>
#include <wchar.h>

PAINT Paint;

/******************************************************************************
function: Create Image
parameter:
    image   :   Pointer to the image cache
    width   :   The width of the picture
    Height  :   The height of the picture
    Color   :   Whether the picture is inverted
******************************************************************************/
void Paint_NewImage(UBYTE *image, UWORD Width, UWORD Height, UWORD Rotate, UWORD Color) {
    Paint.Image = NULL;
    Paint.Image = image;

    Paint.WidthMemory = Width;
    Paint.HeightMemory = Height;
    Paint.Color = Color;
    Paint.Scale = 2;
    Paint.WidthByte = (Width % 8 == 0) ? (Width / 8) : (Width / 8 + 1);
    Paint.HeightByte = Height;
//    printf("WidthByte = %d, HeightByte = %d\r\n", Paint.WidthByte, Paint.HeightByte);
//    printf(" EPD_WIDTH / 8 = %d\r\n",  122 / 8);

    Paint.Rotate = Rotate;
    Paint.Mirror = MIRROR_NONE;

    if (Rotate == ROTATE_0 || Rotate == ROTATE_180) {
        Paint.Width = Width;
        Paint.Height = Height;
    } else {
        Paint.Width = Height;
        Paint.Height = Width;
    }
}

/******************************************************************************
function: Select Image
parameter:
    image : Pointer to the image cache
******************************************************************************/
void Paint_SelectImage(UBYTE *image) {
    Paint.Image = image;
}

/******************************************************************************
function: Select Image Rotate
parameter:
    Rotate : 0,90,180,270
******************************************************************************/
void Paint_SetRotate(UWORD Rotate) {
    if (Rotate == ROTATE_0 || Rotate == ROTATE_90 || Rotate == ROTATE_180 || Rotate == ROTATE_270) {
        Debug("Set image Rotate %d\r\n", Rotate);
        Paint.Rotate = Rotate;
    } else {
        Debug("rotate = 0, 90, 180, 270\r\n");
    }
}

/******************************************************************************
function:	Select Image mirror
parameter:
    mirror   :Not mirror,Horizontal mirror,Vertical mirror,Origin mirror
******************************************************************************/
void Paint_SetMirroring(UBYTE mirror) {
    if (mirror == MIRROR_NONE || mirror == MIRROR_HORIZONTAL ||
        mirror == MIRROR_VERTICAL || mirror == MIRROR_ORIGIN) {
        Debug("mirror image x:%s, y:%s\r\n", (mirror & 0x01) ? "mirror" : "none",
              ((mirror >> 1) & 0x01) ? "mirror" : "none");
        Paint.Mirror = mirror;
    } else {
        Debug("mirror should be MIRROR_NONE, MIRROR_HORIZONTAL, \
        MIRROR_VERTICAL or MIRROR_ORIGIN\r\n");
    }
}

void Paint_SetScale(UBYTE scale) {
    if (scale == 2) {
        Paint.Scale = scale;
        Paint.WidthByte = (Paint.WidthMemory % 8 == 0) ? (Paint.WidthMemory / 8) : (Paint.WidthMemory / 8 + 1);
    } else if (scale == 4) {
        Paint.Scale = scale;
        Paint.WidthByte = (Paint.WidthMemory % 4 == 0) ? (Paint.WidthMemory / 4) : (Paint.WidthMemory / 4 + 1);
    } else if (scale == 7) {//Only applicable with 5in65 e-Paper
        Paint.Scale = scale;
        Paint.WidthByte = (Paint.WidthMemory % 2 == 0) ? (Paint.WidthMemory / 2) : (Paint.WidthMemory / 2 + 1);;
    } else {
        Debug("Set Scale Input parameter error\r\n");
        Debug("Scale Only support: 2 4 7\r\n");
    }
}

/******************************************************************************
function: Draw Pixels
parameter:
    Xpoint : At point X
    Ypoint : At point Y
    Color  : Painted colors
******************************************************************************/
void Paint_SetPixel(UWORD Xpoint, UWORD Ypoint, UWORD Color) {
    if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        Debug("Exceeding display boundaries\r\n");
        return;
    }
    UWORD X, Y;
    switch (Paint.Rotate) {
        case 0:
            X = Xpoint;
            Y = Ypoint;
            break;
        case 90:
            X = Paint.WidthMemory - Ypoint - 1;
            Y = Xpoint;
            break;
        case 180:
            X = Paint.WidthMemory - Xpoint - 1;
            Y = Paint.HeightMemory - Ypoint - 1;
            break;
        case 270:
            X = Ypoint;
            Y = Paint.HeightMemory - Xpoint - 1;
            break;
        default:
            return;
    }

    switch (Paint.Mirror) {
        case MIRROR_NONE:
            break;
        case MIRROR_HORIZONTAL:
            X = Paint.WidthMemory - X - 1;
            break;
        case MIRROR_VERTICAL:
            Y = Paint.HeightMemory - Y - 1;
            break;
        case MIRROR_ORIGIN:
            X = Paint.WidthMemory - X - 1;
            Y = Paint.HeightMemory - Y - 1;
            break;
        default:
            return;
    }

    if (X > Paint.WidthMemory || Y > Paint.HeightMemory) {
        Debug("Exceeding display boundaries\r\n");
        return;
    }

    if (Paint.Scale == 2) {
        UDOUBLE Addr = X / 8 + Y * Paint.WidthByte;
        UBYTE Rdata = Paint.Image[Addr];
        if (Color == BLACK)
            Paint.Image[Addr] = Rdata & ~(0x80 >> (X % 8));
        else
            Paint.Image[Addr] = Rdata | (0x80 >> (X % 8));
    } else if (Paint.Scale == 4) {
        UDOUBLE Addr = X / 4 + Y * Paint.WidthByte;
        Color = Color % 4;//Guaranteed color scale is 4  --- 0~3
        UBYTE Rdata = Paint.Image[Addr];

        Rdata = Rdata & (~(0xC0 >> ((X % 4) * 2)));//Clear first, then set value
        Paint.Image[Addr] = Rdata | ((Color << 6) >> ((X % 4) * 2));
    } else if (Paint.Scale == 7) {
        UDOUBLE Addr = X / 2 + Y * Paint.WidthByte;
        UBYTE Rdata = Paint.Image[Addr];
        Rdata = Rdata & (~(0xF0 >> ((X % 2) * 4)));//Clear first, then set value
        Paint.Image[Addr] = Rdata | ((Color << 4) >> ((X % 2) * 4));
        //printf("Add =  %d ,data = %d\r\n",Addr,Rdata);
    }
}

/******************************************************************************
function: Clear the color of the picture
parameter:
    Color : Painted colors
******************************************************************************/
void Paint_Clear(UWORD Color) {
    UWORD Y, X;
    if (Paint.Scale == 2 || Paint.Scale == 4) {
        for (Y = 0; Y < Paint.HeightByte; Y++) {
            for (X = 0; X < Paint.WidthByte; X++) {//8 pixel =  1 byte
                UDOUBLE Addr = X + Y * Paint.WidthByte;
                Paint.Image[Addr] = Color;
            }
        }
    } else if (Paint.Scale == 7) {
        for (Y = 0; Y < Paint.HeightByte; Y++) {
            for (X = 0; X < Paint.WidthByte; X++) {
                UDOUBLE Addr = X + Y * Paint.WidthByte;
                Paint.Image[Addr] = (Color << 4) | Color;
            }
        }
    }

}

/******************************************************************************
function: Clear the color of a window
parameter:
    Xstart : x starting point
    Ystart : Y starting point
    Xend   : x end point
    Yend   : y end point
    Color  : Painted colors
******************************************************************************/
void Paint_ClearWindows(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, UWORD Color) {
    UWORD X, Y;
    for (Y = Ystart; Y < Yend; Y++) {
        for (X = Xstart; X < Xend; X++) {//8 pixel =  1 byte
            Paint_SetPixel(X, Y, Color);
        }
    }
}

/******************************************************************************
function: Draw Point(Xpoint, Ypoint) Fill the color
parameter:
    Xpoint		: The Xpoint coordinate of the point
    Ypoint		: The Ypoint coordinate of the point
    Color		: Painted color
    Dot_Pixel	: point size
    Dot_Style	: point Style
******************************************************************************/
void Paint_DrawPoint(UWORD Xpoint, UWORD Ypoint, UWORD Color,
                     DOT_PIXEL Dot_Pixel, DOT_STYLE Dot_Style) {
    if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        Debug("Paint_DrawPoint Input exceeds the normal display range\r\n");
        return;
    }

    int16_t XDir_Num, YDir_Num;
    if (Dot_Style == DOT_FILL_AROUND) {
        for (XDir_Num = 0; XDir_Num < 2 * Dot_Pixel - 1; XDir_Num++) {
            for (YDir_Num = 0; YDir_Num < 2 * Dot_Pixel - 1; YDir_Num++) {
                if (Xpoint + XDir_Num - Dot_Pixel < 0 || Ypoint + YDir_Num - Dot_Pixel < 0)
                    break;
                // printf("x = %d, y = %d\r\n", Xpoint + XDir_Num - Dot_Pixel, Ypoint + YDir_Num - Dot_Pixel);
                Paint_SetPixel(Xpoint + XDir_Num - Dot_Pixel, Ypoint + YDir_Num - Dot_Pixel, Color);
            }
        }
    } else {
        for (XDir_Num = 0; XDir_Num < Dot_Pixel; XDir_Num++) {
            for (YDir_Num = 0; YDir_Num < Dot_Pixel; YDir_Num++) {
                Paint_SetPixel(Xpoint + XDir_Num - 1, Ypoint + YDir_Num - 1, Color);
            }
        }
    }
}

/******************************************************************************
function: Draw a line of arbitrary slope
parameter:
    Xstart ：Starting Xpoint point coordinates
    Ystart ：Starting Xpoint point coordinates
    Xend   ：End point Xpoint coordinate
    Yend   ：End point Ypoint coordinate
    Color  ：The color of the line segment
    Line_width : Line width
    Line_Style: Solid and dotted lines
******************************************************************************/
void Paint_DrawLine(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,
                    UWORD Color, DOT_PIXEL Line_width, LINE_STYLE Line_Style) {
    if (Xstart > Paint.Width || Ystart > Paint.Height ||
        Xend > Paint.Width || Yend > Paint.Height) {
        Debug("Paint_DrawLine Input exceeds the normal display range\r\n");
        return;
    }

    UWORD Xpoint = Xstart;
    UWORD Ypoint = Ystart;
    int dx = (int) Xend - (int) Xstart >= 0 ? Xend - Xstart : Xstart - Xend;
    int dy = (int) Yend - (int) Ystart <= 0 ? Yend - Ystart : Ystart - Yend;

    // Increment direction, 1 is positive, -1 is counter;
    int XAddway = Xstart < Xend ? 1 : -1;
    int YAddway = Ystart < Yend ? 1 : -1;

    //Cumulative error
    int Esp = dx + dy;
    char Dotted_Len = 0;

    for (;;) {
        Dotted_Len++;
        //Painted dotted line, 2 point is really virtual
        if (Line_Style == LINE_STYLE_DOTTED && Dotted_Len % 2 == 0) {
            //Debug("LINE_DOTTED\r\n");
            Paint_DrawPoint(Xpoint, Ypoint, IMAGE_BACKGROUND, Line_width, DOT_STYLE_DFT);
            Dotted_Len = 0;
        } else {
            Paint_DrawPoint(Xpoint, Ypoint, Color, Line_width, DOT_STYLE_DFT);
        }
        if (2 * Esp >= dy) {
            if (Xpoint == Xend)
                break;
            Esp += dy;
            Xpoint += XAddway;
        }
        if (2 * Esp <= dx) {
            if (Ypoint == Yend)
                break;
            Esp += dx;
            Ypoint += YAddway;
        }
    }
}

/******************************************************************************
function: Draw a rectangle
parameter:
    Xstart ：Rectangular  Starting Xpoint point coordinates
    Ystart ：Rectangular  Starting Xpoint point coordinates
    Xend   ：Rectangular  End point Xpoint coordinate
    Yend   ：Rectangular  End point Ypoint coordinate
    Color  ：The color of the Rectangular segment
    Line_width: Line width
    Draw_Fill : Whether to fill the inside of the rectangle
******************************************************************************/
void Paint_DrawRectangle(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,
                         UWORD Color, DOT_PIXEL Line_width, DRAW_FILL Draw_Fill) {
    if (Xstart > Paint.Width || Ystart > Paint.Height ||
        Xend > Paint.Width || Yend > Paint.Height) {
        Debug("Input exceeds the normal display range\r\n");
        return;
    }

    if (Draw_Fill) {
        UWORD Ypoint;
        for (Ypoint = Ystart; Ypoint < Yend; Ypoint++) {
            Paint_DrawLine(Xstart, Ypoint, Xend, Ypoint, Color, Line_width, LINE_STYLE_SOLID);
        }
    } else {
        Paint_DrawLine(Xstart, Ystart, Xend, Ystart, Color, Line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(Xstart, Ystart, Xstart, Yend, Color, Line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(Xend, Yend, Xend, Ystart, Color, Line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(Xend, Yend, Xstart, Yend, Color, Line_width, LINE_STYLE_SOLID);
    }
}

/******************************************************************************
function: Use the 8-point method to draw a circle of the
            specified size at the specified position->
parameter:
    X_Center  ：Center X coordinate
    Y_Center  ：Center Y coordinate
    Radius    ：circle Radius
    Color     ：The color of the ：circle segment
    Line_width: Line width
    Draw_Fill : Whether to fill the inside of the Circle
******************************************************************************/
void Paint_DrawCircle(UWORD X_Center, UWORD Y_Center, UWORD Radius,
                      UWORD Color, DOT_PIXEL Line_width, DRAW_FILL Draw_Fill) {
    if (X_Center > Paint.Width || Y_Center >= Paint.Height) {
        Debug("Paint_DrawCircle Input exceeds the normal display range\r\n");
        return;
    }

    //Draw a circle from(0, R) as a starting point
    int16_t XCurrent, YCurrent;
    XCurrent = 0;
    YCurrent = Radius;

    //Cumulative error,judge the next point of the logo
    int16_t Esp = 3 - (Radius << 1);

    int16_t sCountY;
    if (Draw_Fill == DRAW_FILL_FULL) {
        while (XCurrent <= YCurrent) { //Realistic circles
            for (sCountY = XCurrent; sCountY <= YCurrent; sCountY++) {
                Paint_DrawPoint(X_Center + XCurrent, Y_Center + sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//1
                Paint_DrawPoint(X_Center - XCurrent, Y_Center + sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//2
                Paint_DrawPoint(X_Center - sCountY, Y_Center + XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//3
                Paint_DrawPoint(X_Center - sCountY, Y_Center - XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//4
                Paint_DrawPoint(X_Center - XCurrent, Y_Center - sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//5
                Paint_DrawPoint(X_Center + XCurrent, Y_Center - sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//6
                Paint_DrawPoint(X_Center + sCountY, Y_Center - XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//7
                Paint_DrawPoint(X_Center + sCountY, Y_Center + XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);
            }
            if (Esp < 0)
                Esp += 4 * XCurrent + 6;
            else {
                Esp += 10 + 4 * (XCurrent - YCurrent);
                YCurrent--;
            }
            XCurrent++;
        }
    } else { //Draw a hollow circle
        while (XCurrent <= YCurrent) {
            Paint_DrawPoint(X_Center + XCurrent, Y_Center + YCurrent, Color, Line_width, DOT_STYLE_DFT);//1
            Paint_DrawPoint(X_Center - XCurrent, Y_Center + YCurrent, Color, Line_width, DOT_STYLE_DFT);//2
            Paint_DrawPoint(X_Center - YCurrent, Y_Center + XCurrent, Color, Line_width, DOT_STYLE_DFT);//3
            Paint_DrawPoint(X_Center - YCurrent, Y_Center - XCurrent, Color, Line_width, DOT_STYLE_DFT);//4
            Paint_DrawPoint(X_Center - XCurrent, Y_Center - YCurrent, Color, Line_width, DOT_STYLE_DFT);//5
            Paint_DrawPoint(X_Center + XCurrent, Y_Center - YCurrent, Color, Line_width, DOT_STYLE_DFT);//6
            Paint_DrawPoint(X_Center + YCurrent, Y_Center - XCurrent, Color, Line_width, DOT_STYLE_DFT);//7
            Paint_DrawPoint(X_Center + YCurrent, Y_Center + XCurrent, Color, Line_width, DOT_STYLE_DFT);//0

            if (Esp < 0)
                Esp += 4 * XCurrent + 6;
            else {
                Esp += 10 + 4 * (XCurrent - YCurrent);
                YCurrent--;
            }
            XCurrent++;
        }
    }
}

#ifdef BSFONTS
/******************************************************************************
function: Show English characters
parameter:
    Xpoint           ：X coordinate
    Ypoint           ：Y coordinate
    Acsii_Char       ：To display the English characters
    Font             ：A structure pointer that displays a character size
    Color_Foreground : Select the foreground color
    Color_Background : Select the background color
******************************************************************************/
void Paint_DrawChar(UWORD Xpoint, UWORD Ypoint, const char Acsii_Char,
                    sFONT* Font, UWORD Color_Foreground, UWORD Color_Background)
{
    UWORD Page, Column;

    if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        Debug("Paint_DrawChar Input exceeds the normal display range\r\n");
        return;
    }

    uint32_t Char_Offset = (Acsii_Char - ' ') * Font->Height * (Font->Width / 8 + (Font->Width % 8 ? 1 : 0));
    const unsigned char *ptr = &Font->table[Char_Offset];

    for (Page = 0; Page < Font->Height; Page ++ ) {
        for (Column = 0; Column < Font->Width; Column ++ ) {

            //To determine whether the font background color and screen background color is consistent
            if (FONT_BACKGROUND == Color_Background) { //this process is to speed up the scan
                if (*ptr & (0x80 >> (Column % 8)))
                    Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Foreground);
                    // Paint_DrawPoint(Xpoint + Column, Ypoint + Page, Color_Foreground, DOT_PIXEL_DFT, DOT_STYLE_DFT);
            } else {
                if (*ptr & (0x80 >> (Column % 8))) {
                    Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Foreground);
                    // Paint_DrawPoint(Xpoint + Column, Ypoint + Page, Color_Foreground, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                } else {
                    Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Background);
                    // Paint_DrawPoint(Xpoint + Column, Ypoint + Page, Color_Background, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                }
            }
            //One pixel is 8 bits
            if (Column % 8 == 7)
                ptr++;
        }// Write a line
        if (Font->Width % 8 != 0)
            ptr++;
    }// Write all
}

/******************************************************************************
function:	Display the string
parameter:
    Xstart           ：X coordinate
    Ystart           ：Y coordinate
    pString          ：The first address of the English string to be displayed
    Font             ：A structure pointer that displays a character size
    Color_Foreground : Select the foreground color
    Color_Background : Select the background color
******************************************************************************/
void Paint_DrawString_EN(UWORD Xstart, UWORD Ystart, const char * pString,
                         sFONT* Font, UWORD Color_Foreground, UWORD Color_Background)
{
    UWORD Xpoint = Xstart;
    UWORD Ypoint = Ystart;

    if (Xstart > Paint.Width || Ystart > Paint.Height) {
        Debug("Paint_DrawString_EN Input exceeds the normal display range\r\n");
        return;
    }

    while (* pString != '\0') {
        //if X direction filled , reposition to(Xstart,Ypoint),Ypoint is Y direction plus the Height of the character
        if ((Xpoint + Font->Width ) > Paint.Width ) {
            Xpoint = Xstart;
            Ypoint += Font->Height;
        }

        // If the Y direction is full, reposition to(Xstart, Ystart)
        if ((Ypoint  + Font->Height ) > Paint.Height ) {
            Xpoint = Xstart;
            Ypoint = Ystart;
        }
        Paint_DrawChar(Xpoint, Ypoint, * pString, Font, Color_Foreground, Color_Background);

        //The next character of the address
        pString ++;

        //The next word of the abscissa increases the font of the broadband
        Xpoint += Font->Width;
    }
}


/******************************************************************************
function: Display the string
parameter:
    Xstart  ：X coordinate
    Ystart  ：Y coordinate
    pString ：The first address of the Chinese string and English
              string to be displayed
    Font    ：A structure pointer that displays a character size
    Color_Foreground : Select the foreground color
    Color_Background : Select the background color
******************************************************************************/
void Paint_DrawString_CN(UWORD Xstart, UWORD Ystart, const char * pString, cFONT* font,
                        UWORD Color_Foreground, UWORD Color_Background)
{
    const char* p_text = pString;
    int x = Xstart, y = Ystart;
    int i, j,Num;

    /* Send the string character by character on EPD */
    while (*p_text != 0) {
        if(*p_text <= 0x7F) {  //ASCII < 126
            for(Num = 0; Num < font->size; Num++) {
                if(*p_text== font->table[Num].index[0]) {
                    const char* ptr = &font->table[Num].matrix[0];

                    for (j = 0; j < font->Height; j++) {
                        for (i = 0; i < font->Width; i++) {
                            if (FONT_BACKGROUND == Color_Background) { //this process is to speed up the scan
                                if (*ptr & (0x80 >> (i % 8))) {
                                    Paint_SetPixel(x + i, y + j, Color_Foreground);
                                    // Paint_DrawPoint(x + i, y + j, Color_Foreground, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                                }
                            } else {
                                if (*ptr & (0x80 >> (i % 8))) {
                                    Paint_SetPixel(x + i, y + j, Color_Foreground);
                                    // Paint_DrawPoint(x + i, y + j, Color_Foreground, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                                } else {
                                    Paint_SetPixel(x + i, y + j, Color_Background);
                                    // Paint_DrawPoint(x + i, y + j, Color_Background, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                                }
                            }
                            if (i % 8 == 7) {
                                ptr++;
                            }
                        }
                        if (font->Width % 8 != 0) {
                            ptr++;
                        }
                    }
                    break;
                }
            }
            /* Point on the next character */
            p_text += 1;
            /* Decrement the column position by 16 */
            x += font->ASCII_Width;
        } else {        //Chinese
            for(Num = 0; Num < font->size; Num++) {
                if((*p_text== font->table[Num].index[0]) && (*(p_text+1) == font->table[Num].index[1])) {
                    const char* ptr = &font->table[Num].matrix[0];

                    for (j = 0; j < font->Height; j++) {
                        for (i = 0; i < font->Width; i++) {
                            if (FONT_BACKGROUND == Color_Background) { //this process is to speed up the scan
                                if (*ptr & (0x80 >> (i % 8))) {
                                    Paint_SetPixel(x + i, y + j, Color_Foreground);
                                    // Paint_DrawPoint(x + i, y + j, Color_Foreground, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                                }
                            } else {
                                if (*ptr & (0x80 >> (i % 8))) {
                                    Paint_SetPixel(x + i, y + j, Color_Foreground);
                                    // Paint_DrawPoint(x + i, y + j, Color_Foreground, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                                } else {
                                    Paint_SetPixel(x + i, y + j, Color_Background);
                                    // Paint_DrawPoint(x + i, y + j, Color_Background, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                                }
                            }
                            if (i % 8 == 7) {
                                ptr++;
                            }
                        }
                        if (font->Width % 8 != 0) {
                            ptr++;
                        }
                    }
                    break;
                }
            }
            /* Point on the next character */
            p_text += 2;
            /* Decrement the column position by 16 */
            x += font->Width;
        }
    }
}

/******************************************************************************
function:	Display nummber
parameter:
    Xstart           ：X coordinate
    Ystart           : Y coordinate
    Nummber          : The number displayed
    Font             ：A structure pointer that displays a character size
    Color_Foreground : Select the foreground color
    Color_Background : Select the background color
******************************************************************************/
#define  ARRAY_LEN 255
void Paint_DrawNum(UWORD Xpoint, UWORD Ypoint, int32_t Nummber,
                   sFONT* Font, UWORD Color_Foreground, UWORD Color_Background)
{

    int16_t Num_Bit = 0, Str_Bit = 0;
    uint8_t Str_Array[ARRAY_LEN] = {0}, Num_Array[ARRAY_LEN] = {0};
    uint8_t *pStr = Str_Array;

    if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        Debug("Paint_DisNum Input exceeds the normal display range\r\n");
        return;
    }

    //Converts a number to a string
    while (Nummber) {
        Num_Array[Num_Bit] = Nummber % 10 + '0';
        Num_Bit++;
        Nummber /= 10;
    }

    //The string is inverted
    while (Num_Bit > 0) {
        Str_Array[Str_Bit] = Num_Array[Num_Bit - 1];
        Str_Bit ++;
        Num_Bit --;
    }

    //show
    Paint_DrawString_EN(Xpoint, Ypoint, (const char*)pStr, Font, Color_Foreground, Color_Background);
}

/******************************************************************************
function:	Display time
parameter:
    Xstart           ：X coordinate
    Ystart           : Y coordinate
    pTime            : Time-related structures
    Font             ：A structure pointer that displays a character size
    Color_Foreground : Select the foreground color
    Color_Background : Select the background color
******************************************************************************/
void Paint_DrawTime(UWORD Xstart, UWORD Ystart, PAINT_TIME *pTime, sFONT* Font,
                    UWORD Color_Foreground, UWORD Color_Background)
{
    uint8_t value[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

    UWORD Dx = Font->Width;

    //Write data into the cache
    Paint_DrawChar(Xstart                           , Ystart, value[pTime->Hour / 10], Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx                      , Ystart, value[pTime->Hour % 10], Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx  + Dx / 4 + Dx / 2   , Ystart, ':'                    , Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx * 2 + Dx / 2         , Ystart, value[pTime->Min / 10] , Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx * 3 + Dx / 2         , Ystart, value[pTime->Min % 10] , Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx * 4 + Dx / 2 - Dx / 4, Ystart, ':'                    , Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx * 5                  , Ystart, value[pTime->Sec / 10] , Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx * 6                  , Ystart, value[pTime->Sec % 10] , Font, Color_Background, Color_Foreground);
}

#else

int Paint_DrawChar(UWORD xPos, UWORD yPos, encoding_t character, bdf_t *font, UWORD color_fg, UWORD color_bg) {
    if (xPos > Paint.Width || yPos > Paint.Height) {
        Debug("Paint_DrawChar: Input exceeds display boundary\n");
        return 0;
    }

    bitmap_t *bitmap = bdf_get_bitmap(font, character);
    if (bitmap == NULL) {
        Debug("Paint_DrawChar: Character '%lc' U+%04llX is not in bitmap\n",  character, character);
        return 0;
    }

    size_t byte_width = ((bitmap->width + 7) / 8);

    for (UWORD y = 0; y < font->height * font->scale; ++y) {
        for (UWORD x = 0; x < font->width * font->scale; ++x) {
            UWORD bitmapX = (x / font->scale) + font->offsetX - bitmap->offsetX;
            UWORD bitmapY = (y / font->scale) - font->offsetY + bitmap->offsetY + bitmap->height - font->height;

            if (0 <= bitmapX && bitmapX < bitmap->width &&
                0 <= bitmapY && bitmapY < bitmap->height) {
                if (((bitmap->bits[bitmapY * byte_width + (bitmapX / 8)] << (bitmapX % 8)) & 0x80) > 0) {
                    Paint_SetPixel(xPos + x, yPos + y, color_fg);
                } else if (FONT_BACKGROUND != color_bg) {
                    Paint_SetPixel(xPos + x, yPos + y, color_bg);
                }
            } else if (FONT_BACKGROUND != color_bg) {
                Paint_SetPixel(xPos + x, yPos + y, color_bg);
            }
        }
    }

    return bitmap->deviceWidth * font->scale;
}

void Paint_DrawString(UWORD xPos, UWORD yPos, const char *string, bdf_t *font, UWORD color_fg, UWORD color_bg) {
    if (xPos > Paint.Width || yPos > Paint.Height) {
        Debug("Paint_DrawString: Input exceeds display boundary\n");
        return;
    }

    UWORD x = xPos, y = yPos;

    mbstate_t state = {0};
    // fail if this isn't the required initial state
    assert(mbsinit(&state) != 0);

    encoding_t character;
    size_t offset;

    while (*string != '\0') {
        offset = mbrtowc(&character, string, MB_CUR_MAX, &state);
        if (offset <= 0) {
            Debug("Paint_DrawString: String contains invalid UTF-8 characters");
            return;
        }

        // move to new line, and skip newline character
        if (character == '\n') {
            x = xPos;
            y += font->height * font->scale;
            string++;
            continue;
        }

        // move to new line at x boundary
        if ((x + font->width * font->scale) > Paint.Width) {
            x = xPos;
            y += font->height * font->scale;
        }

        // return to beginning at y boundary
        if ((y + font->height * font->scale) > Paint.Height) {
            x = xPos;
            y = yPos;
        }

        x += Paint_DrawChar(x, y, character, font, color_fg, color_bg);
        string += offset;
    }
}

void Paint_DrawCharmap(UWORD xPos, UWORD yPos, bdf_t *font, size_t offset, UWORD color_fg, UWORD color_bg) {
    if (xPos > Paint.Width || yPos > Paint.Height) {
        Debug("Paint_DrawString: Input exceeds display boundary\n");
        return;
    }

    UWORD x = xPos, y = yPos;

    for (size_t i = offset; i < font->numChars; ++i) {
        // move to new line at x boundary
        if ((x + font->width * font->scale) > Paint.Width) {
            x = xPos;
            y += font->height * font->scale;
        }

        // stop at y boundary
        if ((y + font->height * font->scale) > Paint.Height) {
            Debug("Paint_DrawCharmap: Reached display limit at i = %ld\n", i);
            return;
        }

        x += Paint_DrawChar(x, y, font->characters[i].encoding, font, color_fg, color_bg);
    }
}

#endif

/******************************************************************************
function:	Display monochrome bitmap
parameter:
    image_buffer ：A picture data converted to a bitmap
info:
    Use a computer to convert the image into a corresponding array,
    and then embed the array directly into Imagedata.cpp as a .c file.
******************************************************************************/
void Paint_DrawBitMap(const unsigned char *image_buffer) {
    UWORD x, y;
    UDOUBLE Addr = 0;

    for (y = 0; y < Paint.HeightByte; y++) {
        for (x = 0; x < Paint.WidthByte; x++) {//8 pixel =  1 byte
            Addr = x + y * Paint.WidthByte;
            Paint.Image[Addr] = (unsigned char) image_buffer[Addr];
        }
    }
}
