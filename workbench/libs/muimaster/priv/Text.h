/****************************************************************************/
/** Text                                                                   **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Text[];
#else
#define MUIC_Text "Text.mui"
#endif

/* Attributes */

enum {
    MUIA_Text_Contents = 0x8042f8dc, /* V4  isg STRPTR            */
    MUIA_Text_HiChar =   0x804218ff, /* V4  i.. char              */
    MUIA_Text_PreParse = 0x8042566d, /* V4  isg STRPTR            */
    MUIA_Text_SetMax =   0x80424d0a, /* V4  i.. BOOL              */
    MUIA_Text_SetMin =   0x80424e10, /* V4  i.. BOOL              */
    MUIA_Text_SetVMax =  0x80420d8b, /* V11 i.. BOOL              */
};

/* Attributes */

#define MUIA_Text_HiCharIdx   0x804214f5

