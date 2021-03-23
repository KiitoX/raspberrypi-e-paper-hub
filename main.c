#include <stdio.h>
#include <signal.h>

#include "lib/bdf.h"

#ifdef EPD
#include "lib/epd/ER-EPD0583-1.h"
#include "lib/epd/GUI_Paint.h"

void signal_handler(int signo) {
    DEV_Module_Exit();
}
#endif

int main() {
    /**
     * Init
     */

#ifdef EPD
    signal(SIGINT, signal_handler);
#endif

    char *file = "./fonts/scientifica-11.bdf";

    bdf_t *font = bdf_read(file, 1);

#ifdef EPD
    assert(DEV_Module_Init() == 0);

    EPD_0583_1_Init();

    EPD_0583_1_Clear();

    DEV_Delay_ms(500);

    UBYTE *image_black, *image_red;
    UWORD image_size = ((EPD_0583_1_WIDTH % 8 == 0) ? (EPD_0583_1_WIDTH / 8) : (EPD_0583_1_WIDTH / 8 + 1)) * EPD_0583_1_HEIGHT;

    image_black = malloc(image_size);
    assert(image_black != NULL);

    image_red = malloc(image_size);
    assert(image_red != NULL);

    Paint_NewImage(image_black, EPD_0583_1_WIDTH, EPD_0583_1_HEIGHT, ROTATE_0, WHITE);
    Paint_NewImage(image_red, EPD_0583_1_WIDTH, EPD_0583_1_HEIGHT, ROTATE_0, WHITE);
#endif

    /**
     * Main
     */

#ifdef EPD
    // Draw black image
    Paint_SelectImage(image_black);
    Paint_Clear(WHITE);

    Paint_DrawPoint(25, 50, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);
    Paint_DrawPoint(25, 70, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
    Paint_DrawPoint(25, 90, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
    Paint_DrawPoint(25, 110, BLACK, DOT_PIXEL_4X4, DOT_STYLE_DFT);

    Paint_DrawLine(50, 50, 150, 150, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(150, 50, 50, 150, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(100, 200, 100, 300, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    Paint_DrawLine(50, 250, 150, 250, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);

    Paint_DrawString(10, 335, "www.buydisplay.com", font, BLACK, WHITE);

    // Draw red picture
    Paint_SelectImage(image_red);
    Paint_Clear(WHITE);

    Paint_DrawRectangle(50, 50, 150, 150, RED, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(200, 50, 300, 150, RED, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    Paint_DrawCircle(100, 250, 50, RED, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(250, 250, 50, RED, DOT_PIXEL_1X1, DRAW_FILL_FULL);


    Paint_DrawString(10, 310, "hello! EPD", font, WHITE, BLACK);
    Paint_DrawString(10, 360, "123456789", font, RED, WHITE);

    EPD_0583_1_Display(image_black, image_red);
    DEV_Delay_ms(30000);
#endif

    /**
     * Cleanup
     */

#ifdef EPD
    EPD_0583_1_Clear();

    EPD_0583_1_Sleep();

    free(image_red);
    free(image_black);

    DEV_Module_Exit();
#endif

    bdf_free(font);
    return 0;
}
