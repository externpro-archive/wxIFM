/*!
    Implementation file for the drag and drop docking plugin

    \file   dragndrop.cpp
    \author Robin McNeill
    \date   Created: 01/15/05

    Copyright (c) Robin McNeill
    Licensed under the terms of the wxWindows license
*/

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/ifm/dragndrop.h"

#include "dock_left.xpm"
#include "dock_left_mo.xpm"
#include "dock_up.xpm"
#include "dock_up_mo.xpm"
#include "dock_down.xpm"
#include "dock_down_mo.xpm"
#include "dock_right.xpm"
#include "dock_right_mo.xpm"
#include "dock_center.xpm"
#include "dock_center_mo.xpm"

#include "wx/dcclient.h"
#include "wx/msgdlg.h"
#include "wx/settings.h"

// images used for rendering drop targets
wxBitmap img_left, img_right, img_top, img_bottom, img_tab,
    img_leftHover, img_rightHover, img_topHover, img_bottomHover, img_tabHover;

DEFINE_EVENT_TYPE(wxEVT_IFM_INITDRAG);
DEFINE_EVENT_TYPE(wxEVT_IFM_BEGINDRAG);
DEFINE_EVENT_TYPE(wxEVT_IFM_ENDDRAG);
DEFINE_EVENT_TYPE(wxEVT_IFM_DRAGGING);

DEFINE_EVENT_TYPE(wxEVT_IFM_SHOWDROPTARGETS);

IMPLEMENT_DYNAMIC_CLASS(wxIFMDefaultDockingPlugin, wxIFMExtensionPluginBase);
IMPLEMENT_DYNAMIC_CLASS(wxIFMDockEventEx, wxIFMDockEvent);

BEGIN_EVENT_TABLE(wxIFMDefaultDockingPlugin, wxIFMExtensionPluginBase)
#if IFM_CANFLOAT
    EVT_IFM_LEFTDCLICK  (wxIFMDefaultDockingPlugin::OnLeftDClick)
#endif
    EVT_IFM_LEFTUP      (wxIFMDefaultDockingPlugin::OnLeftUp)
    EVT_IFM_MOTION      (wxIFMDefaultDockingPlugin::OnMouseMove)
    EVT_IFM_KEYDOWN     (wxIFMDefaultDockingPlugin::OnKeyDown)
    EVT_IFM_DRAG_INIT   (wxIFMDefaultDockingPlugin::OnDragInit)
    EVT_IFM_DRAG_BEGIN  (wxIFMDefaultDockingPlugin::OnDragBegin)
    EVT_IFM_DRAG_END    (wxIFMDefaultDockingPlugin::OnDragEnd)
    EVT_IFM_DRAG_DRAGGING (wxIFMDefaultDockingPlugin::OnDrag)
    EVT_IFM_SHOWDROPTARGETS (wxIFMDefaultDockingPlugin::OnShowDropTargets)
    EVT_IFM_DOCK        (wxIFMDefaultDockingPlugin::OnDock)
END_EVENT_TABLE()

wxIFMDefaultDockingPlugin::wxIFMDefaultDockingPlugin()
    : wxIFMExtensionPluginBase(),
    m_realtime(false),
    m_dragging(false),
    m_captured(false),
    m_dragx(0),
    m_dragy(0),
    m_oldBtn(NULL)
#if IFM_CANFLOAT
    ,m_window(NULL)
#endif
{ }

bool wxIFMDefaultDockingPlugin::Initialize(wxIFMInterfacePluginBase *plugin)
{
    wxIFMExtensionPluginBase::Initialize(plugin);

    m_ip = wxDynamicCast(plugin, wxIFMDefaultInterfacePlugin);

    if( !m_ip )
    {
        wxMessageBox(wxT("wxIFMDefaultDockingPlugin only extends wxIFMDefaultInterfacePlugin"), wxT("Error"),
            wxICON_ERROR | wxOK);
        return false;
    }

    // initialize drop bitmaps
    img_left = wxBitmap(dock_left_xpm);
    img_leftHover = wxBitmap(dock_left_mo_xpm);
    img_top = wxBitmap(dock_up_xpm);
    img_topHover = wxBitmap(dock_up_mo_xpm);
    img_bottom = wxBitmap(dock_down_xpm);
    img_bottomHover = wxBitmap(dock_down_mo_xpm);
    img_right = wxBitmap(dock_right_xpm);
    img_rightHover = wxBitmap(dock_right_mo_xpm);
    img_tab = wxBitmap(dock_center_xpm);
    img_tabHover = wxBitmap(dock_center_mo_xpm);

    return true;
}

#if IFM_CANFLOAT
void wxIFMDefaultDockingPlugin::OnLeftDClick(wxIFMMouseEvent &event)
{
    // float the component if inside of background rect
    wxIFMComponent *component = event.GetComponent();
    const wxPoint &pos = event.GetMouseEvent().GetPosition();

    if( !component )
    {
        event.Skip();
        return;
    }

    // mouse must be within background rect to initiate floating
    wxIFMHitTestEvent hitevt(component, IFM_COORDS_BACKGROUND, pos);
    GetIP()->ProcessPluginEvent(hitevt);

    if( !hitevt.GetPassed() )
    {
        event.Skip();
        return;
    }
    else
    {
        wxIFMFloatingData *floating_data = IFM_GET_EXTENSION_DATA(component, wxIFMFloatingData);
        wxIFMFloatingWindowBase *parent = NULL;
        bool destroy = false;

        if( floating_data->m_floating )
        {
            parent = floating_data->m_window;
            if( !component->m_docked )
                destroy = true;
        }

        wxIFMFloatEvent evt(component, floating_data->m_rect);
        GetIP()->ProcessPluginEvent(evt);

        // if we floated the root component, destroy the old window instead of updating it
        if( destroy )
        {
            wxIFMDestroyFloatingWindowEvent evt(parent, false);
            GetIP()->ProcessPluginEvent(evt);
        }
        else if( parent )
            parent->Update();
        else
            GetManager()->Update();
    }
}
#endif

