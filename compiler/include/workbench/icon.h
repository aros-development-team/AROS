#ifndef WORKBENCH_ICON_H
#define WORKBENCH_ICON_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef GRAPHICS_VIEW_H
#   include <graphics/view.h>
#endif

#ifndef INTUITION_IMAGECLASS_H
#   include <intuition/imageclass.h>
#endif

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#ifndef DATATYPES_PICTURECLASS_H
#   include <datatypes/pictureclass.h>
#endif

#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif

/*** Icon library name ******************************************************/
#define ICONNAME "icon.library"

/*** Start of icon.library tags *********************************************/
#define ICONA_BASE                          (TAG_USER+0x9000)

/*** Error reporting ********************************************************/
/* Errorcode (LONG *) */
#define ICONA_ErrorCode                      (ICONA_BASE+1)

/* Points to the tag item that caused the error (struct TagItem **). */
#define ICONA_ErrorTagItem                   (ICONA_BASE+75)

/*** Global options for IconControlA() **************************************/
/* Screen to use for remapping icons to (struct Screen *) */
#define ICONCTRLA_SetGlobalScreen            (ICONA_BASE+2)
#define ICONCTRLA_GetGlobalScreen            (ICONA_BASE+3)

/* Icon color remapping precision, default is PRECISION_ICON (LONG) */
#define ICONCTRLA_SetGlobalPrecision         (ICONA_BASE+4)
#define ICONCTRLA_GetGlobalPrecision         (ICONA_BASE+5)

/* Icon frame size dimensions (struct Rectangle *) */
#define ICONCTRLA_SetGlobalEmbossRect        (ICONA_BASE+6)
#define ICONCTRLA_GetGlobalEmbossRect        (ICONA_BASE+7)

/* Render image without frame (BOOL) */
#define ICONCTRLA_SetGlobalFrameless         (ICONA_BASE+8)
#define ICONCTRLA_GetGlobalFrameless         (ICONA_BASE+9)

/* Enable NewIcons support (BOOL) */
#define ICONCTRLA_SetGlobalNewIconsSupport   (ICONA_BASE+10)
#define ICONCTRLA_GetGlobalNewIconsSupport   (ICONA_BASE+11)

/* Enable color icon support (BOOL) */
#define ICONCTRLA_SetGlobalColorIconSupport  (ICONA_BASE+77)
#define ICONCTRLA_GetGlobalColorIconSupport  (ICONA_BASE+78)

/* Set/Get the hook to be called when identifying a file (struct Hook *) */
#define ICONCTRLA_SetGlobalIdentifyHook      (ICONA_BASE+12)
#define ICONCTRLA_GetGlobalIdentifyHook      (ICONA_BASE+13)

/* Maximum length of a file/drawer name supported by icon.library (LONG) */
#define ICONCTRLA_SetGlobalMaxNameLength     (ICONA_BASE+67)
#define ICONCTRLA_GetGlobalMaxNameLength     (ICONA_BASE+68)

/*** Per icon local options for IconControlA() ******************************/
/* Get the icon rendering masks (PLANEPTR) */
#define ICONCTRLA_GetImageMask1         (ICONA_BASE+14)
#define ICONCTRLA_GetImageMask2         (ICONA_BASE+15)

/* Transparent image color, set to -1 if opaque */
#define ICONCTRLA_SetTransparentColor1  (ICONA_BASE+16)
#define ICONCTRLA_GetTransparentColor1  (ICONA_BASE+17)
#define ICONCTRLA_SetTransparentColor2  (ICONA_BASE+18)
#define ICONCTRLA_GetTransparentColor2  (ICONA_BASE+19)

/* Image color palette (struct ColorRegister *) */
#define ICONCTRLA_SetPalette1           (ICONA_BASE+20)
#define ICONCTRLA_GetPalette1           (ICONA_BASE+21)
#define ICONCTRLA_SetPalette2           (ICONA_BASE+22)
#define ICONCTRLA_GetPalette2           (ICONA_BASE+23)

/* Size of image color palette (LONG) */
#define ICONCTRLA_SetPaletteSize1       (ICONA_BASE+24)
#define ICONCTRLA_GetPaletteSize1       (ICONA_BASE+25)
#define ICONCTRLA_SetPaletteSize2       (ICONA_BASE+26)
#define ICONCTRLA_GetPaletteSize2       (ICONA_BASE+27)

