/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		IndComm.cpp																							*/
/*																													*/
/*	Created:																										*/
/*																													*/
/*	Modified:	20.08.07    hal	logging message in sendData() method added											*/
/*				11.05.08	tss	sendData() Mac code added															*/
/*				21.05.08	tss call to convertPathStyle() before sending data was added (Mac only)					*/
/*				29.05.08	tss we don't need to convert path, when command is kPicHasBeenDropped+					*/
/*				05.02.09	tss image paths styles changed in proper places instead of changing them  in sendData()	*/
/*				11.04.14	bu	MAC & CS6																			*/
/*																													*/
/********************************************************************************************************************/

#include "precompiled.h"

#ifndef WINOS
#include "MacPathUtils.h"
#endif


/********************************************************************************************************************/
void setFrontProcess(TargetApp target)
/********************************************************************************************************************/
{
#ifdef _WINDOWS
	SetForegroundWindow(target);
#else
	SetFrontProcess(&target);
#endif
}

/********************************************************************************************************************/
void setFrontProcess(int32 refnum)
/********************************************************************************************************************/
{
	TargetApp target = getClient(refnum);
	setFrontProcess(target);
}

/********************************************************************************************************************/
int32 sendData(TargetApp target, int32 eventID, CMyStream *stream)
/********************************************************************************************************************/
{
	int32 err = noErr;

#ifndef WINOS
	AEEventClass evClass = kAdDocClass;
	AEAddressDesc addr = {typeNull, NULL};

	if((err = AECreateDesc(typeProcessSerialNumber, &target, sizeof(target), &addr)) == noErr)
	{
		AppleEvent	aevent = {typeNull, NULL};

		if((err = AECreateAppleEvent(evClass, eventID, &addr, kAutoGenerateReturnID, kAnyTransactionID, &aevent)) == noErr)
		{
			err = AEPutParamPtr(&aevent, keyDirectObject, typeChar, stream->getData(), stream->getDataLen());
			AppleEvent reply = {typeNull, NULL};
			if (eventID == kAdDocResult)
				LogLine("IndComm.cpp::sendData() - send kAdDocResult...");
			else if (eventID == kAdDocCallback)
				LogLine("IndComm.cpp::sendData() - send kAdDocCallback...");
			else
				LogLine("IndComm.cpp::sendData() - send unknown event, id = %d", eventID);

			err = AESend(&aevent, &reply, kAENoReply, kAENormalPriority, 0, NULL, NULL);
			AEDisposeDesc(&aevent);
		}

		AEDisposeDesc(&addr);
	}
#else
	HWND hWnd = target;

	if(hWnd != NULL) {
		COPYDATASTRUCT data;
		ZERODATA(data);

		data.dwData = eventID;
		data.cbData = stream->getDataLen();
		data.lpData = stream->getData();

		//err = (int32) SendMessage(hWnd, WM_COPYDATA, (WPARAM) 0, (LPARAM) &data);

		DWORD_PTR dwResult = 0;
		err = (int32) SendMessageTimeout(hWnd, WM_COPYDATA, (WPARAM)0, (LPARAM)&data, SMTO_BLOCK, 5000, &dwResult);

		if (err == 0) {
			if(GetLastError() == ERROR_TIMEOUT)
				LogLine("sendData() - execution finished with timeout");
			else
				LogLine("sendData() - execution finished with err = %d", GetLastError());
		}
	}
#endif

	return err;
}