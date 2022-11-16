/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		AdDocPI.cpp																							*/
/*																													*/
/*	Created:																										*/
/*																													*/
/*	Modified:	17.05.08	tss	Mac-related changes: ifdefs, new getAEData() and cmdProc(), setupAdDocPI()			*/
/*				28.06.07	tss	some logging messages added to setupColorProc()										*/
/*				27.07.07	bu	saveAs("EPSF-RENEW") added															*/
/*				20.08.07    hal isCanClosePluggedDocs var. & intallDocFileHandler() added							*/
/*				04.10.07    hal porting to CS3																		*/
/*				09.11.07    hal terminate the application when the plug-in's document isn't closed- bug in CS3 ver.	*/
/*				18.05.08		1) To avoid Mac crush during terminate:												*/
/*									a) two-stage IdleTask on terminating 											*/
/*									b) doCmdCloseAll() with kAdDocCallback											*/
/*								2) getText() - uint16* -> wchar_t related changes, new logging						*/
/*								3) intallDocFileHandler() renaming to installDocFileHandler()						*/
/*								4) additional logging in doCmdOpen()												*/
/*								5) doCmdUpdatePicturesPath() - HFS path issues										*/
/*				12.06.08	bu	doSaveAs() was changed to save QXP documents as INDD documents						*/
/*				27.08.08	tss	doCmdUpdatePicturesPath() was changed to update all pictures paths,					*/
/*								if list of the pictures names is empty												*/
/*				19.09.08    hal doCmdUpdatePicturesPath(), doCmdOpen(), doSaveAs()									*/
/*				29.01.09	tss	cs4: another way of accessing ISession and using Utils, setDocTitle() changes		*/
/*							doInsertText() & addTextFrame() with IWindowList unused (in idlink) and removed			*/
/*				25.11.09	kli	saveAs("PDF") added																	*/
/*				04.12.09	kli	string LOG2( "...", strstr( format, "PDF") ); at doSaveAs deleted					*/
/*				15.12.09	kli	saveAs("TIFF") added																*/
/*				30.03.10	kli	AlfaDocManager::getInstance()->setInitialIDFile() added in doCmdNew, Open, SaveAs	*/
/*				22.04.10	kli	doInsertText() moved back, addTextFrame(IDocument* ,CMyStream* )					*/
/*								now use ILayoutControlData instead of IWindowList									*/
/*				06.10.10    hal doCmdImport() changed, now image placing is possible into the selected frame,		*/
/*								also ConvertText2GraphicFrame(),HasLinkedImage(), GetListOfSelectedFrames(),		*/
/*								GetChildFrames() added																*/
/*				27.04.2012	bu	use precompiled header																*/
/*				11.04.13	bu	MAC & CS6																			*/
/*				23.07.13	bu	SPOT / SEPARATED color type switched												*/
/*				21.10.14	bu	setupColorProc() gets parameters not via parameter list, uses globals instead		*/
/*				18.04.16	bu	SetBleeds() added																	*/
/*				18.05.16	bu	ALFA_ENFORCELOGFILE added															*/
/*				20.10.16	bu	using alfaPreset PDF fixed															*/
/*				14.12.16	i42	add functionality to import and update InDesign text variables						*/
/*																													*/
/********************************************************************************************************************/

#include "precompiled.h"

typedef struct SetupColorData 
{
	IWorkspace	*workspace;
	ISwatchList	*swatchList;
} SetupColorData;

typedef struct SetupTextVarData
{
	ITextVariableTable *textVariableTable;
} SetupTextVarData;

SetupColorData scd;

SetupColorData *GetSetupColorData() {return &scd;};

SetupTextVarData stvd;

SetupTextVarData *GetSetupTextVarData() { return &stvd; };

extern CMyStream sStreamOut;
extern CMyStream sStreamIn;
extern CMyStream sStreamOut2;

const int PATH_LENGTH=2048;
const int TEXT_LENGTH=4096;
static TargetApp target;
int	can_work=1;
int32 refnum_for_save = -1;
///int task_performed = 1;
bool8 isCanClosePluggedDocs = kFalse;
bool8 isCanSavePluggedDocs = kFalse;

/********************************************************************************************************************/
class AppTerminateIdleTask : public CIdleTask
/********************************************************************************************************************/
{
public:
	AppTerminateIdleTask(IPMUnknown* boss): CIdleTask(boss), isFirst(true) {};
	virtual ~AppTerminateIdleTask() {};

	uint32 RunTask(uint32 appFlags, IdleTimer* timeCheck);
	const char* TaskName() { return "AppTerminateIdleTask"; }
private:
	bool isFirst;
};
CREATE_PMINTERFACE(AppTerminateIdleTask, kAppTerminateIdleTaskImpl)

/********************************************************************************************************************/
uint32 AppTerminateIdleTask::RunTask(uint32, IdleTimer *)
/********************************************************************************************************************/
{
	if (isFirst)
	{
		bool16 res = CmdUtils::ProcessScheduledCmds( ICommand::kLowestPriority );
		isFirst = false;
	}
	else
	{
		InterfacePtr<ICommand> quitCmd(CmdUtils::CreateCommand(kQuitCmdBoss));
		if (CmdUtils::ScheduleCommand(quitCmd) != kSuccess)
		{
			LogLine("AppTerminateIdleTask::RunTask() - kQuitCmdBoss failed");
		}

		UninstallTask();
	}

	return 100;
}

/********************************************************************************************************************/
bool8 IsCanClosePluggedDocs()
/********************************************************************************************************************/
{
	return isCanClosePluggedDocs;
}

/********************************************************************************************************************/
bool8 IsCanSavePluggedDocs()
/********************************************************************************************************************/
{
	return isCanSavePluggedDocs;
}

/********************************************************************************************************************/
void sendSaveResult(int32 res)
/********************************************************************************************************************/
{
	CMyStream *streamOut = &sStreamOut;
	streamOut->clear();
	streamOut->add(keyRefnum, refnum_for_save);
	streamOut->add(keyCmd, kSaveDoc);
	streamOut->addResult(res);
	sendData(target, kAdDocResult, streamOut);
}

/********************************************************************************************************************/
class SaveIdleTask : public CIdleTask
/********************************************************************************************************************/
{
public:
	SaveIdleTask(IPMUnknown* boss): CIdleTask(boss) {};
	virtual ~SaveIdleTask() {};

	uint32 RunTask(uint32 appFlags, IdleTimer* timeCheck);
	const char* TaskName() { return "SaveTask"; }
}; 
CREATE_PMINTERFACE(SaveIdleTask, kSaveIdleTaskImpl)

/********************************************************************************************************************/
uint32 SaveIdleTask::RunTask(uint32, IdleTimer *)
/********************************************************************************************************************/
{
	ISession *session = UnifiedAux::GetSession();
	InterfacePtr<IApplication> app(session->QueryApplication());
	if (CheckCreation("SaveIdleTask::RunTask()", app, "IApplication"))
	{
		sendSaveResult(-1);
		return 0;
	}
	InterfacePtr<IActionManager> actionManager(app->QueryActionManager());
	if (CheckCreation("SaveIdleTask::RunTask()", actionManager, "IActionManager"))
	{
		sendSaveResult(-1);
		return 0;
	}

	actionManager->PerformAction(session->GetActiveContext(), kUndoActionID);

	UninstallTask();
	///task_performed = 1;

	return 0;
}

