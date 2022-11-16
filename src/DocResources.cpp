/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		DocResources.cpp																					*/
/*																													*/
/*	Created:	20.06.07	tss																						*/
/*																													*/
/*	Modified:	28.06.07	tss	getColors() changed: keyColOption added and old getColors() function,				*/
/*								in fact, renamed to getSpotColors(). New function getAllColors() added				*/
/*				06.09.07	hal	saveAsEPS(),setEPSPrefs() changed: saving in Binary/ASCII ESP formats				*/
/*				12.09.07    hal	saveDocIntoTempPSFile(), getActiveColorsFrom(), getActiveColorNames() added			*/
/*				18.05.07    tss	additional logging of error results in different places								*/
/*								updateFileName() HSF issues  fixed													*/
/*								updatePicProc() massive changes - see details below									*/
/*								unused disableWindowMenus() is commented											*/
/*								deleting temporary file after getting colors in getActiveColorsFrom()				*/
/*								saveDocIntoTempPSFile() - getting temp file & other Mac path issues.				*/
/*				21.05.2008	tss	getPicProc() changed to send POSIX paths instead of HFS (Mac only)					*/
/*				19.09.08    hal saveAsEPS(), setEPSPrefs() - a bleed for EPS export									*/
/*				10.12.08    tss processColors and spotColors type changed from vector to set to avoid repeating		*/
/*								getActiveColorsFrom() logic  changed to parse all available color comments			*/
/*				29.01.09	tss	cs4 porting: another way of accessing to ISession, Link-related changes				*/
/*								unused disableWindowMenus() with deprecated IWindowList removed						*/
/*				25.11.09	kli	saveAsPDF(), SetupPdfExportPreferences() - exporting to PDF							*/
/*								updatePicProc changed to ImageLinkUtils::RelinkItem() instead of using SetNameInfo()*/
/*				15.12.09	kli	EnumPictures() changed. Now it uses ImageLinkUtils::GetAllLinksInSpread()			*/
/*				16.12.09	kli	saveAsPDF() changed to load default exporting preset instead of preset from session	*/
/*								setPDFPreferences deleted, applySessionsPreferences() and logAllPresets() added		*/
/*				02.07.10	hal	saveAsPDF() changed, bleeding options are checked now								*/
/*				06.07.10	hal	saveAsPDF()  changed, for PDF export: if at least one user-defined export options	*/
/*								preset exists: the one is applied "as is", else [Press Quality] standard preset	used*/
/*								but with the following chenges - 'Do not Downsampling' value is set for				*/
/*								all image types, and also the 'None' value is set inside the 'Compression' fields	*/
/*				29.11.10	hal	saveAsPDF() changed to load default exporting preset from configuration				*/
/*				27.04.2012	bu	use precompiled header																*/
/*				11.04.14	bu	MAC & CS6																			*/
/*				21.05.14	bu	MAC & CS6: do not try to convert POSIX path on getting pictures						*/
/*				21.05.14	bu	CC2014																				*/
/*				12.02.15	bu	replace one picture only in the concerning box; not all links of it					*/
/*				21.09.15	bu	log temporary eps file path in case of failure during getting color names			*/
/*																													*/
/********************************************************************************************************************/

#include "precompiled.h"

#ifndef WINOS
#include "MacPathUtils.h"
#endif

typedef struct UpdatePicData
{
	TargetApp	target;
	int32		dirID;
} UpdatePicData;

extern CMyStream sStreamOut2;
static TargetApp sTarget;
CMyStream *GetOutStream();

/********************************************************************************************************************/
int getNumberOfPages(IDocument *doc)
/********************************************************************************************************************/
{
	InterfacePtr<IPageList> pages(doc, UseDefaultIID());

	return pages->GetPageCount();
}

