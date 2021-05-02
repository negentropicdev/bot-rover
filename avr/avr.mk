#NOTE: After changing the MCU or FCPU values you will need to run a realclean
# to ensure timing calculations are properly updated.

#AVR_MCU is used for specifying the controller for avr-gcc calls
AVR_MCU= atmega328p
AVR_FCPU= 16000000ul

#The AVRDUDE_* variables configure how avrdude is run
#Set the necessary values for your programmer
AVRDUDE_PART= m328p
AVRDUDE_PRG= C232HM
AVRDUDE_PORT= /dev/ttyUSB0
#AVRDUDE_BITC= 10
#AVRDUDE_BAUD 3750000

### C232HM pinout
# 1 SCK  Orange
# 2 MOSI Yellow
# 3 MISO Green
# 4 RST  Brown
# 5 VCC  Red
# 6 GND  Black

# [ 5  2  6 ]
# [ 3  1  4 ]

# [ RED YEL BLK ]
# [ GRN ORG BRN ]

#These flags override the normal flags and ensure a properly compile AVR hex
CXXFLAGS= -g -Os -Wall -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=$(AVR_MCU) -D F_CPU=$(AVR_FCPU)
CFLAGS= -g -Os -Wall -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=$(AVR_MCU) -D F_CPU=$(AVR_FCPU)
LDFLAGS= -Os -Wl,--gc-sections,--relax -mmcu=$(AVR_MCU) -lm
HEXFLAGS= -R .eeprom -O ihex


#Lines below this point should not be edited and ensure hex compiling and
#uploading work appropriately.

CLEANFILES+= *.elf 

#AVR project, establish CROSS_COMPILE variable for AVR-*
CROSS_COMPILE= avr-

#make BITC and BAUD parameters (-B & -b switches) optional
ifeq ($(AVRDUDE_BITC),)
	avrdude_bitc:=
else
	export avrdude_bitc:= -B $(AVRDUDE_BITC)
endif

ifeq ($(AVRDUDE_BAUD),)
	avrdude_baud:=
else
	export avrdude_baud:= -b $(AVRDUDE_BAUD)
endif

define hex
	@echo "    HEX $@"
	$(quiet)  $(LD) -mmcu=$(AVR_MCU) -o $(BUILD_DIR)/$(subst .hex,.elf,$@) $(foreach file,$(call local,$^),$(BUILD_DIR)/$(notdir $(file)))
	$(quiet)  $(HEX) $(BUILD_DIR)/$(subst .hex,.elf,$@) $(BUILD_DIR)/$@
	@mkdir -p $(BIN_DIR)
	$(mv) $(BUILD_DIR)/$@ $(BIN_DIR)
endef

define avrdude
	@echo "    UPLOAD $(notdir $^)"
	$(quiet) $(AVRDUDE) -P $(AVRDUDE_PORT) $(avrdude_bitc) $(avrdude_baud) -c $(AVRDUDE_PRG) -p $(AVRDUDE_PART) -U flash:w:$(BIN_DIR)/$(notdir $^)
endef

%.hex: %.o
	$(hex)

up-%: %
	$(avrdude)
