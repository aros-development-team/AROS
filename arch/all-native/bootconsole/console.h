#ifndef SCREEN_H_
#define SCREEN_H_

#define VGA_TEXT_ADDR   (void *)0xb8000
#define VGA_TEXT_WIDTH  80
#define VGA_TEXT_HEIGHT 25

extern unsigned int  scr_BytesPerLine;
extern unsigned int  scr_BytesPerPix;

extern unsigned int  scr_XPos;
extern unsigned int  scr_YPos;

extern const unsigned int  fontWidth;
extern const unsigned int  fontHeight;
extern const unsigned char fontData[];

#endif /*SCREEN_H_*/
