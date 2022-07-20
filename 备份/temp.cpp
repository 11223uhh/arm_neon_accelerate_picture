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

    int coef_q8[3][3]={{ 15, 174, 67 },{ 128, -92, -36 },{ -10, -118, 128 }};
    unsigned char *temp_s = (unsigned char*)bgr;
    unsigned char *temp_d = yuv;

    
    uint8x8_t vwr_y = vdup_n_u8(coef_q8[0][0]);
    uint8x8_t vwg_y = vdup_n_u8(coef_q8[0][1]);
    uint8x8_t vwb_y = vdup_n_u8(coef_q8[0][2]);

    uint8x8_t vwr_u = vdup_n_u8(coef_q8[1][0]);
    uint8x8_t vwg_u = vdup_n_u8(coef_q8[1][1]);
    uint8x8_t vwb_u = vdup_n_u8(coef_q8[1][2]);

    uint8x8_t vwr_v = vdup_n_u8(coef_q8[2][0]);
    uint8x8_t vwg_v = vdup_n_u8(coef_q8[2][1]);
    uint8x8_t vwb_v = vdup_n_u8(coef_q8[2][2]);
    
    for (int row = 0; row < height; row++) {
        int col = 0;
        for ( ; col < width - 8; col += 8) {
            for(int i=0;i<3;i++){
            uint8x8x3_t vsrc = vld3_u8(temp_s);

            uint16x8_t vsum = vmull_u8(vsrc.val[0], vwr_y);
            vsum = vmlal_u8(vsum, vsrc.val[1], vwg_y);
            vsum = vmlal_u8(vsum, vsrc.val[2], vwb_y);
            vst1_u8(temp_d, vshrn_n_u16(vsum, 8));
            temp_d += 8;

            vsum = vmull_u8(vsrc.val[0], vwr_u);
            vsum = vmlal_u8(vsum, vsrc.val[1], vwg_u);
            vsum = vmlal_u8(vsum, vsrc.val[2], vwb_u);
            vst1_u8(temp_d, vshrn_n_u16(vsum, 8));
            temp_d += 8;

            vsum = vmull_u8(vsrc.val[0], vwr_v);
            vsum = vmlal_u8(vsum, vsrc.val[1], vwg_v);
            vsum = vmlal_u8(vsum, vsrc.val[2], vwb_v);
            vst1_u8(temp_d, vshrn_n_u16(vsum, 8));
            temp_d += 8;
            }
        temp_s += 24;
        }
    }
}
void get_gray_from_yuv(const unsigned char *gray, int width, int height, unsigned char *yuv) 
{
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width ; col ++) {
            *gray=*yuv;
            yuv+=8;
            gray++;
        }
    }
}
int main()
{

    Mat sour_image;
    sour_image=cv::imread("1.jpg",1);

    Mat YUV_444;
    YUV_444.create(sour_image.rows,sour_image.cols,CV_8UC3);//创建空白yuv矩阵
    
    Mat gary_image=Mat::zeros((sour_image.rows,sour_image.cols, CV_64F));

    uchar * bgr_ptr = sour_image.ptr<uchar>(0);
    uchar * yuv_ptr = YUV_444.ptr<uchar>(0);

    BGR_yuv_444(bgr_ptr,sour_image.cols, sour_image.rows, yuv_ptr) ;

    get_gray_from_yuv(const unsigned char *gray, int width, int height, unsigned char *yuv)

    imshow("gray", gary_image);
    waitKey(1);
    return 0;
  

}
