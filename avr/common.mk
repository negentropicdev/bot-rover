ifneq ("$(_common_mk_)", "1")
#$(warning Including common functionality)
export _common_mk_=1

#functions to facilitate walking up directories
#$(call sl,$(PATHVAR)) - Strips trailing slash if exists
sl=$(patsubst %/,%,$1)
up=$(call sl,$(dir $1))

#srcroot:=$(call findsrcroot,$(CURDIR))
findsrcroot=$(if $(wildcard $1/common.mk),$1,$(if $1,$(call findsrcroot,$(call up,$1)),))

#find the root of the source tree
srcroot:=$(call findsrcroot,$(CURDIR))

$(if $(strip $(srcroot)),,$(error Unable to find root of source tree! Make sure you invoke make from within the source tree and common.mk is at the root))

include $(srcroot)/flags.mk
include $(srcroot)/commands.mk
include $(srcroot)/dirs.mk
include $(srcroot)/rules.mk

ifndef NO_AVR

#uncomment the following include to add support for AVR projects.
include $(srcroot)/avr.mk

endif

ifeq ("$(origin V)", "command line")
  VERBOSE=$(V)
endif
ifndef VERBOSE
  VERBOSE=0
endif

ifeq ($(VERBOSE),1)
  quiet:=
else
  export quiet:=@
  makeopts=--no-print-directory
endif

ifdef INCLUDE_DIR
  CC+= -I $(INCLUDE_DIR)
  CXX+= -I $(INCLUDE_DIR)
  export INCLUDE_DIR
endif

ifdef LIB_DIR
  CC+= -L $(LIB_DIR)
  CXX+= -L $(LIB_DIR)
  export LIB_DIR
endif

#find all Makefiles in subdirs of the invocation dir
makefiles:=$(shell find . -name Makefile)
subfiles:=$(filter %/Makefile,$(makefiles))

#$(call log,Found Makefiles- $(subfiles))

#recurse through the Makefiles adding their directories to VPATH
incmakes=$(foreach file,$(subfiles),$(CURDIR)/$(dir $(file)))

include $(foreach file,$(incmakes),$(file)Makefile)

VPATH+=$(LIB_DIR)/ $(BIN_DIR)/ $(BUILD_DIR)/ $(incmakes)

#$(call log,VPATH- $(VPATH))

endif
