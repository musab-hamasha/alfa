/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		DocWchDocFileHandler.cpp																			*/
/*																													*/
/*	Created:	16.08.07    hal																						*/
/*																													*/
/*	Modified:	15.05.08	tss	Mac only bug: we don't need to restore view after close								*/
/*				04.02.09	tss	cs4 porting: we don't need to restore view after close in CS4						*/
/*				27.04.2012	bu	use precompiled header																*/
/*																													*/
/********************************************************************************************************************/

#include "precompiled.h"

int	getRefnum(IDocument *doc);
bool8 IsCanClosePluggedDocs();
bool8 IsCanSavePluggedDocs();


/********************************************************************************************************************/
class DocWchDocFileHandler : public CPMUnknown<IDocFileHandler>
	/********************************************************************************************************************/
{
public:
	DocWchDocFileHandler(IPMUnknown *boss);

	virtual bool8 CanSave(const UIDRef& doc);
	virtual void Save(const UIDRef& doc, UIFlags uiFlags = kFullUI);
	virtual bool8 CanSaveAs(const UIDRef& doc);
	virtual void SaveAs(const UIDRef& doc, const IDFile *destFile = nil, UIFlags uiFlags = kFullUI,
		bool8 asStationery = kFalse, FileTypeInfoID fileTypeID = kInvalidFileTypeInfoID);
	virtual bool8 CanSaveACopy(const UIDRef& doc);
	virtual void SaveACopy(const UIDRef& doc, const IDFile *destFile = nil, UIFlags uiFlags = kFullUI,
		bool8 asStationery = kFalse,
		FileTypeInfoID fileTypeID = kInvalidFileTypeInfoID);
	virtual bool8 CanRevert(const UIDRef& doc);
	virtual void Revert(const UIDRef& doc, UIFlags uiFlags = kFullUI);
	virtual bool8 CanClose(const UIDRef& doc);
	virtual void Close(const UIDRef& doc, UIFlags uiFlags = kFullUI, bool8 allowCancel = kTrue,
		CloseCmdMode cmdMode = kSchedule);
	virtual CloseOptions CheckOnClose(const UIDRef& doc, UIFlags uiFlags, bool8 allowCancel);
	virtual void GetCopyDefaultName(const UIDRef& doc, IDFile *name, bool16& useSystemDefaultDir);

	/* bu 04.08.15 added (2015): */
	virtual void SaveAs2(const UIDRef& doc, const PMString& fileName, UIFlags uiFlags = kFullUI,
		bool8 asStationery = kFalse, FileTypeInfoID fileTypeID = kInvalidFileTypeInfoID);
};
CREATE_PMINTERFACE(DocWchDocFileHandler, kDocWchDocFileHandlerImpl)


/********************************************************************************************************************/
DocWchDocFileHandler::DocWchDocFileHandler(IPMUnknown *boss) : CPMUnknown<IDocFileHandler>(boss)
/********************************************************************************************************************/
{
}

/********************************************************************************************************************/
bool8 DocWchDocFileHandler::CanSave(const UIDRef& doc)
/********************************************************************************************************************/
{

	InterfacePtr<IDocFileHandler> delegate(this, IID_IDOCFILEHANDLERSHADOW);
	return delegate->CanSave(doc);
}

/********************************************************************************************************************/
bool8 DocWchDocFileHandler::CanSaveAs(const UIDRef& doc)
/********************************************************************************************************************/
{
	InterfacePtr<IDocFileHandler> delegate(this, IID_IDOCFILEHANDLERSHADOW);
	return delegate->CanSaveAs(doc);
}

/********************************************************************************************************************/
void DocWchDocFileHandler::SaveAs(const UIDRef& doc, const IDFile *destFile, UIFlags uiFlags, bool8 asStationery, FileTypeInfoID fileTypeID)
/********************************************************************************************************************/
{
	InterfacePtr<IDocFileHandler> delegate(this, IID_IDOCFILEHANDLERSHADOW);
	return delegate->SaveAs(doc, destFile, uiFlags, asStationery, fileTypeID);
}

/********************************************************************************************************************/
bool8 DocWchDocFileHandler::CanSaveACopy(const UIDRef& doc)
/********************************************************************************************************************/
{
	InterfacePtr<IDocFileHandler> delegate(this, IID_IDOCFILEHANDLERSHADOW);
	return delegate->CanSaveACopy(doc);
}

/********************************************************************************************************************/
void DocWchDocFileHandler::SaveACopy(const UIDRef& doc, const IDFile *destFile, UIFlags uiFlags, bool8 asStationery, FileTypeInfoID fileTypeID)
/********************************************************************************************************************/
{
	InterfacePtr<IDocFileHandler> delegate(this, IID_IDOCFILEHANDLERSHADOW);
	return delegate->SaveACopy(doc, destFile, uiFlags, asStationery, fileTypeID);
}

