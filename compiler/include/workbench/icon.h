#ifndef WORKBENCH_ICON_H
#define WORKBENCH_ICON_H

/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Icon definitions
    Lang: english
*/

#ifndef GRAPHICS_VIEW_H
#include <graphics/view.h>
#endif

#ifndef INTUITION_IMAGECLASS_H
#include <intuition/imageclass.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef DATATYPES_PICTURECLASS_H
#include <datatypes/pictureclass.h>
#endif

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

/*** Icon library name ******************************************************/

const STRPTR ICONNAME = "icon.library";

/*** Start of icon.library tags *********************************************/

#define ICONA_Dummy (TAG_USER+0x9000)

/*** Error reporting ********************************************************/

/* Errorcode (LONG *) */
const Tag ICONA_ErrorCode                     = ICONA_Dummy +  1;

/* Points to the tag item that caused the error (struct TagItem **). */
const Tag ICONA_ErrorTagItem                  = ICONA_Dummy + 75;

/*** Global options for IconControlA() **************************************/

/* Screen to use for remapping icons to (struct Screen *) */
const Tag ICONCTRLA_SetGlobalScreen           = ICONA_Dummy +  2;
const Tag ICONCTRLA_GetGlobalScreen           = ICONA_Dummy +  3;

/* Icon color remapping precision, default is PRECISION_ICON (LONG) */
const Tag ICONCTRLA_SetGlobalPrecision        = ICONA_Dummy +  4;
const Tag ICONCTRLA_GetGlobalPrecision        = ICONA_Dummy +  5;

/* Icon frame size dimensions (struct Rectangle *) */
const Tag ICONCTRLA_SetGlobalEmbossRect       = ICONA_Dummy +  6;
const Tag ICONCTRLA_GetGlobalEmbossRect       = ICONA_Dummy +  7;

/* Render image without frame (BOOL) */
const Tag ICONCTRLA_SetGlobalFrameless        = ICONA_Dummy +  8;
const Tag ICONCTRLA_GetGlobalFrameless        = ICONA_Dummy +  9;

/* Enable NewIcons support (BOOL) */
const Tag ICONCTRLA_SetGlobalNewIconsSupport  = ICONA_Dummy + 10;
const Tag ICONCTRLA_GetGlobalNewIconsSupport  = ICONA_Dummy + 11;

/* Enable color icon support (BOOL) */
const Tag ICONCTRLA_SetGlobalColorIconSupport = ICONA_Dummy + 77;
const Tag ICONCTRLA_GetGlobalColorIconSupport = ICONA_Dummy + 78;

/* Set/Get the hook to be called when identifying a file (struct Hook *) */
const Tag ICONCTRLA_SetGlobalIdentifyHook     = ICONA_Dummy + 12;
const Tag ICONCTRLA_GetGlobalIdentifyHook     = ICONA_Dummy + 13;

/* Maximum length of a file/drawer name supported by icon.library (LONG) */
const Tag ICONCTRLA_SetGlobalMaxNameLength    = ICONA_Dummy + 67;
const Tag ICONCTRLA_GetGlobalMaxNameLength    = ICONA_Dummy + 68;

/*** Per icon local options for IconControlA() ******************************/

/* Get the icon rendering masks (PLANEPTR) */
const Tag ICONCTRLA_GetImageMask1             = ICONA_Dummy + 14;
const Tag ICONCTRLA_GetImageMask2             = ICONA_Dummy + 15;

/* Transparent image color, set to -1 if opaque */
const Tag ICONCTRLA_SetTransparentColor1      = ICONA_Dummy + 16;
const Tag ICONCTRLA_GetTransparentColor1      = ICONA_Dummy + 17;
const Tag ICONCTRLA_SetTransparentColor2      = ICONA_Dummy + 18;
const Tag ICONCTRLA_GetTransparentColor2      = ICONA_Dummy + 19;

/* Image color palette (struct ColorRegister *) */
const Tag ICONCTRLA_SetPalette1               = ICONA_Dummy + 20;
const Tag ICONCTRLA_GetPalette1               = ICONA_Dummy + 21;
const Tag ICONCTRLA_SetPalette2               = ICONA_Dummy + 22;
const Tag ICONCTRLA_GetPalette2               = ICONA_Dummy + 23;

