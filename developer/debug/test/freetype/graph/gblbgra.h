
  int                   h        = blit->height;
  const unsigned char*  src_line = blit->src_line;
  unsigned char*        dst_line = blit->dst_line;

  do
  {
    const unsigned char*  src = src_line + blit->src_x * 4;
    unsigned char*        dst = dst_line + blit->dst_x * GDST_INCR;
    int                   w   = blit->width;

    do
    {
      unsigned int  pix_b = src[0];
      unsigned int  pix_g = src[1];
      unsigned int  pix_r = src[2];
      unsigned int  a = src[3];


      if ( a == 0 )
      {
        /* nothing */
      }
      else if ( a == 255 )
      {
        GDST_STOREC(dst,pix_r,pix_g,pix_b);
      }
      else
      {
        GBlenderPixel  back;

        GDST_READ(dst,back);

        {
          unsigned int  ba = 255 - a;
          unsigned int  back_r = (back >> 16) & 255;
          unsigned int  back_g = (back >> 8) & 255;
          unsigned int  back_b = (back) & 255;

#if 1     /* premultiplied blending without gamma correction */
          pix_r = (back_r * ba / 255 + pix_r);
          pix_g = (back_g * ba / 255 + pix_g);
          pix_b = (back_b * ba / 255 + pix_b);

#else     /* gamma-corrected blending */
          const unsigned char*   gamma_ramp_inv = blit->blender->gamma_ramp_inv;
          const unsigned short*  gamma_ramp     = blit->blender->gamma_ramp;

          back_r = gamma_ramp[back_r];
          back_g = gamma_ramp[back_g];
          back_b = gamma_ramp[back_b];

          /* premultiplication undone */
          pix_r = gamma_ramp[pix_r * 255 / a];
          pix_g = gamma_ramp[pix_g * 255 / a];
          pix_b = gamma_ramp[pix_b * 255 / a];

          pix_r = gamma_ramp_inv[(back_r * ba + pix_r * a + 127) / 255];
          pix_g = gamma_ramp_inv[(back_g * ba + pix_g * a + 127) / 255];
          pix_b = gamma_ramp_inv[(back_b * ba + pix_b * a + 127) / 255];
#endif
        }

        GDST_STOREC(dst,pix_r,pix_g,pix_b);
      }

      src += 4;
      dst += GDST_INCR;

    } while ( --w > 0 );

    src_line += blit->src_pitch;
    dst_line += blit->dst_pitch;

  } while ( --h > 0 );
