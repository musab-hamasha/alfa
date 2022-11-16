/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		PluginConfigurationImpl.h																			*/
/*																													*/
/*	Created:																										*/
/*																													*/
/*	Modified:																										*/
/*																													*/
/********************************************************************************************************************/

#ifndef _OM_CONFIGURATION_IMPL
#define _OM_CONFIGURATION_IMPL

#include "PluginConfiguration.h"

#define XML_FILENAME					"alfaConfig.xml"
#define AUTO_EMBED_IMAGES_PARAMNAME		"autoEmbedImages"
#define ALFRESCO_PATTERN_PARAMNAME		"alfrescoPattern"
#define PDF_PRESETNAME_PARAMNAME		"PdfPresetName"    
#define CONFIG_OpenDocShowMessages		"OpenDocShowMessages"
#define CONFIG_PdfVersion14				"PdfVersion14"


/********************************************************************************************************************/
class PluginConfigurationImpl: public PluginConfiguration
/********************************************************************************************************************/
{
public:
	PluginConfigurationImpl();
	void applyDefaultValues();
	void setInitialized();
	bool setParameter(const std::string &paramName, const std::string &paramValue);	
};

#endif