void wxIFMDefaultDockingPlugin::OnLeftUp(wxIFMMouseEvent &event)
{
    if( m_captured )
    {
        m_captured = false;
        GetManager()->ReleaseInput();
    }

    if( m_dragging )
    {
        m_dragging = false;
        wxIFMDragEvent evt(wxEVT_IFM_ENDDRAG, event.GetComponent(),
            event.GetMouseEvent().GetPosition(),
            event.GetMouseEvent().AltDown(), event.GetMouseEvent().ShiftDown(),
            event.GetMouseEvent().ControlDown(), m_realtime);
        GetIP()->ProcessPluginEvent(evt);
    }
    else
        event.Skip();
}

void wxIFMDefaultDockingPlugin::OnMouseMove(wxIFMMouseEvent &event)
{
    wxIFMComponent *component = event.GetComponent();
    const wxPoint &pos = event.GetMouseEvent().GetPosition();

    if( m_dragging )
    {
        wxIFMDragEvent evt(wxEVT_IFM_DRAGGING, component, pos,
            event.GetMouseEvent().AltDown(), event.GetMouseEvent().ShiftDown(),
            event.GetMouseEvent().ControlDown(), m_realtime);
        GetIP()->ProcessPluginEvent(evt);
        return;
    }
    else
    {
        if( m_captured )
        {
            wxPoint distance = m_clickPos - GetManager()->GetCapturedWindow()->ClientToScreen(pos);
            if( distance.x < 0 ) distance.x *= -1;
            if( distance.y < 0 ) distance.y *= -1;

            if( distance.x > m_dragx || distance.y > m_dragy )
            {
                wxIFMDragEvent evt(wxEVT_IFM_BEGINDRAG, component, pos,
                    event.GetMouseEvent().AltDown(), event.GetMouseEvent().ShiftDown(),
                    event.GetMouseEvent().ControlDown(), m_realtime);
                GetIP()->ProcessPluginEvent(evt);
                return;
            }
        }
    }

    event.Skip();
}

void wxIFMDefaultDockingPlugin::OnKeyDown(wxIFMKeyEvent &event)
{
    if( m_dragging )
    {
        int keycode = event.GetKeyEvent().GetKeyCode();
        if( keycode == WXK_ESCAPE )
        {
            m_dragging = false;
            m_captured = false;
            GetManager()->ReleaseInput();

            // cancel dragging
            wxIFMDragEvent evt(wxEVT_IFM_ENDDRAG, event.GetComponent(),
            event.GetKeyEvent().GetPosition(),
            event.GetKeyEvent().AltDown(), event.GetKeyEvent().ShiftDown(),
            event.GetKeyEvent().ControlDown(), m_realtime, true);
            GetIP()->ProcessPluginEvent(evt);
            return;
        }
        else if( keycode == WXK_ALT || keycode == WXK_SHIFT || keycode == WXK_CONTROL )
        {
            // send a drag event for immediate updates
            wxIFMDragEvent evt(wxEVT_IFM_DRAGGING, event.GetComponent(), wxGetMousePosition(),
                wxGetKeyState(WXK_ALT), wxGetKeyState(WXK_SHIFT), wxGetKeyState(WXK_CONTROL),
                m_realtime);
            GetIP()->ProcessPluginEvent(evt);
        }
    }

    event.Skip();
}

void wxIFMDefaultDockingPlugin::OnDragInit(wxIFMInitDragEvent &event)
{
    if( !m_captured )
    {
        wxIFMComponent *component = event.GetDraggedComponent();

        wxASSERT_MSG(component, wxT("Dragging a NULL component?"));
        if( !component )
            return;

        m_captured = true;
        GetManager()->CaptureInput(component);
        m_clickPos = GetManager()->GetCapturedWindow()->ClientToScreen(event.GetPos());

        m_dragx = wxSystemSettings::GetMetric(wxSYS_DRAG_X);
        m_dragy = wxSystemSettings::GetMetric(wxSYS_DRAG_Y);

        if( m_dragx == -1 )
            m_dragx = IFM_DRAG_DISTANCE;
        if( m_dragy == -1 )
            m_dragy = IFM_DRAG_DISTANCE;
    }
}

void wxIFMDefaultDockingPlugin::OnDragBegin(wxIFMDragEvent &event)
{
    m_dragging = true;

    m_oldPos = event.GetPosition();
    wxIFMComponent *component = event.GetComponent();

    // calculate offset

    //when floating, keep the window in the same position relative to the mouse
    wxWindow *win = GetManager()->GetCapturedWindow();

    wxPoint pos = win->ClientToScreen(m_oldPos);


    //gets the offset in "window coordinates - relative to total size of the window, including
    //non-client area
    m_offset = pos - win->GetPosition();

#if IFM_CANFLOAT
    wxIFMFloatingData *floating_data = IFM_GET_EXTENSION_DATA(component, wxIFMFloatingData);

    wxIFMFloatingWindowBase *floating_parent = floating_data->m_window;

    // float the window immediately (if not floating already)
    if( !floating_data->m_floating )
    {
        m_offset = wxPoint(12,12);
        wxIFMFloatEvent floatevt(component, pos, floating_data->m_rect.GetSize());
        GetIP()->ProcessPluginEvent(floatevt);

        // store floating window for later
        m_window = floating_data->m_window;

        // update interface
        GetManager()->Update();
    }
    else
    {
        bool reuse_window = false;

        // if we are dragging the root component, just drag the floating window instead
        if( !component->m_docked )
            reuse_window = true;

        // don't make a new window if we are dragging the only child panel either
        {
            wxIFMComponent *root = floating_data->m_window->m_component;
            if( root->m_children.size() == 1 && root->m_children[0] == component )
                reuse_window = true;
        }

        if( reuse_window )
            m_window = floating_data->m_window;
        else
        {
            // fix the offset being incorrect if you are re-floating an already floating window
            m_offset = wxPoint(12,12);

            // don't delete the old floating window to prevent wierd bugs
            //! \todo Investigate these bugs!
            wxIFMUndockEvent evt(component, false);
            GetIP()->ProcessPluginEvent(evt);

            wxIFMFloatEvent floatevt(component, pos, floating_data->m_rect.GetSize());
            GetIP()->ProcessPluginEvent(floatevt);

            m_window = floating_data->m_window;

            if( floating_parent->GetComponent()->m_children.GetCount() == 0 )
            {
                // FIXME: The huge if statement above prevents this code from ever firing (right now)
                // but when this code does fire, windows is unhappy and messes up mouse capture
                wxIFMDestroyFloatingWindowEvent evt(floating_parent, true);
                GetIP()->ProcessPluginEvent(evt);
            }
            else
                // update the floating window we docked out of
                floating_parent->Update();
        }
    }

#endif

    CreateTargetButtons();
    ShowFrameDropButtons(true);

    GetManager()->DisplayStatusMessage(wxT("Hold down alt to prevent drop target buttons from moving. Hold down shift to hide drop target buttons."));
}

