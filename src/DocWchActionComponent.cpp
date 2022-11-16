/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		DocWchActionComponent.cpp																			*/
/*																													*/
/*	Created:																										*/
/*																													*/
/*	Modified:	28.06.07	tss	some logging messages added to setupColorProc()										*/
/*				27.08.07    hal UpdateActionStates()  changed														*/
/*				13.09.07    hal DoAction() changed , see in  'case kCloseActionID:'									*/
/*				14.05.07    tss ifdef for MessageBeep() added														*/
/*				27.04.2012	bu	use precompiled header																*/
/*																													*/
/********************************************************************************************************************/

#include "precompiled.h"

#define sFormat "alfa Plugin\n\nVersion %s\n\nCopyright (c) 2006-2022 alfa Media Partner GmbH"

extern CMyStream sStreamOut2;
extern int can_work;

static ClassID sSaveDocCmdBoss;
static ClassID sSaveAsDocCmdBoss;
static ClassID sCloseDocCmdBoss;
static ClassID sQuitCmdBoss;
static ClassID sPageSetupCmdBoss;


/********************************************************************************************************************/
class DocWchActionComponent : public CActionComponent
/********************************************************************************************************************/
{
public:
	DocWchActionComponent(IPMUnknown* boss): CActionComponent(boss)	{};
	void UpdateActionStates(IActiveContext *ac, IActionStateList *listToUpdate, GSysPoint mousePoint = 
		kInvalidMousePoint, IPMUnknown *widget = nil);
	void DoAction(IActiveContext* ac, ActionID actionID, GSysPoint mousePoint, IPMUnknown* widget);

private:
	void DoAbout();
};

CREATE_PMINTERFACE(DocWchActionComponent, kDocWchActionComponentImpl)

/********************************************************************************************************************/
static bool isMyDoc(IDocument *doc)
/********************************************************************************************************************/
{
	if(doc == NULL)
		return false;

	if(getRefnum(doc) == 0)
		return false;

	return true;
}

/********************************************************************************************************************/
void DocWchActionComponent::UpdateActionStates(IActiveContext *ac, IActionStateList *listToUpdate, 
											   GSysPoint mousePoint, IPMUnknown *widget)
/********************************************************************************************************************/
{
	IDocument *doc = Utils<ILayoutUIUtils>()->GetFrontDocument();//GetFrontDocument();
	bool myDoc = isMyDoc(doc);
	int32 cnt = listToUpdate->Length();

	for(int32 i=0; i<cnt; i++)
	{
		ActionID actionID = listToUpdate->GetNthAction(i);

		int32 cmd = actionID.Get();
		ClassID boss = 0;

		switch(cmd)
		{
			case kSaveActionID:
			case kCloseActionID:
			case kDocWchAboutActionID:
				listToUpdate->SetNthActionState(i, kEnabledAction);
				break;

			case kSaveAsActionID:
				/*
				bu 09.01.08: 
				if(myDoc == true)
				listToUpdate->SetNthActionState(i, kDisabled_Unselected);
				else
				*/
				boss = kDocumentComponentBoss;
				break;

			case kQuitActionID:
///				if( AlfaDocManager::getInstance()->getNumberOfAlfaDocs() )
///					listToUpdate->SetNthActionState(i, kDisabled_Unselected);
///				else
///					boss = sQuitCmdBoss;
				break;
		}

		IActionComponent *action = (IActionComponent *) ::CreateObject(boss, IID_IACTIONCOMPONENT);
		if(action != NULL)
		{
			action->UpdateActionStates(ac, listToUpdate, mousePoint, widget);
			action->Release();
		}
	}
	return;
}

/********************************************************************************************************************/
void DocWchActionComponent::DoAction(IActiveContext* ac, ActionID actionID,
									 GSysPoint mousePoint, IPMUnknown* widget)
