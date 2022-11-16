/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		PluginConfigurationImpl.cpp																			*/
/*																													*/
/*	Created:	20.06.08	tss																						*/
/*																													*/
/*	Modified:	10.11.08	tss	alfresco pattern handling in setParameter();										*/
/*								clear() is replaced with applyDefaultValues(), that is used also in constructor		*/
/*				29.02.12	bu	unified logging of parameters														*/
/*				27.04.2012	bu	use precompiled header																*/
/*																													*/
/********************************************************************************************************************/

#include "precompiled.h"


/********************************************************************************************************************/
PluginConfigurationImpl::PluginConfigurationImpl()
/********************************************************************************************************************/
{
	applyDefaultValues();
}


/********************************************************************************************************************/
bool PluginConfigurationImpl::setParameter(const std::string &paramName, const std::string &paramValue)
/********************************************************************************************************************/
{
	bool res = false;

	LogLine("PluginConfigurationImpl::setParameter() %s = %s", paramName.c_str(), paramValue.c_str());

	std::string lowerCaseValue = paramValue;
	std::transform( lowerCaseValue.begin(), lowerCaseValue.end(), lowerCaseValue.begin(), ::tolower ); 

	if (paramName.compare(AUTO_EMBED_IMAGES_PARAMNAME) == 0)
	{
		if (lowerCaseValue.compare("true") == 0 || lowerCaseValue.compare("1") == 0)
		{
			autoEmbedImages = true;
			res = true;
		}
		else if (lowerCaseValue.compare("false") == 0 || lowerCaseValue.compare("0") == 0)
		{
			autoEmbedImages = false;
			res = true;
		}
		else
		{	
			LogLine("PluginConfigurationImpl::setParameter() - can't convert value to bool, value  = %s", paramValue.c_str());
			res = false;
		}
	}
	else if (paramName.compare(ALFRESCO_PATTERN_PARAMNAME) == 0)
	{
		alfrescoPattern = paramValue;
		res = true;
	}
	else if (paramName.compare(PDF_PRESETNAME_PARAMNAME) == 0 )
	{
		pdfPresetName = paramValue;
		res = true;
	}
	else if (paramName.compare(CONFIG_OpenDocShowMessages) == 0 )
	{
		if (lowerCaseValue.compare("false") == 0 || lowerCaseValue.compare("0") == 0)
			showDocOpenMessages = false;
		else
			showDocOpenMessages = true;

		res = true;
	}
	else if (paramName.compare(CONFIG_PdfVersion14) == 0 )
	{
		if (lowerCaseValue.compare("false") == 0 || lowerCaseValue.compare("0") == 0)
			pdfVersion14 = false;
		else
			pdfVersion14 = true;

		res = true;
	}
	else
	{
		LogLine("ERROR IN PluginConfigurationImpl::setParameter() - unknown parameter name = %s", paramName.c_str());
		res = false;
	}

	return res;
}

/********************************************************************************************************************/
void PluginConfigurationImpl::setInitialized()
/********************************************************************************************************************/
{
	initialized = true;
}


/********************************************************************************************************************/
void PluginConfigurationImpl::applyDefaultValues()
/********************************************************************************************************************/
{
	initialized = false;
	autoEmbedImages = false;
	alfrescoPattern = "";
	pdfPresetName	= "";
	showDocOpenMessages = false;
	pdfVersion14 = true;
}
