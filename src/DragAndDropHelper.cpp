/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		DragAndDropHelper.cpp																				*/
/*																													*/
/*	Created:	22.08.07	hal																						*/
/*																													*/
/*	Modified:	28.05.08	tss	Final solution for Mac DND problem: read URL-info from Mac-specific DragReference	*/
/*								(see getURLString())																*/
/*								Huge changes in functions CouldAcceptTypes(), compareAndExtract(), getObjectURL()	*/
/*				10.11.08	tss	changes: constructor - to receive config; compareAndExtract() splitted into			*/
/*								isValidURL() + extractAlfrescoImageId() + extractMediaStoreImageId()				*/
/*				01.10.14	bu	dropping pictures with 'ü' in name crashes InDesign; Now checking clipboard for		*/
/*									such character (Windows)														*/
/*				17.03.17	i42	not only 'ü' in picture filenames causes problems, so we call the workaround for	*/
/*									all chars > 0x7f
/********************************************************************************************************************/

#include "precompiled.h"

extern CMyStream sStreamOut2;
const char * const HOST_NAME = "mediastorebild";
void LogLineClipboardImageType();

/********************************************************************************************************************/
class DragAndDropHelper : public CDragDropTargetFlavorHelper
/********************************************************************************************************************/
{
private:
	mutable std::string urlString;
	mutable std::string imageId;
	DocDesc* getDocumentDescriptionFor(const IDragDropTarget* target) const;
	PluginConfiguration config;

#ifdef WINOS
	void getObjectURL(IPMDataObject *dataObject) const;
#else
	void getObjectURL(DragReference &dragReference) const;
#endif

	int32 findTargetPageIndexWithMousePos(const IDragDropTarget* target, const IDragDropController* controller,  
											PMPoint& pageMousePos);
	bool getPasteboardMousePos(const IDragDropTarget* target, const IDragDropController* controller, 
											PMPoint& pasteboardMousePos);
	UIDRef getTargetSpreadUID(const IDragDropTarget* target, PMPoint pasteboardMousePos);
	bool isValidURL() const;
	bool extractMediaStoreImageId() const;
	bool extractAlfrescoImageId() const;
	bool GetPathFromClipboard( std::wstring& path ); 

public:
	DragAndDropHelper(IPMUnknown* boss);
	virtual	~DragAndDropHelper() {};

	/*	Determines whether we can handle the flavors in the drag.
		@param target IN the target the mouse is currently over.
		@param dataIter IN iterator providing access to the data objects within the drag.
		@param fromSource IN the source of the drag.
		@param controller IN the drag drop controller mediating the drag.
		@return a target response ( DragDrop::TargetResponse )-
			Represents the response from a Drag Drop Target when it is 
			asked whether or not it could accept a drag. */
	virtual DragDrop::TargetResponse CouldAcceptTypes(const IDragDropTarget* target, DataObjectIterator* dataIter,
					const IDragDropSource* fromSource, const IDragDropController* controller) const ;

	/*	Performs the actual drag.
		@param target IN the target for this drop.
		@param controller IN the drag drop controller that is mediating the drag.
		@param action IN what the drop means (i.e. copy, move etc)
		@return kSuccess if the drop is executed without error, kFailure, otherwise. */
	virtual ErrorCode ProcessDragDropCommand(IDragDropTarget* target,
						IDragDropController* controller, DragDrop::eCommandType action);

	DECLARE_HELPER_METHODS()
};

CREATE_PMINTERFACE(DragAndDropHelper, kDragAndDropHelperImpl)
DEFINE_HELPER_METHODS(DragAndDropHelper)


/********************************************************************************************************************/
DragAndDropHelper::DragAndDropHelper(IPMUnknown* boss) : CDragDropTargetFlavorHelper(boss), HELPER_METHODS_INIT(boss)
/********************************************************************************************************************/
{
	config = XMLConfigurationReader::getInstance()->GetPluginConfiguration();
	if (!config.isInitialized())
		LogLine("ERROR IN DragAndDropHelper constructor - XML configuration file not read correctly, use defaults...");
}

/********************************************************************************************************************/
DragDrop::TargetResponse DragAndDropHelper::CouldAcceptTypes(const IDragDropTarget* target,
															 DataObjectIterator* dataIter, const IDragDropSource* fromSource, 
															 const IDragDropController* controller) const
