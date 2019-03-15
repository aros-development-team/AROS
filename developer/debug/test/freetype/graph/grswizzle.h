#ifndef GRSWIZZLE_H_
#define GRSWIZZLE_H_

void
gr_swizzle_rect_rgb24( unsigned char*    read_buff,
                       int               read_pitch,
                       unsigned char*    write_buff,
                       int               write_pitch,
                       int               buff_width,
                       int               buff_height,
                       int               x,
                       int               y,
                       int               width,
                       int               height );

void
gr_swizzle_rect_rgb565( unsigned char*    read_buff,
                        int               read_pitch,
                        unsigned char*    write_buff,
                        int               write_pitch,
                        int               buff_width,
                        int               buff_height,
                        int               x,
                        int               y,
                        int               width,
                        int               height );

void
gr_swizzle_rect_xrgb32( unsigned char*    read_buff,
                        int               read_pitch,
                        unsigned char*    write_buff,
                        int               write_pitch,
                        int               buff_width,
                        int               buff_height,
                        int               x,
                        int               y,
                        int               width,
                        int               height );

#endif /* GRSWIZZLE_H_ */
