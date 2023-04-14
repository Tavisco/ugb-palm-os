#include <PalmOS.h>
#include <VFSMgr.h>
#include "uGB.h"
#include "UiResourceIDs.h"
#include "palmosArmData.h"
#include <PceNativeCall.h>
#include "gb.h"


static UInt32 swap32(UInt32 val)
{
	return ((val >> 24) & 0xff) | ((val >> 8) & 0xff00) | ((val & 0xff00) << 8) | ((val & 0xff) << 24);
}

static void* swapPtr(void *ptr)
{
	return (void*)swap32((UInt32)ptr);
}

static Err gameSave(struct PalmosData *pd, UInt16 vrn)
{
	UInt32 now, saveSize = swap32(pd->ramSize);
	void *ram = swapPtr(pd->ramBuffer);
	FileRef fSave;
	Boolean ret;
	Err e;
	Char *fileName = globalsSlotVal(GLOBALS_SLOT_PATH_ROM_FILE);
	Char *saveFileName = MemPtrNew(BASEPATH_LENGTH+SAVE_DIR_NAME_LENGTH+MAX_FILENAME_LENGTH+SAVE_EXTE_NAME_LENGTH);

	MemSet(saveFileName, BASEPATH_LENGTH+SAVE_DIR_NAME_LENGTH+MAX_FILENAME_LENGTH+SAVE_EXTE_NAME_LENGTH, 0);

	StrCopy(saveFileName, fileName);
	StrCat(saveFileName, UGB_SAVE_EXTENSION);

	if (!saveFileName)
		SysFatalAlert("Failed to determine save file name!");
	
	e = VFSFileOpen(vrn, saveFileName, vfsModeWrite | vfsModeCreate | vfsModeTruncate, &fSave);
	if (e != errNone)
		return false;

	ret = errNone == VFSFileWrite(fSave, saveSize, ram, &now) && now == saveSize;
	ret = errNone == VFSFileClose(fSave) && ret;
	
	return ret;
}

static Boolean runRelocateableArmlet(const struct ArmletHeader *hdr, void *param, UInt32 *retValP)
{
	UInt8 space[sizeof(struct RelocatableArmletParams) + sizeof(UInt32) - 1];
	const UInt32 textMin = swap32(hdr->__text_start), textMax = swap32(hdr->__text_end), dataMin = swap32(hdr->__data_start), dataMax = swap32(hdr->__bss_end), stackSz = 4096;
	struct RelocatableArmletParams *p = (struct RelocatableArmletParams*)(((UInt32)space + sizeof(UInt32) - 1) / sizeof(UInt32) * sizeof(UInt32));
	UInt32 ramSize = swap32(hdr->__bss_end) - swap32(hdr->__data_start), *gotStart, *gotEnd, retVal;
	char *ram = MemChunkNew(0, ramSize, 0x1200), *stack = MemPtrNew(stackSz), *entry;
	
	if (!ram || !stack) {
		if (ram)
			MemChunkFree(ram);
		if (stack)
			MemChunkFree(stack);
	}
	
	MemMove(ram - swap32(hdr->__data_start) + swap32(hdr->__data_start), ((char*)hdr) - swap32(hdr->__text_start) + swap32(hdr->__data_data), swap32(hdr->__data_end) - swap32(hdr->__data_start));
	MemSet(ram - swap32(hdr->__data_start) + swap32(hdr->__bss_start), swap32(hdr->__bss_end) - swap32(hdr->__bss_start), 0);
	
	gotStart = (uint32_t*)(ram - swap32(hdr->__data_start) + swap32(hdr->__got_start));
	gotEnd = (uint32_t*)(ram - swap32(hdr->__data_start) + swap32(hdr->__got_end));
	
	while (gotStart < gotEnd) {
		uint32_t word = swap32(*gotStart);
		
		if (word >= textMin && word < textMax)
			word = word - textMin + (UInt32)hdr;
		else if (word >= dataMin && word < dataMax)
			word = word - dataMin + (UInt32)ram;
		else if (word) {
			MemChunkFree(ram);
			MemChunkFree(stack);
			return false;
		}
		
		*gotStart++ = swap32(word);
	}
	
	p->actualParam = swapPtr(param);
	p->got = swapPtr(ram - swap32(hdr->__data_start) + swap32(hdr->__got_start));
	p->stack = swapPtr(stack + stackSz);
	entry = ((char*)hdr) - swap32(hdr->__text_start) + swap32(hdr->__entry);
	
	retVal = PceNativeCall((NativeFuncType*)entry, p);
	
	MemChunkFree(stack);
	MemChunkFree(ram);
	
	if (retValP)
		*retValP = retVal;
	
	return true;
}

