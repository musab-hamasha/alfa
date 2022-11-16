/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		ErrorChecking.cpp																					*/
/*																													*/
/*	Created:																										*/
/*																													*/
/*	Modified:	27.04.2012	bu	use precompiled header																*/
/*																													*/
/********************************************************************************************************************/

#include "precompiled.h"


/********************************************************************************************************************/
int CheckError(char *str, int log_success)
/********************************************************************************************************************/
{
	ErrorCode err = ErrorUtils::PMGetGlobalErrorCode();
	if ( err!=kSuccess )
	{
		PMString err_string = ErrorUtils::PMGetErrorString(err);
		char *err_buf = new char[1024];
		err_string.GetCString(err_buf, 1024);
		LogLine("CheckError(): error: %s, description: %s", str, err_buf);
		delete[] err_buf;

		return -1;
	}

	return 0;
}

/********************************************************************************************************************/
int CheckCreation(char *func, void *object, char *str, int log_success)
/********************************************************************************************************************/
{
	if (object == NULL)
	{
		LogLine("CheckCreation(func: %s, creation: %s) failed.", func, str);
		return -1;
	}

	return 0;
}

/********************************************************************************************************************/
int CheckUIDRef(char *func, UIDRef &ref, char *str, int log_success)
/********************************************************************************************************************/
{
	if (ref == UIDRef::gNull)
	{
		LogLine("CheckUIDRef(%s) failed: %s", func, str);
		return -1;
	}

	return 0;
}

/********************************************************************************************************************/
int CheckErrorCode(char *func, ErrorCode err, char *str, int log_success)
/********************************************************************************************************************/
{
	if (err != kSuccess)
	{
		LogLine("CheckErrorCode(%s) failed: %s", func, str);
		return -1;
	}

	return 0;
}