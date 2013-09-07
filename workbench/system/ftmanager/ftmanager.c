/*
 * Based on source from ftmanager from MorphOS for their ft2.library
 */
#define NO_INLINE_STDARG

#include <libraries/mui.h>
#include <libraries/asl.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#undef NO_INLINE_STDARG
#include <proto/alib.h>
#include <proto/codesets.h>
#include <proto/freetype2.h>
#include <aros/debug.h>

#include "fontbitmap_class.h"
#include "fontinfo_class.h"
#include "fontlist_class.h"
#include "fontwindow_class.h"
#include "globals.h"
#include "locale.h"

#define ARG_TEMPLATE "TTFFONT/A,OUTFONT/A,CODEPAGE"

#define VERSION "$VER: FTManager 1.3 (07.09.2013) ©2011-2013 The AROS Development Team"

enum
{
	ARG_TTFFONT,
	ARG_OUTFONT,
	ARG_CODEPAGE,
	ARG_COUNT
};

/***********************************************************************/

/* Global variables */
BPTR destdir;
UWORD codepage[256];
struct Library *CodesetsBase;

/***********************************************************************/

static struct RDArgs *rda;
static Object *app;
static STRPTR *codesetentries;
static STRPTR *codesetsupported;

/***********************************************************************/

static void Cleanup(void)
{
	if (rda)
		FreeArgs(rda);

	CleanupFontListClass();
	CleanupFontWindowClass();
	CleanupFontInfoClass();
	CleanupFontBitmapClass();

	FT_Done_Library(ftlibrary);

	if (codesetsupported)
		CodesetsFreeA(codesetsupported, NULL);
	FreeVec(codesetentries);
	CloseLibrary(CodesetsBase);

	UnLock(destdir);
}

static int Init(BOOL gui)
{
	FT_Error error;

	CodesetsBase = OpenLibrary("codesets.library", 0);
	if (!CodesetsBase)
	    return 0;

	error = FT_Init_FreeType(&ftlibrary);
	if (error != 0)
	{
		DEBUG_MAIN(dprintf("Init_FreeType error %d\n", error));
		return 0;
	}

	if (gui)
	{
		if (!InitFontBitmapClass() ||
				!InitFontInfoClass() ||
				!InitFontWindowClass() ||
				!InitFontListClass())
		{
			DEBUG_MAIN(dprintf("Can't create custom classes.\n"));
			return 0;
		}
	}

	destdir = Lock("Fonts:", ACCESS_READ);

	return 1;
}


static void SetDefaultCodePage(void)
{
	int k;
	for (k = 0; k < 256; ++k)
		codepage[k] = k;
}

BOOL IsDefaultCodePage(void)
{
	int k;
	for (k = 0; k < 256; ++k)
		if (codepage[k] != k)
			return FALSE;
	return TRUE;
}

static BOOL LoadCodePage(int entryindex, char *codepg)
{
	struct codeset *cs = NULL;

	SetDefaultCodePage();

	if (entryindex == 0 && codepg == NULL) // keep default code page
	{
		return TRUE;
	}

	if (codepg)
	{
		cs = CodesetsFind(codepg, CSA_FallbackToDefault, FALSE, TAG_DONE);
	}
	else if (entryindex > 0)
	{
		cs = CodesetsFind(codesetentries[entryindex],
			CSA_FallbackToDefault, FALSE, TAG_DONE);
	}

	if (cs)
	{
		LONG index;
		for (index = 0 ; index < 256 ; index++)
		{
			codepage[index] = (UWORD)cs->table[index].ucs4;
		}
		return TRUE;
	}

	return FALSE;
}


#define ID_SetSrc	1
#define ID_SetDestDir	2
#define ID_ShowFont	3
#define ID_SetCodePage	4