/********************************************************************************************************************/
static bool getPicProc(UIDRef pgItem, CMyStream* userData, int32& index)
/********************************************************************************************************************/
{

		IDataBase *db = pgItem.GetDataBase();
		InterfacePtr<ILinkManager> linkMgr(db, db->GetRootUID(), UseDefaultIID());
		
		///ILinkManager::QueryResult result; <--- QueryResult is a vector !!!
		UIDList result;

		linkMgr->QueryLinksByObjectUID(pgItem.GetUID(), result);
		if (result.size() == 0) {
			return true;
		}

		UIDRef linkRef = UIDRef(db, *result.begin());
		InterfacePtr<ILink> theLink(linkRef, UseDefaultIID());
		if (theLink == NULL)
		{
			return true;
		}

		InterfacePtr<ILinkResource> linkResource(db, theLink->GetResource(), UseDefaultIID());
		if (linkResource == NULL)
		{
			return true;
		}

		WideString wideName = linkResource->GetLongName(true);
		PMString pmName(wideName);
		PMString *str = &pmName;

		CStr255 key = "";
		CMyStream::getKey(keyPicture, ++index, key);

		CStr255 path = "";
		str->GetCString(path, sizeof(path));
	
		LogLine("Next picture file: %s", path);
	
#ifdef WINOS
		/* if path contains backslashes, replace them by slashes: */
		if (strchr(path, '\\')) {
			for (int i = 0; i < strlen(path); i++) {
				if (path[i] == '\\')
					path[i] = '/';
			}
		}
#else
	/* POSIX conversion will be done in dtplink for Mac */
	if (false && strchr(path, ':')) {
		for (int i = 0; i < strlen(path); i++) {
			if (path[i] == ':')
				path[i] = '/';
		}
	}
#endif
	
		CMyStream *stream = (CMyStream *)userData; // <--- userData is invalid at this point; any access crashes the PlugIn (InDesign's memory mamagement???)

		///stream->add(key, path);
		GetOutStream()->add(key, path);
		///LogLine("Added picture file: %s", path);

		// In case of CS4 path is already POSIX and we don't need this conversion
		return true;
}

/********************************************************************************************************************/
bool doPageItems(UIDList *list, EnumDocProc proc, CMyStream* userData, int32 &index)
/********************************************************************************************************************/
{
	int16 cnt = (int16) list->Length();

	for(int16 i=0; i<cnt; i++)
	{
		UIDRef pgItem = list->GetRef(i);

		if(proc(pgItem, userData, index) == false)
			return false;
	}

	return true;
}

/********************************************************************************************************************/
void EnumPictures(IDocument *doc, EnumDocProc proc, CMyStream* userData)
/********************************************************************************************************************/
{
	IDataBase *db = GetDataBase(doc);
	int32 index = 0;

	InterfacePtr<ISpreadList> spreads( doc, UseDefaultIID() );

	for (int32 i = 0; i < spreads->GetSpreadCount(); ++i)
	{	
		UIDList uidlist = ImageLinkUtils::GetAllLinksInSpread(UIDRef(db, spreads->GetNthSpreadUID(i)));
		
		doPageItems(&uidlist, proc, userData, index);
	}
}

/********************************************************************************************************************/
int getPictures(int32 refnum, CMyStream *stream)
/********************************************************************************************************************/
{
	IDocument* doc = getDocFromRefnum(refnum);
	if(doc == NULL)
		return -1;

	EnumPictures(doc, getPicProc, stream);

	return 0;
}

/********************************************************************************************************************/
static void setEPSPrefs(int32 pgNum, bool omit, PMReal bleedTop,  PMReal bleedBottom,	PMReal bleedLeft,  
						PMReal bleedRight, int32 numPages=0, bool saveAsBinary = false, bool saveWithFonts = false)
