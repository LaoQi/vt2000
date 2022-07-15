#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "font.h"

int read_file(const char * filepath, void ** bytes, size_t *size) {
    printf("read %s\n", filepath);
    FILE *f = fopen(filepath, "rb");
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);

    *bytes = (uint8_t *)malloc(sizeof (uint8_t) * length);
    *size = fread(*bytes, sizeof(uint8_t), length, f);
    if (*size != length) {
        printf("read failed");
        return -1;
    }
    fclose(f);
    return length;
}

int main() {
    void * memory;
    size_t size;
    TTFont *font;
    read_file("fonts/WenQuanYiMicroHeiMono-02.ttf", &memory, &size);
//    read_file("fonts/Ubuntu-Regular.ttf", &memory, &size);
//    read_file("fonts/DinkieBitmap-9pxDemo.ttf", &memory, &size);
    if (!(font = font_load(memory, size))) {
        printf("font load failed!\n");
    }

    font_render(font, 0x6b63);

    font_free(font);
    free(memory);

    return 0;
}
