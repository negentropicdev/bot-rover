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

#log outputs a line of text if not in quiet mode (outputs if V=1 is specified)
log=$(if $(quiet),,$(warning $1))

#creates relative paths to the current directory without a lead / or ./
local=$(subst $(projroot)/./,,$(subst ./,,$(subst $(projroot)/,,$1)))

depstarg=$(dir $1)

#These functions provide pretty output for build steps and optionally display
#the full command used in the build step.

#  Colors:
#Black:   30
#Red:     31
#Green:   32
#Yellow:  33
#Blue:    34
#Magenta: 35
#Cyan:    36
#White:   37
#Reset:   0

#The first $(CC) line turns C source files into object files
#The second $(CC) line generates dependency files
#TODO make dependency output in similar hierarchy as source files
#$(quiet) $(CC) -MM -MP -MT $(BUILD_DIR) $(filter-out %.h,$(call local,$^)) -MF $(BUILD_DIR)/$*.d
define cc
	@echo "\e[0;36m     CC\e[1;36m $(notdir $@)\e[0m"
	@mkdir -p $(BUILD_DIR)
	$(quiet)  $(CC) -MMD -MP -o $(BUILD_DIR)/$(subst .c,.o,$(word 1,$(filter-out %.h,$(notdir $^)))) -c $(word 1,$(filter-out %.a %.h,$(call local,$^)))
endef

#The first $(CXX) line turns C++ source files into object files. It exludes headers and archives from the source list
#The second $(CXX) line generates the dependency makefiles for the source file(s)
#$(quiet) $(CXX) -MM -MP -MT $(BUILD_DIR) $(filter-out %.h,$(call local,$^)) -MF $(BUILD_DIR)/$*.d
define cxx
	@echo "\e[0;36m    CXX\e[1;36m $(notdir $@)\e[0m"
	@mkdir -p $(BUILD_DIR)
	$(quiet)  $(CXX) -MMD -MP -o $(BUILD_DIR)/$(subst .cpp,.o,$(word 1,$(filter-out %.h,$(notdir $^)))) -c $(word 1,$(filter-out %.a %.h,$(call local,$^)))
endef

define ld
	@echo "\e[0;32m    LD \e[1;32m $@\e[0m"
	$(quiet)  $(LD) -o $(BUILD_DIR)/$@ $(foreach file,$(call local,$^),$(BUILD_DIR)/$(notdir $(file))) $(LDFLAGS)
	@mkdir -p $(BIN_DIR)
	$(mv) $(BUILD_DIR)/$@ $(BIN_DIR)
endef

define ar
	@echo "\e[0;33m    AR \e[1;33m $@\e[0m"
	$(quiet)  $(AR) $(BUILD_DIR)/$@ $(foreach file,$(call local,$^),$(BUILD_DIR)/$(notdir $(file)))
	@mkdir -p $(LIB_DIR)
	$(cp) $(BUILD_DIR)/$@ $(LIB_DIR)
endef
