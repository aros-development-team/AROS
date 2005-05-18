#include <exec/initializers.h>
#include <dos/dos.h>
#include <diskfont/diskfont.h>
#include <graphics/text.h>
#include <graphics/gfxbase.h>
#include <intuition/intuition.h>
#include <proto/diskfont.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ARG_TEMPLATE "FONTNAME/A,FONTSIZE/N/A,XDPI/N,YDPI/N,BOLD/S,ITALIC/S,SHOW/S"
#define ARG_NAME 0
#define ARG_SIZE 1
#define ARG_XDPI 2
#define ARG_YDPI 3
#define ARG_BOLD 4
#define ARG_ITALIC 5
#define ARG_SHOW 6
#define NUM_ARGS 7

struct Library *DiskfontBase;
struct UtilityBase *UtilityBase;
struct GfxBase *GfxBase;
struct IntuitionBase *IntuitionBase;

struct TextFont *font;
STRPTR fontname;
ULONG fontsize, xdpi, ydpi;
struct RDArgs *myargs;
ULONG args[NUM_ARGS];
char s[256];

static void cleanup(char *msg)
{
    if (msg) printf("fontinfo: %s\n", msg);
    
    if (font) CloseFont(font);
    
    if (UtilityBase) CloseLibrary((struct Library *)UtilityBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if (DiskfontBase) CloseLibrary(DiskfontBase);
    
    if (myargs) FreeArgs(myargs);
    
    exit(0);
}

static void getarguments(void)
{
    myargs = ReadArgs(ARG_TEMPLATE, args, NULL);
    if (!myargs)
    {
    	Fault(IoErr(), 0, s, 255);
	cleanup(s);
    }
    
    fontname = (STRPTR)args[ARG_NAME];
    fontsize = *(LONG *)args[ARG_SIZE];
    
    xdpi = args[ARG_XDPI] ? *(LONG *)args[ARG_XDPI] : 72;
    ydpi = args[ARG_YDPI] ? *(LONG *)args[ARG_YDPI] : 72;
    
}

static void openlibs(void)
{
    DiskfontBase = OpenLibrary("diskfont.library", 38);
    if (!DiskfontBase) cleanup("Can't open diskfont.library!");
    
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 38);
    if (!GfxBase) cleanup("Can't open graphics.library!");
 
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 38);
    if (!IntuitionBase) cleanup("Can't open intuition.library!");
    
    UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 38);
    if (!UtilityBase) cleanup("Can't open utility.library!");
}

