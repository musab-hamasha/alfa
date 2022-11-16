/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		DocWchID.h																							*/
/*																													*/
/********************************************************************************************************************/

#ifndef __DocWchID_h__ 
#define __DocWchID_h__

#define kDocWchCompanyKey	kSDKDefPlugInCompanyKey		// Company name used internally for menu paths and the like. Must be globally unique, only A-Z, 0-9, space and "_".
#define kDocWchCompanyValue	kSDKDefPlugInCompanyValue	// Company name displayed externally.
#define kPluginVersion		kSDKDefPluginVersionString
#define kDocWchPrefixNumber	0xA4780 					// Unique prefix number for this plug-in(*Must* be obtained from Adobe Developer Support).
#define kDocWchPluginName	"alfa"						// Name of this plug-in.
#define kDocWchAuthor		"alfa Media Partner GmbH"	// Author of this plug-in (for the About Box).

#define kDocWchVersion		"5.554"		           
#define FVERSION			5,5,5,4

#define kSDKDefPersistMajorVersionNumber						kSDKDef_30_PersistMajorVersionNumber
#define kSDKDefPersistMinorVersionNumber						kSDKDef_30_PersistMinorVersionNumber

// Plug-in Prefix: (please change kDocWchPrefixNumber above to modify the prefix.)
#define kMyPrefix			RezLong(kDocWchPrefixNumber)				// Unique numeric prefix for all object model IDs for this plug-in.
#define kDocWchStringPrefix	SDK_DEF_STRINGIZE(kDocWchPrefixNumber)	// String equivalent of the unique prefix number for  this plug-in.

// PluginID:
DECLARE_PMID(kPlugInIDSpace, kDocWchPluginID,						kMyPrefix + 0)

// ClassIDs:
DECLARE_PMID(kClassIDSpace, kDocWchActionComponentBoss,				kMyPrefix + 0)
DECLARE_PMID(kClassIDSpace, kDocWchResponderServiceBoss,			kMyPrefix + 1)
DECLARE_PMID(kClassIDSpace, kDocWchStartupShutdownBoss,				kMyPrefix + 2)
DECLARE_PMID(kClassIDSpace, kMyActionFilterBoss,					kMyPrefix + 3)
DECLARE_PMID(kClassIDSpace, kDocWchDocFileHandlerBoss2,				kMyPrefix + 4) 
DECLARE_PMID(kClassIDSpace, kDragAndDropHelperBoss,     			kMyPrefix + 5)
DECLARE_PMID(kClassIDSpace, kDocBlackBoxDataKey,					kMyPrefix + 6)
DECLARE_PMID(kClassIDSpace, kXMLConfigSAXContentHandlerServiceBoss,	kMyPrefix + 7)
DECLARE_PMID(kClassIDSpace, kSuppUISuppressedUIServiceBoss,			kMyPrefix + 8)

// InterfaceIDs:
DECLARE_PMID(kInterfaceIDSpace, IID_ILOCKAREAOBSERVER,				kMyPrefix + 1)
DECLARE_PMID(kInterfaceIDSpace, IID_IMYIDLETASK,					kMyPrefix + 2)
DECLARE_PMID(kInterfaceIDSpace, IID_ISELSUITE,						kMyPrefix + 3)
DECLARE_PMID(kInterfaceIDSpace, IID_ISAVEIDLETASK,					kMyPrefix + 4)
DECLARE_PMID(kInterfaceIDSpace, IID_IDOCFILEHANDLERSHADOW,			kMyPrefix + 5)  
DECLARE_PMID(kInterfaceIDSpace, IID_IQUITAPPLICATIONOBSERVER,		kMyPrefix + 6) 
DECLARE_PMID(kInterfaceIDSpace, IID_IAPPTERMINATEIDLETASK,			kMyPrefix + 7) 
DECLARE_PMID(kInterfaceIDSpace, IID_ISAVEACOPYIDLETASK,				kMyPrefix + 8)	

// ImplementationIDs:
DECLARE_PMID(kImplementationIDSpace, kDocWchActionComponentImpl,	kMyPrefix + 0)
DECLARE_PMID(kImplementationIDSpace, kDocWchResponderImpl,			kMyPrefix + 1)
DECLARE_PMID(kImplementationIDSpace, kDocWchServiceProviderImpl,	kMyPrefix + 2)
DECLARE_PMID(kImplementationIDSpace, kDocWchStartupShutdownImpl, 	kMyPrefix + 3)
DECLARE_PMID(kImplementationIDSpace, kMyActionFilterImpl, 			kMyPrefix + 4)
DECLARE_PMID(kImplementationIDSpace, kSuppUISuppressedUIImpl,		kMyPrefix + 5)
//lockarea
DECLARE_PMID(kImplementationIDSpace, kLockAreaObserverImpl,			kMyPrefix + 6)
DECLARE_PMID(kImplementationIDSpace, kMyIdleTaskImpl,				kMyPrefix + 7)
DECLARE_PMID(kImplementationIDSpace, kSelSuiteASBImpl,				kMyPrefix + 8)
DECLARE_PMID(kImplementationIDSpace, kSelSuiteCSBImpl,				kMyPrefix + 9)
DECLARE_PMID(kImplementationIDSpace, kSaveIdleTaskImpl,				kMyPrefix + 10)
// DocFileHandler
DECLARE_PMID(kImplementationIDSpace, kDocWchDocFileHandlerImpl,		kMyPrefix + 11)
DECLARE_PMID(kImplementationIDSpace, kQuitApplicationObserverImpl,  kMyPrefix + 12) 
DECLARE_PMID(kImplementationIDSpace, kDragAndDropHelperImpl,        kMyPrefix + 13)
// app.terminate 
DECLARE_PMID(kImplementationIDSpace, kAppTerminateIdleTaskImpl,		kMyPrefix + 14) 
DECLARE_PMID(kImplementationIDSpace, kXMLConfigSAXContentHandlerImpl,kMyPrefix + 15)
DECLARE_PMID(kImplementationIDSpace, kSaveACopyIdleTaskImpl,		kMyPrefix + 16)

// ActionIDs:
DECLARE_PMID(kActionIDSpace, kDocWchAboutActionID,					kMyPrefix + 0)

// "About Plug-ins" sub-menu:
#define kDocWchAboutMenuKey			kDocWchStringPrefix "kDocWchAboutMenuKey"
#define kDocWchAboutMenuPath		kSDKDefStandardAboutMenuPath kDocWchCompanyKey
#define kAlfaSubmenuKey				"kAlfaSubmenuKey"

// Other StringKeys:
#define kDocWchAboutBoxStringKey	kDocWchStringPrefix "kDocWchAboutBoxStringKey"

#endif
