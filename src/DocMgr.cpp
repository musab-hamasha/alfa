/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		DocMgr.cpp																							*/
/*																													*/
/*	Created:																										*/
/*																													*/
/*	Modified:	29.01.09	tss	cs4 porting: another way of accessing to ISession									*/
/*				07.04.10	kli	2 LEAKs fixed in AlfaDocManager::checkDataIsActual()								*/
/*				27.04.2012	bu	use precompiled header																*/
/*																													*/
/********************************************************************************************************************/

//#include "VCPlugInHeaders.h"
#include "precompiled.h"

AlfaDocManager AlfaDocManager::instance;


extern bool8 isCanClosePluggedDocs ;
extern bool8 isCanSavePluggedDocs;

/********************************************************************************************************************/
IDocument*  DocDesc::getDoc()
/********************************************************************************************************************/
{
	return this->doc;
}

/********************************************************************************************************************/
AlfaDocManager::~AlfaDocManager()
/********************************************************************************************************************/
{
	std::list<DocDesc*>::iterator it;
	for (it = dataObjects.begin(); it != dataObjects.end(); it++)
	{
		DocDesc *obj = *it;
		if (obj != NULL)
		{
			delete obj;
		}
	}
}

/********************************************************************************************************************/
DocDesc* AlfaDocManager::attachDataToDocument( UIDRef docRef, int32 docID, TargetApp target,
												CStr path, int pgNum, int fixed )
/********************************************************************************************************************/
{
	InterfacePtr<IDocument> doc(docRef, UseDefaultIID());
	if (CheckCreation("AlfaDocManager::attachDataToDocument()", doc, "IDocument"))
		return NULL;

	DocDesc *p = new DocDesc();
	if(p != NULL)
	{
		p->doc = doc;
		p->refnum = docID;
		strcpy(p->path, path);
		p->target = target;

		// add to array
		dataObjects.push_back(p);
	}

	return p;
}

/********************************************************************************************************************/
UIDRef AlfaDocManager::createDocument(PMReal width, PMReal height, 
							  PMReal left, PMReal top, PMReal right, PMReal bottom, int colNum, PMReal gridDist,
							  PMReal bleedTop, PMReal bleedBottom, PMReal bleedLeft, PMReal bleedRight, int numPages)
/********************************************************************************************************************/
{
	UIDRef result = UIDRef::gNull;
	
	do
	{
		// Create the command.
		InterfacePtr<ICommand> newDocCmd(Utils<IDocumentCommands>()->CreateNewCommand(K2::kFullUI));
		if (CheckCreation("AdDocPI.cpp::createDocument()", newDocCmd, "newDocCmd"))
			break;

		// Set the command's parameterised data.
		InterfacePtr<INewDocCmdData> newDocCmdData(newDocCmd, UseDefaultIID());
		if (CheckCreation("AdDocPI.cpp::createDocument()", newDocCmdData, "newDocCmdData"))
			break;

		newDocCmdData->SetCreateBasicDocument(kFalse); // Override the following defaults.
#ifdef CS6
		newDocCmdData->SetNewDocumentPageSize(PMPageSize(width, height));
#else
		newDocCmdData->SetPageSizePref(PMRect(0.0, 0.0, width, height));
#endif
		if (width>height)
			newDocCmdData->SetWideOrientation(kTrue);
		else
			newDocCmdData->SetWideOrientation(kFalse);
		newDocCmdData->SetMargins(left, top, right, bottom);
		newDocCmdData->SetNumPages(numPages);
		newDocCmdData->SetPagesPerSpread(1);
#ifdef CC
		newDocCmdData->SetColumns_4(colNum, gridDist, IColumns::kVerticalColumnOrientation);
#else
		newDocCmdData->SetColumns(colNum, gridDist, kFalse);
#endif
		newDocCmdData->SetUseUniformBleed( false );

		PMRect bleedBox;
		bleedBox.Left( mm2pt(bleedLeft/1000.f) );
		bleedBox.Top( mm2pt(bleedTop/1000.f) );
		bleedBox.Right( mm2pt(bleedRight/1000.f) );
		bleedBox.Bottom( mm2pt(bleedBottom/1000.f) );
		newDocCmdData->SetBleedBox( bleedBox );

		// Create the new document.
		ErrorCode err = CmdUtils::ProcessCommand(newDocCmd);
		if (CheckErrorCode("AdDocPI.cpp::createDocument()", err, "ProcessCommand()"))
			break;

		PMRect bb2 = newDocCmdData->GetBleedBox();

		LogLine("Left=%f, top=%f, right=%f, bottom=%f", ToDouble(bb2.Left()), ToDouble(bb2.Top()), ToDouble(bb2.Right()), ToDouble(bb2.Bottom()));


		// Pass the UIDRef of the new document back to the caller.
		const UIDList& newDocCmdUIDList = newDocCmd->GetItemListReference();
		result = newDocCmdUIDList.GetRef(0);
		if (CheckUIDRef("AdDocPI.cpp::createDocument()", result, "newDocCmdUIDList"))
			break;

	} while (false);

	return result;
}

/********************************************************************************************************************/
int AlfaDocManager::getNumberOfAlfaDocs()
/********************************************************************************************************************/
{
	return (int) dataObjects.size();
}

