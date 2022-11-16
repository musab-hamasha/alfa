/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		MacPathUtilss.h																						*/
/*																													*/
/*	Created:	21.05.08	tss																						*/
/*																													*/
/*	Modified:	12.05 14 bu modified for Mac																		*/
/*																													*/
/********************************************************************************************************************/

#include "CMyStream.h"

#ifndef WINOS
	void convertPathInStream(CMyStream *stream, CStr key, CFURLPathStyle inPathStyle, CFURLPathStyle outPathStyle);
	void convertPathStyle(char *path, int len, CFURLPathStyle inPathStyle, CFURLPathStyle outPathStyle);
#endif



