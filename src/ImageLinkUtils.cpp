/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		ImageLinkUtils.cpp																					*/
/*																													*/
/*	Created:	19.09.08 hal																						*/
/*																													*/
/*	Modified:	28.01.09	tss	greatly changed: support CS4 with completely different link architecture			*/
/*								and fix 2 bugs in CS3:																*/
/*									1) embedded resource became unembedded after changing path						*/
/*									2) link was not really updated if directory was the same as original			*/
/*				15.12.09	tss	GetAllLinksInSpread() was added. It finds all links inside single spread			*/
/*				16.12.09	tss	RelinkPictures() was changed to find links inside text frames						*/
/*				17.04.10	tss	2 LEAKs in GetAllLinksInSpread()													*/
/*				31.05.10	tss	Added naught check for IHierarchy inside GetAllLinksInSpread()						*/
/*				11.04.14	bu	MAC & CS6																			*/
/*				18.06.14	bu  no replacement fo mutated vowels and sharp s ... anymore							*/
/*				12.02.15	bu	placing new image in existing image box: don't change link, delete it & reimport	*/
/*																													*/
/********************************************************************************************************************/

#include "precompiled.h"

#include <URI.h>
#include <ILinkFacade.h>
#include <LinkQuery.h>
#include <ILinkUtils.h>
#include <ILinkObject.h>
#include "ImageLinkUtils.h"
#include "PluginLogger.h"
#include "Functions.h" 
#include <IFrameContentFacade.h>
#include <IResizeItemsCmdData.h>
#include <ITransformCmdData.h>
#include <ITransformFacade.h>


/********************************************************************************************************************/
IDFile ImageLinkUtils::GetRelinkedIDFile( const WideString& path, const WideString& resourceName )
/********************************************************************************************************************/
{ 
	WideString wsfullname = ::GetUnicodeFullPath( path, resourceName );
	
	return IDFile(wsfullname);
}

/********************************************************************************************************************/
bool ImageLinkUtils::IsRelinkRequired(std::list<char*>& names, const WideString& resourceName)
/********************************************************************************************************************/
{
 	if (names.size() == 0)
		return true;

	for (std::list<char*>::iterator it = names.begin(); it != names.end(); ++it )
	{
		char* name = *it;
		if ( resourceName.compare( WideString(name) ) == 0)
		{
			return true;
		}
	}

	LogLine("ImageLinkUtils::IsRelinkRequired() finished, .. name wasn't found in list to relink" );
	return false;
}


/********************************************************************************************************************/
int ImageLinkUtils::GetAllLinks(UIDList& links)
/********************************************************************************************************************/
{
	IDataBase* db = links.GetDataBase();
	if (db == nil)
		return -1;

	InterfacePtr<ILinkManager> linkMgr(db, db->GetRootUID(), UseDefaultIID());
	LinkQuery query;
	query.SetLinkType(ILink::kImport);
	query.SetLinkType(ILink::kBidirectional);
	query.SetLinkType(ILink::kExport);

	int res = linkMgr->QueryLinks(query, kIDLinkClientID, links);
	///LogLine("ImageLinkUtils::GetAllLinks() - number of quered links = %d", res);
	return res;
}

/********************************************************************************************************************/
int ImageLinkUtils::RelinkItem(const UIDRef& itemRef, const IDFile& newFile)
/********************************************************************************************************************/
{	
	int result = kFailure;
	URI uri;

	bool convertRes = Utils<IURIUtils>()->IDFileToURI(newFile, uri);
	if (!convertRes)
	{
		LogLine("ERROR IN ImageLinkUtils::RelinkItem() - converting IDFile to URI failed");
		return result;
	}
	LogLine("ImageLinkUtils::RelinkItem() - new URI = %s", uri.GetURI().c_str());

	InterfacePtr<const ILink> theLink(itemRef, UseDefaultIID());

	UIDRef resourceRef(itemRef.GetDataBase(), theLink->GetResource());
	result = Utils<Facade::ILinkFacade>()->RelinkResource(resourceRef, uri, kSuppressUI);
	if (result != kSuccess)
	{
		LogLine("ERROR IN ImageLinkUtils::RelinkItem() - relinking error code = %d", result);
		return result;
	}

	UID dummy;
	result = Utils<Facade::ILinkFacade>()->UpdateLink(itemRef, true, kSuppressUI, dummy);
	if (result != kSuccess)
	{
		LogLine("ERROR IN ImageLinkUtils::RelinkItem() - updating error code = %d", result);
		return result;
	}

	return result;
}

