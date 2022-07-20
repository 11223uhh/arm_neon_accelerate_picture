#include "bmp_yuv_420.h"
#include "NEON_2_SSE.h"

#include "stdio.h"



static void BGR_yuv_uv(int *coef_q8,const unsigned char *bgr, int width, int height, unsigned char *yuv_v) 
{
    unsigned char *temp_s = (unsigned char*)bgr;

    unsigned char *temp_d = yuv_v;

    int16x8_t vwb = vdupq_n_s16(coef_q8[0]);
    int16x8_t vwg = vdupq_n_s16(coef_q8[1]);
    int16x8_t vwr = vdupq_n_s16(coef_q8[2]);

    const int8x8_t s8_rounding = vdup_n_s8(127);

    const int16x8_t s16_zero = vdupq_n_s16(0);
    const int16x8_t s16_rounding = vdupq_n_s16(128);

    const uint8x8_t u8_zero = vdup_n_u8(0);
    const uint8x8_t u8_16 = vdup_n_u8(16);
    const uint16x8_t u16_rounding = vdupq_n_u16(128);

    int pitch = width >> 4;

    for (int row = 0; row < height; row++) 
    {
         for (int i = 0; i < pitch; ++i)
        {
            if (row % 2 == 0)
            {
        uint8x16x3_t vsrc = vld3q_u8(temp_s);
        uint8x8_t high_r = vget_high_u8(vsrc.val[2]);
        uint8x8_t low_r = vget_low_u8(vsrc.val[2]);

        uint8x8_t high_g = vget_high_u8(vsrc.val[1]);
        uint8x8_t low_g = vget_low_u8(vsrc.val[1]);

        uint8x8_t high_b = vget_high_u8(vsrc.val[0]);
        uint8x8_t low_b = vget_low_u8(vsrc.val[0]);

        uint8x8x2_t mix_r = vuzp_u8(low_r, high_r);
        uint8x8x2_t mix_g = vuzp_u8(low_g, high_g);
        uint8x8x2_t mix_b = vuzp_u8(low_b, high_b);


        int16x8_t r_data = vreinterpretq_s16_u16(vmovl_u8(mix_r.val[0]));
        int16x8_t g_data = vreinterpretq_s16_u16(vmovl_u8(mix_g.val[0]));
        int16x8_t B_data = vreinterpretq_s16_u16(vmovl_u8(mix_b.val[0]));
        
            /*由于有负数 uint8数据转 int16数据 */
       

        int16x8_t vsum ;
        uint16x8_t temp ;
        vsum=vmulq_s16(B_data, vwb);
        vsum = vmlaq_s16(vsum,g_data, vwg);
        vsum = vmlaq_s16(vsum,r_data, vwr);

        vsum = vaddq_s16(vsum, s16_rounding);

        int8x8_t vsum_int8=vqshrn_n_s16(vsum,8) ; //int 16-int8

         //vsum=vrshrq_n_s16(vsum,8);
        //result=vqmovn_u16(vsum);

        vsum=vaddl_s8(vsum_int8,s8_rounding);//int 8-int16 加上127

        //uint16x8_t u_vsum_ = vreinterpretq_u16_s16(vsum);// int16转uint16
        //uint8x8_t result=vqmovn_u16(u_vsum_);// uint16转uint8
        temp=vreinterpretq_u16_s16(vsum);
        uint8x8_t result=vqmovn_u16(temp);

        vst1_u8(temp_d, result);
        temp_d+=8;
            }
        temp_s+=48;
        }
    }
    return;
}


static void BGR_yuv_y(int *coef_q8,const unsigned char *bgr, int width, int height, unsigned char *gray) {
    unsigned char *temp_s = (unsigned char*)bgr;
    unsigned char *temp_d = gray;
    uint8x8_t vwb = vdup_n_u8(coef_q8[0]);
    uint8x8_t vwg = vdup_n_u8(coef_q8[1]);
    uint8x8_t vwr = vdup_n_u8(coef_q8[2]);
    uint8x8_t temp_da = vdup_n_u8(16);

    uint8x8_t result;

    uint8x8_t result_out;

    for (int i = 0; i < (height*width)/8; i++) {

        uint8x8x3_t vsrc = vld3_u8(temp_s);
        uint16x8_t vsum = vmull_u8(vsrc.val[2], vwr);
        vsum = vmlal_u8(vsum, vsrc.val[1], vwg);
        vsum = vmlal_u8(vsum, vsrc.val[0], vwb);

        vsum=vrshrq_n_u16(vsum,8);
        result=vqmovn_u16(vsum);

        result_out=vqadd_u8(result,temp_da);
        vst1_u8(temp_d, result_out);
        temp_d += 8;
        temp_s += 24;
    }
    return;
}
static void BGR_yuv(const unsigned char *bgr, int width, int height, unsigned char *yuv_v) 
{
    int coef_q8[3][3]={{ 25, 129, 66 },{ 112, -74, -38 },{ -18, -94, 112 }};
    BGR_yuv_y(coef_q8[0],bgr,width, height, yuv_v);
    BGR_yuv_uv(coef_q8[1],bgr,  width,  height, yuv_v+width*height) ;
    BGR_yuv_uv(coef_q8[2],bgr,  width,  height, yuv_v+width*height+width*(height/4)) ;


}
//-----------------------------------------------------------------------------------------
//给定一个图像位图数据、宽、高、颜色表指针及每像素所占的位数等信息,将其写到指定文件中

