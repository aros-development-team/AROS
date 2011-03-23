#ifndef SCREEN_H_
#define SCREEN_H_

enum scr_type
{
    SCR_UNKNOWN,
    SCR_TEXT,
    SCR_GFX
};

extern unsigned char scr_Type;

extern void         *scr_FrameBuffer;
extern unsigned int  scr_Width;
extern unsigned int  scr_Height;
extern unsigned int  scr_BytesPerLine;
extern unsigned int  scr_BytesPerPix;

extern unsigned int  scr_XPos;
extern unsigned int  scr_YPos;

extern char *scr_Mirror;

extern const unsigned int  fontWidth;
extern const unsigned int  fontHeight;
extern const unsigned char fontData[];

void txtClear();
void txtPutc(char chr);

void gfxClear(void);
void gfxPutc(char chr);

int serPutC(unsigned char data);

#endif /*SCREEN_H_*/
