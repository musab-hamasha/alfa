/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		XMLConfigSAXContentHandler.cpp																		*/
/*																													*/
/*	Created:	20.06.08	tss																						*/
/*																													*/
/*	Modified:	27.04.2012	bu	use precompiled header																*/
/*																													*/
/********************************************************************************************************************/

#include "precompiled.h"

CREATE_PMINTERFACE(XMLConfigSAXContentHandler, kXMLConfigSAXContentHandlerImpl)


/********************************************************************************************************************/
void XMLConfigSAXContentHandler::StartElement(const WideString &uri, const WideString &localname, 
																	const WideString &qname, ISAXAttributes *attrs)
/********************************************************************************************************************/
{
	if (isError)
		return;

	if (isFirst)
	{
		isFirst = false;
		if (localname == WideString("configuration"))
		{
			WideString versionStr;
			attrs->GetValue(WideString("version"), versionStr);
#ifdef CC
			LogLine("XMLConfigSAXContentHandler::StartElement() - config file version = %s", PMString(versionStr).GrabCString().c_str());
#else
			LogLine("XMLConfigSAXContentHandler::StartElement() - config file version = %s", PMString(versionStr).GrabCString());
#endif
			if (versionStr != WideString("1.0") && versionStr != WideString("1.1"))
			{
				LogLine("ERROR IN XMLConfigSAXContentHandler::StartElement() - unknown config file version!");
				isError = true;
			}
		}
		else
		{
#ifdef CC
			LogLine("ERROR IN XMLConfigSAXContentHandler::StartElement() - bad root element = %s", 
				PMString(localname).GrabCString().c_str());
#else
			LogLine("ERROR IN XMLConfigSAXContentHandler::StartElement() - bad root element = %s",
				PMString(localname).GrabCString());
#endif
			isError = true;
		}
	}
	else
	{
		currentElement = PMString(localname).GrabCString();
		if (data.find(currentElement) != data.end())
		{
			LogLine("ERROR IN XMLConfigSAXContentHandler::StartElement() - element %s already exists", currentElement.c_str());
			isError = true;
			data.clear();
		}
		else
			data[currentElement] = "";
	}
}

/********************************************************************************************************************/
void XMLConfigSAXContentHandler::EndElement(const WideString &uri, const WideString &localname, 
																							const WideString &qname)
/********************************************************************************************************************/
{
	if (isError)
		return;

	currentElement.clear();
}

/********************************************************************************************************************/
void XMLConfigSAXContentHandler::Characters(const WideString &chars)
/********************************************************************************************************************/
{
	if (isError || currentElement.length() == 0)
		return;

	PMString charsStr(chars);
#ifdef CC
	const char * dataChunk = charsStr.GrabCString().c_str();
#else
	const char * dataChunk = charsStr.GrabCString();
#endif

	std::map<std::string, std::string>::iterator it = data.find(currentElement);
	it->second.append(dataChunk);
	//LogLine("XMLConfigSAXContentHandler::Characters() - data chunk appended: element = %s, value = %s",  
	//	it->first.c_str(), dataChunk);
}

/********************************************************************************************************************/
std::map<std::string, std::string>& XMLConfigSAXContentHandler::getData()
/********************************************************************************************************************/
{
	return data;
}
