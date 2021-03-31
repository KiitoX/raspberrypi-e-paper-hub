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

t_week_boundary get_week_boundaries(const char *time_zone, int week_start) {
    t_week_boundary boundary = {0};

    if (time_zone != NULL) {
        setenv("TZ", time_zone, true);
        printf("Setting timezone: %s\n", time_zone);
    }
    tzset(); // should be called before localtime_r

    struct tm local, start = {0}, end = {0};
    time_t now = time(NULL);
    localtime_r(&now, &local);

    start.tm_year = local.tm_year;
    start.tm_mon = local.tm_mon;
    start.tm_mday = local.tm_mday - local.tm_wday + week_start;
    start.tm_isdst = -1; // let mktime figure DST
    mktime(&start);

    end.tm_year = start.tm_year;
    end.tm_mon = start.tm_mon;
    end.tm_mday = start.tm_mday + 7;
    end.tm_isdst = -1; // let mktime figure DST
    mktime(&end);

    size_t len, len_buf = 32;
    char date_buf[len_buf];

    len = strftime(date_buf, len_buf - 1, "%FT%T%z", &start);
    boundary.start = calloc(len, sizeof(*boundary.start + 1));
    strncpy(boundary.start, date_buf, len);
    printf("Starting at: %s\n", boundary.start);

    len = strftime(date_buf, len_buf - 1, "%FT%T%z", &end);
    boundary.end = calloc(len, sizeof(*boundary.end + 1));
    strncpy(boundary.end, date_buf, len);
    printf("Ending at: %s\n", boundary.end);

    return boundary;
}

void free_week_boundaries(t_week_boundary boundary) {
    free(boundary.start);
    free(boundary.end);
}

#ifdef EPD

void draw_calendar(UBYTE *image_black, UBYTE *image_red) {
    int w = EPD_0583_1_WIDTH, h = EPD_0583_1_HEIGHT;

    char *file = "./fonts/cozette.bdf";
    bdf_t *font3x = bdf_read(file, 3);
    bdf_t *font2x = bdf_read(file, 2);

    // Draw black image
    Paint_SelectImage(image_black);
    Paint_Clear(WHITE);

    int x, y, i;

    for (i = 0; i < 13; ++i) {
        y = 128 + i * 26;
        Paint_DrawLine(13, y, w - 13, y, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    }

    Paint_DrawRectangle(13, 13, w - 13, h - 13, BLACK, DOT_PIXEL_3X3, DRAW_FILL_EMPTY);
    Paint_DrawLine(13, 101, w - 13, 101, BLACK, DOT_PIXEL_3X3, LINE_STYLE_SOLID);

    for (i = 0; i < 7; ++i) {
        x = 61 + i * 82;
        Paint_DrawLine(x, 74, x, h - 13, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);

        Paint_DrawString(x + 1, 73, DAY_NAME(i), font2x, BLACK, WHITE);
        Paint_DrawString(x + 38, 64, "32", font3x, BLACK, WHITE);
    }

    Paint_DrawString(20, 20, "Calendar", font3x, BLACK, WHITE);

    // Draw red picture
    Paint_SelectImage(image_red);
    Paint_Clear(WHITE);

    bdf_free(font3x);
    bdf_free(font2x);

    EPD_0583_1_Display(image_black, image_red);
    DEV_Delay_ms(30000);
}

#endif
