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

    const int8x8_t U_V_constant = vdup_n_s8(127);

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

        int8x8_t vsum_int8=vrshrn_n_s16(vsum,8) ; //int 16-int8
        
        vsum=vaddl_s8(vsum_int8,U_V_constant);
    
        uint8x8_t result=vqmovun_s16(vsum);
        
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

_Bool InsUT_RGB_yuv420(int32_t argc, char **argv) 
{
    char *bath_path=argv[1];
    bmp_data data_bmp;
    
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

    return 0;//读取文件成功
}



