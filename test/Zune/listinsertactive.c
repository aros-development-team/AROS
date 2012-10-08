#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <clib/alib_protos.h>
#include <exec/exec.h>
#include <exec/types.h>

#include <libraries/mui.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

static const char str[] =       "\33bTHERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. "
                                "EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDER AND/OR OTHER PARTIES "
                                "PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR "
                                "IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY "
                                "AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE "
                                "OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE "
                                "COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.\n"
                                "\n"
                                "\33bIN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING "
                                "WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY REDISTRIBUTE THE "
                                "PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY "
                                "GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE "
                                "USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS "
                                "OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR "
                                "THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER "
                                "PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE "
                                "POSSIBILITY OF SUCH DAMAGES.";

#define List(ftxt)               ListviewObject, MUIA_Weight, 50, MUIA_Listview_Input, FALSE, MUIA_Listview_List,\
                                 FloattextObject, MUIA_Frame, MUIV_Frame_ReadList, MUIA_Background, MUII_ReadListBack, MUIA_Floattext_Text, ftxt, MUIA_Floattext_TabSize, 4, MUIA_Floattext_Justify, TRUE, End, End

int main(int argc, char *argv[])
{
        Object *win, *app, *bt, *lv;
        ULONG sigs = 0;
        ULONG id;

        app = ApplicationObject,
                MUIA_Application_Title, "Foo",
                MUIA_Application_Base, "xxxxx",

                SubWindow, win = WindowObject,
                        MUIA_Window_Title, "Bar",

                        WindowContents, VGroup,

                                Child, VGroup,

                                Child, lv = ListviewObject,
                                        MUIA_CycleChain, 1,
                                        MUIA_Listview_List,
                                                ListObject, InputListFrame, End,
                                        End,
                                Child, bt = SimpleButton("Click me!"),
                                End,
                        End,
                End,
        End;

        DoMethod(win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
        DoMethod(bt, MUIM_Notify, MUIA_Pressed, FALSE, app, 2, MUIM_Application_ReturnID, 1);

        set(win, MUIA_Window_Open, TRUE);

        while((id = DoMethod(app,MUIM_Application_NewInput,&sigs)) != MUIV_Application_ReturnID_Quit) {

                if(id == 1) {

                        static const char *foo = "Hello World";

                        DoMethod(lv, MUIM_List_InsertSingle, foo, MUIV_List_Insert_Active);
                }

                if(sigs) {
                        sigs = Wait(sigs | SIGBREAKF_CTRL_C);
                        if (sigs & SIGBREAKF_CTRL_C) break;
                }
        }

        MUI_DisposeObject(app);

        return 0;
}
