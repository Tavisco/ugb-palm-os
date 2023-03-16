APP	= uGB

TOOLCHAIN	?=	/home/tavisco/palm/palmdev_V3/buildtools/toolchain/bin

M68KCC		= $(TOOLCHAIN)/m68k-none-elf-gcc
M68KLD		= $(TOOLCHAIN)/m68k-none-elf-gcc
M68KOBJCOPY	= $(TOOLCHAIN)/m68k-none-elf-objcopy
M68KSDK		?=	/home/tavisco/palm/palmdev_V3/buildtools/palm-os-sdk-master/sdk-5r3/include
M68KRC		?=	/home/tavisco/palm/palmdev_V3/buildtools/pilrc3_3_unofficial/bin/pilrc

M68KLTO		=	-flto
M68KCOMMON	=	-Wno-multichar -funsafe-math-optimizations -Os -m68000 -mno-align-int -mpcrel -fpic -fshort-enums -mshort
M68KWARN	=	-Wsign-compare -Wextra -Wall -Werror -Wno-unused-parameter -Wno-old-style-declaration -Wno-unused-function -Wno-unused-variable -Wno-error=cpp
M68KLKR		=	linkerPalm68K.lkr
M68KCCFLAGS	=	$(M68KLTO) $(M68KWARN) $(M68KCOMMON) -I. -ffunction-sections -fdata-sections
M68KLDFLAGS	=	$(M68KLTO) $(M68KWARN) $(M68KCOMMON) -Wl,--gc-sections -Wl,-T $(M68KLKR)
RCP			=	rsc/uGB.rcp
CREATOR		=	uGB_
TYPE		=	appl

PALMOSSDK	+=	-isystem$(M68KSDK)
PALMOSSDK	+=	-isystem$(M68KSDK)/Core
PALMOSSDK	+=	-isystem$(M68KSDK)/Core/Hardware
PALMOSSDK	+=	-isystem$(M68KSDK)/Core/System
PALMOSSDK	+=	-isystem$(M68KSDK)/Core/UI
PALMOSSDK	+=	-isystem$(M68KSDK)/Dynamic
PALMOSSDK	+=	-isystem$(M68KSDK)/Extensions
PALMOSSDK	+=	-isystem$(M68KSDK)/Extensions/ExpansionMgr
PALMOSSDK	+=	-isystem$(M68KSDK)/Libraries
PALMOSSDK	+=	-isystem$(M68KSDK)/PalmSDK/Incs
PALMOSSDK	+=	-isystem$(M68KSDK)/PalmSDK/Incs/68k
PALMOSSDK	+=	-isystem$(M68KSDK)/PalmSDK/Incs/Common
PALMOSSDK	+=	-isystem$(M68KSDK)/PalmSDK/Incs/Common/system
M68KINCS	+=	-isystempalm68kgccisms $(PALMOSSDK)

M68KOBJS	=	palm68k.m68k.o

$(APP).prc:	 $(APP).bin $(APP).68k.bin
	$(M68KRC) -ro -o $(APP).prc -creator $(CREATOR) -type $(TYPE) -name $(APP) $(RCP) && rm uGB.68k.bin

%.68k.bin: %.68k.elf
	$(M68KOBJCOPY) -O binary $< $@ -j.text -j.rodata

$(APP).68k.elf: $(M68KOBJS)
	$(M68KLD) -o $@ $(M68KLDFLAGS) $^ -lgcc

%.m68k.o: uGB.c Makefile
	$(M68KCC) $(M68KCCFLAGS) $(M68KINCS) -c $< -o $@

clean:
	rm -f $(M68KOBJS) $(APP).68k.elf $(APP).68k.bin $(APP).prc