static Boolean loadROMIntoMemory(struct PalmosData *pd, UInt16 *cardVrnP)
{
	FileRef fGame, fSave;
	UInt32 ptrInt;
	UInt16 vrn;
	Err e;
	UInt32 volIter = vfsIteratorStart;
	Boolean ret = false;
	Char *fileName = globalsSlotVal(GLOBALS_SLOT_PATH_ROM_FILE);
	Char *saveFileName = MemPtrNew(BASEPATH_LENGTH+SAVE_DIR_NAME_LENGTH+MAX_FILENAME_LENGTH+SAVE_EXTE_NAME_LENGTH);
	UInt32 fSize, pos, now, chunkSz = 32768;
	Boolean haveSave = false;

	MemSet(saveFileName, BASEPATH_LENGTH+SAVE_DIR_NAME_LENGTH+MAX_FILENAME_LENGTH+SAVE_EXTE_NAME_LENGTH, 0);

	StrCopy(saveFileName, fileName);
	StrCat(saveFileName, UGB_SAVE_EXTENSION);

	while (volIter != vfsIteratorStop && errNone == VFSVolumeEnumerate(&vrn, &volIter)) {
		e = VFSFileOpen(vrn, fileName, vfsModeRead, &fGame);
		if (e == errNone) {
			if (errNone == VFSFileSize(fGame, &fSize)) {
				
				void *rom, *ram;
				
				e = FtrPtrNew(APP_CREATOR, FTR_ROM_MEMORY, fSize, &rom);
				if (e != errNone)
					SysFatalAlert("Cannot alloc rom");
				
				for (pos = 0; pos < fSize; pos += now) {
					now = fSize - pos;
					if (now > chunkSz)
						now = chunkSz;
					
					e = VFSFileReadData(fGame, now, rom, pos, &now);
					if (e != errNone && e != vfsErrFileEOF)
						SysFatalAlert("read error");
				}
				
				pd->rom = swapPtr(rom);
				pd->romSz = swap32(fSize);
				pd->ramBuffer = swapPtr(ram = MemChunkNew(0, RAM_SIZE, 0x1200));
				pd->ramSize = swap32(RAM_SIZE);
				if (!pd->ramBuffer)
					SysFatalAlert("Failed to alloc ram");
				
				if (errNone == VFSFileOpen(vrn, saveFileName, vfsModeRead, &fSave)) {
					if (errNone == VFSFileSize(fSave, &fSize) && fSize < RAM_SIZE) {
						e = VFSFileRead(fSave, fSize, ram, &now);
						haveSave = (e == errNone || e == vfsErrFileEOF) && now == fSize;
					}
					VFSFileClose(fSave);
				}
				
				if (!haveSave)
					MemSet(ram, RAM_SIZE, 0xff);
				
				if (cardVrnP)
					*cardVrnP = vrn;
				
				ret = true;
			} else {
				SysFatalAlert("VFSFileSize failed");
			}
			VFSFileClose(fGame);
		}
	}
	
	return ret;
}

UInt32 getExtraKeysCallback (void)
{
    return (UInt32)globalsSlotVal(GLOBALS_SLOT_EXTRA_KEY_VALUE);
}

void perFrameCallback (void)
{
	EventType event;

	EvtGetEvent(&event, 0);
	FrmDispatchEvent(&event);
}

