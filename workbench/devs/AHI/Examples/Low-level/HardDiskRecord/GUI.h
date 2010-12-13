/*********************************************/
/*                                           */
/*       Designer (C) Ian OConnor 1994       */
/*                                           */
/*      Designer Produced C header file      */
/*                                           */
/*********************************************/



#define Win0FirstID 0
#define Win0_srclist 0
#define Win0_dstlist 1
#define Win0_volslider 2
#define Win0_ambutton 3
#define Win0_amtext1 4
#define Win0_amtext2 5
#define Win0_gainslider 6
#define Win0_duration 7
#define Win0_length 8
#define Win0_filename 9
#define Win0_fnbutton 10
#define Win0_create 11
#define Win0_begin 12
#define Win0_secleft 13
#define Win0_format 14
#define  SelectAudioID   0
#define  FileNotPrepared   1
#define  OKtext   2
#define  NoHardware   3
#define  UnknownErr   4
#define  Finished   5
#define  Win0Texts1   6
#define  Win0_formatString0   7
#define  Win0_formatString1   8
#define  Win0_volsliderLevelFormat   9
#define  Win0_gainsliderLevelFormat   10
#define  Win0_lengthStringFormat   11
#define  Win0_srclistString   12
#define  Win0_dstlistString   13
#define  Win0_volsliderString   14
#define  Win0_ambuttonString   15
#define  Win0_amtext1String   16
#define  Win0_amtext2String   17
#define  Win0_gainsliderString   18
#define  Win0_durationString   19
#define  Win0_lengthString   20
#define  Win0_filenameString   21
#define  Win0_fnbuttonString   22
#define  Win0_createString   23
#define  Win0_beginString   24
#define  Win0_secleftString   25
#define  Win0_formatString   26
#define  Win0Title   27
#define  Win0ScreenTitle   28

extern struct Gadget *Win0Gadgets[15];
extern struct Gadget *Win0GList;
extern struct Window *Win0;
extern APTR Win0VisualInfo;
extern APTR Win0DrawInfo;
extern ULONG Win0GadgetTags[];
extern UWORD Win0GadgetTypes[];
extern struct NewGadget Win0NewGadgets[];
extern struct Library *AslBase;
extern struct Library *DiskfontBase;
extern struct Library *GadToolsBase;
extern struct GfxBase *GfxBase;
extern struct IntuitionBase *IntuitionBase;
extern struct Library *UtilityBase;
extern struct LocaleBase *LocaleBase;
extern struct Catalog *HardDiskRecord_Catalog;

extern void RendWindowWin0( struct Window *Win, void *vi );
extern int OpenWindowWin0( STRPTR ScrName);
extern void CloseWindowWin0( void );
extern int OpenLibs( void );
extern void CloseLibs( void );
extern STRPTR GetString(LONG strnum);
extern void CloseHardDiskRecordCatalog(void);
extern void OpenHardDiskRecordCatalog(struct Locale *loc, STRPTR language);

