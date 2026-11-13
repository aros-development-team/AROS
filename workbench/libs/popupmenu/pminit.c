//
// PopupMenu
// ©1996-2000 Henrik Isaksson
//
// Library init & cleanup
//

#define INIT 1

#include "pmpriv.h"


//
// Libraries we need
//

struct Library		*UtilityBase=NULL;
struct IntuitionBase	*IntuitionBase=NULL;
struct GfxBase		*GfxBase=NULL;
struct DosLibrary	*DOSBase=NULL;
struct Library		*CxBase=NULL;
struct Library		*LayersBase=NULL;
struct Library		*CyberGfxBase=NULL;

extern struct ExecBase	*SysBase;

APTR			MemPool = NULL;

BOOL V40Gfx=FALSE;
BOOL CyberGfx=FALSE;

void CloseLibs(void)
{
	if(MemPool)		DeletePool(MemPool);
	if(UtilityBase)		CloseLibrary((struct Library *)UtilityBase);
	if(GfxBase)		CloseLibrary((struct Library *)GfxBase);
	if(IntuitionBase)	CloseLibrary((struct Library *)IntuitionBase);
	if(DOSBase)		CloseLibrary((struct Library *)DOSBase);
	if(CxBase)		CloseLibrary((struct Library *)CxBase);
	if(LayersBase)		CloseLibrary((struct Library *)LayersBase);
	if(CyberGfxBase)	CloseLibrary((struct Library *)CyberGfxBase);

	MemPool = NULL;
	UtilityBase=NULL;
	GfxBase=NULL;
	IntuitionBase=NULL;
	DOSBase=NULL;
	CxBase=NULL;
	LayersBase=NULL;
	CyberGfxBase=NULL;
}

BOOL __asm __saveds OpenLibs(register __a6 struct PopupMenuBase *l)
{
	if(UtilityBase) return TRUE;

	l->pmb_UtilityBase=OpenLibrary("utility.library",37L);
	if(l->pmb_UtilityBase) {
		UtilityBase=l->pmb_UtilityBase;
		l->pmb_GfxBase=OpenLibrary("graphics.library",40L);
		if(!l->pmb_GfxBase) l->pmb_GfxBase=OpenLibrary("graphics.library",37L);
		else V40Gfx=TRUE;
		if(l->pmb_GfxBase) {
			GfxBase=(struct GfxBase *)l->pmb_GfxBase;
			l->pmb_IntuitionBase=OpenLibrary("intuition.library",37L);
			if(l->pmb_IntuitionBase) {
				IntuitionBase=(struct IntuitionBase *)l->pmb_IntuitionBase;
				l->pmb_DOSBase=OpenLibrary("dos.library",0L);
				if(l->pmb_DOSBase) {
					DOSBase=(struct DosLibrary *)l->pmb_DOSBase;
					l->pmb_ExecBase = (struct Library *)SysBase;
					l->pmb_CxBase=OpenLibrary("commodities.library",37L);
					if(l->pmb_CxBase) {
						CxBase=l->pmb_CxBase;
						LayersBase=OpenLibrary("layers.library",0);
						l->pmb_LayersBase=LayersBase;
						if(LayersBase) {
							CyberGfxBase=OpenLibrary("cybergraphics.library",39L);
							if(CyberGfxBase)
								CyberGfx=TRUE;
							l->pmb_CyberGfxBase=CyberGfxBase;
						#if defined(__AROS__) || defined(__MORPHOS)
							if((MemPool = CreatePool(MEMF_ANY | MEMF_CLEAR | MEMF_SEM_PROTECTED, 10240L, 10240L))) {
						#else
							if((MemPool = CreatePool(MEMF_ANY, 10240L, 10240L))) {
						#endif

								PM_Prefs_Load(PMP_PATH);

								return TRUE;
							}
						}
					}
				}
			}
		}
	}

	return FALSE;
}

//
// Library initializtion
//

int __asm __saveds __UserLibInit(register __a6 struct PopupMenuBase *l)
{
	//kprintf("UserLibInit, pmbase = %08lx\n", l);

#ifdef __AROS__
    	SysBase = (struct ExecBase *)l->pmb_ExecBase;
#else
	SysBase=*((struct ExecBase **)4);
#endif
	if(!OpenLibs(l)) return -1;

	//kprintf("UserLibInit failed\n");

	return 0;
}

void __asm __saveds __UserLibCleanup(register __a6 struct PopupMenuBase *l)
{

	//kprintf("UserLibCleanUp, pmbase = %08lx\n", l);

	//PM_FreeAllImages();

	PM_Prefs_Free();

	//CloseLibs();

	if(MemPool)		DeletePool(MemPool);
	if(UtilityBase)		CloseLibrary((struct Library *)UtilityBase);
	if(GfxBase)		CloseLibrary((struct Library *)GfxBase);
	if(IntuitionBase)	CloseLibrary((struct Library *)IntuitionBase);
	if(DOSBase)		CloseLibrary((struct Library *)DOSBase);
	if(CxBase)		CloseLibrary((struct Library *)CxBase);
	if(LayersBase)		CloseLibrary((struct Library *)LayersBase);
	if(CyberGfxBase)	CloseLibrary((struct Library *)CyberGfxBase);

	//kprintf("UserLibCleanUp done.\n");
}

#ifdef __AROS__
#include <aros/symbolsets.h>
#include <aros/debug.h>

static int AROS__UserLibInit(struct PopupMenuBase *PopupMenuBase)
{
    return (__UserLibInit(PopupMenuBase) == 0) ? TRUE : FALSE;
}

static int AROS__UserLibCleanup(struct PopupMenuBase *PopupMenuBase)
{
    __UserLibCleanup(PopupMenuBase);
    
    return TRUE;
}

ADD2INITLIB(AROS__UserLibInit, 0);
ADD2EXPUNGELIB(AROS__UserLibCleanup, 0);
#endif