/* Size of image color palette (LONG) */
const Tag ICONCTRLA_SetPaletteSize1           = ICONA_Dummy + 24;
const Tag ICONCTRLA_GetPaletteSize1           = ICONA_Dummy + 25;
const Tag ICONCTRLA_SetPaletteSize2           = ICONA_Dummy + 26;
const Tag ICONCTRLA_GetPaletteSize2           = ICONA_Dummy + 27;

/* Image data; one by per pixel (UBYTE *) */
const Tag ICONCTRLA_SetImageData1             = ICONA_Dummy + 28;
const Tag ICONCTRLA_GetImageData1             = ICONA_Dummy + 29;
const Tag ICONCTRLA_SetImageData2             = ICONA_Dummy + 30;
const Tag ICONCTRLA_GetImageData2             = ICONA_Dummy + 31;

/* Render image without frame (BOOL) */
const Tag ICONCTRLA_SetFrameless              = ICONA_Dummy + 32;
const Tag ICONCTRLA_GetFrameless              = ICONA_Dummy + 33;

/* Enable NewIcons support (BOOL) */
const Tag ICONCTRLA_SetNewIconsSupport        = ICONA_Dummy + 34;
const Tag ICONCTRLA_GetNewIconsSupport        = ICONA_Dummy + 35;

/* Icon aspect ratio (UBYTE *) */
const Tag ICONCTRLA_SetAspectRatio            = ICONA_Dummy + 36;
const Tag ICONCTRLA_GetAspectRatio            = ICONA_Dummy + 37;

/* Icon dimensions, valid only for palette mapped icon images (LONG) */
const Tag ICONCTRLA_SetWidth                  = ICONA_Dummy + 38;
const Tag ICONCTRLA_GetWidth                  = ICONA_Dummy + 39;
const Tag ICONCTRLA_SetHeight                 = ICONA_Dummy + 40;
const Tag ICONCTRLA_GetHeight                 = ICONA_Dummy + 41;

/* Check whether the icon is palette mapped (LONG *) */
const Tag ICONCTRLA_IsPaletteMapped           = ICONA_Dummy + 42;

/* Get the screen the icon is attached to (struct Screen **) */
const Tag ICONCTRLA_GetScreen                 = ICONA_Dummy + 43;

/* Check whether the icon has a real select image (LONG *) */
const Tag ICONCTRLA_HasRealImage2             = ICONA_Dummy + 44;

/* Check whether the icon is of the NewIcon type (LONG *) */
const Tag ICONCTRLA_IsNewIcon                 = ICONA_Dummy + 79;

/* Check if this icon was allocated by icon.library or if it consists
 * solely of a statically allocated DiskObject. (LONG *)
 */
const Tag ICONCTRLA_IsNativeIcon              = ICONA_Dummy + 80;

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
const Tag ICONGETA_GetDefaultType         = ICONA_Dummy + 45;

/* Retrieve default icon for the given name (STRPTR) */
const Tag ICONGETA_GetDefaultName         = ICONA_Dummy + 46;

/* Return default icon if the requested icon file cannot be found (BOOL) */
const Tag ICONGETA_FailIfUnavailable      = ICONA_Dummy + 47;

/* If possible, retrieve a palette mapped icon (BOOL) */
const Tag ICONGETA_GetPaletteMappedIcon   = ICONA_Dummy + 48;

/* Set if the icon returned is a default icon (BOOL *) */
const Tag ICONGETA_IsDefaultIcon          = ICONA_Dummy + 49;

/* Remap the icon to the default screen, if possible (BOOL) */
const Tag ICONGETA_RemapIcon              = ICONA_Dummy + 50;

/* Generate icon image masks (BOOL) */
const Tag ICONGETA_GenerateImageMasks     = ICONA_Dummy + 51;

/* Label text to be assigned to the icon (STRPTR) */
const Tag ICONGETA_Label                  = ICONA_Dummy + 52;

/* Screen to remap the icon to (struct Screen *) */
const Tag ICONGETA_Screen                 = ICONA_Dummy + 69;

