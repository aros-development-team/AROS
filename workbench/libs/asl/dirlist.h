/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

#ifndef AROS_ASMCALL_H
#   include <aros/asmcall.h>
#endif
#ifndef DOS_EXALL_H
#   include <dos/exall.h>
#endif
#ifndef DOS_DOSEXTENS_H
#   include <dos/dosextens.h>
#endif

#define DISPHOOKBUFSIZE 256

struct VolumeInfo
{
    STRPTR	vi_Name;
    LONG	vi_Type;
};

/* This data is static for all objecs (eg. like C ++ static class members.),
** and is pointed to in cl->cl_UserData. If the class is put into
** a loadable module, all this should be put into the libraryBase 
** (except for AslBase, of course).
*/

struct StaticDirListData
{
    struct AslBase_intern *sd_AslBase;
    struct Hook	sd_FileDisplayHook;
    struct Hook	sd_FileConstructHook;
    struct Hook	sd_FileDestructHook;

    struct Hook	sd_VolDisplayHook;
    struct Hook	sd_VolConstructHook;
    struct Hook	sd_VolDestructHook;
    
    UBYTE	sd_DispHookBuf[DISPHOOKBUFSIZE];
    
};


struct path 
{
    STRPTR buf;
    ULONG buflen;
};

struct DirListData
{
    struct path *dld_CurPath;
    
    Object	*dld_FileList;
    Object	*dld_VolumesList;
    
    UBYTE	dld_Flags;
    
};



/* Is currently volumes or files shown in the listview ? */
#define DLFLG_FILELIST (1 << 0)

/* Tells whether a filelist has been read */
#define DLFLG_FILELISTREAD (2 << 0)


#define GetSDLD(cl) \
((struct StaticDirListData *)cl->cl_UserData) 


BOOL AddToPath(STRPTR, STRPTR *, ULONG *, struct AslBase_intern *);
BOOL UpdateFileList(Class *cl, Object *, struct GadgetInfo *, struct AslBase_intern *);
BOOL GetDir(STRPTR, Object *, struct AslBase_intern *);
BOOL GetVolumes(Object *, struct AslBase_intern *);


AROS_UFP3(VOID, FileDisplayHook,
    AROS_UFPA(struct Hook *,		hook,		A0),
    AROS_UFPA(STRPTR *,			dharray,	A2),
    AROS_UFPA(struct ExAllData *, 	ead,		A1)
);
AROS_UFP3(VOID, VolDisplayHook,
    AROS_UFPA(struct Hook *,		hook,		A0),
    AROS_UFPA(STRPTR *,			dharray,	A2),
    AROS_UFPA(struct VolumeInfo *, 	vi,		A1)
);

AROS_UFP3(APTR, VolConstructHook,
    AROS_UFPA(struct Hook *,		hook,		A0),
    AROS_UFPA(APTR,			pool,		A2),
    AROS_UFPA(struct DosList *,		dlist,		A1)
);
AROS_UFP3(VOID, VolDestructHook,
    AROS_UFPA(struct Hook *,		hook,		A0),
    AROS_UFPA(APTR,			pool,		A2),
    AROS_UFPA(struct VolumeInfo *,	vi,		A1)
);
AROS_UFP3(APTR, FileConstructHook,
    AROS_UFPA(struct Hook *,		hook,		A0),
    AROS_UFPA(APTR,			pool,		A2),
    AROS_UFPA(struct ExAllData *,	ead,		A1)
);

AROS_UFP3(VOID, FileDestructHook,
    AROS_UFPA(struct Hook *,		hook,		A0),
    AROS_UFPA(APTR,			pool,		A2),
    AROS_UFPA(struct ExAllData *,	ead,		A1)
);


struct path *path_init(STRPTR initval, struct AslBase_intern *AslBase);
BOOL path_set(struct path *path, STRPTR val, struct AslBase_intern *AslBase);
BOOL path_add(struct path *path, STRPTR toadd, struct AslBase_intern *AslBase);
VOID path_cleanup(struct path *path, struct AslBase_intern *AslBase);
STRPTR path_string(struct path *path);




/* "public" stuff */
#define DIRLIST_TAGBASE 30000


#define AROSA_DirList_Path		DIRLIST_TAGBASE + 1	/* [ISG] */
#define AROSA_DirList_VolumesShown	DIRLIST_TAGBASE + 2	/* [G] */


#define AROSM_DirList_ShowParent 	DIRLIST_TAGBASE + 50
#define AROSM_DirList_ShowVolumes 	DIRLIST_TAGBASE + 51

struct AROSP_DirList_ShowVolumes
{
    STACKULONG		MethodID;
    struct GadgetInfo 	*GInfo;
};

struct AROSP_DirList_ShowParent
{
    STACKULONG		MethodID;
    struct GadgetInfo 	*GInfo;
};

struct IClass *InitDirListClass(struct AslBase_intern *AslBase);
VOID CleanupDirListClass(struct IClass *, struct AslBase_intern *);
