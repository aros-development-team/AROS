/**************************************************************
**** Macros.h : Datatypes for recording keystroke sequence ****
**** Free software under GNU license, started on 15/8/2001 ****
**** © T.Pierron, C.Guillaume.                             ****
**************************************************************/

#ifndef MACROS_H
#define MACROS_H

/* Size of one chunk for record */
#define	SZ_MACRO			256
#define	MAX_MAC			10

/** Structure holding keystrokes **/
typedef struct _Macro
{
	struct _Macro *next;
	UWORD          usage;					/* Nb. of bytes in the array data */
	UBYTE          data[ SZ_MACRO ];		/* Buffer for keystroke record */
}	*Macro;

typedef struct					/* Character added to the buffer */
{
	UBYTE Type, Char;			/* MAC_ACT_ADD_CHAR, character added */
}	*ActChar;

typedef struct					/* Menu command */
{
	UBYTE Type;					/* MAC_ACT_COM_MENU */
	UWORD Qual, Code;			/* Menu code and qualifier */
}	*ActMenu;

typedef ActMenu				ActShortcut;

/** Possible values for `Type' field **/
#define	MAC_ACT_ADD_CHAR				0
#define	MAC_ACT_COM_MENU				1
#define	MAC_ACT_SHORTCUT				2

/** Public functions associated **/
void free_macros     ( void );
void init_macros     ( void );
void start_macro     ( void );
void stop_macro      ( void );
void play_macro      ( int  );
void reg_act_addchar ( UBYTE code );
void reg_act_com     ( UBYTE type, UWORD code, UWORD qual );
void new_slot        ( BYTE dir );
void repeat_macro( Project p );

#endif
