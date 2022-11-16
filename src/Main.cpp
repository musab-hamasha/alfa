/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		Main.cpp																							*/
/*																													*/
/*	Created:																										*/
/*																													*/
/*	Modified:	17.05.08	tss	Mac-related changes: ifdefs, new functions getAEData() and cmdProc(),				*/
/*								changes in setupAdDocPI() and cleanupAdDocPI()										*/
/*				21.05.08	tss	Calls to convertPathInStream() in serverProc() were added (Mac only)				*/
/*				29.01.09	tss cs4 porting: access to ISession														*/
/*				11.12.09	kli	IID_IQUITAPPLICATIONOBSERVER was moved to kSessionBoss from kAppBoss				*/
/*								AutoDetach() now is calling inside DocWatchStartupShutdown::Shutdown()				*/
/*				27.04.12	bu	use precompiled header	*** EXCEPT FOR THIS FILE !!!	***							*/
/*				11.04.14	bu	MAC & CS6																			*/
/*				10.07.14	bu	MAC : re-import pictures fixed														*/
/*																													*/
/********************************************************************************************************************/

//#define __MAIN__

#include "precompiled.h"

#ifndef WINOS
	#include "MacPathUtils.h"
#endif

PlugIn	gPlugIn;

CMyStream sStreamOut;
CMyStream sStreamOut2;
CMyStream sStreamIn;

extern  CMyStream *doCmd(CMyStream *streamIn, int32 *pErr);
void setupAdDocPI();
void cleanupAdDocPI();

/********************************************************************************************************************/
CMyStream *GetOutStream()
/********************************************************************************************************************/
{
	// bu 03.09.14 added; shifting outstream as parameter over several functions mixes up memory
	return &sStreamOut; 
}

/********************************************************************************************************************/
IPlugIn* GetPlugIn()
/********************************************************************************************************************/
{
	return &gPlugIn;
}

/********************************************************************************************************************/
class DocWatchStartupShutdown : public CPMUnknown<IStartupShutdownService>
/********************************************************************************************************************/
{
	public:
		DocWatchStartupShutdown(IPMUnknown* boss): CPMUnknown<IStartupShutdownService>(boss) {};
		virtual ~DocWatchStartupShutdown() {}
		virtual void Startup();
		virtual void Shutdown();
};

CREATE_PMINTERFACE(DocWatchStartupShutdown, kDocWchStartupShutdownImpl)

/********************************************************************************************************************/
void DocWatchStartupShutdown::Startup()
/********************************************************************************************************************/
{

	InterfacePtr<IApplication> app(UnifiedAux::GetSession()->QueryApplication());
	SetLogPath();
	SetupLogfile(kDocWchVersion);
#ifdef CC
	LogLine("Adobe InDesign version: %s", app->GetUIVersionNumberString().GrabCString().c_str());
#else
	LogLine("Adobe InDesign version: %s", app->GetUIVersionNumberString().GrabCString());
#endif
	setupAdDocPI();
	InterfacePtr<IObserver> iObserver(UnifiedAux::GetSession(), IID_IQUITAPPLICATIONOBSERVER);
	iObserver->AutoAttach();
}

/********************************************************************************************************************/
void DocWatchStartupShutdown::Shutdown()
/********************************************************************************************************************/
{
	LogLine("alfaPlugIn deactivated.\n");
	InterfacePtr<IObserver> iObserver(UnifiedAux::GetSession(), IID_IQUITAPPLICATIONOBSERVER);
	iObserver->AutoDetach();
	cleanupAdDocPI();
}

#if WINOS
/********************************************************************************************************************/
static HWND setupWindow(LPTSTR className, LPTSTR winName, WNDPROC wndProc)
/********************************************************************************************************************/
{
	//create window for receiving messages from client apps (via WM_COPYDATA)
	WNDCLASSEX	wc;
	ZERODATA(wc);
	WNDCLASSEX	*w = &wc;

	long len = sizeof(wc);
	memset(w, 0, len);

	HINSTANCE hInst = NULL;
	w->lpfnWndProc		= wndProc;
	w->hInstance		= hInst;
	w->lpszClassName	= className;
	w->cbSize			= sizeof(WNDCLASSEX);
	ATOM atom = RegisterClassEx(w);

	int len2 = WideCharToMultiByte(CP_ACP, 0, className, 512, 0, 0, NULL, NULL);
	PSTR pCharStr = (PSTR) HeapAlloc( GetProcessHeap(), 0,	len2 * sizeof(CHAR));
	if ( pCharStr != NULL )
	{
		WideCharToMultiByte(CP_ACP, 0, className, 512, pCharStr, len2, NULL, NULL);
		HeapFree(GetProcessHeap(), 0, pCharStr); 
	}

	len2 = WideCharToMultiByte(CP_ACP, 0, className, 512, 0, 0, NULL, NULL);
	pCharStr = (PSTR) HeapAlloc( GetProcessHeap(), 0,	len2 * sizeof(CHAR));
	if ( pCharStr != NULL )
	{
		WideCharToMultiByte(CP_ACP, 0, className, 512, pCharStr, len2, NULL, NULL);
		HeapFree(GetProcessHeap(), 0, pCharStr); 
	}

	HWND hWnd = CreateWindow(className, winName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 
		NULL, NULL, hInst, NULL);

	return hWnd;
}

