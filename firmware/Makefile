
# Created from hid-data example. Original copyright below.

# Name: Makefile
# Project: hid-data example
# Author: Christian Starkjohann
# Creation Date: 2008-04-07
# Tabsize: 4
# Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
# License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
# This Revision: $Id: Makefile 692 2008-11-07 15:07:40Z cs $

DEVICE  = attiny2313

# in Hz
F_CPU   = 12000000
FUSE_L  = # see below for fuse values for particular devices
FUSE_H  =
AVRDUDE = avrdude -c usbasp -p $(DEVICE) # edit this line for your programmer

CFLAGS  = -Iusbdrv -I.
OBJECTS = usbdrv/usbdrv.o usbdrv/usbdrvasm.o usbdrv/oddebug.o main.o crc8.o

COMPILE = avr-gcc -Wall -Os -DF_CPU=$(F_CPU) $(CFLAGS) -mmcu=$(DEVICE)

all: main.bin main.hex

clean:
	rm -f test test.o main.bin main.lst main.obj main.cof main.list main.map main.eep.hex main.elf *.o usbdrv/*.o main.s usbdrv/oddebug.s usbdrv/usbdrv.s

# Generic rule for compiling C files:
.c.o:
	$(COMPILE) -c $< -o $@

# Generic rule for assembling Assembler source files:
.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

# Generic rule for compiling C to assembler, used for debugging only.
.c.s:
	$(COMPILE) -S $< -o $@

# file targets:

main.elf: $(OBJECTS)
	$(COMPILE) -o main.elf $(OBJECTS)

%.hex: %.elf
	avr-objcopy -j .text -j .data -O ihex $^ $@

%.bin: %.elf
	avr-objcopy -j .text -j .data -O binary $^ $@
