#include <PalmOS.h>
#include <VFSMgr.h>
#include <PceNativeCall.h>
#include "../uGB.h"
#include "../UiResourceIDs.h"
#include "../palmosArmData.h"
#include "../gb.h"

#define BPP_2	2
#define	BPP_4	4
#define BPP_16	16

enum GameColorMode
{
	NON_COLOR_GAME = 0,
	COLOR_GAME = 1,
	COLOR_GAME_REQUIRE_COLOR = 3
};

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
	Char *romFileName = globalsSlotVal(GLOBALS_SLOT_ROM_FILE_NAME);
	Char *saveFileFullPath = MemPtrNew(MAX_SAVE_FULL_PATH_LEN);

	MemSet(saveFileFullPath, MAX_SAVE_FULL_PATH_LEN, 0);
	StrPrintF(saveFileFullPath, "%s%s%s%s", UGB_BASE_PATH, UGB_SAVE_DIR, romFileName, UGB_SAVE_EXTENSION);

	if (!saveFileFullPath)
		SysFatalAlert("Failed to determine save file path and name!");
	
	e = VFSFileOpen(vrn, saveFileFullPath, vfsModeWrite | vfsModeCreate | vfsModeTruncate, &fSave);
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

static Boolean determineScreenModeForRom(struct PalmosData *pd, void *rom)
{
	UInt8 gbcFlag = *((UInt8*)rom + 0x143);
	Err error = errNone;
	UInt32 depths;
	Boolean deviceSupportColors = false;
	enum GameColorMode gameColorMode;

	if ((gbcFlag & 0x80) == 0)
	{
		// gameboy non-color game
		gameColorMode = NON_COLOR_GAME;
	}
	else if ((gbcFlag & 0x40) == 0)
	{
		// gameboy color game that can run without color
		gameColorMode = COLOR_GAME;
	}
	else
	{
		// gameboy color game that requires color
		gameColorMode = COLOR_GAME_REQUIRE_COLOR;
	}

	// Get all screen modes supported by the device
	error = WinScreenMode(winScreenModeGetSupportedDepths, NULL, NULL, &depths, NULL);
	if (error != errNone)
	{
		SysFatalAlert("Failed to get supported screen depths");
	}

	// if 16bpp supported, emulate game boy color and run in color always
	if (depths & (1UL << (BPP_16 - 1)))
	{
		pd->actAsOriginalGameboy = 0;
		pd->outputColorMode = COLOR_MODE_RGB565;
		return true;
	}

	// if 4bpp and 2bpp both supported: for all games that do NOT REQUIRE color, run in 2bpp. all games that REQUIRE color, run in 4bpp and downmix
	if ((depths & (1UL << (BPP_4 - 1))) && (depths & (1UL << (BPP_2 - 1))))
	{
		if (gameColorMode == COLOR_GAME_REQUIRE_COLOR)
		{
			// set to 4 bpp
			pd->actAsOriginalGameboy = 0;
			pd->outputColorMode = COLOR_MODE_4BPP_MIXED;
			return true;
		}
		else
		{
			// set to 2bpp
			pd->actAsOriginalGameboy = 1;
			pd->outputColorMode = COLOR_MODE_2BPP_DIRECT;
			return true;
		}
	}

	// if 4bpp is supported but 2bpp is not (VERY UNLIKELY): all games run as gameboy color and downmix.
	if (depths & (1UL << (BPP_4 - 1)) && !(depths & (1UL << (BPP_2 - 1))))
	{
		// set to 4 bpp
		pd->actAsOriginalGameboy = 0;
		pd->outputColorMode = COLOR_MODE_4BPP_MIXED;
		return true;
	}

	// if 2bpp is supported but 4bpp is not: run in 2bpp mode. refuse to run color-requiring games (tell user that 4bpp is required)
	if (depths & (1UL << (BPP_2 - 1)) && !(depths & (1UL << (BPP_4 - 1))))
	{
		if (gameColorMode == COLOR_GAME_REQUIRE_COLOR)
		{
			// refuse to run color-requiring games
			FrmAlert(ColorGameRequireColorAlert);
			FrmGotoForm(RomSelectorForm);
			return false;
		}
		else
		{
			// set to 2bpp
			pd->actAsOriginalGameboy = 1;
			pd->outputColorMode = COLOR_MODE_2BPP_DIRECT;
			return false;
		}
	}

	// if only 1bpp is supported - refuse to run anything, tell user that 2bpp is minimum
	FrmAlert(OneBppOnlyAlert);
	FrmGotoForm(RomSelectorForm);
	return false;
}

