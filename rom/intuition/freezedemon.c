#include <intuition/intuitionbase.h>
#include <exec/io.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include LC_LIBDEFS_FILE

void dprintf(const char *, ...);

/* Dump intuition's state if F10 is pressed while F1 is down */

struct InputEvent *handler(void)
{
    struct InputEvent *events = (APTR)REG_A0;
    struct InputEvent *event;
    static BOOL F1_pressed = FALSE;

    for (event = events; event; event = event->ie_NextEvent)
    {
        if (event->ie_Class == IECLASS_RAWKEY)
        {
            switch (event->ie_Code)
            {
            case 0x50: /* F1 */
                F1_pressed = TRUE;
                break;

            case 0xd0: /* F1 up */
                F1_pressed = FALSE;
                break;

            case 0x59: /* F10 */
                if (F1_pressed)
                {
                    REG_A6 = (ULONG)IntuitionBase;
                    MyEmulHandle->EmulCallDirectOS(-152 * 6);
                }
                break;
            }
        }
    }

    return events;
}

struct EmulLibEntry gate =
    {
        TRAP_LIB, 0, (void(*)(void))handler
    };

struct Interrupt interrupt =
    {
        {
            NULL, NULL, NT_INTERRUPT, 120, "FreezeDemon"
        },
        NULL,
        (void(*)())&gate
    };

int main(void)
{
    if (IntuitionBase->LibNode.lib_Version == VERSION_NUMBER)
    {
        struct MsgPort *port = CreateMsgPort();
        if (port)
        {
            struct IOStdReq *req = CreateIORequest(port, sizeof(*req));
            if (req)
            {
                if (OpenDevice("input.device", 0, req, 0) == 0)
                {
                    BYTE old_pri;

                    req->io_Command = IND_ADDHANDLER;
                    req->io_Data = &interrupt;
                    DoIO(req);

                    /* Make sure we have a higher priority than the
                     * input.device, in case it gets trapped in an
                     * endless loop.
                     */
                    old_pri = SetTaskPri(FindTask(NULL), 50);

                    /* Also dump if intuition's input handler has been
                     * frozen for more than 2s.
                     */
                    do
                    {
                        ULONG old_secs = IntuitionBase->Seconds;
                        ULONG old_micros = IntuitionBase->Micros;

                        Delay(200);

                        if (old_secs == IntuitionBase->Seconds &&
                                old_micros == IntuitionBase->Micros)
                        {
                            REG_A6 = (ULONG)IntuitionBase;
                            MyEmulHandle->EmulCallDirectOS(-152 * 6);

                            do
                            {
                                Delay(50);
                            }
                            while (old_secs == IntuitionBase->Seconds &&
                                    old_micros == IntuitionBase->Micros);

                        }
                    }
                    while ((SetSignal(0,0) & 0x1000) == 0);

                    SetTaskPri(FindTask(NULL), old_pri);

                    req->io_Command = IND_REMHANDLER;
                    req->io_Data = &interrupt;
                    DoIO(req);

                    CloseDevice(req);
                }

                DeleteIORequest(req);
            }

            DeleteMsgPort(port);
        }
    }

    return 0;
}