/********************************************************************************************************************/
UIDRef ImageLinkUtils::GetDataLink(const UIDRef& splineItemRef)
/********************************************************************************************************************/
{
	UIDRef dataLinkRef = UIDRef::gNull;

	return dataLinkRef;
}

/********************************************************************************************************************/
int ImageLinkUtils::RelinkPictures(IDocument* doc, const WideString& newPath, std::list<char*>& names, bool logMissingPics)
/********************************************************************************************************************/
{
	PMString newP = PMString(newPath);

	/// old
	/// std::string s = newP.GetPlatformString();
	/// const char *cs = s.c_str();

	/// new
	std::string lnkstr = newP.GrabCString();
	const char *cs = lnkstr.c_str();

//	LogLine("ImageLinkUtils::RelinkPictures() started with path = %s", PMString(newPath).GetPlatformString().c_str() );
	LogLine("ImageLinkUtils::RelinkPictures() started with path = %s", cs);

	if ( !doc )	
		return -1;

	IDataBase* db = ::GetDataBase(doc);
	if ( !db )	
		return -1;


	UIDList links(doc);
	int res = GetAllLinks(links);
	if (res < 0) 
		return -1;

	res = 0;
	for(int linkIndex = 0; linkIndex < links.size(); ++linkIndex)
	{
		InterfacePtr<const ILink> theLink( links.GetRef(linkIndex), UseDefaultIID() );
		if ( !theLink ) 
			continue;

		InterfacePtr<ILinkResource> linkResource(db, theLink->GetResource(), UseDefaultIID());
		if ( !linkResource ) 
			continue;

		WideString resourceName(linkResource->GetShortName(true));

		// check the relinking is  required for this link
		if ( !IsRelinkRequired( names, resourceName ) )
			continue;
		
		WideString wsfullname = ::GetUnicodeFullPath( newPath, resourceName );	
		IDFile newFile;
		FileUtils::PMStringToIDFile(PMString(wsfullname), newFile);

		if ( !SDKFileHelper(newFile).IsExisting()) {
			if (logMissingPics) {
				LogLine("File %s doesn't exist, relinking will not be performed", 
#ifndef WINOS
				newFile.GetFileName().GrabCString());
#else
#ifdef CC
				newFile.GetString().GrabCString().c_str());
#else
				newFile.GetString().GrabCString());
#endif
#endif
				
			}
			continue;
		}
		if ( RelinkItem( links.GetRef(linkIndex), newFile ) != kSuccess )
			--res;
	}
	return res;

}

/********************************************************************************************************************/
UIDList ImageLinkUtils::GetAllLinksInSpread(UIDRef spreadID)
/********************************************************************************************************************/
{
	InterfacePtr<IHierarchy> hier( spreadID, UseDefaultIID() );
	IDataBase* db = spreadID.GetDataBase();

	UIDList list( db );

	UIDList graphicList( db );

	hier->GetDescendents( &list, IID_IDATALINKREFERENCE );
	hier->GetDescendents( &graphicList, IID_IGRAPHICFRAMEDATA );

	for( int i = 0; i < graphicList.Length(); ++i )
	{
		InterfacePtr<IGraphicFrameData> gf( graphicList.GetRef( i ), UseDefaultIID()  );
		if( !gf ) 
			continue;

		// LEAK
		InterfacePtr<IMultiColumnTextFrame> textFrame( gf->QueryMCTextFrame() );
		if( !textFrame ) 
			continue;

		// LEAK
		InterfacePtr<ITextModel> tm( textFrame->QueryTextModel() );
		if( !tm ) 
			continue;

		InterfacePtr<IItemStrand> itemStrand( (IItemStrand*)tm->QueryStrand(kOwnedItemStrandBoss, IItemStrand::kDefaultIID) );
		if( !itemStrand ) 
			continue;

		UIDList strandList( db );
		ClassList clList;
		clList.push_back( IID_IDATALINKREFERENCE );

		itemStrand->GetAllItems( 0, tm->TotalLength(), &strandList, &clList );

		for( int j = 0; j < strandList.Length(); ++j )
		{
			UIDList addedLinks( db );
			InterfacePtr<IHierarchy> hier2( db, strandList[ j ], UseDefaultIID() );
			// KLIM 2010 05 31
			if( !hier2 ) 
				continue;

			hier2->GetDescendents( &addedLinks, IID_IDATALINKREFERENCE );
			list.Append( addedLinks );
		}
	}

	return list;
}