/********************************************************************************************************************/
static int getNumChars(IStoryList *storyList)
/********************************************************************************************************************/
{
	int32 len = 0;
	int32 n = storyList->GetUserAccessibleStoryCount();

	for(int32 i=0; i<n; i++ )
	{
		UIDRef story = storyList->GetNthUserAccessibleStoryUID(i);
		InterfacePtr<ITextModel> textModel(story, UseDefaultIID());
		if (textModel != NULL)
			len += textModel->TotalLength() - 1;
	}

	return len;	
}

/********************************************************************************************************************/
static int getNumChars(int32 refnum)
/********************************************************************************************************************/
{
	int32 len = -1;

	IDocument* doc = getDocFromRefnum(refnum);
	if (doc != NULL)
	{
		InterfacePtr<IStoryList> storyList((IPMUnknown *) doc, UseDefaultIID());
		if(storyList != NULL)
			len = getNumChars(storyList);
	}

	return len;
}

/********************************************************************************************************************/
static int getPluginVersion (CMyStream *stream)
/********************************************************************************************************************/
{
	char sVersion[256];

	sprintf(sVersion, "%s", kDocWchVersion);

	stream->add(keyVersion, sVersion);

	return 0;
}

/********************************************************************************************************************/
static void getText(int32 refnum, CMyStream *stream)
/********************************************************************************************************************/
{	
	IDocument* doc = getDocFromRefnum(refnum);

	if(doc != NULL)
	{
		InterfacePtr<IStoryList> storyList(doc, UseDefaultIID());
		if(storyList != NULL)
		{
			int32 len1 = getNumChars(storyList);
			wchar_t *textData = new wchar_t[len1 + 1];
			wchar_t *tmpTextData = textData;
			int32 n = storyList->GetUserAccessibleStoryCount();

			for(int32 i = 0; i < n; ++i)
			{
				InterfacePtr<ITextModel> textModel(storyList->GetNthUserAccessibleStoryUID(i), UseDefaultIID());
				if(textModel != NULL)
				{
					TextIterator pos1(textModel, 0);
					int32 len2 = textModel->TotalLength() - 1;
					TextIterator pos2(textModel, len2);
					for(TextIterator iter = pos1; iter < pos2; iter++)
						*tmpTextData++ = (*iter).GetValue();
				}
			}
			/* 26.03.13: maybe text exceeds buffer, cut it: */
			if (100 + stream->getBuffLen() / 2 < len1)
				len1 = (stream->getBuffLen() / 2) - 100;

			textData[len1] = 0;

			stream->add(keyTextLen, len1);
			stream->add(keyText, (CStr)textData, (uint32)(len1 * sizeof(wchar_t)));

			// This length doesn't contain last 0
			int neededLen = (int) wcstombs(0, textData, 0);

			/* it fails in case of UNI16 chars are included, use wcslen(): */
			if (neededLen == -1)
				neededLen = (int) wcslen(textData);
			char *logBuf = new char[neededLen + 1];
			wcstombs(logBuf, textData, neededLen);

			/* log buf is limited to 4072 bytes, so check this here: */
			if (neededLen >4000)
				neededLen = 4000;

			logBuf[neededLen] = 0;
			if (*logBuf)
				LogLine("AdDocPI.cpp::getText() - text = %s", logBuf);
			delete[] logBuf;
			delete[] textData;
		}
	}
}

/********************************************************************************************************************/
static int getLines(ITextModel *textModel)
/********************************************************************************************************************/
{
	int32 cnt = 0;
	int32 len = textModel->TotalLength();

	InterfacePtr<IWaxStrand> waxStrand (((IWaxStrand *) textModel->QueryStrand(kFrameListBoss, IID_IWAXSTRAND)));
	if(waxStrand != NULL)
	{
		K2::scoped_ptr<IWaxIterator> waxIterator(waxStrand->NewWaxIterator());
		if(waxIterator != NULL)
		{
			IWaxLine *waxLine = waxIterator->GetFirstWaxLine(0);

			while(waxLine != NULL)
			{
				cnt++;
				waxLine = waxIterator->GetNextWaxLine();
			}
		}
	}

	return cnt;
}

/********************************************************************************************************************/
static int getLines(int32 refnum)
/********************************************************************************************************************/
{
	int32 cnt = -1;

	IDocument* doc = getDocFromRefnum(refnum);
	if(doc != NULL)
	{
		cnt = 0;
		InterfacePtr<IStoryList> storyList((IPMUnknown *) doc, UseDefaultIID());
		if(storyList != NULL)
		{
			int32 n = storyList->GetUserAccessibleStoryCount();

			for(int32 i=0; i<n; i++ )
			{
				UIDRef story = storyList->GetNthUserAccessibleStoryUID(i);

				InterfacePtr<ITextModel> textModel(story, UseDefaultIID());

				if(textModel != NULL)
					cnt += getLines(textModel);
			}
		}
	}

	return cnt;
}

/********************************************************************************************************************/
static int getWidth(int32 refnum)
/********************************************************************************************************************/
{
	int32 w = 0;
	IDocument* doc = getDocFromRefnum(refnum);

	if(doc != NULL)
	{
		InterfacePtr<ISpreadList> spreadList((IPMUnknown *) doc, UseDefaultIID());
		InterfacePtr<IGeometry> spreadGeo(spreadList->QueryNthSpread(0));
		InterfacePtr<ISpread> spread(spreadGeo, UseDefaultIID());

		InterfacePtr<IGeometry> pageGeo(spread->QueryNthPage(0));
		PMRect r = pageGeo->GetPathBoundingBox(::InnerToPasteboardMatrix(pageGeo));

		float x1 = ToFloat(r.Left());
		float x2 = ToFloat(r.Right());

		x1 = x1 * 1000.f;
		x2 = x2 * 1000.f;

		w = (int32) (pt2mm(x2 - x1) + .5);
	}

	return w;
}

/********************************************************************************************************************/
static int getHeight(int32 refnum)
/********************************************************************************************************************/
{
	int32 h = 0;
	IDocument* doc = getDocFromRefnum(refnum);

	if(doc != NULL)
	{
		InterfacePtr<ISpreadList> spreadList((IPMUnknown *) doc, UseDefaultIID());
		InterfacePtr<IGeometry> spreadGeo(spreadList->QueryNthSpread(0));
		InterfacePtr<ISpread> spread(spreadGeo, UseDefaultIID());

		InterfacePtr<IGeometry> pageGeo(spread->QueryNthPage(0));
		PMRect r = pageGeo->GetPathBoundingBox(::InnerToPasteboardMatrix(pageGeo));

		float y1 = ToFloat(r.Top());
		float y2 = ToFloat(r.Bottom());

		y1 = y1 * 1000.f;
		y2 = y2 * 1000.f;

		h = (int32) (pt2mm(y2 - y1) + .5);
	}

	return h;
}