void wxIFMDefaultDockingPlugin::OnDragEnd(wxIFMDragEvent &event)
{
    ShowComponentDropButtons(false);
    ShowFrameDropButtons(false);

    if( !event.WasCanceled() )
    {
        wxPoint pos = event.GetPosition();
        wxIFMComponent *component = event.GetComponent();
        bool new_container = true, destroy_floating_window = false;

#if IFM_CANFLOAT
        wxIFMFloatingWindowBase *m_floatingParent = NULL;
        wxIFMFloatingData *floating_data = IFM_GET_EXTENSION_DATA(component, wxIFMFloatingData);
        // only set this value if we are undocking from a floating window, and we aren't undocking
        // the root container of the floating window
        if( floating_data->m_floating )
            m_floatingParent = floating_data->m_window;
#endif

        wxIFMComponent *destination = NULL;

        if( m_oldBtn && !(m_oldBtn->GetId() == IFM_DOCK_ID_TAB && !m_oldBtn->GetComponent()) )
        {
            wxIFMComponent *parent = component->m_parent;
            if( parent )
            {
                // undock the component first
                wxIFMUndockEvent evt(component);
                GetIP()->ProcessPluginEvent(evt);
            }
#if IFM_CANFLOAT
            else if( !floating_data->m_floating )
#else
            else
#endif
            {
                // no parent and not floating means the component is a top level container
                // remove it from the top level container list first
                wxIFMRemoveTopContainerEvent evt(component);
                GetIP()->ProcessPluginEvent(evt);
                //new_container = false;
            }
#if IFM_CANFLOAT
            else
            {
                // no parent and floating means its the root component of a floating window
                //new_container = false;
                destroy_floating_window = true;
            }
#endif

            // we don't need a new container if we are already a container...
            if( component->GetType() == IFM_COMPONENT_CONTAINER )
                new_container = false;

            // generate the appropriate dock event
            int id = m_oldBtn->GetId();

            if( !m_oldBtn->GetComponent() )
            {
                wxIFMComponent *container;

                if( new_container )
                {
                    // create a new container
                    wxIFMNewComponentEvent newevt(IFM_COMPONENT_CONTAINER);
                    GetIP()->ProcessPluginEvent(newevt);

                    // WARNING: This may return null
                    container = newevt.GetComponent();
                }
                else
                    container = component;

                wxIFMContainerData *data = IFM_GET_EXTENSION_DATA(container, wxIFMContainerData);
                bool front = false;

                switch(id)
                {
                    case IFM_DOCK_ID_FRAME_TOP:
                        front = true;
                    case IFM_DOCK_ID_TOP:
                        data->m_orientation = IFM_ORIENTATION_TOP;
                        container->m_alignment = IFM_ALIGN_HORIZONTAL;
                        break;
                    case IFM_DOCK_ID_FRAME_BOTTOM:
                        front = true;
                    case IFM_DOCK_ID_BOTTOM:
                        data->m_orientation = IFM_ORIENTATION_BOTTOM;
                        container->m_alignment = IFM_ALIGN_HORIZONTAL;
                        break;
                    case IFM_DOCK_ID_FRAME_LEFT:
                        front = true;
                    case IFM_DOCK_ID_LEFT:
                        data->m_orientation = IFM_ORIENTATION_LEFT;
                        container->m_alignment = IFM_ALIGN_VERTICAL;
                        break;
                    case IFM_DOCK_ID_FRAME_RIGHT:
                        front = true;
                    case IFM_DOCK_ID_RIGHT:
                        data->m_orientation = IFM_ORIENTATION_RIGHT;
                        container->m_alignment = IFM_ALIGN_VERTICAL;
                        break;
                }

                // add container to front of the list
                if( front )
                {
                    wxIFMAddTopContainerEvent evt(container, 0);
                    GetIP()->ProcessPluginEvent(evt);
                }
                else
                {
                    wxIFMAddTopContainerEvent evt(container, -1);
                    GetIP()->ProcessPluginEvent(evt);
                }

                if( new_container )
                {
                    // dock the panel into the new container
                    wxIFMDockEvent dockevt(component, container, 0);
                    GetIP()->ProcessPluginEvent(dockevt);
                }
            }
            else
            {
                destination = m_oldBtn->GetComponent();
                wxIFMComponent *container = NULL;
                int index = 0;

                // if we are docking to the left/right of a the only child of a top level vertical
                // container or to the top/bottom the only child of a horizontal one, dock next to that
                // top level container in a new top level container instead of using the generic docking
                // fallback code found at the bottom of this function
                if( destination->GetType() == IFM_COMPONENT_PANEL )
                {
                    wxIFMComponent *parent = destination->m_parent;
                    const wxIFMComponentList &containers = m_ip->GetTopContainerList();

                    if( parent && containers.Find(parent) && parent->m_children.size() == 1 &&
                        (
                            (parent->m_alignment == IFM_ALIGN_VERTICAL && (id == IFM_DOCK_ID_LEFT || id == IFM_DOCK_ID_RIGHT)) ||
                            (parent->m_alignment == IFM_ALIGN_HORIZONTAL && (id == IFM_DOCK_ID_TOP || id == IFM_DOCK_ID_BOTTOM))
                        )
                      )
                    {
                        // dock next to parent container instead
                        destination = parent;
                    }
                }

                if( destination->GetType() == IFM_COMPONENT_CONTAINER )
                {
                    wxIFMContainerData *data = IFM_GET_EXTENSION_DATA(destination, wxIFMContainerData);

                    if( (destination->m_alignment == IFM_ALIGN_VERTICAL && (id == IFM_DOCK_ID_LEFT || id == IFM_DOCK_ID_RIGHT)) ||
                        (destination->m_alignment == IFM_ALIGN_HORIZONTAL && (id == IFM_DOCK_ID_TOP || id == IFM_DOCK_ID_BOTTOM)) )
                    {
                        if( !destination->m_docked )
                        {
                            // create a new container
                            if( new_container )
                            {
                                wxIFMNewComponentEvent newevt(IFM_COMPONENT_CONTAINER);
                                GetIP()->ProcessPluginEvent(newevt);
                                container = newevt.GetComponent();
                            }
                            else
                                container = component;

                            wxIFMContainerData *contdata = IFM_GET_EXTENSION_DATA(container, wxIFMContainerData);

                            container->m_alignment = destination->m_alignment;
                            contdata->m_orientation = data->m_orientation;

                            if( new_container )
                            {
                                // dock into it
                                wxIFMDockEvent dockevt(component, container, 0);
                                GetIP()->ProcessPluginEvent(dockevt);
                            }

                            bool before;
                            if( destination->m_alignment == IFM_ALIGN_VERTICAL )
                                before = id == IFM_DOCK_ID_LEFT ? true : false;
                            else
                                before = id == IFM_DOCK_ID_TOP ? true : false;

                            // if the destination is a container, we need to invert before depending
                            // on what side of the application the container is on
                            if( destination->GetType() == IFM_COMPONENT_CONTAINER )
                            {
                                wxIFMContainerData *contdata = IFM_GET_EXTENSION_DATA(destination, wxIFMContainerData);

                                wxASSERT_MSG(contdata, wxT("container with no container data?"));
                                if( contdata && (contdata->m_orientation == IFM_ORIENTATION_BOTTOM || contdata->m_orientation == IFM_ORIENTATION_RIGHT) )
                                    before = !before;
                            }

                            // add the container as a top container
                            wxIFMAddTopContainerEvent topevt(container, destination, before);
                            GetIP()->ProcessPluginEvent(topevt);
                        }
                        else
                        {
                            //! \todo Fix this, it doesn't do quite what is expected, but at least it doesn't crash

                            // send custom dock event
                            wxIFMDockEventEx dockevt(component, destination, index, id);
                            GetIP()->ProcessPluginEvent(dockevt);
                        }
                    }
                    else if( (destination->m_alignment == IFM_ALIGN_VERTICAL && (id == IFM_DOCK_ID_TOP || id == IFM_DOCK_ID_BOTTOM)) ||
                        (destination->m_alignment == IFM_ALIGN_HORIZONTAL && (id == IFM_DOCK_ID_LEFT || id == IFM_DOCK_ID_RIGHT)) )
                    {
                        int index;

                        if( destination->m_alignment == IFM_ALIGN_VERTICAL )
                            index = id == IFM_DOCK_ID_TOP ? 0 : -1;
                        else
                            index = id == IFM_DOCK_ID_LEFT ? 0 : -1;

                        // send a normal dock event
                        wxIFMDockEvent dockevt(component, destination, index);
                        GetIP()->ProcessPluginEvent(dockevt);
                    }
                }
                // docking into panels as a tab
                else if( destination->GetType() == IFM_COMPONENT_PANEL && id == IFM_DOCK_ID_TAB )
                {
                    // send a normal dock event
                    wxIFMDockEvent dockevt(component, destination, IFM_DEFAULT_INDEX);
                    GetIP()->ProcessPluginEvent(dockevt);
                }
                // generic docking fallback
                else if( destination->m_parent )
                {
                    wxIFMComponent *parent = destination->m_parent;

                    // find the index of the component within its parent
                    {
                        const wxIFMComponentArray &children = parent->m_children;
                        //for( int count = children.GetCount(); index < count; index++ )
                        for( wxIFMComponentArray::const_iterator i = children.begin(), end = children.end(); i != end; ++i, index++ )
                        {
                            //if( children[index] == destination )
                            if( *i == destination )
                                break;
                        }
                    }

                    // send custom dock event
                    wxIFMDockEventEx dockevt(component, parent, index, id);
                    GetIP()->ProcessPluginEvent(dockevt);
                }
            }

#if IFM_CANFLOAT
            // update the destination floating window too
            wxIFMFloatingData *dest_floating_data = NULL;
            if( destination )
                dest_floating_data = IFM_GET_EXTENSION_DATA(destination, wxIFMFloatingData);
            if( destination && dest_floating_data->m_floating )
            {
                wxIFMFloatingWindowBase *window = dest_floating_data->m_window;
                // FIXME: accessing m_rect directly here, is this ok?
                wxIFMUpdateComponentEvent updevt(window->GetComponent(), window->GetComponent()->m_rect);
                GetIP()->ProcessPluginEvent(updevt);
            }
            // update the floating parent if we undocked from it
            if( m_floatingParent )
            {
                m_floatingParent->Update();
            }
#endif

            // update interface
            GetManager()->Update();
        }
#if IFM_CANFLOAT
        else
        {
            component->GetParentWindow()->SetFocus();
        }

        if( destroy_floating_window )
        {
            // destroy the floating window, but not its root component
            // FIXME: If I drag the only panel out of a floating window, the root component
            // of this floating window wont be destroyed
            wxIFMDestroyFloatingWindowEvent evt(m_floatingParent, false);
            GetIP()->AddPendingEvent(evt);
        }
#endif
    }

    DestroyTargetButtons();

    m_oldBtn = NULL;

    GetManager()->ResetStatusMessage();
}