/* Image data; one by per pixel (UBYTE *) */
#define ICONCTRLA_SetImageData1         (ICONA_BASE+28)
#define ICONCTRLA_GetImageData1         (ICONA_BASE+29)
#define ICONCTRLA_SetImageData2         (ICONA_BASE+30)
#define ICONCTRLA_GetImageData2         (ICONA_BASE+31)

/* Render image without frame (BOOL) */
#define ICONCTRLA_SetFrameless          (ICONA_BASE+32)
#define ICONCTRLA_GetFrameless          (ICONA_BASE+33)

/* Enable NewIcons support (BOOL) */
#define ICONCTRLA_SetNewIconsSupport    (ICONA_BASE+34)
#define ICONCTRLA_GetNewIconsSupport    (ICONA_BASE+35)

/* Icon aspect ratio (UBYTE *) */
#define ICONCTRLA_SetAspectRatio        (ICONA_BASE+36)
#define ICONCTRLA_GetAspectRatio        (ICONA_BASE+37)

/* Icon dimensions, valid only for palette mapped icon images (LONG) */
#define ICONCTRLA_SetWidth              (ICONA_BASE+38)
#define ICONCTRLA_GetWidth              (ICONA_BASE+39)
#define ICONCTRLA_SetHeight             (ICONA_BASE+40)
#define ICONCTRLA_GetHeight             (ICONA_BASE+41)

/* Check whether the icon is palette mapped (LONG *) */
#define ICONCTRLA_IsPaletteMapped       (ICONA_BASE+42)

/* Get the screen the icon is attached to (struct Screen **) */
#define ICONCTRLA_GetScreen             (ICONA_BASE+43)

/* Check whether the icon has a real select image (LONG *) */
#define ICONCTRLA_HasRealImage2         (ICONA_BASE+44)

/* Check whether the icon is of the NewIcon type (LONG *) */
#define ICONCTRLA_IsNewIcon             (ICONA_BASE+79)

/* Image data: In RECTFMT_ARGB32 format (ULONG **) */
#define ICONCTRLA_GetARGBImageData1 	(ICONA_BASE+301)
#define ICONCTRLA_GetARGBImageData2 	(ICONA_BASE+303)

/*
    Check if this icon was allocated by icon.library or if it consists
    solely of a statically allocated DiskObject. (LONG *)
*/
#define ICONCTRLA_IsNativeIcon          (ICONA_BASE+80)

/*** Icon aspect handling ***************************************************/
/* Icon aspect ratio is not known */
#define ICON_ASPECT_RATIO_UNKNOWN (0)

/* Pack the aspect ratio into a single byte */
#define PACK_ICON_ASPECT_RATIO(num,den) (((num) << 4) | (den))

/* Unpack the aspect ratio stored in a single byte */
#define UNPACK_ICON_ASPECT_RATIO(v,num,den)     \
        do {                                    \
                num     = (((v) >> 4) & 15);    \
                den     = ( (v)       & 15);    \
        } while(0)

/*** Tags for use with GetIconTagList() *************************************/
/* Default icon type to retrieve (LONG) */
#define ICONGETA_GetDefaultType         (ICONA_BASE+45)

/* Retrieve default icon for the given name (STRPTR) */
#define ICONGETA_GetDefaultName         (ICONA_BASE+46)

/* Return default icon if the requested icon file cannot be found (BOOL) */
#define ICONGETA_FailIfUnavailable      (ICONA_BASE+47)

/* If possible, retrieve a palette mapped icon (BOOL) */
#define ICONGETA_GetPaletteMappedIcon   (ICONA_BASE+48)

/* Set if the icon returned is a default icon (BOOL *) */
#define ICONGETA_IsDefaultIcon          (ICONA_BASE+49)

/* Remap the icon to the default screen, if possible (BOOL) */
#define ICONGETA_RemapIcon              (ICONA_BASE+50)

/* Generate icon image masks (BOOL) */
#define ICONGETA_GenerateImageMasks     (ICONA_BASE+51)

/* Label text to be assigned to the icon (STRPTR) */
#define ICONGETA_Label                  (ICONA_BASE+52)

/* Screen to remap the icon to (struct Screen *) */
#define ICONGETA_Screen                 (ICONA_BASE+69)

/*** Tags for use with PutIconTagList() *************************************/
/* Notify Workbench of the icon being written (BOOL) */
#define ICONPUTA_NotifyWorkbench        (ICONA_BASE+53)

/* Store icon as the default for this type (LONG) */
#define ICONPUTA_PutDefaultType         (ICONA_BASE+54)