/********************************************************************************************************************/
static int setPagesSize(IDocument *doc, PMReal w, PMReal h)
/********************************************************************************************************************/
{
	// Create a SetPageSizeCmd
	InterfacePtr<ICommand> pageCmd(CmdUtils::CreateCommand(kSetPageSizeCmdBoss));

	// Get an IDocSetupCmdData Interface for the SetPageSizeCmd:
	InterfacePtr<IDocSetupCmdData> pageData(pageCmd, IID_IDOCSETUPCMDDATA);

	UIDRef docRef = ::GetUIDRef(doc);
	PMRect r = PMRect(0., 0., w, h);

#ifdef CS6
	pageData->SetDocSetupCmdData(docRef, PMPageSize(w, h), 0, 0, w > h, kLeftToRightBinding, kTrue); /* 12.12.16 / bu modified, dont change number of pages */
#else
	pageData->Set(docRef, r, 0, 0, w > h, kDefaultBinding);
#endif

	// Process the SetPageSizeCmd
	if (CmdUtils::ProcessCommand(pageCmd) != kSuccess)
	{
		LogLine("ERROR IN AdDocPI.cpp::setPagesSize() - can't process kSetPageSizeCmd!");
		return -1;
	}

	Utils<ILayoutUtils>()->InvalidateViews(doc);
	return noErr;
}

/********************************************************************************************************************/
static int setWidth(CMyStream *stream)
/********************************************************************************************************************/
{
	int32 refnum = stream->getRefnum();
	IDocument *doc = getDocFromRefnum(refnum);

	if(doc == NULL)
		return -1;

	float h = mm2pt(getHeight(refnum));
	int32 wi = stream->get(keyWidth, static_cast<int32>(0));
	LogLine("setWidth(%d)", wi);

	float w = mm2pt(wi);
	h = h / 1000.f;
	w = w / 1000.f;

	int32 err = setPagesSize(doc, w, h);
	return err;
}

/********************************************************************************************************************/
static int setHeight(CMyStream *stream)
/********************************************************************************************************************/
{
	int32 refnum = stream->getRefnum();
	IDocument *doc = getDocFromRefnum(refnum);

	if(doc == NULL)
		return -1;

	float w = mm2pt(getWidth(refnum));

	int32 hi = stream->get(keyHeight, static_cast<int32>(0));
	LogLine("setHeight(%d)", hi);

	float h = mm2pt(hi);

	h = h / 1000.f;
	w = w / 1000.f;

	int32 err = setPagesSize(doc, w, h);
	return err;
}

/********************************************************************************************************************/
static int  setFrontDoc(IDocument *doc)
/********************************************************************************************************************/
{
	InterfacePtr<ICommand> setFrontDocCmd(CmdUtils::CreateCommand(kSetFrontDocCmdBoss));
	setFrontDocCmd->SetItemList(UIDList(doc));
	ErrorCode status = CmdUtils::ProcessCommand(setFrontDocCmd);
	if (CheckError("AdDocPI.cpp::setFrontDoc() - kSetFrontDocCmdBoss"))
		return -1;
	return 0;
}

/********************************************************************************************************************/
static int doSetFocusCmd(CMyStream *stream)
/********************************************************************************************************************/
{
	IDocument *doc = getDocFromRefnum(stream->getRefnum());
	if (setFrontDoc(doc))
		return -1;

	return 0;
}

/********************************************************************************************************************/
static bool setupColorProc(CMyStream *stream, int32 index, CStr str1, int32 userData)
/********************************************************************************************************************/
{
///	SetupColorData *p = (SetupColorData *) userData;
	SetupColorData *p = (SetupColorData *) &scd;
	if( !p ) 
		return false;

	IWorkspace	*workspace = p->workspace;
	ISwatchList *swatchList = p->swatchList;

	if( !workspace || !swatchList ) 
		return false;

	PMString colName(str1);
	bool bFound = false;

	// loop over all colors to check the colors is already exist
	int32  numOfCols = swatchList->GetNumSwatches();
	for( int32 i=0; i< numOfCols; i++ )
	{
		UIDRef colUid = swatchList->GetNthSwatch( i ); 
		PMString cNm = Utils<ISwatchUtils>()->GetSwatchName( colUid.GetDataBase(), colUid.GetUID());

		/* bu 17.03.11 added: */
#if WINOS
		{
			char buf[100];
			cNm.GetCString(buf, 100);
			if (!strcmpi(buf, str1)) 
			{
				bFound = true;
				break;
			}	
		}
#endif
		if( cNm.IsEqual( colName, kFalse ) )
		{
			bFound = true;
			break;
		}	
	}
	
	if( !bFound )
	{
		CStr255 key = "";
		CMyStream::getKey(keyCMYK, index, key);
		CStr255 str = "";
		stream->get(key, str);

		float c, m, y, k;
		sscanf(str, "%f %f %f %f", &c, &m, &y, &k);

		/* bu 17.03.11: don't create black: */
		if ((c + m + y == 0.0) && k == 1.0)
		{
			LogLine("Color Black always exists.");
		}
		else
		{
			int32 intColorType = -1;
			CStr255 typeKey  = "";
			CStr255 typeValue = "" ;
			CMyStream::getKey(keyColorType,index,typeKey);		
			stream->get(typeKey, typeValue);
			sscanf(typeValue,"%d", &intColorType);
			LogLine("Set color %s as %s", (colName).GetPlatformString().c_str() , intColorType == 0 ? "SPOT" : "SEPARATED"); 

			// create swatch
			switch(intColorType)
			{
				case 1:
					Utils<ISwatchUtils>()->CreateProcessCMYKSwatch (workspace, colName, c, m, y, k);
				break;

				case 0:
					Utils<ISwatchUtils>()->CreateSpotCMYKSwatch(workspace, colName, c, m, y, k);
				break;

				default:
					LogLine("AdDocPI.cpp::setupColorProc() - unknown colorType");
				break;
			}
		}
	}

	return true;
}

/********************************************************************************************************************/
static bool getValueFromXml(const char* xml, const char* name, PMString &value)
/********************************************************************************************************************/
{
	if (!xml)
		return false;

	PMString xmlStr(xml);

	PMString begin = PMString("<") + PMString(name) + PMString(">");
	PMString end = PMString("</") + PMString(name) + PMString(">");

	std::string xml2 = xmlStr.GrabCString();
	std::string end2 = end.GrabCString();

	CharCounter indexBegin = xmlStr.IndexOfString(begin);
	CharCounter indexEnd = xmlStr.IndexOfString(end);

	if (indexBegin == -1 || indexEnd == -1 || (indexBegin + begin.CharCount()) > indexEnd)
		return false;

	K2::scoped_ptr<PMString> substr(xmlStr.Substring(indexBegin + begin.CharCount(), indexEnd - indexBegin - begin.CharCount()));
	if (!substr)
		value = PMString("");
	else
		value = *substr;
	return true;
}