static void StartEmulation(void)
{
		UInt32 screenPixelW, screenPixelH, screenStride;
		struct PalmosData *pd;
		UInt8 mult = 0;
		MemHandle mh;
		UInt16 vrn;
		UInt32 mask;

		if (errNone == WinScreenGetAttribute(winScreenWidth, &screenPixelW) &&
				errNone == WinScreenGetAttribute(winScreenHeight, &screenPixelH) &&
				errNone == WinScreenGetAttribute(winScreenRowBytes, &screenStride) &&
				screenPixelW >= 160 && screenPixelH >= 144) {
			
			//find the multiple
			while (160 * mult <= screenPixelW && 144 * mult <= screenPixelH)
				mult++;
			
			pd = MemPtrNew(sizeof(struct PalmosData));
			if (pd) {
				if (!loadROMIntoMemory(pd, &vrn))
					SysFatalAlert("Cannot load selected game into memory!");
				else {
					pd->framebuffer = swapPtr(BmpGetBits(WinGetBitmap(WinGetDisplayWindow())));
					pd->framebufferStride = swap32(screenStride / sizeof(UInt16));
					pd->sizeMultiplier = mult - 1;
					pd->frameDithering = 3;
					pd->getExtraKeysCallback = swapPtr(&getExtraKeysCallback);
					pd->perFrameCallback = swapPtr(&perFrameCallback);
					
					//set up key map
					MemSet(pd->keyMapping, sizeof(pd->keyMapping), 0);
					pd->keyMapping[__builtin_ctzl(keyBitHard1)] = KEY_BIT_START;
					pd->keyMapping[__builtin_ctzl(keyBitHard2)] = KEY_BIT_SEL;
					pd->keyMapping[__builtin_ctzl(keyBitHard4)] = KEY_BIT_A;
					pd->keyMapping[__builtin_ctzl(keyBitHard3)] = KEY_BIT_B;
					pd->keyMapping[__builtin_ctzl(keyBitPageUp)] = KEY_BIT_UP;
					pd->keyMapping[__builtin_ctzl(keyBitRockerUp)] = KEY_BIT_UP;
					pd->keyMapping[__builtin_ctzl(keyBitPageDown)] = KEY_BIT_DOWN;
					pd->keyMapping[__builtin_ctzl(keyBitRockerDown)] = KEY_BIT_DOWN;
					pd->keyMapping[__builtin_ctzl(keyBitRockerLeft)] = KEY_BIT_LEFT;
					pd->keyMapping[__builtin_ctzl(keyBitRockerRight)] = KEY_BIT_RIGHT;

					mask = KeySetMask(0);

					// Set the value of the GLOBALS_SLOT_EXTRA_KEY_VALUE slot to the address of the allocated memory
					*globalsSlotPtr(GLOBALS_SLOT_EXTRA_KEY_VALUE) = 0;

					if (!runRelocateableArmlet(MemHandleLock(mh = DmGet1Resource('ARMC', 0)), pd, NULL))
						SysFatalAlert("Failed to load and relocate the ARM code");
						
					KeySetMask(mask);
					
					MemHandleUnlock(mh);
					DmReleaseResource(mh);
				
					if (pd->ramSize && !gameSave(pd, vrn))
						FrmAlert(FailedToSaveAlert);
					
					MemChunkFree(swapPtr(pd->ramBuffer));
					FtrPtrFree(APP_CREATOR, FTR_ROM_MEMORY);
				}
				
				MemPtrFree(pd);
			}
		} else {
			FrmAlert(ResolutionTooLowAlert);
		}

		// (void)WinScreenMode(winScreenModeSet, NULL, NULL, &prevDepth, NULL);
		//InitForm();
}

Boolean PlayerDoCommand(UInt16 command)
{
	Boolean handled = false;

	switch (command)
	{
	case PlayerFormStopButton:
		{
			*globalsSlotPtr(GLOBALS_SLOT_EXTRA_KEY_VALUE) = (void *)EXTRA_KEY_QUIT_EMU;
			ErrAlertCustom(0, "Global set", NULL, NULL);
			handled = true;
			break;
		}

	default:
		break;
	}

	return handled;
}

Boolean PlayerFormHandleEvent(EventType *eventP)
{
	Boolean handled = false;
	FormPtr fp = FrmGetActiveForm();

	switch (eventP->eType)
	{
		case frmOpenEvent:
			FrmDrawForm(fp);
			StartEmulation();
			handled = true;
			break;

		case ctlSelectEvent:
			return PlayerDoCommand(eventP->data.ctlSelect.controlID);
		
		default:
				break;
	}

	return handled;
}