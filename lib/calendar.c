//
// Created by emma on 27/03/2021.
//

#include "calendar.h"

#include <yder.h>
#include <ulfius.h>
#include <string.h>

#define GAPI_CONFIG_ENDPOINT "https://accounts.google.com/.well-known/openid-configuration"
// define GAPI_CLIENT_ID, GAPI_CLIENT_SECRET with your own values here
#define GAPI_CLIENT_ID "96898537413-383r4icp8k0n6l9ndpo0vrjbo59cqadj.apps.googleusercontent.com"
#define GAPI_CLIENT_SECRET "_BnBe0J2Bk1uN4mobWUwvIAe"

struct _i_session i_session;

int create_session() {
    int ret;

    i_global_init();

    y_init_logs("Iddawc", Y_LOG_MODE_CONSOLE, Y_LOG_LEVEL_DEBUG, NULL, "Starting Iddawc client program");

    i_init_session(&i_session);
    // Details on these parameters:
    // https://developers.google.com/identity/protocols/oauth2
    // https://developers.google.com/calendar/auth
    // https://calendar-json.googleapis.com/$discovery/rest?version=v3
    i_set_parameter_list(&i_session,
                         I_OPT_RESPONSE_TYPE, I_RESPONSE_TYPE_CODE,
                         I_OPT_OPENID_CONFIG_ENDPOINT, GAPI_CONFIG_ENDPOINT,
                         I_OPT_CLIENT_ID, GAPI_CLIENT_ID,
                         I_OPT_CLIENT_SECRET, GAPI_CLIENT_SECRET,
                         I_OPT_REDIRECT_URI, "https://emma.click/oauth2redirect",
                         I_OPT_AUTH_ENDPOINT, "https://accounts.google.com/o/oauth2/auth",
                         I_OPT_TOKEN_ENDPOINT, "https://accounts.google.com/o/oauth2/token",
                         I_OPT_REVOCATION_ENDPOINT, "https://accounts.google.com/o/oauth2/revoke",
                         I_OPT_SCOPE, "https://www.googleapis.com/auth/calendar.readonly " // note the space at the end
                                      "https://www.googleapis.com/auth/calendar.events.readonly "
                                      "https://www.googleapis.com/auth/calendar.settings.readonly",
                         I_OPT_STATE_GENERATE, 16,
                         I_OPT_NONCE_GENERATE, 32,
                         I_OPT_NONE);

    if (i_load_openid_config(&i_session) != I_OK) {
        printf("Error loading openid-config\n");
        close_session();
        return 1;
    }

    json_t *scopes_supported = json_object_get(i_session.openid_config, "scopes_supported");

    // the only openid config for gapi I was able to find, does not contain any specialised scopes.
    // therefore I'm adding the ones I'm using here manually, so that iddawc does not complain.
    // alternatively you can disable this (and some other) strict sanity checks by adding
    // I_OPT_OPENID_CONFIG_STRICT, I_STRICT_NO,
    // to the parameter list above
    json_array_append_new(scopes_supported, json_string("https://www.googleapis.com/auth/calendar.readonly"));
    json_array_append_new(scopes_supported, json_string("https://www.googleapis.com/auth/calendar.events.readonly"));
    json_array_append_new(scopes_supported, json_string("https://www.googleapis.com/auth/calendar.settings.readonly"));

    if ((ret = i_build_auth_url_get(&i_session)) != I_OK) {
        printf("Error building auth request %d\n", ret);
        close_session();
        return 1;
    }

    printf("Redirect to %s\n", i_get_str_parameter(&i_session, I_OPT_REDIRECT_TO));

    printf("Enter Redirect URL: ");
    char redirect_url[4097] = {0};
    fgets(redirect_url, 4096, stdin);
    redirect_url[strlen(redirect_url) - 1] = '\0';

    printf("Got redirect [%lu]'%s'\n", strlen(redirect_url), redirect_url);

    time_t now = time(NULL);
    i_set_str_parameter(&i_session, I_OPT_REDIRECT_TO, redirect_url);
    if (i_parse_redirect_to(&i_session) != I_OK) {
        printf("Error parsing redirect url\n");
        close_session();
        return 1;
    }

    if (i_run_token_request(&i_session) != I_OK) {
        printf("Error running token request\n");
        close_session();
        return 1;
    }

    // this is something the lib is supposed to do itself, but it doesn't so eyyyy
    i_session.expires_at = now + ((time_t)i_session.expires_in);

    return 0;
}

