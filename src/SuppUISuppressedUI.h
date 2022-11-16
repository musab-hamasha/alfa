/********************************************************************************************************************/
/*																													*/
/*	Project:	InDesign Plug-in																					*/
/*																													*/
/*	Module:		SuppUISuppressedUI.h																				*/
/*																													*/
/*	Created:																										*/
/*																													*/
/*	Modified:																										*/
/*																													*/
/********************************************************************************************************************/

#include "VCPluginHeaders.h"
#include "ISuppressedUI.h"


/********************************************************************************************************************/
class SuppUISuppressedUI : public CPMUnknown<ISuppressedUI>
/********************************************************************************************************************/
{
	public:
		SuppUISuppressedUI(IPMUnknown *boss):CPMUnknown<ISuppressedUI>(boss) {};
		~SuppUISuppressedUI() {};
		virtual bool16 IsWidgetDisabled( const IControlView* widget ) const;
		virtual bool16 IsWidgetHidden( const IControlView* widget ) const {return kFalse;}
		virtual bool16 IsDragDropDisabled( const IDragDropTarget* target, DataObjectIterator* data,
			const IDragDropSource* source ) const {return kFalse;};
		virtual bool16 IsActionDisabled( ActionID action ) const;
		virtual bool16 IsActionHidden( ActionID action ) const;
		virtual bool16 IsSubMenuDisabled( const PMString& untranslatedSubMenuName ) const {return kFalse;}
		virtual bool16 IsSubMenuHidden( const PMString& untranslatedSubMenuName ) const {return kFalse;}
		virtual bool16 IsPlatformDialogControlSuppressed( const PMString& platformDialogIdentifier ) const {return kFalse;};
		virtual void Reset() {};
};