/********************************************************************************************************************/
void TruncateSpaces( std::string& val )
/********************************************************************************************************************/
{
	if( val.empty() ) 
		return;

	size_t pos;

	pos = val.find_first_not_of(" ");
	if( pos == std::string::npos )
	{
		val.clear();
		return;
	}
	val = val.substr( pos );

	// truncate the end spaces
	pos = val.find_last_not_of(" ");
	if( pos == std::string::npos )
	{
		val.clear();
		return;
	}
	val = val.substr( 0,pos+1 );
}

/********************************************************************************************************************/
bool HasLinkedImage( UIDRef item )
/********************************************************************************************************************/
{
	// check the item
	InterfacePtr<ILinkObject> linkobj( item, UseDefaultIID() );
	if( !linkobj ) 
		return false;

	UIDList  links = linkobj->GetLinks();
	if( !links.IsEmpty() ) 
		return true;

	// check it's childs
	InterfacePtr<IHierarchy> hier( item, UseDefaultIID() );
	if( !hier ) 
		return false;

	UIDList childs( item.GetDataBase() ); 

	hier->GetDescendents( &childs, IID_ILINKOBJECT );
	for( int32 i = 0; i < childs.size(); i++ )
	{
		UIDRef ref = childs.GetRef(i);
		if( ref == item ) 
			continue;

		if( HasLinkedImage( ref ) )  
			return true;
	}

	return false;
}

/********************************************************************************************************************/
bool RelinkImage( const UIDRef& item, const IDFile& path )
/********************************************************************************************************************/
{
	if( !item ) 
		return false;

	// get a link  & link's resource
	InterfacePtr<const ILink> theLink( item, UseDefaultIID());
	if( !theLink ) 
		return false;

	UIDRef resource( item.GetDataBase(), theLink->GetResource() );
	InterfacePtr<ILinkResource> linkResource( resource, UseDefaultIID());
	if( !linkResource ) 
		return false;

	//  image file was changed, but old info is still presented in cache. 
	Utils<Facade::ILinkFacade>()->UpdateResourceStates( resource, ILinkManager::kSynchronous );

	// get a path inside the link resource
	PMString linkPath( linkResource->GetLongName(true));

	std::string lnkstr = linkPath.GrabCString();
	TruncateSpaces( lnkstr );
	//std::string pastr = path.GetString().GrabCString();
	std::string pastr = path.GetFileName().GrabCString();
	TruncateSpaces( pastr );

	// check the path is empty or equal by old one
	if( pastr.empty() ) 
		return false;

	if( pastr.compare(lnkstr) == 0 ) 
		return false;

	URI uri;
	if ( !Utils<IURIUtils>()->IDFileToURI(path, uri) ) 
		return false;

	if( !Utils<Facade::ILinkFacade>()->CanRelinkResource( resource ) )	
		return false;

	int err = Utils<Facade::ILinkFacade>()->RelinkResource( resource, uri, kSuppressUI );
	if( err != kSuccess ) 
		return false;

	UID dummy;
	err =  Utils<Facade::ILinkFacade>()->UpdateLink( item, true, kSuppressUI, dummy);
	return  (err == kSuccess) ? true : false ;
}

/********************************************************************************************************************/
bool  ConvertText2GraphicFrame( UIDRef& item )
/********************************************************************************************************************/
{
	if( !item ) 
		return false;

	// the frame which contain a picture couldn't be converted
	if( HasLinkedImage( item ) )  
		return true;

	InterfacePtr<IFrameType> type( item, UseDefaultIID() );
	if( !type ) 
		return true;

	if( type->IsNotAFrame() ) 
		return true;

	if( type->IsGraphicFrame() ) 
		return true;

	// ... convert ...
	InterfacePtr<ICommand> cmd( CmdUtils::CreateCommand( kConvertItemToFrameCmdBoss ) );
	if( !cmd ) 
		return false;

	cmd->SetItemList( UIDList(item) );
	if( CmdUtils::ProcessCommand( cmd ) != kSuccess ) 	
		return false;

	// get new frame as a  result 
	const UIDList* pReslist =  cmd->GetItemList();
	item = pReslist->GetRef(0);

	return true;
}

