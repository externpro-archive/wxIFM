/*!
    Defintions for the IFM system

    \file   ifmdefs.h
    \author Robin McNeill
    \date   Created: 10/15/04

    Copyright (c) Robin McNeill
    Licensed under the terms of the wxWindows license
*/

#ifndef _IFM_DEFS_H_
#define _IFM_DEFS_H_

// ===================================================
//                COMPILE TIME SETTINGS
// ===================================================

//! Controls whether or not Panels with tabs are allowed to become tabs within other panels
#define IFM_ALLOW_NESTED_PANELS 0

// make cvs integration easier by using standard wxwidgets export methods
#ifdef WXMAKINGDLL_IFM
#    define WXDLLIMPEXP_IFM WXEXPORT
#elif defined(WXUSINGDLL)
#    define WXDLLIMPEXP_IFM WXIMPORT
#else /* not making nor using DLL */
#    define WXDLLIMPEXP_IFM
#endif

#include <wx/hashmap.h>
#include <wx/dynarray.h>

//#include <vector>
//#include <map>

#include <wx/app.h>

// DLL options compatibility check:
// this code is used when integrate with wxWidgets CVS
//WX_CHECK_BUILD_OPTIONS("wxIFM")

#define IFM_GET_EXTENSION_DATA(c, d)    (wxDynamicCast((c)->GetExtensionData(d::DataKey()), d))

/*
Some forward declares
*/
class wxIFMInterfacePluginBase;
class wxIFMExtensionPluginBase;
class wxIFMPluginEvent;
class wxInterfaceManager;
class wxIFMExtensionDataBase;
class wxIFMFloatingData;
class wxIFMComponent;
class wxIFMChildDataBase;
class wxIFMFloatingWindowBase;

//typedef std::vector<wxIFMInterfacePluginBase *> wxIFMInterfacePluginArray;
//typedef std::vector<wxIFMComponent *> wxIFMComponentArray;
//typedef std::vector<wxIFMFloatingWindowBase *> wxIFMFloatingWindowArray;
WX_DEFINE_ARRAY(wxIFMInterfacePluginBase*, wxIFMInterfacePluginArray);
WX_DEFINE_ARRAY(wxIFMComponent*, wxIFMComponentArray);
WX_DEFINE_ARRAY(wxIFMFloatingWindowBase*, wxIFMFloatingWindowArray);

/*
Data key stuff
*/
typedef int wxIFMComponentDataKeyType; //!< Data type used to retrieve component data
//typedef std::map<const wxIFMComponentDataKeyType, wxIFMExtensionDataBase *> wxIFMComponentDataMap;
WX_DECLARE_HASH_MAP(wxIFMComponentDataKeyType, wxIFMExtensionDataBase*, wxIntegerHash, wxIntegerEqual, wxIFMComponentDataMap);
#define DECLARE_IFM_DATA_KEY(name)  extern const wxIFMComponentDataKeyType name;
#define DEFINE_IFM_DATA_KEY(name)   const wxIFMComponentDataKeyType name = GetNewDataKey();

/*
Component type stuff
*/
#define DECLARE_IFM_COMPONENT_TYPE(name)    extern const int name;
#define DEFINE_IFM_COMPONENT_TYPE(name)     const int name = GetNewComponentType();
#define IFM_COMPONENT_UNDEFINED 0 // uninitialized component (error!)
#define IFM_COMPONENT_FLOATING_ROOT -1 //!< Special value used to create root components for floating windows

/*
Child types stuff
*/
class wxWindow;
//typedef std::map<const wxWindow*, wxIFMComponent*> wxIFMChildWindowMap;
WX_DECLARE_HASH_MAP(unsigned int, wxIFMComponent*, wxIntegerHash, wxIntegerEqual, wxIFMChildWindowMap);
#define DECLARE_IFM_CHILD_TYPE(name)    extern WXDLLIMPEXP_IFM const int name;
#define DEFINE_IFM_CHILD_TYPE(name)     const int name = GetNewChildType();
#define IFM_CHILDTYPE_UNDEFINED 0 // don't know (error!)
DECLARE_IFM_CHILD_TYPE(IFM_CHILD_GENERIC) // defined in manager.cpp
DECLARE_IFM_CHILD_TYPE(IFM_CHILD_TOOLBAR) // defined in manager.cpp

