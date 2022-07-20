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

        vsum=vmulq_s16(B_data, vwb);
        vsum = vmlaq_s16(vsum,g_data, vwg);
        vsum = vmlaq_s16(vsum,r_data, vwr);

        vsum = vaddq_s16(vsum, s16_rounding);

        int8x8_t vsum_int8=vqshrn_n_s16(vsum,8) ; //int 16-int8
        vsum=vaddl_s8(vsum_int8,s8_rounding);//int 8-int16 加上127

        //uint16x8_t u_vsum_ = vreinterpretq_u16_s16(vsum);// int16转uint16
        //uint8x8_t result=vqmovn_u16(u_vsum_);// uint16转uint8

        uint8x8_t result=vqshrun_n_s16(vsum,0);//int16-uint8

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
    uint8x8_t vwr = vdup_n_u8(coef_q8[0]);
    uint8x8_t vwg = vdup_n_u8(coef_q8[1]);
    uint8x8_t vwb = vdup_n_u8(coef_q8[2]);
    for (int row = 0; row < height; row++) {
        int col = 0;
        for ( ; col < width - 8; col += 8) {
            uint8x8x3_t vsrc = vld3_u8(temp_s);
            uint16x8_t vsum = vmull_u8(vsrc.val[0], vwr);
            vsum = vmlal_u8(vsum, vsrc.val[1], vwg);
            vsum = vmlal_u8(vsum, vsrc.val[2], vwb);
            vst1_u8(temp_d, vshrn_n_u16(vsum, 8));
            temp_d += 8;
            temp_s += 24;
        }

    }
    return;
}
static void BGR_yuv(const unsigned char *bgr, int width, int height, unsigned char *yuv_v) 
{
    int coef_q8[3][3]={{ 29, 150, 77 },{ 112, -74, -38 },{ -18, -94, 112 }};
    BGR_yuv_y(coef_q8[0],bgr,width, height, yuv_v);
    BGR_yuv_uv(coef_q8[1],bgr,  width,  height, yuv_v+width*height) ;
    BGR_yuv_uv(coef_q8[2],bgr,  width,  height, yuv_v+width*height+width*(height/4)) ;


}
void get_gray( unsigned char *gra, int width, int height, unsigned char *yuv_v)
{
    for(int i=0;i<width*height;i++)
    {
        *gra=*yuv_v;
        gra++;yuv_v++;
    }
}


#include "Windows.h"
//几个全局变量，存放读入图像的位图数据、宽、高、颜色表及每像素所占位数(比特) 
//此处定义全局变量主要为了后面的图像数据访问及图像存储作准备
unsigned char* pBmpBuf;//读入图像数据的指针
int bmpWidth;//图像的宽
int bmpHeight;//图像的高
RGBQUAD* pColorTable;//颜色表指针
int biBitCount;//图像类型

bool readBmp(char* bmpName)
{
	//二进制读方式打开指定的图像文件
	FILE* fp = fopen(bmpName, "rb");
	if (fp == 0) return 0;
 
 
	//跳过位图文件头结构BITMAPFILEHEADER
	fseek(fp, sizeof(BITMAPFILEHEADER), 0);
 
 
	//定义位图信息头结构变量，读取位图信息头进内存，存放在变量head中
	BITMAPINFOHEADER head;
	fread(&head, sizeof(BITMAPINFOHEADER), 1, fp);
 
	//获取图像宽、高、每像素所占位数等信息 100 *100 8bit图像
	bmpWidth = head.biWidth;
	bmpHeight = head.biHeight;
	biBitCount = head.biBitCount;
 
	//定义变量，计算图像每行像素所占的字节数（必须是4的倍数）
	int lineByte = (bmpWidth * biBitCount / 8 + 3) / 4 * 4;
 
	//灰度图像有颜色表，且颜色表表项为256
	if (biBitCount == 8) {
		//申请颜色表所需要的空间，读颜色表进内存
		pColorTable = new RGBQUAD[256];
		fread(pColorTable, sizeof(RGBQUAD), 256, fp);
	}
 
	//申请位图数据所需要的空间，读位图数据进内存
	pBmpBuf = new unsigned char[lineByte * bmpHeight];
	fread(pBmpBuf, 1, lineByte * bmpHeight, fp);
 
	//关闭文件
	fclose(fp);
 
	return 1;
}


int main()
{


    Mat sour_image;
    sour_image=cv::imread("1.jpg",1);


    unsigned char yuv_ptr[sour_image.cols*sour_image.rows+sour_image.cols*(sour_image.rows/2)];




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

    for(int i=0;i<(sour_image.cols*sour_image.rows)/4;i++)
    {
        printf("u %d=%d, \n",i,yuv_ptr[i+sour_image.cols*sour_image.rows]);
    }
    printf("\n\n\n");
    for(int i=0;i<(sour_image.cols*sour_image.rows)/4;i++)
    {
        printf("v %d=%d, \n",i,yuv_ptr[i+sour_image.cols*sour_image.rows+sour_image.cols*(sour_image.rows/4)]);
    }
    printf("\n\n\n");

    printf("\n\n\n");
    */
    std::ofstream ofs("xxx.yuv", ofstream::binary);
    ofs.write(reinterpret_cast<const char*>(yuv_ptr), sizeof(unsigned char) * (sour_image.rows*sour_image.cols*1.5));
    ofs.close();
 
    return 0;
  

}