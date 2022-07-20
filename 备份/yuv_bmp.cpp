#include "yuv_bmp.h"
#include "NEON_2_SSE.h"
#include "stdio.h"


/*
    R=Y+(V*204)>>7-223
    G=Y-(U*50)>>7-(V*104)>>7+135
    B=Y+2*U-277

    Y 
    R= Y+360*V>>8 -180
    G= Y -(88*U)>>8 -(184*V)>>8+136
    B= Y+(455*U)>>8 -228

    R[0]  --Y[0] U[0] V[0] 
    R[1]  --Y[1] U[0] V[0]

    R[2]  --Y[2] U[1] V[1] 
    R[3]  --Y[3] U[1] V[1]

    R[15] --Y[15] U[7] V[7]
*/


static void yuv_BGR( unsigned char *bgr, int width, int height, unsigned char *yuv_v)
{
    uint16x8_t V_R=vdupq_n_u16(204);
    uint16x8_t Y_R=vdupq_n_u16(298);

    uint16x8_t U_G=vdupq_n_u16(100);
    uint16x8_t V_G=vdupq_n_u16(208);

    uint16x8_t U_B=vdupq_n_u16(129);

    uint16x8_t R_const=vdupq_n_u16(223);
    uint16x8_t G_const=vdupq_n_u16(135);
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
    uint16x8_t R_vsum1 ;

    uint16x8_t B_vsum ;
    uint16x8_t B_vsum1 ;

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

       

        R_vsum= vmulq_u16 (v_data_copy, V_R);
        R_vsum1=vmulq_u16(vmovl_u8(y_data), Y_R);

        R_vsum=vrshrq_n_u16(R_vsum,7);
        R_vsum1=vrshrq_n_u16(R_vsum1,8);

        R_vsum=vqaddq_u16(R_vsum,R_vsum1);

        R_vsum=vqsubq_u16(R_vsum,R_const);
    
        G_vsum=vmulq_u16(u_data_copy, U_G);
        G_vsum2=vmulq_u16(vmovl_u8(y_data), Y_R);

        G_vsum=vrshrq_n_u16(G_vsum,8);
        G_vsum2=vrshrq_n_u16(G_vsum2,8);

        G_vsum1=vmulq_u16(v_data_copy, V_G);
        G_vsum1=vrshrq_n_u16(G_vsum1,8);

        G_vsum2=vqaddq_u16(G_vsum2,G_const);

        G_vsum=vqsubq_u16(G_vsum2,G_vsum);
        G_vsum=vqsubq_u16(G_vsum,G_vsum1);


        B_vsum=vmulq_u16(u_data_copy, U_B);
        B_vsum1=vmulq_u16(vmovl_u8(y_data), vdupq_n_u16(149));

        B_vsum=vrshrq_n_u16(B_vsum,6);//
        B_vsum1=vrshrq_n_u16(B_vsum1,7);

        B_vsum=vqaddq_u16(B_vsum,B_vsum1);
        B_vsum=vqsubq_u16(B_vsum,B_const);


        result.val[0]=vqmovn_u16(B_vsum);
        result.val[1]=vqmovn_u16(G_vsum);
        result.val[2]=vqmovn_u16(R_vsum);

    

        vst3_u8(temp_d, result);
        temp_d+=24;
        temp_s+=8;
        ptr_u+=4;
        ptr_v+=4;

    }
    if(j%2==0)
    {
        ptr_u=ptr_u-width/2;
        ptr_v=ptr_v-width/2;
    }
    
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
    
}
