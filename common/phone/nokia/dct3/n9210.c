
#include "../../../gsmstate.h"

#ifdef GSM_ENABLE_NOKIA9210

#include <string.h>
#include <time.h>

#include "../../../gsmcomon.h"
#include "../../../misc/coding.h"
#include "../../../service/gsmlogo.h"
#include "../../pfunc.h"
#include "../nfunc.h"
#include "n9210.h"
#include "dct3func.h"

static GSM_Error N9210_GetBitmap(GSM_StateMachine *s, GSM_Bitmap *Bitmap)
{
	unsigned char OpReq[] = {N6110_FRAME_HEADER, 0x70};

	s->Phone.Data.Bitmap=Bitmap;	
	switch (Bitmap->Type) {
	case GSM_OperatorLogo:
		dprintf("Getting operator logo\n");
		/* This is like DCT3_GetNetworkInfo */
		return GSM_WaitFor (s, OpReq, 4, 0x0a, 4, ID_GetBitmap);
	case GSM_StartupLogo:
		dprintf("Getting startup logo\n");
		return N71_92_GetPhoneSetting(s, ID_GetBitmap, 0x15);
	case GSM_WelcomeNoteText:
		dprintf("Getting welcome note\n");
		return N71_92_GetPhoneSetting(s, ID_GetBitmap, 0x02);
	default:
		break;
	}
	return GE_NOTSUPPORTED;
}

static GSM_Error N9210_ReplySetOpLogo(GSM_Protocol_Message msg, GSM_Phone_Data *Data, GSM_User *User)
{
	dprintf("Operator logo clear/set\n");
	return GE_NONE;
}

static GSM_Error N9210_SetBitmap(GSM_StateMachine *s, GSM_Bitmap *Bitmap)
{
	GSM_Error		error;
	GSM_Phone_Bitmap_Types	Type;
	int			Width, Height, i,count=3;
	unsigned char		req[600] = { N7110_FRAME_HEADER };
	unsigned char 		reqStartup[1000] = {
		N6110_FRAME_HEADER, 0xec,
		0x15,			/* Startup Logo setting */
		0x04, 0x00, 0x00, 0x00, 0x30, 0x00,
		0x02, 0xc0, 0x54, 0x00, 0x03, 0xc0,
		0xf8, 0xf8, 0x01, 0x04};
	unsigned char 		reqStartupText[500] = {
		N7110_FRAME_HEADER, 0xec,
		0x02};			/* Startup Text setting */
	unsigned char 		reqClrOp[] = {
		N7110_FRAME_HEADER, 0xAF,
		0x02};			/* Number of logo = 0 - 0x04 */

	switch (Bitmap->Type) {
	case GSM_StartupLogo:
		if (Bitmap->Location!=1) return GE_NOTSUPPORTED;
		Type=GSM_NokiaStartupLogo;
		PHONE_GetBitmapWidthHeight(Type, &Width, &Height);
		PHONE_EncodeBitmap(Type, reqStartup + 21, Bitmap);		
		dprintf("Setting startup logo\n");
		return GSM_WaitFor (s, reqStartup, 21+PHONE_GetBitmapSize(Type), 0x7A, 4, ID_SetBitmap);
	case GSM_WelcomeNoteText:	
		/* Nokia bug: Unicode text is moved one char to left */
		CopyUnicodeString(reqStartupText + 4, Bitmap->Text);
		reqStartupText[4] = 0x02;
		i = 5 + strlen(DecodeUnicodeString(Bitmap->Text)) * 2;
		reqStartupText[i++] = 0;
		reqStartupText[i++] = 0;
		return GSM_WaitFor (s, reqStartupText, i, 0x7A, 4, ID_SetBitmap);
	case GSM_OperatorLogo:
		/* First part for clearing logo */
		if (!strcmp(Bitmap->NetworkCode,"000 00")) {
			for (i=0;i<5;i++) {
				reqClrOp[4] = i;
				error=GSM_WaitFor (s, reqClrOp, 5, 0x0A, 4, ID_SetBitmap);
				if (error != GE_NONE) return error;
			}
		}
		Type=GSM_NokiaOperatorLogo;
		req[count++] = 0xA3;
		req[count++] = 0x01;
		req[count++] = 0x00; /* Logo removed */
		NOKIA_EncodeNetworkCode(req+count, "000 00");
		count = count + 3;
		req[count++] = 0x00;
		req[count++] = 0x04;
		req[count++] = 0x08; /* Length of rest + 2 */
		memcpy(req+count, "\x00\x00\x00\x00\x00\x00", 6);
		count += 6;
		error=GSM_WaitFor (s, req, count, 0x0A, 4, ID_SetBitmap);
		if (error != GE_NONE) return error;
		/* We wanted only clear - now exit */
		if (!strcmp(Bitmap->NetworkCode,"000 00")) return error;

		/* Now setting logo */	
		count=3;
		req[count++] = 0xA3;
		req[count++] = 0x01;
		req[count++] = 0x01; /* Logo set */
		NOKIA_EncodeNetworkCode(req+count, Bitmap->NetworkCode);
		count = count + 3;
		req[count++] = 0x00;
		req[count++] = 0x04;
		req[count++] = PHONE_GetBitmapSize(Type)+8;
		PHONE_GetBitmapWidthHeight(Type, &Width, &Height);
		req[count++] = Width;
		req[count++] = Height;
		req[count++] = PHONE_GetBitmapSize(Type);
		req[count++] = 0x00;
		req[count++] = 0x00;
		req[count++] = 0x00;
		PHONE_EncodeBitmap(Type, req+count, Bitmap);		
		return GSM_WaitFor (s, req, count+PHONE_GetBitmapSize(Type), 0x0A, 4, ID_SetBitmap);
	default:
		break;
	}
	return GE_NOTSUPPORTED;
}

