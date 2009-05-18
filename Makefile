################################################################################
# wxIFM Makefile
# This script uses the multi-architecture build concept borrowed from the 
# website http://make.paulandlesley.org/multi-arch.html.
#
# Author: David Russak
#
# Version: $Id$
#
################################################################################

# This make script is first called from the main source directory and then one 
# directory level down in the build directory. The variable SRCDIR, defined in 
# target.mk and is passed in to the second invocation of this makefile and is 
# used to detect the current directory level. 

ifdef SRCDIR
  SOLUTION_DIR := ../..
else
  SOLUTION_DIR := ..
endif

SHARED_DIR := $(SOLUTION_DIR)/../Shared
MAKE_DIR := $(SHARED_DIR)/make
ifeq ($(SDL_EXTERN),)
  SDL_EXTERN := $(SOLUTION_DIR)/../sdlExtern
endif

include $(MAKE_DIR)/exports.mk

# OUTPUT_DIR is used in this file and in target.mk
OUTPUT_DIR := $(strip $(SOLUTION_DIR)/$(LIB_DIR))

ifeq (,$(filter _%,$(notdir $(CURDIR))))

include $(MAKE_DIR)/target.mk

else

  include $(OUTPUT_DIR)/locals.mk

  .SUFFIXES:

  # This is the portion of the script which will be run in the build 
  # directory. It uses VPATH to find the source files.

  INC := -I$(SOLUTION_DIR) $(WX_INCLUDES) -I$(SOLUTION_DIR)/wxIFM/include \
         -I$(SOLUTION_DIR)/wxIFM/src/ifm/xpm -I$(SDL_EXTERN)/include
  LIB := $(OUTPUT_DIR)/libWxIFM$(BLD_LTR).a
  VPATH := $(SRCDIR) $(SRCDIR)/src/ifm
  USER_SPECIALS := $(INC)

  CPP_SRC :=  definterface.cpp defplugin.cpp dragndrop.cpp events.cpp \
              manager.cpp plugin.cpp resize.cpp

  include $(MAKE_DIR)/obj_dep.mk

  # Create the library
  $(LIB): $(C_OBJS) $(CPP_OBJS)
	  @echo Creating library $(LIB).
	  $(AR) $(LIB) $(C_OBJS) $(CPP_OBJS)

  include $(MAKE_DIR)/rules.mk

endif
