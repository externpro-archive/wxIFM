set(lib_name ifm)
#######################################
# include directories
include_directories(
  include
  src/ifm/xpm
  )
#######################################
# library sources
########################
# docs
set(docs_srcs
  docs/building.dox
  docs/coordsystems.dox
  docs/internals.dox
  docs/main.dox
  docs/overview.dox
  )
source_group(docs FILES ${docs_srcs})
list(APPEND ${lib_name}_libsrcs ${docs_srcs})
########################
# headers
set(ifm_hdrs
  include/wx/ifm/definterface.h
  include/wx/ifm/defplugin.h
  include/wx/ifm/dragndrop.h
  include/wx/ifm/events.h
  include/wx/ifm/ifm.h
  include/wx/ifm/ifmdefs.h
  include/wx/ifm/manager.h
  include/wx/ifm/plugin.h
  include/wx/ifm/resize.h
  )
list(APPEND ${lib_name}_libsrcs ${ifm_hdrs})
########################
# source
set(ifm_srcs
  src/ifm/definterface.cpp
  src/ifm/defplugin.cpp
  src/ifm/dragndrop.cpp
  src/ifm/events.cpp
  src/ifm/manager.cpp
  src/ifm/plugin.cpp
  src/ifm/resize.cpp
  )
list(APPEND ${lib_name}_libsrcs ${ifm_srcs})
########################
# source
set(ifm_misc
  changelog.txt
  todo.txt
  )
list(APPEND ${lib_name}_libsrcs ${ifm_misc})
#######################################
# library
add_library(${lib_name} STATIC ${${lib_name}_libsrcs})
set_target_properties(${lib_name} PROPERTIES OUTPUT_NAME ${wxbasename}_${lib_name}-${wxversion})
#######################################
# install
install(TARGETS ${lib_name} DESTINATION lib${NUMBITS})
install(FILES ${ifm_hdrs} DESTINATION include/wx/ifm)
