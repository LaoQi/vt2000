#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "font.h"

#define UCS_PLANE_RANGE 65536

typedef struct {
    uint16_t plane1[UCS_PLANE_RANGE];
} TTFontTableGLYFIndex;

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
    TTFontTableGLYFIndex glyf_index;
};

static uint32_t cmap_find(TTFont *font, uint32_t code);
static int cmap_format0(TTFont *font, uint32_t offset);
static int cmap_format4(TTFont *font, uint32_t offset);
static int cmap_format6(TTFont *font, uint32_t offset);
static int cmap_format12(TTFont *font, uint32_t offset);

static inline uint8_t get_uint8(const TTFont* font, uint32_t offset)
{
    return *(font->ttf_bytes + offset);
}

static inline uint8_t get_int8(const TTFont* font, uint32_t offset)
{
    return (int8_t) get_uint8(font, offset);
}

static inline uint16_t get_uint16(const TTFont* font, uint32_t offset)
{
    const uint8_t *rebase = font->ttf_bytes + offset;
    uint16_t b1 = rebase[0], b0 = rebase[1];
    return (uint16_t) (b1 << 8 | b0);
}

static inline int16_t get_int16(const TTFont* font, uint32_t offset)
{
    return (int16_t) get_uint16(font, offset);
}

static inline uint32_t get_uint32(const TTFont* font, uint32_t offset)
{
    const uint8_t *rebase = font->ttf_bytes + offset;
    uint32_t b3 = rebase[0], b2 = rebase[1], b1 = rebase[2], b0 = rebase[3];
    return (uint32_t) (b3 << 24 | b2 << 16 | b1 << 8 | b0);
}

/**
 * cmap table https://docs.microsoft.com/en-us/typography/opentype/spec/cmap
 * support platformID encodingID formatMath Description
 * | Unicode platform   | 0 | 3  | formats 4 or 6 | Unicode BMP only
 * | Unicode platform   | 0 | 4  | formats 10 or 12 | Unicode full repertoire
 * | Macintosh platform | 1 | -  | - | -
 * | Windows platform   | 3 | 1  | formats 4 | Unicode BMP
 * | Windows platform   | 3 | 10 | formats 12 | Unicode full repertoire
 *
 * search order
 * Unicode full repertoire > Unicode BMP > Platform Macintosh
 */
int cmap_init(TTFont *font) {
    uint32_t entry, category, cmapOffset = 0;
    uint16_t i, num, cmapFormat;
    uint32_t offset[3] = {0, 0, 0};
    uint16_t  format[3] = {0, 0, 0};
    // skip uint16 version
    num = get_uint16(font, font->tables.cmap.offset + 2);
    // read all subTable
    for (i = 0; i < num; ++i) {
        entry = font->tables.cmap.offset + 4 + i * 8;
        category = get_uint32(font, entry);
//        platformID = get_uint16(font, entry);
//        encodingID = get_uint16(font, entry + 2);

        switch (category) {
            case 0x0003: // Unicode platform Unicode BMP only
            case 0x0301: // Windows platform Unicode BMP
                offset[1] = get_uint32(font, entry + 4);
                format[1] = get_uint16(font, font->tables.cmap.offset + offset[1]);
                break;
            case 0x0004: // Unicode platform  Unicode full repertoire
            case 0x0310: // Windows platform  Unicode full repertoire
                offset[0] = get_uint32(font, entry + 4);
                format[0] = get_uint16(font, font->tables.cmap.offset + offset[0]);
                break;
            case 0x0100: // Macintosh platform
                offset[2] = get_uint32(font, entry + 4);
                format[2] = get_uint16(font, font->tables.cmap.offset + offset[2]);
                break;
            default:
                break;
        }
    }

    for (i = 0; i < 3; ++i) {
        if (offset[i] > 0) {
            cmapOffset = offset[i] + font->tables.cmap.offset;
            cmapFormat = format[i];
            DebugPrintf("init cmap format %d\n", format[i])
            break;
        }
    }

    if (cmapOffset == 0) {
        // failed
        return -1;
    }

    memset(font->glyf_index.plane1, 0, sizeof(uint16_t) * UCS_PLANE_RANGE);

    switch (cmapFormat) {
        case 0:
            return cmap_format0(font, cmapOffset);
        case 4:
            return cmap_format4(font, cmapOffset);
        case 6:
            return cmap_format6(font, cmapOffset);
        case 12:
            return cmap_format12(font, cmapOffset);
        default:
            return -1;
    }

}

