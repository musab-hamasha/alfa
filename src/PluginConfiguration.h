/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		PluginConfiguration.h																				*/
/*																													*/
/*	Created:																										*/
/*																													*/
/*	Modified:	25.01.12 bu	showDocOpenMessages added																*/
/*																													*/
/********************************************************************************************************************/

#ifndef _PLUGIN_CONFIGURATION
#define _PLUGIN_CONFIGURATION

#include <string>


/********************************************************************************************************************/
class PluginConfiguration
/********************************************************************************************************************/
{
protected:
	bool autoEmbedImages;
	bool showDocOpenMessages;
	std::string alfrescoPattern;
	bool initialized;
	bool pdfVersion14;
	std::string pdfPresetName;  

public:
	virtual const bool doesAutoEmbedImages() const;
	virtual const bool showOpenDocMessages() const;
	virtual const std::string getAlfrescoPattern() const;
	virtual bool isInitialized() const;
	virtual const bool isPdfVersion14() const;
	virtual const std::string getPdfPresetName() const; 
};

#endif