/********************************************************************************************************************/
{
	if( !target || !dataIter ) return DragDrop::kWontAcceptTargetResponse;
	if( fromSource ) return DragDrop::kWontAcceptTargetResponse;

	// get a target document
	DocDesc *dd = getDocumentDescriptionFor( target );
	if( !dd ) 
	{
		// it is not plugged document
		return DragDrop::kDropWillCopy;    
	}

	//--------------------------------
	// check for URL from alfresco
	//--------------------------------
	DataExchangeResponse response =  dataIter->FlavorExistsWithPriorityInAllObjects( kURLExternalFlavor );
	if( response.CanDo() ) 
	{
		IPMDataObject *firstObject = dataIter->First();
		if ( !firstObject )
		{
			return DragDrop::kWontAcceptTargetResponse;
		}

		// get a URL from dragged object
#ifdef WINOS
		getObjectURL(firstObject);
#else
		DragReference dragRef;
		controller->GetDragReference(&dragRef);
		getObjectURL(dragRef);
#endif
		// checking for if host name equals to media store host name (HOST_NAME)
		if (isValidURL() && (extractMediaStoreImageId() || extractAlfrescoImageId()))
		{
			LogLine("DragAndDropHelper::CouldAcceptTypes()  - this is MediaStore/Alfresco link, drag will be accepted");
			return DragDrop::TargetResponse(DataExchangeResponse(kTrue, kURLExternalFlavor, kNormalFlavorFlag,
				kHighestFlavorPriority), DragDrop::kDropWillCopy);
		}

		return DragDrop::kWontAcceptTargetResponse;
	}

	//----------------------------------------
	// check for a file is dragged from folder
	//----------------------------------------
	response = dataIter->FlavorExistsWithPriorityInAllObjects( kDesktopExternalFlavor );
	if( response.CanDo() )
	{
		return DragDrop::TargetResponse(response, DragDrop::kDropWillCopy); 
	}

	return DragDrop::kWontAcceptTargetResponse;
}


/********************************************************************************************************************/
ErrorCode DragAndDropHelper::ProcessDragDropCommand(IDragDropTarget* target, 
													IDragDropController* controller, DragDrop::eCommandType	action)