/********************************************************************************************************************/
static bool setupTextVarsProc(CMyStream *stream, int32 index, CStr str1, int32 userData)
/********************************************************************************************************************/
{
	SetupTextVarData *p = (SetupTextVarData *)&stvd;
	if (!p)
		return false;

	ITextVariableTable *textVariableTable = p->textVariableTable;

	if (!textVariableTable)
		return false;


	// str1 looks like this: "<key>abc</key><value>def ghi</value>"
	// try to extract key and value from str1
	PMString textVarKey;
	if (!getValueFromXml(str1, keyTextVarKey, textVarKey))
		return false;

	PMString textVarValue;
	if (!getValueFromXml(str1, keyTextVarValue, textVarValue))
		return false;

	// if textVarKey already exists in the document, we update it, else we add it
	InterfacePtr<ITextVariable> textVariable(textVariableTable->QueryVariable(WideString(textVarKey)));
	
	ErrorCode result = kFailure;
	if (textVariable)
	{
		// if the value for the variable is empty, we delete the variable, else we update the contents
		if (textVarValue.empty())
		{
			//result = Utils<ITextVariableFacade>()->RemoveVariable(textVariableTable, WideString(textVarKey)); // removes the variable only

			// ... lets do it with javascript

			// first gotta do some ugly string escaping
			std::string theKey = JavascriptHelper::EscapeParameter(textVarKey.GrabCString());

			std::stringstream jsRemoveTextVar;
			jsRemoveTextVar << boost::format("var textvar = app.activeDocument.textVariables.item('%1%');\n") % theKey
				<< std::string("var instances = textvar.associatedInstances;\n")
				<< std::string("for (var index = 0; index < instances.length; ++index) {\n")
				<< std::string("instances[index].remove(); }\n")
				<< std::string("textvar.remove();\n");

			result = JavascriptHelper::RunScript(jsRemoveTextVar.str());
		}
		else
		{
			// for some reason adobe doesnt include ChangeVariableInfo in the sdk, so we cannot use it!! :((
			//ChangeVariableInfo chgInfo;
			//chgInfo.SetBaseTypeID(kCustomTextVariableBoss);
			//chgInfo.SetName(WideString(textVarKey.c_str()));
			//chgInfo.SetContents(WideString(textVarValue.c_str()));
			//result = Utils<ITextVariableFacade>()->ChangeVariable(textVariableTable, WideString(textVarKey), chgInfo);

			// ...as a workaround we use javascript to update the content of the text variable

			// first gotta do some ugly string escaping
			std::string theKey = JavascriptHelper::EscapeParameter(textVarKey.GrabCString());
			std::string theValue = JavascriptHelper::EscapeParameter(textVarValue.GrabCString());

			std::stringstream jsUpdateTextVar;
			jsUpdateTextVar << boost::format("var textvar = app.activeDocument.textVariables.item('%1%');\n") % theKey
				<< std::string("textvar.variableType = VariableTypes.CUSTOM_TEXT_TYPE;\n")
				<< boost::format("textvar.variableOptions.contents = '%1%';\n") % theValue;

			result = JavascriptHelper::RunScript(jsUpdateTextVar.str());
		}
	}
	else
	{
		if (!textVarValue.empty())
		{
			AddVariableInfo newInfo;
			newInfo.SetBaseTypeID(kCustomTextVariableBoss);
			newInfo.SetName(WideString(textVarKey));
			newInfo.SetContents(WideString(textVarValue));
			result = Utils<ITextVariableFacade>()->AddVariable(textVariableTable, newInfo);
		}
		else
		{
			LogLine("AdDocPI.cpp::setupTextVarsProc() - tried to add text variable without value");
			return true; // ignore empty values
		}
	}

	if (result == kFailure)
	{
		LogLine("AdDocPI.cpp::setupTextVarsProc() - failed to add text variables");
		return false;
	}

	return true;
}

/********************************************************************************************************************/
static void setupColors(IDocument *doc, CMyStream *stream)
/********************************************************************************************************************/
{
	InterfacePtr<IWorkspace> workspace (doc->GetDocWorkSpace(), IID_IWORKSPACE);
	InterfacePtr<ISwatchList> swatchList(workspace, UseDefaultIID());

	///SetupColorData pb = {workspace, swatchList};
	scd.workspace = workspace;
	scd.swatchList = swatchList;

	///CMyStream::EnumKey(stream, keyColName, setupColorProc, (int32) &pb);
	CMyStream::EnumKey(stream, keyColName, setupColorProc, NULL);
}

/********************************************************************************************************************/
static void setupTextVars(IDocument *doc, CMyStream *stream)
/********************************************************************************************************************/
{
	InterfacePtr<ITextVariableTable> textVarTable(doc->GetDocWorkSpace(), IID_ITEXTVARIABLETABLE);

	stvd.textVariableTable = textVarTable;

	CMyStream::EnumKey(stream, keyTextVarName, setupTextVarsProc, NULL);
}

/********************************************************************************************************************/
int setDocTitle(IDocument *doc, char *title)
/********************************************************************************************************************/
{
	PMString pm_title(title);
	doc->SetName(pm_title);
	if (CheckError("AdDocPI.cpp::setDocTitle() - SetName()", 0))
		return -1;

	Utils<IDocumentUIUtils>()->UpdatePresentationLabels(doc);


	if (CheckError("AdDocPI.cpp::setDocTitle() - updating title", 0))
		return -1;

	return 0;
}

/********************************************************************************************************************/
void installDocFileHandler( InterfacePtr<IDocument> doc )
/********************************************************************************************************************/
{
	if (doc == nil)
	{
		return;
	}

	InterfacePtr<IClassIDData> docFileHandlerData(doc, IID_ICLASSIDDATA);
	docFileHandlerData->Set(kDocWchDocFileHandlerBoss2);
}

/********************************************************************************************************************/
static void doCmdCloseAll(bool reportClosing)
/********************************************************************************************************************/
{
	
	isCanClosePluggedDocs = kTrue;
	std::list<IDocument*> docs;
	AlfaDocManager::getInstance()->getAllAlfaDocs(docs);

	std::list<IDocument*>::iterator it;
	for( it = docs.begin(); it != docs.end(); it++ )
	{
		IDocument* doc = *it;
		DocDesc *docDesc = AlfaDocManager::getInstance()->getDataForAlfaDoc(doc);
		closeDocAux(doc);
		if( reportClosing )
		{
			sStreamOut2.setup(kDocHasBeenClosed, docDesc->refnum);
			sendData(docDesc->target, kAdDocCallback, &sStreamOut2);
		}
	}
	isCanClosePluggedDocs = kFalse;
	
}

