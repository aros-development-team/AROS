struct KeyButtonInfo
{
    /* You must initialize these! */
    struct Window 	*win;
    struct Gadget 	*glist;
    struct Gadget 	*lastgad;
    
    /* read this if you want to know if a button is already pressed down */
    UWORD 		lastcode;
};


#define BUTTON_MAGIC_LONGWORD			(('O' << 24L) + ('o' << 16L) + ('p' << 8L) + 's')

struct InitData
{
    struct Gadget 	*idata_Gadget;		/* NULL for sole images */
    struct TextAttr 	*idata_TextAttr;
    const char 		*idata_Label;
    APTR 		idata_VisualInfo;
    ULONG 		idata_Underscore;
};

/* AROS: taken from boopsigads.i */

struct LocalObjData
{
    struct InitData	lod_IData;
    UWORD		lod_UnderOff;
    UWORD		lod_UnderWidth;
    UWORD		lod_UnderY;
    UWORD		lod_RestLen;
};

#define KEYB_SHORTCUT			1
#define FILESTR_CHANGED			2

#define RAWKEY_UP			0x4C
#define RAWKEY_DOWN			0x4D

struct StrGadUserData
{
    ULONG flags;
    struct Process *proc;
    struct MsgPort *msgport;
    struct IntuiMessage fakeimsg;
};

#define USERFLAG_UP_DOWN_ARROW		0x1
#define USERFLAG_MATCH_FILE		0x2

extern int REGARGS CatStrLen (char *);
extern char REGARGS KeyFromStr (char *, char);
extern struct Gadget * REGARGS my_CreateButtonGadget(struct Gadget *, ULONG, struct NewGadget *);
extern struct Gadget * REGARGS my_CreateIntegerGadget(struct Gadget *, struct NewGadget *, int, LONG, ULONG);
extern struct Gadget * REGARGS my_CreateStringGadget (struct Gadget *, struct NewGadget *, int, char *);
extern void REGARGS my_SetStringGadget (struct Window *, struct Gadget *, char *);
extern void REGARGS my_SetIntegerGadget (struct Window *, struct Gadget *, long);
extern void REGARGS my_FreeGadgets (struct Gadget *);
extern void REGARGS my_SelectGadget (struct Gadget *, struct Window *);
extern struct Gadget *REGARGS my_GetKeyGadget (UBYTE, struct Gadget *);
/* convert rawkey to ascii and check if gadget down/up. Returns gadgetid if up. */
extern ULONG REGARGS CheckGadgetKey (int code, int qual,
					char *, struct KeyButtonInfo *);
/* press gadget down, if key (code) comes up gadget will pop up! */
extern void REGARGS my_DownGadget (struct Gadget *, UWORD, struct KeyButtonInfo *);

struct Image * REGARGS my_CreateGadgetLabelImage (struct Image *, struct NewGadget *, char *, WORD, WORD, UWORD);
void REGARGS my_FreeLabelImages (struct Image *);

