/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		JavascriptHelper.cpp																				*/
/*																													*/
/*	Created:																										*/
/*																													*/
/*	Modified:																										*/
/*				        	  	         																			*/
/*																													*/
/********************************************************************************************************************/

#include "precompiled.h"

/********************************************************************************************************************/
ErrorCode JavascriptHelper::RunScript(const std::string& script)
/********************************************************************************************************************/
{
	ErrorCode result = kFailure;

	CAlert::InformationAlert(PMString(script.c_str()));

	do
	{
		InterfacePtr<IScriptManager> scriptManager(Utils<IScriptUtils>()->QueryScriptManager(kJavaScriptMgrBoss));
		if (!scriptManager)
			break;

		InterfacePtr<IScriptEngine> scriptEngine(scriptManager->QueryDefaultEngine());
		if (!scriptEngine)
			break;

		InterfacePtr<IScriptRunner> scriptRunner(scriptEngine, UseDefaultIID());
		if (!scriptRunner)
			break;

		RunScriptParams params(scriptRunner);
		result = scriptRunner->RunScript(PMString(script.c_str()), params);

	} while (false);

	return result;
}

/********************************************************************************************************************/
std::string JavascriptHelper::EscapeParameter(std::string param)
/********************************************************************************************************************/
{
	boost::replace_all(param, "\\", "\\\\");
	boost::replace_all(param, "'", "\\'");

	return param;
}