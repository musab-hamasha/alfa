/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		DocMgr.h																							*/
/*																													*/
/*	Created:																										*/
/*																													*/
/*	Modified:																										*/
/*																													*/
/********************************************************************************************************************/

#ifndef _DOCMGR_H
#define _DOCMGR_H

#include "VCPlugInHeaders.h"
#include <IDocument.h>
#include <IObserver.h>
#include <IGeometry.h>
#include <ITransform.h>
#include "idtypes.h"
#include "CMyStream.h"


typedef struct DocDesc
{
	friend class AlfaDocManager;

	int32				refnum;
	CStr255 			path;
	TargetApp			target;
	IDFile				initialIDFile;

protected:
	IDocument  *doc;

public:
	IDocument*  getDoc();

} DocDesc;


class AlfaDocManager
{

private:
	static AlfaDocManager instance;
	std::list<DocDesc*> dataObjects;

public:
	static AlfaDocManager* getInstance()

	{
		return &instance;
	}

	~AlfaDocManager();

	UIDRef createDocument(PMReal width, PMReal height, PMReal left, PMReal top, PMReal right, 
							PMReal bottom, int colNum, PMReal gridDist,
							PMReal bleedTop, PMReal bleedBottom, PMReal bleedLeft, PMReal bleedRight, int numPages);

	DocDesc* attachDataToDocument( UIDRef docRef, int32 docID, TargetApp target, CStr path, int pgNum, int fixed );
	int	        getNumberOfAlfaDocs();
	DocDesc*    getDataForAlfaDoc(IDocument *doc);
	DocDesc*    getDataForAlfaDocByID(int32 refnum);
	int			getIDForAlfaDoc(IDocument *doc);
	IDocument*  getAlfaDocByID(int32 refnum);
	bool        getAllAlfaDocs( std::list<IDocument*>& docs );
	int			closeAlfaDoc(int32 refnum);
	IDFile		getInitialIDFile( const UIDRef& docRef );
	void		setInitialIDFile( const UIDRef& docRef, const IDFile& idFil );

protected:
	AlfaDocManager( ){}
};

DocDesc		*getDoc(IDocument *doc);
IDocument	*getDocFromRefnum(int32 refnum);
int			getRefnum(IDocument *doc);
TargetApp	getClient(int32);

#endif
