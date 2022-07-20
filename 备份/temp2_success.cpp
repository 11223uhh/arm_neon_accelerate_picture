#include<stdio.h>
#include "NEON_2_SSE.h"
#include<opencv2/opencv.hpp>
#include<opencv2/highgui.hpp>


#include<time.h>

#define USE_SSE4 1
using namespace cv;
using namespace std;

#define USE_NEON 1


/*  BT.2020(高清)标准 rgb转yuv

    Y = 0.2627 * R + 0.6780 * G + 0.0593 * B         　　　　  
    U =-0.1396*R - 0.3604*G + 0.5*B;              
    V = 0.5*R - 0.4598*G -0.0402*B;            
         

*/

static void BGR_yuv_444(const unsigned char *bgr, int width, int height, unsigned char *yuv) 
{

    int coef_q8[3][3]={{ 15, 174, 67 },{ 128, -92, -36 },{ -10, -118, 127 }};
    unsigned char *temp_s = (unsigned char*)bgr;
    unsigned char *temp_d = yuv;

    
    int16x8_t vwr_y = vdupq_n_s16(coef_q8[0][0]);
    int16x8_t vwg_y = vdupq_n_s16(coef_q8[0][1]);
    int16x8_t vwb_y = vdupq_n_s16(coef_q8[0][2]);

    int16x8_t vwr_u = vdupq_n_s16(coef_q8[1][0]);
    int16x8_t vwg_u = vdupq_n_s16(coef_q8[1][1]);
    int16x8_t vwb_u = vdupq_n_s16(coef_q8[1][2]);

    int16x8_t vwr_v = vdupq_n_s16(coef_q8[2][0]);
    int16x8_t vwg_v = vdupq_n_s16(coef_q8[2][1]);
    int16x8_t vwb_v = vdupq_n_s16(coef_q8[2][2]);

    //printf("Y parma=%d\n",vwr_v.m64_i16[0]);
    //printf("u parma=%d\n",vwg_v.m64_i16[0]);
   // printf("v parma=%d\n",vwb_v.m64_i16[0]);

    //printf("width%d,height%d\n",width,height);
   
    for (int row = 0; row < height; row++) {
        int col = 0;
        for ( ; col < width - 8; col += 8) {

            uint8x8x3_t vsrc = vld3_u8(temp_s);

            //printf("b=%d,g=%d,r=%d\n",vsrc.val[0].m64_u8[0],vsrc.val[1].m64_u8[0],vsrc.val[2].m64_u8[0]);
            int16x8_t vsum ;

            vsum=vmulq_s16(vmovl_u8(vsrc.val[0]), vwr_y);
            vsum = vmlaq_s16(vsum,vmovl_u8(vsrc.val[1]), vwg_y);
            vsum = vmlaq_s16(vsum,vmovl_u8(vsrc.val[2]), vwb_y);
            vst1_u8(temp_d, vqshrun_n_s16(vsum, 8));
            printf("%d\n",*temp_d);
            temp_d+=8;

            vsum=vmulq_s16(vmovl_u8(vsrc.val[0]), vwr_u);
            vsum = vmlaq_s16(vsum,vmovl_u8(vsrc.val[1]), vwg_u);
            vsum = vmlaq_s16(vsum,vmovl_u8(vsrc.val[2]), vwb_u);
            vst1_u8(temp_d, vqshrun_n_s16(vsum, 8));
            printf("%d\n",*temp_d);
            temp_d+=8;  
            vsum=vmulq_s16(vmovl_u8(vsrc.val[0]), vwr_v);
            vsum = vmlaq_s16(vsum,vmovl_u8(vsrc.val[1]), vwg_v);
            vsum = vmlaq_s16(vsum,vmovl_u8(vsrc.val[2]), vwb_v);
            vst1_u8(temp_d, vqshrun_n_s16(vsum, 8));
            printf("%d\n",*temp_d);
            
        }
    }
}

int main()
{


    const unsigned char sour_image[24]={20,10,15,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10};

    unsigned char yuv[24];
    int gary_image[8];


    BGR_yuv_444(sour_image,9, 1, yuv) ;

    for(int i=0;i<24;i++)
    printf("%d, ",yuv[i]);


    return 0;
  

}
