
  GBLENDER_VARS(blender,color);

  int                   h        = blit->height;
  const unsigned char*  src_line = blit->src_line;
  unsigned char*        dst_line = blit->dst_line;

  gblender_use_channels( blender, 0 );

  /* make compiler happy */
  (r)=(r);
  (g)=(g);
  (b)=(b);

  do
  {
    const unsigned char*  src = src_line + (blit->src_x);
    unsigned char*        dst = dst_line + blit->dst_x*GDST_INCR;
    int                   w   = blit->width;

    do
    {
      int  a = GBLENDER_SHADE_INDEX(src[0]);

      if ( a == 0 )
      {
        /* nothing */
      }
      else if ( a == GBLENDER_SHADE_COUNT-1 )
      {
        GDST_COPY(dst);
      }
      else
      {
        GBlenderPixel  back;

        GDST_READ(dst,back);

        GBLENDER_LOOKUP( blender, back );

#ifdef GBLENDER_STORE_BYTES
        GDST_STOREB(dst,_gcells,a);
#else
        GDST_STOREP(dst,_gcells,a);
#endif
      }

      src += 1;
      dst += GDST_INCR;
    }
    while (--w > 0);

    src_line += blit->src_pitch;
    dst_line += blit->dst_pitch;
  }
  while (--h > 0);

  GBLENDER_CLOSE(blender);