static int ftmanager_gui(void)
{
	int ret = RETURN_FAIL;
	Object *win, *src, *dest, *fontlist, *fontlv, *codepagecycle, *quit;
	int countfrom, countto;

	if (!Init(TRUE))
		return RETURN_FAIL;

	SetDefaultCodePage();

	codesetsupported = CodesetsSupportedA(NULL); // Available codesets
	countfrom = 0;
	while (codesetsupported[countfrom])
	{
		countfrom++;
	}
	codesetentries = AllocVec((sizeof (STRPTR)) * (countfrom + 2), MEMF_CLEAR);
	if (!codesetentries)
	{
	    Cleanup();
	    return RETURN_FAIL;
	}
	codesetentries[0] = "----";
	countfrom = 0;
	countto = 1;
	while (codesetsupported[countfrom])
	{
		if (strncmp(codesetsupported[countfrom], "UTF", 3))
		{
			codesetentries[countto] = codesetsupported[countfrom];
			countto++;
		}
		countfrom++;
	}
	app = ApplicationObject,
		MUIA_Application_Title, __(MSG_APP_NAME),
		MUIA_Application_Version,(IPTR) VERSION,
		MUIA_Application_Copyright, "Copyright 2002-2003 by Emmanuel Lesueur",
		MUIA_Application_Author, "Emmanuel Lesueur",
		MUIA_Application_Description, __(MSG_APP_TITLE),
		MUIA_Application_Base, "FTMANAGER",
		SubWindow, win = WindowObject,
			MUIA_Window_ID, MAKE_ID('F','T','2','M'),
			MUIA_Window_Title, __(MSG_WINDOW_TITLE),
			MUIA_Window_Width, 400,
			MUIA_Window_RootObject,VGroup,
				Child, fontlv = ListviewObject,
					MUIA_Listview_List, fontlist = FontListObject,
						End,
					End,
				Child, ColGroup(2),
					Child, Label2(_(MSG_LABEL_SOURCE)),
					Child, PopaslObject,
						MUIA_Popasl_Type, ASL_FileRequest,
						MUIA_Popstring_String, src = StringObject,
							StringFrame,
							MUIA_String_Contents, __(MSG_ASL_FONTS_TRUETYPE),
                                                        MUIA_String_AdvanceOnCR, TRUE,
							MUIA_CycleChain, TRUE,
							End,
						MUIA_Popstring_Button, PopButton(MUII_PopDrawer),
						ASLFR_RejectIcons, TRUE,
						ASLFR_DrawersOnly, TRUE,
						End,
					Child, Label2(_(MSG_LABEL_DESTINATION)),
					Child, PopaslObject,
						MUIA_Popasl_Type, ASL_FileRequest,
						MUIA_Popstring_String, dest = StringObject,
							StringFrame,
							MUIA_String_Contents, __(MSG_ASL_FONTS),
							MUIA_String_AdvanceOnCR, TRUE,
							MUIA_CycleChain, TRUE,
							End,
						MUIA_Popstring_Button, PopButton(MUII_PopDrawer),
						ASLFR_DoSaveMode, TRUE,
						ASLFR_DrawersOnly, TRUE,
						ASLFR_RejectIcons, TRUE,
						End,
					Child, Label2(_(MSG_LABEL_CODEPAGE)),
					Child, codepagecycle = CycleObject,
						MUIA_Cycle_Entries, codesetentries,
						End,
					End,
				Child, quit = SimpleButton(_(MSG_QUIT)),
				End,
			End,
		End;

	if (app)
	{
		IPTR t;

		ret = RETURN_OK;

		DoMethod(src, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
				fontlist, 2, MUIM_FontList_AddDir, MUIV_TriggerValue);

		DoMethod(dest, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
				app, 2, MUIM_Application_ReturnID, ID_SetDestDir);

		DoMethod(codepagecycle, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
				app, 2, MUIM_Application_ReturnID, ID_SetCodePage);

		DoMethod(fontlv, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
				app, 2, MUIM_Application_ReturnID, ID_ShowFont);

		DoMethod(win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
				app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

		DoMethod(quit, MUIM_Notify, MUIA_Pressed, FALSE,
				app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

                DoMethod(fontlist, MUIM_FontList_AddDir, XGET(src, MUIA_String_Contents));

		set(win, MUIA_Window_Open, TRUE);
		t = 0;
		get(win, MUIA_Window_Open, &t);
		if (t)
		{
			BOOL running = TRUE;
			ULONG sigs = 0;
			ULONG id;

			do
			{
				id = DoMethod(app, MUIM_Application_NewInput, &sigs);
				switch (id)
				{
					case MUIV_Application_ReturnID_Quit:
						running = FALSE;
						sigs = 0;
						break;

					case ID_SetDestDir:
						{
							CONST_STRPTR name = NULL;
							BPTR newdir;

							get(dest, MUIA_String_Contents, &name);
							if (!name)
							    break;

							newdir = Lock(name, ACCESS_READ);
							if (newdir)
							{
								UnLock(destdir);
								destdir = newdir;
							}
						}
						break;

					case ID_ShowFont:
						{
							struct MUIS_FontList_Entry *entry;
							Object *w;

							DoMethod(fontlist, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &entry);

							w = FontWindowObject,
							    MUIA_FontWindow_Filename, entry->FileName,
							    MUIA_UserData, app,
								End;

							if (w)
							{
								DoMethod(w, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
										app, 6, MUIM_Application_PushMethod, app, 3,
										MUIM_CallHook, &CloseWinHook, w);

								DoMethod(app, OM_ADDMEMBER, w);
								set(w, MUIA_Window_Open, TRUE);
								get(w, MUIA_Window_Open, &t);
								if (!t)
								{
									MUI_DisposeObject(w);
								}
							}

						}
						break;

					case ID_SetCodePage:
						{
							IPTR entry = 0;
							get(codepagecycle, MUIA_Cycle_Active, &entry);
							LoadCodePage(entry, NULL);
						}
						break;
				}

				if (sigs)
				{
					sigs = Wait(sigs | SIGBREAKF_CTRL_C);
					if (sigs & SIGBREAKF_CTRL_C)
					{
						running = FALSE;
					}
				}
			}
			while (running);
		}
		else
		{
			printf("Can't open window.\n");
		}

		MUI_DisposeObject(app);
	}
	else
	{
		printf("Can't create MUI application.\n");
	}

	Cleanup();

	return ret;
}

static int ftmanager_cli(void)
{
	// TODO implement me
	return RETURN_FAIL;
}

int main(int argc, char **argv)
{
	int retval = RETURN_FAIL;

        Locale_Initialize();

	if (argc > 1)
	{
		retval = ftmanager_cli();
	}
	else
	{
		// Starting from CLI without arguments opens GUI, too
		retval = ftmanager_gui();
	}

        Locale_Deinitialize();

	return retval;
}