static GSM_Error N9210_ReplyIncomingSMS(GSM_Protocol_Message msg, GSM_Phone_Data *Data, GSM_User *User)
{
	GSM_SMSMessage sms;

#ifdef DEBUG
	dprintf("SMS message received\n");
	sms.State 	= GSM_UnRead;
	sms.InboxFolder = true;
	DCT3_DecodeSMSFrame(&sms,msg.Buffer+5);
#endif
	if (Data->EnableIncomingSMS && User->IncomingSMS!=NULL) {
		sms.State 	= GSM_UnRead;
		sms.InboxFolder = true;
		DCT3_DecodeSMSFrame(&sms,msg.Buffer+5);

		User->IncomingSMS(Data->Device,sms);
	}
	return GE_NONE;
}

#ifdef GSM_ENABLE_N71_92INCOMINGINFO
static GSM_Error N9210_ReplySetIncomingSMS(GSM_Protocol_Message msg, GSM_Phone_Data *Data, GSM_User *User)
{
	switch (msg.Buffer[3]) {
	case 0x0e:
		Data->EnableIncomingSMS = true;
		dprintf("Incoming SMS enabled\n");
		return GE_NONE;
	case 0x0f:
		dprintf("Error enabling incoming SMS\n");
		switch (msg.Buffer[4]) {
		case 0x0c:
			dprintf("No PIN ?\n");
			return GE_SECURITYERROR;
		default:
			dprintf("ERROR: unknown %i\n",msg.Buffer[4]);
		}
	}
	return GE_UNKNOWNRESPONSE;
}
#endif

static GSM_Error N9210_SetIncomingSMS(GSM_StateMachine *s, bool enable)
{
#ifdef GSM_ENABLE_N71_92INCOMINGINFO
	unsigned char req[] = {N6110_FRAME_HEADER, 0x0d, 0x00, 0x00, 0x02};

	if (enable!=s->Phone.Data.EnableIncomingSMS) {
		if (enable) {
			dprintf("Enabling incoming SMS\n");
			return GSM_WaitFor (s, req, 7, 0x02, 4, ID_SetIncomingSMS);
		} else {
			s->Phone.Data.EnableIncomingSMS = false;
			dprintf("Disabling incoming SMS\n");
		}
	}
	return GE_NONE;
#else
	return GE_SOURCENOTAVAILABLE;
#endif
}

static GSM_Error N9210_Initialise (GSM_StateMachine *s)
{
#ifdef DEBUG
	DCT3_SetIncomingCB(s,true);

#ifdef GSM_ENABLE_N71_92INCOMINGINFO
	N9210_SetIncomingSMS(s,true);
#endif

#endif
	return GE_NONE;
}

