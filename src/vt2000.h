#ifndef VT2000_VT2000_H
#define VT2000_VT2000_H

#ifndef VT_malloc
#define VT_malloc(x)  (malloc(x))
#define VT_free(x)    (free(x))
#endif

int VT_Init(int width, int height);
void VT_Update();

#endif //VT2000_VT2000_H