static Boolean loadROMIntoMemory(struct PalmosData *pd, UInt16 *cardVrnP)
{
	FileRef fGame, fSave;
	UInt32 ptrInt;
	UInt16 vrn;
	Err e;
	UInt32 volIter = vfsIteratorStart;
	Boolean ret = false;
	Char *romFileName = globalsSlotVal(GLOBALS_SLOT_ROM_FILE_NAME);
	Char *romFileFullPath = MemPtrNew(MAX_ROM_FULL_PATH_LEN);
	Char *saveFileFullPath = MemPtrNew(MAX_SAVE_FULL_PATH_LEN);
	UInt32 fSize, pos, now, chunkSz = 32768;
	Boolean haveSave = false;

	// Clear variables
	MemSet(romFileFullPath, MAX_ROM_FULL_PATH_LEN, 0);
	MemSet(saveFileFullPath, MAX_SAVE_FULL_PATH_LEN, 0);

	// Prepare romFile path
	StrPrintF(romFileFullPath, "%s%s", UGB_BASE_PATH, romFileName);

	// Prepare saveFile path
	StrPrintF(saveFileFullPath, "%s%s%s%s", UGB_BASE_PATH, UGB_SAVE_DIR, romFileName, UGB_SAVE_EXTENSION);

	while (volIter != vfsIteratorStop && errNone == VFSVolumeEnumerate(&vrn, &volIter)) {
		e = VFSFileOpen(vrn, romFileFullPath, vfsModeRead, &fGame);
		if (e == errNone) {
			if (errNone == VFSFileSize(fGame, &fSize)) {
				void *rom, *ram;

				e = FtrPtrNew(APP_CREATOR, FTR_ROM_MEMORY, fSize, &rom);
				if (e != errNone)
					SysFatalAlert("Failed to allocate memory for rom. Try to free some internal memory and restart the emulator.");
				
				for (pos = 0; pos < fSize; pos += now) {
					now = fSize - pos;
					if (now > chunkSz)
						now = chunkSz;
					
					e = VFSFileReadData(fGame, now, rom, pos, &now);
					switch (e)
					{
					case errNone:
					case vfsErrFileEOF:
						break;
					case expErrNotOpen:
						SysFatalAlert("Failed to read ROM file: Slot driver library has not been opened");
					case vfsErrFileBadRef:
						SysFatalAlert("Failed to read ROM file: The fileref is invalid. The rom name contains non-latin characters?");
					case vfsErrFilePermissionDenied:
						SysFatalAlert("Failed to read ROM file: The file is read only");
					case vfsErrIsADirectory:
						SysFatalAlert("Failed to read ROM file: This operation requires a regular file, not a directory");
					case vfsErrNoFileSystem:
						SysFatalAlert("Failed to read ROM file: No installed filesystem supports this operation");
					default:
						SysFatalAlert("Failed to read ROM file: Unknown error");
						break;
					}
				}
				if (!determineScreenModeForRom(pd, rom))
					return false;
				pd->rom = swapPtr(rom);
				pd->romSz = swap32(fSize);
				pd->ramBuffer = swapPtr(ram = MemChunkNew(0, RAM_SIZE, 0x1200));
				pd->ramSize = swap32(RAM_SIZE);
				if (!pd->ramBuffer)
					SysFatalAlert("Failed to alloc ram");
				
				if (errNone == VFSFileOpen(vrn, saveFileFullPath, vfsModeRead, &fSave)) {
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

				VFSFileClose(fGame);
				return true;
			} else {
				SysFatalAlert("VFSFileSize failed");
			}
		} else if (e != vfsErrFileNotFound) {
			SysFatalAlert("VFSFileOpen failed");
		}
	}
	SysFatalAlert("Failed to open ROM file");
	return false;
}

static void setScreenModeForRom(uint8_t outputColorMode)
{
	UInt32 desiredDepth;
	switch (outputColorMode)
	{
	case COLOR_MODE_RGB565:
		desiredDepth = BPP_16;
		break;
	case COLOR_MODE_4BPP_MIXED:
		desiredDepth = BPP_4;
		break;
	case COLOR_MODE_2BPP_DIRECT:
		desiredDepth = BPP_2;
		break;
	default:
		SysFatalAlert("Unknown color mode");
		break;
	}

	if (errNone != WinScreenMode(winScreenModeSet, NULL, NULL, &desiredDepth, NULL))
	{
		SysFatalAlert("Failed to set screen mode");
	}
}

static void CalcAverageTick(struct RuntimeVars *runtimeVars)
{
	UInt32 newtick = TimGetTicks();
	UInt32 oldestTick = runtimeVars->ticklist[runtimeVars->tickindex];  

	runtimeVars->ticksum = runtimeVars->ticksum - oldestTick + newtick;
	runtimeVars->ticklist[runtimeVars->tickindex] = newtick; 

	if (++runtimeVars->tickindex == FPS_CALC_MAX_SAMPLES)
		runtimeVars->tickindex = 0;

	UInt32 fps = (FPS_CALC_MAX_SAMPLES * 100) / (newtick - oldestTick) * (runtimeVars->frameSkipping + 1);

	Char *fpsStr;
	fpsStr = (Char *)MemPtrNew(maxStrIToALen);
	MemSet(fpsStr, maxStrIToALen, 0);

	StrIToA(fpsStr, fps);
	Int16 fpsStrSize = (fps > 99) ? 3 : ((fps > 9) ? 2 : 1);

	WinDrawChars(fpsStr, fpsStrSize, 52, 147); 
}

// This is used to pass on-screen buttons to the emulator core
UInt32 getExtraKeysCallback (void)
{
	return (UInt32)globalsSlotVal(GLOBALS_SLOT_EXTRA_KEY_VALUE);
}

void perFrameCallback (void)
{
	struct RuntimeVars *runtimeVars;
	EventType event;

	runtimeVars = globalsSlotVal(GLOBALS_SLOT_RUNTIME_VARS);

	if (runtimeVars->showFPS)
		CalcAverageTick(runtimeVars);

	EvtGetEvent(&event, 0);
	FrmDispatchEvent(&event);

	if (!runtimeVars->formDrawn) {
		FormPtr fp = FrmGetActiveForm();
		
		FrmDrawForm(fp);
		runtimeVars->formDrawn = 1;

		if (!runtimeVars->showFPS)
			FrmHideObject(fp,  FrmGetObjectIndex(fp, PlayerFormFpsLabel));
	}
}

static void StartEmulation(void)
{
	UInt32 screenPixelW, screenPixelH, screenStride;
	UInt8 mult = 0;
	MemHandle mh;
	UInt16 vrn, latestPrefSize, initDelay, old_initDelay, old_initDblDelay, period, old_period;
	UInt32 mask, old_mask;
	Boolean queueAhead,old_queueAhead;
	struct PalmosData *pd;
	struct UgbPrefs *prefs;
	struct RuntimeVars *runtimeVars; 
	static int keyBits[] = {KEY_BIT_LEFT, KEY_BIT_UP, KEY_BIT_RIGHT, KEY_BIT_DOWN, KEY_BIT_SEL, KEY_BIT_START, KEY_BIT_B, KEY_BIT_A};

	if (errNone == WinScreenGetAttribute(winScreenWidth, &screenPixelW) &&
			errNone == WinScreenGetAttribute(winScreenHeight, &screenPixelH) &&
			screenPixelW >= 160 && screenPixelH >= 144) {
		
		//find the multiple
		while (160 * mult <= screenPixelW && 144 * mult <= screenPixelH)
			mult++;
		
		pd = MemPtrNew(sizeof(struct PalmosData));
		if (pd) {
			if (loadROMIntoMemory(pd, &vrn))
			{
				latestPrefSize = sizeof(struct UgbPrefs);

				prefs = MemPtrNew(latestPrefSize);
				if (!prefs)
				{
					SysFatalAlert("Failed to allocate memory to preferences");
				}
				MemSet(prefs, latestPrefSize, 0);
				MemSet(prefs->keys, sizeof(prefs->keys), 0);

				// Get the application preferences
				PrefGetAppPreferences(APP_CREATOR, PREFERENCES_ID, prefs, &latestPrefSize, true);

				setScreenModeForRom(pd->outputColorMode);

				if (errNone != WinScreenGetAttribute(winScreenRowBytes, &screenStride))
					SysFatalAlert("Failed to get screen stride");

				pd->framebuffer = swapPtr(BmpGetBits(WinGetBitmap(WinGetDisplayWindow())));
				pd->framebufferStride = swap32(screenStride);
				pd->sizeMultiplier = mult - 1;
				pd->frameDithering = prefs->frameSkipping + 1;
				pd->getExtraKeysCallback = swapPtr(&getExtraKeysCallback);
				pd->perFrameCallback = swapPtr(&perFrameCallback);

				MemSet(pd->keyMapping, sizeof(pd->keyMapping), 0);

				for (int i = 0; i < 8; ++i) {
					if (prefs->keys[i] != 0) {
						pd->keyMapping[__builtin_ctzl(prefs->keys[i])] = keyBits[i];
					}
				}

				mask = KeySetMask(0);

				initDelay = slowestKeyDelayRate;
				period = slowestKeyPeriodRate;
				queueAhead = false;

				KeyRates(false, &old_initDelay ,&old_period, &old_initDblDelay, &old_queueAhead);
				KeyRates(true, &initDelay, &period, &initDelay, &queueAhead);

				// Set the value of the GLOBALS_SLOT_EXTRA_KEY_VALUE slot to the address of the allocated memory
				*globalsSlotPtr(GLOBALS_SLOT_EXTRA_KEY_VALUE) = 0;

				// Initialize the runtimeVars
				runtimeVars = MemPtrNew(sizeof(struct RuntimeVars));
				MemSet(runtimeVars, sizeof(struct RuntimeVars), 0);

				runtimeVars->formDrawn = false;
				runtimeVars->tickindex = 0;
				runtimeVars->ticksum = 0;
				for (UInt8 i = 0; i < FPS_CALC_MAX_SAMPLES; i++)
				{
					runtimeVars->ticklist[i] = 0;
				}
				runtimeVars->frameSkipping = prefs->frameSkipping;
				runtimeVars->showFPS = prefs->showFPS;

				*globalsSlotPtr(GLOBALS_SLOT_RUNTIME_VARS) = runtimeVars;

				if (!runRelocateableArmlet(MemHandleLock(mh = DmGet1Resource('ARMC', 0)), pd, NULL))
					SysFatalAlert("Failed to load and relocate the ARM code");
					
				KeySetMask(mask);
				KeyRates(true,&old_initDelay,&old_period,&old_initDblDelay,&old_queueAhead);
				
				MemHandleUnlock(mh);
				DmReleaseResource(mh);
			
				WinDrawChars("Writing save file...", 20, 36, 147);

				if (pd->ramSize && !gameSave(pd, vrn))
					FrmAlert(FailedToSaveAlert);
				
				MemChunkFree(swapPtr(pd->ramBuffer));
				MemPtrFree(prefs);
				if (errNone != WinScreenMode(winScreenModeSetToDefaults, NULL, NULL, NULL, NULL))
				{
					SysFatalAlert("Failed to set screen mode");
				}
			}
			MemPtrFree(pd);
		}
	} else {
		FrmAlert(ResolutionTooLowAlert);
	}
}

Boolean PlayerDoCommand(UInt16 command)
{
	Boolean handled = false;

	switch (command)
	{
	case PlayerFormStopButton:
		{
			*globalsSlotPtr(GLOBALS_SLOT_EXTRA_KEY_VALUE) = (void *)EXTRA_KEY_QUIT_EMU;
			FrmGotoForm(RomSelectorForm);
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
			WinDrawChars("Loading ROM...", 14, 0, 0);
			StartEmulation();
			handled = true;
			break;

		case ctlSelectEvent:
			return PlayerDoCommand(eventP->data.ctlSelect.controlID);

		case frmCloseEvent:
			FtrPtrFree(APP_CREATOR, FTR_ROM_MEMORY);
			break;

		default:
				break;
	}

	return handled;
}