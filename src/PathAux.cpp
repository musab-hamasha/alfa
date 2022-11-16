/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		PathAux.cpp																							*/
/*																													*/
/*	Created:	20.06.08	tss																						*/
/*																													*/
/*	Modified:	tss cs4 porting: access to ISession																	*/
/*																													*/
/********************************************************************************************************************/

#include "precompiled.h"

extern PlugIn gPlugIn;


/********************************************************************************************************************/
bool PathAux::GetPlugInPath(std::string &pluginPath)
/********************************************************************************************************************/
{
	InterfacePtr <IPlugInList> plugInsList(UnifiedAux::GetSession(), IID_IPLUGINLIST);
	if (plugInsList == NULL)
	{
		LogLine("ERROR IN PathAux::GetPlugInPath() - can't get IPlugInList from gSession!");
		return false;
	}

	IDFile pluginFile;
	if (!plugInsList->GetPathName(gPlugIn.GetPluginID(), &pluginFile))
	{
		LogLine("ERROR IN PathAux::GetPlugInPath() - GetPathName() failed!");
		return false;
	}

	InterfacePtr<ICoreFilename> coreFilename((ICoreFilename*)::CreateObject(kCoreFilenameBoss, IID_ICOREFILENAME));
	coreFilename->Initialize(&pluginFile);
	PMString *fileName = coreFilename->GetFullName();

	pluginPath = std::string(fileName->GrabCString());
	///pluginPath = std::string("");

	LogLine("PathAux::GetPlugInPath() = %s", pluginPath.c_str());

	return true;
}