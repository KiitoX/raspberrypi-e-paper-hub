//
// Created by emma on 27/03/2021.
//

#define _XOPEN_SOURCE
#include <time.h>
#undef _XOPEN_SOURCE

#include "calendar.h"

#include <yder.h>
#include <ulfius.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

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
    week.start_string = calloc(len + 1, sizeof(*week.start_string));
    assert(week.start_string != NULL);
    strncpy(week.start_string, date_buf, len);
    printf("Starting at: %s\n", week.start_string);

    len = strftime(date_buf, len_buf - 1, "%FT%T%z", &week.end);
    week.end_string = calloc(len + 1, sizeof(*week.end_string));
    assert(week.end_string != NULL);
    strncpy(week.end_string, date_buf, len);
    printf("Ending at: %s\n", week.end_string);

    return week;
}

void free_week(t_week week) {
    free(week.start_string);
    free(week.end_string);
}

t_google_calendar g_calendar = {0};

void load_calendar_list() {
    json_t *j_resp = api_request("https://www.googleapis.com/calendar/v3/users/me/calendarList");
    if (j_resp != NULL) {
        json_t *j_elem;
        size_t i;
        if (json_is_array(json_object_get(j_resp, "items"))) {
            g_calendar.num_calendars = json_array_size(json_object_get(j_resp, "items"));
            g_calendar.calendars = calloc(g_calendar.num_calendars, sizeof(*g_calendar.calendars));
            assert(g_calendar.calendars != NULL);

            json_array_foreach(json_object_get(j_resp, "items"), i, j_elem) {
                if (json_is_boolean(json_object_get(j_elem, "hidden"))) {
                    // only store calendars not hidden by the user
                    --g_calendar.num_calendars;
                } else {
                    struct calendar *cal = &g_calendar.calendars[i];

                    json_t *id = json_object_get(j_elem, "id");
                    assert(json_is_string(id));
                    cal->id = calloc(json_string_length(id) + 1, sizeof(*cal->id));
                    assert(cal->id != NULL);
                    strncpy(cal->id, json_string_value(id), json_string_length(id));

                    json_t *summary = json_object_get(j_elem, "summary");
                    assert(json_is_string(summary));
                    cal->name = calloc(json_string_length(summary) + 1, sizeof(*cal->name));
                    assert(cal->name != NULL);
                    strncpy(cal->name, json_string_value(summary), json_string_length(summary));

                    if (json_is_boolean(json_object_get(j_elem, "primary")) &&
                        json_boolean_value(json_object_get(j_elem, "primary"))) {
                        g_calendar.primary = cal;
                    }
                }
            }

            g_calendar.calendars = reallocarray(g_calendar.calendars, g_calendar.num_calendars, sizeof(*g_calendar.calendars));
            assert(g_calendar.num_calendars == 0 || g_calendar.calendars != NULL);
        }
        json_decref(j_resp);
    }
}

void load_user_settings() {
    json_t *j_resp = api_request("https://www.googleapis.com/calendar/v3/users/me/settings");
    if (j_resp != NULL) {
        json_t *j_elem;
        size_t i;
        json_t *key_tz = json_string("timezone");
        json_t *key_week = json_string("weekStart");
        if (json_is_array(json_object_get(j_resp, "items"))) {
            json_array_foreach(json_object_get(j_resp, "items"), i, j_elem) {
                json_t *id = json_object_get(j_elem, "id");
                json_t *value = json_object_get(j_elem, "value");
                if (json_is_string(id) && json_is_string(value)) {
                    if (json_equal(id, key_tz)) {
                        g_calendar.time_zone = calloc(json_string_length(value) + 1, sizeof(*g_calendar.time_zone));
                        assert(g_calendar.time_zone != NULL);
                        strncpy(g_calendar.time_zone, json_string_value(value), json_string_length(value));
                    } else if (json_equal(id, key_week)) {
                        // week start: "0": Sunday, "1": Monday, "6": Saturday
                        const char *week_start = json_string_value(value);
                        errno = 0;
                        g_calendar.week_start = (int)strtol(week_start, NULL, 10);
                        assert(errno == 0);
                    }
                }
            }
        }
        json_decref(key_tz);
        json_decref(key_week);
        json_decref(j_resp);
    }
}