/********************************************************************************************************************/
{
	if (action != DragDrop::kDropCommand) 
		return kSuccess;

	// get a target document
	DocDesc *dd = getDocumentDescriptionFor( target );
	if( !dd ) 
		return kSuccess;     // it is not plugged document

	IPMDataObject* pObj = controller->GetDragItem( 1 );
	if( !pObj ) {
		LogLine("Failure: controller->GetDragItem( 1 ) is null");
		return kFailure;
	}

	//----------------------------------------------
	// find a page & an inside mouse position 
	//----------------------------------------------
	PMPoint pageMousePos;
	int32 pageIdx =  findTargetPageIndexWithMousePos( target, controller, pageMousePos );

	if(pObj->FlavorExists( kURLExternalFlavor ) && pageIdx != -1 )
	{
		// the returned index must be 1-based for client application
		pageIdx++ ;

		LogLine("DragAndDropHelper::ProcessDragDropCommand()  - keyFileName = %s", imageId.c_str());

		return kSuccess;
	}

	/* entry point for our drop action: */
	if( pObj->FlavorExists( kDesktopExternalFlavor ) )
	{
		ErrorCode  err = kSuccess;

		///LogLineClipboardImageType();

		std::wstring clip;
		bool ok = DragAndDropHelper::GetPathFromClipboard( clip );

		// ok is true means: I got something to handle, here: path with 'ü' inside
		if (ok) {
			IDFile idf;

			WideString ws( clip.c_str());

			idf.SetPath( ws );

			bool res;
			if (pageIdx == -1)
			{
				InterfacePtr<IControlView> view(target, UseDefaultIID());
				if (!view)
					return kFailure;

				PMPoint pt = Utils<ILayoutUIUtils>()->GlobalToPasteboard(view, controller->GetDragMouseLocation());

				InterfacePtr<ISpread> spread(Utils<IPasteboardUtils>()->QueryNearestSpread(view, pt));
				if (!spread)
					return false;

				res = PlaceImageoOutsidePage(idf, dd->getDoc(), spread, pt, true, true);
			}
			else
			{
				res = PlaceImage(idf, dd->getDoc(), pageIdx, pageMousePos.X(),
					pageMousePos.Y(), true, true);
			}
			return res ? kSuccess : kFailure;
		}

		if( pObj->GetUsage() == IPMDataObject::kInternalizeUsage )
		{
			do 
			{
				if (controller->NeedsToInternalize())
					err = controller->InternalizeDrag( kDesktopExternalFlavor, kPMSysFileFlavor );

				if( err != kSuccess ) {
					LogLine("controller->InternalizeDrag() failed");
					break;
				}

				InterfacePtr<IDataExchangeHandler> handler(controller->QueryTargetHandler());
				if( !handler ) {
					LogLine("IDataExchangeHandler failed");
					break;
				}

				InterfacePtr<ISysFileListData> data( handler, UseDefaultIID() );
				if( !data || data->Length() < 1 ) {
					LogLine("data->Length() < 1 ");
					break;
				}

				const IDFile& dropfile = data->GetSysFileItem( 0 );
#ifdef WINOS
				std::string fpath = dropfile.GetString().GetPlatformString();
#else
				std::string fpath = dropfile.GetFileName().GetPlatformString();
#endif
				LogLine("DragAndDropHelper::ProcessDragDropCommand, file = %s", fpath.c_str() );

				IDFile file = dropfile;

				//Check path
				SDKFileHelper fHelper(dropfile);
				if ( fHelper.IsExisting() == kFalse )
				{
					LogLine("ERROR: in ProcessDragDropCommand() - file doesn't exist, check clipboard");
					// try to get the path from clipboard
					std::wstring clipPath;
					bool rs = DragAndDropHelper::GetPathFromClipboard( clipPath );
					if( /* !rs || */ clipPath.empty() )
					{
						// 
						LogLine("ERROR: in ProcessDragDropCommand() - attempt to get path from clipboard failed.");
						break;
					}

					WideString wpath( clipPath.c_str() );

					file.SetPath( wpath );
				}

				bool res;
				if (pageIdx == -1)
				{
					InterfacePtr<IControlView> view( target, UseDefaultIID() );
					if( !view ) 
						return kFailure;

					PMPoint pt = Utils<ILayoutUIUtils>()->GlobalToPasteboard( view, controller->GetDragMouseLocation() );

					InterfacePtr<ISpread> spread( Utils<IPasteboardUtils>()->QueryNearestSpread( view, pt ) );
					if( !spread ) 
						return false;

					res = PlaceImageoOutsidePage(file,dd->getDoc(), spread, pt, true, true );
				}
				else
				{
					res = PlaceImage( file, dd->getDoc(), pageIdx, pageMousePos.X(),
						pageMousePos.Y(), true, true );
				}
				return res ? kSuccess : kFailure;
				
			} while ( false );
		}

	}

	return kFailure;
}

/********************************************************************************************************************/
void LogLineClipboardImageType()
/********************************************************************************************************************/
{
#if WINOS
	HGLOBAL  globMem;
	wchar_t  thePicPath[512];

	if (!CountClipboardFormats())
		return ; 

	if (!OpenClipboard(NULL))
		return ;

	globMem = GetClipboardData(CF_HDROP);
	if (globMem)
	{
		DragQueryFileW( (HDROP)globMem, 0, (LPWSTR)thePicPath, 512);

		// convert to multibyte
		//wchar_t* pWide = (wchar_t*)thePicPath.get();
		//size_t	len = wcslen( thePicPath);

		int len2 = WideCharToMultiByte(CP_ACP, 0, thePicPath, 512, 0, 0, NULL, NULL);

		PSTR pCharStr = (PSTR) HeapAlloc( GetProcessHeap(), 0,	len2 * sizeof(CHAR));
		if ( pCharStr != NULL )
		{
			WideCharToMultiByte(CP_ACP, 0, thePicPath, 512, pCharStr, len2, NULL, NULL);
			LogLine("DoClipboardWin() CF_HDROP data = %s", pCharStr);
			HeapFree(GetProcessHeap(), 0, pCharStr); 
		}        
	}
	else
	{
		LogLine("DoClipboardWin() hasn't CF_HDROP data " );
	}

	CloseClipboard () ;
#endif
} 

/********************************************************************************************************************/
int32  DragAndDropHelper::findTargetPageIndexWithMousePos(const IDragDropTarget* target,
														  const IDragDropController* controller,  PMPoint& pageMousePos)
