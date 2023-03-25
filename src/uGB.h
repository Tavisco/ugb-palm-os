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
#define GLOBALS_SLOT_ROMS_LIST		1
#define APP_FILE_CREATOR			'UGB_'
#define ftrShrVarsNum				(UInt16)0
#define UGB_BASE_PATH				"/Palm/Programs/uGB/"


typedef struct SharedVariables
{
	Char **romFileName;
} SharedVariables;


// uGB.c
void *GetObjectPtr(UInt16 objectID);

// uGBRomSelector.c
Boolean RomSelectorFormHandleEvent(EventType * eventP);

#endif /* UGB_H */