static void openfont(void)
{
    struct TTextAttr ta;
    struct TagItem tags[] =
    {
    	{TA_DeviceDPI, (xdpi << 16) | ydpi  },
    	{TAG_DONE   	    	      	    }
    };
    
    ta.tta_Name = fontname;
    ta.tta_YSize = fontsize;
    ta.tta_Style = FSF_TAGGED;
    if (args[ARG_BOLD]) ta.tta_Style |= FSF_BOLD;
    if (args[ARG_ITALIC]) ta.tta_Style |= FSF_ITALIC;
    
    ta.tta_Flags = FPF_DESIGNED;
    ta.tta_Tags = tags;
    
    printf("fontname = \"%s\"  fontsize = %ld\n", fontname, fontsize);
    
    font = OpenDiskFont((struct TextAttr *)&ta);
    if (!font) cleanup("Could not open font!");
    
    printf("Opened font addr = %p\n", font);
    printf("Opened font has xsize = %d ysize = %d  baseline = %d  flags = 0x%x  style = 0x%x\n",
    	    font->tf_XSize,
	    font->tf_YSize,
	    font->tf_Baseline,
	    font->tf_Flags,
	    font->tf_Style);
    printf("boldsmear = %d\n", font->tf_BoldSmear);
    printf("tf_CharData = %p\n", font->tf_CharData);
        
    if (font->tf_Flags & FPF_DISKFONT)
    {
    	struct DiskFontHeader *dfh = (struct DiskFontHeader *)(((UBYTE *)font) - (UBYTE *)OFFSET(DiskFontHeader, dfh_TF));
    	
	printf("DiskFontHeader:\n");
	printf("  succ %p\n", dfh->dfh_DF.ln_Succ);
	printf("  pred %p\n", dfh->dfh_DF.ln_Pred);
	printf("  type %x\n", dfh->dfh_DF.ln_Type);
	printf("  pri  %x\n", dfh->dfh_DF.ln_Pri);
	printf("  name %p (%s)\n", dfh->dfh_DF.ln_Name, dfh->dfh_DF.ln_Name);
	printf("  FileID %x\n", dfh->dfh_FileID);
	printf("  Revision %x\n", dfh->dfh_Revision);
	printf("  Segment  %p\n", dfh->dfh_Segment);
	printf("  Name %p (%s)\n", dfh->dfh_Name, dfh->dfh_Name);
    }
    
    printf("tf_Message:\n");
    printf("  succ %p\n", font->tf_Message.mn_Node.ln_Succ);
    printf("  pred %p\n", font->tf_Message.mn_Node.ln_Pred);
    printf("  type %x\n", font->tf_Message.mn_Node.ln_Type);
    printf("  pri  %x\n", font->tf_Message.mn_Node.ln_Pri);
    printf("  name %p (%s)\n", font->tf_Message.mn_Node.ln_Name, font->tf_Message.mn_Node.ln_Name);
    printf("  replyport %p\n", font->tf_Message.mn_ReplyPort);
    printf("  length %x\n", font->tf_Message.mn_Length);
    
    printf("textfontextension:\n");
    if (ExtendFont(font, 0))
    {
    	struct TextFontExtension *tfe = ((struct TextFontExtension *)font->tf_Extension);
    	struct TagItem *extension = tfe->tfe_Tags;
	struct TagItem *tag, *tstate = extension;
	LONG dpivalue;
	
	printf("  taglist:\n");
	while((tag = NextTagItem((const struct TagItem **)&tstate)))
	{
	    printf("    {0x%08lx,0x%08lx}\n", tag->ti_Tag, tag->ti_Data);
	}
	
	dpivalue = GetTagData(TA_DeviceDPI, 0, extension);
	
	printf("  ta_devicedpi of opened font is: xdpi %ld  ydpi: %ld\n",
	    	dpivalue >> 16, dpivalue & 0xFFFF);
	
	printf("  tfe_MatchWord %x\n", tfe->tfe_MatchWord);
	printf("  tfe_Flags0 %x\n", tfe->tfe_Flags0);
	printf("  tfe_Flags1 %x\n", tfe->tfe_Flags1);
	printf("  tfe_OrigReplyPort %p\n", tfe->tfe_OrigReplyPort);
	printf("  tfe_OFontPatchS %p\n", tfe->tfe_OFontPatchS);
	printf("  tfe_OFontPatchK %p\n", tfe->tfe_OFontPatchK);
	
    }
    if (font->tf_Style & FSF_COLORFONT)
    {
    	struct ColorTextFont *ctf = (struct ColorTextFont *)font;
	WORD i;
	
	printf(" ctf_Flags %x\n", ctf->ctf_Flags);
	printf(" ctf_Depth %x\n", ctf->ctf_Depth);
	printf(" ctf_FgColor %x\n", ctf->ctf_FgColor);
	printf(" ctf_Low %x\n", ctf->ctf_Low);
	printf(" ctf_High %x\n", ctf->ctf_High);
	printf(" ctf_PlanePick %x\n", ctf->ctf_PlanePick);
	printf(" ctf_PlaneOnOff %x\n", ctf->ctf_PlaneOnOff);
	printf(" ctf_colorFontColors %p\n", ctf->ctf_ColorFontColors);

	for(i = 0; i <8; i++)
	{
	    printf(" ctf_CharData[%d] %p\n", i, ctf->ctf_CharData[i]);
	}	
    }
    
    {
    	struct TextFont *tf = (struct TextFont *)GfxBase->TextFonts.lh_Head;
	
	printf("SYSFONTLIST:\n");
	while(tf->tf_Message.mn_Node.ln_Succ)
	{
	    printf("  addr %p  name %s size %d\n", tf, tf->tf_Message.mn_Node.ln_Name, tf->tf_YSize);
	    tf = (struct TextFont *)tf->tf_Message.mn_Node.ln_Succ;
	}
    }
}

