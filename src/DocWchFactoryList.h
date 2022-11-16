/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		DocWchFactoryList.h																					*/
/*																													*/
/*	Created:																										*/
/*																													*/
/*	Modified:																										*/
/*																													*/
/********************************************************************************************************************/

REGISTER_PMINTERFACE(DocWchActionComponent, kDocWchActionComponentImpl)
REGISTER_PMINTERFACE(DocWatchStartupShutdown, kDocWchStartupShutdownImpl)
REGISTER_PMINTERFACE(MyActionFilter, kMyActionFilterImpl)
REGISTER_PMINTERFACE(SuppUISuppressedUI, kSuppUISuppressedUIImpl)
//REGISTER_PMINTERFACE(MyIdleTask, kMyIdleTaskImpl)
REGISTER_PMINTERFACE(SaveIdleTask, kSaveIdleTaskImpl)
REGISTER_PMINTERFACE(DocWchDocFileHandler, kDocWchDocFileHandlerImpl)
REGISTER_PMINTERFACE(AppQuitObserver, kQuitApplicationObserverImpl)   
REGISTER_PMINTERFACE(DragAndDropHelper, kDragAndDropHelperImpl)
REGISTER_PMINTERFACE(AppTerminateIdleTask, kAppTerminateIdleTaskImpl)
REGISTER_PMINTERFACE(XMLConfigSAXContentHandler, kXMLConfigSAXContentHandlerImpl)
//REGISTER_PMINTERFACE(SaveACopyIdleTask, kSaveACopyIdleTaskImpl)