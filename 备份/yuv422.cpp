void encodeYUV420SP_NEON_Intrinsics(unsigned char *__restrict__ yuv420sp,
                                    unsigned char *__restrict__ argb, int width, int height) {
    const uint16x8_t u16_rounding = vdupq_n_u16(128);
    const int16x8_t s16_rounding = vdupq_n_s16(128); // +128， u/v 中内层的 +128
    const int8x8_t s8_rounding = vdup_n_s8(
            128); // -128，即 0x80，最高成了符号位，实际只有 7 位用来表示数字，用来处理符号位， u/v 中外层的 +128
    const uint8x16_t offset = vdupq_n_u8(16);
    const uint16x8_t mask = vdupq_n_s16(255);
 
//    测试
//    int16x8_t test = vaddl_s8 (s8_rounding, s8_rounding);// -256
//    int8x8_t test_0 = vdup_n_s8(127); // 正常为 127
//    int8x8_t test_1 = vadd_s8(test_0, test_0); // -2，因为计算溢出到符号位
 
    int frameSize = width * height;
 
    int yIndex = 0;
    int uvIndex = frameSize;
 
    int i;
    int j;
    for (j = 0; j < height; j++) {
        for (i = 0; i < width >> 4; i++) 
        {
            // Load rgb
            uint8x16x4_t pixel_argb = vld4q_u8(argb);
            argb += 4 * 16;
 
            uint8x8x2_t uint8_r;
            uint8x8x2_t uint8_g;
            uint8x8x2_t uint8_b;
            uint8_r.val[0] = vget_low_u8(pixel_argb.val[2]);
            uint8_r.val[1] = vget_high_u8(pixel_argb.val[2]);
            uint8_g.val[0] = vget_low_u8(pixel_argb.val[1]);
            uint8_g.val[1] = vget_high_u8(pixel_argb.val[1]);
            uint8_b.val[0] = vget_low_u8(pixel_argb.val[0]);
            uint8_b.val[1] = vget_high_u8(pixel_argb.val[0]);
 
            // Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;
            uint16x8x2_t uint16_y;
 
            uint8x8_t scalar = vdup_n_u8(66);
            uint8x16_t y;
 
            uint16_y.val[0] = vmull_u8(uint8_r.val[0], scalar);
            uint16_y.val[1] = vmull_u8(uint8_r.val[1], scalar);
            scalar = vdup_n_u8(129);
            uint16_y.val[0] = vmlal_u8(uint16_y.val[0], uint8_g.val[0], scalar);
            uint16_y.val[1] = vmlal_u8(uint16_y.val[1], uint8_g.val[1], scalar);
            scalar = vdup_n_u8(25);
            uint16_y.val[0] = vmlal_u8(uint16_y.val[0], uint8_b.val[0], scalar);
            uint16_y.val[1] = vmlal_u8(uint16_y.val[1], uint8_b.val[1], scalar);
 
            uint16_y.val[0] = vaddq_u16(uint16_y.val[0], u16_rounding);
            uint16_y.val[1] = vaddq_u16(uint16_y.val[1], u16_rounding);
 
            y = vcombine_u8(vqshrn_n_u16(uint16_y.val[0], 8), vqshrn_n_u16(uint16_y.val[1], 8));
            y = vaddq_u8(y, offset);
 
            vst1q_u8(yuv420sp + yIndex, y);
            yIndex += 16;
 
            // 在偶数行中计算 U 和 V
            if (j % 2 == 0) {
 
//                uint8_t U = ((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
//                uint8_t V = ((112 * R - 94 * G - 18 * B + 128) >> 8) + 128;
 
                int16x8_t u_scalar = vdupq_n_s16(-38);
                int16x8_t v_scalar = vdupq_n_s16(112);
 
#if 1
                // 因为 u,v 的值只有 y 的一半，所以只取高位计算
                int16x8_t r = vreinterpretq_s16_u16(
                        vandq_u16(vreinterpretq_u16_u8(pixel_argb.val[2]), mask));
 
//                 测试
//                uint16x8_t test_0 = vreinterpretq_u16_u8(pixel_argb.val[2]);
//                uint16x8_t test_1 = vandq_u16(test_0, mask);
//                int16x8_t  test_2 = vreinterpretq_s16_u16(test_1);
 
                int16x8_t g = vreinterpretq_s16_u16(
                        vandq_u16(vreinterpretq_u16_u8(pixel_argb.val[1]), mask));
                int16x8_t b = vreinterpretq_s16_u16(
                        vandq_u16(vreinterpretq_u16_u8(pixel_argb.val[0]), mask));

#endif
                int16x8_t u;
                int16x8_t v;
                uint8x8x2_t uv;
 
                u = vmulq_s16(r, u_scalar);
                v = vmulq_s16(r, v_scalar);
 
                u_scalar = vdupq_n_s16(-74);
                v_scalar = vdupq_n_s16(-94);
                u = vmlaq_s16(u, g, u_scalar);
                v = vmlaq_s16(v, g, v_scalar);
 
                u_scalar = vdupq_n_s16(112);
                v_scalar = vdupq_n_s16(-18);
                u = vmlaq_s16(u, b, u_scalar);
                v = vmlaq_s16(v, b, v_scalar);
 
                u = vaddq_s16(u, s16_rounding);
                v = vaddq_s16(v, s16_rounding);
 
                uv.val[1] = vreinterpret_u8_s8(vadd_s8(vqshrn_n_s16(u, 8), s8_rounding));
//                 测试
//                int8x8_t test_3 = vqshrn_n_s16(u, 8);
//                int8x8_t test_4 = vadd_s8(test_3, s8_rounding); //
//                uint8x8_t test_5 = vreinterpret_u8_s8(test_4);
 
                uv.val[0] = vreinterpret_u8_s8(vadd_s8(vqshrn_n_s16(v, 8), s8_rounding));
 
                vst2_u8(yuv420sp + uvIndex, uv);
 
                uvIndex += 2 * 8;
            }
        }
 
        // 处理余数的好办法
        for (i = ((width >> 4) << 4); i < width; i++) {
            uint8_t R = argb[2];
            uint8_t G = argb[1];
            uint8_t B = argb[0];
            argb += 4;
 
            // well known RGB to YUV algorithm
            uint8_t Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;
            uint8_t U = ((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
            uint8_t V = ((112 * R - 94 * G - 18 * B + 128) >> 8) + 128;
 
            // NV21有一个 Y 平面和 V-U 交叉平面，每一个平面的采样值都是 2
            // 意思是每4个 Y 像素（上下左右，不是横向连续的四个）对应1个 V 和1个 U
            // 像素和其他扫描线。
            yuv420sp[yIndex++] = Y;
            if (j % 2 == 0 && i % 2 == 0) {
                yuv420sp[uvIndex++] = V;
                yuv420sp[uvIndex++] = U;
            }
        }
    }
}