/********************************************************************************************************************/
{
	int32 foundPageIdx = -1;

	// get a mouse cursor position at the pasteboard coordinates
	PMPoint pasteboardMousePos;
	if( !getPasteboardMousePos(target,controller,pasteboardMousePos) )
	{
		return foundPageIdx;
	}

	// Determine the target spread to drop the items into.
	UIDRef spreadRef = getTargetSpreadUID(target,pasteboardMousePos);
	if( spreadRef.GetUID() == kInvalidUID )
	{
		return foundPageIdx;
	}

	// find a page & one's index  by a mouse position
	InterfacePtr<ISpread> spread(spreadRef, UseDefaultIID());
	
	UID   foundPageUID = 0;
	
	for (int32 pageIndex = 0; pageIndex < spread->GetNumPages(); pageIndex++)
	{
		// return the IGeometry of the  page.
		IGeometry* pageGeometry = spread->QueryNthPage(pageIndex);

		// the page bounding box in the PAGE coordinates
		PMRect pb = pageGeometry->GetStrokeBoundingBox();

		//Convert the page rectangle coordinates into the pasteboard coordinates
		::TransformInnerRectToPasteboard (pageGeometry, &pb);

		if ( pasteboardMousePos.ConstrainTo( pb ) == kFalse )
		{
			//Convert the mouse coordinates into the page coordinates
			pageMousePos = pasteboardMousePos; 
			::TransformPasteboardPointToInner(pageGeometry, &pageMousePos);


			// get the UID of the indexed page ;
			foundPageUID = spread->GetNthPageUID(pageIndex);
			break;
		}
	}

	InterfacePtr<ILayoutControlData> layoutData(target, UseDefaultIID());
	if ( CheckCreation("DragAndDropHelper::findTargetPageIndexWithMousePos()", layoutData, "layoutData", 0) )
		return foundPageIdx;

	InterfacePtr<IPageList> pagesList( layoutData->GetDocument(), IID_IPAGELIST  );
	if(pagesList != nil)
	{
		foundPageIdx = pagesList->GetPageIndex(foundPageUID);
	}

	return foundPageIdx;
}

/********************************************************************************************************************/
bool DragAndDropHelper::getPasteboardMousePos(const IDragDropTarget* target,
									  const IDragDropController* controller, PMPoint& pasteboardMousePos  )
/********************************************************************************************************************/
{
	// check the parameters
	if( target == nil ||  controller ==  nil )
	{
		LogLine("DragAndDropHelper::getPasteboardMousePos() - target or controller is null");
		return false;
	}

	InterfacePtr<IControlView> layoutView(target, UseDefaultIID());
	if (!layoutView)
	{
		LogLine("DragAndDropHelper::getPasteboardMousePos() - layoutView is null");
		return false;
	}

	// get a global coordinates of a mouse cursor & convert one to pasteboard coordinates
	GSysPoint where = controller->GetDragMouseLocation();
	pasteboardMousePos = Utils<ILayoutUIUtils>()->GlobalToPasteboard(layoutView, where);

	return true;
}

/********************************************************************************************************************/
bool DragAndDropHelper::isValidURL() const
/********************************************************************************************************************/
{
	LogLine("DragAndDropHelper::isValidURL(), urlString = %s", urlString.c_str());	

	std::string::size_type pos = urlString.find("//");
	int pos2 = (int) urlString.find_first_of(":/", pos + 2);
	if (urlString.size() < 8 		// strlen("http://") or "file://"
		|| pos == std::string::npos || pos2 == std::string::npos)
	{
		LogLine("DragAndDropHelper::isValidURL() - not a valid URL, do not accept drag...");
		return false;
	}

	return true;
}

/********************************************************************************************************************/
bool DragAndDropHelper::extractMediaStoreImageId() const
/********************************************************************************************************************/
{
	std::string::size_type pos = urlString.find("//");
	int pos2 = (int) urlString.find_first_of(":/", pos + 2);

	std::string hostNameStr = urlString.substr(pos + 2, pos2 - pos - 2);

	bool res = hostNameStr.compare(HOST_NAME) == 0;	
	if (res)
	{
		pos = urlString.rfind('/');
		imageId = urlString.substr(pos + 1);
		LogLine("DragAndDropHelper::extractMediaStoreImageId() - imageId = %s", imageId.c_str());
	}
	else
		LogLine("DragAndDropHelper::extractMediaStoreImageId() - this is not a MediaStore link");

	return res;
}

