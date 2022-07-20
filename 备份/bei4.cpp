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
    const int16x8_t s16_zero = vdupq_n_s16(0);

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
        vsum = vmaxq_s16(vsum, s16_zero);

        uint8x8_t result=vqrshrun_n_s16(vsum,8) ;//uint16转 uint8

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
    int coef_q8[3][3]={{ 15, 174, 67 },{ 128, -92, -36 },{ -10, -118, 127 }};
    BGR_yuv_y(coef_q8[0],bgr,width, height, yuv_v);

    BGR_yuv_uv(coef_q8[1],bgr,width, height, yuv_v+width*height,4);

    BGR_yuv_uv(coef_q8[2],bgr,width, height, (yuv_v+width*height+width*height/4),4);


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
    
    for(int i=0;i<sour_image.cols*sour_image.rows;i++)
    {
        printf("gray_%d=%d, \n",i,yuv_ptr[i]);
    }
    printf("\n\n\n");

    for(int i=0;i<(sour_image.cols*sour_image.rows)/4;i++)
    {
        printf("u %d=%d, \n",i,yuv_ptr[i+sour_image.cols*sour_image.rows]);
    }
    printf("\n\n\n");
    for(int i=0;i<(sour_image.cols*sour_image.rows)/4;i++)
    {
        printf("v %d=%d, \n",i,yuv_ptr[i+sour_image.cols*sour_image.rows+sour_image.cols*sour_image.rows/4]);
    }
    printf("\n\n\n");

    printf("\n\n\n");
    
    get_gray(gra_ptr, sour_image.cols,sour_image.rows, yuv_ptr);

    imshow("gray", gary_image);
    waitKey(0);
 
    return 0;
  

}
