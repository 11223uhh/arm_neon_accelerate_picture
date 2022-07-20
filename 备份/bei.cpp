#include<stdio.h>
#include "NEON_2_SSE.h"
#include<opencv2/opencv.hpp>
#include<opencv2/highgui.hpp>
using namespace cv;
using namespace std;
#include "NEON_2_SSE.h"
#include<opencv2/opencv.hpp>
#include<opencv2/highgui.hpp>
using namespace cv;
using namespace std;

void BGR2GRAY(const unsigned char *bgr, int width, int height, unsigned char *gray) 
{
    int coef_q8[3] = { 29, 150, 77 };
    unsigned char *temp_s = (unsigned char*)bgr;
    unsigned char *temp_d = gray;
    uint8x8_t vwr = vdup_n_u16(coef_q8[0]);
    uint8x8_t vwg = vdup_n_u16(coef_q8[1]);
    uint8x8_t vwb = vdup_n_u16(coef_q8[2]);
    
    for (int row = 0; row < height; row++) 
    {
        int col = 0;
        for ( ; col < width - 8; col += 8) 
        {
            printf("hello\n");
            uint8x8x3_t vsrc = vld3_u8(temp_s);
            uint16x8_t vsum = vmull_u8(vsrc.val[0], vwr);
        

            vsum = vmlal_u8(vsum, vsrc.val[1], vwg);
            vsum = vmlal_u8(vsum, vsrc.val[2], vwb);

            vst1_u8(temp_d, vshrn_n_u16(vsum, 8));
        }
    }
    

}
//int coef_q8[3] = { 29, 150, 77 };

int main()
{
    unsigned char test_bgr[24]={20,10,15,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10};
    unsigned char gray[8];
    Mat sour_image;
    sour_image=cv::imread("1.jpg",1);
    Mat gary_image=Mat::zeros(100, 100, CV_64F);;

    uchar * rgb_ptr = sour_image.ptr<uchar>(0);

    BGR2GRAY(test_bgr,9,1,gray);

    imshow("gray", img);
    waitKey(6000);
    /*
    for(int i=0;i<8;i++)
    printf("%d , ",gray[i]);*/

}
