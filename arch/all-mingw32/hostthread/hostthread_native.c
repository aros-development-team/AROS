#include <windows.h>
#include <aros/kernel_host.h>
#include <aros/hostthread.h>

/* Host-side API */

void *__declspec(dllexport) GetMsg(struct ThreadHandle *th)
{
    MSG msg;
    BOOL res;

    for (;;) {
        res = GetMessage(&msg, NULL, WM_QUIT, WM_USER);
        switch (res) {
        case -1:
            return NULL;
        case 0:
            return (void *)-1;
        }
        if (msg.message == WM_USER)
            return (void *)msg.wParam;
    }
}       

void __declspec(dllexport) CauseInterrupt(struct ThreadHandle *th, void *data)
{
    CauseIRQ(0, th, data);
}

DWORD WINAPI ThreadEntry(struct ThreadHandle *tn)
{
    return tn->entry(tn);
}

/* AROS-side API */

HANDLE __declspec(dllexport) CreateNewThread(void *entry, struct ThreadHandle *tn)
{
    tn->entry = entry;
    tn->handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadEntry, tn, 0, &tn->id);
    return tn->handle;
}

BOOL __declspec(dllexport) KillThread(struct ThreadHandle *tn)
{
    return PostThreadMessage(tn->id, WM_QUIT, 0, 0);
}

BOOL __declspec(dllexport) PutThreadMsg(struct ThreadHandle *tn, void *msg)
{
    return PostThreadMessage(tn->id, WM_USER, (WPARAM)msg, 0);
}