#define IFM_COORDS_ABSOLUTE     1 //!< The actual size of the component including everything
#define IFM_COORDS_BACKGROUND   2 /*!< \brief Area in which the component can draw its background.
                                       Excludes border widths*/
#define IFM_COORDS_CLIENT       3 /*!< \brief Area in which a component is allowed to place children.
                                       Excludes margin and border widths*/
#define IFM_COORDS_FLOATINGWINDOW 4 //!< \brief Size of the floating window that would contain this component.

#define IFM_ALIGN_NONE          0 //!< erronous allignment value
#define IFM_ALIGN_HORIZONTAL    1 //!< children of a component are layed out horizontally
#define IFM_ALIGN_VERTICAL      2 //!< children of a component are layed out vertically

/*
    Various flags
*/
#define IFM_NO_RESIZE_UPDATE    0x00000001 //!< don't update the interface in response to an EVT_SIZE event
#define IFM_DEFAULT_FLAGS       0

#define IFM_DISABLE_STATUS_MESSAGES -1 //!< used to disable the display of status messages in the frames status bar
#define IFM_USE_CURRENT_VALUE   -1 //!< used to specify that existing values should be used during SetRect events
#define IFM_NO_MAXIMUM          -2 //!< no maximum size for a given direction
#define IFM_NO_MINIMUM          -3 //!< no minimum size for a given direction
#define IFM_NO_MAXIMUM_SIZE     wxSize(IFM_NO_MAXIMUM,IFM_NO_MAXIMUM) //!< no maximum size
#define IFM_NO_MINIMUM_SIZE     wxSize(IFM_NO_MINIMUM,IFM_NO_MINIMUM) //!< no minimum size
#define IFM_USE_CHILD_MINSIZE   wxSize(-2,-2) //!< query the child for its min size with wxWindow::GetBestSize
#define IFM_USE_CURRENT_SIZE    wxSize(IFM_USE_CURRENT_VALUE,IFM_USE_CURRENT_VALUE)
#define IFM_DEFAULT_INDEX       -1
#define IFM_DEFAULT_POS         wxPoint(-1, -1)
#define IFM_DEFAULT_SIZE        wxSize(100, 100)
#define IFM_DEFAULT_RECT        wxRect(IFM_DEFAULT_POS, wxSize(-1,-1))
#define IFM_INTERFACE_ACTIVE    -1
#define IFM_INTERFACE_ALL       -2
#define IFM_DEFAULT_COMPONENT_NAME  wxT("")

// floating
#if defined(__WXMSW__) || defined(__WXGTK__) || defined(__WXMAC__)
#   define IFM_CANFLOAT    1 // floating supported
#else
#   define IFM_CANFLOAT    0 // no reparent yet
#endif

class wxIFMDimensions;

/*!
    Data structure used to store the widths of borders and margins for each side of a component
*/
class wxIFMDimensions
{
public:
    int top, bottom, left, right;

    wxIFMDimensions()
    {
        top =
        bottom =
        left =
        right = 0;
    }

    /*!
        Sets the dimension value for all sides.
        \param i Dimension value to set.
    */
    void Set(int i)
    {
        top =
        left =
        right =
        bottom = i;
    }

    wxIFMDimensions &operator =(const wxIFMDimensions &rect)
    {
        top = rect.top;
        bottom = rect.bottom;
        left = rect.left;
        right = rect.right;
        return *this;
    }
};

typedef wxIFMDimensions wxIFMBorders;
typedef wxIFMDimensions wxIFMMargins;

// Rect and Size object arrays needed for certain events
WX_DECLARE_EXPORTED_OBJARRAY(wxRect, wxRectArray);
WX_DECLARE_EXPORTED_OBJARRAY(wxSize, wxSizeArray);

#endif // _IFM_DEFS_H_
