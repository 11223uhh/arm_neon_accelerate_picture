/*
成功实现uv
static void BGR_yuv_v(int *coef_q8,const unsigned char *bgr, int width, int height, unsigned char *yuv_v,int step) 
{
    unsigned char *temp_s = (unsigned char*)bgr;

    unsigned char *temp_d = yuv_v;

    int16x8_t vwb = vdupq_n_s16(coef_q8[0]);
    int16x8_t vwg = vdupq_n_s16(coef_q8[1]);
    int16x8_t vwr = vdupq_n_s16(coef_q8[2]);
    const int16x8_t s16_zero = vdupq_n_s16(0);

    if(step==0)step=1;

    for (int count = 0; count < (height*width)/8; count++) 
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
        temp_s+=24;
        
    }
    return;
}

*/