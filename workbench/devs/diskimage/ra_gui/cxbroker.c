/* Copyright 2007-2012 Fredrik Wikstrom. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
*/

#include "diskimagegui.h"
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/commodities.h>

struct Library *CxBase;
struct MsgPort *BrokerPort;
CxObj *Broker;

BOOL RegisterCxBroker (void) {
	struct NewBroker nb;
	STRPTR popkey;

	CxBase = OpenLibrary("commodities.library", MIN_OS_VERSION);
	if (!CxBase) {
		goto error;
	}

	BrokerPort = CreateMsgPort();
	if (!BrokerPort) {
		goto error;
	}

	ClearMem(&nb, sizeof(nb));
	nb.nb_Version = NB_VERSION;
	nb.nb_Name = PROGNAME;
	nb.nb_Title = PROGNAME;
	nb.nb_Descr = (STRPTR)GetString(&LocaleInfo, MSG_APPDESCRIPTION);
	nb.nb_Unique = NBU_UNIQUE|NBU_NOTIFY;
	nb.nb_Flags = COF_SHOW_HIDE;
	nb.nb_Pri = TTInteger(Icon, "CX_PRIORITY", 0);
	nb.nb_Port = BrokerPort;

	Broker = CxBroker(&nb, NULL);
	if (!Broker) {
		goto error;
	}

	popkey = TTString(Icon, "CX_POPKEY", NULL);
	if (popkey && TrimStr(popkey)[0]) {
		CxObj *filter, *sender, *translate;
		filter = CxFilter(popkey);
		sender = CxSender(BrokerPort, EVT_POPKEY);
		translate = CxTranslate(NULL);
		AttachCxObj(Broker, filter);
		AttachCxObj(filter, sender);
		AttachCxObj(filter, translate);
		if (!filter || !sender || !translate) {
			goto error;
		}
	}

	ActivateCxObj(Broker, TRUE);

	return TRUE;
error:
	UnregisterCxBroker();
	return FALSE;
}

void UnregisterCxBroker (void) {
	if (BrokerPort) {
		struct Message *msg;

		DeleteCxObjAll(Broker);
		Broker = NULL;

		while (msg = GetMsg(BrokerPort)) {
			ReplyMsg(msg);
		}

		DeleteMsgPort(BrokerPort);
		BrokerPort = NULL;
	}
	if (CxBase) {
		CloseLibrary(CxBase);
		CxBase = NULL;
	}
}
