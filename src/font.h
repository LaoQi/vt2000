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

    typedef struct TTFont TTFont;

    TTFont *font_load(const void *mem, size_t size);
    void font_free(TTFont *ttFont);
    void font_set_size(TTFont *ttFont, uint16_t size);
    int font_render(TTFont *ttFont, uint_least32_t codepoint);

#ifdef __cplusplus
}
#endif
#endif //VT2000_FONT_H
