/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		ErrorCheckingLogging.h																				*/
/*																													*/
/*	Created:																										*/
/*																													*/
/*	Modified:																										*/
/*																													*/
/********************************************************************************************************************/

#ifndef _ERRORCHECKINGLogLineGING_H
#define _ERRORCHECKINGLogLineGING_H

int CheckError(char *str, int log_success = 1);
int CheckCreation(char *func, void *object, char *str, int log_success = 1);
int CheckUIDRef(char *func, UIDRef &ref, char *str, int log_success = 1);
int CheckErrorCode(char *func, ErrorCode err, char *str, int log_success = 1);

#endif