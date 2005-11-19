
#include <libraries/iffparse.h>

/* Header file for 2View.c */

enum ScreenWidths {low,high,superhigh,unknown_w};
typedef enum ScreenWidths ScreenWidths;

enum ScreenHeights {nolace,lace,prodlace,unknown_h};
typedef enum ScreenHeights ScreenHeights;

enum ScreenTypes {ham,ehb,unknown_t};
typedef enum ScreenTypes ScreenTypes;

enum ButtonTypes {none=0,select,menu};
typedef enum ButtonTypes ButtonTypes;

typedef struct CAMG
{
   ULONG viewmodes;
} CAMG;

typedef struct CRNG
{
   WORD pad1;
   WORD rate;
   WORD active;
   UBYTE low,high;
} CRNG;

typedef struct DRNG
{
   UBYTE min;
   UBYTE max;
   WORD rate;
   WORD flags;
   UBYTE ntrue;
   UBYTE nregs;
} DRNG;

typedef struct DIndex
{
   UBYTE cell;
   UBYTE index;
} DIndex;

#define NORMAL_MODE 0
#define SHAM 1
#define MACROPAINT 2


#define MIN(x,y) ((x)<(y)) ? (x) : (y)



/* Prototypes for functions defined in 2View.c */
int main (int argc, char ** argv);
void ReadAndDisplay(char *filename,struct IFFHandle *iff);
void setScreenColors(struct Screen *scr, UBYTE *colorMap, UBYTE depth,
		     UWORD *destColorMap,UBYTE *colors);
void ReadBodyIntoBitmap(struct BitMap *bm,
			UBYTE *buffer,
			ULONG bufferSize);
void GetALine(BYTE *src,
			UBYTE *dest,
			ULONG *pos,
			UWORD width,
			unsigned char Compression);

void getBMHD(UBYTE *bmhd);
void ParseArgs(IPTR *args);
ButtonTypes checkButton(void);
void printError(char *fmt,...);
void cleanup();
BOOL dumpRastPort(struct RastPort *rp,struct ViewPort *vp);
void cycleColors(UBYTE *cycleTable,UWORD *colorTable,UBYTE length,UBYTE numColors);
UBYTE interpretCRNG(UBYTE *cycleTable,CRNG *crng,UBYTE *rate);
UBYTE interpretDRNG(UBYTE *cycleTable,DRNG *drng,UBYTE *rate);
void toggleCycling(void);

#if 0
void setupSHAM(struct Screen *scr,UWORD *sham);
void setupDynHires(struct Screen *scr,UWORD *colorBuf);
#endif

