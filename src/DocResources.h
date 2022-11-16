/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		DocResources.h																						*/
/*																													*/
/*	Created:																										*/
/*																													*/
/*	Modified:																										*/
/*																													*/
/********************************************************************************************************************/

#include <SDKFileHelper.h>
#include <IDocument.h>
#include <IColumns.h>
#include "CMyStream.h"
#include <set>

#define TF_BLOCK_SIZE  40000
typedef bool (*EnumDocProc)(UIDRef pgItem, CMyStream* userData, int32& index);

int saveAsEPS(IDocument* doc, int32 pgNum, IDFile file, PMReal bleedTop,  PMReal bleedBottom, PMReal bleedLeft,  PMReal bleedRight, bool saveAsBinary, bool saveWithFonts);
int saveAsPDF(IDocument *doc, int32 pgNum, IDFile idFile, CStr path, CStr format, PMReal bleedTop,  PMReal bleedBottom, PMReal bleedLeft,  PMReal bleedRight);
int getColors(int32 refnum, CMyStream *streamIn, CMyStream *streamOut);
int getPictures(int32 refnum, CMyStream *stream);
int getActiveColorNames(int32 refnum,  CMyStream *streamOut, int32 colOption);
int getActiveColorsFrom( PMString tempFileName, std::set<std::string> &spotColors, std::set<std::string> &processColors );
int getNumberOfPages(IDocument *doc);
bool saveDocIntoTempPSFile(IDocument *doc, PMString& tempFileName );
void EnumPictures(IDocument *doc, EnumDocProc proc, CMyStream* userData);
void makeColumnsList(int numCols, PMReal dist, PMReal columnWidth, PMRealList& pageCols);
int CreateSpreads(const UIDRef docUIDRef, const int32 numberOfSpreads);
