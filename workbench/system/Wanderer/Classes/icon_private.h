#ifndef _WANDERER_CLASSES_ICON_PRIVATE_H_
#define _WANDERER_CLASSES_ICON_PRIVATE_H_

#include "icon.h"

/*** Instance data **********************************************************/
struct Icon_DATA
{
    /* filename is put into IcD_IconNode->name */
    struct Node                   IcD_IconNode;
    struct Node                   IcD_SelectionNode;

#define IcD_Filename_TXTBUFF IcD_IconNode.name
    UBYTE                         *IcD_Label_TXTBUFF;
    UBYTE                         *IcD_DisplayedLabel_TXTBUFF;
    ULONG                         IcD_DisplayedLabel_SplitParts;
    ULONG                          IcD_DisplayedLabel_Width;

    struct DiskObject             *IcD_DiskObj;                       /* The icons disk objects */
    struct FileInfoBlock          IcD_FileInfoBlock;

    LONG                          IcD_IconX,                          /* Top Left Co-ords of Icons "AREA" */
                                  IcD_IconY;

    ULONG                         IcD_IconWidth,                      /* Width/Height of Icon "Image" */
                                  IcD_IconHeight,
                                  IcD_AreaWidth,                      /* Width/Height of Icon "AREA" ..    */
                                  IcD_AreaHeight;                     /* if the icons Label Width is larger than
                                                                         IcD_IconWidth, AreaWidth = the icons label Width
                                                                         else it will be the same as IcD_IconWidth */

    ULONG                         IcD_Type;
    ULONG                         IcD_Flags;

    UBYTE                         *IcD_Date_TXTBUFF;
    ULONG                         IcD_Date_Width;
    UBYTE                         *IcD_Time_TXTBUFF;
    ULONG                         IcD_Time_Width;
    UBYTE                         *IcD_Size_TXTBUFF;
    ULONG                         IcD_Size_Width;
    UBYTE                         *IcD_Protection_TXTBUFF;

    void                           *IcD_UDATA;
};

#endif /* _WANDERER_CLASSES_ICON_PRIVATE_H_ */