/********************************************************************************************************************/
{
	ClassID boss = 0;

	switch(actionID.Get())
	{
		case kDocWchAboutActionID:
			this->DoAbout();
		break;

		case kSaveActionID:
			LogLine("DocWchActionComponent::DoAction() - action is kSaveActionID...");
			boss = kDocumentComponentBoss;
		break;

		case kSaveAsActionID:
			LogLine("DocWchActionComponent::DoAction() - action is kSaveAsActionID...");
			boss = kDocumentComponentBoss;
		break;

		case kCloseActionID:
			{
				LogLine("DocWchActionComponent::DoAction() - action is kCloseActionID...");
				//---- HALK 20070913
				//  boss = kDocumentComponentBoss;

				//   Don't close a document by File->Close menu action, but send only
				// a message to a client
				//   NOTE: close a document by pressing of a window's 'X' button are
				// processed in  DocWchDocFileHandler::Close()
				//
				DocDesc *dd = getDoc(ac->GetContextDocument());
				if( dd != NULL )
				{
					LogLine("DocWchActionComponent::DoAction() - kDocHasBeenClosed is sending...");
					// sent a notification message to a client application
					CMyStream *stream = &sStreamOut2;
					stream->setup(kDocHasBeenClosed, dd->refnum);
					sendData(dd->target, kAdDocCallback, stream);
				}
				else
				{
					// for non-plugin opened documents
					boss = kDocumentComponentBoss;
				}
				break;
			}

		case kQuitActionID:
///			if( AlfaDocManager::getInstance()->getNumberOfAlfaDocs() )
///				#ifdef WINOS
///					MessageBeep(MB_OK);
///				#else
///				;
///				#endif
///			else
				boss = sQuitCmdBoss;
			break;
	}

	IActionComponent *action = (IActionComponent *) ::CreateObject(boss, IID_IACTIONCOMPONENT);
	if(action != NULL)
	{
		action->DoAction(ac, actionID, mousePoint, widget);
		action->Release();
	}
}

/********************************************************************************************************************/
void DocWchActionComponent::DoAbout()
/********************************************************************************************************************/
{
	CStr255 str = "";
	sprintf(str, sFormat, kDocWchVersion);

	CAlert::ModalAlert(	str,		
						kOKString,
						kNullString, 					// No second button
						kNullString, 					// No third button
						1,								// Set OK button to default
						CAlert::eInformationIcon		// Information icon.
						);
}

/********************************************************************************************************************/
class MyActionFilter : public CPMUnknown<IActionFilter>
/********************************************************************************************************************/
{
public:
	MyActionFilter(IPMUnknown *boss): CPMUnknown<IActionFilter>(boss) 
	{
	}

	void	FilterAction(ClassID* componentClass, ActionID *actionID, PMString* actionName,
		PMString* actionArea, int16* actionType, uint32* enablingType, bool16* userEditable);
};


CREATE_PMINTERFACE(MyActionFilter, kMyActionFilterImpl)


/********************************************************************************************************************/
void MyActionFilter::FilterAction(ClassID* actionClass, ActionID *actionID, PMString* actionName,
								  PMString* actionArea, int16* actionType, uint32* enablingType, bool16* userEditable)
/********************************************************************************************************************/
{
	int32 cmd = actionID->Get();

	if (can_work)
	{

		switch(cmd)
		{
		case kSaveActionID:
			sSaveDocCmdBoss = *actionClass;
			*actionClass = kDocWchActionComponentBoss;
		break;

		case kSaveAsActionID:
			sSaveAsDocCmdBoss = *actionClass;
			*actionClass = kDocWchActionComponentBoss;
		break;

		case kCloseActionID:
			sCloseDocCmdBoss = *actionClass;
			*actionClass = kDocWchActionComponentBoss;
		break;

 		case kQuitActionID:
			if((sQuitCmdBoss = *actionClass) == 0)
			{
				LogLine("MyActionFilter::FilterAction() - kQuitActionID - in if...");
				sQuitCmdBoss = kApplKBSCBoss;
			}
			*actionClass = kDocWchActionComponentBoss;
		break;
		}
	}
}