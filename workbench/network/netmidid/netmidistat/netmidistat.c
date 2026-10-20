/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: prints what the network MIDI service says about its ports.
*/

#include <proto/exec.h>
#include <exec/memory.h>

#include <stdio.h>
#include <string.h>

#include "netmidi_status.h"

int main(void)
{
    static struct NetMidiStatus copy;
    struct NetMidiStatus *status;
    int found = 0;
    int port;

    Forbid();
    status = (struct NetMidiStatus *)FindSemaphore((STRPTR)NETMIDI_STATUS_NAME);
    if (status && status->ns_Version == NETMIDI_STATUS_VERSION) {
        /* shared, and obtained before task switching is allowed again: the
           service cannot take it away meanwhile */
        ObtainSemaphoreShared(&status->ns_Semaphore);
        Permit();
        memcpy(&copy, status, sizeof(copy));
        ReleaseSemaphore(&status->ns_Semaphore);
        found = 1;
    } else {
        Permit();
    }
    if (!found) {
        puts(status ? "The network MIDI service is of another version than this command."
                    : "The network MIDI service is not running.");
        return 5;
    }
    printf("session \"%s\", %lu invitations refused\n", copy.ns_Session,
           (unsigned long)copy.ns_Refused);
    for (port = 0; port < NETMIDI_STATUS_PORTS; ++port) {
        const struct NetMidiPortStatus *p = &copy.ns_Port[port];

        if (!p->ps_Present)
            continue;
        printf("%s.%d \"%s\": in %lu messages in %lu packets, %lu lost; "
               "out %lu messages in %lu packets; timed %lu, late %lu; "
               "SysEx dropped %lu; journals omitted %lu; undefined %lu\n",
               port < 16 ? "applemidi" : "networkmidi2", port % 16,
               p->ps_Name, (unsigned long)p->ps_MessagesIn,
               (unsigned long)p->ps_PacketsIn, (unsigned long)p->ps_PacketsLost,
               (unsigned long)p->ps_MessagesOut,
               (unsigned long)p->ps_PacketsOut, (unsigned long)p->ps_Timed,
               (unsigned long)p->ps_Late, (unsigned long)p->ps_SysExDropped,
               (unsigned long)p->ps_JournalsOmitted,
               (unsigned long)p->ps_Undefined);
    }
    return 0;
}
