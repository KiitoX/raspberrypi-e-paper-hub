//
// Created by emma on 27/03/2021.
//

#pragma once

#include <iddawc.h>
#include <jansson.h>
#include <stdbool.h>

#define JSON_DEBUG(json) char *debug_dump = json_dumps(json, JSON_INDENT(2)); puts(debug_dump); free(debug_dump)

int create_session();
struct _u_request init_api_request(const char *url);
json_t *get_api_response(struct _u_request req);
json_t *api_request(const char *url);
void close_session();

#define DAY_SUNDAY 0
#define DAY_MONDAY 1
#define DAY_TUESDAY 2
#define DAY_WEDNESDAY 3
#define DAY_THURSDAY 4
#define DAY_FRIDAY 5
#define DAY_SATURDAY 6

typedef struct {
    char *start_string, *end_string;
    struct tm start, end, today;
} t_week;

t_week get_week(const char *time_zone, int week_start);
void free_week(t_week week);

typedef struct {
    char *time_zone;
    int week_start;
    t_week week;
    size_t num_calendars;
    struct calendar {
        char *id;
        char *name;
        size_t num_events;
        struct event {
            char *name;
            char *description;
            bool all_day; // true if start and end are date only
            struct tm start, end;
        } *events;
    } *calendars;
    struct calendar *primary;
} t_google_calendar;

void init_google_calendar();
void destroy_google_calendar();

void load_events();

#ifdef EPD

#include "epd/ER-EPD0583-1.h"
#include "epd/GUI_Paint.h"

void init_calendar();
void destroy_calendar();

void draw_calendar(UBYTE *image_black, UBYTE *image_red);
void draw_events(UBYTE *image_black, UBYTE *image_red);

#endif