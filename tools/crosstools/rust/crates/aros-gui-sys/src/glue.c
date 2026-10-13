/* Intuition windows, input and signals for Rust on AROS. Contract: glue.h. */
#include <stdint.h>
#include <string.h>
#include <exec/memory.h>
#include <devices/inputevent.h>
#include <devices/timer.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/keymap.h>

#include "glue.h"

/* Smallest interior worth opening; below this a window is unusable anyway. */
#define MIN_WINDOW_SIZE 64

/* Wanderer may not have opened the Workbench screen yet when a program
   launched from S:User-Startup gets here, so give it a moment. */
static struct Screen *lock_screen(void)
{
    int tries;
    for (tries = 0; tries < 200; tries++) {
        struct Screen *screen = LockPubScreen(NULL);
        if (screen)
            return screen;
        Delay(10);
    }
    return NULL;
}

int aros_gui_screen_size(int32_t *width, int32_t *height)
{
    struct Screen *screen = lock_screen();
    if (!screen)
        return -1;
    *width = screen->Width;
    *height = screen->Height;
    UnlockPubScreen(NULL, screen);
    return 0;
}

int aros_gui_screen_palette(uint32_t *rgb, int count)
{
    struct Screen *screen = lock_screen();
    struct ColorMap *cm;
    ULONG gun[3];
    int i, n = 0;

    if (!screen)
        return 0;
    cm = screen->ViewPort.ColorMap;
    if (cm) {
        n = cm->Count < count ? cm->Count : count;
        for (i = 0; i < n; i++) {
            GetRGB32(cm, i, 1, gun);
            rgb[i] = ((gun[0] >> 24) << 16) | ((gun[1] >> 24) << 8) | (gun[2] >> 24);
        }
    }
    UnlockPubScreen(NULL, screen);
    return n;
}

/* Intuition keeps the title pointer and reads it on every redraw of the
   frame, so the window must own its copy. */
static char *dup_string(const char *text)
{
    size_t len = text ? strlen(text) : 0;
    char *copy = AllocVec(len + 1, MEMF_ANY);
    if (!copy)
        return NULL;
    if (len)
        memcpy(copy, text, len);
    copy[len] = 0;
    return copy;
}

void aros_gui_window_info(uintptr_t handle, struct aros_gui_window_info *out)
{
    struct Window *window = (struct Window *)handle;

    out->handle = handle;
    out->left = window->LeftEdge;
    out->top = window->TopEdge;
    out->width = window->Width;
    out->height = window->Height;
    out->inner_width = window->Width - window->BorderLeft - window->BorderRight;
    out->inner_height = window->Height - window->BorderTop - window->BorderBottom;
    out->border_left = window->BorderLeft;
    out->border_top = window->BorderTop;
    out->sigmask = 1UL << window->UserPort->mp_SigBit;
}

void aros_gui_window_set_min_size(uintptr_t handle, int32_t width, int32_t height)
{
    struct Window *window = (struct Window *)handle;
    int32_t bw = window->BorderLeft + window->BorderRight;
    int32_t bh = window->BorderTop + window->BorderBottom;

    if (width < 1)
        width = 1;
    if (height < 1)
        height = 1;
    /* A maximum of ~0 lifts Intuition's default limit of the initial size. */
    WindowLimits(window, width + bw, height + bh, (UWORD)~0, (UWORD)~0);
}

int aros_gui_window_open(const struct aros_gui_window_attrs *attrs,
    struct aros_gui_window_info *out)
{
    struct Screen *screen = lock_screen();
    struct Window *window;
    int32_t width = attrs->inner_width, height = attrs->inner_height;
    int32_t left = attrs->left, top = attrs->top;
    int resizable = (attrs->flags & AROS_GUI_WINDOW_RESIZABLE) != 0;
    int borderless = (attrs->flags & AROS_GUI_WINDOW_BORDERLESS) != 0;
    char *title;

    if (!screen)
        return -1;
    title = dup_string(attrs->title);
    if (!title) {
        UnlockPubScreen(NULL, screen);
        return -1;
    }

