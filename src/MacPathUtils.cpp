/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		MacPathUtilss.cpp																					*/
/*																													*/
/*	Created:	21.05.08	tss																						*/
/*																													*/
/*	Modified:																										*/
/*																													*/
/********************************************************************************************************************/

#ifndef WINOS

#include "MacPathUtils.h"
#include "PluginLogger.h"
	
	
/********************************************************************************************************************/
void convertPathStyle(char *path, int len, CFURLPathStyle inPathStyle, CFURLPathStyle outPathStyle)
/********************************************************************************************************************/
{	
	CFStringRef cfPathRef = CFStringCreateWithCString(kCFAllocatorDefault, path, kCFStringEncodingASCII);
	CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, cfPathRef, inPathStyle, true);
	CFStringRef newPath = CFURLCopyFileSystemPath(url, outPathStyle);
	CFStringGetCString(newPath, path, len, kCFStringEncodingASCII);
	CFRelease(url);
	CFRelease(newPath);
}
	
/********************************************************************************************************************/
void replaceKey(CMyStream *stream, CStr key, CStr newValue)
/********************************************************************************************************************/
{
	char *streamData = stream->getData();
	int32 streamDataLen = stream->getDataLen();
		
	CStr255 oldValue;
	stream->get(key, oldValue);
	
	char *dataWithoutKey = new char[streamDataLen];
		
	int lenStart = stream->findstr(streamData, key) - 1 - streamData;
	int lenOldTag = 2 * strlen(key) + 5 + strlen(oldValue);		// 5 - length of brackets
	int lenEnd = streamDataLen - lenStart - lenOldTag;
	
	if (lenStart > 0)
		memcpy(dataWithoutKey, streamData, lenStart);
	
	if (lenEnd > 0)
		memcpy(dataWithoutKey + lenStart, streamData + lenStart + lenOldTag, lenEnd);
			
	dataWithoutKey[lenStart+lenEnd] = 0;
	stream->setup(dataWithoutKey, lenStart + lenEnd);
		
	delete[] dataWithoutKey;
		
	stream->add(key, newValue);	
}
	
/********************************************************************************************************************/
void convertPathInStream(CMyStream *stream, CStr key, CFURLPathStyle inPathStyle, CFURLPathStyle outPathStyle)
/********************************************************************************************************************/
{
	if (stream->findstr(stream->getData(), key) == NULL)
	{
		LogLine("MacPathUtils::convertPathInStream() - key %s doesn't exist in stream", key);
		return;
	}
		
	char value[MAX_PATH];
	stream->get(key, value, MAX_PATH);
		
	convertPathStyle(value, MAX_PATH, inPathStyle, outPathStyle);
		
	replaceKey(stream, key, value);
}
#endif

