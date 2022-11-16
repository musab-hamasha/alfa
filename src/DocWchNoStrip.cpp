/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		DocWchNoStrip.cpp																					*/
/*																													*/
/*	Created:																										*/
/*																													*/
/*	Modified:																										*/
/*																													*/
/********************************************************************************************************************/
/** DontDeadStrip
	references all implementations to stop the compiler dead stripping them from the executable image.
*/
#include "precompiled.h"

extern bool16 gFalse;

void DontDeadStrip();


/********************************************************************************************************************/
void DontDeadStrip()
/********************************************************************************************************************/
{
	if (gFalse)
	{
		#include "DocWchFactoryList.h"
	}
}