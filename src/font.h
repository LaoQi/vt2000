/**
 *
 */

#ifndef VT2000_FONT_H
#define VT2000_FONT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TTFontDebug

#ifdef TTFontDebug
#define DebugPrintf(fmt, ...) printf(fmt, ##__VA_ARGS__);
#else
#define DebugPrintf(expr, fmt, ...)
#endif

    typedef struct TTFont TTFont;
    typedef struct TTFontBitmap TTFontBitmap;

    TTFont *font_load(const void *mem, size_t size);
    void font_free(TTFont *ttFont);
    void font_set_size(TTFont *ttFont, uint16_t size);
    int font_render(TTFont *ttFont, uint32_t codepoint);
    void font_free_bitmap(TTFontBitmap *bitmap);

#ifdef __cplusplus
}
#endif
#endif //VT2000_FONT_H
