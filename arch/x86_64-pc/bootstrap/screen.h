#ifndef SCREEN_H_
#define SCREEN_H_

enum scr_type
{
    SCR_UNKNOWN,
    SCR_TEXT,
    SCR_GFX
};

struct multiboot;

extern void         *fb;
extern unsigned int  wc;
extern unsigned int  hc;
extern unsigned int  pitch;
extern unsigned int  bpp;

extern unsigned int x;
extern unsigned int y;

extern unsigned char use_serial;

extern const unsigned int  fontWidth;
extern const unsigned int  fontHeight;
extern const unsigned char fontData[];

void initScreen(struct multiboot *mb);
void clr(void);
void Putc(char);

void txtClear();
void txtPutc(char chr);

void gfxClear(void);
void gfxPutc(char chr);

void initSerial(char *opts);
int serPutC(unsigned char data);

void scr_RawPutChars(char *, int);
void kprintf(const char *, ...);

#endif /*SCREEN_H_*/