/********************************************************************************************************************/
bool GetListOfSelectedFrames( const UIDRef& parentSpreadLayer, UIDList& selectedUIDs )
/********************************************************************************************************************/
{
	selectedUIDs.Clear();
	if( !parentSpreadLayer ) 
		return false;

	IDataBase* db = parentSpreadLayer.GetDataBase();
	if( !db ) 
		return false;

	ISelectionManager* selMgr = Utils<ISelectionUtils>()->GetActiveSelection();
	if( !selMgr) 
		return false;

	InterfacePtr<ILayoutSelectionSuite> layselSuite(selMgr, UseDefaultIID());
	if( !layselSuite || layselSuite->IsLayoutSelectionEmpty( )  ) 
		return false;

	UIDList childs( db );
	if( !FrameUtils::GetChildFrames(  parentSpreadLayer, childs ) ) 
		return false;

	for( int i = 0; i< childs.size(); i++ )
	{
		UID  uid = childs[i];
		if( layselSuite->IsPageItemSelected( uid ) )
			selectedUIDs.Append( uid );
	}
	return true;
}

/********************************************************************************************************************/
bool PlaceImage( const IDFile& file, IDocument *document, int pagenum, 
				const PMReal x, const PMReal y, bool checkSelection, bool dragAndDrop )
/********************************************************************************************************************/
{
	if( !document ) 
		return false; 

	std::string path = file.GetString().GetPlatformString();
//	std::string path = file.GetFileName().GetPlatformString();
	LogLine("PlaceImage() - file path = %s", path.c_str() );

	//Check path
	SDKFileHelper fHelper(file);
	if ( fHelper.IsExisting() == kFalse )
	{
		LogLine("ERROR: in PlaceImage() - file isn't exist");
		return false;
	}

	// Get the document's database
	IDataBase* db = ::GetDataBase(document);
	if( !db ) 
		return false;

	//Get pgNum-th page UIDRef
	InterfacePtr<IPageList> pageList( document, UseDefaultIID());
	if( !pageList ) 
		return false;

	UIDRef pageUIDRef( db, pageList->GetNthPageUID(pagenum) );
	if( !pageUIDRef ) 
		return false;

	//By default (0,0) is left corner of margins, get margins to fix it
	PMRect margins;
	getMargins(document, pagenum, &margins);
	PMReal xpos = x - margins.Left();
	PMReal ypos = y - margins.Top();

	//Convert page coordinates to spread coordinates
	SDKLayoutHelper layoutHelper;
	PMRect boundsInPageCoords(xpos, ypos, xpos+100, ypos+100);
	PMRect boundsInParentCoords = layoutHelper.PageToSpread(pageUIDRef, boundsInPageCoords);

	// Get the list of document layers &spread list
	InterfacePtr<ILayerList> layerList(document, IID_ILAYERLIST);
	if( !layerList ) 
		return false;

	InterfacePtr<ISpreadList> spreadList(document, UseDefaultIID());
	if( !spreadList ) 
		return false;

	// Get the spread
	UIDRef spreadUIDRef( db, spreadList->GetNthSpreadUID(pagenum) );
	if( !spreadUIDRef ) 
		return false;

	InterfacePtr<ISpread> spread(spreadUIDRef, UseDefaultIID());
	if( !spread ) 
		return false;

	// Get the document layer
	int32 spreadLayerPosition;

	UIDRef activeLayerRef(layerList->GetNextActiveLayer(int32(0)));
	InterfacePtr<IDocumentLayer> documentLayer(activeLayerRef, UseDefaultIID());
	
	if( !documentLayer ) 
		return false;

	// Get the content spread layer associated with the document layer
	InterfacePtr<ISpreadLayer> contentSpreadLayer(spread->QueryLayer(documentLayer,
		&spreadLayerPosition, kFalse));
	if( !contentSpreadLayer ) 
		return false;

	InterfacePtr<IHierarchy> contentSpreadLayerHierarchy((ISpreadLayer*)contentSpreadLayer, IID_IHIERARCHY);
	if( !contentSpreadLayerHierarchy ) 
		return false;

	// get UID of current spread layer
	UIDRef spLayerRef = ::GetUIDRef(contentSpreadLayerHierarchy);
	do 
	{
		if( !checkSelection ) 
			break;
		
		UIDList  selectedUIDs( db );
		// get a list of selected frames on the layer
		if( !::GetListOfSelectedFrames( spLayerRef, selectedUIDs ) ) break;
		if( selectedUIDs.size() != 1 )
		{
			LogLine("PlaceImage: selected more one frame ");
			break;
		}

		// get the selected frame & convert into a graphic( spline) frame
		UIDRef frameRef = selectedUIDs.GetRef(0);
		ConvertText2GraphicFrame( frameRef );
		if( !frameRef ) 
			break;

		// put an image into the frame
		return ::PlaceImageIntoFrame( file, frameRef ) ;
	} 
	while ( false);

	do 
	{
		
		if ( !dragAndDrop ) 
			break;
		//...no selected frame or a group is selected and PlaceImage is called from ProcessDragDropCommand
		// search frame under the mouse pointer 
		UIDRef frame;
		if ( !FrameUtils::GetFrameByCoordinate(   spLayerRef,  frame,  boundsInParentCoords.LeftTop() ) )
			break;

		ConvertText2GraphicFrame( frame );
		if( !frame ) 
			break;

		// put an image into the frame
		return ::PlaceImageIntoFrame( file, frame ) ;
	} while(false);

	// ...no selected frame or a group is selected ..
	// create new frame on the layer & put an image inside the frame.
	//
	LogLine("PlaceImage: place image into new frame ... ");

	UIDRef frameRef = layoutHelper.PlaceFileInFrame(fHelper.GetIDFile(), spLayerRef, 
		boundsInParentCoords, K2:: kSuppressUI);
	if( !frameRef ) 
		return false;

	return true;
}