void wxIFMDefaultDockingPlugin::OnDrag(wxIFMDragEvent &event)
{
    wxPoint client_pos = event.GetPosition();
    wxIFMComponent *component = event.GetComponent();

    wxPoint pos;
    pos = GetManager()->GetCapturedWindow()->ClientToScreen(client_pos);

    wxIFMDockTargetButton *btn = GetDockTargetButtonByPos(pos);

#if IFM_CANFLOAT
    wxIFMFloatingData *floating_data = IFM_GET_EXTENSION_DATA(component, wxIFMFloatingData);

    if( floating_data->m_floating )
        component = GetFloatingComponentByPosExclusion(pos, floating_data->m_window);

    if( !component )
#endif
        component = m_ip->GetComponentByPos(GetManager()->GetParent()->ScreenToClient(pos), NULL);

    // if the component the mouse is over
    if( btn )
    {
        wxSetCursor(wxCursor(wxCURSOR_ARROW));
        btn->SetHover(true);
    }
    else
    {
#if !IFM_CANFLOAT
        wxSetCursor(wxCursor(wxCURSOR_NO_ENTRY));
#endif

        if( event.ShiftDown() )
        {
            ShowFrameDropButtons(false);
            ShowComponentDropButtons(false);
        }
        else
        {
            static wxIFMComponent *last_component = NULL;
            static bool last_component_set = false;
            if( event.AltDown() )
            {
                if( !last_component_set )
                {
                    last_component = component;
                    last_component_set = true;
                }
                component = last_component;
            }
            else
            {
                last_component_set = false;
                last_component = NULL;
            }

            // special processing for tabbed panels is required. If the mouse is over the active tab
            // of a tabbed panel, display buttons for that tabbed panel instead of the tab
            if( component && component->GetType() == IFM_COMPONENT_PANEL_TAB )
                component = component->m_parent;

            wxIFMShowDropTargetsEvent evt(component, GetManager()->GetParent()->ScreenToClient(pos), event.GetComponent());
            GetIP()->ProcessPluginEvent(evt);
        }

#if IFM_CANFLOAT
        // move the floating window
        if( m_window )
            m_window->GetWindow()->Move(pos - m_offset);
#endif

        m_oldPos = pos;
    }

    if( m_oldBtn && m_oldBtn != btn )
            m_oldBtn->SetHover(false);

    m_oldBtn = btn;
}

