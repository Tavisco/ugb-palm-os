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
#define MAX_ROM_FULL_PATH_LEN		BASEPATH_LENGTH+MAX_FILENAME_LENGTH
#define MAX_SAVE_FULL_PATH_LEN		MAX_ROM_FULL_PATH_LEN+SAVE_DIR_NAME_LENGTH+SAVE_EXTE_NAME_LENGTH

#define APP_CREATOR					'UGB_'
#define UGB_BASE_PATH				"/Palm/Programs/uGB/"
#define UGB_SAVE_DIR				"saves/"
#define UGB_SAVE_EXTENSION			".sav"
#define FTR_ROM_MEMORY				(UInt16)0

// From PalmSource
#define keyBitRockerUp				0x00010000	// 5-way rocker
#define keyBitRockerDown			0x00020000
#define keyBitRockerLeft			0x00040000
#define keyBitRockerRight			0x00080000
#define keyBitRockerCenter			0x00100000

#define RAM_SIZE		(128UL << 10)


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

	static inline void* innerGlobalsSlotVal(UInt8 slotID)	//[0] is reserved
	{
		return a5[slotID];
	}

	static inline void* globalsSlotVal(UInt8 slotID)	//[0] is reserved
	{
		if (!slotID || slotID > NUM_GLOBALS_SLOTS)
			return NULL;

		return innerGlobalsSlotVal(slotID);
	}

	#define GLOBALS_SLOT_ROMS_LIST			1
	#define GLOBALS_SLOT_ROM_FILE_NAME		2
	#define GLOBALS_SLOT_EXTRA_KEY_VALUE	3
#endif


// uGB.c
void *GetObjectPtr(UInt16 objectID);

// uGBRomSelector.c
Boolean RomSelectorFormHandleEvent(EventType *eventP);
Boolean RomSelectorDoCommand(UInt16 command);
void OpenAboutDialog(void);
UInt32 getExtraKeysCallback (void);
void perFrameCallback (void);

// uGBPlayer.c
Boolean PlayerDoCommand(UInt16 command);
Boolean PlayerFormHandleEvent(EventType *eventP);

// uGBKeyBinding.c
Boolean KeyBindingDoCommand(UInt16 command);
Boolean KeyBindingHandleEvent(EventType *eventP);

#endif /* UGB_H */