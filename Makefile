ifdef SRCDIR
  SOLUTION_DIR := ../..
else
  SOLUTION_DIR := ..
endif

include $(SOLUTION_DIR)/make/exports.mk
BIN_PATH := $(SOLUTION_DIR)/$(BIN_DIR)
include $(strip $(BIN_PATH))/locals.mk

ifeq (,$(filter _%,$(notdir $(CURDIR))))

include target.mk

else
  .SUFFIXES:

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