/********************************************************************************************************************/
DocDesc* AlfaDocManager::getDataForAlfaDoc(IDocument *doc)
/********************************************************************************************************************/
{
	std::list<DocDesc*>::iterator it;
	for (it = dataObjects.begin(); it != dataObjects.end(); it++)
	{
		DocDesc *obj = *it;
		if (obj == NULL)
		{
			continue;
		}
		if( obj->doc == doc)
		{
			return obj;
		}
	}

	LogLine("AlfaDocManager::getDataForAlfaDoc() - not found ");

	return NULL;
}

/********************************************************************************************************************/
DocDesc* AlfaDocManager::getDataForAlfaDocByID(int32 refnum)
/********************************************************************************************************************/
{
	std::list<DocDesc*>::iterator it;
	for (it = dataObjects.begin(); it != dataObjects.end(); it++)
	{
		DocDesc *obj = *it;
		if (obj == NULL)
		{
			continue;
		}
		if( obj->refnum == refnum)
		{
			return obj;
		}
	}

	return NULL;
}

/********************************************************************************************************************/
int  AlfaDocManager::getIDForAlfaDoc(IDocument *doc)
/********************************************************************************************************************/
{

	DocDesc* dd = getDataForAlfaDoc(doc);
	if( dd != NULL )
		return dd->refnum;

	return -1;
}

/********************************************************************************************************************/
IDocument*  AlfaDocManager::getAlfaDocByID(int32 refnum)
/********************************************************************************************************************/
{
	DocDesc *dd = getDataForAlfaDocByID(refnum);
	if( dd != NULL)
	{
		return dd->getDoc();
	}
	else {
		if (dd)
			LogLine("doc with refnum %d not found in descriptor list", refnum);
	//	else
	//		LogLine("no AlfaDoc descriptor, maybe we're opening the first doc now...");
	}

	//LogLine("AlfaDocManager::getAlfaDocByID() - not found ");

	return NULL;
}

/********************************************************************************************************************/
bool  AlfaDocManager::getAllAlfaDocs( std::list<IDocument*>& docs )
/********************************************************************************************************************/
{
	docs.clear();

	std::list<DocDesc*>::iterator it;
	for (it = dataObjects.begin(); it != dataObjects.end(); it++)
	{
		DocDesc *obj = *it;
		if (obj == NULL)
		{
			continue;
		}

		IDocument* d = obj->getDoc();
		if (d == NULL)
		{
			continue;
		}
		docs.push_back(d);
		//--
	}

	return true;
}

/********************************************************************************************************************/
int   AlfaDocManager::closeAlfaDoc(int32 refnum)
/********************************************************************************************************************/
{
	isCanClosePluggedDocs = kTrue;

	IDocument *doc = getDocFromRefnum(refnum);
	if (doc == NULL)
		return -1;

	int err = closeDocAux(doc);

	std::list<DocDesc*>::iterator it; 
	for (it = dataObjects.begin(); it != dataObjects.end(); it++)
	{
		DocDesc *obj = *it;
		if (obj == NULL)
		{
			continue;
		}
		if( obj->refnum == refnum)
		{
			delete obj;
			dataObjects.erase(it);
			break;
		}
	}

	isCanClosePluggedDocs = kFalse;

	return err;
}

/********************************************************************************************************************/
void AlfaDocManager::setInitialIDFile( const UIDRef& docRef, const IDFile& idFile )
/********************************************************************************************************************/
{
	std::list<DocDesc*>::iterator it;
	for( it = dataObjects.begin(); it != dataObjects.end(); it++ )
	{
		DocDesc *obj = *it;
		if( obj == NULL ) continue;

		if( ::GetUIDRef( obj->doc ) == docRef )
		{
			obj->initialIDFile = idFile;
			return;
		}
	}
}

/********************************************************************************************************************/
IDFile AlfaDocManager::getInitialIDFile( const UIDRef& docRef )
/********************************************************************************************************************/
{
	std::list<DocDesc*>::iterator it;
	for( it = dataObjects.begin(); it != dataObjects.end(); it++ )
	{
		DocDesc *obj = *it;
		if( obj == NULL ) continue;

		if( ::GetUIDRef( obj->doc ) == docRef )
			return obj->initialIDFile;
	
	}

	return IDFile( WideString("") );

}

/********************************************************************************************************************/
DocDesc *getDoc(IDocument *doc)
/********************************************************************************************************************/
{
	return AlfaDocManager::getInstance()->getDataForAlfaDoc(doc);
}

/********************************************************************************************************************/
static DocDesc *getDoc(int32 refnum)
/********************************************************************************************************************/
{
	return AlfaDocManager::getInstance()->getDataForAlfaDocByID(refnum);
}

/********************************************************************************************************************/
int getRefnum(IDocument *doc)
/********************************************************************************************************************/
{
	
	return AlfaDocManager::getInstance()->getIDForAlfaDoc(doc);
}

/********************************************************************************************************************/
IDocument *getDocFromRefnum(int32 refnum)
/********************************************************************************************************************/
{
	return AlfaDocManager::getInstance()->getAlfaDocByID(refnum);
}

/********************************************************************************************************************/
TargetApp getClient(int32 refnum)
/********************************************************************************************************************/
{
	TargetApp target; // = 0;

	DocDesc *p = getDoc(refnum);

	if(p != NULL)
		target = p->target;
	else
		ZERODATA(target);

	return target;
}

/********************************************************************************************************************/
void getPath(int32 refnum, CStr path)
/********************************************************************************************************************/
{
	DocDesc *p = getDoc(refnum);

	if(p != NULL)
		strcpy(path, p->path);
}

