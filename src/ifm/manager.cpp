/*!
    Implementation file for core IFM components

    \file   manager.cpp
    \author Robin McNeill
    \date   Created: 10/15/04

    Copyright (c) Robin McNeill
    Licensed under the terms of the wxWindows license
*/

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/ifm/manager.h"
#include "wx/ifm/plugin.h"
#include "wx/ifm/definterface.h"
#include "wx/ifm/defplugin.h"

#include "wx/statusbr.h"
#include "wx/dc.h"

DEFINE_IFM_DATA_KEY(IFM_FLOATING_DATA_KEY)

DEFINE_IFM_CHILD_TYPE(IFM_CHILD_GENERIC)
DEFINE_IFM_CHILD_TYPE(IFM_CHILD_TOOLBAR)

IMPLEMENT_DYNAMIC_CLASS(wxIFMChildDataBase, wxObject);

#include <wx/arrimpl.cpp>
WX_DEFINE_EXPORTED_OBJARRAY(wxRectArray);
WX_DEFINE_EXPORTED_OBJARRAY(wxSizeArray);

wxIFMComponentDataKeyType GetNewDataKey()
{
    static int next;

    return next++;
}

int GetNewComponentType()
{
    static int next = 1;

    return next++;
}

int GetNewChildType()
{
    static int next = 1;

    return next++;
}

/*
wxInterfaceManager implementation
*/
wxInterfaceManager::wxInterfaceManager(wxWindow *parent, wxWindow *content)
    : m_initialized(false),
    m_flags(IFM_DEFAULT_FLAGS),
    m_parent(parent),
    m_content(content),
    m_capturedComponent(NULL),
    m_capturedType(IFM_COMPONENT_UNDEFINED),
#if IFM_CANFLOAT
    m_floatingCapture(NULL),
#endif
    m_useUpdateRect(false),
    m_statusbar(NULL),
    m_statusbarPane(IFM_DISABLE_STATUS_MESSAGES),
    m_statusMessageDisplayed(false)
{
    wxASSERT_MSG(m_parent, wxT("An interface cannot have a NULL parent"));
}

wxInterfaceManager::~wxInterfaceManager()
{

}

wxWindow *wxInterfaceManager::GetParent() const
{
    return m_parent;
}

const wxRect &wxInterfaceManager::GetInterfaceRect() const
{
    return m_updateRect;
}

void wxInterfaceManager::SetInterfaceRect(const wxRect &rect)
{
    m_useUpdateRect = true;
    m_updateRect = rect;
}

void wxInterfaceManager::ResetInterfaceRect()
{
    m_useUpdateRect = false;
}

wxWindow *wxInterfaceManager::GetContentWindow() const
{
    return m_content;
}

void wxInterfaceManager::SetContentWindow(wxWindow *content)
{
    m_content = content;
}

void wxInterfaceManager::SetFlags(int flags)
{
    m_flags = flags;
}

int wxInterfaceManager::GetFlags() const
{
    return m_flags;
}

void wxInterfaceManager::HideChild(wxWindow *child, bool update)
{
    ShowChild(child, false, update);
}

wxIFMInterfacePluginBase *wxInterfaceManager::GetActiveIP() const
{
    return m_interfacePlugins[m_activeInterface];
}

bool wxInterfaceManager::IsInputCaptured() const
{
    return m_capturedComponent != NULL;
}

wxIFMComponent *wxInterfaceManager::GetCapturedComponent() const
{
    return m_capturedComponent;
}

bool wxInterfaceManager::Initialize(bool defaultPlugins, int flags)
{
    m_flags = flags;

    // event spying
    m_parent->PushEventHandler(this);

    if( defaultPlugins )
    {
        // load default interface plugin
        wxIFMInterfacePluginBase *plugin = new wxIFMDefaultInterfacePlugin();
        if( AddInterfacePlugin(plugin) == -1 )
        {
            delete plugin;
            return (m_initialized = false);
        }
    }

    return (m_initialized = true);
}

