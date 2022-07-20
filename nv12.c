#include "nv12.h"
#include "NEON_2_SSE.h"

#include "stdio.h"



static void NV_BGR_yuv_uv(int *coef_q8_1,int *coef_q8_2,const unsigned char *bgr, int width, int height, unsigned char *yuv_v) 
{
    unsigned char *temp_s = (unsigned char*)bgr;
    unsigned char *temp_d = yuv_v;
    int16x8_t vwb_U = vdupq_n_s16(coef_q8_1[0]);
    int16x8_t vwg_U = vdupq_n_s16(coef_q8_1[1]);
    int16x8_t vwr_U = vdupq_n_s16(coef_q8_1[2]);

    int16x8_t vwb_V = vdupq_n_s16(coef_q8_2[0]);
    int16x8_t vwg_V = vdupq_n_s16(coef_q8_2[1]);
    int16x8_t vwr_V = vdupq_n_s16(coef_q8_2[2]);
    const int8x8_t U_V_constant = vdup_n_s8(127);

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
       

            int16x8_t vsum_U ;
            vsum_U=vmulq_s16(B_data, vwb_U);
            vsum_U = vmlaq_s16(vsum_U,g_data, vwg_U);
            vsum_U = vmlaq_s16(vsum_U,r_data, vwr_U);
            vsum_U=vaddl_s8(vrshrn_n_s16(vsum_U,8),U_V_constant);


            int16x8_t vsum_V ;
            vsum_V=vmulq_s16(B_data, vwb_V);
            vsum_V = vmlaq_s16(vsum_V,g_data, vwg_V);
            vsum_V = vmlaq_s16(vsum_V,r_data, vwr_V);
            vsum_V=vaddl_s8(vrshrn_n_s16(vsum_V,8),U_V_constant);

            uint8x8x2_t result;
            result.val[1]=vqmovun_s16(vsum_V);
            result.val[0]=vqmovun_s16(vsum_U);

            vst2_u8(temp_d, result);
            temp_d+=16;
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
static void NV_BGR_yuv(const unsigned char *bgr, int width, int height, unsigned char *yuv_v) 
{
    int coef_q8[3][3]={{ 25, 129, 66 },{ 112, -74, -38 },{ -18, -94, 112 }};
    BGR_yuv_y(coef_q8[0],bgr,width, height, yuv_v);
    NV_BGR_yuv_uv(coef_q8[1],coef_q8[2],bgr,  width,  height, yuv_v+width*height) ;

}


_Bool InsUT_RGB_NV12(int32_t argc, char **argv) 
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



        NV_BGR_yuv(data_bmp.data_ptr,data_bmp.width,data_bmp.height,yuv_ptr);


        file = fopen("xxx.yuv", "wb");
        fwrite(yuv_ptr, sizeof(unsigned char), sizeof(unsigned char) * (data_bmp.width*data_bmp.height*1.5), file);

	    fclose(file);//关闭yuv文件
        printf("write ok");
        free(bmp_temp);
        free(yuv_ptr);
    }

    return 1;//读取文件成功
}


