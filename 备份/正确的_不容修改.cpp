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

  Y =  (0.257*R) + (0.504*G) + (0.098*B) + 16

Cb = U = -(0.148*R) - (0.291*G) + (0.439*B) + 128

Cr = V =  (0.439*R) - (0.368*G) - (0.071*B) + 128        
         

*/
// vsrc 0 通道 B 1 通道G

// step 0 不间隔采样
// step 1 不间隔采样

// step 4 每隔4个采样

static void BGR_yuv_uv(int *coef_q8,const unsigned char *bgr, int width, int height, unsigned char *yuv_v,int step) 
{
    unsigned char *temp_s = (unsigned char*)bgr;

    unsigned char *temp_d = yuv_v;

    int16x8_t vwb = vdupq_n_s16(coef_q8[0]);
    int16x8_t vwg = vdupq_n_s16(coef_q8[1]);
    int16x8_t vwr = vdupq_n_s16(coef_q8[2]);

    const uint8x8_t u8_zero = vdup_n_u8(0);
    const uint8x8_t u8_16 = vdup_n_u8(16);
    const uint16x8_t u16_rounding = vdupq_n_u16(128);

    const int16x8_t s16_zero = vdupq_n_s16(0);
    const int8x8_t s8_rounding = vdup_n_s8(127);
    const int16x8_t s16_rounding = vdupq_n_s16(128);

    if(step==0)step=1;

    for (int count = 0; count < (height*width)/(8*step); count++) 
    {
        uint8x8x3_t vsrc = vld3_u8( temp_s);//加载8个点的RGB数据

            /*由于有负数 uint8数据转 int16数据 */
        uint16x8_t B_data = vmovl_u8(vsrc.val[0]);
        int16x8_t  B_data_new=vreinterpretq_s16_u16(B_data);
        uint16x8_t g_data = vmovl_u8(vsrc.val[1]);
        int16x8_t  g_data_new=vreinterpretq_s16_u16(g_data);
        uint16x8_t r_data = vmovl_u8(vsrc.val[2]);
        int16x8_t  r_data_new=vreinterpretq_s16_u16(r_data);

        int16x8_t vsum ;
        vsum=vmulq_s16(B_data_new, vwb);
        vsum = vmlaq_s16(vsum,g_data_new, vwg);
        vsum = vmlaq_s16(vsum,r_data_new, vwr);

        int8x8_t vsum_s8 = vqshrn_n_s16(vsum, 8);// int16 -int8
        vsum = vaddl_s8(vsum_s8, s8_rounding);//// int8-int16 加上127

        uint16x8_t u_vsum_ = vreinterpretq_u16_s16(vsum);// int16转uint16
        uint8x8_t result=vqmovn_u16(u_vsum_);// uint16转int8
        

        vst1_u8(temp_d, result);
        temp_d+=8;
        temp_s+=24*step;
        
    }
    return;
}


static void BGR_yuv_y(int *coef_q8,const unsigned char *bgr, int width, int height, unsigned char *gray) {
    unsigned char *temp_s = (unsigned char*)bgr;
    unsigned char *temp_d = gray;
    uint8x8_t vwr = vdup_n_u8(coef_q8[0]);
    uint8x8_t vwg = vdup_n_u8(coef_q8[1]);
    uint8x8_t vwb = vdup_n_u8(coef_q8[2]);

    for (int count = 0; count < (height*width)/8; count++) 
    {
        uint8x8x3_t vsrc = vld3_u8(temp_s);
        uint16x8_t vsum = vmull_u8(vsrc.val[0], vwr);
        vsum = vmlal_u8(vsum, vsrc.val[1], vwg);
        vsum = vmlal_u8(vsum, vsrc.val[2], vwb);
        vst1_u8(temp_d, vshrn_n_u16(vsum, 8));
        temp_d += 8;
        temp_s += 24;
    }
    return;
}
static void BGR_yuv(const unsigned char *bgr, int width, int height, unsigned char *yuv_v) 
{
    int coef_q8[3][3]={{ 25, 129, 66 },{ 112, -74, -38 },{ -18, -94, 112 }};
    BGR_yuv_y(coef_q8[0],bgr,width, height, yuv_v);

    BGR_yuv_uv(coef_q8[1],bgr,width, height, yuv_v+width*height,1);

    BGR_yuv_uv(coef_q8[2],bgr,width, height, (yuv_v+width*height*2),1);


}
void get_gray( unsigned char *gra, int width, int height, unsigned char *yuv_v)
{
    for(int i=0;i<width*height;i++)
    {
        *gra=*yuv_v;
        gra++;yuv_v++;
    }
}
int main()
{


    Mat sour_image;
    sour_image=cv::imread("1.jpg",1);

    Mat YUV_420;
    YUV_420.create(sour_image.rows,sour_image.cols,CV_8UC3);//创建空白yuv矩阵
    
    Mat gary_image=Mat::zeros(sour_image.rows,sour_image.cols, CV_8UC1);;

    uchar * bgr_ptr = sour_image.ptr<uchar>(0);
    
    unsigned char * gra_ptr = gary_image.ptr<uchar>(0);
    unsigned char * yuv_ptr = YUV_420.ptr<uchar>(0);




    BGR_yuv(bgr_ptr,sour_image.cols, sour_image.rows, yuv_ptr) ;
    /*
   for(int i=0;i<sour_image.cols*sour_image.rows*3;i+=3)
    printf("count %d b=%d,g=%d,r=%d\n",(i/3),bgr_ptr[i],bgr_ptr[i+1],bgr_ptr[i+2]);
    printf("\n\n\n");

    for(int i=0;i<sour_image.cols*sour_image.rows;i++)
    {
        printf("gray_%d=%d, \n",i,yuv_ptr[i]);
    }
    printf("\n\n\n");

    for(int i=0;i<(sour_image.cols*sour_image.rows);i++)
    {
        printf("u %d=%d, \n",i,yuv_ptr[i+sour_image.cols*sour_image.rows]);
    }
    printf("\n\n\n");
    for(int i=0;i<(sour_image.cols*sour_image.rows);i++)
    {
        printf("v %d=%d, \n",i,yuv_ptr[i+sour_image.cols*sour_image.rows*2]);
    }
    printf("\n\n\n");

    printf("\n\n\n");
    */
    std::ofstream ofs("xxx.yuv", ofstream::binary);
    ofs.write(reinterpret_cast<const char*>(yuv_ptr), sizeof(unsigned char) * sour_image.rows*sour_image.cols*3);
    ofs.close();

 
    return 0;
  

}