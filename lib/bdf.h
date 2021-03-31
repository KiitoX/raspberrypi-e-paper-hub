//
// Created by emma on 22/03/2021.
//

#ifndef EPAPER_BDF_H
#define EPAPER_BDF_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>

typedef wchar_t encoding_t;

typedef struct {
#ifdef BDF_CHARNAME
    char *name; // limited to 14 chars in bdf 2.1
#endif
    uint8_t deviceWidth; // x offset for the next character
    // bitmap bounds
    uint8_t width; // in bits/pixels
    uint8_t height; // in bits/pixels
    // bitmap offset from origin
    int8_t offsetX;
    int8_t offsetY;
    encoding_t encoding;
    uint8_t *bits;
} bitmap_t;

typedef struct {
    uint8_t size; // point size
    char *familyName;
    uint8_t ascent; // pixels above the baseline
    uint8_t descent; // pixels below the baseline, these two sum up to size
    uint8_t defaultChar;
    uint8_t scale;
    // global character bounds
    uint8_t width; // in bits/pixels
    uint8_t height; // in bits/pixels
    // origin offset from bottom left corner
    int8_t offsetX;
    int8_t offsetY;
    size_t numChars;
    bitmap_t *characters;
} bdf_t;

bdf_t *bdf_read(const char *file_name, uint8_t scale);
void bdf_free(bdf_t *font);

bitmap_t *bdf_get_bitmap(bdf_t *font, encoding_t encoding);

void bdf_print_bitmap(bdf_t *font, bitmap_t *bitmap);

#endif //EPAPER_BDF_H