void wxIFMDefaultDockingPlugin::OnDock(wxIFMDockEvent &event)
{
    wxIFMDockEventEx *evt = wxDynamicCast(&event,wxIFMDockEventEx);

    if( evt )
    {
        int where = evt->GetWhere();
        int index = evt->GetIndex();

        wxIFMComponent *dest = evt->GetDestination();

        if( (dest->m_alignment == IFM_ALIGN_HORIZONTAL && where == IFM_DOCK_ID_RIGHT) ||
            (dest->m_alignment == IFM_ALIGN_VERTICAL &&  where == IFM_DOCK_ID_BOTTOM) )
        {
            evt->SetIndex(index + 1);
        }
        else if( (dest->m_alignment == IFM_ALIGN_HORIZONTAL && (where == IFM_DOCK_ID_TOP || where == IFM_DOCK_ID_BOTTOM)) ||
            (dest->m_alignment == IFM_ALIGN_VERTICAL && (where == IFM_DOCK_ID_LEFT || where == IFM_DOCK_ID_RIGHT)) )
        {
            // need to create a container, undock the child at the current index,
            // dock the container where the child was, dock the child into the container,
            // and finally dock the component into the container

            // just change the orientation of the container if it has one child
            if( dest->m_children.size() == 1 )
            {
                if( dest->m_alignment == IFM_ALIGN_HORIZONTAL )
                    dest->m_alignment = IFM_ALIGN_VERTICAL;
                else if( dest->m_alignment == IFM_ALIGN_VERTICAL )
                    dest->m_alignment = IFM_ALIGN_HORIZONTAL;

                event.Skip();
            }
            else
            {
                // make a container
                wxIFMNewComponentEvent newevt(IFM_COMPONENT_CONTAINER);
                GetIP()->ProcessPluginEvent(newevt);
                wxIFMComponent *container = newevt.GetComponent();

                container->m_alignment = dest->m_alignment == IFM_ALIGN_HORIZONTAL ? IFM_ALIGN_VERTICAL : IFM_ALIGN_HORIZONTAL;

                wxIFMComponent *child = dest->m_children[index];

                // dock the container where the child was
                wxIFMDockEvent dockevt1(container, dest, index);
                GetIP()->ProcessPluginEvent(dockevt1);

                // undock the child
                wxIFMUndockEvent undockevt(child, false);
                GetIP()->ProcessPluginEvent(undockevt);

                // dock the child into the container
                wxIFMDockEvent dockevt2(child, container, IFM_DEFAULT_INDEX);
                GetIP()->ProcessPluginEvent(dockevt2);

                // dock the component into the container using the special dock event
                // for proper "where" processing
                wxIFMDockEventEx dockevt3(event.GetComponent(), container, 0, where);
                GetIP()->ProcessPluginEvent(dockevt3);
            }

            return;
        }
    }

    // let the container handle the docking now
    event.Skip();
}