static GSM_Reply_Function N9210ReplyFunctions[] = {
	{DCT3_ReplySendSMSMessage,	"\x02",0x03,0x02,ID_IncomingFrame	},
	{DCT3_ReplySendSMSMessage,	"\x02",0x03,0x03,ID_IncomingFrame	},
#ifdef GSM_ENABLE_N71_92INCOMINGINFO
	{N9210_ReplySetIncomingSMS,	"\x02",0x03,0x0E,ID_SetIncomingSMS	},
	{N9210_ReplySetIncomingSMS,	"\x02",0x03,0x0F,ID_SetIncomingSMS	},
#endif
	{N9210_ReplyIncomingSMS,	"\x02",0x03,0x11,ID_IncomingFrame	},
#ifdef GSM_ENABLE_CELLBROADCAST
	{DCT3_ReplySetIncomingCB,	"\x02",0x03,0x21,ID_SetIncomingCB	},
	{DCT3_ReplySetIncomingCB,	"\x02",0x03,0x22,ID_SetIncomingCB	},
	{DCT3_ReplyIncomingCB,		"\x02",0x03,0x23,ID_IncomingFrame	},
#endif
	{DCT3_ReplySetSMSC,		"\x02",0x03,0x31,ID_SetSMSC		},
	{DCT3_ReplyGetSMSC,		"\x02",0x03,0x34,ID_GetSMSC		},
	{DCT3_ReplyGetSMSC,		"\x02",0x03,0x35,ID_GetSMSC		},

	{N61_91_ReplySetOpLogo,		"\x05",0x03,0x31,ID_SetBitmap		},
	{N61_91_ReplySetOpLogo,		"\x05",0x03,0x32,ID_SetBitmap		},

	{DCT3_ReplyGetNetworkInfo,	"\x0A",0x03,0x71,ID_GetNetworkInfo	},
	{DCT3_ReplyGetNetworkInfo,	"\x0A",0x03,0x71,ID_IncomingFrame	},
	{N71_92_ReplyGetSignalQuality,	"\x0A",0x03,0x82,ID_GetSignalQuality	},
	{N9210_ReplySetOpLogo,		"\x0A",0x03,0xA4,ID_SetBitmap		},
	{N9210_ReplySetOpLogo,		"\x0A",0x03,0xB0,ID_SetBitmap		},

	{N71_92_ReplyGetBatteryCharge,	"\x17",0x03,0x03,ID_GetBatteryCharge	},

	{DCT3_ReplySetDateTime,		"\x19",0x03,0x61,ID_SetDateTime		},
	{DCT3_ReplyGetDateTime,		"\x19",0x03,0x63,ID_GetDateTime		},

	{DCT3_ReplyEnableSecurity,	"\x40",0x02,0x64,ID_EnableSecurity	},
	{DCT3_ReplyGetIMEI,		"\x40",0x02,0x66,ID_GetIMEI		},
	{DCT3_ReplyDialCommand,		"\x40",0x02,0x7C,ID_DialVoice		},
	{DCT3_ReplyDialCommand,		"\x40",0x02,0x7C,ID_CancelCall		},
	{DCT3_ReplyDialCommand,		"\x40",0x02,0x7C,ID_AnswerCall		},
	{DCT3_ReplyNetmonitor,		"\x40",0x02,0x7E,ID_Netmonitor		},
	{NOKIA_ReplyGetPhoneString,	"\x40",0x02,0xC8,ID_GetHardware		},
	{NOKIA_ReplyGetPhoneString,	"\x40",0x02,0xC8,ID_GetPPM		},
	{NOKIA_ReplyGetPhoneString,	"\x40",0x02,0xCA,ID_GetProductCode	},
	{NOKIA_ReplyGetPhoneString,	"\x40",0x02,0xCC,ID_GetManufactureMonth	},
	{NOKIA_ReplyGetPhoneString,	"\x40",0x02,0xCC,ID_GetOriginalIMEI	},

	{N71_92_ReplyPhoneSetting,	"\x7a",0x04,0x02,ID_GetBitmap		},
	{N71_92_ReplyPhoneSetting,	"\x7a",0x04,0x02,ID_SetBitmap		},
	{N71_92_ReplyPhoneSetting,	"\x7a",0x04,0x15,ID_GetBitmap		},
	{N71_92_ReplyPhoneSetting,	"\x7a",0x04,0x15,ID_SetBitmap		},

	{DCT3DCT4_ReplyGetModelFirmware,"\xD2",0x02,0x00,ID_GetModel		},
	{DCT3DCT4_ReplyGetModelFirmware,"\xD2",0x02,0x00,ID_GetFirmware		},

	{NULL,				"\x00",0x00,0x00,ID_None		}
};