struct _u_request init_api_request(const char *url) {
    struct _u_request req;

    ulfius_init_request(&req);

    ulfius_set_request_properties(&req,
                                  U_OPT_HTTP_VERB, "GET",
                                  U_OPT_HTTP_URL, url,
                                  U_OPT_NONE);

    return req;
}

json_t *get_api_response(struct _u_request req) {
    struct _u_response resp;
    json_t *j_resp = NULL;

    ulfius_init_response(&resp);

    int ret = i_perform_api_request(&i_session, &req, &resp, true, I_BEARER_TYPE_HEADER, false, false);
    if (ret == I_OK && resp.status == 200) {
        j_resp = ulfius_get_json_body_response(&resp, NULL);
    }

    ulfius_clean_request(&req);
    ulfius_clean_response(&resp);

    return j_resp;
}

json_t *api_request(const char *url) {
    struct _u_request req = init_api_request(url);
    return get_api_response(req);
}

void close_session() {
    i_clean_session(&i_session);

    y_close_logs();

    i_global_close();
}

t_week get_week(const char *time_zone, int week_start) {
    t_week week = {0};

    if (time_zone != NULL) {
        setenv("TZ", time_zone, true);
        printf("Setting timezone: %s\n", time_zone);
    }
    tzset(); // should be called before localtime_r

    time_t now = time(NULL);
    localtime_r(&now, &week.today);

    week.start.tm_year = week.today.tm_year;
    week.start.tm_mon = week.today.tm_mon;
    week.start.tm_mday = week.today.tm_mday - week.today.tm_wday + week_start;
    week.start.tm_isdst = -1; // let mktime figure DST
    mktime(&week.start);

    week.end.tm_year = week.start.tm_year;
    week.end.tm_mon = week.start.tm_mon;
    week.end.tm_mday = week.start.tm_mday + 7;
    week.end.tm_isdst = -1; // let mktime figure DST
    mktime(&week.end);

    size_t len, len_buf = 32;
    char date_buf[len_buf];

    len = strftime(date_buf, len_buf - 1, "%FT%T%z", &week.start);
    week.start_string = calloc(len, sizeof(*week.start_string + 1));
    strncpy(week.start_string, date_buf, len);
    printf("Starting at: %s\n", week.start_string);

    len = strftime(date_buf, len_buf - 1, "%FT%T%z", &week.end);
    week.end_string = calloc(len, sizeof(*week.end_string + 1));
    strncpy(week.end_string, date_buf, len);
    printf("Ending at: %s\n", week.end_string);

    return week;
}

void free_week(t_week week) {
    free(week.start_string);
    free(week.end_string);
}

#ifdef EPD

bdf_t *font_large;
bdf_t *font_large_mono;
bdf_t *font_medium;
bdf_t *font_small;
bdf_t *font_icons;
t_week week;

UBYTE *image_black;
UBYTE *image_red;

void init_calendar(UBYTE *image_black_, UBYTE *image_red_) {
    image_black = image_black_;
    image_red = image_red_;

    font_large = bdf_read("./fonts/LodeSans-15.bdf", 2);
    font_large_mono = bdf_read("./fonts/LodeSansMono-15.bdf", 2);
    font_medium = bdf_read("./fonts/cozette.bdf", 2);
    font_small = bdf_read("./fonts/cozette.bdf", 1);
    // font_small = bdf_read("./fonts/scientifica-11.bdf", 1); // this one is /really/ small
    font_icons = bdf_read("./fonts/siji.bdf", 2);

    week = get_week(NULL, DAY_SUNDAY);
}

void destroy_calendar() {
    image_black = NULL;
    image_red = NULL;

    bdf_free(font_large);
    bdf_free(font_large_mono);
    bdf_free(font_medium);
    bdf_free(font_small);
    bdf_free(font_icons);

    free_week(week);
}

