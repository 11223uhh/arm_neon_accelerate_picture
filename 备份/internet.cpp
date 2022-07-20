


typedef unsigned char byte;
void RGB_to_NV12_intrinsic(byte* pNV12, byte* pRGB, int width, int height)
{
    const uint8x8_t u8_zero = vdup_n_u8(0);
    const uint8x8_t u8_16 = vdup_n_u8(16);
    const uint16x8_t u16_rounding = vdupq_n_u16(128);

    const int16x8_t s16_zero = vdupq_n_s16(0);
    const int8x8_t s8_rounding = vdup_n_s8(-128);
    const int16x8_t s16_rounding = vdupq_n_s16(128);

    byte* UVPtr = pNV12 + width * height;
    int pitch = width >> 4;

    for (int j = 0; j < height; ++j)
    {
        for (int i = 0; i < pitch; ++i)
        {
            // Load RGB 16 pixel
            uint8x16x3_t pixel_rgb = vld3q_u8(pRGB);

            uint8x8_t high_r = vget_high_u8(pixel_rgb.val[0]);
            uint8x8_t low_r = vget_low_u8(pixel_rgb.val[0]);
            uint8x8_t high_g = vget_high_u8(pixel_rgb.val[1]);
            uint8x8_t low_g = vget_low_u8(pixel_rgb.val[1]);
            uint8x8_t high_b = vget_high_u8(pixel_rgb.val[2]);
            uint8x8_t low_b = vget_low_u8(pixel_rgb.val[2]);

            // NOTE:
            // declaration may not appear after executable statement in block
            uint16x8_t high_y;
            uint16x8_t low_y;           

            uint8x8_t scalar = vdup_n_u8(66);  // scalar = 66
            high_y = vmull_u8(high_r, scalar);  // Y = R * 66
            low_y = vmull_u8(low_r, scalar);

            scalar = vdup_n_u8(129);
            high_y = vmlal_u8(high_y, high_g, scalar);  // Y = Y + R*129
            low_y = vmlal_u8(low_y, low_g, scalar);

            scalar = vdup_n_u8(25);
            high_y = vmlal_u8(high_y, high_b, scalar);  // Y = Y + B*25
            low_y = vmlal_u8(low_y, low_b, scalar);

            high_y = vaddq_u16(high_y, u16_rounding);  // Y = Y + 128
            low_y = vaddq_u16(low_y, u16_rounding);

            uint8x8_t u8_low_y = vshrn_n_u16(low_y, 8);  // Y = Y >> 8
            uint8x8_t u8_high_y = vshrn_n_u16(high_y, 8);

            low_y = vaddl_u8(u8_low_y, u8_16);  // Y = Y + 16
            high_y = vaddl_u8(u8_high_y, u8_16);

            uint8x16_t pixel_y = vcombine_u8(vqmovn_u16(low_y), vqmovn_u16(high_y));

            // Store
            vst1q_u8(pNV12, pixel_y);  

            if (j % 2 == 0)
            {
                uint8x8x2_t mix_r = vuzp_u8(low_r, high_r);
                uint8x8x2_t mix_g = vuzp_u8(low_g, high_g);
                uint8x8x2_t mix_b = vuzp_u8(low_b, high_b);

                int16x8_t signed_r = vreinterpretq_s16_u16(vaddl_u8(mix_r.val[0], u8_zero));
                int16x8_t signed_g = vreinterpretq_s16_u16(vaddl_u8(mix_g.val[0], u8_zero));
                int16x8_t signed_b = vreinterpretq_s16_u16(vaddl_u8(mix_b.val[0], u8_zero));

                int16x8_t signed_u;
                int16x8_t signed_v;

                int16x8_t signed_scalar = vdupq_n_s16(-38);
                signed_u = vmulq_s16(signed_r, signed_scalar);

                signed_scalar = vdupq_n_s16(112);
                signed_v = vmulq_s16(signed_r, signed_scalar);

                signed_scalar = vdupq_n_s16(-74);
                signed_u = vmlaq_s16(signed_u, signed_g, signed_scalar);

                signed_scalar = vdupq_n_s16(-94);
                signed_v = vmlaq_s16(signed_v, signed_g, signed_scalar);

                signed_scalar = vdupq_n_s16(112);
                signed_u = vmlaq_s16(signed_u, signed_b, signed_scalar);

                signed_scalar = vdupq_n_s16(-18);
                signed_v = vmlaq_s16(signed_v, signed_b, signed_scalar);

                signed_u = vaddq_s16(signed_u, s16_rounding);
                signed_v = vaddq_s16(signed_v, s16_rounding);

                int8x8_t s8_u = vshrn_n_s16(signed_u, 8);
                int8x8_t s8_v = vshrn_n_s16(signed_v, 8);

                signed_u = vsubl_s8(s8_u, s8_rounding);
                signed_v = vsubl_s8(s8_v, s8_rounding);

                signed_u = vmaxq_s16(signed_u, s16_zero);
                signed_v = vmaxq_s16(signed_v, s16_zero);

                uint16x8_t unsigned_u = vreinterpretq_u16_s16(signed_u);
                uint16x8_t unsigned_v = vreinterpretq_u16_s16(signed_v);

                uint8x8x2_t result;
                result.val[0] = vqmovn_u16(unsigned_u);
                result.val[1] = vqmovn_u16(unsigned_v);

                vst2_u8(UVPtr, result);
                UVPtr += 16;
            }

            pRGB += 3*16;
            pNV12 += 16;
        }
    }
}