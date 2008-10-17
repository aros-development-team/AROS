#include <windows.h>
#include <aros/kernel_host.h>
#include <aros/hostthread.h>

#define D(x)

/* Host-side API */

void *__declspec(dllexport) GetMsg(struct ThreadHandle *th)
{
    MSG msg;
    BOOL res;

    D(printf("[HT host] GetMsg(0x%p)\n", th));
    for (;;) {
        res = GetMessage(&msg, NULL, WM_QUIT, WM_USER);
        D(printf("[HT host] GetMessage() returned %ld\n", res));
        switch (res) {
        case -1:
            return NULL;
        case 0:
            return (void *)-1;
        }
        if (msg.message == WM_USER) {
            D(printf("[HT host] Thread message 0x%p arrived\n", msg.wParam));
            return (void *)msg.wParam;
        }
    }
}       

void __declspec(dllexport) CauseInterrupt(struct ThreadHandle *th, void *data)
{
    D(printf("[HT host] Interrupt for thread 0x%p, data 0x%p\n", th, data));
    CauseIRQ(0, th, data);
}

DWORD WINAPI ThreadEntry(struct ThreadHandle *tn)
{
    return tn->entry(tn);
}

/* AROS-side API */

HANDLE __declspec(dllexport) CreateNewThread(struct ThreadHandle *tn)
{
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
