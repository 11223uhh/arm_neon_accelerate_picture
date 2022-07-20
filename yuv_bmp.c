#include "yuv_bmp.h"
#include "NEON_2_SSE.h"
#include "stdio.h"

/*
 0.0625 0.125 0.25 0.5 1 2 4 8 16
*/
/*
    YUV转RGB SDTV标准
    R=1.164*Y         +1.596*V+C
    G=1.164*Y-0.391*U -0.813*V+C
    B=1.164*Y+2.018*U    

    简化1.0
    temp=1.164*Y
    R=temp         +1.596*V+C
    G=temp-0.391*U -0.813*V+C
    B=temp+2.000*U   

    temp=149*Y>>7                   4次乘法 4次位移 8为加减

    R=temp         +1.596*V+223          
    G=temp-0.391*U -0.813*V+135
    B=temp+ U+U         +277

    temp=149*Y>>7                   4次乘法 3次位移 8为加减

    R=temp         +1.596*V+223          
    G=(temp-50*U-104*V)>>7+135  
    B=temp+ U+U         +277
    

    简化2.0
    temp=Y+Y>>3              ~1.125*Y


*/
static void yuv_BGR( unsigned char *bgr, int width, int height, unsigned char *yuv_v)
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
    unsigned char *ptr_u = temp_s+width*height;
    unsigned char *ptr_v = temp_s+width*height+(width*height)/4;
    int pitch = width/8;
    uint16x8_t G_vsum ;
    uint16x8_t G_vsum1 ;
    uint16x8_t G_vsum2 ;

    uint16x8_t R_vsum ;
    uint16x8_t B_vsum ;

    uint8x8x3_t result;
    for (int j = 0; j < height; ++j)
    {
        for (int i = 0; i < pitch; ++i)
        {
            uint8x8_t y_data = vld1_u8(temp_s);
            uint8x8_t u_data = vld1_u8(ptr_u);
            uint8x8_t v_data = vld1_u8(ptr_v);
            uint16x8_t u_data_copy = vmovl_u8(vzip_u8(u_data, u_data).val[0]);
            uint16x8_t v_data_copy =vmovl_u8(vzip_u8(v_data, v_data).val[0]);

            uint16x8_t temp_R =vmulq_u16(vmovl_u8(y_data), Y_R);
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
    

            result.val[0]=vqmovn_u16(B_vsum);
            result.val[1]=vqmovn_u16(G_vsum);
            result.val[2]=vqmovn_u16(R_vsum);

            vst3_u8(temp_d, result);
            temp_d+=24;
            temp_s+=8;
            ptr_u+=4;
            ptr_v+=4;

    }
        ptr_u=ptr_u-(1>>j%2)*(width/2);
        ptr_v=ptr_v-(1>>j%2)*(width/2);
    
    }

} 

_Bool InsUT_yuv420_RGB(int32_t argc, char **argv) 
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

    yuv_BGR( bgr_temp,  width,  height,yuv_ptr);

    file = fopen("xxx.bmp", "wb");

    fwrite(bgr_temp, sizeof(unsigned char), sizeof(unsigned char) * (width*height*3), file);

	fclose(file);//关闭yuv文件
    /*
    printf("y=\n");
    for(int i=0;i<400;i++)
    {
        printf("count%d %d, \n", i,yuv_ptr[i]);
    }
    printf("\n");
    printf("u=\n");
    for(int i=0;i<(400)/2;i++)
    {
        printf("count%d,%d, \n", i,yuv_ptr[i+width*height]);
    }

    printf("\n");
    printf("v=\n");
    for(int i=0;i<(400)/2;i++)
    {
        printf("count%d %d, \n", i,yuv_ptr[i+width*height+(width*height)/4]);
    }
    printf("\n\n\n");
    
   
    for(int i=0;i<400*3;i+=3)
    {
        printf("count=%d, %B=%d,G=%d, R=%d\n",(i/3),bgr_temp[i],bgr_temp[i+1],bgr_temp[i+2]);
    }
    */
}