void draw_calendar() {
    int w = EPD_0583_1_WIDTH, h = EPD_0583_1_HEIGHT;

    size_t buf_size = 64;
    char buf[65] = {0};

    // Clear images
    Paint_SelectImage(image_red);
    Paint_Clear(WHITE);

    // Draw black image
    Paint_SelectImage(image_black);
    Paint_Clear(WHITE);

    int x, y, i;

    // border
    Paint_DrawRectangle(10, 10, w - 10, h - 10, BLACK, DOT_PIXEL_3X3, DRAW_FILL_EMPTY);
    // Black Background
    Paint_DrawRectangle(16, 100, w - 16, h - 15, BLACK, DOT_PIXEL_3X3, DRAW_FILL_FULL);

    // horizontal hour separators
    for (i = 0; i < 13; ++i) {
        y = 128 + i * 26;
        Paint_DrawLine(15, y, w - 15, y, WHITE, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    }

    // hour labels
    struct tm hour = {0, .tm_hour = 7};
    for (i = 0; i < 14; ++i) {
        y = 103 + i * 26;
        strftime(buf, buf_size, "%H", &hour);
        Paint_DrawString(15, y, buf, font_medium, WHITE, WHITE);
        Paint_DrawString(42, y + 2, "00", font_small, WHITE, WHITE);
        ++hour.tm_hour;
    }

    // vertical day separators
    for (i = 0; i < 7; ++i) {
        x = 61 + i * 82;
        Paint_DrawLine(x, 74, x, 103, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
        Paint_DrawLine(x, 103, x, h - 15, WHITE, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);

        struct tm day = week.start;
        day.tm_mday += i;
        mktime(&day);

        // highlight the current day
        if (day.tm_mday == week.today.tm_mday) {
            Paint_SelectImage(image_red);
        }

        // short weekday name
        strftime(buf, buf_size, "%a", &day);
        Paint_DrawString(x + 1, 73, buf, font_medium, BLACK, WHITE);

        // day of month
        strftime(buf, buf_size, "%d", &day);
        Paint_DrawString(x + 48, 68, buf, font_large_mono, BLACK, WHITE);

        if (day.tm_mday == week.today.tm_mday) {
            Paint_SelectImage(image_black);
        }
    }

    // white horizontal separator
    Paint_DrawLine(14, 101, w - 14, 101, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // header
    strftime(buf, buf_size, "Calendar - %d %B %Y", &week.today);
    Paint_DrawString(20, 20, buf, font_large, BLACK, WHITE);
}

void draw_event(char *text, bdf_t *font, uint16_t quarter_offset, uint16_t quarter_length, uint16_t day_of_week) {
    uint16_t x = 65 + day_of_week * 82;
    uint16_t w = 74;
    uint16_t y = 104 + (quarter_offset / 2) * 26 + (quarter_offset % 2) * 12;
    uint16_t skip_borders = (quarter_offset % 2) ? ((quarter_length / 2) * 3 + ((quarter_length - 1) / 2) * 1) : ((quarter_length / 2) * 1 + ((quarter_length - 1) / 2) * 3);
    uint16_t h = quarter_length * 11 + skip_borders;

    bool passed = day_of_week < DAY_WEDNESDAY || (day_of_week == DAY_WEDNESDAY && (quarter_offset + quarter_length) < 6);
    bool today = day_of_week == DAY_WEDNESDAY;

    if (today && !passed) {
        Paint_SelectImage(image_red);
        Paint_DrawRectangle(x, y, x + w, y + h, RED, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    } else {
        Paint_SelectImage(image_black);
        Paint_DrawRectangle(x, y, x + w, y + h, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }

    if (!today && !passed) {
        Paint_SelectImage(image_red);
        Paint_DrawString(x - 2, y - 2, text, font, RED, WHITE);
    } else if (today && !passed) {
        Paint_SelectImage(image_red);
        Paint_DrawString(x - 2, y - 2, text, font, WHITE, WHITE);
        Paint_SelectImage(image_black);
        Paint_DrawString(x - 2, y - 2, text, font, WHITE, WHITE);
    } else {
        Paint_SelectImage(image_black);
        Paint_DrawString(x - 2, y - 2, text, font, BLACK, WHITE);
    }
}

void draw_events() {
    draw_event("length", font_small, 0, 3, DAY_SUNDAY);
    draw_event("of", font_small, 3, 2, DAY_SUNDAY);
    draw_event("more stuff", font_small, 3, 2, DAY_TUESDAY);
    draw_event("events", font_small, 5, 1, DAY_TUESDAY);
    for (int j = 0; j < 12; ++j) {
        draw_event("loop test", font_small, j, 1, DAY_WEDNESDAY);
    }
    draw_event("Smalltalk", font_small, 3, 4, DAY_THURSDAY);
    draw_event("Smalltalk2", font_small, 7, 1, DAY_THURSDAY);
    draw_event("Smalltalk3", font_small, 8, 4, DAY_THURSDAY);
    draw_event("Some Meeting", font_small, 12, 1, DAY_THURSDAY);
    draw_event("Call me", font_small, 13, 2, DAY_THURSDAY);
}

#endif
