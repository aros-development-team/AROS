/***************************************************************************
** Object Types for MUI_MakeObject()
***************************************************************************/

#define MUIO_Label          1   /* STRPTR label, ULONG flags */
#define MUIO_Button         2   /* STRPTR label */
#define MUIO_Checkmark      3   /* STRPTR label */
#define MUIO_Cycle          4   /* STRPTR label, STRPTR *entries */
#define MUIO_Radio          5   /* STRPTR label, STRPTR *entries */
#define MUIO_Slider         6   /* STRPTR label, LONG min, LONG max */
#define MUIO_String         7   /* STRPTR label, LONG maxlen */
#define MUIO_PopButton      8   /* STRPTR imagespec */
#define MUIO_HSpace         9   /* LONG space   */
#define MUIO_VSpace        10   /* LONG space   */
#define MUIO_HBar          11   /* LONG space   */
#define MUIO_VBar          12   /* LONG space   */
#define MUIO_MenustripNM   13   /* struct NewMenu *nm, ULONG flags */
#define MUIO_Menuitem      14   /* STRPTR label, STRPTR shortcut, ULONG flags, ULONG data  */
#define MUIO_BarTitle      15   /* STRPTR label */
#define MUIO_NumericButton 16   /* STRPTR label, LONG min, LONG max, STRPTR format */

#define MUIO_Menuitem_CopyStrings (1<<30)

#define MUIO_Label_SingleFrame   (1<< 8)
#define MUIO_Label_DoubleFrame   (1<< 9)
#define MUIO_Label_LeftAligned   (1<<10)
#define MUIO_Label_Centered      (1<<11)
#define MUIO_Label_FreeVert      (1<<12)

#define MUIO_MenustripNM_CommandKeyCheck (1<<0) /* check for "localized" menu items such as "O\0Open" */