/********************************************************************************************************************/
bool PlaceImageoOutsidePage ( const IDFile& file,IDocument *document, ISpread* spread, PMPoint mousePos, 
							 bool checkSelection, bool dragAndDrop )
/********************************************************************************************************************/
{
	if (! document ) 
		return false;

	if (!spread ) 
		return false;

	//Check path
	SDKFileHelper fHelper(file);
	if ( fHelper.IsExisting() == kFalse )
	{
		LogLine("ERROR: in PlaceImage() - file isn't exist");
		return false;
	}

	IDataBase* db = GetDataBase( document );
	if (!db ) 
		return false;

	InterfacePtr<ILayerList> layerList(document, IID_ILAYERLIST);
	if( !layerList ) 
		return false;

	// Get the document layer
	int32 spreadLayerPosition;
	UIDRef activeLayerRef(layerList->GetNextActiveLayer(int32(0)));
	InterfacePtr<IDocumentLayer> documentLayer(activeLayerRef, UseDefaultIID());
	if( !documentLayer ) 
		return false;

	// Get the content spread layer associated with the document layer
	InterfacePtr<ISpreadLayer> contentSpreadLayer(spread->QueryLayer(documentLayer,
		&spreadLayerPosition, kFalse));
	if( !contentSpreadLayer ) 
		return false;

	InterfacePtr<IHierarchy> contentSpreadLayerHierarchy((ISpreadLayer*)contentSpreadLayer, IID_IHIERARCHY);
	if( !contentSpreadLayerHierarchy ) 
		return false;

	// get UID of current spread layer
	UIDRef spLayerRef = ::GetUIDRef(contentSpreadLayerHierarchy);
	do
	{
		if( !checkSelection ) 
			break;

		UIDList  selectedUIDs( db );
		// get a list of selected frames on the layer
		if( !::GetListOfSelectedFrames( spLayerRef, selectedUIDs ) ) break;
		if( selectedUIDs.size() != 1 )
		{
			LogLine("PlaceImage: selected more one frame ");
			break;
		}

		// get the selected frame & convert into a graphic( spline) frame
		UIDRef frameRef = selectedUIDs.GetRef(0);
		ConvertText2GraphicFrame( frameRef );
		if( !frameRef ) break;

		// put an image into the frame
		return ::PlaceImageIntoFrame( file, frameRef ) ;
	} while ( false);

	do
	{
		if ( !dragAndDrop ) 
			break;

		//...no selected frame or a group is selected and PlaceImage is called from ProcessDragDropCommand
		// search frame under the mouse pointer
		UIDRef frame;
		if ( !FrameUtils::GetFrameByCoordinate(   spLayerRef,  frame,  mousePos ) )
			break;

		ConvertText2GraphicFrame( frame );
		if( !frame )
			break;

		// put an image into the frame
		return ::PlaceImageIntoFrame( file, frame ) ;
	} while(false);

	// ...no selected frame or a group is selected ..
	// create new frame on the layer & put an image inside the frame.

	::TransformPasteboardPointToInner(spread, &mousePos);
	PMRect boundsInSpreadCoords(mousePos.X(), mousePos.Y(), mousePos.X()+100, mousePos.Y()+100);

	SDKLayoutHelper layoutHelper;

	UIDRef frameRef = layoutHelper.PlaceFileInFrame(fHelper.GetIDFile(), spLayerRef,
		boundsInSpreadCoords, K2:: kSuppressUI);
	if( !frameRef ) 
		return false;

	return true;
}

