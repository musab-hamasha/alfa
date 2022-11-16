/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		AppQuitServer.cpp																					*/
/*																													*/
/*	Created:	21.08.07	hal																						*/
/*																													*/
/*	Modified:	29.01.09	tss	cs4 porting: another way of accessing to ISession									*/
/*				11.12.09	kli	IID_IQUITAPPLICATIONOBSERVER was moved to kSessionBoss from kAppBoss				*/
/*				13.05.2010	kli	ISubject in AutoAttach/AutoDetach is got from application, not session				*/
/*				27.04.2012	bu	use precompiled header																*/
/*																													*/
/********************************************************************************************************************/

#include "precompiled.h"

extern bool8 isCanClosePluggedDocs;


/********************************************************************************************************************/
class AppQuitObserver : public CObserver
/********************************************************************************************************************/
{
public:
	AppQuitObserver(IPMUnknown* boss): CObserver(boss) {}
	~AppQuitObserver() {}

	virtual void AutoAttach();
	virtual void AutoDetach();
	virtual void Update(const ClassID& theChange, ISubject* theSubject, const PMIID &protocol, void* changedBy);
};

CREATE_PMINTERFACE(AppQuitObserver, kQuitApplicationObserverImpl)

/********************************************************************************************************************/
void AppQuitObserver::AutoAttach()
/********************************************************************************************************************/
{
	CObserver::AutoAttach();
	//
	// get the ISubject - This interface provides a notification mechanism for objects
	// that need to be informed of changes made to another object.
	// When an object is modified, calling the Change method of its ISubject interface
	// notifies attached observers of the change.
	//
	InterfacePtr<IApplication> application( UnifiedAux::GetSession()->QueryApplication() );
	InterfacePtr<ISubject> subject( application, IID_ISUBJECT );
	if (!subject)
	{
		return;
	}

	// Check whether AppQuitObserver is attached to this subject
	if (subject->IsAttached(this, IID_IAPPLICATION, IID_IQUITAPPLICATIONOBSERVER) == kFalse)
	{
		// attach this object to IApplication as an observer
		subject->AttachObserver(this, IID_IAPPLICATION, IID_IQUITAPPLICATIONOBSERVER);
	}
}

/********************************************************************************************************************/
void AppQuitObserver::AutoDetach()
/********************************************************************************************************************/
{
	CObserver::AutoDetach();

	InterfacePtr<IApplication> application( UnifiedAux::GetSession()->QueryApplication() );
	InterfacePtr<ISubject> subject( application, IID_ISUBJECT );
	if(!subject)
	{
		return;
	}
	if (subject->IsAttached(this, IID_IAPPLICATION,IID_IQUITAPPLICATIONOBSERVER) == kTrue)
	{
		subject->DetachObserver(this, IID_IAPPLICATION,IID_IQUITAPPLICATIONOBSERVER);
	}
}

/********************************************************************************************************************/
void AppQuitObserver::Update(const ClassID& theChange, ISubject* theSubject, const PMIID &protocol, void* changedBy)
/********************************************************************************************************************/
{
	if (theChange == kCloseAllAndQuitCmdBoss)
	{
		isCanClosePluggedDocs = kTrue;
	}
	else if ( theChange == kQuitCmdBoss)
	{
	}
}