void RGBtoNV12(unsigned char* pRGB, int width, int height,unsigned char* pNV12)
{
    int frameSize = width * height;
    int yIndex = 0;
    int uvIndex = frameSize;

    int R, G, B, Y, U, V;
    int index = 0;
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            R = pRGB[index++];
            G = pRGB[index++];
            B = pRGB[index++];

            Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;
            U = ((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
            V = ((112 * R - 94 * G - 18 * B + 128) >> 8) + 128;
            
            // NV12  YYYYYYYY UVUV 
            // NV21  YYYYYYYY VUVU
            pNV12[yIndex++] = (byte)((Y < 0) ? 0 : ((Y > 255) ? 255 : Y));
            if (j % 2 == 0 && index % 2 == 0)
            {
                pNV12[uvIndex++] = (byte)((U < 0) ? 0 : ((U > 255) ? 255 : U));
                pNV12[uvIndex++] = (byte)((V < 0) ? 0 : ((V > 255) ? 255 : V));
            }
        }
    }
}
void RGB_to_NV12_intrinsic(unsigned char* pRGB, int width, int height,unsigned char* pNV12)
{
    const uint8x8_t u8_zero = vdup_n_u8(0);
    const uint8x8_t u8_16 = vdup_n_u8(16);
    const uint16x8_t u16_rounding = vdupq_n_u16(128);

    const int16x8_t s16_zero = vdupq_n_s16(0);
    const int8x8_t s8_rounding = vdup_n_s8(-128);
    const int16x8_t s16_rounding = vdupq_n_s16(128);

    byte* UVPtr = pNV12 + width * height;
    int pitch = width >> 4;

    for (int j = 0; j < height; ++j)
    {
        for (int i = 0; i < pitch; ++i)
        {
            // Load RGB 16 pixel
            uint8x16x3_t pixel_rgb = vld3q_u8(pRGB);

            uint8x8_t high_r = vget_high_u8(pixel_rgb.val[0]);
            uint8x8_t low_r = vget_low_u8(pixel_rgb.val[0]);
            uint8x8_t high_g = vget_high_u8(pixel_rgb.val[1]);
            uint8x8_t low_g = vget_low_u8(pixel_rgb.val[1]);
            uint8x8_t high_b = vget_high_u8(pixel_rgb.val[2]);
            uint8x8_t low_b = vget_low_u8(pixel_rgb.val[2]);

            // NOTE:
            // declaration may not appear after executable statement in block
            uint16x8_t high_y;
            uint16x8_t low_y;           

            uint8x8_t scalar = vdup_n_u8(66);  // scalar = 66
            high_y = vmull_u8(high_r, scalar);  // Y = R * 66
            low_y = vmull_u8(low_r, scalar);

            scalar = vdup_n_u8(129);
            high_y = vmlal_u8(high_y, high_g, scalar);  // Y = Y + R*129
            low_y = vmlal_u8(low_y, low_g, scalar);

            scalar = vdup_n_u8(25);
            high_y = vmlal_u8(high_y, high_b, scalar);  // Y = Y + B*25
            low_y = vmlal_u8(low_y, low_b, scalar);

            high_y = vaddq_u16(high_y, u16_rounding);  // Y = Y + 128
            low_y = vaddq_u16(low_y, u16_rounding);

            uint8x8_t u8_low_y = vshrn_n_u16(low_y, 8);  // Y = Y >> 8
            uint8x8_t u8_high_y = vshrn_n_u16(high_y, 8);

            low_y = vaddl_u8(u8_low_y, u8_16);  // Y = Y + 16
            high_y = vaddl_u8(u8_high_y, u8_16);

            uint8x16_t pixel_y = vcombine_u8(vqmovn_u16(low_y), vqmovn_u16(high_y));

            // Store
            vst1q_u8(pNV12, pixel_y);  

            if (j % 2 == 0)
            {
                uint8x8x2_t mix_r = vuzp_u8(low_r, high_r);
                uint8x8x2_t mix_g = vuzp_u8(low_g, high_g);
                uint8x8x2_t mix_b = vuzp_u8(low_b, high_b);

                int16x8_t signed_r = vreinterpretq_s16_u16(vaddl_u8(mix_r.val[0], u8_zero));
                int16x8_t signed_g = vreinterpretq_s16_u16(vaddl_u8(mix_g.val[0], u8_zero));
                int16x8_t signed_b = vreinterpretq_s16_u16(vaddl_u8(mix_b.val[0], u8_zero));

                int16x8_t signed_u;
                int16x8_t signed_v;

                int16x8_t signed_scalar = vdupq_n_s16(-38);
                signed_u = vmulq_s16(signed_r, signed_scalar);

                signed_scalar = vdupq_n_s16(112);
                signed_v = vmulq_s16(signed_r, signed_scalar);

                signed_scalar = vdupq_n_s16(-74);
                signed_u = vmlaq_s16(signed_u, signed_g, signed_scalar);

                signed_scalar = vdupq_n_s16(-94);
                signed_v = vmlaq_s16(signed_v, signed_g, signed_scalar);

                signed_scalar = vdupq_n_s16(112);
                signed_u = vmlaq_s16(signed_u, signed_b, signed_scalar);

                signed_scalar = vdupq_n_s16(-18);
                signed_v = vmlaq_s16(signed_v, signed_b, signed_scalar);

                signed_u = vaddq_s16(signed_u, s16_rounding);
                signed_v = vaddq_s16(signed_v, s16_rounding);

                int8x8_t s8_u = vshrn_n_s16(signed_u, 8);
                int8x8_t s8_v = vshrn_n_s16(signed_v, 8);

                signed_u = vsubl_s8(s8_u, s8_rounding);
                signed_v = vsubl_s8(s8_v, s8_rounding);

                signed_u = vmaxq_s16(signed_u, s16_zero);
                signed_v = vmaxq_s16(signed_v, s16_zero);

                uint16x8_t unsigned_u = vreinterpretq_u16_s16(signed_u);
                uint16x8_t unsigned_v = vreinterpretq_u16_s16(signed_v);

                uint8x8x2_t result;
                result.val[0] = vqmovn_u16(unsigned_u);
                result.val[1] = vqmovn_u16(unsigned_v);

                vst2_u8(UVPtr, result);
                UVPtr += 16;
            }

            pRGB += 3*16;
            pNV12 += 16;
        }
    }
}