/********************************************************************************************************************/
{
	InterfacePtr<ICommand> prefsCmd(CmdUtils::CreateCommand(kSetEPSExportPrefsCmdBoss));
	CheckCreation("DocResources.cpp::setEPSPrefs()", prefsCmd, "ICommand", 1);
	InterfacePtr<IEPSExportPrefsCmdData> prefsData(prefsCmd, IID_IEPSSEPEXPORTPREFSCMDDATA);
	CheckCreation("DocResources.cpp::setEPSPrefs()", prefsData, "IEPSExportPrefsCmdData", 1);

	prefsData->SetExportEPSPageOption(IEPSExportPreferences::kExportRanges);

	if(saveAsBinary)
		prefsData->SetExportEPSDataFormat(IEPSExportPreferences::kExportBinaryData);
	else
		prefsData->SetExportEPSDataFormat(IEPSExportPreferences::kExportASCIIData);

	if(saveWithFonts)
		prefsData->SetExportEPSIncludeFonts(IEPSExportPreferences::kExportIncludeFontsWhole);
	else
		prefsData->SetExportEPSIncludeFonts(IEPSExportPreferences::kExportIncludeFontsNone);

	PMString pmStr;
	pmStr.AsNumber(pgNum);
	if (numPages)
	{
		pmStr.Append("-");
		pmStr.AppendNumber(numPages);
	}

#ifdef CC
//	LogLine("DocResources.cpp::setEPSPrefs() - range string = %s", pmStr.GetUTF8String().c_str());
#else
//	LogLine("DocResources.cpp::setEPSPrefs() - range string = %s", pmStr.GrabCString());
#endif

	prefsData->SetExportEPSPageRange(pmStr);

	if (omit)
	{
		prefsData->SetExportEPSAllPageMarks(IEPSExportPreferences::kExportAllPageMarksOFF);
		prefsData->SetExportEPSBitmapSampling(IEPSExportPreferences::kExportBMSampleLowRes);
		prefsData->SetExportEPSIncludeFonts(IEPSExportPreferences::kExportIncludeFontsNone);
		prefsData->SetExportEPSOmitBitmapImages(IEPSExportPreferences::kExportOmitBitmapImagesON);
		prefsData->SetExportEPSOmitEPS(IEPSExportPreferences::kExportOmitEPSON);
		prefsData->SetExportEPSOmitPDF(IEPSExportPreferences::kExportOmitPDFON);
		prefsData->SetExportEPSOPIReplace(IEPSExportPreferences::kExportOPIReplaceOFF);
		prefsData->SetExportEPSPreview(IEPSExportPreferences::kExportPreviewNone);
	}
	else
	{
		prefsData->SetExportEPSOmitEPS(IEPSExportPreferences::kExportOmitEPSOFF);
		prefsData->SetExportEPSOmitBitmapImages(IEPSExportPreferences::kExportOmitBitmapImagesOFF);
		prefsData->SetExportEPSOmitPDF(IEPSExportPreferences::kExportOmitPDFOFF);
		prefsData->SetExportEPSPreview(IEPSExportPreferences::kExportPreviewTIFF);
	}

	prefsData->SetExportEPSBleedOnOff( IEPSExportPreferences::kExportBleedON );
	prefsData->SetExportEPSBleedTop( mm2pt(bleedTop/1000.f) );
	prefsData->SetExportEPSBleedBottom( mm2pt(bleedBottom/1000.f) );
	prefsData->SetExportEPSBleedInside( mm2pt(bleedLeft/1000.f) );
	prefsData->SetExportEPSBleedOutside( mm2pt(bleedRight/1000.f) );

	CmdUtils::ProcessCommand(prefsCmd);
}

/********************************************************************************************************************/
int saveAsEPS(IDocument* doc, int32 pgNum, IDFile file, PMReal bleedTop,  PMReal bleedBottom,
			   PMReal bleedLeft,  PMReal bleedRight, bool saveAsBinary, bool saveWithFonts  )
/********************************************************************************************************************/
{
	setEPSPrefs(pgNum, false, bleedTop, bleedBottom, bleedLeft,  bleedRight, 0, saveAsBinary, saveWithFonts);

	InterfacePtr<ICommand> exportCmd(CmdUtils::CreateCommand(kExportEPSCmdBoss));
	InterfacePtr<IExportEPSCmdData> exportData(exportCmd, IID_IEXPORTEPSCMDDATA);


	exportData->Set(file, doc, 0, 0);  							

	return (CmdUtils::ProcessCommand(exportCmd) == kSuccess) ? 0 : -1;
}

/********************************************************************************************************************/
void applyAdditionalPreferences(IPDFExportPrefs* exportPrefs)
/********************************************************************************************************************/
{
	exportPrefs->SetPDFExCompressColorImages( IPDFExportPrefs::kExportCompressImagesNone );
	exportPrefs->SetPDFExCompressGrayImages( IPDFExportPrefs::kExportCompressImagesNone );
	exportPrefs->SetPDFExCompressMonoImages( IPDFExportPrefs::kExportCompressImagesNone );
	exportPrefs->SetPDFExOmitBitmapImages( IPDFExportPrefs::kExportOmitPDFOFF );
	exportPrefs->SetPDFExOmitEPS( IPDFExportPrefs::kExportOmitEPSOFF );
	exportPrefs->SetPDFExOmitPDF( IPDFExportPrefs::kExportOmitPDFOFF );
	//exportPrefs->SetPDFExColorSpace( IPDFExportPrefs::kExportPDFColorSpaceCMYK );
	//exportPrefs->SetExportLayers( IPDFExportPrefs::kExportVisiblePrintableLayers );
	//exportPrefs->SetPDFExSampleColorImages( IPDFExportPrefs::kExportSampleImagesNone );
}

/********************************************************************************************************************/
int saveAsPDF(IDocument *doc, int32 pgNum, IDFile idFile, CStr path, CStr format, PMReal bleedTop,  
			   PMReal bleedBottom, PMReal bleedLeft,  PMReal bleedRight)
