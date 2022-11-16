/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		UnifiedAux.cpp																						*/
/*																													*/
/*	Created:	29.01.2009	tss																						*/
/*																													*/
/*	Modified:	10.11.08	tss	now it's singletone - to read config only once										*/
/*				29.01.09	tss	cs4 porting: access to ISession and ISAXParserOptions								*/
/*				18.04.10	KLIM	LEAK: XMLConfigurationReader::getInstance() was changed to use smart pointer.	*/
/*									XMLConfigurationReader::sm_ptr is now used instead of ::instance.				*/
/*				27.04.2012	bu	use precompiled header																*/
/*																													*/
/********************************************************************************************************************/

#include "precompiled.h"


/********************************************************************************************************************/
ISession* UnifiedAux::GetSession() 
/********************************************************************************************************************/
{
	return GetExecutionContextSession();
}
