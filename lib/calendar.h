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

#define DAY_NAME ((char []){"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"})

typedef struct {
    char *start, *end;
} t_week_boundary;

t_week_boundary get_week_boundaries(const char *time_zone, int week_start);
void free_week_boundaries(t_week_boundary boundary);

#ifdef EPD

#include "epd/ER-EPD0583-1.h"
#include "epd/GUI_Paint.h"

void draw_calendar(UBYTE *image_black, UBYTE *image_red);

#endif