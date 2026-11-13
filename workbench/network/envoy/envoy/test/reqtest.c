/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: ReqTest - open the envoy.library requesters and print what they
          return. With ENV:Envoy/RequesterAutoClose set they close by
          themselves, which is how the hosted test run uses this.

    ReqTest HOST/S,LOGIN/S,USER/S,PASSWORD/S,ALL/S,NAME/K,WINDOW/S
    WINDOW opens a window and passes it as the requesters' Window and
    OptimWindow, as applications do.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/envoy.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <envoy/envoy.h>
#include <stdio.h>
#include <string.h>

static struct Window *Win;

static void RunHost(void)
{
    char host[128] = "";
    BOOL ok = HostRequest(HREQ_Buffer, (IPTR)host, HREQ_BuffSize, sizeof(host), HREQ_Title, (IPTR)"ReqTest: pick a host",
                          HREQ_Window, (IPTR)Win, HREQ_OptimWindow, (IPTR)Win, TAG_DONE);
    printf("HostRequest: %s, buffer '%s'\n", ok ? "TRUE" : "FALSE", host);
}

static void RunLogin(CONST_STRPTR name)
{
    char user[32] = "", pass[32] = "";
    BOOL ok = LoginRequest(LREQ_NameBuff, (IPTR)user, LREQ_NameBuffLen, sizeof(user),
                           LREQ_PassBuff, (IPTR)pass, LREQ_PassBuffLen, sizeof(pass),
                           name ? LREQ_UserName : TAG_IGNORE, (IPTR)name,
                           LREQ_Window, (IPTR)Win, LREQ_OptimWindow, (IPTR)Win, TAG_DONE);
    printf("LoginRequest: %s, name '%s', password '%s'\n", ok ? "TRUE" : "FALSE", user, pass);
}

static void RunUser(void)
{
    char user[32] = "?", group[32] = "?";
    BOOL ok = UserRequest(UGREQ_UserBuff, (IPTR)user, UGREQ_UserBuffLen, sizeof(user),
                          UGREQ_GroupBuff, (IPTR)group, UGREQ_GroupBuffLen, sizeof(group),
                          UGREQ_Window, (IPTR)Win, UGREQ_OptimWindow, (IPTR)Win, TAG_DONE);
    printf("UserRequest: %s, user '%s', group '%s'\n", ok ? "TRUE" : "FALSE", user, group);
}

static void RunPassword(void)
{
    char newpw[32] = "";
    BOOL ok = PasswordRequest(PWREQ_OldPassword, (IPTR)"secret", PWREQ_NewPWBuff, (IPTR)newpw,
                              PWREQ_NewPWBuffLen, sizeof(newpw), PWREQ_PWRequired, TRUE,
                              PWREQ_Window, (IPTR)Win, PWREQ_OptimWindow, (IPTR)Win, TAG_DONE);
    printf("PasswordRequest (old 'secret', required): %s, new password '%s'\n", ok ? "TRUE" : "FALSE", newpw);
    ok = PasswordRequest(PWREQ_NewPWBuff, (IPTR)newpw, PWREQ_NewPWBuffLen, sizeof(newpw), TAG_DONE);
    printf("PasswordRequest (no old, not required): %s, new password '%s'\n", ok ? "TRUE" : "FALSE", newpw);
}

int main(void)
{
    IPTR args[7] = { 0 };
    struct RDArgs *rda;

    if (!(rda = ReadArgs("HOST/S,LOGIN/S,USER/S,PASSWORD/S,ALL/S,NAME/K,WINDOW/S", args, NULL)))
    {
        PrintFault(IoErr(), "ReqTest");
        return RETURN_FAIL;
    }
    if (args[6])
        Win = OpenWindowTags(NULL, WA_Left, 40, WA_Top, 40, WA_Width, 420, WA_Height, 260,
                             WA_Title, (IPTR)"ReqTest caller window", WA_DragBar, TRUE, WA_DepthGadget, TRUE,
                             WA_Activate, TRUE, TAG_DONE);
    printf("envoy.library %d.%d%s\n", EnvoyBase->lib_Version, EnvoyBase->lib_Revision, Win ? ", with a caller window" : "");
    fflush(stdout);
    if (args[0] || args[4]) { RunHost(); fflush(stdout); }
    if (args[1] || args[4]) { RunLogin((CONST_STRPTR)args[5]); fflush(stdout); }
    if (args[2] || args[4]) { RunUser(); fflush(stdout); }
    if (args[3] || args[4]) { RunPassword(); fflush(stdout); }
    if (Win)
        CloseWindow(Win);
    FreeArgs(rda);
    return RETURN_OK;
}
