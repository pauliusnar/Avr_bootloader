# simple AVR Makefile
#
# written by michael cousins (http://github.com/mcous)
# released to the public domain

# Makefile
#
# targets:
#   all:    compiles the source code
#   test:   tests the isp connection to the mcu
#   flash:  writes compiled hex file to the mcu's flash memory
#   fuse:   writes the fuse bytes to the MCU
#   disasm: disassembles the code for debugging
#   clean:  removes all .hex, .elf, and .o files in the source code and library directories

# parameters (change this stuff accordingly)
# project name
PRJ = main
# avr mcu
MCU = atmega328p
# mcu clock frequency
CLK = 16000000
# avr programmer (and port if necessary)
# e.g. PRG = usbtiny -or- PRG = arduino -P /dev/tty.usbmodem411
PRG = atmelice_isp
# fuse values for avr: low, high, and extended
# these values are from an Arduino Uno (ATMega328P)
# see http://www.engbedded.com/fusecalc/ for other MCUs and options
LFU = 0xFF
HFU = 0xDE
EFU = 0xFD
# program source files (not including external libraries)
SRC = $(PRJ).c
# where to look for external libraries (consisting of .c/.cpp files and .h files)
# e.g. EXT = ../../EyeToSee ../../YouSART
EXT = ./src ./include ./lib
OUTPUT_DIR = ./output/

# read data from mcu 
# avrdude -p m328p -c atmelice_isp -C"C:\Program Files (x86)\Arduino\hardware\tools\avr\etc\avrdude.conf" -U flash:r:readflach.hex:i
#################################################################################################
# \/ stuff nobody needs to worry about until such time that worrying about it is appropriate \/ #
#################################################################################################



# include path
INCLUDE := $(foreach dir, $(EXT), -I$(dir))

# c flags
CFLAGS    = -Wall -Os -DF_CPU=$(CLK) -mmcu=$(MCU) $(INCLUDE)
# any aditional flags for c++
CPPFLAGS =

# AVRDUDE = avrdude -c $(PRG) -p $(MCU) -C "C:\\PauliusNarkevicius\\Atmega\\Arduino\\hardware\\tools\\avr\\etc\\avrdude.conf "
# CONFIG_FILE_PATH = "C:\Program Files (x86)\Arduino\hardware\tools\avr\etc\avrdude.conf"
CONFIG_FILE_PATH = "C:\PauliusNarkevicius\Atmega\Arduino\hardware\tools\avr\etc\avrdude.conf"


AVRDUDE = avrdude -c $(PRG) -p $(MCU) -C $(CONFIG_FILE_PATH)
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE    = avr-size --format=avr --mcu=$(MCU)
CC      = avr-gcc