void load_events() {
    char url_buffer[256];
    size_t buf_size = 256;

    for (int j = 0; j < g_calendar.num_calendars; ++j) {
        struct calendar *cal = &g_calendar.calendars[j];

        snprintf(url_buffer, buf_size, "https://www.googleapis.com/calendar/v3/calendars/%s/events", cal->id);

        // the results may be paginated, we will want to go make sure we get everything here
        struct _u_request req = init_api_request(url_buffer);
        ulfius_set_request_properties(&req,
                                      U_OPT_URL_PARAMETER, "singleEvents", "true", // expand recurring events
                                      U_OPT_URL_PARAMETER, "orderBy", "startTime",
                                      U_OPT_URL_PARAMETER, "timeMin", g_calendar.week.start_string,
                                      U_OPT_URL_PARAMETER, "timeMax", g_calendar.week.end_string,
                                      U_OPT_NONE);
        json_t *j_resp = get_api_response(req);
        if (j_resp != NULL) {
            json_t *j_elem;
            size_t i;
            if (json_is_array(json_object_get(j_resp, "items"))) {
                cal->num_events = json_array_size(json_object_get(j_resp, "items"));
                if (cal->num_events == 0) {
                    continue;
                }
                cal->events = calloc(cal->num_events, sizeof(*cal->events));
                assert(cal->events != NULL);

                json_array_foreach(json_object_get(j_resp, "items"), i, j_elem) {
                    struct event *evt = &cal->events[i];
                    json_t *summary = json_object_get(j_elem, "summary");
                    assert(json_is_string(summary));
                    evt->name = calloc(json_string_length(summary) + 1, sizeof(*evt->name));
                    assert(evt->name != NULL);
                    strncpy(evt->name, json_string_value(summary), json_string_length(summary));

                    json_t *description = json_object_get(j_elem, "description");
                    if (json_is_string(description)) {
                        evt->description = calloc(json_string_length(description) + 1, sizeof(*evt->description));
                        assert(evt->description != NULL);
                        strncpy(evt->description, json_string_value(description), json_string_length(description));
                    }

                    json_t *start = json_object_get(j_elem, "start");
                    assert(json_is_object(start));
                    json_t *start_date = json_object_get(start, "date");
                    json_t *start_datetime = json_object_get(start, "dateTime");
                    if (json_is_string(start_datetime)) {
                        evt->all_day = false;
                        assert(strptime(json_string_value(start_datetime), "%FT%T%z", &evt->start) != NULL);
                    } else if (json_is_string(start_date)) {
                        evt->all_day = true;
                        assert(strptime(json_string_value(start_date), "%F", &evt->start) != NULL);
                    } else assert(false);

                    json_t *end = json_object_get(j_elem, "end");
                    assert(json_is_object(end));
                    json_t *end_date = json_object_get(end, "date");
                    json_t *end_datetime = json_object_get(end, "dateTime");
                    if (json_is_string(end_datetime)) {
                        assert(strptime(json_string_value(end_datetime), "%FT%T%z", &evt->end) != NULL);
                    } else if (json_is_string(end_date)) {
                        assert(strptime(json_string_value(end_date), "%F", &evt->end) != NULL);
                    } else assert(false);
                }
            }
            json_decref(j_resp);
        }
    }
}

void init_google_calendar() {
    g_calendar.num_calendars = 0;
    g_calendar.week_start = DAY_SUNDAY;

    g_calendar.time_zone = NULL;

    g_calendar.primary = NULL;

    load_calendar_list();
    load_user_settings();

    g_calendar.week = get_week(g_calendar.time_zone, g_calendar.week_start);
}

void destroy_google_calendar() {
    free_week(g_calendar.week);

    for (int i = 0; i < g_calendar.num_calendars; ++i) {
        free(g_calendar.calendars[i].id);
        free(g_calendar.calendars[i].name);

        for (int j = 0; j < g_calendar.calendars[i].num_events; ++j) {
            free(g_calendar.calendars[i].events[j].name);
            free(g_calendar.calendars[i].events[j].description);
        }

        free(g_calendar.calendars[i].events);
    }

    free(g_calendar.calendars);

    free(g_calendar.time_zone);
}

#ifdef EPD

bdf_t *font_large;
bdf_t *font_large_mono;
bdf_t *font_medium;
bdf_t *font_small;
bdf_t *font_icons;

