/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		ImageLinkUtils.h																					*/
/*																													*/
/*	Created:																										*/
/*																													*/
/*	Modified:																										*/
/*																													*/
/********************************************************************************************************************/

#ifndef _IMAGE_LINK_UTILS
#define _IMAGE_LINK_UTILS

#include <IDataLink.h>
#include <IDocument.h>
#include <vector>
#include <string>
#include <PMReal.h>
#include <IDFile.h>
#include <WideString.h> 

class ISpread;

class ImageLinkUtils
{
public:
	static int GetAllLinks(UIDList& links);
	static int RelinkItem(const UIDRef& linkRef, const IDFile& newFile);
	static int RelinkPictures(IDocument* doc, const WideString& newpath, std::list<char*>& names, bool logMissingPics);
	static UIDList GetAllLinksInSpread(UIDRef spreadID);
	static UIDRef GetDataLink(const UIDRef& splineItemRef);

private:
	static IDFile GetRelinkedIDFile(const WideString& path, const WideString& resourceName);
	static bool IsRelinkRequired( std::list<char*>& names, const WideString& resourceName);
};

bool HasLinkedImage( UIDRef item );
bool RelinkImage( const UIDRef& item, const IDFile& path );
bool PlaceImageIntoFrame( const IDFile& idfile, const UIDRef& frame );
bool PlaceImage( const IDFile& file, IDocument *document, int pagenum, const PMReal xpos, const PMReal ypos, bool checkSelection = false, bool dragAndDrop = false );
bool PlaceImageoOutsidePage ( const IDFile& file,IDocument *document, ISpread* spread, PMPoint mousePos, bool checkSelection, bool dragAndDrop );
bool PlaceImageIntoFrame( const char* file, const UIDRef& frame );
bool GetListOfSelectedFrames( const UIDRef& parentSpreadLayer, UIDList& selectedUIDs );
#endif
