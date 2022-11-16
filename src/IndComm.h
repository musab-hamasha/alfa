/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		IndComm.h																							*/
/*																													*/
/*	Created:																										*/
/*																													*/
/*	Modified:																										*/
/*																													*/
/********************************************************************************************************************/

#include "CMyStream.h"
#include "PluginLogger.h"

TargetApp getClient(int32 refnum);
void setFrontProcess(TargetApp target);
void setFrontProcess(int32 refnum);
int32 sendData(TargetApp target, int32 eventID, CMyStream *stream);