_Bool InsUT_RGB_yuv420(int32_t argc, char **argv) 
{
    char *bath_path=argv[1];
    bmp_data data_bmp;
    
    BITMAPINFOHEADER_test head;  
    FILE *file;
    int lineByte;
    unsigned char *bmp_temp=NULL;
    unsigned char *yuv_ptr=NULL;
    FILE *fp=fopen(bath_path,"rb");//二进制读方式打开指定的图像文件
    printf("\n path= %s\n",bath_path);

    if(fp==0){printf("not find file\n");return 0;}


   

    data_bmp.width = atoi(argv[2]);

    data_bmp.height = atoi(argv[3]);
    data_bmp.BitCount =24;
    printf("picture= width=%d, height=%d, bitcount=%d,\n",data_bmp.width,data_bmp.height,data_bmp.BitCount);
    {
        lineByte=(data_bmp.width * data_bmp.BitCount/8+3)/4*4;

        
        bmp_temp=(unsigned char *)malloc(lineByte * data_bmp.height+4000);
        yuv_ptr=(unsigned char *)malloc(data_bmp.width*data_bmp.height+data_bmp.width*(data_bmp.height/2)+4000);

        data_bmp.data_ptr=bmp_temp;//创建bmp缓存

        printf("create temp success\n ");

        fread(data_bmp.data_ptr,1,lineByte * data_bmp.height,fp);//将文件写入bmp数组
        printf( "write bmp to array \n");

    	fclose(fp);//关闭bmp文件
        printf( "close bmp  \n");



        BGR_yuv(data_bmp.data_ptr,data_bmp.width,data_bmp.height,yuv_ptr);
        //RGBtoNV12(data_bmp.data_ptr,data_bmp.width,data_bmp.height,yuv_ptr);

        for(int i=0;i<100;i++)
        printf("log data= %d, ",data_bmp.data_ptr[i]);

        file = fopen("xxx.yuv", "wb");
        fwrite(yuv_ptr, sizeof(unsigned char), sizeof(unsigned char) * (data_bmp.width*data_bmp.height*1.5), file);

	    fclose(file);//关闭yuv文件
        printf("write ok");
        free(bmp_temp);
        free(yuv_ptr);
    }

    return 1;//读取文件成功
}



