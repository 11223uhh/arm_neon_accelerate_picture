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

/*
    temp=149*Y>>7                   4次乘法 3次位移 8为加减
    R=temp         +1.596*V+223          
    G=(temp-50*U-104*V)>>7+135  
    B=temp+ U+U         +277
*/
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

    for (int j = 0; j < height; ++j)
    {
        for (int i = 0; i < pitch; ++i)
        {

            uint8x8x2_t y_data = vld2_u8(temp_s);
            uint8x8x2_t u_v_data = vld2_u8(ptr_u_v);

            uint16x8_t u_data_copy =vmovl_u8(u_v_data.val[0]);
            uint16x8_t v_data_copy =vmovl_u8(u_v_data.val[1]);
            /*第一组八点 */
            uint16x8_t temp_R =vmulq_u16(vmovl_u8(y_data.val[0]), Y_R);
            uint16x8_t temp_R_NO_shift =temp_R;
            temp_R=vrshrq_n_u16(temp_R,7);

            R_vsum= vmulq_u16 (v_data_copy, V_R);
            R_vsum=vrshrq_n_u16(R_vsum,7);
            R_vsum=vqaddq_u16(R_vsum,temp_R);
            R_vsum=vqsubq_u16(R_vsum,R_const);
    
            B_vsum= vmulq_u16 (u_data_copy, U_B);
            B_vsum=vrshrq_n_u16(B_vsum,6);
            B_vsum=vqaddq_u16(B_vsum,temp_R);
            B_vsum=vqsubq_u16(B_vsum,B_const);
//    G=(temp-50*U-104*V)>>7+135  

            G_vsum=vqaddq_u16(temp_R_NO_shift,G_const);
            G_vsum=vqsubq_u16(G_vsum,vmulq_u16 (u_data_copy, U_G));
            G_vsum=vqsubq_u16(G_vsum,vmulq_u16 (v_data_copy, V_G));
            G_vsum=vrshrq_n_u16(G_vsum,7);

            R_result=vqmovn_u16(R_vsum);
            G_result=vqmovn_u16(G_vsum);
            B_result=vqmovn_u16(B_vsum);
          /*第二组八点 */
            temp_R =vmulq_u16(vmovl_u8(y_data.val[1]), Y_R);
            temp_R_NO_shift =temp_R;
            temp_R=vrshrq_n_u16(temp_R,7);

            R_vsum= vmulq_u16 (v_data_copy, V_R);
            R_vsum=vrshrq_n_u16(R_vsum,7);
            R_vsum=vqaddq_u16(R_vsum,temp_R);
            R_vsum=vqsubq_u16(R_vsum,R_const);
    
            B_vsum= vmulq_u16 (u_data_copy, U_B);
            B_vsum=vrshrq_n_u16(B_vsum,6);
            B_vsum=vqaddq_u16(B_vsum,temp_R);
            B_vsum=vqsubq_u16(B_vsum,B_const);
//    G=(temp-50*U-104*V)>>7+135  

            G_vsum=vqaddq_u16(temp_R_NO_shift,G_const);
            G_vsum=vqsubq_u16(G_vsum,vmulq_u16 (u_data_copy, U_G));
            G_vsum=vqsubq_u16(G_vsum,vmulq_u16 (v_data_copy, V_G));
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

static void Yuv420sp2bgr(uint8_t* dst, uint8_t* srcY, uint8_t* srcU, int imgW, int imgH, int dstPitch, int srcPitch)
{
    const int Yuv2RgbBitShift = 7;
    const int Yuv2RgbYFac = 149; // 1.164 * 128
    const int Yuv2RgbYYY = 21;  // 0.164 * 128
    const int Yuv2RgbV2R = 204; // 1.596 * 128
    const int Yuv2RgbU2G = 50;  // 0.392 * 128
    const int Yuv2RgbV2G = 104; // 0.812 * 128
    const int Yuv2RgbU2B = 258; // 2.016 * 128
    for (int i = 0; i < imgH - 1; i += 2) {
        unsigned char* dstL0 = dst + i * dstPitch;
        unsigned char* dstL1 = dstL0 + dstPitch;
        unsigned char* srcY0 = srcY + i * srcPitch;
        unsigned char* srcY1 = srcY0 + srcPitch;
        unsigned char* srcUV = srcU + (i / 2) * srcPitch;
        int j = 0;
        uint8x8_t vSft = vdup_n_u8(1 << Yuv2RgbBitShift);
        uint8x8_t vYYY = vdup_n_u8(Yuv2RgbYYY);
        int16x8_t vV2R = vdupq_n_s16(Yuv2RgbV2R);
        int16x8_t vU2G = vdupq_n_s16(Yuv2RgbU2G);
        int16x8_t vV2G = vdupq_n_s16(Yuv2RgbV2G);
        int16x8_t vU2B = vdupq_n_s16(Yuv2RgbU2B);
        for (; j < imgW - 7; j += 8) {
            uint8x8_t vY0 = vqsub_u8(vld1_u8(&srcY0[j]), vdup_n_u8(16));
            uint8x8_t vY1 = vqsub_u8(vld1_u8(&srcY1[j]), vdup_n_u8(16));
            uint8x8_t vU0 = vld1_u8(&srcUV[j]);
            uint8x8x2_t vUV = vtrn_u8(vU0, vU0);
            int16x8_t vUU = vsubq_s16(vreinterpretq_s16_u16(vmovl_u8(vUV.val[0])), vdupq_n_s16(128));
            int16x8_t vVV = vsubq_s16(vreinterpretq_s16_u16(vmovl_u8(vUV.val[1])), vdupq_n_s16(128));
            int16x8_t vTmp0 = vreinterpretq_s16_u16(vmull_u8(vY0, vYYY));
            int16x8_t vTmp1 = vreinterpretq_s16_u16(vmull_u8(vY1, vYYY));
            int16x8_t vAdd0 = vreinterpretq_s16_u16(vmull_u8(vY0, vSft));
            int16x8_t vAdd1 = vreinterpretq_s16_u16(vmull_u8(vY1, vSft));

            int16x8_t vTmpR = vmulq_s16(vV2R, vVV);
            int16x8_t vTmpG = vaddq_s16(vmulq_s16(vU2G, vUU), vmulq_s16(vV2G, vVV));
            int16x8_t vTmpB = vmulq_s16(vU2B, vUU);

            uint8x8x3_t vRGB;
            int16x8_t vRR = vmaxq_s16(vdupq_n_s16(1), vqaddq_s16(vAdd0, vqaddq_s16(vTmp0, vTmpR)));
            int16x8_t vGG = vmaxq_s16(vdupq_n_s16(1), vqaddq_s16(vAdd0, vqsubq_s16(vTmp0, vTmpG)));
            int16x8_t vBB = vmaxq_s16(vdupq_n_s16(1), vqaddq_s16(vAdd0, vqaddq_s16(vTmp0, vTmpB)));
            vRGB.val[2] = vqmovn_u16(vrshrq_n_u16(vreinterpretq_u16_s16(vRR), 7));
            vRGB.val[1] = vqmovn_u16(vrshrq_n_u16(vreinterpretq_u16_s16(vGG), 7));
            vRGB.val[0] = vqmovn_u16(vrshrq_n_u16(vreinterpretq_u16_s16(vBB), 7));
            vst3_u8(&dstL0[3 * j], vRGB);
            
            vRR = vmaxq_s16(vdupq_n_s16(1), vqaddq_s16(vAdd1, vqaddq_s16(vTmp1, vTmpR)));
            vGG = vmaxq_s16(vdupq_n_s16(1), vqaddq_s16(vAdd1, vqsubq_s16(vTmp1, vTmpG)));
            vBB = vmaxq_s16(vdupq_n_s16(1), vqaddq_s16(vAdd1, vqaddq_s16(vTmp1, vTmpB)));
            vRGB.val[2] = vqmovn_u16(vrshrq_n_u16(vreinterpretq_u16_s16(vRR), 7));
            vRGB.val[1] = vqmovn_u16(vrshrq_n_u16(vreinterpretq_u16_s16(vGG), 7));
            vRGB.val[0] = vqmovn_u16(vrshrq_n_u16(vreinterpretq_u16_s16(vBB), 7));
            vst3_u8(&dstL1[3 * j], vRGB);
        }
    }
    return;
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
    //Yuv420sp2bgr(bgr_temp, yuv_ptr, yuv_ptr+width*height, width, height, 1920*3, 1920);
    file = fopen("xxx.bmp", "wb");

    fwrite(bgr_temp, sizeof(unsigned char), sizeof(unsigned char) * (width*height*3), file);

	fclose(file);//关闭yuv文件
}
_Bool InsUT_NV12_RGB_my(int32_t argc, char **argv) 
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
    //Yuv420sp2bgr(bgr_temp, yuv_ptr, yuv_ptr+width*height, width, height, 1920*3, 1920);
    file = fopen("xxx.bmp", "wb");

    fwrite(bgr_temp, sizeof(unsigned char), sizeof(unsigned char) * (width*height*3), file);

	fclose(file);//关闭yuv文件
}