# generate list of objects
# CFILES    = $(filter %.c, $(SRC))
EXTC     := $(foreach dir, $(EXT), $(wildcard $(dir)/*.c))
# CPPFILES  = $(filter %.cpp, $(SRC))
EXTCPP   := $(foreach dir, $(EXT), $(wildcard $(dir)/*.cpp))
OBJ       = $(EXTCPP:./src/%.cpp=$(OUTPUT_DIR)%.o) 
DEP = $(OBJ:%.o=%.d)


# Bootloader filepath
MERGE__TOOL_PATH = ".\bootloader\srecord-1.63-win32\srec_cat.exe"
BOOTLOADER_PATH = ".\bootloader\OWL\owl_20191203\targets\328p_16mhz_9600.hex"
MAIN_PATH = ".\output\main.hex"
MERGED_PROGRAM_PATH = ".\output\main_with_bootloader.hex"
COM_PORT = COM1
BAUD_RATE = 9600


# user targets
# compile all files
all: $(PRJ).hex 

PCB_preapere: fuse
	$(AVRDUDE) -U flash:w:$(BOOTLOADER_PATH):i 
	.\bootloader\OWL\owl_20191203\owl --targetname=328p_16mhz_9600.hex --flashfile=$(MAIN_PATH) --serialport=$(COM_PORT) --b$(BAUD_RATE)
	python -c "print('\7')"
	$(info -PCB was prepeared-)


upload_bootloader_with_program: all fuse
	$(info -Merging bootloader into program-)
	$(MERGE__TOOL_PATH) $(MAIN_PATH) -I $(BOOTLOADER_PATH) -I -o $(MERGED_PROGRAM_PATH) -I
	$(AVRDUDE) -U flash:w:$(MERGED_PROGRAM_PATH):i 
# $(AVRDUDE) -U flash:w:".\output\main_with_bootloader.hex":i -F 

# $(AVRDUDE) -U flash:w:$(OUTPUT_DIR)$(PRJ)_with_bootloader.hex:i -F 


upload_using_bootloader:  $(PRJ).hex
	$(info -Flashing using bootloader-)
	.\bootloader\OWL\owl_20191203\owl --targetname=328p_16mhz_9600.hex --flashfile=$(MAIN_PATH) --serialport=$(COM_PORT) --b$(BAUD_RATE)

# for optiboot
# avrdude -c arduino -p m328p -P COM18 -C $(CONFIG_FILE_PATH) -U flash:w:$(OUTPUT_DIR)$(PRJ).hex:i -b9600 -v 


upload_bootloader: fuse
	$(info -Flashing bootloader-)
	$(AVRDUDE) -U flash:w:$(BOOTLOADER_PATH):i -e 

test:
	$(info -----DEP = $(DEP))
# $(AVRDUDE) -v


# flash program to mcu
flash: all
	$(AVRDUDE) -U flash:w:$(OUTPUT_DIR)$(PRJ).hex:i -F 

# write fuses to mcu
fuse:
	$(AVRDUDE) -U lfuse:w:$(LFU):m -U hfuse:w:$(HFU):m -U efuse:w:$(EFU):m

# generate disassembly files for debugging
disasm: $(PRJ).elf
	$(OBJDUMP) -d $(PRJ).elf

# remove compiled files
clean:
	$(info -Clearing build-)
	-del -f *.o *.elf *.hex .\src\*.o .\output\**




# other targets
# objects from c files
# -include $(DEP)
DEL_DEP:
	$(info -----------------------deleting dep)
	-del -f .\output\*.d

.PHONY: obj
$(OBJ): $(OUTPUT_DIR)%.o: ./src/%.cpp DEL_DEP
	$(CC) $(CFLAGS) -MMD -c $< -o $@

# objects from c++ files
.cpp.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# elf file
$(PRJ).elf: $(OBJ)
	$(CC) $(CFLAGS) -o $(OUTPUT_DIR)$(PRJ).elf $(OBJ)

# hex file
$(PRJ).hex: $(PRJ).elf
	$(OBJCOPY) -j .text -j .data -O ihex $(OUTPUT_DIR)$(PRJ).elf $(OUTPUT_DIR)$(PRJ).hex
	$(SIZE) $(OUTPUT_DIR)$(PRJ).elf

build_bootloader:
# Create bootloader.o from bootloader.c
	avr-gcc -g -Wall -Os -fno-split-wide-types -mrelax -mmcu=atmega328p -DF_CPU=16000000 -c -o bootloader.o bootloader.c
# Create bootloader.elf from bootloader.o
	avr-gcc -g -Wall -Os -fno-split-wide-types -mrelax -mmcu=atmega328p -DF_CPU=16000000 -Wl,--section-start=.text=0x7000 -Wl,--relax -o bootloader.elf bootloader.o -lc
# Diassemble the code from .elf
	avr-objdump -h -S "bootloader.elf" > "bootloader.lss"
# Give symbol table
	avr-nm -n bootloader.elf > bootloader.sym
# Create .hex from .elf
	avr-objcopy  -O ihex -j .text -j .data bootloader.elf bootloader.hex 
# 
	avr-size -d --mcu=atmega328p bootloader.hex


write_bootloader: build_bootloader bootloader.c
	atprogram -t pickit4 -d atmega328p -i ISP -cl 4mhz -f program --verify -c -f bootloader.hex

write_fuse: 
# Write fuses LOW:HIGH:EXTENDED
	atprogram -t pickit4 -d atmega328p -i ISP  -f write -fs --values DFD0FE

read_program: 
	atprogram -t pickit4 -d atmega328p -i ISP -cl 4mhz -f read -fl -f read.hex

read_eeprom:
	atprogram -t pickit4 -d atmega328p -i ISP -cl 4mhz -f read -ee -f eeprom.hex


build_program: main.c
	avr-gcc -Wall -g -Os -DF_CPU=16000000 -mmcu=atmega328p -c main.c
	avr-gcc -Wall -g -Os -DF_CPU=16000000 -mmcu=atmega328p -o main.elf main.o
	avr-objcopy  -O ihex -j .text -j .data main.elf main.hex 
	avr-objcopy  -O binary -j .text -j .data main.elf main.bin 
	avr-objdump -h -S "main.elf" > "main.lss"
	avr-nm -n main.elf > main.sym
	avr-size -d --mcu=atmega328p main.hex

write_program:
	atprogram -t pickit4 -d atmega328p -i ISP -cl 4mhz -f program -f main.hex
		
extra: write_bootloader delay read_program
	avr-size -d --mcu=atmega328p bootloader.hex

delay:
	ping 127.0.0.1 -n 4 > nul
