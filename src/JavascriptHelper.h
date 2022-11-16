/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		JavascriptHelper.h																							*/
/*																													*/
/*	Created:																										*/
/*																													*/
/*	Modified:																										*/
/*																													*/
/********************************************************************************************************************/

#pragma once

#ifndef _JAVASCRIPT_HELPER
#define _JAVASCRIPT_HELPER

class JavascriptHelper
{
public:

	static ErrorCode RunScript(const std::string& script);
	static std::string EscapeParameter(std::string param);
};

#endif