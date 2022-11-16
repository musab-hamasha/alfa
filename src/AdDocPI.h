/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		AdDocPI.h																							*/
/*																													*/
/*	Created:																										*/
/*																													*/
/*	Modified:																										*/
/*																													*/
/********************************************************************************************************************/

#ifndef _ADDOCPI_H
#define _ADDOCPI_H

void doUserClose(UIDRef docRef);
void doUserSave(UIDRef docRef);
int closeDocAux(IDocument *doc);
int getMargins(IDocument *doc, int pgNum, PMRect *rect);
#endif