/* Store icon as a default for the given name (STRPTR) */
#define ICONPUTA_PutDefaultName         (ICONA_BASE+55)

/* 
    Don't save the the original planar image with the file if writing a 
    palette mapped icon.  Replace it with a tiny replacement image.
*/
#define ICONPUTA_DropPlanarIconImage    (ICONA_BASE+56)

/* Don't write the chunky icon image data to disk */
#define ICONPUTA_DropChunkyIconImage    (ICONA_BASE+57)

/* Don't write the NewIcons tool types to disk */
#define ICONPUTA_DropNewIconToolTypes   (ICONA_BASE+58)

/* Try to compress the image data more efficiently. */
#define ICONPUTA_OptimizeImageSpace     (ICONA_BASE+59)

/* 
    Don't write the entire icon file back to disk, only change 
    do->do_CurrentX / do->do_CurrentY.
*/
#define ICONPUTA_OnlyUpdatePosition     (ICONA_BASE+72)

/* 
    Preserve the original planar image data when writing a pelette mapped 
    icon to disk (BOOL). 
*/
#define ICONPUTA_PreserveOldIconImages  (ICONA_BASE+84)

/*** For use with the file identification hook ******************************/
struct IconIdentifyMsg
{
        /* Libraries that are already opened. */
        struct Library *        iim_SysBase;
        struct Library *        iim_DOSBase;
        struct Library *        iim_UtilityBase;
        struct Library *        iim_IconBase;

        /* File context information. */
        BPTR                    iim_FileLock;   /* Lock on the object to return an icon for. */
        BPTR                    iim_ParentLock; /* Lock on the object's parent directory, if available. */
        struct FileInfoBlock *  iim_FIB;        /* Already initialized. */
        BPTR                    iim_FileHandle; /* Pointer to the file to examine,
                                                 * positioned right at the first byte.
                                                 * May be NULL.
                                                 */
        struct TagItem *        iim_Tags;       /* Tags passed to GetIconTagList(). */
};

/*** Tags for use with DupDiskObjectA() *************************************/
/* Duplicate do_DrawerData */
#define ICONDUPA_DuplicateDrawerData    (ICONA_BASE+60)

/* Duplicate the Image structures. */
#define ICONDUPA_DuplicateImages        (ICONA_BASE+61)

/* Duplicate the image data (Image->ImageData) itself. */
#define ICONDUPA_DuplicateImageData     (ICONA_BASE+62)

/* Duplicate the default tool. */
#define ICONDUPA_DuplicateDefaultTool   (ICONA_BASE+63)

/* Duplicate the tool types list. */
#define ICONDUPA_DuplicateToolTypes     (ICONA_BASE+64)

/* Duplicate the tool window. */
#define ICONDUPA_DuplicateToolWindow    (ICONA_BASE+65)

/* 
    If the icon to be duplicated is in fact a palette mapped icon which has 
    never been set up to be displayed on the screen, turn the duplicate into 
    that palette mapped icon.
 */
#define ICONDUPA_ActivateImageData      (ICONA_BASE+82)

/*** Tags for use with DrawIconStateA() and GetIconRectangleA() *************/
/* Drawing information to use (struct DrawInfo *) */
#define ICONDRAWA_DrawInfo              (ICONA_BASE+66)

/* Draw the icon without the surrounding frame (BOOL) */
#define ICONDRAWA_Frameless             (ICONA_BASE+70)

/* Erase the background before drawing a frameless icon (BOOL) */
#define ICONDRAWA_EraseBackground       (ICONA_BASE+71)

/* Draw the icon without the surrounding border and frame (BOOL) */
#define ICONDRAWA_Borderless            (ICONA_BASE+83)

/*** Reserved tags **********************************************************/
#define ICONA_Reserved1                 (ICONA_BASE+73)
#define ICONA_Reserved2                 (ICONA_BASE+74)
#define ICONA_Reserved3                 (ICONA_BASE+76)
#define ICONA_Reserved4                 (ICONA_BASE+81)
#define ICONA_Reserved5                 (ICONA_BASE+85)
#define ICONA_Reserved6                 (ICONA_BASE+86)
#define ICONA_Reserved7                 (ICONA_BASE+87)
#define ICONA_Reserved8                 (ICONA_BASE+88)

/*** Last tag ***************************************************************/
#define ICONA_LAST_TAG                  (ICONA_BASE+88)

#endif /* WORKBENCH_ICON_H */
