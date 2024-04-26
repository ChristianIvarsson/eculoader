PROC=powerpc

ifdef OS
TYPE=eabi
else
# You need the SPE capable compiler
TYPE=linux-gnuspe
endif

AS  = $(PROC)-$(TYPE)-as
CC  = $(PROC)-$(TYPE)-gcc
CPP = $(PROC)-$(TYPE)-cpp
AR  = $(PROC)-$(TYPE)-ar
LD  = $(PROC)-$(TYPE)-ld
NM  = $(PROC)-$(TYPE)-nm
OBJDUMP = $(PROC)-$(TYPE)-objdump
OBJCOPY = $(PROC)-$(TYPE)-objcopy
SIZE    = $(PROC)-$(TYPE)-size

ifdef OS
LZTOOL=../hostutils/maintolz77.exe
CSTOOL=../hostutils/bin2csfile.exe
else
LZTOOL=../hostutils/maintolz77
CSTOOL=../hostutils/bin2csfile
endif

-include ../.private/mpc5566_overrides.mk

# Where in ram should the loader be uploaded to
# e39 is able to fit an image just over 8 k starting from 0x40004000
# Would be better off at 0x40002000 but then you get a cranky e78 sooo.. may have to figure out something better
# Then there's e39a where gm's loader is at 0x40007000
# The compressed image is currently using this base + 0x4000 as its target
ifndef LOADER_BASE
LOADER_BASE=0x40004000
endif

ifndef MODFLAG
MODFLAG=
# MODFLAG += -DBAMMODE
# MODFLAG += -DenableDebugBox
# MODFLAG += -DdisableFlash
MODFLAG += -DenableBroadcast
endif

# The actual core, e200z6, is not available in gcc so a similar one had to be picked

ifdef OS
TARGPROC = -mcpu=ec603e -mspe -mno-toc -mbig-endian
else
TARGPROC = -mspe -mno-toc -mbig-endian
endif
GLOBFLAGS= $(MODFLAG) -DLDR_BASE=$(LOADER_BASE) -Dmpc5566 -DPOWERPC -I../sysfiles
ASFLAGS  = $(TARGPROC) $(GLOBFLAGS)
CFLAGS   = $(TARGPROC) $(GLOBFLAGS) -Os -Wall -Werror -Wpedantic -std=gnu99

.PHONY: all

all : clean \
	out/mainloader.bin \
	out/loader.bin

# # # # # # # # # # # # # # # #
# # # # # # # # # # # # # # # #
# Main loader
MAINOBJ= \
	out/mainloader.o \
	out/mainhelper.o \
	out/mainflash.o \
	out/gmlan.o \
	out/md5s.o \
	out/md5.o

mainmessage:
	@echo Building main loader..

# Linker script
out/mainloader.ld: linkerscripts/mainloader.in
	@$(CPP) -P $(GLOBFLAGS) $< $@

out/mainloader.o : mainloader/mainloader.c
	@$(CC) $(CFLAGS) -c $< -o $@

out/mainhelper.o : mainloader/mainhelper.S
	@$(CC) $(ASFLAGS) -c $< -o $@

out/mainflash.o : mainloader/mainflash.c
	@$(CC) $(CFLAGS) -c $< -o $@

out/gmlan.o : ../shared/gmlan.c
	@$(CC) $(CFLAGS) -c $< -o $@

out/md5s.o : ../shared/md5_ppc.S
	@$(CC) $(ASFLAGS) -c $< -o $@

out/md5.o : ../shared/md5.c
	@$(CC) $(CFLAGS) -c $< -o $@

out/mainloader.elf : mainmessage out/mainloader.ld $(MAINOBJ)
	@$(LD) -nostdlib -Tout/mainloader.ld -o $@ $(MAINOBJ)

# This is _NOT_ vanilla lz77. Do not use other tools than this one
out/mainloader.bin : out/mainloader.elf
	@$(OBJCOPY) -O binary  $< $@
	@$(LZTOOL) out/mainloader.bin out/mainloader.lz


# # # # # # # # # # # # # # # #
# # # # # # # # # # # # # # # #
# Preloader / packaged main loader
PREOBJ= \
	out/preloader.o \
	out/prehelper.o

premessage:
	@echo Building preloader..

# Linker script
out/preloader.ld: linkerscripts/preloader.in
	@$(CPP) -P $(GLOBFLAGS) $< $@

out/startup.o : startup.S
	@$(CC) $(ASFLAGS) -c $< -o $@

out/preloader.o : preloader/preloader.c
	@$(CC) $(CFLAGS) -c $< -o $@

out/prehelper.o : preloader/prehelper.S
	@$(CC) $(ASFLAGS) -c $< -o $@

# Also fetching addresses of symbols in the main binary
out/loader.elf : premessage out/preloader.ld out/startup.o $(PREOBJ)
	@$(LD) -nostdlib -R out/mainloader.elf -Tout/preloader.ld -o $@ $(PREOBJ)

out/loader.bin : out/loader.elf
	@$(OBJCOPY) -O binary  $< $@
	@rm -f Bootloader_mpc5566.cs
	@$(CSTOOL) out/loader.bin Bootloader_mpc5566
ifdef COPY_CS
	@echo "- Copying .cs file -"
	@cp Bootloader_mpc5566.cs ../../Trionic/TrionicCANLib/bootloaders/Bootloader_mpc5566.cs
endif
	@echo " "
	@echo " "
	@echo ___Main loader_________________
	@$(SIZE) -B out/mainloader.elf $(MAINOBJ)
	@echo " "
	@echo ___Pre loader__________________
	@$(SIZE) -B out/loader.elf $(PREOBJ)
	@echo " "
ifdef OS
	@./makehelper.bat
else
	@./makehelper.sh
endif
	@echo "Image base: "$(LOADER_BASE)

clean :
	@echo Cleaning..
	@rm -f out/*.o
	@rm -f out/*.ld
	@rm -f out/*.elf
	@rm -f out/*.bin
	@rm -f out/*.lz