/********************************************************************************************************************/
{
	int32 styleIdx = -1;

	if( !doc )  
		return -1; 

	UIDRef docRef = GetUIDRef( doc );
	if( !docRef )  
		return -2; 

	InterfacePtr<IPageList> pages( docRef, UseDefaultIID() );
	if( !pages || pgNum < 0 || pgNum >= pages->GetPageCount() ) 
		return -3; 
	
	InterfacePtr<IGlobalRecompose> recompose( doc, UseDefaultIID() );
	if( !recompose  ) 
		return-4; 

	if( recompose ) 
		recompose->ForceRecompositionToComplete(); // fixes broken text-on-path and text shadows

	InterfacePtr<ICommand> cmd( CmdUtils::CreateCommand( kPDFExportCmdBoss ) );
	if( !cmd  ) 
		return -5; 
	
	InterfacePtr<ISysFileData> sysFileData( cmd, UseDefaultIID() );
	if( !sysFileData ) 
		return -6; 

	sysFileData->Set( idFile );

	InterfacePtr<IBoolData> useProgressBar( cmd, IID_IUSEPROGRESSINDICATOR );
	if( !useProgressBar ) 
		return -7; 

	useProgressBar->Set( kFalse );

	// the command parameter: take off an export dialog
	InterfacePtr<IUIFlagData> uiFlagData( cmd, IID_IUIFLAGDATA );
	if( !uiFlagData ) 
		return -8; 

	uiFlagData->Set( kSuppressUI );

	// the command parameter: list of pages to be exported
	InterfacePtr<IOutputPages> outputPages( cmd, UseDefaultIID() );
	if( !outputPages ) 
		return -9; 

	outputPages->InitializeFrom( UIDList( docRef.GetDataBase(), pages->GetNthPageUID( pgNum ) ), kFalse );

	// export preferences
	InterfacePtr<IPDFExportPrefs> exportPrefs( cmd, IID_IPDFEXPORTPREFS );
	if(  !exportPrefs ) 
		return -10; 

	InterfacePtr<IPDFExptStyleListMgr>
	styleMgr((IPDFExptStyleListMgr*)::QuerySessionPreferences(IID_IPDFEXPORTSTYLELISTMGR));
	if( !styleMgr ) 
		return -11; 
     
	// get the name of PDF export preset from configuration 
	PluginConfiguration config = XMLConfigurationReader::getInstance()->GetPluginConfiguration();
	std::string stPresetName = config.getPdfPresetName();

	/* if no PDF presetting is give, use default <alfaPresets>: */
	if (strlen(stPresetName.c_str()) == 0)
		stPresetName = "alfaPresets";

	bool isUserDefFound = true;
	
	if (strlen(stPresetName.c_str()) > 0) {
		styleIdx = styleMgr->GetStyleIndexByName( stPresetName.c_str() );
		LogLine("configured style ID = %d", styleIdx);
	}

	if( styleIdx == -1 )
		styleIdx = styleMgr->GetStyleIndexByName("[Qualitativ hochwertiger Druck]");
	
	if( styleIdx == -1 )
	{
		// user-defined export options preset wasn't found, standard [Press Quality] preset will be used
		isUserDefFound = false;
		styleIdx = 4;	// index of "[Press Quality]" preset
	}

	// get a preset by index
	UIDRef styleRef = styleMgr->GetNthStyleRef(styleIdx);
	InterfacePtr<IPDFExportPrefs> pStylePrefs(styleRef, UseDefaultIID());
	if( !pStylePrefs ) 
		return -12; 
#ifdef CC
	LogLine("DocResources.cpp::saveAsPDF() - used preset name: %s, format: %s", pStylePrefs->GetUIName().GrabCString().c_str(), format);
#else
	LogLine("DocResources.cpp::saveAsPDF() - used preset name: %s, format: %s", pStylePrefs->GetUIName().GrabCString(), format);
#endif

	// copy options ...
	exportPrefs->CopyPrefs(pStylePrefs);

	// ... change some options, if not user-defined ...
	if( !isUserDefFound )
		applyAdditionalPreferences( exportPrefs ); 
	
	// use a bleeding when export
	exportPrefs->SetUseDocumentBleed( kTrue );
	exportPrefs->SetPDFExBleedTop ( mm2pt(bleedTop/1000.f) );
	exportPrefs->SetPDFExBleedBottom ( mm2pt(bleedBottom/1000.f) );
 	exportPrefs->SetPDFExBleedInside( mm2pt(bleedLeft/1000.f) );
 	exportPrefs->SetPDFExBleedOutside( mm2pt(bleedRight/1000.f) ); 			
	
	if (config.isPdfVersion14())
		exportPrefs->SetPDFExStandardsCompliance(IPDFExportPrefs::kPDFVersion14); 


	/* bu 27.02.12 overwrite PDF version to X3: */
	if (strstr(format, "X3")) {
		LogLine("set PDF version to X3");
		exportPrefs->SetPDFExStandardsCompliance(IPDFExportPrefs::kExportPDFX32003);
	}

	return (CmdUtils::ProcessCommand( cmd ) == kSuccess ) ? 0 : -13; 
}

