/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		XMLConfigSAXContentHandler.h																		*/
/*																													*/
/********************************************************************************************************************/

#ifndef _XML_CONFIGURATION_READER
#define _XML_CONFIGURATION_READER

#include <string>

#include "PluginConfigurationImpl.h"

/********************************************************************************************************************/
class XMLConfigurationReader
/********************************************************************************************************************/
{
private:
	// LEAK
	class InstPtr
	{
	public:
		InstPtr() : m_ptr(0) {}
		~InstPtr() { delete m_ptr; }
		XMLConfigurationReader* Get()
		{
			if( !m_ptr )
				m_ptr = new XMLConfigurationReader();
			return m_ptr;
		}
	private:
		XMLConfigurationReader* m_ptr;
	};

	static InstPtr sm_ptr;

	XMLConfigurationReader();
	PluginConfigurationImpl config;
	bool readParameters(const std::string& filename);

public:	
	static XMLConfigurationReader* getInstance();
	const PluginConfiguration& GetPluginConfiguration();
};

#endif