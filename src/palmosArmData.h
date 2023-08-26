#ifndef _PALMOS_ARM_DATA_H_
#define _PALMOS_ARM_DATA_H_

#include <stdint.h>

#define COLOR_MODE_RGB565		0		//16bpp color
#define COLOR_MODE_2BPP_DIRECT	1		//4 greys, no color mixing (only suitable for grey games in grey mode)
#define COLOR_MODE_4BPP_MIXED	2		//16 greys, color mixed (slower) - suitable for color games run in grey mode

//actual useful param
struct PalmosData {
	const void *rom;		//in
	uint32_t romSz;			//in
	void *ramBuffer;		//in: 128K
	uint32_t ramSize;		//out

	uint32_t (*getExtraKeysCallback)(void);
	void (*perFrameCallback)(void);

	void *framebuffer;
	uint32_t framebufferStride;		//IN BYTES (was in pixels)
	uint8_t sizeMultiplier;
	uint8_t frameDithering;			//display one every this many frames!

	uint8_t keyMapping[32];			//Nth byte is the key bit to give to GB (See gb.h) for Nth bit in palm's KeyCurrentState()

	uint8_t actAsOriginalGameboy;	//0 for gameboy color, 1  for gameboy original (non-color)
	uint8_t outputColorMode;		//COLOR_MODE_*
};

#define EXTRA_KEY_QUIT_EMU		0x80000000

//param passed to armlet directly
struct RelocatableArmletParams {
	void *actualParam;
	void *got;
	void *stack;
};

struct ArmletHeader {
	uint32_t __text_start;
	uint32_t __text_end;
	uint32_t __data_data;
	uint32_t __data_start;
	uint32_t __data_end;
	uint32_t __got_start;
	uint32_t __got_end;
	uint32_t __bss_start;
	uint32_t __bss_end;
	uint32_t __entry;
};

#endif
