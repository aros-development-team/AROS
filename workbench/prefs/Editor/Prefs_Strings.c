/*********************************************************
** Prefs_Strings.c: locale library support of pref tool **
** Written by T.Pierron, 24-feb-2001                    **
*********************************************************/

#include <libraries/gadtools.h>
#include <libraries/locale.h>

#include <proto/locale.h>

#define  CATCOMP_NUMBERS		/* We will need the string id */
#define  CATCOMP_STRINGS		/* and the english string corresponding to the id */
#include "strings.h"
#define	LocaleInfo				LocaleInfoTmp
#include "../../tools/Edit/strings.h"

/* Gadget messages */
STRPTR PrefMsg[] = {
	MSG_TITLEWIN_STR,
	MSG_TAB_STR,              MSG_SEPARATORS_STR,      MSG_TXTFONT_STR,       MSG_SCRFONT_STR, MSG_PUBSCREEN_STR, NULL,
	MSG_BACKDROP_STR,         MSG_LEFTMARGIN_STR,      MSG_AUTOINDENT_STR,    MSG_EXTEND_STR,  NULL,
	MSG_PREFSSAVE_STR,        MSG_USEPREFS_STR,        MSG_DISCARDPREFS_STR,  NULL,
	NULL,                     MSG_USEDEF_STR,          MSG_CHOOSEIT_STR,      NULL,
	NULL,                     MSG_CLONEPARENT_STR,     NULL,                  NULL, NULL,
	MSG_COLOR_BACK_STR,       MSG_COLOR_TEXT_STR,      MSG_COLOR_FILLTXT_STR, MSG_COLOR_FILLSEL_STR,
	MSG_COLOR_MARGINBACK_STR, MSG_COLOR_MARGINTXT_STR, MSG_COLOR_SHINE_STR,   MSG_COLOR_SHADE_STR,
	MSG_COLOR_PANELBACK_STR,  MSG_COLOR_PANELTEXT_STR, MSG_COLOR_GLYPH_STR,   MSG_COLOR_MARKEDLINES_STR, NULL
};

/* Global error messages (taken from Jano) */
STRPTR Errors[] = {
	ERR_BADOS_STR,				ERR_NOASLREQ_STR,
	/*ERR_NOWRTACCESS_STR,*/		ERR_NOMEM_STR,
	ERR_NOGUI_STR,				ERR_NONAME_STR,
	ERR_WRITECLIP_STR,		ERR_OPENCLIP_STR,
	ERR_LOADFILE_STR,			ERR_NOTXTINCLIP_STR,
	ERR_WRONG_TYPE_STR,		ERR_READCLIP_STR,
	ERR_NOBRACKET_STR,		ERR_NOT_FOUND_STR,
	ERR_LOADFONT_STR,			ERR_NOPREFEDITOR_STR,
	ERR_BADPREFSFILE_STR,	ERR_FILEMODIFIED_STR,
	ERR_SLC_STR,				ERR_NOSEARCHTEXT_STR
};


struct NewMenu newmenu[] =
{
	/* Part of strings are shared with Jano */
	{NM_TITLE, MSG_PROJECTTITLE_STR, NULL, 0, 0L, NULL},
	{	NM_ITEM, MSG_OPENNEWFILE_STR,	 	"O", 0, 0L, (APTR)101},
	{	NM_ITEM, MSG_SAVEFILEAS_STR,		"A", 0, 0L, (APTR)102},
	{	NM_ITEM, (STRPTR)NM_BARLABEL,		NULL,0, 0L, NULL},
	{	NM_ITEM, MSG_QUIT_STR,				"Q", 0, 0L, (APTR)103},

	{NM_TITLE, MSG_EDITTITLE_STR,			NULL, 0, 0L, NULL},
	{	NM_ITEM, MSG_RESETDEFAULT_STR,	"D", 0, 0L, (APTR)201},
	{	NM_ITEM, MSG_LASTSAVED_STR,		"L", 0, 0L, (APTR)202},

	{NM_END, 0, 0, 0, 0, 0}
};

/* String ID to quote from catalogs for newmenus (IDs aren't contiguous) */
WORD StrID[] = {
	MSG_PROJECTTITLE, MSG_OPENNEWFILE, MSG_SAVEFILEAS,
	MSG_QUIT,         MSG_EDITTITLE,   MSG_RESETDEFAULT,
	MSG_LASTSAVED
};

#define	EOT(table)		(table+sizeof(table)/sizeof(table[0]))

static void *prefs_cat = NULL;
static void *jano_cat  = NULL;

/*** Localise all strings of the program ***/
void prefs_local(void)
{
	WORD MsgID;
	/* Custom prefs messages */
	if( (prefs_cat = (void *) OpenCatalogA(NULL,"System/Prefs/EditorPrefs.catalog",NULL)) &&
	    (jano_cat = (void *) OpenCatalogA(NULL,"System/Tools/Editor.catalog",NULL)) )
	{
		{	/* Various message of pref */
			STRPTR *str;
			for(str = PrefMsg, MsgID=MSG_TITLEWIN; str < EOT(PrefMsg); str++)
				if(*str != NULL)
					*str = (STRPTR) GetCatalogStr(prefs_cat,MsgID++,*str);
		}
		{	/* NewMenus */
			struct NewMenu *nm;
			for(nm=newmenu,MsgID=0; nm->nm_Type != NM_END; nm++)
				if(nm->nm_Label != NM_BARLABEL)
					nm->nm_Label  = (STRPTR)
						GetCatalogStr(MsgID<5 ? jano_cat : prefs_cat,StrID[MsgID],nm->nm_Label), MsgID++;
		}
		/* Common messages with Jano */
		{
			STRPTR *str;
			for(str = Errors, MsgID=ERR_BADOS; str < EOT(Errors); str++)
				*str = (STRPTR) GetCatalogStr(jano_cat,MsgID++,*str);
		}
	}
}

/*** Free allocated ressource ***/
void free_locale(void)
{
	if(prefs_cat) CloseCatalog(prefs_cat);
	if(jano_cat)  CloseCatalog(jano_cat);
}
