#ifndef GDI_HOSTLIB_H
#define GDI_HOSTLIB_H

struct gdi_func
{
    APTR  __stdcall (*CreateDC)(STRPTR lpszDriver, STRPTR lpszDevice, STRPTR lpszOutput, APTR lpInitData);
    APTR  __stdcall (*CreateCompatibleDC)(APTR hdc);
    LONG  __stdcall (*DeleteDC)(APTR hdc);
    LONG  __stdcall (*GetDeviceCaps)(APTR hdc, LONG nIndex);
    APTR  __stdcall (*CreateBitmapIndirect)(CONST BITMAP *lpbm);
    APTR  __stdcall (*CreateCompatibleBitmap)(APTR hdc, ULONG nWidth, ULONG nHeight);
    APTR  __stdcall (*CreateSolidBrush)(ULONG crColor);
    LONG  __stdcall (*DeleteObject)(APTR hObject);
    APTR  __stdcall (*SelectObject)(APTR hdc, APTR hgdiobj);
    ULONG __stdcall (*SetBkColor)(APTR hdc, ULONG crColor);
    ULONG __stdcall (*SetTextColor)(APTR hdc, ULONG crColor);
    ULONG __stdcall (*SetROP2)(APTR hdc, ULONG fnDrawMode);
    LONG  __stdcall (*BitBlt)(APTR hdcDest, LONG nXDest, LONG nYDest, LONG nWidth, LONG nHeight, APTR hdcSrc,
                                            LONG nXSrc, LONG nYSrc, ULONG dwRop);
    LONG  __stdcall (*PatBlt)(APTR hdc, LONG nXLeft, LONG nYLeft, LONG nWidth, LONG nHeight, ULONG dwRop);
    ULONG __stdcall (*GetPixel)(APTR hdc, LONG nXPos, LONG nYPos);
    ULONG __stdcall (*SetPixel)(APTR hdc, LONG X, LONG Y, ULONG crColor);
    LONG  __stdcall (*GetDIBits)(APTR hdc, APTR hbmp, ULONG uStartScan, ULONG cScanLines,
                                               void *lpvBits, BITMAPINFO *lpbi, ULONG uUsage);
    LONG  __stdcall (*StretchDIBits)(APTR hdc, LONG XDest, LONG YDest, LONG nDestWidth, LONG nDestHeight,
                                                   LONG XSrc, LONG YSrc, LONG nSrcWidth, LONG nSrcHeight,
                                                   const void *lpBits, const BITMAPINFO *lpBitsInfo, ULONG iUsage, ULONG dwRop);
};

struct user_func
{
    APTR  __stdcall (*CreateIconIndirect)(ICONINFO *piconinfo);
    ULONG __stdcall (*DestroyIcon)(APTR hIcon);
    APTR  __stdcall (*SetCursor)(APTR hCursor);
    ULONG __stdcall (*SetWindowPos)(APTR hWnd, APTR hWndInsertAfter, LONG X, LONG Y, LONG cx, LONG cy, ULONG uFlags);
    ULONG __stdcall (*RedrawWindow)(APTR hWnd, CONST RECT* lpRect, APTR hrgnUpdate, ULONG flags);
};

struct native_func
{
    struct GDI_Control *(*GDI_Init)(void);
    void  (*GDI_Shutdown)(struct GDI_Control *ctl);
    ULONG (*GDI_PutMsg)(void *win, ULONG msg, IPTR wp, IPTR lp);
    void  (*GDI_KbdAck)(void);
    void  (*GDI_MouseAck)(void);
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
