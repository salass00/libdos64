/*
 * Copyright (C) 2026 Fredrik Wikstrom <fredrik@a500.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "libdos64_internal.h"

#include <exec/alerts.h>
#include <proto/exec.h>

#include <string.h>

#ifdef __GNUC__
#define STATIC_ASSERT(cond,msg) _Static_assert(cond, msg)
#else
#define STATIC_ASSERT(cond,msg)
#endif

struct DosPacket64
{
	struct Message *dp_Link;
	struct MsgPort *dp_Port;

	LONG  dp_Type;
	SIPTR dp_Res0;

	SIPTR dp_Res2;
	BYTE  dp_pad1[4];
	QUAD  dp_Res1;

	SIPTR dp_Arg1;
	BYTE  dp_pad2[4];
	QUAD  dp_Arg2;
	SIPTR dp_Arg3;
	SIPTR dp_Arg4;
	QUAD  dp_Arg5;
};

STATIC_ASSERT(offsetof(struct DosPacket64, dp_Link) == offsetof(struct DosPacket, dp_Link),
              "offset of dp_Link differs between DosPacket64 and DosPacket.");
STATIC_ASSERT(offsetof(struct DosPacket64, dp_Port) == offsetof(struct DosPacket, dp_Port),
              "offset of dp_Port differs between DosPacket64 and DosPacket.");
STATIC_ASSERT(offsetof(struct DosPacket64, dp_Type) == offsetof(struct DosPacket, dp_Type),
              "offset of dp_Type differs between DosPacket64 and DosPacket.");
STATIC_ASSERT(offsetof(struct DosPacket64, dp_Res0) == offsetof(struct DosPacket, dp_Res1),
              "offset of dp_Res0 and dp_Res1 differs between DosPacket64 and DosPacket.");
STATIC_ASSERT(offsetof(struct DosPacket64, dp_Res2) == offsetof(struct DosPacket, dp_Res2),
              "offset of dp_Res2 differs between DosPacket64 and DosPacket.");

#define MAGIC_RES0_VALUE ((SIPTR)-69)

QUAD DoPkt64(struct MsgPort *port, LONG errReturn, LONG action, LONG arg1, QUAD arg2, LONG arg3)
{
	struct Process *me;
	struct MsgPort *replyport;
	struct Message msg;
	struct DosPacket64 pkt;

	me = (struct Process *)FindTask(NULL);
	if (me->pr_Task.tc_Node.ln_Type != NT_PROCESS)
	{
		/* Not task-callable */
		return errReturn;
	}
	replyport = &me->pr_MsgPort;

	msg.mn_Node.ln_Name = (char *)&pkt;
	msg.mn_Length       = sizeof(msg);
	msg.mn_ReplyPort    = replyport;

	memset(&pkt, 0, sizeof(pkt));

	pkt.dp_Link = &msg;
	pkt.dp_Port = replyport;
	pkt.dp_Type = action;
	pkt.dp_Res0 = MAGIC_RES0_VALUE;
	pkt.dp_Arg1 = arg1;
	pkt.dp_Arg2 = arg2;
	pkt.dp_Arg3 = arg3;

	PutMsg(port, &msg);

	while (TRUE)
	{
		WaitPort(replyport);
		if (GetMsg(replyport) != &msg)
		{
			/* Unexpected message received */
			Alert(AN_AsyncPkt);
			continue;
		}

		break;
	}

	/* If dp_Res0 was overwritten then the fs does not support 64-bit packets. */
	if (pkt.dp_Res0 != MAGIC_RES0_VALUE)
	{
		SetIoErr(ERROR_ACTION_NOT_KNOWN);
		return errReturn;
	}

	SetIoErr(pkt.dp_Res2);
	return pkt.dp_Res1;
}

