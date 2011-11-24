/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Host-side part of GDI hidd. Handles windows and receives events.
    Lang: English.
*/

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <aros/irq.h>

#include "gdi.h"
#include "bitmap.h"

#define D(x)
#define DACT(x)
#define DKBD(x)
#define DMSE(x)
#define DWIN(x)

LRESULT CALLBACK display_callback(HWND win, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK bitmap_callback(HWND win, UINT msg, WPARAM wp, LPARAM lp);

/* Global variables shared by all windows (theoretical) */

static WNDCLASS display_class_desc =
{
    CS_NOCLOSE,
    display_callback,
    0,
    0,
    NULL,
    NULL,
    NULL,
    (HBRUSH)(COLOR_WINDOW + 1),
    NULL,
    "AROS_Screen"
};

static WNDCLASS bitmap_class_desc =
{
    CS_NOCLOSE,
    bitmap_callback,
    0,
    0,
    NULL,
    NULL,
    NULL,
    (HBRUSH)(COLOR_WINDOW + 1),
    NULL,
    "AROS_Bitmap"
};

static DWORD thread_id;     /* Window service thread ID                       */
static HWND window_active;  /* Currently active AROS window                   */
static ULONG display_class; /* Display window class                           */
static ULONG bitmap_class;  /* Bitmap window class                            */
static DWORD last_key;      /* Last pressed key - used to suppress autorepeat */
static HANDLE KbdAck;       /* Keyboard acknowledge event                     */
static HANDLE MouseAck;     /* Mouse acknowledge event                        */

/* Virtual hardware registers */
static volatile struct GDI_Control gdictl;

/****************************************************************************************/

static ULONG SendKbdIRQ(UINT msg, DWORD key)
{
    DKBD(printf("[GDI] Keyboard event 0x%04lX key 0x%04lX\n", msg, key));

    /*
     * Lock until AROS have read the previous event.
     * This should fix swallowing or duplicating events, which happens because
     * of race condition between this thread and AROS:
     * 1. We signal a keypress.
     * 2. AROS gets a signal and wakes up. But has no time to read the event, Windows context switches.
     * 3. We get one more keypress, send another event.
     * 4. AROS wakes up again, reads event. Note that this is already another event (3).
     * 5. AROS sees one more event, and reads it again. Result: first keypress lost, second duplicated.
     * Note that this event is set to signalled state during creation.
     *
     * Be careful! Doing this here means AROS MUST read events and reset locks. Otherwise we'll deadlock
     * here upon the second event. Never dispose a keyboard or mouse HIDD objects, for example. There's
     * no need to, but who knows...
     *
     * In case of futrher problems a FIFO event queue needs to be implemented here.
     */
    WaitForSingleObject(KbdAck, INFINITE);

    gdictl.KbdEvent = msg;
    gdictl.KeyCode  = key;

    return KrnCauseIRQ(gdictl.KbdIrq);
}

/* We have to use this weird hook technique because there's no other way to prevent "Win" keys
   from opening theif stupid "Start menu". */
LRESULT CALLBACK key_callback(int code, WPARAM wp, KBDLLHOOKSTRUCT *lp)
{
    DWORD key;

    if ((code == HC_ACTION) && window_active)
    {
        wp &= 0xFFFFFFFB; /* This masks out difference between WM_SYSKEY* and WM_KEY* */
        key = (lp->scanCode & 0xFF) | ((lp->flags & LLKHF_EXTENDED) << 8);

        /*
         * Here we get raw keypresses, including autorepeats. We have to sort out
         * autorepeats in some smart way.
         */
        switch(wp)
        {
        case WM_KEYDOWN:
            if (key == last_key)
                return 1;
            last_key = key;
            break;

        case WM_KEYUP:
            if (key == last_key)
                last_key = 0;
        }
        SendKbdIRQ(wp, key);
        return 1;
    }
    return CallNextHookEx(NULL, code, wp, (LPARAM)lp);
}

LRESULT CALLBACK display_callback(HWND win, UINT msg, WPARAM wp, LPARAM lp)
{
    struct gfx_data *gdata;

    switch(msg)
    {
    case WM_SETCURSOR:
        gdata = (struct gfx_data *)GetWindowLongPtr(win, GWLP_USERDATA);

        if (gdata->cursor)
        {
            SetCursor(gdata->cursor);
            return 0;
        }
        break;

    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEWHEEL:
        if (win == window_active)
        {
            DMSE(printf("[GDI] Mouse event 0x%04X, window 0x%p\n", msg, win));

            /* Lock to prevent event loss. See SendKbdIRQ() above for explanation. */
            WaitForSingleObject(MouseAck, INFINITE);

            gdictl.MouseEvent = msg;
            gdictl.MouseX = GET_X_LPARAM(lp);
            gdictl.MouseY = GET_Y_LPARAM(lp);
            gdictl.Buttons = wp & 0x0000FFFF;
            gdictl.WheelDelta = wp >> 16;
            KrnCauseIRQ(gdictl.MouseIrq);
        }
        return 0;

#ifndef __x86_64__
    /*
     * This keyboard-related fragment is not used on Windows NT because keyboard hook
     * intercepts all keyboard messages. It is left here for Windows 9x.
     */
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (lp & 0x40000000) /* Ignore autorepeats */
            return 0;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        SendKbdIRQ(msg & 0xFFFFFFFB, (lp >> 16) & 0x000001FF);
        return 0;
#endif

    case WM_ACTIVATE:
        DACT(printf("[GDI] WM_ACTIVATE, Window 0x%p, wParam 0x%08lX\n", win, wp));
        /* In some cases Windows can activate an iconified window (for example if we minimize it
           by clicking its button on the taskbar). We process deactivation messages regardless of
           minimized state, but we handle activation only when it's done on a non-minimized window.
           This behavior was discovered by trial and error, i hope it's really ok now. */
        if ((wp & 0x0000FFFF) != WA_INACTIVE)
        {
            if (!(wp & 0xFFFF0000))
            {
                window_active = win;
                gdictl.Active = (void *)GetWindowLongPtr(win, GWLP_USERDATA);
                KrnCauseIRQ(gdictl.GfxIrq);
            }
        }
        else
        {
            window_active = NULL;

            /* Send WM_KEYUP in order to prevent "stuck keys" phenomena */
            if (last_key)
            {
                SendKbdIRQ(WM_KEYUP, last_key);
                last_key = 0;
            }
        }
        break;
    }
    return DefWindowProc(win, msg, wp, lp);
}

LRESULT CALLBACK bitmap_callback(HWND win, UINT msg, WPARAM wp, LPARAM lp)
{
    HDC window_dc;
    PAINTSTRUCT ps;
    LONG x, y, xsize, ysize;
    struct bitmap_data *bmdata;

    switch(msg)
    {
    case WM_PAINT:
        bmdata = (struct bitmap_data *)GetWindowLongPtr(win, GWLP_USERDATA);
        window_dc = BeginPaint(win, &ps);
        x = ps.rcPaint.left;
        y = ps.rcPaint.top;
        xsize = ps.rcPaint.right - ps.rcPaint.left + 1;
        ysize = ps.rcPaint.bottom - ps.rcPaint.top + 1;
        BitBlt(window_dc, x, y, xsize, ysize, bmdata->dc, x, y, SRCCOPY);
        EndPaint(win, &ps);
        return 0;

    default:
        return DefWindowProc(win, msg, wp, lp);
    }
}

DWORD WINAPI gdithread_entry(struct GDI_Control *ctl)
{
    HHOOK keyhook;
    BOOL res;
    MSG msg;
    struct gfx_data *gdata;
    struct bitmap_data *bmdata;
    LONG width, height;
    HWND prev;

    ctl->KbdEvent   = 0; /* Just in case... */
    ctl->MouseEvent = 0;

    keyhook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)key_callback, display_class_desc.hInstance, 0);

    do
    {
        res = GetMessage(&msg, NULL, 0, 0);
        D(printf("[GDI] GetMessage returned %ld\n", res));

        if (res > 0)
        {
            D(printf("[GDI] Message %lu for window 0x%p\n", msg.message, msg.hwnd));

            switch (msg.message)
            {
            case NOTY_SHOW:
                gdata = (struct gfx_data *)msg.wParam;
                DWIN(printf("[GDI] NOTY_SHOW, Display data: 0x%p\n", gdata));
                width = 0;
                height = 0;
                prev  = HWND_TOP;

                /* Traverse through the bitmaps list and (re)open every bitmap's window. This will
                   cause rearranging them in the correct Z-order */
                for (bmdata = (struct bitmap_data *)gdata->bitmaps.mlh_Head;
                     bmdata->node.mln_Succ; bmdata = (struct bitmap_data *)bmdata->node.mln_Succ)
                {

                    /* Display window dimensions based on its size will be calculated only for the
                       first (frontmost) bitmap */
                    if (!width)
                    {
                        width  = GetSystemMetrics(SM_CXFIXEDFRAME) * 2 + bmdata->win_width;
                        height = GetSystemMetrics(SM_CYFIXEDFRAME) * 2 + bmdata->win_height + GetSystemMetrics(SM_CYCAPTION);

                        DWIN(printf("[GDI] Display window: 0x%p\n", gdata->fbwin));
                        if (gdata->fbwin)
                        {
                            DWIN(printf("[GDI] Resizing display...\n"));
                            SetWindowPos(gdata->fbwin, 0, 0, 0, width, height, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
                        }
                    }

                    /* Open display window if we don't have it. Try every time, for sure */
                    if (!gdata->fbwin)
                    {
                        DWIN(printf("[GDI] Opening display...\n"));

                        /*
                         * We create the window as invisible, and show it only after setting gdata pointer.
                         * Otherwise we miss activation event (since gdata is NULL)
                         */
                        gdata->fbwin = CreateWindow((LPCSTR)display_class, "AROS Screen", WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
                                                     CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL,
                                                     display_class_desc.hInstance, NULL);
                        DWIN(printf("[GDI] Opened display window: 0x%p\n", gdata->fbwin));
                        if (gdata->fbwin)
                        {
                            SetWindowLongPtr(gdata->fbwin, GWLP_USERDATA, (LONG_PTR)gdata);
                            ShowWindow(gdata->fbwin, SW_SHOW);
                        }
                    }

                    /* Open bitmap window only if we really have display window */
                    if (gdata->fbwin)
                    {
                        DWIN(printf("[GDI] Showing bitmap data 0x%p, window 0x%p\n", bmdata, bmdata->window));

                        /*
                         * WS_DISABLED here causes forwarding all input to the parent window (i. e. display window).
                         * In future we may need some more sophisticated input handling because current driver architecture
                         * allows further transformation to rootless mode where every screen will have its own separate window
                         * on a Windows desktop.
                         */
                        if (!bmdata->window)
                        {
                            bmdata->window = CreateWindow((LPCSTR)bitmap_class, NULL, WS_BORDER|WS_CHILD|WS_CLIPSIBLINGS|WS_DISABLED, bmdata->bm_left - 1, bmdata->bm_top - 1, bmdata->bm_width + 2, bmdata->bm_height + 2,
                                                           gdata->fbwin, NULL, display_class_desc.hInstance, NULL);
                            DWIN(printf("[GDI] Opened bitmap window: 0x%p\n", bmdata->window));
                        }

                        if (bmdata->window)
                        {
                            SetWindowLongPtr(bmdata->window, GWLP_USERDATA, (LONG_PTR)bmdata);
                            /*
                             * We actually show the window only now, because otherwise it will pop up in front of all windows, and then
                             * immediately jump backwards, causing irritating flicker
                             */
                            SetWindowPos(bmdata->window, prev, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
                            UpdateWindow(bmdata->window);

                            prev = bmdata->window;
                        }
                    }
                }

                /* If we got no bitmaps, close the display window */
                if ((prev == HWND_TOP) && gdata->fbwin)
                {
                    DestroyWindow(gdata->fbwin);
                    gdata->fbwin = NULL;
                }

                ctl->ShowDone = TRUE;
                KrnCauseIRQ(ctl->GfxIrq);
                break;

            default:
                DispatchMessage(&msg);
                break;
            }
        }
    } while (res > 0);

    /* TODO: Further cleanup (close windows etc) */

    if (keyhook)
        UnhookWindowsHookEx(keyhook);

    return 0;
}

/****************************************************************************************/

volatile struct GDI_Control *__declspec(dllexport) __aros GDI_Init(void)
{
    long irq;
    HANDLE th;

    irq = KrnAllocIRQ();
    if (irq != -1)
    {
        gdictl.GfxIrq = irq;
        irq = KrnAllocIRQ();
        if (irq != -1)
        {
            gdictl.KbdIrq = irq;
            irq = KrnAllocIRQ();
            if (irq != -1)
            {
                gdictl.MouseIrq = irq;

                display_class_desc.hInstance = GetModuleHandle(NULL);
                display_class_desc.hIcon     = LoadIcon(display_class_desc.hInstance, MAKEINTRESOURCE(101));
                display_class_desc.hCursor   = LoadCursor(NULL, IDC_ARROW);
                display_class = RegisterClass(&display_class_desc);
                D(printf("[GDI] Created display window class 0x%04x\n", display_class));

                if (display_class)
                {
                    bitmap_class_desc.hInstance = display_class_desc.hInstance;
                    bitmap_class = RegisterClass(&bitmap_class_desc);
                    D(printf("[GDI] Created bitmap window class 0x%04x\n", bitmap_class));

                    if (bitmap_class)
                    {
                        KbdAck = CreateEvent(NULL, FALSE, TRUE, NULL);
                        if (KbdAck)
                        {
                            MouseAck = CreateEvent(NULL, FALSE, TRUE, NULL);
                            if (MouseAck)
                            {
                                gdictl.ShowDone = FALSE;
                                gdictl.Active   = NULL;

                                window_active = NULL;
                                last_key = 0;
                                th = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)gdithread_entry, (void *)&gdictl, 0, &thread_id);

                                D(printf("[GDI] Started thread 0x%p ID 0x%08lX\n", th, thread_id));
                                if (th)
                                {
                                    CloseHandle(th);
                                    return &gdictl;
                                }
                                CloseHandle(MouseAck);
                            }
                            CloseHandle(KbdAck);
                        }
                        UnregisterClass((LPCSTR)bitmap_class, bitmap_class_desc.hInstance);
                    }
                    UnregisterClass((LPCSTR)display_class, display_class_desc.hInstance);
                }
                KrnFreeIRQ(gdictl.MouseIrq);
            }
            KrnFreeIRQ(gdictl.KbdIrq);
        }
        KrnFreeIRQ(gdictl.GfxIrq);
    }
    return NULL;
}

void __declspec(dllexport) __aros GDI_Shutdown(struct GDI_Control *ctl)
{
    PostThreadMessage(thread_id, WM_QUIT, 0, 0);
    UnregisterClass((LPCSTR)bitmap_class, bitmap_class_desc.hInstance);
    UnregisterClass((LPCSTR)display_class, display_class_desc.hInstance);
    CloseHandle(MouseAck);
    CloseHandle(KbdAck);
    KrnFreeIRQ(ctl->MouseIrq);
    KrnFreeIRQ(ctl->KbdIrq);
    KrnFreeIRQ(ctl->GfxIrq);
}

/* I'm too lazy to import one more .dll (kernel32). I'd better write these stubs. */
ULONG __declspec(dllexport) __aros GDI_PutMsg(void *window, UINT msg, WPARAM wp, LPARAM lp)
{
    if (window)
        return PostMessage(window, msg, wp, lp);
    else
        return PostThreadMessage(thread_id, msg, wp, lp);
}

ULONG __declspec(dllexport) __aros GDI_KbdAck(void)
{
    return SetEvent(KbdAck);
}

ULONG __declspec(dllexport) __aros GDI_MouseAck(void)
{
    return SetEvent(MouseAck);
}