/********************************************************************************************************************/
bool DragAndDropHelper::extractAlfrescoImageId() const
/********************************************************************************************************************/
{
	std::string alfrescoPattern = config.getAlfrescoPattern();
	if (alfrescoPattern.length() == 0)
	{
		LogLine("DragAndDropHelper::extractAlfrescoImageId() - alfresco pattern is not specified");
		return false;
	}

	std::string::size_type patternPos = urlString.find(alfrescoPattern);
	if (patternPos == std::string::npos)
	{
		LogLine("DragAndDropHelper::extractAlfrescoImageId() - URL doesn't contain alfresco pattern");
		return false;
	}

	std::string::size_type slashPos = urlString.find('/', patternPos + alfrescoPattern.length());	
	if (slashPos == std::string::npos)
	{
		LogLine("DragAndDropHelper::extractAlfrescoImageId() - slash wasn't found after alfresco pattern");
		return false;
	}

	imageId = urlString.substr(patternPos + alfrescoPattern.length(), slashPos - patternPos - alfrescoPattern.length());
	LogLine("DragAndDropHelper::extractAlfrescoImageId() - alfresco imageId = %s", imageId.c_str());
	return true;
}

/********************************************************************************************************************/
UIDRef DragAndDropHelper::getTargetSpreadUID(const IDragDropTarget* target, PMPoint pasteboardMousePos )
/********************************************************************************************************************/
{
	UIDRef  spreadRef = UIDRef::gNull;

	// check the parameters
	if( target == nil  )
		return UIDRef::gNull;

	InterfacePtr<IControlView> layoutView(target, UseDefaultIID());
	if (!layoutView)
		return UIDRef::gNull;

	InterfacePtr<ISpread> targetSpread(Utils<IPasteboardUtils>()->QueryNearestSpread(layoutView, pasteboardMousePos));
	if (CheckCreation("DragAndDropHelper::getTargetSpreadUID()", targetSpread, "targetSpread", 0))
		return UIDRef::gNull;
	spreadRef = ::GetUIDRef(targetSpread);

	InterfacePtr<ILayoutControlData> layoutData(target, UseDefaultIID());
	if (CheckCreation("DragAndDropHelper::getTargetSpreadUID()", layoutData, "layoutData", 0))
		return UIDRef::gNull;

    if (spreadRef != layoutData->GetSpreadRef())
	{
		InterfacePtr<ICommand> setSpreadCmd(CmdUtils::CreateCommand(kSetSpreadCmdBoss));
		if (CheckCreation("DragAndDropHelper::getTargetSpreadUID()", setSpreadCmd, "setSpreadCmd", 0))
			return UIDRef::gNull;

		InterfacePtr<ILayoutCmdData> setSpreadLayoutCmdData(setSpreadCmd, UseDefaultIID());
		if (CheckCreation("DragAndDropHelper::getTargetSpreadUID()", setSpreadLayoutCmdData, "setSpreadLayoutCmdData", 0))
			return UIDRef::gNull;

		setSpreadLayoutCmdData->Set(::GetUIDRef(layoutData->GetDocument()), layoutData);
		setSpreadCmd->SetItemList(UIDList(spreadRef));
		ErrorCode res = CmdUtils::ProcessCommand(setSpreadCmd);
		if (CheckErrorCode("DragAndDropHelper::getTargetSpreadUID()", res, "changing spread", 0))
			return UIDRef::gNull;
	}
	return spreadRef;
}

/********************************************************************************************************************/
DocDesc*  DragAndDropHelper::getDocumentDescriptionFor( const IDragDropTarget* target) const
/********************************************************************************************************************/
{
	InterfacePtr<ILayoutControlData> layoutData(target, UseDefaultIID());
	if (layoutData != nil)
	{
		IDocument* document = layoutData->GetDocument();
		if (document != nil)
		{
			return getDoc(document);
		}
	}
	return nil;
}

#ifdef WINOS

