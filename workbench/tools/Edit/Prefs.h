/**************************************************************
**** Prefs.h: internal representation of preference file.  ****
**** Free software under GNU license, started on 16.2.2000 ****
**** © T.Pierron, C.Guillaume.                             ****
**************************************************************/

#ifndef PREFS_H
#define PREFS_H

#ifndef	GUI_H
#include "Gui.h"
#endif

#include <graphics/text.h>
#include "Version.h"

/*** Global preferences ***/
typedef struct prefs
{
	char   use_pub;            /* TRUE if a pubscreen is created */
	char   use_txtfont;        /* TRUE if a custom text font is used */
	char   use_scrfont;        /* TRUE if a custom screen font is used */
	char   backdrop;           /* TRUE if use a backdrop'ed window */
	char   left_margin;        /* Left margin for easy line selection */
	char   auto_indent;        /* TRUE if auto indent mode */
	char   xtend;              /* TRUE if use PC-like numeric keypad */
	char   reserved;           /* Unused for now */
	char   matchcase;          /* TRUE if search is case insensitive */
	char   wholewords;         /* TRUE if only whole words match in a search */
	char   tabsize;            /* Tabulation size */
	char   depth;              /* Depth of custom screen */
	STRPTR wordssep;           /* List of characters used to separate words */
	struct TextAttr attrtxt;   /* Font used to edit text */
	struct TextAttr attrscr;   /* Font used for graphical interface */
	WORD   left,top;           /* Left corner of the window */
	WORD   width,height;       /* Dimensions */

	/* Information about duplicated screen */
	WORD   scrw,scrh,scrd;     /* Properties of cloned screen */
	ULONG  modeid;             /* Mode id of new screen */
	ULONG  vmd;                /* Display ID of pubscreen */
#ifdef	GUI_H                /* Not always required */
	struct pens pen;           /* Pens offset used */

	/* Information allocated at run-time */
	struct TextFont *txtfont;  /* Font usable with rastport */
	struct TextFont *scrfont;  /* Font used to render gui */
	struct Screen   *parent;   /* Parent screen */
	struct Screen   *current;  /* Screen where window remains */
#endif
}	PREFS;

/** Maximal fields saved in preference file **/
#define	MAX_NUMFIELD   sizeof(sizefields)

/** Command search to edit preference of editor **/
#define	SYS_DIR        "SYS:"
#define	PREF_DIR       "Prefs/"
#define	PREF_NAME      "JanoPrefs"

/** Character types to split words **/
#define	ALPHA          0
#define	SEPARATOR      1
#define	SPACE          2
#define	MAX_SPLIT      40    /* Maximal string separator length */

/** Table where character type can be found **/
extern UBYTE TypeChar[256];

#define	ID_JANO        MAKE_ID('J','A','N','O')
#define	ID_PREF        MAKE_ID('P','R','E','F')

/** Open preference file according to mode **/
APTR open_prefs(STRPTR name, UBYTE mode);

/** Values for `mode' **/
#define	MODE_USE       1     /* Open file for reading (name can be NULL) */
#define	MODE_SAVE      2     /* Open file for writing with IFF header */


/** Load/save preference file **/
UBYTE load_prefs(PREFS *, STRPTR file);
UBYTE save_prefs(PREFS *);

/** Search for janoprefs and launch it **/
void  setup_winpref(void);

/** Ask user for a new screen mode/font **/
struct TextFont *change_fonts(struct TextAttr *buf, void *Wnd, BOOL fixed);

extern PREFS prefs;

/** Inhibit unwanted functions **/
#ifdef	JANOPREF
#define	init_tabstop(X)

/** Ask user for a new screen mode **/
ULONG change_screen_mode(WORD *,ULONG);
#else

/** Check changes between two prefs and make them effective **/
void update_prefs( Project );

/** Ask user for a new preference file with ASL **/
void ask_prefs( Project, char save, CONST_STRPTR);

/** Ask user for a new screen mode **/
ULONG change_screen_mode(UBYTE *,ULONG);

/** Change screenmode (shortcut of gui) **/
void ask_new_screen( void );

/** Ask user for a new font (shortcut of gui) **/
void ask_new_font( void );


#endif /* JANOPREF */

/** Convert a TextFont struct into a TextAttr **/
void text_to_attr(struct TextFont *, struct TextAttr *);

/** Try to load a preference file **/
UBYTE load_prefs( PREFS *, STRPTR );

/** Set preference to default settings **/
void set_default_prefs( PREFS *, struct Screen * );

#endif /* PREFS_H */

