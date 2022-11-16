/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		FrameUtils.cpp																					*/
/*																													*/
/*	Created:	            																						*/
/*																													*/
/*	Modified:	                                                                                                   			*/
/*																													*/
/********************************************************************************************************************/

#include "precompiled.h"
#include <IResizeItemsCmdData.h>
#include <ITransformCmdData.h>
#include <ITransformFacade.h>

/********************************************************************************************************************/
UIDRef FrameUtils::GetSplineFrame( const UIDRef& item )
/********************************************************************************************************************/
{
	if ( !item )
		return UIDRef::gNull;

	IDataBase* pDB = item.GetDataBase();
	if ( !pDB )
		return UIDRef::gNull;

	InterfacePtr<IHierarchy> node( item, UseDefaultIID() );
	while ( node )
	{
		UID uid = ::GetUID( node );
		ClassID cls = pDB->GetClass( uid );
		if ( cls == kSplineItemBoss )
			return UIDRef( pDB, uid );

		InterfacePtr<IHierarchy> parent( node->QueryParent() );
		node = parent;
	}
	return UIDRef::gNull;
}

/********************************************************************************************************************/
bool FrameUtils::GetChildFrames( const UIDRef& parent, UIDList& childsList )
/********************************************************************************************************************/
{
	if ( !parent )
		return false;

	IDataBase* db = parent.GetDataBase();
	if ( !db )
		return false;

	InterfacePtr<IHierarchy> hier( parent, UseDefaultIID() );
	if ( !hier )
		return false;

	UIDList localChilds( db );
	hier->GetDescendents( &localChilds, IID_IGRAPHICFRAMEDATA );
	for ( int32 i = 0; i < localChilds.size(); i++ )
	{
		UIDRef  uidfr = localChilds.GetRef( i );
		if ( uidfr == parent )
			continue;

		childsList.Append( uidfr.GetUID() );
		// recursive call to get own childs
		GetChildFrames( uidfr, childsList );
	}
	return true;
}

/********************************************************************************************************************/
PBPMRect FrameUtils::GetBoundBoxOnPasteboard( const UIDRef& Item )
/********************************************************************************************************************/
{
	if( !Item ) 
		return PBPMRect();

	PBPMRect rcItem( Utils<Facade::IGeometryFacade>()->GetItemBounds( Item,
		Transform::PasteboardCoordinates(), Geometry::OuterStrokeBounds() ) );

	return rcItem;
}

/********************************************************************************************************************/
bool FrameUtils::GetFrameByCoordinate(  const UIDRef& parentSpreadLayer, UIDRef& frame, PMPoint pt)
/********************************************************************************************************************/
{
	//----------------------------
	//search frame which in the foreground 
	//----------------------------
	
	if( !parentSpreadLayer ) 
		return false;
	
	IDataBase* db = parentSpreadLayer.GetDataBase();
	if( !db ) 
		return false;

	UIDList childs( db );
	if( !GetChildFrames( parentSpreadLayer, childs ) ) 
		return false;

	UIDList items ( db );
	for( int i = 0; i< childs.size(); i++ )
	{
		PBPMRect rcItem = ( Utils<Facade::IGeometryFacade>()->GetItemBounds( childs.GetRef( i ) ,

			Transform::SpreadCoordinates(), 

			Geometry::OuterStrokeBounds() ) );

		if ( !rcItem.IsEmpty() )
			if ( rcItem.PointIn( pt ) )
			{
				items.Append(childs.GetRef(i ));
			}
	}

	if (items.size() > 0 )
	{
		//----------------------------
		//search frame which in the foreground 
		//----------------------------
		do 
		{
			InterfacePtr<IHierarchy> hierPerent(  parentSpreadLayer, UseDefaultIID());
			if (!hierPerent)
			{
				break;
			}
			
			InterfacePtr<IHierarchy> child(  items.GetRef(0), UseDefaultIID());
			if ( !child )
			{
				break;
			}

			int32 maxIndex = hierPerent->GetChildIndex( child );
			int32 index ;
			int n = 0;

			for(int i = 1; i< items.size(); i++)
			{	
				InterfacePtr<IHierarchy> child2(  items.GetRef(i), UseDefaultIID());
				if ( !child2 )
				{
					continue;
				}

				index = hierPerent->GetChildIndex( child2 );
				if (maxIndex < index)
					n = i;
			}
			
			frame = items.GetRef(n);
			return true;
		} while (false);
		
		frame = items.GetRef(0);
		return true;
	}
		
	return false;
}

/********************************************************************************************************************/
bool FrameUtils::Resize( const UIDRef& frameRef, PMReal resizeX, PMReal resizeY )
/********************************************************************************************************************/
{
	InterfacePtr<ICommand> resizeCmd( CmdUtils::CreateCommand( kResizeItemsCmdBoss ) );
	if ( !resizeCmd )
		return false;

	InterfacePtr<IResizeItemsCmdData> resizeData( resizeCmd, UseDefaultIID() );
	if ( !resizeData )
		return kFailure;

	resizeCmd->SetItemList( UIDList( frameRef ) );
	resizeData->SetResizeData(
		Transform::PasteboardCoordinates(),
		Geometry::PathBounds(),
#ifdef CS6
		Transform::LeftTopLocation( Geometry::OuterStrokeBounds() ),
#else
		Transform::LeftTopLocation(),  // Transform::MiddleTopLocation()
#endif
		Geometry::ResizeTo( resizeX, resizeY ) );

	return ( CmdUtils::ProcessCommand( resizeCmd ) == kSuccess ) ? true: false;
}

/********************************************************************************************************************/
bool FrameUtils::MoveRelative( const UIDRef& frameRef, PMReal relativeX, PMReal relativeY )
/********************************************************************************************************************/
{
	InterfacePtr<ICommand> transformCmd( CmdUtils::CreateCommand( kTransformPageItemsCmdBoss ) );
	if ( !transformCmd )
		return false;

	InterfacePtr<ITransformCmdData> transformData( transformCmd, UseDefaultIID() );
	if ( !transformData )
		return false;

	transformCmd->SetItemList( UIDList( frameRef ) );
	transformData->SetTransformData( Transform::PasteboardCoordinates(),
#ifdef CS6
		Transform::LeftTopLocation( Geometry::OuterStrokeBounds() ),
#else
		Transform::LeftTopLocation(),       //  Transform::MiddleTopLocation(),
#endif
		Transform::TranslateBy( relativeX, relativeY ) );

	return ( CmdUtils::ProcessCommand( transformCmd ) == kSuccess ) ? true: false;
}

/********************************************************************************************************************/
bool FrameUtils::Rotate( const UIDRef& frameRef, PMReal angle )
/********************************************************************************************************************/
{
	ErrorCode result = Utils<Facade::ITransformFacade>()->TransformItems( UIDList( frameRef ),
		Transform::PasteboardCoordinates(),
#ifdef CS6
		Transform::CenterLocation( Geometry::OuterStrokeBounds() ),
#else
		Transform::CenterLocation(),
#endif
		Transform::RotateBy( angle ) );
	return result == kSuccess;
}