GSM_Phone_Functions N9210Phone = {
	"9210",
	N9210ReplyFunctions,
	N9210_Initialise,
	PHONE_Terminate,
	GSM_DispatchMessage,
	DCT3DCT4_GetModel,
	DCT3DCT4_GetFirmware,
	DCT3_GetIMEI,
	N71_92_GetDateTime,
	NOTIMPLEMENTED,		/*	GetAlarm		*/
	NOTIMPLEMENTED,		/*	GetMemory		*/
	NOTIMPLEMENTED,		/*	GetMemoryStatus		*/
	DCT3_GetSMSC,
	NOTIMPLEMENTED,		/*	GetSMSMessage		*/
	NOTIMPLEMENTED,		/*	GetSMSFolders		*/
	NOKIA_GetManufacturer,
	NOTIMPLEMENTED,		/*	GetNextSMSMessage	*/
	NOTIMPLEMENTED,		/*	GetSMSStatus		*/
	N9210_SetIncomingSMS,
	DCT3_GetNetworkInfo,
	NOTIMPLEMENTED,		/*	Reset			*/
	DCT3_DialVoice,
	DCT3_AnswerCall,
	DCT3_CancelCall,
	NOTIMPLEMENTED,		/*	GetRingtone		*/
	NOTIMPLEMENTED,		/*	GetWAPBookmark		*/
	N9210_GetBitmap,
	NOTIMPLEMENTED,		/*	SetRingtone		*/
	NOTIMPLEMENTED,		/*	SaveSMSMessage		*/
	DCT3_SendSMSMessage,
	N71_92_SetDateTime,
	NOTIMPLEMENTED,		/*	SetAlarm		*/
	N9210_SetBitmap,
	NOTIMPLEMENTED,		/* 	SetMemory 		*/
	NOTIMPLEMENTED,		/* 	DeleteSMS 		*/
	NOTIMPLEMENTED,		/* 	SetWAPBookmark 		*/
	NOTIMPLEMENTED, 	/* 	DeleteWAPBookmark 	*/
	NOTIMPLEMENTED,		/* 	GetWAPSettings 		*/
	DCT3_SetIncomingCB,
	DCT3_SetSMSC,		/* 	FIXME: test it		*/
	DCT3_GetManufactureMonth,
	DCT3_GetProductCode,
	DCT3_GetOriginalIMEI,
	DCT3_GetHardware,
	DCT3_GetPPM,
	NOTIMPLEMENTED,		/*	PressKey		*/
	NOTSUPPORTED,		/*	GetToDo			*/
	NOTSUPPORTED,		/*	DeleteAllToDo		*/
	NOTSUPPORTED,		/*	SetToDo			*/
	NOTSUPPORTED,		/*	PlayTone		*/
	NOTSUPPORTED,		/*	EnterSecurityCode	*/
	NOTSUPPORTED,		/*	GetSecurityStatus	*/
	NOTIMPLEMENTED, 	/*	GetProfile		*/
	NOTSUPPORTED,		/*	GetRingtonesInfo	*/
	NOTSUPPORTED,		/* 	SetWAPSettings 		*/
	NOTIMPLEMENTED,		/*	GetSpeedDial		*/
	NOTIMPLEMENTED,		/*	SetSpeedDial		*/
	NOTIMPLEMENTED,		/*	ResetPhoneSettings	*/
	NOTSUPPORTED,		/*	SendDTMF		*/
	NOTSUPPORTED,		/*	GetDisplayStatus	*/
	NOTIMPLEMENTED,		/*	SetAutoNetworkLogin	*/
	NOTSUPPORTED, 		/*	SetProfile		*/
	NOTSUPPORTED,		/*	GetSIMIMSI		*/
	NOTSUPPORTED,		/*	SetIncomingCall		*/
    	NOTSUPPORTED,		/*  	GetNextCalendar		*/
	NOTSUPPORTED,		/*	DelCalendar		*/
	NOTSUPPORTED,		/*	AddCalendar		*/
	N71_92_GetBatteryCharge,
	N71_92_GetSignalQuality,
	NOTSUPPORTED,       	/*  	GetCategory 		*/
        NOTSUPPORTED,        	/*  	GetCategoryStatus 	*/
    	NOTSUPPORTED,		/*  	GetFMStation        	*/
	NOTIMPLEMENTED		/*  	SetIncomingUSSD		*/
};

#endif