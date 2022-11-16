/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		FrameUtils.h																					*/
/*																													*/
/*	Created:																										*/
/*																													*/
/*	Modified:																										*/
/*																													*/
/********************************************************************************************************************/

#ifndef _FRAME_UTILS
#define _FRAME_UTILS

class FrameUtils
{
public:
	static UIDRef GetSplineFrame( const UIDRef& item );
	static bool GetChildFrames( const UIDRef& parent, UIDList& childsList );
	static PBPMRect GetBoundBoxOnPasteboard( const UIDRef& Item );
	static bool GetFrameByCoordinate( const UIDRef& parentSpreadLayer, UIDRef& frame, PMPoint pt );
	static bool Resize( const UIDRef& frameRef, PMReal resizeX, PMReal resizeY );
	static bool MoveRelative( const UIDRef& frameRef, PMReal relativeX, PMReal relativeY );
	static bool Rotate( const UIDRef& frameRef, PMReal angle );
};
#endif
