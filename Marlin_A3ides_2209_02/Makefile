#-------------------------------------------------------------------------------
# Makefile to compile and link Marlin_A3ides_2209_02

PROJECT := Marlin_A3ides_2209_02
CONFIG  ?= Release_Boot
PRINTER ?= 2
FW_BUILDNR ?= 0

PRINTER_PACKFW := $(PRINTER)
ifeq ($(PRINTER_PACKFW), 4)
	PRINTER_PACKFW := 1
endif

ARDUINO     := 187
MOTHERBOARD := 1823

PWD := $(shell pwd | tr '\\\\' /)
OUT := $(CONFIG)
UTL ?= ${PWD}"/../Utils/hex2dfu"
SCR := ${PWD}"/../Scripts"

PREFIX ?= arm-none-eabi

GCC := $(PREFIX)-gcc
GPP := $(PREFIX)-g++
OBJCOPY := $(PREFIX)-objcopy
HEX2DFU := $(UTL)"/hex2dfu"
PACK_FW := $(SCR)"/pack_fw.py"
PRIVKEY ?= $(SCR)"/private.key"

SRCLIST := '.srclist'

ASMSRC := $(shell (cat $(SRCLIST) | grep ".s$$"))
GCCSRC := $(shell (cat $(SRCLIST) | grep ".c$$"))
GPPSRC := $(shell (cat $(SRCLIST) | grep ".cpp$$"))

ASMOBJ := $(addprefix $(OUT)/,$(ASMSRC:.s=.o))
GCCOBJ := $(addprefix $(OUT)/,$(GCCSRC:.c=.o))
GPPOBJ := $(addprefix $(OUT)/,$(GPPSRC:.cpp=.o))

OUTMAP := $(OUT)/$(PROJECT).map
OUTELF := $(OUT)/$(PROJECT).elf
OUTHEX := $(OUT)/$(PROJECT).hex
OUTBIN := $(OUT)/$(PROJECT).bin
OUTDFU := $(OUT)/$(PROJECT).dfu
OUTDIR := $(OUT)/.outdir


SYMBOLS := \
	-D__weak='__attribute__((weak))' \
	-D__packed='__attribute__((__packed__))' \
	-DSTM32F407xx \
	-DSTM32F4xx \
	-DHAVE_HWSERIAL2 \
	-DSTM32GENERIC \
	-DSTM32F4 \
	-DARDUINO=$(ARDUINO) \
	-DMOTHERBOARD=$(MOTHERBOARD) \
	-DUSE_HAL_DRIVER \
	-DUSBD_USE_CDC \
	-DUSBCON \
	-DLCDSIM \
	-DPRINTER_TYPE=$(PRINTER) \
	-DFW_BUILDNR=$(FW_BUILDNR)

ifneq (,$(findstring Debug, $(CONFIG)))
    SYMBOLS += -D_DEBUG
endif

ifneq (,$(findstring Boot, $(CONFIG)))
    SYMBOLS += -D_BOOTLOADER
endif


INCLUDES := \
	-I./Inc \
	-I./Drivers/STM32F4xx_HAL_Driver/Inc \
	-I./Drivers/STM32F4xx_HAL_Driver/Inc/Legacy \
	-I./Drivers/CMSIS/Device/ST/STM32F4xx/Include \
	-I./Drivers/CMSIS/Include \
	-I./Middlewares/Third_Party/FatFs/src \
	-I./Middlewares/Third_Party/FreeRTOS/Source/include \
	-I./Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS \
	-I./Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F \
	-I./Middlewares/ST/STM32_USB_Device_Library/Core/Inc \
	-I./Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc \
	-I./Middlewares/ST/STM32_USB_Host_Library/Core/Inc \
	-I./Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc \
	-I./Arduino_Core_A3ides/cores/arduino \
	-I./Arduino_Core_A3ides/cores/arduino/stm32 \
	-I./Arduino_Core_A3ides/variants/A3IDES_F407VET6_2209 \
	-I./Arduino_Core_A3ides/libraries/LiquidCrystal_I2C-1.1.2 \
	-I./Arduino_Core_A3ides/libraries/TMCStepper-0.3.3/src \
	-I./Middlewares/Third_Party/zlib \
	-I./Middlewares/Third_Party/lpng \
	-I./Gui \
	-I../GuiAPI/include \
	-I./Common


# common flags
CMNFLAGS := -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16

