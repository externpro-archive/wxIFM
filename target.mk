# <sdlOwner name='David Russak' email='david.russak@sdl.usu.edu' />
################################################################################
# This file and the multi-architecture build idea was borrowed from the website
# http://make.paulandlesley.org/multi-arch.html. Much of the commentary and code
# below is from the above source.
# This is script is used to jump from a source directory to a target directory.
#
# Author:
#   David Russak
#
# Revisions
#   ??Dec05 David Russak Created.
#   26Jan06 David Russak Added comments
#   06Mar06 David Russak Added rule to create locals.mk if needed. 
#
################################################################################

.SUFFIXES:

# Define the rules to build in the target subdirectories.
# The change directory command is given with the make command -C.
#
MAKETARGET = $(MAKE) --no-print-directory -C $@ -f $(CURDIR)/Makefile \
		SRCDIR=$(CURDIR) $(MAKECMDGOALS)

.PHONY: $(OBJDIR) make_locals clean subclean distclean
$(OBJDIR): make_locals
	+@[ -d $@ ] || mkdir -p $@
	+@$(MAKETARGET)

# Make the locals.mk make file if needed.
make_locals: $(BIN_PATH)/locals.mk

$(BIN_PATH)/locals.mk:
	$(MAKE) --no-print-directory -C ../ -f Makefile make_locals

# These rules keep make from trying to use the match-anything rule below to
# rebuild the makefiles--ouch!  Obviously, if you don't follow my convention
# of using a `.mk' suffix on all non-standard makefiles you'll need to change
# the pattern rule.
#
Makefile : ;
%.mk :: ;

# The clean rules are best handled from the source directory: since we're
# rigorous about keeping the target directories containing only target files
# and the source directory containing only source files, `clean' is as trivial
# as removing the target directories!
#

clean subclean distclean:
	rm -rf $(OBJDIR)