void wxIFMDefaultDockingPlugin::DrawHintRect(wxDC &dc, const wxRect &rect)
{
    dc.SetPen(*wxGREY_PEN);
    dc.SetLogicalFunction(wxXOR);

    // draw border
    int top = rect.y, left = rect.x, right = left + rect.width - 1, bottom = top + rect.height - 1;
    dc.DrawLine(left + 1, top, right, top);
    dc.DrawLine(left, top, left, bottom);
    dc.DrawLine(left, bottom, right + 1, bottom);
    dc.DrawLine(right, top, right, bottom);

    dc.SetLogicalFunction(wxCOPY);
    dc.SetPen(wxNullPen);
}

inline void create_helper(DockButtonArray &array, wxIFMDockTargetButton *btn)
{
    //btn->Raise();
    array.push_back(btn);
}

void wxIFMDefaultDockingPlugin::CreateTargetButtons()
{
    wxWindow *parent = GetManager()->GetParent();
    wxPoint pos;
    wxIFMDockTargetButton *btn;

    // Frame buttons
    wxRect client_rect = GetManager()->GetInterfaceRect();
    client_rect.SetPosition(GetManager()->GetParent()->ClientToScreen(client_rect.GetPosition()));

    // left button
    pos.x = IFM_DOCK_TARGET_BUTTON_WIDTH + client_rect.x;
    pos.y = client_rect.height / 2 - IFM_DOCK_TARGET_BUTTON_WIDTH / 2 + client_rect.y;
    btn = new wxIFMDockTargetButton(parent, pos, IFM_DOCK_ID_FRAME_LEFT, IFM_DOCK_ICON_LEFT);
    create_helper(m_dockButtonArray, btn);

    // top button
    pos.x = client_rect.width / 2 - IFM_DOCK_TARGET_BUTTON_WIDTH / 2 + client_rect.x;
    pos.y = IFM_DOCK_TARGET_BUTTON_WIDTH + client_rect.y;
    btn = new wxIFMDockTargetButton(parent, pos, IFM_DOCK_ID_FRAME_TOP, IFM_DOCK_ICON_TOP);
    create_helper(m_dockButtonArray, btn);

    // right button
    pos.x = client_rect.width - IFM_DOCK_TARGET_BUTTON_WIDTH - IFM_DOCK_TARGET_BUTTON_WIDTH + client_rect.x;
    pos.y = client_rect.height / 2 - IFM_DOCK_TARGET_BUTTON_WIDTH / 2 + client_rect.y;
    btn = new wxIFMDockTargetButton(parent, pos, IFM_DOCK_ID_FRAME_RIGHT, IFM_DOCK_ICON_RIGHT);
    create_helper(m_dockButtonArray, btn);

    // bottom button
    pos.x = client_rect.width / 2 - IFM_DOCK_TARGET_BUTTON_WIDTH / 2 + client_rect.x;
    pos.y = client_rect.height - IFM_DOCK_TARGET_BUTTON_WIDTH - IFM_DOCK_TARGET_BUTTON_WIDTH + client_rect.y;
    btn = new wxIFMDockTargetButton(parent, pos, IFM_DOCK_ID_FRAME_BOTTOM, IFM_DOCK_ICON_BOTTOM);
    create_helper(m_dockButtonArray, btn);

    // Component buttons
    pos.x = pos.y = 3000; // random value off screen

    // left button
    btn = new wxIFMDockTargetButton(parent, pos, IFM_DOCK_ID_LEFT, IFM_DOCK_ICON_LEFT);
    create_helper(m_dockButtonArray, btn);

    // top button
    btn = new wxIFMDockTargetButton(parent, pos, IFM_DOCK_ID_TOP, IFM_DOCK_ICON_TOP);
    create_helper(m_dockButtonArray, btn);

    // right button
    btn = new wxIFMDockTargetButton(parent, pos, IFM_DOCK_ID_RIGHT, IFM_DOCK_ICON_RIGHT);
    create_helper(m_dockButtonArray, btn);

    // bottom button
    btn = new wxIFMDockTargetButton(parent, pos, IFM_DOCK_ID_BOTTOM, IFM_DOCK_ICON_BOTTOM);
    create_helper(m_dockButtonArray, btn);

    // tabulate button
    btn = new wxIFMDockTargetButton(parent, pos, IFM_DOCK_ID_TAB, IFM_DOCK_ICON_TAB);
    create_helper(m_dockButtonArray, btn);
}

void wxIFMDefaultDockingPlugin::DestroyTargetButtons()
{
    for( int i = 0, count = m_dockButtonArray.GetCount(); i < count; ++i )
    //for( DockButtonArray::iterator i = m_dockButtonArray.begin(), end = m_dockButtonArray.end(); i != end; ++i )
        m_dockButtonArray[i]->Destroy();
        //(*i)->Destroy();
    m_dockButtonArray.clear();
}

void wxIFMDefaultDockingPlugin::ShowFrameDropButtons(bool show)
{
    static bool last = false;

    if( show != last )
        last = show;
    else
        return;

    for( int i = 0, end = 4; i < end; ++i )
    {
        // work around the lack of being able to display windows without activating them
        // through wxWidgets API.
        #ifdef __WXMSW__
            m_dockButtonArray[i]->wxWindowBase::Show(show); // need this to keep wx happy
            ::ShowWindow((HWND)m_dockButtonArray[i]->GetHandle(), show ? SW_SHOWNOACTIVATE : SW_HIDE);
        #else
            m_dockButtonArray[i]->Show(show);
        #endif
    }

#ifndef __WXMSW__
    GetManager()->GetParent()->SetFocus();
#endif
}

void wxIFMDefaultDockingPlugin::ShowComponentDropButtons(bool show)
{
    static bool last = false;

    if( show != last )
        last = show;
    else
        return;

    for( int i = 4, end = m_dockButtonArray.GetCount(); i < end; ++i )
    {
        #ifdef __WXMSW__
            m_dockButtonArray[i]->wxWindowBase::Show(show); // need this to keep wx happy
            ::ShowWindow((HWND)m_dockButtonArray[i]->GetHandle(), show ? SW_SHOWNOACTIVATE : SW_HIDE);
        #else
            m_dockButtonArray[i]->Show(show);
        #endif
    }

#ifndef __WXMSW__
    GetManager()->GetParent()->SetFocus();
#endif
}

