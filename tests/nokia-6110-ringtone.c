/* Test for decoding Ringtine on Nokia 6110 driver */

#include <gammu.h>
#include <stdlib.h>
#include <stdio.h>
#include "../libgammu/protocol/protocol.h" /* Needed for GSM_Protocol_Message */
#include "../libgammu/gsmstate.h" /* Needed for state machine internals */

#include "common.h"

unsigned char data[] = {
	0x01, 0x01, 0x9E, 0x00, 0x00, 0x00, 0x01, 0x2C, 0x02, 0x4A, 0x3A, 0x6D, 0x4D, 0x85, 0xB8, 0x81,
	0x4D, 0x95, 0x89, 0x85, 0xCD, 0xD1, 0xA4, 0x04, 0x1E, 0x89, 0x22, 0xD5, 0x16, 0x49, 0x16, 0x11,
	0x41, 0x64, 0x14, 0x41, 0x34, 0x11, 0x42, 0x4D, 0xA0, 0xCA, 0x14, 0x45, 0x05, 0x10, 0x65, 0x06,
	0x12, 0x41, 0x85, 0x90, 0x61, 0x06, 0x50, 0x61, 0x05, 0x90, 0x61, 0x06, 0x52, 0x61, 0x85, 0x90,
	0x61, 0x06, 0x52, 0x61, 0x85, 0x90, 0x51, 0x05, 0x12, 0x41, 0x85, 0x90, 0x61, 0x05, 0x92, 0x45,
	0x84, 0x50, 0x59, 0x05, 0x10, 0x4D, 0x04, 0x50, 0x93, 0x68, 0x32, 0x85, 0x11, 0x41, 0x44, 0x19,
	0x41, 0x84, 0x90, 0x61, 0x64, 0x18, 0x41, 0x94, 0x18, 0x41, 0x64, 0x98, 0x61, 0x94, 0x18, 0x41,
	0x64, 0x98, 0x61, 0x94, 0x98, 0x61, 0x64, 0x14, 0x41, 0x44, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	};

/* This is not part of API! */
extern GSM_Error N6110_ReplyGetRingtone(GSM_Protocol_Message msg, GSM_StateMachine *s);

int main(int argc UNUSED, char **argv UNUSED)
{
	GSM_Debug_Info *debug_info;
	GSM_StateMachine *s;
	GSM_Protocol_Message msg;
	GSM_Error error;
	GSM_Ringtone Ringtone;

	debug_info = GSM_GetGlobalDebug();
	GSM_SetDebugFileDescriptor(stderr, FALSE, debug_info);
	GSM_SetDebugLevel("textall", debug_info);

	/* Allocates state machine */
	s = GSM_AllocStateMachine();
	test_result(s != NULL);
	debug_info = GSM_GetDebug(s);
	GSM_SetDebugGlobal(TRUE, debug_info);
	GSM_SetDebugFileDescriptor(stderr, FALSE, debug_info);
	GSM_SetDebugLevel("textall", debug_info);

	/* Init message */
	msg.Type = 0x40;
	msg.Length = sizeof(data);
	msg.Buffer = data;

	s->Phone.Data.Ringtone = &Ringtone;
	Ringtone.Format = RING_NOTETONE;

	/* Parse it */
	error = N6110_ReplyGetRingtone(msg, s);

	/* Free state machine */
	GSM_FreeStateMachine(s);

	gammu_test_result(error, "N6110_ReplyGetRingtone");

	return 0;
}

/* Editor configuration
 * vim: noexpandtab sw=8 ts=8 sts=8 tw=72:
 */
