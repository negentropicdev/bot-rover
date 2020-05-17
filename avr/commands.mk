#The following capitalized variables store the command-line commands to perform various functions
#These can be edited to customize how/which commands are called.

CC=$(CROSS_COMPILE)gcc $(CFLAGS)
CXX=$(CROSS_COMPILE)g++ $(CXXFLAGS)
LD=$(CROSS_COMPILE)g++
AR=$(CROSS_COMPILE)ar $(ARFLAGS)
HEX=$(CROSS_COMPILE)objcopy $(HEXFLAGS)
AVRDUDE=avrdude
RM=rm -rf
CP=cp -r
MV=mv
MKDIR=mkdir -p

#These lower cased commands wrap the above calls to control verbosity of the
#output. In general, none of the lower cased variables or commands should be
#changed.

rm=$(quiet)$(RM)
cp=$(quiet)$(CP)
mv=$(quiet)$(MV)
mkdir=$(quiet)$(MKDIR)

makeopts=
make=@$(MAKE) $(makeopts)

log=$(if $(quiet),,$(warning $1))
local=$(subst $(CURDIR)/./,,$(subst ./,,$(subst $(CURDIR)/,,$1)))

#These functions provide pretty output for build steps and optionally display
#the full command used in the build step.

define cc
	@echo "    CC $(notdir $@)"
	@mkdir -p $(BUILD_DIR)
	$(quiet)  $(CC) -o $(BUILD_DIR)/$(subst .c,.o,$(word 1,$(filter-out %.h,$(notdir $^)))) -c $(word 1,$(filter-out %.a %.h,$(call local,$^)))
	@echo "    DEP $@"
	$(quiet) $(CC) -MM -MP -MT $(BUILD_DIR) $(filter-out %.h,$(call local,$^)) -MF $(BUILD_DIR)/$*.d
endef

define cxx
	@echo "    CXX $(notdir $@)"
	@mkdir -p $(BUILD_DIR)
	$(quiet)  $(CXX) -o $(BUILD_DIR)/$(subst .cpp,.o,$(word 1,$(filter-out %.h,$(notdir $^)))) -c $(word 1,$(filter-out %.a %.h,$(call local,$^)))
	@echo "    DEP $@"
	$(quiet) $(CXX) -MM -MP -MT $(BUILD_DIR) $(filter-out %.h,$(call local,$^)) -MF $(BUILD_DIR)/$*.d
endef

define ld
	@echo "    LD $@"
	$(quiet)  $(LD) -o $(BUILD_DIR)/$@ $(foreach file,$(call local,$^),$(BUILD_DIR)/$(notdir $(file))) $(LDFLAGS)
	@mkdir -p $(BIN_DIR)
	$(mv) $(BUILD_DIR)/$@ $(BIN_DIR)
endef

define ar
	@echo "    AR $@"
	$(quiet)  $(AR) $(BUILD_DIR)/$@ $(foreach file,$(call local,$^),$(BUILD_DIR)/$(notdir $(file)))
	@mkdir -p $(LIB_DIR)
	$(cp) $(BUILD_DIR)/$@ $(LIB_DIR)
endef
