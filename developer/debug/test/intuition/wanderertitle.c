/* Copyright (C) 2026, The AROS Development Team. All rights reserved.
 * Run on an idle Wanderer desktop. Do not close the test window manually.
 * A drawer title must survive disposal of its WBHM_TYPE_OPEN message.
 * The window and empty RAM drawer are left for visual inspection; close
 * the window and delete the printed path after the test.
 */
#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/workbench.h>
#include <stdio.h>
#include <string.h>
#include "../dos/test.h"
#define BLOCKS 2048
static char path[96];
static struct Screen *screen;
static struct Window *window;
static struct Window *previous[64];
static unsigned previous_count;
static APTR blocks[BLOCKS];
static BOOL opened, created;
static struct Window *find_window(BOOL new_window)
{
    struct Window *w, *found = NULL;
    ULONG lock = LockIBase(0);
    for (w = screen->FirstWindow; w; w = w->NextWindow)
    {
        unsigned i;
        if (!new_window)
        {
            if (w == window) { found = w; break; }
            continue;
        }
        for (i = 0; i < previous_count && previous[i] != w; i++);
        if (i == previous_count) { found = w; break; }
    }
    UnlockIBase(lock);
    return found;
}
static BOOL title_matches(void)
{
    struct Window *w;
    BOOL matches = FALSE;
    ULONG lock = LockIBase(0);
    for (w = screen->FirstWindow; w; w = w->NextWindow)
        if (w == window)
        { matches = w->Title && strncmp((char *)w->Title, path, strlen(path) + 1) == 0; break; }
    UnlockIBase(lock);
    return matches;
}
void cleanup(void)
{
    int i;
    for (i = 0; i < BLOCKS; i++) if (blocks[i]) FreeVec(blocks[i]);
    if (screen) UnlockPubScreen(NULL, screen);
    if (created) printf("Close the test drawer and delete %s when finished.\n", path);
}
int main(void)
{
    BPTR dir;
    int i;
    LONG width, height;
    struct Window *w;
    ULONG lock;
    snprintf(path, sizeof(path), "RAM:WandererTitle-%lx", (unsigned long)(IPTR)FindTask(NULL));
    dir = Lock(path, SHARED_LOCK);
    if (dir) { UnLock(dir); puts("Test directory already exists"); return FAIL; }
    dir = CreateDir(path);
    TEST(dir != BNULL);
    created = TRUE; UnLock(dir);
    screen = LockPubScreen("Workbench"); TEST(screen != NULL);
    lock = LockIBase(0);
    for (w = screen->FirstWindow; w && previous_count < 64; w = w->NextWindow)
        previous[previous_count++] = w;
    UnlockIBase(lock);
    TEST(w == NULL);
    opened = OpenWorkbenchObjectA(path, NULL); TEST(opened);
    for (i = 0; i < 100 && !(window = find_window(TRUE)); i++) Delay(5);
    TEST(window != NULL);
    width = window->Width; height = window->Height;
    Delay(10);
    for (i = 0; i < BLOCKS; i++)
    {
        blocks[i] = AllocVec(strlen(path) + 1, MEMF_PUBLIC);
        if (!blocks[i]) break;
        memset(blocks[i], 'X', strlen(path));
        ((char *)blocks[i])[strlen(path)] = 0;
    }
    printf("Allocated %d small blocks after opening the drawer\n", i);
    for (i = 0; i < 12; i++)
    {
        TEST(find_window(FALSE) != NULL);
        ChangeWindowBox(window, window->LeftEdge, window->TopEdge,
                        width + (i % 2 ? 0 : 24), height + (i % 2 ? 0 : 16));
        Delay(5); TEST(title_matches());
    }
    puts("PASS: drawer title survives message disposal, allocation reuse and resize");
    cleanup(); return OK;
}
