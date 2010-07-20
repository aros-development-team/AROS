/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Host-side part of GDI hidd. Handles windows and receives events.
    Lang: English.
*/

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <aros/kernel_host.h>
#include "gdi.h"
#include "bitmap.h"

#define D(x)
#define DKBD(x)
#define DWIN(x)

LRESULT CALLBACK display_callback(HWND win, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK bitmap_callback(HWND win, UINT msg, WPARAM wp, LPARAM lp);

/* Global variables shared by all windows (theoretical)	*/

static WNDCLASS display_class_desc = {
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
static WNDCLASS bitmap_class_desc = {
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
static DWORD thread_id;	    /* Window service thread ID			      */
static BOOL window_active;  /* Flag - AROS window is active		      */
static ULONG display_class; /* Display window class		   	      */
static ULONG bitmap_class;  /* Bitmap window class			      */
static DWORD last_key;      /* Last pressed key - used to suppress autorepeat */

/* Virtual hardware registers */
static struct GDI_Control gdictl;

/****************************************************************************************/

static ULONG SendKbdIRQ(UINT msg, DWORD key)
{
    DKBD(printf("[GDI] Keyboard event 0x%04lX key 0x%04lX\n", msg, key));
    gdictl.KbdEvent = msg;
    gdictl.KeyCode = key;

    return KrnCauseIRQ(gdictl.KbdIrq);
}

/* We have to use this weird hook technique because there's no other way to prevent "Win" keys
   from opening theif stupid "Start menu". */
LRESULT CALLBACK key_callback(int code, WPARAM wp, KBDLLHOOKSTRUCT *lp)
{
    DWORD key;

    if ((code == HC_ACTION) && window_active) {
    	wp &= 0xFFFFFFFB; /* This masks out difference between WM_SYSKEY* and WM_KEY* */
    	key = (lp->scanCode & 0xFF) | ((lp->flags & LLKHF_EXTENDED) << 8);
    	/* Here we get raw keypresses, including autorepeats. We have to sort out autorepeats
    	   in some smart way. */
    	switch(wp) {
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

    switch(msg) {
    case WM_SETCURSOR:
        gdata = (struct gfx_data *)GetWindowLongPtr(win, GWLP_USERDATA);

	if (gdata->cursor) {
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
        gdictl.MouseEvent = msg;
        gdictl.MouseX = GET_X_LPARAM(lp);
    	gdictl.MouseY = GET_Y_LPARAM(lp);
    	gdictl.Buttons = wp & 0x0000FFFF;
    	gdictl.WheelDelta = wp >> 16;
        KrnCauseIRQ(gdictl.MouseIrq);
    	return 0;
/* This keyboard-related fragment is not used on Windows NT because keyboard hook intercepts all keyboard messages.
   It is left here for Windows 9x. */
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (lp & 0x40000000) /* Ignore autorepeats */
            return 0;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        SendKbdIRQ(msg & 0xFFFFFFFB, (lp >> 16) & 0x000001FF);
        return 0;
    case WM_ACTIVATE:
        DWIN(printf("[GDI] WM_ACTIVATE, wParam is 0x%08lX\n", wp));
        /* In some cases Windows can activate an iconified window (for example if we minimize it
           by clicking its button on the taskbar). We process deactivation messages regardless of
           minimized state, but we handle activation only when it's done on a non-minimized window.
           This behavior was discovered by trial and error, i hope it's really ok now. */
    	if ((wp & 0x0000FFFF) != WA_INACTIVE) {
            if (!(wp & 0xFFFF0000))
                window_active = TRUE;
        } else {
            window_active = FALSE;
	    /* Send WM_KEYUP in order to prevent "stuck keys" phenomena */
	    if (last_key) {
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
    LONG bm_xend, bm_yend;
    struct bitmap_data *bmdata;

    switch(msg) {
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

    keyhook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)key_callback, display_class_desc.hInstance, 0);
    do {
        res = GetMessage(&msg, NULL, 0, 0);
        D(printf("[GDI] GetMessage returned %ld\n", res));
        if (res > 0) {
            D(printf("[GDI] Message %lu for window 0x%p\n", msg.message, msg.hwnd));
            switch (msg.message) {
            case NOTY_SHOW:
                gdata = (struct gfx_data *)msg.wParam;
		bmdata = (struct bitmap_data *)msg.lParam;
		DWIN(printf("[GDI] NOTY_SHOW, Display data: 0x%p, Bitmap data: 0x%p\n"));

		/* We MUST get a bitmap */
            	width = GetSystemMetrics(SM_CXFIXEDFRAME) * 2 + bmdata->win_width;
            	height = GetSystemMetrics(SM_CYFIXEDFRAME) * 2 + bmdata->win_height + GetSystemMetrics(SM_CYCAPTION);
		/* Do we already have a window? */
            	if (!gdata->fbwin) {
		    /* Create it if we don't */
		    DWIN(printf("[GDI] Opening display...\n"));
            	    gdata->fbwin = CreateWindow((LPCSTR)display_class, "AROS Screen", WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_VISIBLE,
					        CW_USEDEFAULT, CW_USEDEFAULT, width,  height, NULL, NULL,
						display_class_desc.hInstance, NULL);
		    if (gdata->fbwin)
		        SetWindowLongPtr(gdata->fbwin, GWLP_USERDATA, (LONG_PTR)gdata);
            	} else if ((struct bitmap_data *)gdata->bitmaps.mlh_Head == bmdata) {
		    /* If the displayed bitmap is the frontmost one, adjust window size */
		    DWIN(printf("[GDI] Resizing display...\n"));
		    SetWindowPos(gdata->fbwin, 0, 0, 0, width, height, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
	        }
		DWIN(printf("[GDI] Display window: 0x%p\n", gdata->fbwin));
	        if (gdata->fbwin) {
		    /* WS_DISABLED here causes forwarding all input to the parent window (i. e. display window).
		       In future we may need some more sophisticated input handling because current driver architecture
		       allows further transformation to rootless mode where every screen will have its own separate window
		       on a Windows desktop. */
		    if (!bmdata->window)
		        bmdata->window = CreateWindow((LPCSTR)bitmap_class, NULL, WS_BORDER|WS_CHILD|WS_CLIPSIBLINGS|WS_DISABLED, bmdata->bm_left - 1, bmdata->bm_top - 1, bmdata->bm_width + 2, bmdata->bm_height + 2,
						      gdata->fbwin, NULL, display_class_desc.hInstance, NULL);
		    DWIN(printf("[GDI] Bitmap window: 0x%p\n", bmdata->window));
		    if (bmdata->window) {
			/* Find out previous window */
		        struct bitmap_data *prev_bitmap = (struct bitmap_data *)bmdata->node.mln_Pred;
			HWND prev_win = prev_bitmap->node.mln_Pred ? prev_bitmap->window : HWND_TOP;
			
		        SetWindowLongPtr(bmdata->window, GWLP_USERDATA, (LONG_PTR)bmdata);
		        /* We actually show the window only now, because otherwise it will pop up in front of all windows, and then
		           immediately jump backwards, causing irritating flicker */
		        SetWindowPos(bmdata->window, prev_win, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
		        UpdateWindow(bmdata->window);
		    }
            	}
            	KrnCauseIRQ(ctl->GfxIrq);
            	break;
            default:
            	DispatchMessage(&msg);
            }
	}
    } while (res > 0);
    
    /* TODO: Further cleanup (close windows etc) */

    if (keyhook)
	UnhookWindowsHookEx(keyhook);

    return 0;
}

/****************************************************************************************/

struct GDI_Control *__declspec(dllexport) GDI_Init(void)
{
    long irq;
    HANDLE th;

    irq = KrnAllocIRQ();
    if (irq != -1) {
        gdictl.GfxIrq = irq;
        irq = KrnAllocIRQ();
	if (irq != -1) {
	    gdictl.KbdIrq = irq;
	    irq = KrnAllocIRQ();
	    if (irq != -1) {
	        gdictl.MouseIrq = irq;

		display_class_desc.hInstance = GetModuleHandle(NULL);
		display_class_desc.hIcon = LoadIcon(display_class_desc.hInstance, MAKEINTRESOURCE(101));
		display_class_desc.hCursor = LoadCursor(NULL, IDC_ARROW);
		display_class = RegisterClass(&display_class_desc);
		D(printf("[GDI] Created display window class 0x%04x\n", display_class));
		if (display_class) {
		    bitmap_class_desc.hInstance = display_class_desc.hInstance;
		    bitmap_class = RegisterClass(&bitmap_class_desc);
		    D(printf("[GDI] Created bitmap window class 0x%04x\n", bitmap_class));
		    if (bitmap_class) {
		
		        window_active = FALSE;
		        last_key = 0;
		        th = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)gdithread_entry, &gdictl, 0, &thread_id);
		        D(printf("[GDI] Started thread 0x%p ID 0x%08lX\n", th, thread_id));
		        if (th) {
			    CloseHandle(th);
			    return &gdictl;
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

void __declspec(dllexport) GDI_Shutdown(struct GDI_Control *ctl)
{
    PostThreadMessage(thread_id, WM_QUIT, 0, 0);
    UnregisterClass((LPCSTR)display_class, display_class_desc.hInstance);
    KrnFreeIRQ(ctl->MouseIrq);
    KrnFreeIRQ(ctl->KbdIrq);
    KrnFreeIRQ(ctl->GfxIrq);
}

ULONG __declspec(dllexport) GDI_PutMsg(void *window, UINT msg, WPARAM wp, LPARAM lp)
{
    if (window)
    	return PostMessage(window, msg, wp, lp);
    else
        return PostThreadMessage(thread_id, msg, wp, lp);
}