void wxIFMDefaultDockingPlugin::OnShowDropTargets(wxIFMShowDropTargetsEvent &event)
{
    //! \todo Clean this function
    wxPoint pos = event.GetPosition();
    wxIFMComponent *component = event.GetComponent();

    ShowFrameDropButtons(true);

#if IFM_CANFLOAT
    wxIFMFloatingData *floating_data = NULL;
    if( component )
         floating_data = IFM_GET_EXTENSION_DATA(component, wxIFMFloatingData);
#endif

#if wxCHECK_VERSION(2,7,0)
    if( !GetManager()->GetParent()->GetClientRect().Contains(pos) && !component )
#else
    if( !GetManager()->GetParent()->GetClientRect().Inside(pos) && !component )
#endif
    {
        ShowComponentDropButtons(false);
        return;
    }
    else if( component )
    {
        // don't display target buttons if you hover over the component you're dragging
        // or if you're hovering over the parent of the control you are dragging and the
        // parent's only child is the control you are dragging.
        // also don't display buttons if hovering over the root component of a floating window
        if( (component == event.GetDraggedComponent()) ||
            (component == event.GetDraggedComponent()->m_parent &&
             component->m_children.GetCount() == 1) ||
#if IFM_CANFLOAT
            (floating_data->m_floating && !component->m_docked) ||
#endif
            (wxIFMComponent::IsChildOf(event.GetDraggedComponent(), component))
            )
        {
            ShowComponentDropButtons(false);
            return;
        }
        else
        {
            wxPoint center;
            wxIFMRectEvent rectevt(wxEVT_IFM_GETRECT, component);
            GetIP()->ProcessPluginEvent(rectevt);
            wxRect rect = rectevt.GetRect();
            center.x = rect.x + rect.width / 2;
            center.y = rect.y + rect.height / 2;
            pos = center;
            ShowComponentDropButtons(true);
        }
    }
    else
    {
        wxPoint center;
        wxIFMRectEvent rectevt(wxEVT_IFM_GETCONTENTRECT, NULL);
        GetIP()->ProcessPluginEvent(rectevt);
        wxRect rect = rectevt.GetRect();
        center.x = rect.x + rect.width / 2;
        center.y = rect.y + rect.height / 2;
        pos = center;
        ShowComponentDropButtons(true);
    }

    // position buttons properly over floating windows
    if( component )
        pos = component->GetParentWindow()->ClientToScreen(pos);
    else
        pos = GetManager()->GetParent()->ClientToScreen(pos);

    wxIFMDockTargetButton *btn;
    wxPoint new_pos, tab_pos;
    int i = m_dockButtonArray.GetCount() - 1;
    wxRect rect;

    // tabulate
    btn = m_dockButtonArray[i--];
    tab_pos.x = pos.x - IFM_DOCK_TARGET_BUTTON_WIDTH / 2;
    tab_pos.y = pos.y - IFM_DOCK_TARGET_BUTTON_WIDTH / 2;
    btn->Move(tab_pos);

    // bottom
    btn = m_dockButtonArray[i--];
    new_pos.y = tab_pos.y + IFM_DOCK_TARGET_BUTTON_WIDTH;
    new_pos.x = tab_pos.x;
    btn->Move(new_pos);

    // right
    btn = m_dockButtonArray[i--];
    new_pos.x = tab_pos.x + IFM_DOCK_TARGET_BUTTON_WIDTH;
    new_pos.y = tab_pos.y;
    btn->Move(new_pos);

    // top
    btn = m_dockButtonArray[i--];
    new_pos.y = tab_pos.y - IFM_DOCK_TARGET_BUTTON_WIDTH;
    new_pos.x = tab_pos.x;
    btn->Move(new_pos);

    rect.y = new_pos.y;

    // left
    btn = m_dockButtonArray[i];
    new_pos.x = tab_pos.x - IFM_DOCK_TARGET_BUTTON_WIDTH;
    new_pos.y = tab_pos.y;
    btn->Move(new_pos);

    rect.x = new_pos.x;
    rect.width = IFM_DOCK_TARGET_BUTTON_WIDTH * 3;
    rect.height = IFM_DOCK_TARGET_BUTTON_WIDTH * 3;

    wxRect client_rect = GetManager()->GetInterfaceRect();
    client_rect.SetPosition(GetManager()->GetParent()->ClientToScreen(client_rect.GetPosition()));

    // position the edge buttons here as well. They normally don't move,
    // but if any component drop button overlaps one of the edge buttons,
    // we need to "nudge" the edge button out of the way

    // left button
    wxRect btn_size(0, 0, IFM_DOCK_TARGET_BUTTON_WIDTH, IFM_DOCK_TARGET_BUTTON_WIDTH);
    btn_size.x = IFM_DOCK_TARGET_BUTTON_WIDTH + client_rect.x;
    btn_size.y = client_rect.height / 2 - IFM_DOCK_TARGET_BUTTON_WIDTH / 2 + client_rect.y;
    if( btn_size.Intersects(rect) )
        btn_size.y = rect.y - IFM_DOCK_TARGET_BUTTON_WIDTH;
    m_dockButtonArray[0]->Move(btn_size.GetPosition());

    // top button
    btn_size.x = client_rect.width / 2 - IFM_DOCK_TARGET_BUTTON_WIDTH / 2 + client_rect.x;
    btn_size.y = IFM_DOCK_TARGET_BUTTON_WIDTH + client_rect.y;
    if( btn_size.Intersects(rect) )
        btn_size.x = rect.x - IFM_DOCK_TARGET_BUTTON_WIDTH;
    m_dockButtonArray[1]->Move(btn_size.GetPosition());

    // right button
    btn_size.x = client_rect.width - IFM_DOCK_TARGET_BUTTON_WIDTH - IFM_DOCK_TARGET_BUTTON_WIDTH + client_rect.x;
    btn_size.y = client_rect.height / 2 - IFM_DOCK_TARGET_BUTTON_WIDTH / 2 + client_rect.y;
    if( btn_size.Intersects(rect) )
        btn_size.y = rect.y - IFM_DOCK_TARGET_BUTTON_WIDTH;
    m_dockButtonArray[2]->Move(btn_size.GetPosition());

    // bottom button
    btn_size.x = client_rect.width / 2 - IFM_DOCK_TARGET_BUTTON_WIDTH / 2 + client_rect.x;
    btn_size.y = client_rect.height - IFM_DOCK_TARGET_BUTTON_WIDTH - IFM_DOCK_TARGET_BUTTON_WIDTH + client_rect.y;
    if( btn_size.Intersects(rect) )
        btn_size.x = rect.x - IFM_DOCK_TARGET_BUTTON_WIDTH;
    m_dockButtonArray[3]->Move(btn_size.GetPosition());

    {
        for( int i = 4, count = m_dockButtonArray.GetCount(); i < count; ++i )
        //for( DockButtonArray::iterator i = m_dockButtonArray.begin() + 4, end = m_dockButtonArray.end(); i != end; ++i )
        {
            m_dockButtonArray[i]->SetComponent(component);
            //(*i)->SetComponent(component);
        }
    }
}

