#ifndef GDI_HOSTLIB_H
#define GDI_HOSTLIB_H

#include <aros/config.h>

struct gdi_func {
    __attribute__((stdcall)) APTR (*CreateDC)(STRPTR lpszDriver, STRPTR lpszDevice, STRPTR lpszOutput, APTR lpInitData);
    __attribute__((stdcall)) APTR (*CreateCompatibleDC)(APTR hdc);
    __attribute__((stdcall)) LONG (*DeleteDC)(APTR hdc);
    __attribute__((stdcall)) LONG (*GetDeviceCaps)(APTR hdc, LONG nIndex);
    __attribute__((stdcall)) APTR (*CreateCompatibleBitmap)(APTR hdc, ULONG nWidth, ULONG nHeight);
    __attribute__((stdcall)) APTR (*CreateSolidBrush)(ULONG crColor);
    __attribute__((stdcall)) LONG (*DeleteObject)(APTR hObject);
    __attribute__((stdcall)) APTR (*SelectObject)(APTR hdc, APTR hgdiobj);
    __attribute__((stdcall)) LONG (*BitBlt)(APTR hdcDest, LONG nXDest, LONG nYDest, LONG nWidth, LONG nHeight, APTR hdcSrc,
    					    LONG nXSrc, LONG nYSrc, ULONG dwRop);
    __attribute__((stdcall)) LONG (*PatBlt)(APTR hdc, LONG nXLeft, LONG nYLeft, LONG nWidth, LONG nHeight, ULONG dwRop);
    __attribute__((stdcall)) ULONG (*GetPixel)(APTR hdc, LONG nXPos, LONG nYPos);
    __attribute__((stdcall)) ULONG (*SetPixel)(APTR hdc, LONG X, LONG Y, ULONG crColor);
};

struct user_func {
    __attribute__((stdcall)) ULONG (*RedrawWindow)(APTR hWnd, CONST RECT* lpRect, APTR hrgnUpdate, ULONG flags);
};

struct native_func {
    ULONG (*GDI_Init)(void);
    ULONG (*GDI_PutMsg)(void *win, ULONG msg, IPTR wp, IPTR lp);
};

extern void *gdi_handle;
extern struct gdi_func *gdi_func;

extern void *user_handle;
extern struct user_func *user_func;

extern void *native_handle;
extern struct native_func *native_func;

#define GDI_SOFILE  "Gdi32.dll"
#define USER_SOFILE "User32.dll"
#define NATIVE_SOFILE "Libs\\Host\\wingdi.dll"

#define GDICALL(func,...) (gdi_func->func(__VA_ARGS__))
#define USERCALL(func,...) (user_func->func(__VA_ARGS__))
#define NATIVECALL(func,...) (native_func->func(__VA_ARGS__))

#endif
