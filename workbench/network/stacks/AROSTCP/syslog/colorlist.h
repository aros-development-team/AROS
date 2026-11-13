/* ColorList header file */

#ifndef PRIVATE_COLORLIST_H
#define PRIVATE_COLORLIST_H

struct ColorListData
{
  ULONG ErrorPen;
  ULONG InfoPen;
  ULONG VerbosePen;
};

struct MUI_CustomClass *ColorList;

struct MUI_CustomClass *CreateColorListClass(void);

#define CLL_ChangePens  0x6EDA4FA0

#endif
