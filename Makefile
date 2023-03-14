APP	= uGB


M68KCC		= m68k-none-elf-gcc
M68KLD		= m68k-none-elf-gcc
M68KOBJCOPY	= m68k-none-elf-objcopy
M68KSDK		?= /mnt/hgfs/S/My\ Documents/SDK/
M68KRC		= pilrc

M68KLTO		=	-flto
M68KCOMMON	=	-Wno-multichar -funsafe-math-optimizations -Os -m68000 -mno-align-int -mpcrel -fpic -fshort-enums -mshort
M68KWARN	=	-Wsign-compare -Wextra -Wall -Werror -Wno-unused-parameter -Wno-old-style-declaration -Wno-unused-function -Wno-unused-variable -Wno-error=cpp
M68KLKR		=	linkerPalm68K.lkr
M68KCCFLAGS	=	$(M68KLTO) $(M68KWARN) $(M68KCOMMON) -I. -ffunction-sections -fdata-sections
M68KLDFLAGS	=	$(M68KLTO) $(M68KWARN) $(M68KCOMMON) -Wl,--gc-sections -Wl,-T $(M68KLKR)

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
	 pilrc -ro -type appl -creator uGB_ -name uGB palm68kgccisms/gcc.rcp $@

%.68k.bin: %.68k.elf
	$(M68KOBJCOPY) -O binary $< $@ -j.text -j.rodata

$(APP).68k.elf: $(M68KOBJS)
	$(M68KLD) -o $@ $(M68KLDFLAGS) $^ -lgcc

%.m68k.o: %.c Makefile
	$(M68KCC) $(M68KCCFLAGS) $(M68KINCS) -c $< -o $@

clean:
	rm -f $(M68KOBJS) $(APP).68k.elf $(APP).68k.bin $(APP).prc

