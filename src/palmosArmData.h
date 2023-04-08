#ifndef _PALMOS_ARM_DATA_H_
#define _PALMOS_ARM_DATA_H_

#include <stdint.h>

//actual useful param
struct PalmosData {
	const void *rom;	//in
	uint32_t romSz;		//in
	void *ramBuffer;	//in: 128K
	uint32_t ramSize;	//out
	
	uint32_t (*getExtraKeysCallback)(void);
	void (*perFrameCallback)(void);
	
	uint16_t *framebuffer;
	uint32_t framebufferStride;			//IN PIXELS, not bytes!
	uint8_t sizeMultiplier;
	uint8_t frameDithering;				//display one every this many frames!
	
	uint8_t keyMapping[32];				//Nth byte is the key bit to give to GB (See gb.h) for Nth bit in palm's KeyCurrentState()
};


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