/********************************************************************************************************************/
bool PlaceImageIntoFrame( const IDFile& idfile, const UIDRef& my_frame )
/********************************************************************************************************************/
{
	UIDRef frame = my_frame;
	if( !frame ) 
		return false;

	IDataBase* db = frame.GetDataBase();
	if( !db ) 
		return false;

	URI uri;
	if( !FileUtils::CanOpen( idfile, FileUtils::kRead ) ) 
		return false;

	if( !Utils<IURIUtils>()->IDFileToURI( idfile, uri ) ) 
		return false;

	PMRect oldImageBox;
	PMReal angle = 0;

	UID linkUid = Utils<ILinkUtils>()->FindLink( frame );
	if( linkUid != kInvalidUID )
	{
		/*
		This would lead to replacing all links on page: 
		return RelinkImage( UIDRef( db, linkUid ), idfile );
		*/

		/* delete the link: */
		UIDRef linkRef (db, linkUid);
		InterfacePtr <ILink> link(linkRef, UseDefaultIID());

		const UIDRef objRef (db, link->GetObject());

		// preserve rotation angle of old image frame
		InterfacePtr<ITransform> transform( objRef, UseDefaultIID() );
		if ( transform )
		{
			angle = 360 - transform->GetItemRotationAngle();
			FrameUtils::Rotate( objRef, -angle );
		}

		// preserve position of old image frame
		oldImageBox = Utils<Facade::IGeometryFacade>()->GetItemBounds( objRef, Transform::PasteboardCoordinates(), Geometry::OuterStrokeBounds() );

		InterfacePtr<ICommand> deleteCmd( CmdUtils::CreateCommand( kDeleteCmdBoss ) );
		if( deleteCmd )
		{
			const UIDList l(db, objRef.GetUID());
			deleteCmd->SetItemList( l );
			CmdUtils::ProcessCommand( deleteCmd );
		}		
	}

	/* import the picture: */
	InterfacePtr<ICommand> importCmd( CmdUtils::CreateCommand( kImportResourceCmdBoss ) );
	if( !importCmd ) 
		return false;

	InterfacePtr<IImportResourceCmdData> importCmdData( importCmd, IID_IIMPORTRESOURCECMDDATA ); // no kDefaultIID
	if( !importCmdData ) 
		return false;

	importCmdData->Set( frame.GetDataBase(), uri, kSuppressUI );

	if( CmdUtils::ProcessCommand( importCmd ) != kSuccess ) 
		return false;

	InterfacePtr<ICommand> placecmd( CmdUtils::CreateCommand( kPlaceItemInGraphicFrameCmdBoss ) );
	if( !placecmd ) 
		return false;

	placecmd->SetItemList( importCmd->GetItemListReference() );

	InterfacePtr<IPlacePIData> placePIData( placecmd, UseDefaultIID() );
	if( !placePIData ) 
		return false;

	placePIData->Set( frame, 0, kFalse );

	ErrorCode result = CmdUtils::ProcessCommand( placecmd );

	if ( !oldImageBox.IsEmpty() )
	{
		// restore image box settings

		// find new link
		UID newLinkUid = Utils<ILinkUtils>()->FindLink( frame );
		if ( newLinkUid != kInvalidUID )
		{
			InterfacePtr <ILink> newLink( db, newLinkUid, UseDefaultIID() );

			const UIDRef newObjRef( db, newLink->GetObject() );

			FrameUtils::Resize( newObjRef, oldImageBox.Width(), oldImageBox.Height());

			PMRect splineBox( Utils<Facade::IGeometryFacade>()->GetItemBounds( frame, Transform::PasteboardCoordinates(), Geometry::PathBounds() ) );
			FrameUtils::MoveRelative( newObjRef, oldImageBox.Left() - splineBox.Left(), oldImageBox.Top() - splineBox.Top() );

			FrameUtils::Rotate( newObjRef, angle );
		}
	}

	return ( result == kSuccess ) ? true : false;
}