/********************************************************************************************************************/
static int doCmdNew(CMyStream *stream)
/********************************************************************************************************************/
{
	char path[PATH_LENGTH], title[TEXT_LENGTH];
	memset(path, 0, PATH_LENGTH);
	memset(title, 0, TEXT_LENGTH);

	stream->get(keyFileName, path);
	stream->get(keyTitle, title);
	PMReal width = stream->getPt(keyWidth);
	PMReal height = stream->getPt(keyHeight);
	PMReal top = stream->getPt(keyTop);
	PMReal bottom = stream->getPt(keyBottom);
	PMReal left	= stream->getPt(keyLeft);
	PMReal right = stream->getPt(keyRight);
	int32 pg_num = stream->get(keyNumPages, 1);
	PMReal grid_width = stream->getPt(keyGridWidth);
	PMReal grid_dist = stream->getPt(keyGridDistance);
	int32 fixed_border = stream->get(keyFixedBorder, 1);
	PMReal bleedTop = stream->get(keyBleedTop, 1);
	PMReal bleedBottom = stream->get(keyBleedBottom, 1);
	PMReal bleedLeft = stream->get(keyBleedLeft, 1);
	PMReal bleedRight = stream->get(keyBleedRight, 1);
	int32  width_um = stream->get(keyWidth, 1);
	int32  height_um = stream->get(keyHeight, 1);

	int col_num = 1;

	AlfaDocManager* pDm = AlfaDocManager::getInstance();
	int32 docID = stream->getRefnum();

	// does a document already exist with such ID ?
	IDocument* pd = pDm->getAlfaDocByID(docID);
	if( pd != NULL )
	{
		LogLine("AdDocPI.cpp::doCmdNew() - the document already exists with ID = %d", docID );
		return -1;
	}

	LogLine("Create new document, width=%ld, height=%ld, bleedTop=%ld, bleedBottom=%ld, bleedLeft=%ld, bleedRight=%ld",
		width_um, height_um, ToUInt32(bleedTop), ToUInt32(bleedBottom), ToUInt32 (bleedLeft), ToUInt32 (bleedRight));

	// create  the document & attach data
	UIDRef docRef = pDm->createDocument(width, height, left, top, right, bottom, col_num, grid_dist,
		bleedTop, bleedBottom, bleedLeft, bleedRight, pg_num);

	pDm->attachDataToDocument( docRef, docID, stream->getTargetApp(),
		path, pg_num, fixed_border );

	SDKLayoutHelper sdkLayoutHelper;
	ErrorCode err = sdkLayoutHelper.OpenLayoutWindow(docRef);
	if (CheckErrorCode("AdDocPI.cpp::doCmdNew()", err, "open document window"))
		return -1;

	InterfacePtr<IDocument> doc(docRef, UseDefaultIID());
	if (CheckCreation("AdDocPI.cpp::doCmdNew()", doc, "IDocument"))
		return -1;

	SDKFileHelper fHelper(path);

	AlfaDocManager::getInstance()->setInitialIDFile( docRef, fHelper.GetIDFile() );

	err = sdkLayoutHelper.SaveDocumentAs(docRef, fHelper.GetIDFile());
	if (CheckErrorCode("AdDocPI.cpp::doCmdNew()", err, "save as document"))
		return -1;

	if ( title[0]!=0 )
	{
		if (setDocTitle(doc, title))
			LogLine("ERROR IN AdDocPI.cpp::doCmdNew() - Setting title failed!");
	}

	installDocFileHandler(doc);

	return noErr;
}

/********************************************************************************************************************/
static int doCmdOpen(CMyStream *stream)
/********************************************************************************************************************/
{

	char path[PATH_LENGTH], title[TEXT_LENGTH];
	memset(path, 0, PATH_LENGTH);
	memset(title, 0, TEXT_LENGTH);

	stream->get(keyFileName, path);
	stream->get(keyTitle, title);
	int fixedBorder = stream->get(keyFixedBorder, 1);

	SDKFileHelper fHelper(path);
	SDKLayoutHelper sdkLayoutHelper;


	AlfaDocManager* pDm = AlfaDocManager::getInstance();
	int32 docID = stream->getRefnum();
	
	// does a document already exist with such ID ?
	IDocument* pd = pDm->getAlfaDocByID(docID);
	if( pd != NULL )
	{
		LogLine("AdDocPI.cpp::doCmdOpen() - the document already exists with ID = %d", docID );
		return -1;
	}

	/* bu 25.01.12: check for open flags: */
	PluginConfiguration config = XMLConfigurationReader::getInstance()->GetPluginConfiguration();

	target = stream->getTargetApp();
	UIDRef docRef = sdkLayoutHelper.OpenDocument(fHelper.GetIDFile(), config.showOpenDocMessages() ? K2::kFullUI : K2::kSuppressUI);
	if (docRef == UIDRef::gNull)
	{
		LogLine("AdDocPI.cpp:: doCmdOpen() - document was NOT opened");
		return -1;
	}

	if (sdkLayoutHelper.OpenLayoutWindow(docRef)) {
		LogLine("AdDocPI.cpp:: OpenLayoutWindow() failed");
		return -1;
	}

	InterfacePtr<IDocument> doc(docRef, UseDefaultIID());
	if (CheckCreation("AdDocPI.cpp::doCmdOpen()", doc, "IDocument")) {
		LogLine("AdDocPI.cpp:: CheckCreation(IDocument) failed");
		return -1;
	}

	InterfacePtr<IPageList> pageList(doc, UseDefaultIID());
	if (CheckCreation("AdDocPI.cpp::doCmdOpen()", pageList, "IPageList")) {
		LogLine("AdDocPI.cpp:: CheckCreation(IPageList) failed");
		return -1;
	}

	pDm->attachDataToDocument( docRef, docID, stream->getTargetApp(),
		path, pageList->GetPageCount(), fixedBorder );

	AlfaDocManager::getInstance()->setInitialIDFile( docRef, fHelper.GetIDFile() );

	if (title[0]!=0)
	{
		if (setDocTitle(doc, title))
			LogLine("ERROR IN AdDocPI.cpp::doCmdOpen() - Setting title failed!");
	}

	setupColors(doc, stream);
	installDocFileHandler(  doc  );

	// get a folder for current document as base folder to relink images
	std::string newpath(path);
#ifndef WINOS
	const char * const separator = ":";
#else
	const char * const separator = "\\";
#endif

	std::basic_string<char>::size_type sepIndex = -1;
	sepIndex = newpath.find_last_of( separator );
	if( sepIndex != -1 )
		newpath = newpath.substr( 0, sepIndex );

	std::list<char*> names;
	
	// relink & update 
	ErrorCode res = ImageLinkUtils::RelinkPictures(doc, WideString(newpath.c_str()), names, false);
	if (res != kSuccess)
	{
		LogLine("ERROR IN AdDocPI.cpp::doCmdOpen() - relinking failed, code = %d", res);
		return -1;
	}

	return noErr;
}

/********************************************************************************************************************/
int SaveIndesignDocument( const UIDRef& docRef,  UIFlags uiFlags = K2::kSuppressUI ) 
/********************************************************************************************************************/
{
	if( !docRef )	
		return kFailure;

	InterfacePtr<IDocFileHandler> docHandler(Utils<IDocumentUtils>()->QueryDocFileHandler(docRef));
	if (!docHandler) 
		return kFailure;

	ErrorCode result = kFailure;
	docHandler->Save (docRef,  uiFlags);
	result = ErrorUtils::PMGetGlobalErrorCode();

	return result;
}

/********************************************************************************************************************/
int SaveIndesignDocumentAs( const UIDRef& docRef, const PMString& fpath, UIFlags uiFlags = K2::kSuppressUI ) 
/********************************************************************************************************************/
{
	if( !docRef )	
		return kFailure;

	InterfacePtr<IDocFileHandler> docHandler(Utils<IDocumentUtils>()->QueryDocFileHandler(docRef));
	if (!docHandler) 
		return kFailure;

	IDFile idfile(fpath);
	ErrorCode result = kFailure;

	docHandler->SaveAs (docRef, &idfile, uiFlags);
	result = ErrorUtils::PMGetGlobalErrorCode();

	return result;
}