/********************************************************************************************************************/
bool8 DocWchDocFileHandler::CanRevert(const UIDRef& doc)
/********************************************************************************************************************/
{
	InterfacePtr<IDocFileHandler> delegate(this, IID_IDOCFILEHANDLERSHADOW);
	return delegate->CanRevert(doc);
}

/********************************************************************************************************************/
void DocWchDocFileHandler::Revert(const UIDRef& doc, UIFlags uiFlags)
/********************************************************************************************************************/
{
	InterfacePtr<IDocFileHandler> delegate(this, IID_IDOCFILEHANDLERSHADOW);
	return delegate->Revert(doc, uiFlags);
}

/********************************************************************************************************************/
bool8 DocWchDocFileHandler::CanClose(const UIDRef& docRef)
/********************************************************************************************************************/
{
	InterfacePtr<IDocFileHandler> delegate(this, IID_IDOCFILEHANDLERSHADOW);
	return delegate->CanClose(docRef);
}

/********************************************************************************************************************/
void DocWchDocFileHandler::Save(const UIDRef& docRef, UIFlags uiFlags)
/********************************************************************************************************************/
{
	bool canSave = false;

	InterfacePtr<IDocument> doc(docRef, UseDefaultIID());
	if (doc == NULL)
	{
		LogLine("DocWchDocFileHandler::Save() ... doc is NULL ");
		return;
	}

	//-------------------------------------------
	// prevent a saving of the plugged document
	//-------------------------------------------

	// isn't plugged document
	if (getRefnum(doc) == 0)
	{
		canSave = true;
	}
	else
	{
		// is possible to close ( for doCmdSave()  )
		if (IsCanSavePluggedDocs() == kTrue)
		{
			canSave = true;
		}
	}

	LogLine("DocWchDocFileHandler::Save() canSave = %s", canSave ? "true" : "false");
	if (canSave == true)
	{
		InterfacePtr<IDocFileHandler> delegate(this, IID_IDOCFILEHANDLERSHADOW);
		return delegate->Save(docRef, uiFlags);
	}
}

/********************************************************************************************************************/
void DocWchDocFileHandler::Close(const UIDRef& docRef, UIFlags uiFlags, bool8 allowCancel, CloseCmdMode cmdMode)
/********************************************************************************************************************/
{
	bool canClose = false;

	InterfacePtr<IDocument> doc(docRef, UseDefaultIID());
	if (doc == NULL)
	{
		return;
	}

	//-------------------------------------------
	// prevent a closing of the plugged document
	//-------------------------------------------

	// isn't plugged document
	if (getRefnum(doc) == 0)
	{
		canClose = true;
	}
	else
	{
		// is possible to close ( for doCmdClose() & "AppQuit" )
		if (IsCanClosePluggedDocs() == kTrue)
		{
			canClose = true;
		}
	}

	LogLine("DocWchDocFileHandler::Close() canClose = %s", canClose ? "true" : "false");

	if (canClose == true)
	{
		InterfacePtr<IDocFileHandler> delegate(this, IID_IDOCFILEHANDLERSHADOW);
		return delegate->Close(docRef, uiFlags, allowCancel, cmdMode);
	}
}

/********************************************************************************************************************/
IDocFileHandler::CloseOptions DocWchDocFileHandler::CheckOnClose(const UIDRef& doc, UIFlags uiFlags, bool8 allowCancel)
/********************************************************************************************************************/
{
	InterfacePtr<IDocFileHandler> delegate(this, IID_IDOCFILEHANDLERSHADOW);
	return delegate->CheckOnClose(doc, uiFlags, allowCancel);
}

/********************************************************************************************************************/
void DocWchDocFileHandler::GetCopyDefaultName(const UIDRef& doc, IDFile *name, bool16& useSystemDefaultDir)
/********************************************************************************************************************/
{
	InterfacePtr<IDocFileHandler> delegate(this, IID_IDOCFILEHANDLERSHADOW);
	return delegate->GetCopyDefaultName(doc, name, useSystemDefaultDir);
}

/* bu added 04.08.15 (CC2015): */

/********************************************************************************************************************/
void DocWchDocFileHandler::SaveAs2(const UIDRef& doc, const PMString& fileName, UIFlags uiFlags,
	bool8 asStationery, FileTypeInfoID fileTypeID)
	/********************************************************************************************************************/
{
	///InterfacePtr<IDocFileHandler> delegate(this, IID_IDOCFILEHANDLERSHADOW);
	///return delegate->SaveAs(doc, destFile, uiFlags, asStationery, fileTypeID);
}