static void NV12_BGR( unsigned char *bgr, int width, int height, unsigned char *yuv_v)
{
    uint16x8_t V_R=vdupq_n_u16(204);
    uint16x8_t Y_R=vdupq_n_u16(149);

    uint16x8_t U_G=vdupq_n_u16(50);
    uint16x8_t V_G=vdupq_n_u16(104);

    uint16x8_t U_B=vdupq_n_u16(129);

    uint16x8_t R_const=vdupq_n_u16(223);
    uint16x8_t G_const=vdupq_n_u16(135*128);
    uint16x8_t B_const=vdupq_n_u16(277);


    unsigned char *temp_s = (unsigned char*)yuv_v;
    unsigned char *temp_d = bgr;
    unsigned char *ptr_u_v = temp_s+width*height;
    int pitch = width/16;
    uint16x8_t G_vsum ;

    uint16x8_t R_vsum ;
    uint16x8_t B_vsum ;

    uint8x8_t R_result ;
    uint8x8_t G_result ;
    uint8x8_t B_result ;

    uint8x8x2_t R_result__ ;
    uint8x8x2_t G_result__ ;
    uint8x8x2_t B_result__ ;

    uint8x16x3_t result;

    // Y1 Y2 Y3 Y4 Y5 Y6 Y7 Y8 Y9 Y10
    // Y1 Y3 Y5 Y2 Y4 Y6
    // U1 U2 U3 U1 U2 U3
    // V1 V2 V3 V1 V2 V3
    // R1 R3 R5 R2 R4 R6
    // u1 v1 u2 v2 u3 v3 u4 v4  u1 u2 u3 u4 u1 u2 u3 u4
    // u1 v1 u2 v2 u3 v3 u4 v4

    for (int j = 0; j < height; ++j)
    {
        for (int i = 0; i < pitch; ++i)
        {
            uint8x8x2_t y_data = vld2_u8(temp_s);
            uint8x8x2_t u_v_data = vld2_u8(ptr_u_v);

            uint16x8_t u_data_copy =vmovl_u8(u_v_data.val[0]);
            uint16x8_t v_data_copy =vmovl_u8(u_v_data.val[1]);

            uint16x8_t temp_R =vmulq_u16(vmovl_u8(y_data.val[0]), Y_R);
            uint16x8_t temp_R_NO_shift =temp_R;
            temp_R=vrshrq_n_u16(temp_R,7);

            R_vsum= vmulq_u16 (v_data_copy, V_R);
            R_vsum=vrshrq_n_u16(R_vsum,7);
            R_vsum=vqaddq_u16(R_vsum,temp_R);
            R_vsum=vqsubq_u16(R_vsum,R_const);
    
            B_vsum=vqaddq_u16(u_data_copy, u_data_copy);
            B_vsum=vqaddq_u16(B_vsum,temp_R);
            B_vsum=vqsubq_u16(B_vsum,B_const);
//    G=(temp-50*U-104*V)>>7+135  

            G_vsum=vqaddq_u16(temp_R_NO_shift,G_const);
            G_vsum=vmlsq_u16(G_vsum,u_data_copy,U_G);
            G_vsum=vmlsq_u16(G_vsum,v_data_copy,V_G);
            G_vsum=vrshrq_n_u16(G_vsum,7);

            R_result=vqmovn_u16(R_vsum);
            G_result=vqmovn_u16(G_vsum);
            B_result=vqmovn_u16(B_vsum);

            temp_R =vmulq_u16(vmovl_u8(y_data.val[1]), Y_R);
            temp_R_NO_shift =temp_R;
            temp_R=vrshrq_n_u16(temp_R,7);

            R_vsum= vmulq_u16 (v_data_copy, V_R);
            R_vsum=vrshrq_n_u16(R_vsum,7);
            R_vsum=vqaddq_u16(R_vsum,temp_R);
            R_vsum=vqsubq_u16(R_vsum,R_const);
    
            B_vsum=vqaddq_u16(u_data_copy, u_data_copy);
            B_vsum=vqaddq_u16(B_vsum,temp_R);
            B_vsum=vqsubq_u16(B_vsum,B_const);
//    G=(temp-50*U-104*V)>>7+135  

            G_vsum=vqaddq_u16(temp_R_NO_shift,G_const);
            G_vsum=vmlsq_u16(G_vsum,u_data_copy,U_G);
            G_vsum=vmlsq_u16(G_vsum,v_data_copy,V_G);
            G_vsum=vrshrq_n_u16(G_vsum,7);

            //R_result=vqmovn_u16(R_vsum);
            //G_result=vqmovn_u16(G_vsum);
            //B_result=vqmovn_u16(B_vsum);
            R_result__=vzip_u8(R_result,vqmovn_u16(R_vsum));
            G_result__=vzip_u8(G_result,vqmovn_u16(G_vsum));
            B_result__=vzip_u8(B_result,vqmovn_u16(B_vsum));
            result.val[0]=vcombine_u8(B_result__.val[0],B_result__.val[1]);
            result.val[1]=vcombine_u8(G_result__.val[0],G_result__.val[1]);
            result.val[2]=vcombine_u8(R_result__.val[0],R_result__.val[1]);

            vst3q_u8(temp_d,result);
            temp_d+=48;
            temp_s+=16;
            ptr_u_v+=16;

        }
        ptr_u_v=ptr_u_v-(1>>j%2)*(width);
    
    }
} 

_Bool InsUT_NV12_RGB(int32_t argc, char **argv) 
{
    FILE *file;
    char *yuv_path=argv[1];
    FILE *fp=fopen(yuv_path,"rb");//二进制读方式打开指定的图像文件
    printf("\n path= %s\n",yuv_path);
    unsigned char *bgr_temp;
    unsigned char *yuv_ptr;
    int width;
    int height;
    if(fp==0){printf("not find file\n");return 0;}
    width = atoi(argv[2]);

    height = atoi(argv[3]);

    bgr_temp=(unsigned char *)malloc(width * height*3);
    yuv_ptr=(unsigned char *)malloc(width*height+width*(height/2));

    fread(yuv_ptr,1,width*height*1.5,fp);//将yuv写入yuv缓存

    NV12_BGR( bgr_temp,  width,  height,yuv_ptr);

    file = fopen("xxx.bmp", "wb");

    fwrite(bgr_temp, sizeof(unsigned char), sizeof(unsigned char) * (width*height*3), file);

	fclose(file);//关闭yuv文件
}
