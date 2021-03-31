#include <stdio.h>
#include <signal.h>
#include <locale.h>

#include "lib/bdf.h"
#include "lib/calendar.h"

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

    // disable locale specific string handling
    // enables UTF-8 to multi-byte handling
    setlocale(LC_ALL, "");

#ifdef EPD
    signal(SIGINT, signal_handler);
#endif

    char *file = "./fonts/cozette.bdf";

    bdf_t *font = bdf_read(file, 1);

#ifdef GAPI_TEST
    create_session();
#endif

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

    Paint_NewImage(image_black, EPD_0583_1_WIDTH, EPD_0583_1_HEIGHT, ROTATE_180, WHITE);
    Paint_NewImage(image_red, EPD_0583_1_WIDTH, EPD_0583_1_HEIGHT, ROTATE_180, WHITE);
#endif

    /**
     * Main
     */

#ifdef GAPI_TEST
    size_t i;
    json_t *j_resp = NULL, *j_elem;

    j_resp = api_request("https://www.googleapis.com/calendar/v3/users/me/calendarList");
    if (j_resp != NULL) {
        if (json_is_array(json_object_get(j_resp, "items"))) {
            json_array_foreach(json_object_get(j_resp, "items"), i, j_elem) {
                // all of the config stuff contained herein is sent again with events.list
                if (json_is_string(json_object_get(j_elem, "id")) &&
                    json_is_string(json_object_get(j_elem, "summary"))) {
                    printf("%s: '%s'\n",
                           json_string_value(json_object_get(j_elem, "id")),
                           json_string_value(json_object_get(j_elem, "summary")));
                    // TODO add to a list or something
                }
            }
        }
        json_decref(j_resp);
    }

    const char *settingsListGet = "https://www.googleapis.com/calendar/v3/users/me/settings";
    char *time_zone = NULL;
    int week_start = DAY_SUNDAY;
    j_resp = api_request(settingsListGet);
    if (j_resp != NULL) {
        if (json_is_array(json_object_get(j_resp, "items"))) {
            json_array_foreach(json_object_get(j_resp, "items"), i, j_elem) {
                if (json_is_string(json_object_get(j_elem, "id")) &&
                    json_is_string(json_object_get(j_elem, "value"))) {
                    if (0 == strcmp(json_string_value(json_object_get(j_elem, "id")), "timezone")) {
                        const char *tzBuf = json_string_value(json_object_get(j_elem, "value"));
                        time_zone = calloc(strlen(tzBuf) + 1, sizeof(*time_zone));
                        strcpy(time_zone, tzBuf);
                    } else if (0 == strcmp(json_string_value(json_object_get(j_elem, "value")), "weekStart")) {
                        const char *weekBuf = json_string_value(json_object_get(j_elem, "value"));
                        // week start: "0": Sunday, "1": Monday, "6": Saturday
                        errno = 0;
                        week_start = (int)strtol(weekBuf, NULL, 10);
                        if (errno != 0) {
                            printf("Failed to convert %s to integer.\n", weekBuf);
                        }
                    }
                }
            }
        }
        json_decref(j_resp);
    }

    // this is paginated, we will want to go through until we're done, though also limit it to &current_week
    const char *eventsListGet = "https://www.googleapis.com/calendar/v3/calendars/manuel.manu.delfin@gmail.com/events";

    t_week_boundary boundary = get_week_boundaries(time_zone, week_start);

    struct _u_request req = init_api_request(eventsListGet);
    ulfius_set_request_properties(&req,
                                  U_OPT_URL_PARAMETER, "singleEvents", "true", // expand recurring events
                                  U_OPT_URL_PARAMETER, "orderBy", "startTime",
                                  U_OPT_URL_PARAMETER, "timeMin", boundary.start,
                                  U_OPT_URL_PARAMETER, "timeMax", boundary.end,
                                  U_OPT_NONE);
    j_resp = get_api_response(req);
    if (j_resp != NULL) {
        JSON_DEBUG(j_resp);

        json_decref(j_resp);
    }
    free_week_boundaries(boundary);
    free(time_zone);
#endif

    draw_calendar(image_black, image_red);

#ifdef BDF_TEST
    puts("");
    printf("font: %ld characters, %hhd pt, bounds: %hhu %hhu %hhd %hhd\n", font->numChars, font->size, font->width, font->height, font->offsetX, font->offsetY);
    puts("");

    for (int i = 0; i < 250; ++i) {
        bdf_print_bitmap(font, &font->characters[i]);
        puts("");
    }

    bdf_print_bitmap(font, bdf_get_bitmap(font, 1033));
#endif

#ifdef EPD_TEST
    // Draw black image
    Paint_SelectImage(image_black);
    Paint_Clear(WHITE);

    Paint_DrawCharmap(10, 10, font, 0, BLACK, WHITE);

    // Draw red picture
    Paint_SelectImage(image_red);
    Paint_Clear(WHITE);

    EPD_0583_1_Display(image_black, image_red);
    DEV_Delay_ms(30000);

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

    Paint_DrawString(350, 10, "abcdefghijklmnopqrstuvwxyz\nABCDEFGHIJKLMNOPQRSTUVWXYZ", font, BLACK, WHITE);
    Paint_DrawString(10, 335, "testing the black font", font, BLACK, WHITE);

    // Draw red picture
    Paint_SelectImage(image_red);
    Paint_Clear(WHITE);

    Paint_DrawRectangle(50, 50, 150, 150, RED, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(200, 50, 300, 150, RED, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    Paint_DrawCircle(100, 250, 50, RED, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(250, 250, 50, RED, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    Paint_DrawString(10, 310, "hello! e-paper", font, WHITE, RED);
    Paint_DrawString(10, 360, "0123456789", font, RED, WHITE);

    EPD_0583_1_Display(image_black, image_red);
    DEV_Delay_ms(3000);
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

#ifdef GAPI_TEST
    close_session();
#endif

    bdf_free(font);
    return 0;
}