/********************************************************************************************************************/
int getColors(int32 refnum, CMyStream *streamIn, CMyStream *streamOut)
/********************************************************************************************************************/
{
	///LogLine("getColors() / streamOut = %d", streamOut);
	int colOption = streamIn->get(keyColOption, static_cast<int32>(0));

	return getActiveColorNames(refnum,  streamOut,  colOption);
}

/********************************************************************************************************************/
void makeColumnsList(int numCols, PMReal dist, PMReal columnWidth, PMRealList& pageCols)
/********************************************************************************************************************/
{
	PMReal	columnStart=0;
	pageCols.clear();
	pageCols.push_back(columnStart);

	for (int32 i=0; i<numCols; i++)
	{
		columnStart += columnWidth;
		pageCols.push_back(columnStart);
		if ( i != numCols-1 )
		{
			columnStart += dist;
			pageCols.push_back(columnStart);
		}
	}
}

/********************************************************************************************************************/
int CreateSpreads(const UIDRef docUIDRef, const int32 numberOfSpreads)
/********************************************************************************************************************/
{
	InterfacePtr<ICommand> newSpreadCmd(CmdUtils::CreateCommand(kNewSpreadCmdBoss));
	if (CheckCreation("DocResources.cpp::CreateSpreads()", newSpreadCmd, "newSpreadCmd"))
		return -1;

	InterfacePtr<INewSpreadCmdData> newSpreadCmdData(newSpreadCmd,UseDefaultIID());
	if (CheckCreation("DocResources.cpp::CreateSpreads()", newSpreadCmdData, "newSpreadCmdData"))
		return -1;

	// we place the new spread at the end of the document
#ifdef CS6
	newSpreadCmdData->SetNewSpreadCmdData(docUIDRef, 1, ISpreadList::kAtTheEnd, 
		INewSpreadCmdData::kPreventPagesFromShufflingThroughNewSpread, 1, PMPageSize(0, 0), 0);
#else 
///#ifdef CS5
	K2Vector<PMRect> pageBoundingBoxes;
	pageBoundingBoxes.push_back( PMRect(0, 0, 0, 0) ); 
    int32 pagesPerNewSpread = 1;
	newSpreadCmdData->SetNewSpreadCmdData(docUIDRef, 1, ISpreadList::kAtTheEnd, pagesPerNewSpread,&pageBoundingBoxes, nil );
///#else
///	newSpreadCmdData->Set(docUIDRef, 1, ISpreadList::kAtTheEnd, PMRect(0, 0, 0, 0), 1);
///#endif
#endif

	InterfacePtr<IIntData> bindingLocation(newSpreadCmd, IID_IBINDINGLOCATION);
	if (CheckCreation("DocResources.cpp::CreateSpreads()", bindingLocation, "bindingLocation"))
		return -1;

	// choosing anything but default binding can lead to undesirable results.
	bindingLocation->Set(ISpread::kDefaultBinding);

	// process the command
	if (CmdUtils::ProcessCommand(newSpreadCmd) != kSuccess)
		return -1;

	return 0;
}

/********************************************************************************************************************/
int getActiveColorNames(int32 refnum,  CMyStream *streamOut,  int32 colOption)
/********************************************************************************************************************/
{
	PMString tempFileName;

	IDocument* doc = getDocFromRefnum(refnum);
	if(doc == NULL)
	{
		LogLine("DocResources.cpp::getActiveColorNames() - doc is NULL, will return -1...");
		return -1;
	}

	//  create a temporary PostScript file
	//LogLine("calling saveDocIntoTempPSFile()...");

	if( !saveDocIntoTempPSFile(doc, tempFileName ) )
	{
		LogLine("DocResources.cpp::getActiveColorNames() - temporary PostScipt file wasn't created");
		return -1;
	}

	//LogLine("back from saveDocIntoTempPSFile()...");

	// get an information about colors from file
	std::set<std::string> spotColors;
	std::set<std::string> processColors;

	//LogLine("calling getActiveColorsFrom()...");

	getActiveColorsFrom(tempFileName, spotColors, processColors);

	//LogLine("back from getActiveColorsFrom()...");

	// push the got information into the stream
	int32 index=1;

	if ( colOption != 0  )
	{
		// i.e. not only spot colors
		for (std::set<std::string>::iterator it = processColors.begin(); it!=processColors.end(); ++it)
		{
			LogLine("Next process color: %s", const_cast<char*>(it->c_str()));

			streamOut->add(keyColName, const_cast<char*>(it->c_str()), index++);
		}
	}

	for (std::set<std::string>::iterator it = spotColors.begin(); it!=spotColors.end(); ++it)
	{
		LogLine("Next spot color: %s", const_cast<char*>(it->c_str()));

		streamOut->add(keyColName, const_cast<char*>(it->c_str()), index++);
	}

	return 0;
}