/*********************************  REPLACED SEE BELOW  *************************************************************/
LRESULT CALLBACK serverProcX(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
/********************************************************************************************************************/
{
	LRESULT result = 0;

	if(msg == WM_COPYDATA)
	{
		COPYDATASTRUCT *p = (COPYDATASTRUCT *) lParam;

		switch(p->dwData)
		{
		case kAdDocCmd:
			{
				///if (task_performed==0)
				///{
				///	LogLine("Main.cpp::serverProc() - plugin is busy, will return kBusy...");
				///	return kBusy;
				///}

				CMyStream *sIn = &sStreamIn;
				sIn->setup((CStr) p->lpData, p->cbData);

				int32 refnum = sIn->get(keyRefnum, 0L);
				int32 cmd = sIn->get(keyCmd, 0L);

				CMyStream *sOut = doCmd(sIn, NULL);

				if (cmd!=kSaveDoc)
				{
					sOut->add(keyRefnum, refnum);
					sOut->add(keyCmd, cmd);

					TargetApp target = sIn->getTargetApp();
					sendData(target, kAdDocResult, sOut);
				}
				else
				{
					//LogLine("Main.cpp::serverProc() - cmd==kSaveDoc...");
					return kWaitForResult;
				}

				break;
			}
		default:
			{
				result = -1;
				break;
			}
		}
	}
	else
		result = DefWindowProc(hWnd, msg, wParam, lParam);

	return result;
}

/********************************************************************************************************************
	
	Alternative message processing in serverProc SUP-30520
    ------------------------------------------------------

	The serverProc receives also messages while other messages still in process. 
	This happens within one thread! 

	Assumption: In a cooperative multitasking system message (WIN32?) 
	events will be processed during the idle time of a programm,
	within some system calls. 

	When the Messages will be received in unpredictable order the usage of global shared memory 
	for message processing is dangerous (sStreamIn sStreamOut ...).

	To avoid the effort of a plugin rewrite, messages will be now collected in a queue 
	to process all commands in sequence. 

 ********************************************************************************************************************/

class CommandMessage
{
	COPYDATASTRUCT data;

public:
	CommandMessage(COPYDATASTRUCT* inData);
	~CommandMessage();

	COPYDATASTRUCT* getData();
	int32 getCommand();
};

int32 executeCommand(std::shared_ptr<CommandMessage> message);

static std::deque<std::shared_ptr<CommandMessage>> commandQueue;

CommandMessage::CommandMessage(COPYDATASTRUCT* inData) {
	data.dwData = inData->dwData;
	data.cbData = inData->cbData;
	data.lpData = new char[inData->cbData+1];
	memcpy(data.lpData, inData->lpData, inData->cbData);
}

CommandMessage::~CommandMessage() {
	if (data.lpData != NULL) {
		delete(data.lpData);
	}
}

COPYDATASTRUCT* CommandMessage::getData() {
	return &data;
}

int32 CommandMessage::getCommand() {

	CMyStream* sInTemp = new CMyStream();
	sInTemp->setup(16384);
	sInTemp->setup((CStr)data.lpData, data.cbData);
	int32 cmd = sInTemp->get(keyCmd, 0L);
	delete(sInTemp);

	return cmd;
}

void processQueueCommands() {
	int32 cmd = -1;

	while (true) {
		if (commandQueue.size() > 0) {
			std::shared_ptr<CommandMessage> message = commandQueue.front();

			COPYDATASTRUCT* p = message->getData();
			CMyStream* sIn = &sStreamIn;
			sIn->setup((CStr)p->lpData, p->cbData);

			int32 refnum = sIn->get(keyRefnum, 0L);
			int32 cmd = sIn->get(keyCmd, 0L);

			LogLine("Begin process command: %d ref: %d cnt: %d ", cmd, refnum, commandQueue.size());

			CMyStream* sOut = doCmd(sIn, NULL);

			if (cmd != kSaveDoc)
			{
				sOut->add(keyRefnum, refnum);
				sOut->add(keyCmd, cmd);

				TargetApp target = sIn->getTargetApp();
				sendData(target, kAdDocResult, sOut);
			}
	
			LogLine("End process command: %d cnt: %d", cmd, commandQueue.size());

			if (commandQueue.size() > 0)
				commandQueue.pop_front();
			else
				break;
		}
		else
			break;
	}
}

BOOL commandInProcess(int32 cmd) {
	for (const auto& item : commandQueue) {
		if (cmd == item->getCommand())
			return TRUE;
	}
	return FALSE;
}

int32 executeCommand(std::shared_ptr<CommandMessage> message) {
	
	int32 cmd = message->getCommand();

	// ignore dupplicate saveAs messages 
	/*
	if (cmd == kSaveAs && commandInProcess(cmd)) {
		LogLine("Duplicate kSaveAs command ignored!");
		return cmd;
	}
	*/

	LogLine("Push command: %d ", cmd);

	// when the command queue is still in process (number of messages in queue is higher than 1),
	// just add the message to the queue.
	// otherwise start the message processing.
	commandQueue.push_back(message);
	if (commandQueue.size() > 1) 
		return cmd;

	processQueueCommands();

	return cmd;
}

/********************************************************************************************************************/
LRESULT CALLBACK serverProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
/********************************************************************************************************************/
{
	LRESULT result = 0;

	if (msg == WM_COPYDATA)
	{
		std::shared_ptr<CommandMessage> message = std::make_shared<CommandMessage>((COPYDATASTRUCT*)lParam);
		switch (message->getData()->dwData)
		{
			case kAdDocCmd:
			{
				int32 cmd = executeCommand(message);
				if (cmd == kSaveDoc)
					result = kWaitForResult;
				break;
			}
			default:
			{
				result = -1;
				break;
			}
		}
	}
	else
		result = DefWindowProc(hWnd, msg, wParam, lParam);

	return result;
}

#else

	static AEEventHandlerUPP sCmdProcUPP = NULL;

/********************************************************************************************************************/
CStr getAEData(AppleEvent * aevent, OSType key, int32 * pLen)
/********************************************************************************************************************/
{
		CStr p = NULL;
#ifdef CC
		Size len = 0;
#else
		int32 len = 0;
#endif

		OSErr err = AEGetParamPtr(aevent, key, typeChar, NULL, NULL, 0, &len);

		if (len > 0 && (p = (CStr) malloc(len)) != NULL)
			err = AEGetParamPtr(aevent, key, typeChar, NULL, p, len, &len);

		if (pLen != NULL)
			*pLen = len;

		return p;
	}

	
/********************************************************************************************************************
 * If plug-in is busy, then string "busy" is returned in the reply event.
 * If command is kSaveDoc - then string "wait" is returned in the reply event: it means, that sender must wait for result 
 * - it will be sent in another event (using sendData()).
 * Otherwise, result (data from the output stream) is returned in the result event.									*/
/********************************************************************************************************************/
pascal OSErr cmdProc(AppleEvent *aevent, AppleEvent *reply, int32)
/********************************************************************************************************************/
{
		char *result;
		int resultLen;

		int32 len = 0;
		CStr p = getAEData(aevent, keyDirectObject, &len);

		if(p == NULL)
			return 0;

		int32 cmd = -1;
		///if (task_performed == 0)
		///{
		///	LogLine("Main.cpp::cmdProc() - plugin is busy, will return kBusy...");
		///	result = "busy";
		///	resultLen = strlen(result) + 1;
		///}
		///else
		{
			CMyStream *sIn = &sStreamIn;
			sIn->setup(p, len);

			cmd = sIn->get(keyCmd, static_cast<int32>(0));
			int32 refnum = sIn->get(keyRefnum, static_cast<int32>(0));

			LogLine("Main.cpp::cmdProc() - cmd value = %d, name = %s", cmd, command_names[cmd]);

			switch (cmd) {
				case kNewDoc:
				case kOpenDoc:
				case kSaveAs:
				case kUpdatePicturesPathCmd:
					convertPathInStream(sIn, keyFileName, kCFURLPOSIXPathStyle, kCFURLHFSPathStyle);
				break;
					
			}
			
			// We receive paths in POSIX format and need to convert them into HFS - because all path related SDK functions
			// deal with HFS paths
			//convertPathInStream(sIn, keyFileName, kCFURLPOSIXPathStyle, kCFURLHFSPathStyle);
			//convertPathInStream(sIn, keyPictDir, kCFURLPOSIXPathStyle, kCFURLHFSPathStyle);

			CMyStream *sOut = doCmd(sIn, NULL);

			if (cmd != kSaveDoc)
			{
				sOut->add(keyRefnum, refnum);
				sOut->add(keyCmd, cmd);
				result = sOut->getData();
				resultLen = sOut->getDataLen();
			}
			else
			{
				result = "wait";
				resultLen = strlen(result) + 1;
			}
		}

		AEPutParamPtr(reply, keyAEResult, typeChar, result, resultLen);
		free(p);

		return 0;
	}

#endif

/********************************************************************************************************************/
void setupAdDocPI()
/********************************************************************************************************************/
{
#if WINOS
	HWND hClient = setupWindow(kAlfaServerClass, kAlfaServerWindow, serverProc);
#else
	sCmdProcUPP = NewAEEventHandlerUPP((AEEventHandlerProcPtr)cmdProc);
	AEInstallEventHandler(kAdDocClass, kAdDocCmd, sCmdProcUPP, 0L, false);
#endif

	sStreamIn.setup(16384);
	sStreamOut.setup(16384);
	sStreamOut2.setup(16384);
}

/********************************************************************************************************************/
void cleanupAdDocPI()
/********************************************************************************************************************/
{
#if WINOS
	UnregisterClass(kAlfaServerClass, NULL);
#else
	AERemoveEventHandler(kAdDocClass, kAdDocCallback, sCmdProcUPP, false);
	DisposeAEEventHandlerUPP(sCmdProcUPP);
#endif
}
