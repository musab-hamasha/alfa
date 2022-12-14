/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		PluginConfiguration.cpp																				*/
/*																													*/
/*	Created:	20.06.08	tss																						*/
/*																													*/
/*	Modified:	??.??.??	tss cs4 porting: access to ISession														*/
/*				10.11.08	tss	getAlfrescoPattern(), autoEmbedImages->lowercase									*/
/*				29.11.10	hal	getPdfPresetName() added															*/
/*				30.11.10    hal	getLogFilePath() added																*/
/*				25.01.12	bu	showDocOpenMessages() added															*/
/*				27.04.2012	bu	use precompiled header																*/
/*				26.07.13	bu	pdfVersion14 added																	*/
/*																													*/
/********************************************************************************************************************/

#include "precompiled.h"


/********************************************************************************************************************/
const bool PluginConfiguration::doesAutoEmbedImages() const
/********************************************************************************************************************/
{
	return autoEmbedImages;
}

/********************************************************************************************************************/
const std::string PluginConfiguration::getAlfrescoPattern() const
/********************************************************************************************************************/
{
	return alfrescoPattern;
}

/********************************************************************************************************************/
bool PluginConfiguration::isInitialized() const
/********************************************************************************************************************/
{
	return initialized;
}

/********************************************************************************************************************/
const std::string PluginConfiguration::getPdfPresetName() const
/********************************************************************************************************************/
{
	return pdfPresetName;
}

/********************************************************************************************************************/
const bool PluginConfiguration::showOpenDocMessages() const
/********************************************************************************************************************/
{
	return showDocOpenMessages;
}

/********************************************************************************************************************/
const bool PluginConfiguration::isPdfVersion14() const
/********************************************************************************************************************/
{
	return pdfVersion14;
}

