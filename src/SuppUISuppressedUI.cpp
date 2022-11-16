/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		SuppUISuppressedUI.cpp																				*/
/*																													*/
/*	Created:																										*/
/*																													*/
/*	Modified:																										*/
/*																													*/
/********************************************************************************************************************/

#include "precompiled.h"

/* CREATE_PMINTERFACE
Binds the C++ implementation class onto its ImplementationID making the C++ code callable by the application. */
CREATE_PMINTERFACE(SuppUISuppressedUI, kSuppUISuppressedUIImpl)

extern int can_work;


/********************************************************************************************************************/
static bool isMyDoc(IDocument *doc)
/********************************************************************************************************************/
{
	if (can_work==0)
		return false;

	if(doc == NULL)
		return false;

	if(getRefnum(doc) == -1)
		return false;

	return true;
}

/********************************************************************************************************************/
bool16	SuppUISuppressedUI::IsActionDisabled( ActionID action ) const
/********************************************************************************************************************/
{	
	if (!can_work)
		return kFalse;

	if (action.Get() != 113409)
		return kFalse;

	IDocument *doc = Utils<ILayoutUIUtils>()->GetFrontDocument();
	if (!isMyDoc(doc))
		return kFalse;

	InterfacePtr<ISelSuite> selSuite(UnifiedAux::GetSession()->GetActiveContext()->GetContextSelection(), 
		UseDefaultIID());
	if (selSuite==NULL)
		return kFalse;

	return selSuite->CanDoSomething(getDoc(doc));
}

/********************************************************************************************************************/
bool16	SuppUISuppressedUI::IsActionHidden( ActionID action ) const
/********************************************************************************************************************/
{
if (!can_work)
		return kFalse;

	if (action.Get()!=6146 && action.Get()!=kClearActionID && action!=kCutActionID)
		return kFalse;

	IDocument *doc = Utils<ILayoutUIUtils>()->GetFrontDocument();
	if (!isMyDoc(doc))
		return kFalse;

	bool16 res = kFalse;

	return res; /* jump out before something is going wrong... */

	//"Delete" and "Cut" actions (codes: 272 && 269)
	if (action == kCutActionID || action.Get()==kClearActionID)
	{
		LogLine("SuppUISuppressedUI::IsActionHidden() - action = %s", action.Get());

		InterfacePtr<ISelSuite> selSuite(UnifiedAux::GetSession()->GetActiveContext()->GetContextSelection(), UseDefaultIID());
		if (selSuite==NULL)
			res = kFalse;
		else
		{
			DocDesc *docDesc = getDoc(doc);
			res = selSuite->CanDoSomething(docDesc);
		}
	}
	LogLine("SuppUISuppressedUI::IsActionHidden(never reached)");

	//"Margins and columns..." action
	if (action.Get()==6146)
	{
		IDataBase *db = ::GetDataBase(doc);
		IDocumentPresentation *frontDocPresentation = Utils<IDocumentUIUtils>()->GetFrontmostPresentationForDocument(db);
		InterfacePtr<ILayoutControlData> layoutCData(Utils<ILayoutUIUtils>()->QueryLayoutData(frontDocPresentation));

		UID pageUID = layoutCData->GetPage();
		InterfacePtr<IPageList> pages(doc, UseDefaultIID());
		int number = pages->GetPageIndex(pageUID);
		LogLine("SuppUISuppressedUI::IsActionHidden() - Current page number = %d", number);
		DocDesc *doc_desc = getDoc(doc);
	}

	return res;
}

/********************************************************************************************************************/
bool16 SuppUISuppressedUI::IsWidgetDisabled( const IControlView* widget ) const
/********************************************************************************************************************/
{
	return kFalse;
}