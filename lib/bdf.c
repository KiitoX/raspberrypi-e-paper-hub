//
// Created by emma on 22/03/2021.
//

#include "bdf.h"

enum token {
    ASD
};

bool starts_with(const char *s, const char *prefix) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

void read_header(char *buffer, size_t buf_len, FILE *bdf_file, bdf_t *font) {
    assert(starts_with(buffer, "STARTFONT 2.1"));

    do {
        assert(fgets(buffer, buf_len, bdf_file) != NULL);

        if (starts_with(buffer, "SIZE")) {
#ifdef BDF_DEBUG
            printf("read: %s", buffer);
#endif
            assert(sscanf(buffer, "SIZE %hhu", &(font->size)) == 1);
#ifdef BDF_DEBUG
            printf("scan: size = %hhu\n\n", font->size);
#endif
        } else if (starts_with(buffer, "FONTBOUNDINGBOX")) {
#ifdef BDF_DEBUG
            printf("read: %s", buffer);
#endif
            assert(sscanf(buffer, "FONTBOUNDINGBOX %hhu %hhu %hhd %hhd",
                          &(font->width), &(font->height),
                          &(font->offsetX), &(font->offsetY)) == 4);
#ifdef BDF_DEBUG
            printf("scan: w = %hhu, h = %hhu, x = %hhd, y = %hhd\n\n",
                   font->width, font->height,
                   font->offsetX, font->offsetY);
#endif
        }
    } while (!starts_with(buffer, "STARTPROPERTIES"));
}

void read_properties(char *buffer, size_t buf_len, FILE *bdf_file, bdf_t *font) {
    do {
        assert(fgets(buffer, buf_len, bdf_file) != NULL);

        if (starts_with(buffer, "FONT_ASCENT")) {
#ifdef BDF_DEBUG
            printf("read: %s", buffer);
#endif
            assert(sscanf(buffer, "FONT_ASCENT %hhu", &(font->ascent)) == 1);
#ifdef BDF_DEBUG
            printf("scan: ascent = %hhu\n\n", font->ascent);
#endif
        } else if (starts_with(buffer, "FONT_DESCENT")) {
#ifdef BDF_DEBUG
            printf("read: %s", buffer);
#endif
            assert(sscanf(buffer, "FONT_DESCENT %hhu", &(font->descent)) == 1);
#ifdef BDF_DEBUG
            printf("scan: descent = %hhu\n\n", font->descent);
#endif
        } else if (starts_with(buffer, "DEFAULT_CHAR")) {
#ifdef BDF_DEBUG
            printf("read: %s", buffer);
#endif
            assert(sscanf(buffer, "DEFAULT_CHAR %hhu", &(font->defaultChar)) == 1);
#ifdef BDF_DEBUG
            printf("scan: default = %hhu\n\n", font->defaultChar);
#endif
        } else if (starts_with(buffer, "FAMILY_NAME")) {
#ifdef BDF_DEBUG
            printf("read: %s", buffer);
#endif
            font->familyName = calloc(96, sizeof(*font->familyName));
            assert(sscanf(buffer, "FAMILY_NAME \"%[^\"]\"", font->familyName) == 1);
#ifdef BDF_DEBUG
            printf("scan: familyName = '%s'\n\n", font->familyName);
#endif
        }
    } while (!starts_with(buffer, "ENDPROPERTIES"));
}

void read_character(char *buffer, size_t buf_len, FILE *bdf_file, bdf_t *font, bitmap_t *bitmap) {
    assert(fgets(buffer, buf_len, bdf_file) != NULL);
    assert(starts_with(buffer, "STARTCHAR"));
#ifdef BDF_CHARNAME
#ifdef BDF_DEBUG
    printf("read: %s", buffer);
#endif
    assert(sscanf(buffer, "STARTCHAR %ms", &bitmap->name) == 1);
#ifdef BDF_DEBUG
    printf("scan: charname = %s\n\n", bitmap->name);
#endif
#endif

    do {
        assert(fgets(buffer, buf_len, bdf_file) != NULL);

        if (starts_with(buffer, "ENCODING")) {
#ifdef BDF_DEBUG
            printf("read: %s", buffer);
#endif
            assert(sscanf(buffer, "ENCODING %hu", &(bitmap->encoding)) == 1);
#ifdef BDF_DEBUG
            printf("scan: encoding = %hu\n\n", bitmap->encoding);
#endif
        } else if (starts_with(buffer, "DWIDTH")) {
#ifdef BDF_DEBUG
            printf("read: %s", buffer);
#endif
            assert(sscanf(buffer, "DWIDTH %hhu", &(bitmap->deviceWidth)) == 1);
#ifdef BDF_DEBUG
            printf("scan: width = %hhu\n\n", bitmap->deviceWidth);
#endif
        } else if (starts_with(buffer, "BBX")) {
#ifdef BDF_DEBUG
            printf("read: %s", buffer);
#endif
            assert(sscanf(buffer, "BBX %hhu %hhu %hhd %hhd",
                          &(bitmap->width), &(bitmap->height),
                          &(bitmap->offsetX), &(bitmap->offsetY)) == 4);
#ifdef BDF_DEBUG
            printf("scan: w = %hhu, h = %hhu, x = %hhd, y = %hhd\n\n",
                   bitmap->width, bitmap->height,
                   bitmap->offsetX, bitmap->offsetY);
#endif
        }
    } while (!starts_with(buffer, "BITMAP"));

    // round the width to the nearest multiple of eight required to fit it
    size_t byte_width = ((bitmap->width + 7) / 8);
    bitmap->bits = malloc(sizeof(*bitmap->bits) * byte_width * bitmap->height);
    assert(bitmap->bits != NULL);

    char byte[3] = "00\0";

    errno = 0;
    for (int y = 0; y < bitmap->height; ++y) {
        assert(fgets(buffer, buf_len, bdf_file) != NULL);
        for (int x = 0; x < byte_width; ++x) {
            strncpy(byte, buffer + x * 2, 2);
            bitmap->bits[y * byte_width + x] = strtol(byte, NULL, 16);
            assert(errno == 0);
        }
    }

    assert(fgets(buffer, buf_len, bdf_file) != NULL);
    assert(starts_with(buffer, "ENDCHAR"));
}