/********************************************************************************************************************/
static void doCmdSaveTimer(CMyStream *stream)
/********************************************************************************************************************/
{
	ErrorCode err;
	isCanSavePluggedDocs = kTrue;
	refnum_for_save = stream->getRefnum();
	IDocument *doc = getDocFromRefnum(refnum_for_save);
	if (doc == NULL)
	{
		isCanSavePluggedDocs = kFalse;
		sendSaveResult(-1);
		return;
	}

#ifndef WINOS
	err = Utils<IDocumentUtils>()->DoSave(doc);
#else
	
	AlfaDocManager* docMng = AlfaDocManager::getInstance();
	IDFile  destFileID = docMng->getInitialIDFile( ::GetUIDRef(doc) );
	
	PMString destFilePath = destFileID.GetString();
	//PMString destFilePath = destFileID.GetFileName();
	//@TODO 
#ifdef CC
	LogLine("AdDocPI.cpp::doCmdSaveTimer() destination file path: %s", destFilePath.GrabCString().c_str() );
#else
	LogLine("AdDocPI.cpp::doCmdSaveTimer() destination file path: %s", destFilePath.GrabCString());
#endif
		
	err = SaveIndesignDocumentAs( ::GetUIDRef(doc ), destFilePath);
	
#endif

	if (err != kSuccess)
		LogLine("ERROR IN AdDocPI.cpp::doCmdSaveTimer() - during DoSave()!");

	isCanSavePluggedDocs = kFalse;
	sendSaveResult(err);
	return;
	
}

/********************************************************************************************************************/
static int32 doSaveAs(CMyStream *stream)
/********************************************************************************************************************/
{
	bool epsWithFonts;
	int32 refnum = stream->getRefnum();

	CStr255 path = "",format = "";
	stream->get(keyFileName, path);
	stream->get(keyFormat, format);
	int32 pgNum = stream->get(keyPageNum, 1);

	PMReal bleedTop = stream->get(keyBleedTop, 1);
	PMReal bleedBottom = stream->get(keyBleedBottom, 1);
	PMReal bleedLeft = stream->get(keyBleedLeft, 1);
	PMReal bleedRight = stream->get(keyBleedRight, 1);

	LogLine("AdDocPI.cpp::doSaveAs()  - pgNum = %d, path = %s, format = %s", --pgNum, path, format);

	/* 27.07.07 bu added: */
	/* special case: format is "EPSF_RENEW" - open doc, make eps, close doc */
	if (strstr(format, "RENEW"))
	{
		CStr255	epsFileName;
		char* p;

		strcpy(epsFileName, path);
		p = strrchr(epsFileName, '.');
		if (p)
			strcpy(p, ".eps");

		SDKFileHelper fHelperIND(path);
		SDKFileHelper fHelperEPS(epsFileName);

		SDKLayoutHelper sdkLayoutHelper;
		UIDRef docRef = sdkLayoutHelper.OpenDocument(fHelperIND.GetIDFile(), K2::kSuppressUI);

		InterfacePtr<IDocument> doc(docRef, UseDefaultIID());

		saveAsEPS(doc, pgNum, fHelperEPS.GetIDFile(), bleedTop, bleedBottom, bleedLeft,bleedRight, false, false);

		InterfacePtr<IDocFileHandler> docFileHandler(Utils<IDocumentUtils>()->QueryDocFileHandler(docRef));
		docFileHandler->Close(docRef, kSuppressUI, kFalse, IDocFileHandler::kSchedule);
		return 0;
	}

	/* bu: moved from upper lines to here: */
	IDocument *doc = getDocFromRefnum(refnum);
	if (doc == NULL) 
		return -1;

	/* bu added 12.06.08 */
	if (strstr(format, "INDD"))
	{
		/* origin file format was probable QuarkXPress; just calling "Save()" would lead to "Save" dialog */
		SDKFileHelper fHelperIND(path);

		doc->SaveAs(fHelperIND.GetIDFile());

		if( fHelperIND.IsExisting() )
		{
			AlfaDocManager::getInstance()->setInitialIDFile( ::GetUIDRef(doc), fHelperIND.GetIDFile() );

			LogLine("AdDocPI.cpp::doSaveAs() - IDFile was saved, path = %s", fHelperIND.GetPath().GetPlatformString().c_str() );
		}

		return 0;
	}

	/* 25.09.07 bu added: */
	epsWithFonts = strstr(format, "WITHFONTS") ? true : false;

	if (strstr(format, "ASCII") != NULL)
	{
		// save as EPS -ASCII
		SDKFileHelper fHelper(path);

		saveAsEPS(doc, pgNum, fHelper.GetIDFile(), bleedTop, bleedBottom, bleedLeft, bleedRight, false, epsWithFonts);
	}
	else if(strstr(format, "EPSF") != NULL)
	{
		// save as EPS -binary
		SDKFileHelper fHelper(path);

		saveAsEPS(doc, pgNum, fHelper.GetIDFile(), bleedTop, bleedBottom, bleedLeft, bleedRight, true, epsWithFonts);
	}
	else if( strstr( format, "PDF" ) != NULL )
	{
		SDKFileHelper fHelper(path);
		LogLine( "AdDocPI.cpp::doSaveAs() - saving as PDF..." );
		saveAsPDF( doc, pgNum, fHelper.GetIDFile(), path, format, bleedTop, bleedBottom, bleedLeft, bleedRight );
	}
	else 
		return -1;

	return noErr;
}

/********************************************************************************************************************/
int closeDocAux(IDocument *doc)
/********************************************************************************************************************/
{
	UIDRef docRef = ::GetUIDRef(doc);
	DocDesc *docDesc = getDoc(doc);
	if (docDesc!=NULL)
	{
		InterfacePtr<IObserver> docObserver( docDesc->getDoc(), IID_ILOCKAREAOBSERVER );
		if( docObserver == NULL )
			; //LogLine("AdDocPI.cpp::closeDocAux() - doc hasn't observer...");
		else
		{
			LogLine("AdDocPI.cpp::closeDocAux() - will detach observer...");
			docObserver->AutoDetach();
		}
	}

	InterfacePtr<IDocFileHandler> docFileHandler(Utils<IDocumentUtils>()->QueryDocFileHandler(docRef));
	if (CheckCreation("AdDocPI.cpp::closeDocAux()", docFileHandler, "docFileHandler", 1))
		return -1;

	if (docFileHandler->CanClose(docRef))
	{
		LogLine("AdDocPI.cpp::closeDocAux(), close doc");
		docFileHandler->Close(docRef, kSuppressUI, kFalse, IDocFileHandler::kSchedule);
	}

#ifdef WINOS
	HWND hwnd = FindWindow(L"InDesign", NULL);


	if (!hwnd) {
		LogLine("InDesign window not found.");
		return 0;
	}

	ShowWindow(hwnd, SW_SHOWMINIMIZED); 
#endif

	return 0;
}

/********************************************************************************************************************/
static int32 doCmdClose(int32 refnum)
/********************************************************************************************************************/
{
	return AlfaDocManager::getInstance()->closeAlfaDoc(refnum);
}

/********************************************************************************************************************/
static int32 doSetColors(CMyStream *stream)
/********************************************************************************************************************/
{
	int32 refnum = stream->getRefnum();

	IDocument *doc = getDocFromRefnum(refnum);
	if(doc == NULL)
	{
		return -1;
	}
	setupColors(doc, stream);

	return noErr;
}

/********************************************************************************************************************/
static int32 doSetTextVars(CMyStream *stream)
/********************************************************************************************************************/
{
	int32 refnum = stream->getRefnum();

	IDocument *doc = getDocFromRefnum(refnum);
	if (doc == NULL)
	{
		return -1;
	}

	setupTextVars(doc, stream);

	return noErr;
}