wxIFMDockTargetButton *wxIFMDefaultDockingPlugin::GetDockTargetButtonByPos(const wxPoint &pos)
{
    wxIFMDockTargetButton *btn;
    for( int i = 0, count = m_dockButtonArray.GetCount(); i < count; ++i )
    //for( DockButtonArray::const_iterator i = m_dockButtonArray.begin(), end = m_dockButtonArray.end(); i != end; ++i )
    {
        btn = m_dockButtonArray[i];
        //btn = *i;
#if wxCHECK_VERSION(2,7,0)
        if( btn->IsShown() && btn->GetRect().Contains(pos) )
#else
        if( btn->IsShown() && btn->GetRect().Inside(pos) )
#endif
        {
            return btn;
        }
    }

    return NULL;
}

#if IFM_CANFLOAT
wxIFMComponent *wxIFMDefaultDockingPlugin::GetFloatingComponentByPosExclusion(const wxPoint &pos, wxIFMFloatingWindowBase *exclude)
{
    wxIFMFloatingWindowArray windows = GetIP()->GetFloatingWindows();
    wxIFMComponent *ret = NULL;
    wxIFMFloatingWindowBase *window;

    for( int i = 0, count = windows.GetCount(); i < count; ++i )
    //for( wxIFMFloatingWindowArray::const_iterator i = windows.begin(), end = windows.end(); i != end; ++i )
    {
        window = windows[i];
        //window = *i;

        if( window != exclude )
        {
            ret = GetIP()->GetComponentByPos(window->GetWindow()->ScreenToClient(pos), window->GetComponent());
            if( ret )
                return ret;
        }
    }

    return ret;
}
#endif

BEGIN_EVENT_TABLE(wxIFMDockTargetButton, wxWindow)
    EVT_PAINT   (wxIFMDockTargetButton::OnPaint)
    EVT_ERASE_BACKGROUND (wxIFMDockTargetButton::OnEraseBackground)
END_EVENT_TABLE()

wxIFMDockTargetButton::wxIFMDockTargetButton(wxWindow *parent, const wxPoint &pos, int id, int icon, wxIFMComponent *component)
#if 0 //defined(__WXMAC__) || defined (__WXCOCOA)
        : wxWindow(parent, wxID_ANY, pos, IFM_DOCK_TARGET_BUTTON_SIZE, wxCLIP_SIBLINGS | wxNO_BORDER ),
#else
        : wxFrame(parent, wxID_ANY, wxT(""), pos, IFM_DOCK_TARGET_BUTTON_SIZE, /*wxSTAY_ON_TOP | */wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT | wxNO_BORDER ),
#endif
        m_hover(false),
        m_id(id),
        m_icon(icon),
        m_component(component)
{

}

void wxIFMDockTargetButton::SetHover(bool hover)
{
    if( m_hover != hover )
    {
        m_hover = hover;
        Refresh();
    }
}

void wxIFMDockTargetButton::OnEraseBackground(wxEraseEvent &WXUNUSED(event))
{

}

void wxIFMDockTargetButton::OnPaint(wxPaintEvent &WXUNUSED(event))
{
    wxPaintDC dc(this);

    switch(m_icon)
    {
        case IFM_DOCK_ICON_LEFT:
            if( m_hover )
                dc.DrawBitmap(img_leftHover, 0, 0, false);
            else
                dc.DrawBitmap(img_left, 0, 0, false);
            break;
        case IFM_DOCK_ICON_RIGHT:
            if( m_hover )
                dc.DrawBitmap(img_rightHover, 0, 0, false);
            else
                dc.DrawBitmap(img_right, 0, 0, false);
            break;
        case IFM_DOCK_ICON_TOP:
            if( m_hover )
                dc.DrawBitmap(img_topHover, 0, 0, false);
            else
                dc.DrawBitmap(img_top, 0, 0, false);
            break;
        case IFM_DOCK_ICON_BOTTOM:
            if( m_hover )
                dc.DrawBitmap(img_bottomHover, 0, 0, false);
            else
                dc.DrawBitmap(img_bottom, 0, 0, false);
            break;
        case IFM_DOCK_ICON_TAB:
            if( m_hover )
                dc.DrawBitmap(img_tabHover, 0, 0, false);
            else
                dc.DrawBitmap(img_tab, 0, 0, false);
            break;
    }
}