int font_init(TTFont *font) {
    uint32_t magic_number = get_uint32(font, 0);

    if (magic_number != 0x00010000 && magic_number != 0x74727565) {
        return -1;
    }
    // parse tables
    uint32_t tag;
    uint16_t numTables = get_uint32(font, 4);
    uint16_t count = 0;
    uint32_t offset;

    #define CASE_FONT_TABLE_TAG(tag, hex) \
        case hex: \
            font->tables.tag.offset = get_uint32(font, offset + 8); \
            font->tables.tag.length = get_uint32(font, offset + 12); \
            break;

    while (count < numTables) {
        count++;
        offset = 16 * count + 12;
        tag = get_uint32(font, offset);
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

void font_set_size(TTFont *font, uint16_t size) {
    font->font_size = size;
}

void font_free_bitmap(TTFontBitmap *bitmap) {
    if (!bitmap) return;
    free(bitmap);
}

int font_render(TTFont *font, uint32_t codepoint) {
    uint32_t glyf = cmap_find(font, codepoint);
    DebugPrintf("find glyf index %d\n", glyf)
    return 0;
}

static uint32_t cmap_find(TTFont *font, uint32_t code) {
    if (code > 0xFFFF) return 0;
    uint16_t shortCode = (uint16_t) code;
    return font->glyf_index.plane1[shortCode];
}

static int cmap_format0(TTFont *font, uint32_t offset) {
    int i;
    for(i = 0; i < 256; ++i) {
        // skip uint16 format uint16 length uint16 language
        font->glyf_index.plane1[i] = get_uint8(font, offset + 6 + i);
    }
    return 0;
}

static int cmap_format4(TTFont *font, uint32_t offset)
{
    int i, j, total = 0;
    uint16_t segCount, segCountX2, endCode, startCode, idRangeOffset;
    int16_t idDelta;
    uint32_t tmpOffset, segArraySize;

    segCountX2 = get_uint16(font, offset + 6);
    segCount = segCountX2 / 2;

    segArraySize = sizeof(uint16_t) * segCount;
    for (i = 0; i < segCount; ++i) {
        tmpOffset = offset + 14 + (i * 2);
        endCode = get_uint16(font, tmpOffset);
        tmpOffset += segArraySize + 2;
        startCode = get_uint16(font, tmpOffset);
        tmpOffset += segArraySize;
        idDelta = get_int16(font, tmpOffset);
        tmpOffset += segArraySize;
        idRangeOffset = get_uint16(font, tmpOffset);

        for (j = startCode; j <= endCode; ++j) {
            if (idRangeOffset > 0) {
                tmpOffset += idRangeOffset + ((j - startCode) * 2);
                font->glyf_index.plane1[j] = get_uint16(font, tmpOffset);
            } else {
                font->glyf_index.plane1[j] = j + idDelta;
            }
            total++;
        }
    }

    return 0;
}

static int cmap_format6(TTFont *font, uint32_t offset)
{
    uint16_t i, firstCode, entryCount;
    firstCode = get_uint16(font, offset + 6);
    entryCount = get_uint16(font, offset + 8);
    for (i = 0; i < entryCount; ++i) {
        font->glyf_index.plane1[firstCode + i] = get_uint16(font, offset + 10 + (2 * i));
    }
    return 0;
}

static int cmap_format12(TTFont *font, uint32_t offset)
{
    uint32_t i, j, num, startCharCode, endCharCode, startGlyphID, tmpOffset;
    num = get_uint32(font, offset + 12);
    for (i = 0; i < num; ++i) {
        tmpOffset = offset + 16 + (i * 12);
        startCharCode = get_uint32(font, tmpOffset);
        endCharCode = get_uint32(font, tmpOffset + 4);
        startGlyphID = get_uint32(font, tmpOffset + 8);
        if (startCharCode > 0xFFFF) {
            // @todo add other plane
            break;
        }
        for (j = startCharCode; j <= endCharCode; ++j) {
            if (j < 0xFFFF) {
                font->glyf_index.plane1[j] = j - startCharCode + startGlyphID;
            }
        }
    }
    return 0;
}