/********************************************************************************************************************/
static void addTextFrame(UIDRef spread, UIDRef page, PMRect &r1, PMString *str)
/********************************************************************************************************************/
{
	PMRect r2(r1);

	InterfacePtr<ITransform> transform(page, UseDefaultIID());
	if (!transform) return;
	PMRect marginBBox;
	InterfacePtr<IMargins> margins(transform, IID_IMARGINS);
	// Note it's OK if the page does not have margins.
	if( margins )
		margins->GetMargins( &marginBBox.Left(), &marginBBox.Top(), &marginBBox.Right(), &marginBBox.Bottom() );

	r2.MoveRel(marginBBox.Left(), marginBBox.Top());

	::TransformInnerRectToParent(transform, &r2);

	r2.Right( r2.Right() - marginBBox.Left() - marginBBox.Right() );
	r2.Bottom( r2.Bottom() - marginBBox.Top() - marginBBox.Bottom() );

	UIDRef textModelRef;
	SDKLayoutHelper layoutHelper;
	UIDRef result = layoutHelper.CreateTextFrame(spread, r2, 1, kFalse, &textModelRef);

	InterfacePtr<ITextModel> textModel(textModelRef, UseDefaultIID());
	InterfacePtr<ITextModelCmds> textModelCmds(textModel, UseDefaultIID());
	if( !textModelCmds ) 
		return;

    boost::shared_ptr<WideString> text(new WideString(*str));

	TextIndex pos = 0;
	InterfacePtr<ICommand> insertCmd(textModelCmds->InsertCmd(pos, text));

	ErrorCode status = CmdUtils::ProcessCommand(insertCmd);
}

/********************************************************************************************************************/
static void addTextFrame(IDocument *doc, CMyStream *stream)
/********************************************************************************************************************/
{
	InterfacePtr<ILayoutControlData> layoutData( UnifiedAux::GetSession()->GetActiveContext()->GetContextView(), 
		UseDefaultIID() );
	if( !layoutData ) 
		return;

	InterfacePtr<ISpread> spread( layoutData->GetSpreadRef(), IID_ISPREAD );

	if( !spread ) 
		return;

	InterfacePtr<IDocumentLayer> docLayer( Utils<ILayerUtils>()->QueryDocumentActiveLayer( doc ) );
	if( !docLayer ) 
		return;

	InterfacePtr<ISpreadLayer> spreadLayer( spread->QueryLayer( docLayer ) );
	if( !spreadLayer ) 
		return;

	UIDRef parent = ::GetUIDRef( spreadLayer );

	InterfacePtr<IGeometry> pageGeo(spread->QueryNthPage(0));
	PMRect r = pageGeo->GetPathBoundingBox(::ParentToPasteboardMatrix(pageGeo));
	PMReal width = r.Width();
	PMReal height = r.Height();

	int refnum = AlfaDocManager::getInstance()->getIDForAlfaDoc(doc);
	PMRect r1( 0, 0, width, height );

	UIDRef docRef = ::GetUIDRef(doc);

	InterfacePtr<IPageList> pageList(docRef, UseDefaultIID());
	UIDRef page = UIDRef(docRef.GetDataBase(), pageList->GetNthPageUID(0));

	char text[10001] = "";
	stream->get(keyText, text, 10000);

	PMString str(text);

	addTextFrame(parent, page, r1, &str);
}

/********************************************************************************************************************/
static OSErr doInsertText(CMyStream *stream)
/********************************************************************************************************************/
{
	int32 refnum = stream->getRefnum();

	IDocument *doc = getDocFromRefnum(refnum);

	if(doc != NULL)
		addTextFrame(doc, stream);

	return noErr;	
}

/********************************************************************************************************************/
static int32 doTerminate()
/********************************************************************************************************************/
{
	doCmdCloseAll(true);
	InterfacePtr<IIdleTask> task(UnifiedAux::GetSession(), IID_IAPPTERMINATEIDLETASK);
	if(task != NULL)
		task->InstallTask(100);

	return noErr;
}

/********************************************************************************************************************/
int getMargins(IDocument *doc, int pgNum, PMRect *rect)
/********************************************************************************************************************/
{
	InterfacePtr<IPageList> pages(doc, UseDefaultIID());
	if (CheckCreation("AdDocPI.cpp::getMargins()", pages, "IPageList"))
		return -1;

	UID page = pages->GetNthPageUID(pgNum);
	if (!page.IsValid())
	{
		LogLine("ERROR IN AdDocPI.cpp::getMargins() - bad page UID returned!");
		return -1;
	}

	InterfacePtr<IMargins> margins(pages->QueryPageMargins(page));
	if (margins==NULL)
	{
		LogLine("ERROR IN AdDocPI.cpp::getMargins() - bad IMargins pointer returned!");
		return -1;
	}

	PMReal rleft, rtop, rright, rbottom;
	margins->GetMargins(&(rect->Left()), &(rect->Top()), &(rect->Right()), &(rect->Bottom()));

	return 0;
}

/********************************************************************************************************************/
static int32 doCmdGeometry(CMyStream *streamIn, CMyStream *streamOut)
/********************************************************************************************************************/
{
	int32 pgNum = streamIn->get(keyPageNum, 1);
	LogLine("AdDocPI.cpp::doCmdGeometry() - corrected pgNum = %d", --pgNum);

	int32 refnum = streamIn->getRefnum();
	IDocument *doc = getDocFromRefnum(refnum);
	if (CheckCreation("AdDocPI.cpp::doCmdGeometry()", doc, "IDocument"))
		return -1;

	PMRect margins;
	getMargins(doc, pgNum, &margins);

	LogLine("AdDocPI.cpp::doCmdGeometry() - left = %d, top = %d", ToFloat(margins.Left()), ToFloat(margins.Top()));
	LogLine("AdDocPI.cpp::doCmdGeometry() - right = %d, bottom = %d", ToFloat(margins.Right()), ToFloat(margins.Bottom()));

	int32 left, top, right, bottom;
	left = (int32)(pt2mm((float)(ToFloat(margins.Left())*1000.f)) + 0.5);
	top = (int32)(pt2mm((float)(ToFloat(margins.Top())*1000.f)) + 0.5);
	right = (int32)(pt2mm((float)(ToFloat(margins.Right())*1000.f)) + 0.5);
	bottom = (int32)(pt2mm((float)(ToFloat(margins.Bottom())*1000.f)) + 0.5);

	LogLine("AdDocPI.cpp::doCmdGeometry() - left = %d, top = %d", left, top);
	LogLine("AdDocPI.cpp::doCmdGeometry() - right = %d, bottom = %d ", right, bottom);

	streamOut->add(keyLeft, left);
	streamOut->add(keyTop, top);
	streamOut->add(keyRight, right);
	streamOut->add(keyBottom, bottom);
	streamOut->add(keyWidth, getWidth(refnum));
	streamOut->add(keyHeight, getHeight(refnum));

	return 0;
}

