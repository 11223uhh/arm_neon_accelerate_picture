#ifndef __BMP__YUV_420_H
#define __BMP__YUV_420_H
#include"stdint.h"
#include "stdbool.h"

struct bmp_data
{
    int width;
    int height;
    int BitCount;
    unsigned char *data_ptr;
};

typedef unsigned char byte;
typedef struct bmp_data bmp_data;


_Bool InsUT_RGB_yuv420 (int argc,char *argv[]) ;

#endif