# compiler flags
ASMFLAGS := $(CMNFLAGS) $(SYMBOLS) -Wa,--no-warn -x assembler-with-cpp
GCCFLAGS := $(CMNFLAGS) -std=gnu11 $(SYMBOLS) $(INCLUDES) -ffunction-sections -fdata-sections -fstack-usage -Wall
GPPFLAGS := $(CMNFLAGS) -std=gnu++11 $(SYMBOLS) $(INCLUDES) -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions -fstack-usage -Wall -fno-threadsafe-statics

ifneq (,$(findstring Debug, $(CONFIG)))
	ASMFLAGS += -g
	GCCFLAGS += -O0 -g3
	GPPFLAGS += -O0 -g
else
	GCCFLAGS += -Os
	GPPFLAGS += -Os
endif

# linker script
ifneq (,$(findstring Boot, $(CONFIG)))
	LDSCRIPT := "./STM32F407VG_FLASH_boot.ld"
else
	LDSCRIPT := "./STM32F407VG_FLASH.ld"
endif

# linker flags
LDFLAGS  := $(CMNFLAGS) -T$(LDSCRIPT) -specs=nosys.specs -static -Wl,-cref,-u,Reset_Handler "-Wl,-Map=$(OUTMAP)" -Wl,--gc-sections -Wl,--defsym=malloc_getpagesize_P=0x1000 -Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group


build: dir elf hex bin dfu

dir: $(OUTDIR)

elf: $(OUTELF)

hex: $(OUTHEX)

bin: $(OUTBIN)

dfu: $(OUTDFU)

$(OUTDIR) :
	@echo 'creating output directory tree'
	@if [ ! -d $(OUT) ]; then mkdir $(OUT); fi
	@cat $(SRCLIST) | sed "s/\/[^\/]*$$//" | uniq > $(OUT)/.dirlist
	@cat $(OUT)/.dirlist | sed "s/^/$(OUT)\//" | xargs mkdir -p
	@echo >$(OUTDIR)

$(OUT)/%.o : %.s $(OUTDIR)
	@echo 'compiling' $<
	@$(GCC) -c $(ASMFLAGS) -o $@ $<

$(OUT)/%.o : %.c $(OUTDIR)
	@echo 'compiling' $<
	@$(GCC) -c $(GCCFLAGS) -o $@ $<

$(OUT)/%.o : %.cpp $(OUTDIR)
	@echo 'compiling' $<
	@$(GPP) -c $< $(GPPFLAGS) -o $@

$(OUTELF): $(ASMOBJ) $(GCCOBJ) $(GPPOBJ)
	@echo 'linking' $@
	@cat $(SRCLIST) | sed "s/\.\(s\|c\|cpp\)$$/\.o/;s/^/$(OUT)\//" > $(OUT)/.objlist
	@$(GPP) -o $(OUTELF) @$(OUT)/.objlist $(LDFLAGS)
#	@$(GPP) -o $(OUTELF) $(OUTOBJ) $(LDFLAGS)

$(OUTHEX): $(OUTELF)
	@echo 'generating hex file' $@
	@$(OBJCOPY) -O ihex $< $@

$(OUTBIN): $(OUTELF)
	@echo 'generating bin file' $@
	@$(OBJCOPY) -O binary $< $@

$(OUTDFU): $(OUTHEX)
	@echo 'generating dfu file' $@
	@$(HEX2DFU) -i $< -o $@

test:
	@echo 'PROJECT     = '$(PROJECT)
	@echo 'CONFIG      = '$(CONFIG)
	@echo 'PRINTER     = '$(PRINTER)
	@echo 'ARDUINO     = '$(ARDUINO)
	@echo 'MOTHERBOARD = '$(MOTHERBOARD)
	@echo 'PWD         = '$(PWD)
	@echo 'OUT         = '$(OUT)
	@echo 'GCC         = '$(GCC)
	@echo 'GPP         = '$(GPP)
	@echo 'OBJCOPY     = '$(OBJCOPY)
	@echo 'HEX2DFU     = '$(HEX2DFU)
	@echo 'PACK_FW     = '$(PACK_FW)
	@echo 'FW_BUILDNR  = '$(FW_BUILDNR)
	@echo 'OUTMAP      = '$(OUTMAP)
	@echo 'OUTELF      = '$(OUTELF)
	@echo 'OUTHEX      = '$(OUTHEX)
	@echo 'OUTBIN      = '$(OUTBIN)
	@echo 'OUTDIR      = '$(OUTDIR)
	@echo 'SYMBOLS     = '$(SYMBOLS)
	@echo 'LDSCRIPT    = '$(LDSCRIPT)
	@echo 'ASMSRC      = '$(ASMSRC)

.PHONY: clean

clean:
	@echo 'clean - removing output files'
	@rm -rfv $(OUT)
