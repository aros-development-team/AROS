/*
    Copyright  1995-2008, The AROS Development Team. All rights reserved.
    $Id: gdi.c 27757 2008-01-26 15:05:40Z verhaegs $

    Desc: Host-side part of GDI hidd. Handles windows and receives events.
    Lang: English.
*/

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <aros/kernel_host.h>
#include "gdi.h"

#define D(x)

static DWORD thread_id;
__declspec(dllexport) struct MouseData GDI_MouseData;

/****************************************************************************************/

LRESULT CALLBACK window_callback(HWND win, UINT msg, WPARAM wp, LPARAM lp)
{
    HDC bitmap_dc, window_dc;
    PAINTSTRUCT ps;
    LONG x, y, xsize, ysize;

    switch(msg) {
    case WM_PAINT:
        bitmap_dc = (HDC)GetWindowLongPtr(win, GWLP_USERDATA);
        window_dc = BeginPaint(win, &ps);
        x = ps.rcPaint.left;
        y = ps.rcPaint.top;
        xsize = ps.rcPaint.right - ps.rcPaint.left + 1;
        ysize = ps.rcPaint.bottom - ps.rcPaint.top + 1;
        BitBlt(window_dc, x, y, xsize, ysize, bitmap_dc, x, y, SRCCOPY);
        EndPaint(win, &ps);
        return 0;
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEWHEEL:
        GDI_MouseData.EventCode = msg;
        GDI_MouseData.MouseX = GET_X_LPARAM(lp);
    	GDI_MouseData.MouseY = GET_Y_LPARAM(lp);
    	GDI_MouseData.Buttons = wp & 0x0000FFFF;
    	GDI_MouseData.WheelDelta = wp >> 16;
        CauseException(3);
    	return 0;
    default:
        return DefWindowProc(win, msg, wp, lp);
    }
}

/****************************************************************************************/

DWORD WINAPI gdithread_entry(LPVOID p)
{
    BOOL res;
    MSG msg;
    ATOM wcl;
    WINDOWPLACEMENT wpos;
    WNDCLASS wcl_desc = {
        CS_NOCLOSE,
        window_callback,
        0,
        0,
        NULL,
        NULL,
        NULL,
        COLOR_WINDOW,
        NULL,
        "AROS_Screen"
    };
    struct gfx_data *gdata;
    LONG width, height;

    wcl_desc.hInstance = GetModuleHandle(NULL);
    wcl_desc.hIcon = LoadIcon(wcl_desc.hInstance, MAKEINTRESOURCE(101));
    wcl_desc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcl = RegisterClass(&wcl_desc);
    D(printf("[GDI] Created window class 0x%p\n", wcl));
    if (wcl) {
    	do {
            res = GetMessage(&msg, NULL, 0, 0);
            D(printf("[GDI] GetMessage returned %ld\n", res));
            if (res > 0) {
            	D(printf("[GDI] Got message %lu\n", msg.message));
            	switch (msg.message) {
            	case NOTY_SHOW:
            	    gdata = (struct gfx_data *)msg.wParam;
            	    if (gdata->bitmap) {
            	        width = GetSystemMetrics(SM_CXFIXEDFRAME) * 2 + gdata->width;
            	        height = GetSystemMetrics(SM_CYFIXEDFRAME) * 2 + gdata->height + GetSystemMetrics(SM_CYCAPTION);
            	    	if (!gdata->fbwin) {
            	    	    gdata->fbwin = CreateWindow(wcl, "AROS Screen", WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
						        CW_USEDEFAULT, CW_USEDEFAULT, width,  height, NULL, NULL,
						        wcl_desc.hInstance, NULL);
            	    	    ShowWindow(gdata->fbwin, SW_SHOW);
            	        } else {
            	            wpos.length = sizeof(wpos);
		            if (GetWindowPlacement(msg.hwnd, &wpos)) {
            			wpos.rcNormalPosition.right = wpos.rcNormalPosition.left + width;
	            	    	wpos.rcNormalPosition.bottom = wpos.rcNormalPosition.top + height;
	            	    	SetWindowPlacement(msg.hwnd, &wpos);
	            	    }
	            	}
	            	if (gdata->fbwin) {
	            	    SetWindowLongPtr(gdata->fbwin, GWLP_USERDATA, gdata->bitmap_dc);
            	            RedrawWindow(gdata->fbwin, NULL, NULL, RDW_INVALIDATE|RDW_UPDATENOW);
            	        }
            	    } else {
            	        if (gdata->fbwin) {
            	            DestroyWindow(gdata->fbwin);
            	            gdata->fbwin = NULL;
            	        }
            	    }
            	    CauseException(2);
            	    break;
            	default:
            	    DispatchMessage(&msg);
            	}
	    }
        } while (res > 0);
        UnregisterClass(wcl, wcl_desc.hInstance);
    }
}

/****************************************************************************************/

ULONG __declspec(dllexport) GDI_Init(void)
{
    HANDLE th;
    
    th = CreateThread(NULL, 0, gdithread_entry, NULL, 0, &thread_id);
    D(printf("[GDI] Started thread 0x%p ID 0x%08lX\n", th, thread_id));
    if (th)
        CloseHandle(th);
    return th ? 1 : 0;
}

/****************************************************************************************/

ULONG __declspec(dllexport) GDI_PutMsg(void *window, UINT msg, WPARAM wp, LPARAM lp)
{
    if (window)
    	return PostMessage(window, msg, wp, lp);
    else
        return PostThreadMessage(thread_id, msg, wp, lp);
}