// getActiveColorsFrom() -
//   Get information about the colors from the temp file (PostScript).
// The needed information are stored in some last lines in
// the end of the file :
// ....
// %%DocumentProcessColors:  Cyan Magenta Yellow Black
// %%DocumentCustomColors: (Gold)
// %%+ (Silver)
// %%+ ...
// ....

/********************************************************************************************************************/
int getActiveColorsFrom(PMString tempFileName, std::set<std::string> &spotColors, std::set<std::string> &processColors )
/********************************************************************************************************************/
{
	spotColors.clear();
	processColors.clear();

#ifdef CC
	FILE *f = fopen(tempFileName.GrabCString().c_str(), "rb");
#else
	FILE *f = fopen(tempFileName.GrabCString(), "rb");
#endif

	if (!f)
	{
		LogLine("DocResources.cpp::getActiveColorsFrom() - temporary PostScipt file wasn't opened");
		return -1;
	}

	fseek (f, 0L, SEEK_END);
	unsigned int fileSize = ftell (f);
	unsigned int blockSize = TF_BLOCK_SIZE;
	unsigned int blockOffset = ( blockSize > fileSize ) ? 0 : fileSize - blockSize;

	fseek(f, blockOffset , SEEK_SET);
	char *buf = (char*) new char[blockSize + 1];
	unsigned int nRead = (int) fread(buf, 1, blockSize, f);
	buf[nRead]=0;

	//---------------------------------------------
	//  GET INFO from the buffer
	// ....
	// %%DocumentProcessColors:  Cyan Magenta Yellow Black
	// %%DocumentCustomColors: (Gold)
	//-------------------------------------------
	std::list<int> keyOffsets;
	// find the "%%" and save these offsets
	// ignore the "%%+" - it is addition to prev. pattern
	for ( int i=0; i<nRead; i++ )
	{		
		if ( buf[i] == '%' &&  buf[i+1] == '%' && buf[i+2] != '+' )
		{
			keyOffsets.push_back(i);
			buf[i] = 0;
		}
	}

	// loop over all keyOffsets
	for (std::list<int>::iterator it = keyOffsets.begin(); it!=keyOffsets.end(); ++it)
	{
		int nOffset = *it + 2;
		char* pOffset = buf + nOffset;
		char *p1 = NULL;
		// The Process Colors
		if ( strstr(pOffset, "DocumentProcessColors:") != NULL )
		{
			//LogLine("DocResources.cpp::getActiveColorsFrom() - DocumentProcessColors block was found ");

			if ( strstr(pOffset, "Cyan") != NULL)
			{
				processColors.insert("Cyan");
			}
			if ( strstr(pOffset, "Magenta") != NULL)
			{
				processColors.insert("Magenta");
			}
			if ( strstr(pOffset, "Yellow") != NULL)
			{
				processColors.insert("Yellow");
			}
			if ( strstr(pOffset, "Black") != NULL)
			{
				processColors.insert("Black");
			}
		}
		// find the paragraph contains list of used the Spot Colors
		else if( (p1 = strstr( pOffset, "DocumentCustomColors:")) != NULL )
		{
			// ex.:
			//  %%DocumentCustomColors: (Gold) 
			//  %%+  (Silver) ...
			//
			//LogLine("DocResources.cpp::getActiveColorsFrom() - DocumentCustomColors block was found ");

			p1 += strlen("DocumentCustomColors:");
			char *p2 = NULL;
			while ( (p2 = strchr(p1, '(' )) != NULL )
			{
				p2++;
				if( (p1 = strchr(p2, ')' )) != NULL )
				{
					*p1++ = 0;

					LogLine("DocResources.cpp::getActiveColorsFrom() - spot color found = %s", p2);
					if (strstr(p2, "atend") == NULL)
					{
						std::string nm(p2);
						spotColors.insert(nm);
					}
				}
				else
				{
					break;
				}
			}
		}
	}

	fclose(f);
	delete [] buf;

#ifdef CC
	unlink(tempFileName.GrabCString().c_str());
#else
	unlink(tempFileName.GrabCString());

#endif /* CC */

	return 0;
}

