#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <exec/memory.h>
#include <intuition/gadgetclass.h>

#include "gadgets.h"
#include "bootmenu_intern.h"

struct ButtonGadget *createButton
	(
		ULONG left, ULONG top, ULONG width, ULONG height,
		struct Gadget *prev,
		STRPTR name,
		UWORD id,
		struct BootMenuBase *BootMenuBase
	)
{
    struct TagItem tags[] =
    {
        {GA_Left,           left}, /* 0 */
        {GA_Top,             top}, /* 1 */
        {GA_Height,       height}, /* 2 */
        {GA_Width,         width}, /* 3 */
        {GA_Border,         NULL}, /* 4 */
        {GA_SelectRender,   NULL}, /* 5 */
        {GA_Previous, (IPTR)prev}, /* 6 */
        {GA_Text,     (IPTR)name}, /* 7 */
        {GA_ID,         (IPTR)id}, /* 8 */
        {GA_Immediate,      TRUE},
        {GA_RelVerify,      TRUE},
        {TAG_DONE,           0UL}
    };
    struct ButtonGadget *button;

	if ((button = AllocMem(sizeof(struct ButtonGadget), MEMF_PUBLIC | MEMF_CLEAR)) != NULL)
	{
		tags[4].ti_Data = (IPTR)&button->uborder2;
		tags[5].ti_Data = (IPTR)&button->sborder2;
		button->XY1[1] = height;
		button->XY1[4] = width;
		button->XY2[0] = width;
		button->XY2[2] = width;
		button->XY2[3] = height;
		button->XY2[5] = height;
		button->uborder1.FrontPen = 1;
		button->uborder1.DrawMode = JAM1;
		button->uborder1.Count = 3;
		button->uborder1.XY = button->XY2;
		button->uborder2.FrontPen = 2;
		button->uborder2.DrawMode = JAM1;
		button->uborder2.Count = 3;
		button->uborder2.XY = button->XY1;
		button->uborder2.NextBorder = &button->uborder1;
		button->sborder1.FrontPen = 2;
		button->sborder1.DrawMode = JAM1;
		button->sborder1.Count = 3;
		button->sborder1.XY = button->XY2;
		button->sborder2.FrontPen = 1;
		button->sborder2.DrawMode = JAM1;
		button->sborder2.Count = 3;
		button->sborder2.XY = button->XY1;
		button->sborder2.NextBorder = &button->sborder1;
		button->button = NewObjectA(NULL, FRBUTTONCLASS, tags);
		if (button->button)
		{
			button->gadget = prev->NextGadget;
			button->gadget->NextGadget = NULL;
			return button;
		}
		FreeMem(button, sizeof(struct ButtonGadget));
	}
	return NULL;
}

void freeButtonGadget(struct ButtonGadget *button, struct BootMenuBase *BootMenuBase) 
{
	DisposeObject(button->button);
	FreeMem(button, sizeof(struct ButtonGadget));
}
