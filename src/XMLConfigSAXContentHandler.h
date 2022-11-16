/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		XMLConfigSAXContentHandler.h																		*/
/*																													*/
/*	Created:																										*/
/*																													*/
/*	Modified:																										*/
/*																													*/
/********************************************************************************************************************/

#ifndef _XML_CONFIG_SAX_CONTENT_HANDLER
#define _XML_CONFIG_SAX_CONTENT_HANDLER

#include <map>
#include <CSAXContentHandler.h>


/********************************************************************************************************************/
class XMLConfigSAXContentHandler : public CSAXContentHandler
/********************************************************************************************************************/
{
private:
	bool isFirst;
	bool isError;
	std::string currentElement;
	std::map<std::string, std::string> data;

public:
	XMLConfigSAXContentHandler(IPMUnknown *boss) : CSAXContentHandler(boss), isFirst(true), isError(false) {}
	virtual ~XMLConfigSAXContentHandler() {}
	virtual void StartDocument(ISAXServices* saxServices) {}
	virtual void EndDocument() {}
	virtual void StartElement(const WideString &uri, const WideString &localname, const WideString &qname, ISAXAttributes* attrs);
	virtual void EndElement(const WideString &uri, const WideString &localname, const WideString &qname);
	virtual void Characters(const WideString &chars);
	virtual void ProcessingInstruction(const WideString &target, const WideString &wsdata) {}
	virtual void StartPrefixMapping(const WideString &prefix, const WideString &uri) {}
	virtual void EndPrefixMapping(const WideString &prefix) {}
	virtual void IgnorableWhitespace(const WideString &chars) {}
	virtual void SkippedEntity(const WideString &name) {}
	virtual void ExtComment(const WideString &comment) {}
	virtual void ExtXMLDecl(const WideString &version, const WideString &encoding,
		const WideString &standalone, const WideString &actualEncoding) {}
	std::map<std::string, std::string>& getData();
};
#endif