/********************************************************************************************************************/
bool saveDocIntoTempPSFile(IDocument *doc, PMString& tempFileName )
/********************************************************************************************************************/
{
	// get the name for temp file ( in system temp folder )
#ifndef WINOS
	char *posixTempFilePath = tempnam(0, "alfaind");
	//LogLine("DocResources.cpp::saveDocIntoTempPSFile() - temporary file POSIX path = %s", posixTempFilePath);

	char hfsTempFilePath[MAX_PATH];
	CFStringRef cfPathRef = CFStringCreateWithCString(kCFAllocatorDefault, posixTempFilePath, kCFStringEncodingASCII);
	CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, cfPathRef, kCFURLPOSIXPathStyle, true);
	CFStringRef hfsPath = CFURLCopyFileSystemPath(url, kCFURLHFSPathStyle);
	CFStringGetCString(hfsPath, hfsTempFilePath, MAX_PATH, kCFStringEncodingASCII);
	CFRelease(url);
	CFRelease(hfsPath);
	LogLine("DocResources.cpp::saveDocIntoTempPSFile() - temporary file HFS path = %s", hfsTempFilePath);

	tempFileName = hfsTempFilePath;
#else
	/* on CITRIX, the temp file stuff makes problems. */
	// char tempPath[2048], tempName[2048];
    // GetTempFileNameA(tempPath, "indplug", 0, tempName);
	// tempFileName =  tempName;

    // bu 23.05.16 changed: */
	AlfaDocManager* docMng = AlfaDocManager::getInstance();
	IDFile  parentFolderID, tmpFileID = docMng->getInitialIDFile(::GetUIDRef(doc));
	PMString destFilePath = tmpFileID.GetString() + ".epsTmp";
	tempFileName = destFilePath;

	/* bu 29.06.16: On Win7, the temp file cannot be deleted and will be checked-in into motif container: */
	FileUtils::GetParentDirectory(tmpFileID, parentFolderID);
	FileUtils::GetParentDirectory(parentFolderID, parentFolderID);

	destFilePath = parentFolderID.GetString() + "\\clrtmp.eps";
	tempFileName = destFilePath;
	char buf[256];
#if CC
		strcpy(buf, tempFileName.GrabCString().c_str());
#else
	strcpy(buf, tempFileName.GrabCString());
#endif
	LogLine("Temp. EPS file: %s", buf);


#endif