void wxInterfaceManager::Shutdown()
{
    wxASSERT_MSG(m_initialized, wxT("Interface not initialized"));

    RemoveAllInterfacePlugins();
    m_parent->RemoveEventHandler(this);
}

bool wxInterfaceManager::AddChild(wxIFMChildDataBase *data, bool update)
{
    wxASSERT_MSG(m_initialized, wxT("Interface not initialized"));
    wxASSERT_MSG(data, wxT("Child data cannot be NULL."));

    wxASSERT_MSG(data->m_child, wxT("Adding a NULL child?"));
    wxASSERT_MSG(data->m_type != IFM_CHILDTYPE_UNDEFINED, wxT("Must specify a child type"));

    // create event
    wxIFMAddChildEvent event(data);
    
    // process event
    GetActiveIP()->ProcessPluginEvent(event);
    if( !event.GetSuccess() )
        return false;

    // update interface?
    if( update )
        Update();

    return true;
}

void wxInterfaceManager::ShowChild(wxWindow *child, bool show, bool update)
{
    // send showchild event
    wxIFMShowChildEvent event(child, show, update);
    GetActiveIP()->ProcessPluginEvent(event);
}

bool wxInterfaceManager::IsChildVisible(wxWindow *child) 
{
    wxIFMQueryChildEvent event(child);
    if(GetActiveIP()->ProcessPluginEvent(event))
        return event.IsVisible();

    return false;
}

void wxInterfaceManager::SetChildSize(wxWindow *child, const wxSize &desired, const wxSize &min,
                                      const wxSize &max, bool update)
{
    // send setchildsize event
    wxIFMSetChildSizeEvent event(child, desired, min, max, update);
    GetActiveIP()->ProcessPluginEvent(event);
}

int wxInterfaceManager::AddInterfacePlugin(wxIFMInterfacePluginBase *plugin, bool select)
{
    // add the default component extension as the very _last_ plugin in the 
    // interface event handler chain so that it always exists and always goes last
    {
        wxIFMDefaultPlugin *_plugin = new wxIFMDefaultPlugin(plugin);
        plugin->SetNextHandler(_plugin);
        _plugin->SetPreviousHandler(plugin);
    }

    if( !plugin->Initialize(this) )
        return -1;

    m_interfacePlugins.push_back(plugin);

    int index = m_interfacePlugins.size() - 1;

    if( select )
        SetActiveInterface(m_interfacePlugins.size() - 1);

    return index;
}

void wxInterfaceManager::SetActiveInterface(int plugin)
{
    for( int i = 0, count = m_interfacePlugins.GetCount(); i < count; ++i )
    //int n = 0;
    //for( wxIFMInterfacePluginArray::const_iterator i = m_interfacePlugins.begin(), end = m_interfacePlugins.end(); i != end; ++i, n++ )
    {
        if( i == plugin )
        //if( n == plugin )
            m_interfacePlugins[i]->Enable();
            //(*i)->Enable();
        else
            m_interfacePlugins[i]->Disable();
            //(*i)->Disable();
    }

    m_activeInterface = plugin;
}

void wxInterfaceManager::RemoveInterfacePlugin(int interface_index)
{
    m_interfacePlugins[interface_index]->Shutdown();

    // remove and delete default component plugin
    wxIFMInterfacePluginBase *plugin = m_interfacePlugins[interface_index];
    wxIFMDefaultPlugin *_plugin = wxDynamicCast(plugin->GetNextHandler(), wxIFMDefaultPlugin);

    delete _plugin;
    delete plugin;

    m_interfacePlugins.erase(m_interfacePlugins.begin() + interface_index);
}

void wxInterfaceManager::RemoveAllInterfacePlugins()
{
    while( m_interfacePlugins.size() > 0 )
        RemoveInterfacePlugin(0);
}

