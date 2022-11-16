/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		XMLConfigurationReader.cpp																			*/
/*																													*/
/*	Created:	20.06.08	tss																						*/
/*																													*/
/*	Modified:	10.11.08	tss	now it's singletone - to read config only once										*/
/*				29.01.09	tss	cs4 porting: access to ISession and ISAXParserOptions								*/
/*				18.04.10	KLIM	LEAK: XMLConfigurationReader::getInstance() was changed to use smart pointer.	*/
/*									XMLConfigurationReader::sm_ptr is now used instead of ::instance.				*/
/*				15.03.12	bu		autoEmbedImages = false as default												*/
/*				26.07.13	bu		pdfVersion14: if set to false -> any version is allowed (alfaPresets)			*/
/*																													*/
/********************************************************************************************************************/

#include "precompiled.h"

XMLConfigurationReader::InstPtr XMLConfigurationReader::sm_ptr;


/********************************************************************************************************************/
XMLConfigurationReader::XMLConfigurationReader()
/********************************************************************************************************************/
{
	std::string pluginPathStr;
	if (!PathAux::GetPlugInPath(pluginPathStr))
		return;

	// cut filename
#ifdef _WINDOWS
	pluginPathStr = pluginPathStr.substr(0, pluginPathStr.find_last_of("\\/") + 1);
#else
	pluginPathStr = pluginPathStr.substr(0, pluginPathStr.find_last_of(":") + 1);
#endif

	if (!readParameters(pluginPathStr + XML_FILENAME))
	{
		config.applyDefaultValues();
		return;
	}

	config.setInitialized();
}

/********************************************************************************************************************/
XMLConfigurationReader* XMLConfigurationReader::getInstance()
/********************************************************************************************************************/
{
	return sm_ptr.Get();
}


/********************************************************************************************************************/
bool XMLConfigurationReader::readParameters(const std::string& filePath)
/********************************************************************************************************************/
{
	LogLine("XMLConfigurationReader::readParameters() from %s", filePath.c_str());

	SDKFileHelper fileHelper(filePath.c_str());
	InterfacePtr<IPMStream> readStream(StreamUtil::CreateFileStreamRead(fileHelper.GetIDFile()));
	if (readStream == NULL)
	{
		//LogLine("ERROR IN XMLConfigurationReader::readParameters() - can't create stream for reading!");
		LogLine("Configuration file does not exist, use defaults.");
		return false;
	}

	InterfacePtr<IK2ServiceRegistry> serviceRegistry(UnifiedAux::GetSession(), UseDefaultIID());
	InterfacePtr<IK2ServiceProvider> xmlProvider(serviceRegistry->QueryServiceProviderByClassID(kXMLParserService, kXMLParserServiceBoss));
	InterfacePtr<ISAXServices> saxServices(xmlProvider, UseDefaultIID());
	if (saxServices == NULL)
	{
		LogLine("ERROR IN XMLConfigurationReader::readParameters() - can't get ISAXServices interface!");
		return false;
	}

	// Set up out custom handler
	InterfacePtr<ISAXContentHandler> saxHandler(::CreateObject2<ISAXContentHandler>(kXMLConfigSAXContentHandlerServiceBoss));
	saxHandler->Register(saxServices);

	InterfacePtr<ISAXParserOptions> parserOptions(saxServices, UseDefaultIID());
	if (parserOptions == NULL)
	{
		LogLine("ERROR IN XMLConfigurationReader::readParameters() - can't get ISAXParserOptions interface!");
		return false;
	}

	// tss 27.01.2008 some of them are commented, because in cs4 they are not really needed and depricated
	parserOptions->SetNamespacesFeature(kFalse);
	parserOptions->SetShowWarningAlert(kTrue);

	// Returns 0 if it succeeded....
	bool16 parseFailed = saxServices->ParseStream(readStream, saxHandler);
	XMLConfigSAXContentHandler* mySAXHandler = reinterpret_cast<XMLConfigSAXContentHandler*>(saxHandler.get());
	std::map<std::string, std::string> data = mySAXHandler->getData();

	if (parseFailed || ErrorUtils::PMGetGlobalErrorCode() != kSuccess)
	{
		LogLine("ERROR IN XMLConfigurationReader::readParameters() - parsing failed!");
		return false;
	}

	bool res = true;
	for (std::map<std::string, std::string>::iterator it = data.begin(); it != data.end(); ++it)
	{
		if (!config.setParameter(it->first, it->second))
		{
			LogLine("ERROR IN XMLConfigurationReader::readParameters() - unknown parameter: %s = %s", 
				it->first.c_str(), it->second.c_str());
			res = false;
		}
	}

	return res;
}

/********************************************************************************************************************/
const PluginConfiguration& XMLConfigurationReader::GetPluginConfiguration()
/********************************************************************************************************************/
{
	return config;
}