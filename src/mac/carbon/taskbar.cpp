/////////////////////////////////////////////////////////////////////////////
// Name:       	taskbar.cpp
// Purpose:     wxTaskBarIcon OSX Implementation
// Author:      Ryan Norton
// Modified by:
// Created:     09/25/2004
// RCS-ID:      $Id$
// Copyright:   (c) 2004 Ryan Norton
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#include "wx/defs.h"

#ifdef wxHAS_TASK_BAR_ICON

#include "wx/mac/private.h"

#include "wx/taskbar.h"
#include "wx/menu.h"
#include "wx/icon.h"

#if 0

#include "wx/frame.h"
#include "wx/dialog.h"

#endif

//
//TODO:  Implement  Apple Software Guidelines - show the top window it it's not shown,
//and force it to be unminimized - and all unminimized windows should be brought to 
//the front
//
//TODO:
IMPLEMENT_DYNAMIC_CLASS(wxTaskBarIcon, wxEvtHandler)

pascal OSStatus wxDockEventHandler(	EventHandlerCallRef inHandlerCallRef,
									EventRef inEvent, void* pData)
{
    wxTaskBarIcon*& pTB = (wxTaskBarIcon*&) pData;

    const UInt32 eventClass = GetEventClass(inEvent);
    const UInt32 eventKind = GetEventKind(inEvent);
            
    if (eventClass == kEventClassCommand && eventKind == kEventCommandProcess) 
    {
    //TODO:
    //TODO:	This is a complete copy of 
    //static pascal OSStatus wxMacAppCommandEventHandler( EventHandlerCallRef handler , EventRef event , void *data )
    //FIND A WAY TO EXTERN THIS AND USE THAT HERE INSTEAD!!
    //TODO:
	MenuRef hMenu = MAC_WXHMENU(pTB->GetCurrentMenu()->GetHMenu());
    OSStatus result = eventNotHandledErr ;

    HICommand command ;
    OSErr err;
    
    err =    GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, 	
                            NULL, sizeof(HICommand), NULL, &command);
    wxASSERT(err == noErr);
                
    MenuItemIndex menuItemIndex;
    err = GetIndMenuItemWithCommandID(hMenu, command.commandID, 1, NULL, &menuItemIndex);
    wxASSERT(err == noErr);
                
                    
    MenuCommand id = command.commandID ;
    wxMenuItem* item = NULL;
    // for items we don't really control
    if ( id == kHICommandPreferences )
    {
        id = wxApp::s_macPreferencesMenuItemId ;
        
        wxMenuBar* mbar = wxMenuBar::MacGetInstalledMenuBar() ;
        if ( mbar )
        {
            wxMenu* menu = NULL ;
            item = mbar->FindItem( id , &menu ) ;
        }
    }
   else if (id != 0)
        GetMenuItemRefCon( hMenu , menuItemIndex , (UInt32*) &item ) ;

    if ( item )
    {
                if (item->IsCheckable())
                {
                    item->Check( !item->IsChecked() ) ;
                }

                item->GetMenu()->SendEvent( id , item->IsCheckable() ? item->IsChecked() : -1 ) ;
                result = noErr ;
    }
    return result ;
    }
    
    wxASSERT(eventClass == kEventClassApplication && eventKind == kEventAppGetDockTileMenu);
	
	//set the internal event
	pTB->SetInternalEvent(inEvent);
	
	//process the right click event
	wxTaskBarIconEvent evt(wxEVT_TASKBAR_RIGHT_UP,NULL);
	pTB->ProcessEvent(evt);
		
	//set the internal event
	pTB->SetInternalEvent(NULL);

	return noErr;
}

DEFINE_ONE_SHOT_HANDLER_GETTER( wxDockEventHandler );

wxTaskBarIcon::wxTaskBarIcon(const wxTaskBarIconType& nType)
    : m_nType(nType), m_pEvent(NULL), m_pEventHandlerRef(NULL), m_pMenu(NULL), m_iconAdded(false)
{
    //Register the events that will return the dock menu
	EventTypeSpec tbEventList[] = { { kEventClassCommand, kEventProcessCommand },
                                    { kEventClassApplication, kEventAppGetDockTileMenu } };
	
#ifdef __WXDEBUG__
	OSStatus err =
#endif
      InstallApplicationEventHandler(
            GetwxDockEventHandlerUPP(),
            GetEventTypeCount(tbEventList), tbEventList, 
			this, (&(EventHandlerRef&)m_pEventHandlerRef));
			
	wxASSERT(err == noErr);
}

wxTaskBarIcon::~wxTaskBarIcon()
{
    RemoveEventHandler((EventHandlerRef&)m_pEventHandlerRef);
}

void wxTaskBarIcon::SetInternalEvent(void* pEvent)
{
	m_pEvent = pEvent;
}

wxMenu* wxTaskBarIcon::GetCurrentMenu()
{
    return m_pMenu;
}

// Operations:
bool wxTaskBarIcon::SetIcon(const wxIcon& icon, const wxString& tooltip)
{
    #if 0
    wxASSERT(wxTheApp);
    wxWindow* pTopWindow = wxTheApp->GetTopWindow();
    
    wxASSERT(pTopWindow);
    
    if(pTopWindow->IsKindOf(CLASSINFO(wxDialog)))
        ((wxDialog*)pTopWindow)->SetIcon(icon);
    else
    {
        wxASSERT(pTopWindow->IsKindOf(CLASSINFO(wxFrame)));
        ((wxFrame*)pTopWindow)->SetIcon(icon);
    }
    
    return true;
    #else
    //TODO: (IT WORKS!)  Make work without mask - mayby by using a wxDC?
    
    wxASSERT(icon.GetMask() != NULL);
    
	CGImageRef pImage;
	//create the icon from the bitmap and mask bitmap contained within
	OSStatus err = CreateCGImageFromPixMaps(
		GetGWorldPixMap(MAC_WXHBITMAP(icon.GetHBITMAP())),
		GetGWorldPixMap(MAC_WXHBITMAP(icon.GetMask()->GetMaskBitmap())),
		&pImage
		);
		
	wxASSERT(err == 0);
	
	err = SetApplicationDockTileImage(pImage);
		
	wxASSERT(err == 0);
    
    if (pImage != NULL)
        CGImageRelease(pImage);

	return m_iconAdded = err == noErr;
    #endif
}
	
bool wxTaskBarIcon::RemoveIcon()
{
	OSStatus err = RestoreApplicationDockTileImage();
	wxASSERT(err == 0);

	return !(m_iconAdded = !(err == noErr));
}
	
bool wxTaskBarIcon::PopupMenu(wxMenu *menu)
{
	wxASSERT(m_pEvent != NULL);
    
    if (m_pMenu)
        delete m_pMenu;
        
    m_pMenu = menu;
    menu->SetEventHandler(this);

	//note to self - a MenuRef IS A MenuHandle
	MenuRef hMenu = MAC_WXHMENU(menu->GetHMenu());

	//When we call SetEventParameter it will decrement
	//the reference count of the menu - we need to make
	//sure it stays around in the wxMenu class here
	RetainMenu(hMenu);

	//set the actual dock menu
	OSStatus err = SetEventParameter((EventRef) m_pEvent, kEventParamMenuRef, 
					typeMenuRef, sizeof(MenuRef), 
					 &hMenu);
	wxASSERT(err == 0);
	
	return err == noErr;
}

#endif //wxHAS_TASK_BAR_ICON