bool wxInterfaceManager::AddExtensionPlugin(wxIFMExtensionPluginBase *plugin/*, int interface_index*/)
{
    return GetActiveIP()->PushExtensionPlugin(plugin);
}

void wxInterfaceManager::RemoveExtensionPlugin(/*int interface_index*/)
{
    GetActiveIP()->PopExtensionPlugin();
}

void wxInterfaceManager::RemoveAllExtensionPlugins(/*int interface_index*/)
{
    GetActiveIP()->PopAllExtensionPlugins();
}

void wxInterfaceManager::CaptureInput(wxIFMComponent *component)
{
    wxASSERT_MSG(component, wxT("Invalid component attempting to capture input"));
    wxASSERT_MSG(!m_capturedComponent, wxT("A component already has captured input!"));

#if IFM_CANFLOAT
    wxIFMFloatingData *data = IFM_GET_EXTENSION_DATA(component, wxIFMFloatingData);
    if( data->m_floating )
    {
        m_floatingCapture = data->m_window;
        m_floatingCapture->GetWindow()->CaptureMouse();
    }
    else
#endif
        m_parent->CaptureMouse();

    m_capturedComponent = component;
    m_capturedType = component->GetType();
}

void wxInterfaceManager::ReleaseInput()
{
    wxASSERT_MSG(m_capturedComponent, wxT("Releasing input capture but nothing has captured it!"));
    if( !m_capturedComponent )
        return;

#if IFM_CANFLOAT
    if( m_floatingCapture )
    {
        m_floatingCapture->GetWindow()->ReleaseMouse();
        m_floatingCapture = NULL;
    }
    else
#endif
        m_parent->ReleaseMouse();

    m_capturedComponent = NULL;
    m_capturedType = IFM_COMPONENT_UNDEFINED;
}

wxWindow *wxInterfaceManager::GetCapturedWindow() const
{
    if( !m_capturedComponent )
        return NULL;

#if IFM_CANFLOAT
    if( m_floatingCapture )
        return m_floatingCapture->GetWindow();
    else
#endif
        return m_parent;
}

void wxInterfaceManager::Update(wxRect rect, bool floating)
{
    wxRect *_rect;

    // if the application gave us a rect to use, use it
    if( m_useUpdateRect )
        _rect = &m_updateRect;
    else
    {
        if( rect == IFM_DEFAULT_RECT )
            m_updateRect = m_parent->GetClientRect();
        else
        {
            m_updateRect = rect;
            m_useUpdateRect = true;
        }
        _rect = &m_updateRect;
    }

    // generate update interface event
    wxIFMUpdateEvent updevt(m_content, *_rect, floating);
    GetActiveIP()->ProcessPluginEvent(updevt);
}

void wxInterfaceManager::AddPendingUpdate(wxRect rect, bool floating)
{
    wxRect *_rect;

    // if the application gave us a rect to use, use it
    if( m_useUpdateRect )
        _rect = &m_updateRect;
    else
    {
        if( rect == IFM_DEFAULT_RECT )
            m_updateRect = m_parent->GetClientRect();
        else
        {
            m_updateRect = rect;
            m_useUpdateRect = true;
        }
        _rect = &m_updateRect;
    }

    // generate update interface event
    wxIFMUpdateEvent updevt(m_content, *_rect, floating);
    GetActiveIP()->AddPendingEvent(updevt);
}

void wxInterfaceManager::UpdateConfiguration()
{
    // tell plugins their configuration data has changed
    wxIFMUpdateConfigEvent event;
    GetActiveIP()->ProcessPluginEvent(event);

    // update the interface including floating windows
    Update(IFM_DEFAULT_RECT, true);
}

void wxInterfaceManager::SetStatusMessagePane(wxStatusBar *sb, int pane)
{
    wxASSERT_MSG(sb, wxT("NULL status bar?"));
    m_statusbar = sb;
    m_statusbarPane = pane;
}