#if THIS_WAS_BEFORE_MAY_24_2016
	// get the document's print data
	LogLine("Getting print data...");

	InterfacePtr<IPrintData> docPrintData(doc->GetDocWorkSpace(), UseDefaultIID());
	if (docPrintData == nil)
	{
		ASSERT(docPrintData);
		return false;
	}

	LogLine("Generating print command action...");
	InterfacePtr<ICommand> printActionCmd(CmdUtils::CreateCommand(kPrintActionCmdBoss));
	if (printActionCmd == nil)
	{
		LogLine("DocResources.cpp::saveDocIntoTempPSFile() - CreateCommand(kPrintActionCmdBoss) failed");
		return false;
	}

	LogLine("Generating print command data...");
	InterfacePtr<IPrintCmdData> printActionCmdData(printActionCmd, UseDefaultIID());
	if (printActionCmdData == nil)
	{
		LogLine("DocResources.cpp::saveDocIntoTempPSFile() - printActionCmdData is nil ");
		return false;
	}

	//-----------------------------------------
	//  set the default print style
	//----------------------------------------

	LogLine("Getting old print settings...");
	InterfacePtr<IWorkspace> workspace(UnifiedAux::GetSession()->QueryWorkspace());
	InterfacePtr<IPrStStyleListMgr> printStyleListMgr(workspace, UseDefaultIID());
	int32 index = printStyleListMgr->GetStyleIndexByName("kPrSt_DefaultName");
	UIDRef localPrintStyle = printStyleListMgr->GetNthStyleRef(index);
	InterfacePtr<IPrintData> printData(localPrintStyle, UseDefaultIID());

	//----------------------------------------------
	//  get a range of pages
	//
	//----------------------------------------------
	PMString pageRangeString;
	int32 whichPages = printData->GetWhichPages();
	//
	// make a range string that contains all pages in doc
	InterfacePtr<IPageList> pageList(doc, UseDefaultIID());
	ASSERT(pageList);
	int32 numPages = pageList->GetPageCount();
	UIDList pagesUIDList(::GetDataBase(doc));
	for (int32 i = 0 ; i < numPages ; i++)
	{
		pagesUIDList.Append(pageList->GetNthPageUID(i));
	}
	pageList->GetPageRangeString(pagesUIDList, &pageRangeString);

	//save old settings
	LogLine("Saving old print settings...");

	PMString oldStyleName    = docPrintData->GetStyleName();
	//
	int32    oldPrintTo      = printData->GetPrintTo();
	PMString oldPrinterName  = printData->GetPrinter();
	PMString oldFileName     = printData->GetFileName();
	PMString oldPPDName      = printData->GetPPDName();
	int32    oldWhichPages   = printData->GetWhichPages();
	PMString oldPageRange    = printData->GetPageRange();
	//
	//where we are printing to:  kVirtualPrinter (PostScript file with PPD selected), 
	// kPrepressFile (PostScript file Device Independent), kEPSFile (EPS file)

	LogLine("Setting new print settings...");
	printData->SetPrintTo(IPrintData::kPrepressFile);
	printData->SetPrinter(PMString("kPrepress File"));
	printData->SetPPDName(PMString("kDevice Independent")); 
	printData->SetFileName(tempFileName);
	printData->SetWhichPages(whichPages);
	printData->SetPageRange(pageRangeString);

	//--------------------------------------------
	//  Execute the print action cmd
	//--------------------------------------------
	//
	// settings for printing command
	//

	printActionCmdData->SetIDoc(doc);
	// suppress all dialogs
	printActionCmdData->SetPrintUIOptions(IPrintCmdData::kSuppressEverything);
	printActionCmdData->SetPrtStyleUIDRef(localPrintStyle);

	// PRINTING
	LogLine("Print command...");

	ErrorCode result = CmdUtils::ExecuteCommand(printActionCmd);
	LogLine("DocResources.cpp::saveDocIntoTempPSFile() - printActionCmd result = %d", result);
#else
	/* 24.05.2016 bu: try the standard saveAs EPS function: */
	{
		int err;
		IDFile theTmpFile(tempFileName);

		err = saveAsEPS(doc, 1, theTmpFile, 0, 0, 0, 0, false, false);
		if (err)
			LogLine("saveAsEPS() returns %d", err);
	}
#endif /* THIS_WAS_BEFORE_MAY_24_2016 */


#ifndef WINOS
	// HFS path is only needed for pass to SDK functions. For the next file reading using fopen() we need to have POSIX path.
	tempFileName = posixTempFilePath;
	// we are responsible for freeing memory allocated by tempnam call
	free(posixTempFilePath);
#endif

	bool res = true;
	// 18.05.2008 - tss. Just to ensure that file has been exported successfully, we will get and log its filesize
#ifdef CC
	FILE* f = fopen(tempFileName.GrabCString().c_str(), "rb");
#else
	FILE* f = fopen(tempFileName.GrabCString(), "rb");
#endif /* CC */

	if (f != NULL)
	{
		if (fseek(f, 0L, SEEK_END) != 0)
		{
			LogLine("ERROR IN DocResources.cpp::saveDocIntoTempPSFile() - can't perform seek up to the end of exported PS file");
			res = false;
		}

		fclose(f);
	}
	else
	{
#ifdef CC
		LogLine("ERROR IN DocResources.cpp::saveDocIntoTempPSFile() - exported PS file <%s> doesn't exist", tempFileName.GrabCString().c_str());
#else
		LogLine("ERROR IN DocResources.cpp::saveDocIntoTempPSFile() - exported PS file <%s> doesn't exist", tempFileName.GrabCString());
#endif /* CC */

	}
	res = false;

#if THIS_WAS_BEFORE_MAY_24_2016
	//restore old settings
	LogLine("Restoring old EPS settings...");

	docPrintData->SetStyleName(oldStyleName);
	printData->SetPrintTo(oldPrintTo);
	printData->SetPrinter(oldPrinterName);
	printData->SetPPDName(oldPPDName); 
	printData->SetFileName(oldFileName);
	printData->SetWhichPages(oldWhichPages);
	printData->SetPageRange(oldPageRange);

	if (result != kSuccess || !res)
	{
		LogLine("DocResources.cpp::saveDocIntoTempPSFile() - ExecuteCommand(printActionCmd) failed");
		return false;
	}
#endif /* THIS_WAS_BEFORE_MAY_24_2016 */

	return true;
}