/*** Tags for use with PutIconTagList() *************************************/

/* Notify Workbench of the icon being written (BOOL) */
const Tag ICONPUTA_NotifyWorkbench        = ICONA_Dummy + 53;

/* Store icon as the default for this type (LONG) */
const Tag ICONPUTA_PutDefaultType         = ICONA_Dummy + 54;

/* Store icon as a default for the given name (STRPTR) */
const Tag ICONPUTA_PutDefaultName         = ICONA_Dummy + 55;

/* Don't save the the original planar image with the file if writing a
 * palette mapped icon.  Replace it with a tiny replacement image.
 */
const Tag ICONPUTA_DropPlanarIconImage    = ICONA_Dummy + 56;

/* Don't write the chunky icon image data to disk */
const Tag ICONPUTA_DropChunkyIconImage    = ICONA_Dummy + 57;

/* Don't write the NewIcons tool types to disk */
const Tag ICONPUTA_DropNewIconToolTypes   = ICONA_Dummy + 58;

/* Try to compress the image data more efficiently. */
const Tag ICONPUTA_OptimizeImageSpace     = ICONA_Dummy + 59;

/* Don't write the entire icon file back to disk,
 * only change do->do_CurrentX / do->do_CurrentY.
 */
const Tag ICONPUTA_OnlyUpdatePosition     = ICONA_Dummy + 72;

/* Preserve the original planar image data when writing
   a pelette mapped icon to disk (BOOL). */
const Tag ICONPUTA_PreserveOldIconImages  = ICONA_Dummy + 84;

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

/*** Tags for use with DupDiskObjectA() ***/

/* Duplicate do_DrawerData */
const Tag ICONDUPA_DuplicateDrawerData    = ICONA_Dummy + 60;

/* Duplicate the Image structures. */
const Tag ICONDUPA_DuplicateImages        = ICONA_Dummy + 61;

/* Duplicate the image data (Image->ImageData) itself. */
const Tag ICONDUPA_DuplicateImageData     = ICONA_Dummy + 62;

/* Duplicate the default tool. */
const Tag ICONDUPA_DuplicateDefaultTool   = ICONA_Dummy + 63;

/* Duplicate the tool types list. */
const Tag ICONDUPA_DuplicateToolTypes     = ICONA_Dummy + 64;

/* Duplicate the tool window. */
const Tag ICONDUPA_DuplicateToolWindow    = ICONA_Dummy + 65;

/* If the icon to be duplicated is in fact a palette mapped
 * icon which has never been set up to be displayed on the
 * screen, turn the duplicate into that palette mapped icon.
 */
const Tag ICONDUPA_ActivateImageData      = ICONA_Dummy + 82;

/*** Tags for use with DrawIconStateA() and GetIconRectangleA() *************/

/* Drawing information to use (struct DrawInfo *) */
const Tag ICONDRAWA_DrawInfo              = ICONA_Dummy + 66;

/* Draw the icon without the surrounding frame (BOOL) */
const Tag ICONDRAWA_Frameless             = ICONA_Dummy + 70;

/* Erase the background before drawing a frameless icon (BOOL) */
const Tag ICONDRAWA_EraseBackground       = ICONA_Dummy + 71;

/* Draw the icon without the surrounding border and frame (BOOL) */
const Tag ICONDRAWA_Borderless            = ICONA_Dummy + 83;

/*** Reserved tags **********************************************************/

const Tag ICONA_Reserved1                 = ICONA_Dummy + 73;
const Tag ICONA_Reserved2                 = ICONA_Dummy + 74;
const Tag ICONA_Reserved3                 = ICONA_Dummy + 76;
const Tag ICONA_Reserved4                 = ICONA_Dummy + 81;
const Tag ICONA_Reserved5                 = ICONA_Dummy + 85;
const Tag ICONA_Reserved6                 = ICONA_Dummy + 86;
const Tag ICONA_Reserved7                 = ICONA_Dummy + 87;
const Tag ICONA_Reserved8                 = ICONA_Dummy + 88;

/*** Last tag ***************************************************************/

const Tag ICONA_LAST_TAG                  = ICONA_Dummy + 88;

#endif /* WORKBENCH_ICON_H */
