/***************************************************************
**** Utils.h: Prototypes of useful functions                ****
**** Free software under GNU license, started on 11/11/2000 ****
**** © T.Pierron, C.Guillaume.                              ****
***************************************************************/

#ifndef	PREFS_UTILH_H
#define	PREFS_UTILH_H

/*** Convert number to dec (left-aligned) ***/
WORD AddNum(ULONG nb, STRPTR buf);

/*** Measure the maximal lenght of a NULL-terminated array of string ***/
WORD meas_table(UBYTE **strings);

/*** Extract some information of a TextFont struct ***/
void font_info(UBYTE *buf, struct TextFont *);

/*** Same job with a (struct Screen *) ***/
void scr_info(UBYTE *buf, WORD Width, WORD Height, WORD Depth);

/*** Try to load an already loaded font ***/
struct TextFont *get_old_font( STRPTR /* name/size */ );

/*** Be sure a window fits in a screen ***/
void fit_in_screen(struct NewWindow *, struct Screen *);

/*** Performs some checks on what user has enterred ***/
void check_tab(struct Gadget *);

/*** Set default preference ***/
void default_prefs();

/*** Save current configuration to restore it later if desired ***/
void save_config( char ConfigFile );

/*** Restore config ***/
void restore_config( PREFS * );

void load_pref(PREFS *prefs);
void save_pref_as(PREFS *prefs);
void default_prefs(PREFS *prefs);

void free_asl(void);

#ifdef	ErrMsg
#undef ErrMsg
#endif

#define ErrMsg(num)		Errors[ num-ERR_BADOS ]
extern  UBYTE *Errors[];

#endif