/********************************************************************************************************************/
void DragAndDropHelper::getObjectURL(IPMDataObject* dataObject) const
/********************************************************************************************************************/
{
	urlString.clear();

	int len = dataObject->GetSizeOfFlavorData(kURLExternalFlavor);
	if (len == 0)
	{
		LogLine("DragAndDropHelper.cpp::getObjectURL() - 0-sized flavor data...");
		return;
	}
	LogLine("DragAndDropHelper.cpp::getObjectURL() size of flavor data = %d", len);

	IPMStream* inStream = dataObject->GetStreamForReading(kURLExternalFlavor);
	if (inStream == NULL)
	{
		LogLine("DragAndDropHelper.cpp::getObjectURL() - URL flavor data reading stream is NULL");
		return;
	}
	
	LogLine("before XferBuf");

//	K2::scoped_array<char> buf(new char[len + 1]);
	K2::scoped_array<char> buf(new char[len + 100]);
	int32 res = inStream->XferByte(reinterpret_cast<unsigned char*>(buf.get()), len + 1);
	if (res > 0)
	{
		buf[len] = 0;
		LogLine("DragAndDropHelper.cpp::getObjectURL() - data = %s", buf.get());
		urlString = buf.get();
	}
	inStream->Seek(0, kSeekFromStart);
	dataObject->FinishedWithStream(inStream);
}
#else
/********************************************************************************************************************/
void DragAndDropHelper::getObjectURL(DragReference &dragRef) const
/********************************************************************************************************************/
{
	urlString.clear();

	if (dragRef == NULL)
	{
		LogLine("DragAndDropHelper.cpp::getObjectURL() - drag reference is NULL...");
		return;
	}

	unsigned short totalItems;
	OSErr err = CountDragItems(dragRef, &totalItems);
	if (err != noErr)
	{
		LogLine("ERROR IN DragAndDropHelper.cpp::getObjectURL() - CountDragItems() err = %d", err);
		return;
	}
	LogLine("DragAndDropHelper.cpp::getObjectURL() - number of items in drag = %d", totalItems);
	if (totalItems != 1)
	{
		LogLine("DragAndDropHelper.cpp::getObjectURL() - dragging multiple items is not supported...");
		return;
	}

	ItemReference itemRef;
	err = GetDragItemReferenceNumber(dragRef, 1, &itemRef);
	if (err != noErr)
	{
		LogLine("DragAndDropHelper.cpp::getObjectURL() - can't get ItemReference of the first item, err = %d", err);
		return;
	}

	long dataSize;
	err = GetFlavorDataSize(dragRef, itemRef, kScrapFlavorTypeText, &dataSize);
	if (err != noErr)
	{
		// If we got this error, possible kScrapFlavorTypeText doesn't exist
		LogLine("DragAndDropHelper.cpp::getObjectURL() - can't get flavor data size, err = %d", err);
		return;
	}
	LogLine("DragAndDropHelper.cpp::getObjectURL() - flavor data size = %d", dataSize);

	if (dataSize == 0)
	{
		LogLine("DragAndDropHelper.cpp::getObjectURL() - flavor data has zero size...");
		return;
	}

	K2::scoped_array<char> buf(new char[dataSize + 1]);
	err = GetFlavorData(dragRef, itemRef, kScrapFlavorTypeText, buf.get(), &dataSize, 0);
	if (err != noErr)
	{
		LogLine("DragAndDropHelper.cpp::getObjectURL() - GetFlavorData() failed, err = %d", err);
		return;
	}

	buf[dataSize] = 0;
	LogLine("DragAndDropHelper.cpp::getObjectURL() - data = %s", buf.get());

	urlString = buf.get();
}
#endif

/********************************************************************************************************************/
bool DragAndDropHelper::GetPathFromClipboard( std::wstring& path )
/********************************************************************************************************************/
{
	bool result = false;
#if WINOS
	int			csLength = 512;
	char		cs[512];
	wchar_t		thePicPath[512];

	path.clear();

	if (!CountClipboardFormats())	
		return false; 

	if (!OpenClipboard(NULL))		
		return false;

	HGLOBAL globMem = GetClipboardData(CF_HDROP);
	if( !globMem ) {
		CloseClipboard () ;
		return false; 
	}

	DragQueryFileW( (HDROP)globMem, 0, (LPWSTR)thePicPath, 512);
	std::wstring widestr(thePicPath);
	path = widestr;

	WideCharToMultiByte(CP_ACP, 0, thePicPath, -1, cs, csLength, 0, NULL);

	for (int i = 0; i < csLength; i++)
	{
		if (cs[i] < 0) // meaning its > 0x7f -> umlaut
		{
			result = true;
			EmptyClipboard();
		}
	}

	CloseClipboard();

	/*if (strchr(cs, 'ü')) {
		LogLine("Have image path with critical character: %s", cs);
		return true;
	}*/
#endif
	return result;
} 