void init_calendar() {
    font_large = bdf_read("./fonts/LodeSans-15.bdf", 2);
    font_large_mono = bdf_read("./fonts/LodeSansMono-15.bdf", 2);
    font_medium = bdf_read("./fonts/cozette.bdf", 2);
    font_small = bdf_read("./fonts/cozette.bdf", 1);
    // font_small = bdf_read("./fonts/scientifica-11.bdf", 1); // this one is /really/ small
    font_icons = bdf_read("./fonts/siji.bdf", 2);
}

void destroy_calendar() {
    bdf_free(font_large);
    bdf_free(font_large_mono);
    bdf_free(font_medium);
    bdf_free(font_small);
    bdf_free(font_icons);
}

const int hour_start = 7;
const int hour_segments = 14;

void draw_calendar(UBYTE *image_black, UBYTE *image_red) {
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
    struct tm hour = {0, .tm_hour = hour_start};
    for (i = 0; i < hour_segments; ++i) {
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

        struct tm day = g_calendar.week.start;
        day.tm_mday += i;
        mktime(&day);

        // highlight the current day
        if (day.tm_mday == g_calendar.week.today.tm_mday) {
            Paint_SelectImage(image_red);
        }

        // short weekday name
        strftime(buf, buf_size, "%a", &day);
        Paint_DrawString(x + 1, 73, buf, font_medium, BLACK, WHITE);

        // day of month
        strftime(buf, buf_size, "%d", &day);
        Paint_DrawString(x + 48, 68, buf, font_large_mono, BLACK, WHITE);

        if (day.tm_mday == g_calendar.week.today.tm_mday) {
            Paint_SelectImage(image_black);
        }
    }

    // white horizontal separator
    Paint_DrawLine(14, 101, w - 14, 101, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // header
    strftime(buf, buf_size, "Calendar - %d %B %Y", &g_calendar.week.today);
    Paint_DrawString(20, 20, buf, font_large, BLACK, WHITE);
}

void draw_event(UBYTE *image_black, UBYTE *image_red, const char *text, uint16_t offset, uint16_t length, uint16_t day_of_week) {
    // the offset and length is in 30 min segments, which are round down/up

    uint16_t x = 65 + day_of_week * 82;
    uint16_t w = 74;
    uint16_t y = 104 + (offset / 2) * 26 + (offset % 2) * 12;
    uint16_t skip_borders = (offset % 2) ? ((length / 2) * 3 + ((length - 1) / 2) * 1) : ((length / 2) * 1 + ((length - 1) / 2) * 3);
    uint16_t h = length * 11 + skip_borders;

    int weekday_today = g_calendar.week.today.tm_wday;
    bool today = day_of_week == weekday_today;
    bool passed = day_of_week < weekday_today || (today && (offset + length) < 6);

    if (today && !passed) {
        Paint_SelectImage(image_red);
        Paint_DrawRectangle(x, y, x + w, y + h, RED, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    } else {
        Paint_SelectImage(image_black);
        Paint_DrawRectangle(x, y, x + w, y + h, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }

    if (!today && !passed) {
        Paint_SelectImage(image_red);
        Paint_DrawStringRect(x - 2, y - 2, text, font_small, RED, WHITE, w + 4, h + 4, true, 12);
    } else if (today && !passed) {
        Paint_SelectImage(image_red);
        Paint_DrawStringRect(x - 2, y - 2, text, font_small, WHITE, WHITE, w + 4, h + 4, true, 12);
        Paint_SelectImage(image_black);
        Paint_DrawStringRect(x - 2, y - 2, text, font_small, WHITE, WHITE, w + 4, h + 4, true, 12);
    } else {
        Paint_SelectImage(image_black);
        Paint_DrawStringRect(x - 2, y - 2, text, font_small, BLACK, WHITE, w + 4, h + 4, true, 12);
    }
}

void draw_events(UBYTE *image_black, UBYTE *image_red) {
    for (int i = 0; i < g_calendar.num_calendars; ++i) {
        struct calendar *cal = &g_calendar.calendars[i];

        for (int j = 0; j < cal->num_events; ++j) {
            struct event *evt = &cal->events[j];

            if (evt->all_day) {
                // TODO
            } else {
                int offset = (evt->start.tm_hour - hour_start) * 2 + (evt->start.tm_min) / 30;
                int length = (evt->end.tm_hour - evt->start.tm_hour) * 2 + (evt->end.tm_min - evt->start.tm_min) / 30;

                draw_event(image_black, image_red, evt->name, offset, length, evt->start.tm_wday);
            }
        }
    }
}

#endif
