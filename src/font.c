#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "font.h"

typedef struct {
    uint32_t offset;
    uint32_t length;
} TTFontTable;

typedef struct {
    TTFontTable cmap;
    TTFontTable glyf;
    TTFontTable head;
    TTFontTable hmtx;
    TTFontTable loca;
} TTFontTables;

struct TTFont{
    uint16_t font_size;
    size_t ttf_size;
    const uint8_t *ttf_bytes;
    TTFontTables tables;
};

static int cmap_format4(TTFont *font);
static int cmap_format6(TTFont *font);
static int cmap_format10(TTFont *font);
static int cmap_format12(TTFont *font);

static inline uint8_t getU8(const TTFont* font, uint32_t offset)
{
    return *(font->ttf_bytes + offset);
}

static inline uint8_t getI8(const TTFont* font, uint32_t offset)
{
    return (int8_t) getU8(font, offset);
}

static inline uint16_t getU16(const TTFont* font, uint32_t offset)
{
    const uint8_t *rebase = font->ttf_bytes + offset;
    uint16_t b1 = rebase[0], b0 = rebase[1];
    return (uint16_t) (b1 << 8 | b0);
}

static inline int16_t getI16(const TTFont* font, uint32_t offset)
{
    return (int16_t) getU16(font, offset);
}

static inline uint32_t getU32(const TTFont* font, uint32_t offset)
{
    const uint8_t *rebase = font->ttf_bytes + offset;
    uint32_t b3 = rebase[0], b2 = rebase[1], b1 = rebase[2], b0 = rebase[3];
    return (uint32_t) (b3 << 24 | b2 << 16 | b1 << 8 | b0);
}

int cmap_init(TTFont *font) {
    uint32_t entry, table;
    uint16_t idx, numTables, platformID, encodingID, format;
    // skip uint16 version
    numTables = getU16(font, font->tables.cmap.offset + 2);
    printf("read cmap subtables %d\n", numTables);
    for (idx = 0; idx < numTables; ++idx) {
        entry = font->tables.cmap.offset + 4 + idx * 8;
        platformID = getU16(font, entry);
        encodingID = getU16(font, entry + 2);
        printf("cmap subtable %d platform %d encoding %d\n", idx, platformID, encodingID);
    }

    return 0;
}

int font_init(TTFont *font) {
    uint32_t magic_number = getU32(font, 0);

    if (magic_number != 0x00010000 && magic_number != 0x74727565) {
        return -1;
    }
    // parse tables
    uint32_t tag;
    uint16_t numTables = getU32(font, 4);
    uint16_t count = 0;
    uint32_t offset;

    #define CASE_FONT_TABLE_TAG(tag, hex) \
        case hex: \
            font->tables.tag.offset = getU32(font, offset + 8); \
            font->tables.tag.length = getU32(font, offset + 12); \
            break;

    while (count < numTables) {
        count++;
        offset = 16 * count + 12;
        tag = getU32(font, offset);
        switch (tag) {
            CASE_FONT_TABLE_TAG(cmap, 0x636d6170)
            CASE_FONT_TABLE_TAG(glyf, 0x676c7966)
            CASE_FONT_TABLE_TAG(head, 0x68656164)
            CASE_FONT_TABLE_TAG(hmtx, 0x686d7478)
            CASE_FONT_TABLE_TAG(loca, 0x6c6f6361)
            default:
                // ignore
                break;
        }
        // @todo check tables
    }

    if (cmap_init(font) < 0) {
        return -2;
    }

    return 0;
}

TTFont *font_load(const void *mem, size_t size) {
    TTFont *font;
    if (!(font = calloc(1, sizeof *font))) {
        return NULL;
    }
    font->ttf_bytes = mem;
    font->ttf_size = size;
    if (font_init(font) < 0) {
        font_free(font);
        return NULL;
    }
    return font;
}

void font_free(TTFont *ttFont){
    if (!ttFont) return;
    free(ttFont);
}

void font_set_size(TTFont *ttFont, uint16_t size) {
    ttFont->font_size = size;
}


