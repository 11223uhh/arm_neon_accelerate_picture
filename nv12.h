#ifndef __NV12__H
#define __NV12__H
#include"stdint.h"
#include "stdbool.h"
_Bool InsUT_RGB_NV12(int32_t argc, char **argv);
_Bool InsUT_NV12_RGB(int32_t argc, char **argv);

struct bmp_data
{
    int width;
    int height;
    int BitCount;
    unsigned char *data_ptr;
};

typedef unsigned char byte;
typedef struct bmp_data bmp_data;
#endif