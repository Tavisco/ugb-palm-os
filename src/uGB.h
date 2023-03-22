/*
 * uGB.h
 *
 * header file for uGB Palm OS Wrapper
 * 
 */
#ifndef UGB_H
#define UGB_H

#ifndef __ARM__

	//globals (8 slots maximum, each stores a void*, zero-inited at app start)

	#define NUM_GLOBALS_SLOTS		8

	register void** a5 asm("a5");

	static inline void** globalsSlotPtr(UInt8 slotID)	//[0] is reserved
	{
		if (!slotID || slotID > NUM_GLOBALS_SLOTS)
			return NULL;

		return a5 + slotID;
	}

	static inline void* globalsSlotVal(UInt8 slotID)	//[0] is reserved
	{
		if (!slotID || slotID > NUM_GLOBALS_SLOTS)
			return NULL;

		return a5[slotID];
	}

	#define GLOBALS_SLOT_ROMS_LIST		1
	#define GLOBALS_SLOT_VOL_ITER		2

#endif

#define MAX_ROMS		64	// the maximum number of roms to exist

// uGB.c
void *GetObjectPtr(UInt16 objectID);

// uGBRomSelector.c
Boolean RomSelectorFormHandleEvent(EventType * eventP);

#endif /* UGB_H */