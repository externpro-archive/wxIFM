################################################################################
# This script uses the multi-architecture build concept borrowed from the 
# website http://make.paulandlesley.org/multi-arch.html.
#
# Author:
#   David Russak
#
# Revisions
#   ??Dec05 David Russak Created.
#   26Jan06 David Russak Added comments. WR12069
#   06Mar06 David Russak Added rule to create locals.mk if needed. 
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

include $(SOLUTION_DIR)/make/exports.mk

# BIN_PATH is used in this file and in target.mk
BIN_PATH := $(strip $(SOLUTION_DIR)/$(BIN_DIR))

ifeq (,$(filter _%,$(notdir $(CURDIR))))

include target.mk

else

  include $(BIN_PATH)/locals.mk

  .SUFFIXES:

  # This is the portion of the script which will be run in the build 
  # directory. It uses VPATH to find the source files.

  INC := -I$(SOLUTION_DIR) $(WX_INCLUDES) -I$(SOLUTION_DIR)/wxIFM/include \
         -I$(SOLUTION_DIR)/wxIFM/src/ifm/xpm
  LIB := $(BIN_PATH)/libWxIFM.a
  VPATH := $(SRCDIR) $(SRCDIR)/src/ifm
  USER_SPECIALS := $(INC)

  CPP_SRC :=  definterface.cpp defplugin.cpp dragndrop.cpp events.cpp \
              manager.cpp plugin.cpp resize.cpp

  include $(SOLUTION_DIR)/make/obj_dep.mk

  # Create the library
  $(LIB): $(C_OBJS) $(CPP_OBJS)
	  @echo Creating library $(LIB).
	  $(AR) $(LIB) $(C_OBJS) $(CPP_OBJS)

  include $(SOLUTION_DIR)/make/rules.mk

endif