void wxInterfaceManager::DisplayStatusMessage(const wxString &message)
{
    if( m_statusbarPane != IFM_DISABLE_STATUS_MESSAGES )
    {
        if( !m_statusMessageDisplayed )
        {
            m_statusMessageDisplayed = true;
            m_oldStatusMessage = m_statusbar->GetStatusText(m_statusbarPane);
        }

        m_statusbar->SetStatusText(message, m_statusbarPane);
    }
}

void wxInterfaceManager::ResetStatusMessage()
{
    if( m_statusbarPane != IFM_DISABLE_STATUS_MESSAGES )
    {
        if( m_statusMessageDisplayed )
        {
            m_statusMessageDisplayed = false;
            m_statusbar->SetStatusText(m_oldStatusMessage, m_statusbarPane);
            m_oldStatusMessage = wxT("");
        }
    }
}

#if IFM_CANFLOAT

/*
wxIFMFloatingWindow implementation
*/
BEGIN_EVENT_TABLE(wxIFMFloatingWindowBase, wxEvtHandler)
    EVT_MOUSE_EVENTS (wxIFMFloatingWindowBase::OnMouseEvent)
    EVT_PAINT       (wxIFMFloatingWindowBase::OnPaint)
    EVT_MOVE        (wxIFMFloatingWindowBase::OnMoving)
    EVT_MOVING      (wxIFMFloatingWindowBase::OnMoving)
    EVT_SIZE        (wxIFMFloatingWindowBase::OnSize)
    EVT_SIZING      (wxIFMFloatingWindowBase::OnSize)
    EVT_KEY_DOWN    (wxIFMFloatingWindowBase::OnKeyDown)
    EVT_KEY_UP      (wxIFMFloatingWindowBase::OnKeyUp)
    EVT_SET_CURSOR  (wxIFMFloatingWindowBase::OnSetCursor)
    EVT_SHOW        (wxIFMFloatingWindowBase::OnShow)
    EVT_ERASE_BACKGROUND (wxIFMFloatingWindowBase::OnEraseBg)
END_EVENT_TABLE()