    /* A GLA context renders into the window's RastPort, and a window larger
       than the screen leaves it nothing valid to present: the window stays
       blank with no GL error. Clamp to what the screen can hold. */
    {
        int32_t max_width = screen->Width - screen->WBorLeft - screen->WBorRight;
        int32_t max_height = screen->Height - screen->BarHeight - 1
            - screen->WBorTop - screen->WBorBottom;

        if (width > max_width)
            width = max_width;
        if (height > max_height)
            height = max_height;
        if (width < MIN_WINDOW_SIZE)
            width = MIN_WINDOW_SIZE;
        if (height < MIN_WINDOW_SIZE)
            height = MIN_WINDOW_SIZE;
    }
    if (left < 0)
        left = (screen->Width - width) / 2;
    if (top < 0)
        top = (screen->Height - height) / 2;

    /* SMART_REFRESH keeps the contents of an obscured window, so a GL client
       is not asked to repaint whenever another window passes over. RMBTrap
       turns the right button into a button instead of the menu key. */
    window = OpenWindowTags(NULL,
        WA_Title,         (IPTR)title,
        WA_Left,          left,
        WA_Top,           top,
        WA_InnerWidth,    width,
        WA_InnerHeight,   height,
        WA_Activate,      (attrs->flags & AROS_GUI_WINDOW_ACTIVATE) != 0,
        WA_Borderless,    borderless,
        WA_DragBar,       !borderless,
        WA_DepthGadget,   !borderless,
        WA_CloseGadget,   !borderless,
        WA_SizeGadget,    resizable && !borderless,
        WA_SizeBBottom,   resizable && !borderless,
        WA_SmartRefresh,  TRUE,
        WA_NoCareRefresh, TRUE,
        WA_ReportMouse,   TRUE,
        WA_RMBTrap,       TRUE,
        WA_NewLookMenus,  TRUE,
        WA_IDCMP,         IDCMP_CLOSEWINDOW | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE
                        | IDCMP_RAWKEY | IDCMP_NEWSIZE | IDCMP_CHANGEWINDOW
                        | IDCMP_ACTIVEWINDOW | IDCMP_INACTIVEWINDOW,
        WA_PubScreen,     (IPTR)screen,
        TAG_DONE);
    UnlockPubScreen(NULL, screen);

    if (!window) {
        FreeVec(title);
        return -1;
    }

    /* UserData is the application's own; close frees the title from it. */
    window->UserData = (BYTE *)title;
    aros_gui_window_set_min_size((uintptr_t)window, attrs->min_width, attrs->min_height);
    aros_gui_window_info((uintptr_t)window, out);
    return 0;
}

void aros_gui_window_close(uintptr_t handle)
{
    struct Window *window = (struct Window *)handle;
    char *title;

    if (!window)
        return;
    title = (char *)window->UserData;
    CloseWindow(window);
    /* Only after the window is gone can the title it points at be freed. */
    FreeVec(title);
}

int aros_gui_window_set_title(uintptr_t handle, const char *text)
{
    struct Window *window = (struct Window *)handle;
    char *title = dup_string(text);
    char *old;

    if (!title)
        return -1;
    old = (char *)window->UserData;
    SetWindowTitles(window, (CONST_STRPTR)title, (CONST_STRPTR)~0);
    window->UserData = (BYTE *)title;
    FreeVec(old);
    return 0;
}

void aros_gui_window_set_inner_size(uintptr_t handle, int32_t width, int32_t height)
{
    struct Window *window = (struct Window *)handle;

    ChangeWindowBox(window, window->LeftEdge, window->TopEdge,
        width + window->BorderLeft + window->BorderRight,
        height + window->BorderTop + window->BorderBottom);
}

void aros_gui_window_set_position(uintptr_t handle, int32_t left, int32_t top)
{
    struct Window *window = (struct Window *)handle;

    ChangeWindowBox(window, left, top, window->Width, window->Height);
}

void aros_gui_window_set_pointer_visible(uintptr_t handle, int visible)
{
    /* A 1x1 sprite with both planes clear: header, one line, terminator. */
    static UWORD blank[6];
    struct Window *window = (struct Window *)handle;

    if (visible)
        ClearPointer(window);
    else
        SetPointer(window, blank, 1, 16, 0, 0);
}