static void action(void)
{
    WORD i;
    LONG totcharsize = 0;
    LONG totcharkern = 0;
    LONG totcharspace = 0;
    LONG totsomething = 0;
    LONG numsomething = 0;
    LONG numcharspace = 0;
    
    printf("lowchar  = %d\n", font->tf_LoChar);
    printf("hichar   = %d\n", font->tf_HiChar);
    printf("isprop   = %s\n", (font->tf_Flags & FPF_PROPORTIONAL) ? "yes" : "no");
    printf("haskern  = %s\n", font->tf_CharKern ? "yes" : "no");
    printf("hasspace = %s\n", font->tf_CharSpace ? "yes" : "no");
    puts("");
    
    for(i = font->tf_LoChar; i <= font->tf_HiChar + 1; i++)
    {
    	printf("#%03d  kerning = %s%4d%s blackwidth = %4ld spacing = %s%4d%s\n",
	       i,
	       font->tf_CharKern ? "" : "[",
	       (font->tf_CharKern ? ((WORD *)font->tf_CharKern)[i - font->tf_LoChar] : 0),
	       font->tf_CharKern ? "" : "]",
	       ((LONG *)font->tf_CharLoc)[i - font->tf_LoChar] & 0xFFFF,
	       font->tf_CharSpace ? "" : "[",
	       (font->tf_CharSpace ? ((WORD *)font->tf_CharSpace)[i - font->tf_LoChar] : 0),
	       font->tf_CharSpace ? "" : "]");

    	if (font->tf_CharKern) totcharkern += ((WORD *)font->tf_CharKern)[i - font->tf_LoChar];
	if (font->tf_CharSpace)
	{
	    ULONG old_totcharspace = totcharspace;
	    totcharspace += ((WORD *)font->tf_CharSpace)[i - font->tf_LoChar];
	    if (totcharspace != old_totcharspace) numcharspace++;
	}
	
    	totcharsize += ((LONG *)font->tf_CharLoc)[i - font->tf_LoChar] & 0xFFFF;
	
	if ((i >= 110) && (font->tf_CharKern && font->tf_CharSpace))
	{
	    ULONG old_totsomething = totsomething;
	    totsomething += ((WORD *)font->tf_CharSpace)[i - font->tf_LoChar] +
	    	    	    ((WORD *)font->tf_CharKern)[i - font->tf_LoChar];
			    
	    if (totsomething != old_totsomething) 
	    numsomething++;
	    
	}
    }
    
    if (numsomething) printf("average something = %ld\n", totsomething / numsomething);
    if (numcharspace) printf("average charspace = %ld\n", totcharspace / numcharspace);
}

static void showfont(void)
{
    static char *text = "ABCDEFGHabcdefgh1234567890";
    static struct RastPort temprp;
    struct TextExtent te;
    struct Screen *scr;
    struct Window *win;
    
    InitRastPort(&temprp);
    SetFont(&temprp, font);
    
    TextExtent(&temprp, text, strlen(text), &te);
    
    if ((scr = LockPubScreen(NULL)))
    {
    	win = OpenWindowTags(NULL, WA_PubScreen, (ULONG)scr,
	    	    	    	   WA_InnerWidth, te.te_Width + 6,
				   WA_InnerHeight, te.te_Height + 6,
				   WA_CloseGadget, TRUE,
				   WA_DragBar, TRUE,
				   WA_DepthGadget, TRUE,
				   WA_Activate, TRUE,
				   WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_VANILLAKEY,
				   WA_Title, (ULONG)"FontInfo",
				   TAG_DONE);
				   
	if (win)
	{
	    SetFont(win->RPort, font);
	    SetAPen(win->RPort, 1);
	    Move(win->RPort, win->BorderLeft + 3 - te.te_Extent.MinX,
	    	    	     win->BorderTop + 3 - te.te_Extent.MinY);
	    Text(win->RPort, text, strlen(text));
	    WaitPort(win->UserPort);
	    CloseWindow(win);
	}			   
    	UnlockPubScreen(NULL, scr);
    }
}

int main(void)
{
    getarguments();
    openlibs();
    openfont();
    action();
    if (args[ARG_SHOW]) showfont();
    cleanup(0);
    
    return 0;
}