wxIFMFloatingWindowBase::wxIFMFloatingWindowBase(wxIFMInterfacePluginBase *ip, wxWindow *parent,
        wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
    : m_ip(ip),
    m_component(NULL),
    m_destroyRoot(true)
{
    m_window = new wxWindow(parent, id, pos, size, style, name);
    ConnectEvents();
}

wxIFMFloatingWindowBase::wxIFMFloatingWindowBase(wxIFMInterfacePluginBase *ip)
    : m_ip(ip)
{ }

wxIFMFloatingWindowBase::~wxIFMFloatingWindowBase()
{
    DisconnectEvents();
    m_window->Destroy();

    // delete our root container
    if( m_destroyRoot )
    {
        wxIFMDeleteComponentEvent evt(m_component);
        GetIP()->ProcessPluginEvent(evt);
    }
}

wxIFMInterfacePluginBase *wxIFMFloatingWindowBase::GetIP()
{
    return m_ip;
}

wxInterfaceManager *wxIFMFloatingWindowBase::GetManager()
{
    return m_ip->GetManager();
}

wxWindow *wxIFMFloatingWindowBase::GetWindow() const
{
    return m_window;
}

wxIFMComponent *wxIFMFloatingWindowBase::GetComponent() const
{
    return m_component;
}

void wxIFMFloatingWindowBase::ConnectEvents()
{
    m_window->PushEventHandler(this);
}

void wxIFMFloatingWindowBase::DisconnectEvents()
{
    m_window->RemoveEventHandler(this);
}

void wxIFMFloatingWindowBase::OnSize(wxSizeEvent &event)
{
    wxIFMFloatingSizeEvent sizeevt(
        (event.GetEventType() == wxEVT_SIZE ? wxEVT_IFM_FLOATING_SIZE : wxEVT_IFM_FLOATING_SIZING),
        this, event);
    if( !GetIP()->ProcessPluginEvent(sizeevt) )
        event.Skip();
}

void wxIFMFloatingWindowBase::OnMoving(wxMoveEvent &event)
{
    wxIFMFloatingMoveEvent moveevt(
        (event.GetEventType() == wxEVT_MOVE ? wxEVT_IFM_FLOATING_MOVE : wxEVT_IFM_FLOATING_MOVING),
        this, event);
    if( !GetIP()->ProcessPluginEvent(moveevt) )
        event.Skip();
}

void wxIFMFloatingWindowBase::OnPaint(wxPaintEvent &WXUNUSED(event))
{
    // send BEGINPAINT message to get a DC with which to paint
    wxIFMBeginPaintEvent beginpaint(m_window);
    GetIP()->ProcessPluginEvent(beginpaint);

    wxDC *dc = beginpaint.GetDC();
    wxASSERT_MSG(dc, wxT("Invalid DC returned by EVT_IFM_BEGINPAINT"));

    m_component->Paint(*dc, m_window->GetUpdateRegion());

    // send ENDPAINT message to clean up the DC used to paint
    wxIFMEndPaintEvent endpaint(dc);
    GetIP()->ProcessPluginEvent(endpaint);
}

void wxIFMFloatingWindowBase::OnKeyDown(wxKeyEvent &event)
{
    wxIFMKeyEvent evt(wxEVT_IFM_KEYDOWN, GetManager()->GetCapturedComponent(), event);
    if( !GetIP()->ProcessPluginEvent(evt) )
        event.Skip();
}

void wxIFMFloatingWindowBase::OnKeyUp(wxKeyEvent &event)
{
    wxIFMKeyEvent evt(wxEVT_IFM_KEYUP, GetManager()->GetCapturedComponent(), event);
    if( !GetIP()->ProcessPluginEvent(evt) )
        event.Skip();
}

void wxIFMFloatingWindowBase::OnSetCursor(wxSetCursorEvent &event)
{
    wxIFMSetCursorEvent evt(event, GetComponentByPos(wxPoint(event.GetX(), event.GetY())));
    if( !GetIP()->ProcessPluginEvent(evt) )
        event.Skip();
}

void wxIFMFloatingWindowBase::OnMouseEvent(wxMouseEvent &event)
{
    wxEventType type = 0, _type = event.GetEventType();

    if( _type == wxEVT_LEFT_DOWN )
        type = wxEVT_IFM_LEFTDOWN;
    else if( _type == wxEVT_LEFT_UP )
        type = wxEVT_IFM_LEFTUP;
    else if( _type == wxEVT_LEFT_DCLICK )
        type = wxEVT_IFM_LEFTDCLICK;
    else if( _type == wxEVT_RIGHT_DOWN )
        type = wxEVT_IFM_RIGHTDOWN;
    else if( _type == wxEVT_RIGHT_UP )
        type = wxEVT_IFM_RIGHTUP;
    else if( _type == wxEVT_RIGHT_DCLICK )
        type = wxEVT_IFM_RIGHTDCLICK;
    else if( _type == wxEVT_MIDDLE_DOWN )
        type = wxEVT_IFM_MIDDLEDOWN;
    else if( _type == wxEVT_MIDDLE_UP )
        type = wxEVT_IFM_MIDDLEUP;
    else if( _type == wxEVT_MIDDLE_DCLICK )
        type = wxEVT_IFM_MIDDLEDCLICK;
    else if( _type == wxEVT_MOTION )
        type = wxEVT_IFM_MOTION;
    else if( _type == wxEVT_MOUSEWHEEL )
        type = wxEVT_IFM_MOUSEWHEEL;

    // figure out which component the mouse is over
    wxIFMComponent *component = NULL;

    if( GetManager()->IsInputCaptured() )
        component = GetManager()->GetCapturedComponent();
    else
        component = GetComponentByPos(event.GetPosition());

    // convert event position into screen coordinates first
    /*
    wxPoint screen = m_window->ClientToScreen(event.GetPosition());
    event.m_x = screen.x;
    event.m_y = screen.y;
    */

    // generate mouse event
    wxIFMMouseEvent evt(type, component, event);
    if( !GetIP()->ProcessPluginEvent(evt) )
        event.Skip();
}

void wxIFMFloatingWindowBase::OnShow(wxShowEvent &event)
{
#if wxCHECK_VERSION(2,9,0)
    m_component->Show(event.IsShown(), true);
#else
    m_component->Show(event.GetShow(), true);
#endif
}

void wxIFMFloatingWindowBase::OnEraseBg(wxEraseEvent &WXUNUSED(event))
{

}

wxIFMComponent *wxIFMFloatingWindowBase::GetComponentByPos(const wxPoint &pos, wxIFMComponent *component)
{
    if( !m_window->IsShown() )
        return NULL;
    else
        return GetIP()->GetComponentByPos(pos, (component == NULL) ? m_component : component);
}

void wxIFMFloatingWindowBase::Update(bool force)
{
    if( m_window->IsShown() || force )
    {
        wxIFMUpdateComponentEvent updevt(m_component, m_component->m_rect);
        GetIP()->ProcessPluginEvent(updevt);
    }
}

void wxIFMFloatingWindowBase::AddPendingUpdate()
{
    if( m_window->IsShown() )
    {
        wxIFMUpdateComponentEvent updevt(m_component, m_component->m_rect);
        GetIP()->AddPendingPluginEvent(updevt);
    }
}

#endif

/*
wxIFMComponent implementation
*/
wxIFMComponent::wxIFMComponent(wxIFMInterfacePluginBase *ip, int type)
    : m_type(type),
    m_ip(ip),
    m_minSize(IFM_NO_MINIMUM_SIZE),
    m_maxSize(IFM_NO_MAXIMUM_SIZE),
    m_fixed(false),
    m_hidden(false),
    m_visible(true),
    m_canHide(true),
    m_docked(false),
    m_alignment(IFM_ALIGN_NONE),
    m_parent(NULL),
    m_childType(IFM_CHILDTYPE_UNDEFINED),
    m_child(NULL)
{
#if IFM_CANFLOAT
    // create floating data data
    wxIFMExtensionDataBase *data = new wxIFMFloatingData;
    m_data[data->GetDataKey()] = data;
#endif
}

wxIFMComponent::wxIFMComponent(const wxIFMComponent &)
{ }

wxIFMComponent::~wxIFMComponent()
{
    // clean up data
    for( wxIFMComponentDataMap::iterator i = m_data.begin(), end = m_data.end(); i != end; ++i )
        delete i->second;
}

wxInterfaceManager *wxIFMComponent::GetManager()
{
    return m_ip->GetManager();
}

int wxIFMComponent::GetType() const
{
    return m_type;
}

void wxIFMComponent::AddExtensionData(wxIFMExtensionDataBase *data)
{
    m_data[data->GetDataKey()] = data;
}

wxIFMExtensionDataBase *wxIFMComponent::GetExtensionData(wxIFMComponentDataKeyType key)
{
    return m_data[key];
}

wxIFMExtensionDataBase *wxIFMComponent::RemoveExtensionData(wxIFMComponentDataKeyType key)
{
    wxIFMExtensionDataBase *ret = m_data[key];
    m_data.erase(key);
    return ret;
}

int wxIFMComponent::GetNextVisibleComponent(const wxIFMComponentArray &components, int start)
{
    for( int i = start, count = components.GetCount(); i < count; ++i )
    {
        if( components[i]->IsVisible() )
            return i;
    }

    // no visible component found
    return -1;
}

bool wxIFMComponent::IsChildOf(wxIFMComponent *parent, wxIFMComponent *child)
{
    wxIFMComponentArray &children = parent->m_children;
    wxIFMComponent *component;
    for( int i = 0, size = children.GetCount(); i < size; ++i )
    //for( wxIFMComponentArray::const_iterator i = children.begin(), end = children.end(); i != end; ++i )
    {
        component = children[i];
        //component = *i;

        if( component == child )
            return true;
        else
        {
            // look through children of the child too
            if( wxIFMComponent::IsChildOf(component, child) )
                return true;
        }
    }

    return false;
}

/*
wxIFMComponent *wxIFMComponent::FindChildWindow(wxWindow *child, const wxIFMComponentArray &components)
{
    wxIFMComponent *component;

    // look for the component containing the child window
    //for( int i = 0, count = components.GetCount(); i < count; ++i )
    for( wxIFMComponentArray::const_iterator i = components.begin(), end = components.end(); i != end; ++i )
    {
        //component = components[i];
        component = *i;

        if( component->m_child == child )
            return component;
    }

    // no component found
    return NULL;
}
*/

void wxIFMComponent::Paint(wxDC &dc, const wxRegion &region)
{
    // get component rect first
    wxRect rect = m_rect;

    // set clipping region of DC
    dc.DestroyClippingRegion();
#if wxCHECK_VERSION(2,9,0)
    dc.SetDeviceClippingRegion(region);
#else
    dc.SetClippingRegion(region);
#endif

    // paint background first
    wxIFMPaintEvent bgevt(wxEVT_IFM_PAINTBG, this, region, dc);
    m_ip->ProcessPluginEvent(bgevt);

    // paint border second
    wxIFMPaintEvent bdevt(wxEVT_IFM_PAINTBORDER, this, region, dc);
    m_ip->ProcessPluginEvent(bdevt);

    // paint decorations last
    wxIFMPaintEvent dcevt(wxEVT_IFM_PAINTDECOR, this, region, dc);
    m_ip->ProcessPluginEvent(dcevt);

    // recursively paint children of this component
    for( size_t i = 0; i < m_children.GetCount(); i++ )
    //for( wxIFMComponentArray::const_iterator i = m_children.begin(), end = m_children.end(); i != end; ++i )
    {
        //wxIFMComponent *child = *i;
        wxIFMComponent *child = m_children[i];

        // only paint the child if needed
        if( child->IsVisible() )
        {
            wxRegionContain result = region.Contains(child->m_rect);
            if( result == wxPartRegion || result == wxInRegion )
            {
                //wxRegion new_region = region;
                //new_region.Intersect(child->m_rect);
                child->Paint(dc, region);
            }
        }
    }
}

wxWindow *wxIFMComponent::GetParentWindow()
{
#if IFM_CANFLOAT
    wxIFMFloatingData *data = IFM_GET_EXTENSION_DATA(this, wxIFMFloatingData);
    if( data->m_floating )
        return data->m_window->GetWindow();
    else
#endif
        return GetManager()->GetParent();
}

wxRect wxIFMComponent::GetRect()
{
    // FIXME: should I use GETRECT event here?
    return m_rect;
}

wxRect wxIFMComponent::GetBackgroundRect()
{
    wxIFMRectEvent evt(wxEVT_IFM_GETBACKGROUNDRECT, this);
    m_ip->ProcessPluginEvent(evt);
    return evt.GetRect();
}

wxRect wxIFMComponent::GetClientRect()
{
    wxIFMRectEvent evt(wxEVT_IFM_GETCLIENTRECT, this);
    m_ip->ProcessPluginEvent(evt);
    return evt.GetRect();
}

wxRect wxIFMComponent::GetConvertedRect(wxRect rect, int coords_from, int coords_to)
{
    wxIFMConvertRectEvent evt(this, coords_from, coords_to, rect);
    m_ip->ProcessPluginEvent(evt);
    return evt.GetRect();
}

wxSize wxIFMComponent::GetDesiredSize()
{
    wxIFMRectEvent evt(wxEVT_IFM_GETDESIREDSIZE, this);
    m_ip->ProcessPluginEvent(evt);
    return evt.GetSize();
}

void wxIFMComponent::SetDesiredSize(const wxSize &size)
{
    wxIFMRectEvent evt(wxEVT_IFM_SETDESIREDSIZE, this, wxPoint(), size);
    m_ip->ProcessPluginEvent(evt);
}

wxSize wxIFMComponent::GetMinSize()
{
    wxIFMRectEvent evt(wxEVT_IFM_GETMINSIZE, this);
    m_ip->ProcessPluginEvent(evt);
    return evt.GetSize();
}

wxSize wxIFMComponent::GetMaxSize()
{
    wxIFMRectEvent evt(wxEVT_IFM_GETMAXSIZE, this);
    m_ip->ProcessPluginEvent(evt);
    return evt.GetSize();
}

void wxIFMComponent::Show(bool s, bool update)
{
    wxIFMShowComponentEvent evt(this, s, update);
    m_ip->ProcessPluginEvent(evt);
    m_hidden = !s;
}

bool wxIFMComponent::IsShown()
{
    return !m_hidden;
}

void wxIFMComponent::VisibilityChanged(bool vis)
{
    if( vis == m_visible )
        return; // we are alread visible

    wxIFMComponentVisibilityChangedEvent evt(this, vis);
    m_ip->ProcessPluginEvent(evt);
    m_visible = vis;
}

bool wxIFMComponent::IsVisible()
{
    return (!m_hidden) & m_visible;
}

/*
    wxIFMChildDataBase implementation
*/
wxIFMChildDataBase::wxIFMChildDataBase()
    : m_type(IFM_CHILDTYPE_UNDEFINED),
    m_minSize(IFM_NO_MINIMUM_SIZE),
    m_maxSize(IFM_NO_MAXIMUM_SIZE),
    m_child(NULL),
    m_hidden(false),
    m_hideable(true),
    m_fixed(false)
{ }

wxIFMChildDataBase::wxIFMChildDataBase(wxWindow *child, int type, const wxString &name,
    wxSize size, bool hidden, wxSize minSize, wxSize maxSize)
    : m_type(type),
    m_desiredSize(size),
    m_minSize(minSize),
    m_maxSize(maxSize),
    m_child(child),
    m_hidden(hidden),
    m_hideable(true),
    m_fixed(false),
    m_name(name)
{ }

wxIFMChildDataBase::wxIFMChildDataBase(const wxIFMChildDataBase &data)
    : m_type(data.m_type),
    m_desiredSize(data.m_desiredSize),
    m_minSize(data.m_minSize),
    m_maxSize(data.m_maxSize),
    m_child(data.m_child),
    m_hidden(data.m_hidden),
    m_hideable(data.m_hideable),
    m_fixed(data.m_fixed),
    m_name(data.m_name)
{ }

wxIFMChildDataBase::~wxIFMChildDataBase()
{ }

/*
    wxIFMExtensionDataBase implementation
*/
wxIFMExtensionDataBase::~wxIFMExtensionDataBase()
{ }

wxIFMComponentDataKeyType wxIFMExtensionDataBase::DataKey()
{
    return IFM_COMPONENT_UNDEFINED;
}

/*
    wxIFMFloatingData implementation
*/
#if IFM_CANFLOAT
wxIFMFloatingData::wxIFMFloatingData()
    : wxIFMExtensionDataBase(),
    m_floating(false),
    m_window(NULL),
    m_rect(IFM_DEFAULT_RECT)
{ }

wxIFMComponentDataKeyType wxIFMFloatingData::GetDataKey() const
{
    return IFM_FLOATING_DATA_KEY;
}

wxIFMComponentDataKeyType wxIFMFloatingData::DataKey()
{
    return IFM_FLOATING_DATA_KEY;
}

#endif