/********************************************************************************************************************/
static int doSetBleeds(CMyStream *stream)
/********************************************************************************************************************/
{
	/* at first, we need the current height and width: */
	int32 refnum = stream->getRefnum();

	float h = mm2pt(getHeight(refnum) / 1000.);
	float w = mm2pt(getWidth(refnum) / 1000.);

	/* then, get the bleed values from stream: */
	PMReal bleedTop = stream->get(keyBleedTop, 1);
	PMReal bleedBottom = stream->get(keyBleedBottom, 1);
	PMReal bleedLeft = stream->get(keyBleedLeft, 1);
	PMReal bleedRight = stream->get(keyBleedRight, 1);

	LogLine("SetBleeds(): Left=%.0f, Top=%.0f, Right=%.0f, Bottom=%.0f", ToDouble(bleedLeft), ToDouble(bleedTop), ToDouble(bleedRight), ToDouble(bleedBottom));

	/* now the values in points: */
	PMRect bleedBox;
	bleedBox.Left(mm2pt(bleedLeft / 1000.f));
	bleedBox.Top(mm2pt(bleedTop / 1000.f));
	bleedBox.Right(mm2pt(bleedRight / 1000.f));
	bleedBox.Bottom(mm2pt(bleedBottom / 1000.f));

	// Create a SetPageSizeCmd  

	InterfacePtr<ICommand> pageCmd(CmdUtils::CreateCommand(kSetPageSetupPrefsCmdBoss));

	// Get an IDocSetupCmdData  for the SetPageSizeCmd:
	InterfacePtr<IDocSetupCmdData> pageData(pageCmd, IID_IDOCSETUPCMDDATA);
	
	IDocument *doc = getDocFromRefnum(refnum);
	UIDRef docRef = ::GetUIDRef(doc);

#ifdef CS6
	pageData->SetDocSetupCmdData(docRef, PMPageSize(w, h), 0, 0, w > h, kLeftToRightBinding, kFalse); /* bu 13.12.16 Dont change number of pages */
#else
	pageData->Set(docRef, PMRect(0., 0., w, h), 0, 0, w > h, kDefaultBinding);
#endif

	pageData->SetUseUniformBleed(false);
	pageData->SetBleedBox(bleedBox);

	// Process the SetPageSizeCmd
	if (CmdUtils::ProcessCommand(pageCmd) != kSuccess)
	{
		LogLine("ERROR IN AdDocPI.cpp::setPagesSize() - can't process kSetPageSizeCmd!");
		return -1;
	}

	Utils<ILayoutUtils>()->InvalidateViews(doc);
	return noErr;

}

/********************************************************************************************************************/
void doTestCmd(CMyStream *stream)
/********************************************************************************************************************/
{
	int32 refnum = stream->getRefnum();
	IDocument *doc = getDocFromRefnum(refnum);
	if (CheckCreation("AdDocPI.cpp::doTestCmd()", doc, "IDocument", 1))
		return;
	IDataBase *db = GetDataBase(doc);

	CAlert::InformationAlert("Dummy command");
}

/********************************************************************************************************************/
static  int doCmdUpdatePicturesPath(CMyStream *stream)
/********************************************************************************************************************/
{
	char path[PATH_LENGTH];
	stream->get(keyFileName, path);

	IDocument* doc = getDocFromRefnum(stream->getRefnum());
	if( !doc ) 
		return -1;

	std::list<char *> fileList;

	stream->stream2vector(keyText, fileList, PATH_LENGTH);

#if WINOS
	std::string inPath = path;

	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &inPath[0], (int)inPath.size(), NULL, 0);
	std::wstring pathStr(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &inPath[0], (int)inPath.size(), &pathStr[0], size_needed);

	int res = ImageLinkUtils::RelinkPictures(doc, WideString(pathStr.c_str()), fileList, true);
#else
	int res = ImageLinkUtils::RelinkPictures(doc, WideString(path), fileList, true);
#endif
	
	// remove the names of files from the list
	for (std::list<char *>::iterator it = fileList.begin(); it!=fileList.end(); ++it)
		delete[] *it;

	return (res == 0) ? noErr : -1;
}

/********************************************************************************************************************/
static int32 doCmdImport(CMyStream *stream)
/********************************************************************************************************************/
{
	char path[PATH_LENGTH];
	memset(path, 0, PATH_LENGTH);
	stream->get(keyFileName, path);
	PMReal xpos = stream->getPt(keyXPos);
	PMReal ypos = stream->getPt(keyYPos);
	int32 pgNum = stream->get(keyPageNum, 1);

	//get document
	int32 refnum = stream->getRefnum();
	IDocument *document = getDocFromRefnum(refnum);
	if (!document)	return -1;
	
	WideString stPath( path );
	SDKFileHelper fHelper(stPath);
	if ( fHelper.IsExisting() == kFalse )
	{
		LogLine("ERROR: in doCmdImport() - file doesn't exist");
		return false;
	}

	// in PlaceImage(), a page index is 0-based
	pgNum -- ;
	bool res = PlaceImage( fHelper.GetIDFile(), document, pgNum, xpos, ypos, true );
	return (res) ? 0 : -1;
}

/********************************************************************************************************************/
CMyStream *doCmd(CMyStream *streamIn, int32 *pErr)
/********************************************************************************************************************/
{
	int32 cmd = streamIn->get(keyCmd, static_cast<int32>(0));
	LogLine("AdDocPI.cpp::doCmd(): %s", command_names[cmd]);

	target =  streamIn->getTargetApp();

	CMyStream *streamOut = &sStreamOut;

	streamOut->clear();

	int32 refnum = streamIn->getRefnum();
	int32 result = -1;
		
	switch(cmd) {
		case kGetVersion:
			result = getPluginVersion(streamOut);
		break;

		case kNewDoc:
			result = doCmdNew(streamIn);
		break;

		case kOpenDoc:
			result = doCmdOpen(streamIn);
		break;

		case kSaveDoc:
			doCmdSaveTimer(streamIn);
		return NULL;

		case kCloseDoc:
			result = doCmdClose(refnum);
		break;

		case kInsertText:
			result = doInsertText(streamIn);
		break;

		case kGetCharacters:
			result = getNumChars(refnum);
		break;

		case kGetLines:
			result = getLines(refnum);
		break;

		case kGetTextStart:
			getText(refnum, streamOut);
		break;

		case kGetWidth:
			result = getWidth(refnum);
		break;

		case kGetHeight:
			result = getHeight(refnum);
		break;

		case kSetWidth:
			result = setWidth(streamIn);
		break;

		case kSetHeight:
			result = setHeight(streamIn);
		break;

		case kSetFocus:
			result = doSetFocusCmd(streamIn);
		break;

		case kTerminate:
			result = doTerminate();
		break;

		case kSaveAs:
			result = doSaveAs(streamIn);
		break;

		case kSetColors:
			result = doSetColors(streamIn);
		break;

		case kGetColors:
			result = getColors(refnum, streamIn, streamOut);
		break;

		case kGetPictures:
			result = getPictures(refnum, streamOut);
			//LogLine("stream: %s", streamOut->getData());
		break;

		case kImportImage:
			result = doCmdImport(streamIn);
		break;

		case kSetPictDir:
		break;

		case kGetPageGeometry:
			result = doCmdGeometry(streamIn, streamOut);
		break;

		case kUpdatePicturesPathCmd:
			result = doCmdUpdatePicturesPath(streamIn);
		break;

		case kSetBleeds:
			result = doSetBleeds(streamIn);
		break;

		case kSetTextVars:
			result = doSetTextVars(streamIn);

		case kLastCmd:
			result = 0;
///			doTestCmd(streamIn);
		break;
	}

	streamOut->addResult(result);

	return streamOut;
}
