#ifndef WORKBOOK_H
#define WORKBOOK_H

#include <intuition/classes.h>

struct WorkbookBase {
    APTR wb_IntuitionBase;
    APTR wb_DOSBase;
    APTR wb_UtilityBase;
    APTR wb_GadToolsBase;
    APTR wb_IconBase;
    APTR wb_WorkbenchBase;
    APTR wb_GfxBase;

    Class  *wb_WBApp;
    Class  *wb_WBWindow;
    Class  *wb_WBVirtual;
    Class  *wb_WBIcon;
    Class  *wb_WBSet;

    Object *wb_App;
};

int WB_Main(struct WorkbookBase *wb);
void snoop(Class *cl, Object *obj, Msg msg);

#define IntuitionBase wb->wb_IntuitionBase
#define DOSBase       wb->wb_DOSBase
#define UtilityBase   wb->wb_UtilityBase
#define GadToolsBase  wb->wb_GadToolsBase
#define IconBase      wb->wb_IconBase
#define WorkbenchBase wb->wb_WorkbenchBase
#define GfxBase       wb->wb_GfxBase

#endif /* WORKBOOK_H */