bdf_t *bdf_read(const char *file_name, uint8_t scale) {
    size_t buf_len = 250;
    char *buffer = malloc(sizeof(*buffer) * buf_len);
    assert(buffer != NULL);

    FILE *bdf_file = fopen(file_name, "r");
    assert(bdf_file != NULL);

    assert(fgets(buffer, buf_len, bdf_file) != NULL);

    bdf_t *font = malloc(sizeof(*font));
    assert(font != NULL);
    font->defaultChar = 0;
    font->scale = scale;

    read_header(buffer, buf_len, bdf_file, font);
    read_properties(buffer, buf_len, bdf_file, font);

    assert(fgets(buffer, buf_len, bdf_file) != NULL);
#ifdef DEBUG
    printf("read: %s", buffer);
#endif
    assert(starts_with(buffer, "CHARS"));
    assert(sscanf(buffer, "CHARS %lu", &(font->numChars)) == 1);
#ifdef DEBUG
    printf("scan: chars = %lu\n\n", font->numChars);
#endif

    font->characters = malloc(sizeof(*font->characters) * font->numChars);
    assert(font->characters != NULL);

    for (int i = 0; i < font->numChars; ++i) {
        read_character(buffer, buf_len, bdf_file, font, &font->characters[i]);
    }

    assert(fgets(buffer, buf_len, bdf_file) != NULL);
    assert(starts_with(buffer, "ENDFONT"));

    fclose(bdf_file);
    free(buffer);

    return font;
}

void bdf_free(bdf_t *font) {
    for (int i = 0; i < font->numChars; ++i) {
#ifdef BDF_CHARNAME
        free(font->characters[i].name);
#endif
        free(font->characters[i].bits);
    }

    free(font->characters);
    free(font->familyName);
    free(font);
}

size_t clamp(size_t val, size_t lower, size_t upper) {
    return (val <= lower) ? lower : ((val > upper) ? upper : val);
}

int compare_encoding(const void *a, const void *b) {
    return (*((encoding_t *)a)) - (((bitmap_t *)b)->encoding);
}

// binary search
bitmap_t *bdf_get_bitmap(bdf_t *font, encoding_t encoding) {
    printf("Looking up character '%lc' (%d) in %s (%ld)\n", encoding, encoding, font->familyName, font->numChars);
    return bsearch(&encoding, font->characters, font->numChars, sizeof(*font->characters), compare_encoding);
}

// similar to interpolation search
bitmap_t *bdf_get_bitmap_alt(bdf_t *font, encoding_t encoding) {
    size_t i = 0, lower = 0, upper = font->numChars - 1, diff;
    bitmap_t *selected;
#ifdef DEBUG
    size_t n = 0;
#endif
    do {
        selected = &font->characters[i];
        diff = encoding - selected->encoding;
        if (diff >= 0) {
            lower = i;
        }
        if (diff <= 0) {
            upper = i;
        }
        i = clamp(i + diff, lower, upper);
#ifdef DEBUG
        n++;
#endif
    } while (lower < upper);
#ifdef DEBUG
    printf("selection iterations: %ld\n", n);
#endif

    if (selected->encoding == encoding) {
        return selected;
    } else {
        return NULL;
    }
}

void bdf_print_bitmap(bdf_t *font, bitmap_t *bitmap) {
    size_t byte_width = ((bitmap->width + 7) / 8);

    for (int y = 0; y < font->height * font->scale; ++y) {
        for (int x = 0; x < font->width * font->scale; ++x) {
            int bitmapX = (x / font->scale) + font->offsetX - bitmap->offsetX;
            int bitmapY = (y / font->scale) - font->offsetY + bitmap->offsetY + bitmap->height - font->height;

            if (0 <= bitmapX && bitmapX < bitmap->width &&
                0 <= bitmapY && bitmapY < bitmap->height) {
                if (((bitmap->bits[bitmapY * byte_width + (bitmapX / 8)] << (bitmapX % 8)) & 0x80) > 0) {
                    printf("██");
                } else {
                    printf("▒▒");
                }
            } else {
                printf("░░");
            }
        }
        puts("");
    }
}