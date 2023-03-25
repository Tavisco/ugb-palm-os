/*
 * uGB.h
 *
 * header file for uGB Palm OS Wrapper
 * 
 */
#ifndef UGB_H
#define UGB_H

#define MAX_ROMS					64	// the maximum number of roms to exist
#define MAX_FILENAME_LENGTH			256	// the maximum filename length allowed
#define BASEPATH_LENGTH				20
#define SAVE_DIR_NAME_LENGTH		7
#define SAVE_EXTE_NAME_LENGTH		7
#define GLOBALS_SLOT_ROMS_LIST		1
#define APP_CREATOR					'UGB_'
#define UGB_BASE_PATH				"/Palm/Programs/uGB/"
#define UGB_SAVE_DIR				"saves/"
#define UGB_SAVE_EXTENSION			".sav"
#define FTR_ROM_MEMORY				(UInt16)0


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

	#define GLOBALS_SLOT_ROMS_LIST			1
	#define GLOBALS_SLOT_ROM_SAVENAME		2

#endif


// uGB.c
void *GetObjectPtr(UInt16 objectID);

// uGBRomSelector.c
Boolean RomSelectorFormHandleEvent(EventType * eventP);

#endif /* UGB_H */