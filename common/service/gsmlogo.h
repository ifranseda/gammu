/* (c) 2002-2004 by Marcin Wiacek */

#ifndef __gsm_bitmaps_h
#define __gsm_bitmaps_h

#include "../gsmcomon.h"


#include <gammu-calendar.h>

typedef enum {
	GSM_NokiaStartupLogo = 1,	/*size 84*48*/
	GSM_NokiaOperatorLogo,		/*size 72*14*/
	GSM_Nokia7110OperatorLogo,	/*size 78*21*/
	GSM_Nokia6510OperatorLogo,	/*size 78*21*/
	GSM_NokiaCallerLogo,		/*size 72*14*/
	GSM_NokiaPictureImage,		/*size 72*28*/
	GSM_Nokia7110StartupLogo,	/*size 96*65*/
	GSM_Nokia6210StartupLogo,	/*size 96*60*/
	GSM_AlcatelBMMIPicture,
	GSM_EMSSmallPicture,		/*size  8* 8*/
	GSM_EMSMediumPicture,		/*size 16*16*/
	GSM_EMSBigPicture,		/*size 32*32*/
	GSM_EMSVariablePicture
} GSM_Phone_Bitmap_Types;

bool GSM_IsPointBitmap		(GSM_Bitmap 		*bmp, int x, int y);
void GSM_SetPointBitmap		(GSM_Bitmap 		*bmp, int x, int y);
void GSM_ClearPointBitmap	(GSM_Bitmap 		*bmp, int x, int y);
void GSM_ClearBitmap		(GSM_Bitmap 		*bmp);
void GSM_ResizeBitmap		(GSM_Bitmap 		*dest, GSM_Bitmap *src, int width, int height);
void GSM_ReverseBitmap		(GSM_Bitmap 		*Bitmap);
void GSM_GetMaxBitmapWidthHeight(GSM_Bitmap_Types 	Type, unsigned char *width, unsigned char *height);
int  GSM_GetBitmapSize		(GSM_Bitmap 		*bmp);
void GSM_PrintBitmap		(FILE 			*file, GSM_Bitmap *bitmap);

GSM_Error GSM_SaveBitmapFile	(char *FileName, GSM_MultiBitmap *bitmap);
GSM_Error GSM_ReadBitmapFile	(char *FileName, GSM_MultiBitmap *bitmap);

GSM_Error BMP2Bitmap            (unsigned char *buffer, FILE *file,GSM_Bitmap *bitmap);
GSM_Error Bitmap2BMP            (unsigned char *buffer, FILE *file,GSM_Bitmap *bitmap);

void PHONE_GetBitmapWidthHeight	(GSM_Phone_Bitmap_Types Type, int *width, int *height);
int  PHONE_GetBitmapSize	(GSM_Phone_Bitmap_Types Type, int width, int height);
void PHONE_ClearBitmap		(GSM_Phone_Bitmap_Types Type, char *buffer, int width, int height);
void PHONE_DecodeBitmap		(GSM_Phone_Bitmap_Types Type, char *buffer, GSM_Bitmap *Bitmap);
void PHONE_EncodeBitmap		(GSM_Phone_Bitmap_Types Type, char *buffer, GSM_Bitmap *Bitmap);

void NOKIA_CopyBitmap		(GSM_Phone_Bitmap_Types Type, GSM_Bitmap *Bitmap, char *Buffer, int *Length);

#endif

/* How should editor hadle tabs in this file? Add editor commands here.
 * vim: noexpandtab sw=8 ts=8 sts=8:
 */
