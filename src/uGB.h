/*
 * uGB.h
 *
 * header file for uGB Palm OS Wrapper
 * 
 */
#ifndef UGB_H
#define UGB_H

#define MAX_ROMS		16	// the maximum number of roms to exist
#define GLOBALS_SLOT_ROMS_LIST		1

#define appFileCreator 'UGB_'
#define ftrShrVarsNum (UInt16)0

typedef struct SharedVariables
{
	Char **romFileName;
} SharedVariables;


// uGB.c
void *GetObjectPtr(UInt16 objectID);

// uGBRomSelector.c
Boolean RomSelectorFormHandleEvent(EventType * eventP);

#endif /* UGB_H */