void aros_gui_window_focus(uintptr_t handle)
{
    struct Window *window = (struct Window *)handle;

    WindowToFront(window);
    ActivateWindow(window);
}

int aros_gui_window_is_active(uintptr_t handle)
{
    struct Window *window = (struct Window *)handle;

    return (window->Flags & WFLG_WINDOWACTIVE) != 0;
}

/* Copies the next pending message into `out` and replies it. Returns 0 when
   the port is empty. */
int aros_gui_window_next_message(uintptr_t handle, struct aros_gui_message *out)
{
    struct Window *window = (struct Window *)handle;
    struct IntuiMessage *msg = (struct IntuiMessage *)GetMsg(window->UserPort);

    if (!msg)
        return 0;

    out->class = msg->Class;
    out->code = msg->Code;
    out->qualifier = msg->Qualifier;
    out->mouse_x = msg->MouseX - window->BorderLeft;
    out->mouse_y = msg->MouseY - window->BorderTop;

    ReplyMsg((struct Message *)msg);
    return 1;
}

int aros_gui_map_rawkey(uint16_t code, uint16_t qualifier, char *buf, int len)
{
    struct InputEvent ie;

    memset(&ie, 0, sizeof(ie));
    ie.ie_Class = IECLASS_RAWKEY;
    ie.ie_Code = code;
    ie.ie_Qualifier = qualifier;
    return MapRawKey(&ie, (STRPTR)buf, len, NULL);
}

void *aros_gui_task_self(void)
{
    return FindTask(NULL);
}

int aros_gui_signal_alloc(void)
{
    return (int)AllocSignal(-1);
}

void aros_gui_signal_free(int bit)
{
    if (bit >= 0)
        FreeSignal(bit);
}

void aros_gui_signal_send(void *task, uint32_t mask)
{
    if (task)
        Signal((struct Task *)task, mask);
}

/* The timer for timed waits, opened by the first one in the waiting task:
   its port signal belongs to that task. */
static struct MsgPort *timer_port;
static struct timerequest *timer_req;

static int timer_open(void)
{
    if (timer_req)
        return 1;
    timer_port = CreateMsgPort();
    if (!timer_port)
        return 0;
    timer_req = (struct timerequest *)CreateIORequest(timer_port, sizeof(*timer_req));
    if (timer_req && OpenDevice((CONST_STRPTR)TIMERNAME, UNIT_MICROHZ,
            (struct IORequest *)timer_req, 0) == 0)
        return 1;
    if (timer_req)
        DeleteIORequest((struct IORequest *)timer_req);
    DeleteMsgPort(timer_port);
    timer_req = NULL;
    timer_port = NULL;
    return 0;
}

void aros_gui_wait_cleanup(void)
{
    if (!timer_req)
        return;
    CloseDevice((struct IORequest *)timer_req);
    DeleteIORequest((struct IORequest *)timer_req);
    DeleteMsgPort(timer_port);
    timer_req = NULL;
    timer_port = NULL;
}

uint32_t aros_gui_wait(uint32_t mask, int64_t timeout_us, int *timed_out)
{
    uint32_t timer_mask, got;

    *timed_out = 0;
    if (timeout_us == 0) {
        got = SetSignal(0, mask) & mask;
        *timed_out = got == 0;
        return got;
    }
    if (timeout_us < 0 || !timer_open())
        return Wait(mask) & mask;

    timer_mask = 1UL << timer_port->mp_SigBit;
    timer_req->tr_node.io_Command = TR_ADDREQUEST;
    timer_req->tr_time.tv_secs = (ULONG)(timeout_us / 1000000);
    timer_req->tr_time.tv_micro = (ULONG)(timeout_us % 1000000);
    SetSignal(0, timer_mask);
    SendIO((struct IORequest *)timer_req);

    got = Wait(mask | timer_mask);

    if (!CheckIO((struct IORequest *)timer_req))
        AbortIO((struct IORequest *)timer_req);
    WaitIO((struct IORequest *)timer_req);
    SetSignal(0, timer_mask);

    *timed_out = (got & mask) == 0;
    return got & mask;
}
