#include <PalmOS.h>
#include <VFSMgr.h>
#include "uGB.h"
#include "UiResourceIDs.h"
#include "palmosArmData.h"
#include <PceNativeCall.h>
#include "gb.h"

#define RAM_SIZE		(128UL << 10)

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
	
	e = VFSFileOpen(vrn, "/game.sav", vfsModeWrite | vfsModeCreate | vfsModeTruncate, &fSave);
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

static Boolean gameFind(struct PalmosData *pd, UInt16 *cardVrnP, Int16 lstSelection)
{
	UInt32 volIter;
	FileRef fGame, fSave;
	Boolean ret = false;
	UInt16 vrn;
	Err e;
	Char **romFileName;
	
	
		//volIter = (UInt32 *)globalsSlotVal(GLOBALS_SLOT_VOL_ITER);
		romFileName = (Char **)globalsSlotVal(GLOBALS_SLOT_ROMS_LIST);

		Char *fileName = MemPtrNew(59); // should check for err
		StrCat(fileName, "/PALM/Programs/uGB/");
		StrCat(fileName, romFileName[lstSelection]);

		while (volIter != vfsIteratorStop && errNone == VFSVolumeEnumerate(&vrn, &volIter)) {

		// if (errNone != VFSVolumeEnumerate(&vrn, volIter))
		// {
		// 	SysFatalAlert("Failed to enumerate");
		// }

			e = VFSFileOpen(vrn, fileName, vfsModeRead, &fGame);
			if (e == errNone) { // This is probably where it's failing...
				
				UInt32 fSize, pos, now, chunkSz = 32768;
				
				if (errNone == VFSFileSize(fGame, &fSize)) {
					
					Boolean haveSave = false;
					void *rom, *ram;
					
					e = FtrPtrNew('____', '__', fSize, &rom);
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
					
					if (errNone == VFSFileOpen(vrn, "/game.sav", vfsModeRead, &fSave)) {
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
				}
				VFSFileClose(fGame);
			} else {
				SysFatalAlert("Failed to load file from VFS");
			}
		}
	
	return ret;
}

static void RomSelectorInit(FormType *frmP)
{
	UInt16 vrn, romCount, i;
	Char **romFileName;
	UInt32 volIter = vfsIteratorStart;
	Boolean actualVolIterSet = false;
	Err err = errNone;

	romCount = 0;

	romFileName = *globalsSlotPtr(GLOBALS_SLOT_ROMS_LIST);

	while (volIter != vfsIteratorStop && errNone == VFSVolumeEnumerate(&vrn, &volIter)) 
	{
		FileInfoType info; 
		FileRef dirRef; 
		UInt32 dirIterator; 
		Char *fileName = MemPtrNew(40); // should check for err 
		
		// open the directory first, to get the directory reference 
		err = VFSFileOpen(vrn, "/PALM/Programs/uGB", vfsModeRead, &dirRef); 
		if(err == errNone) { 
			info.nameP = fileName; // point to local buffer 
			info.nameBufLen = 40; 
			dirIterator = vfsIteratorStart;
			while (dirIterator != vfsIteratorStop) { 
				// Get the next file 
				err = VFSDirEntryEnumerate (dirRef, &dirIterator, &info); 
				if (err == errNone && info.attributes != vfsFileAttrDirectory) {
					StrCopy(romFileName[romCount], fileName);
					romCount++;
					if (!actualVolIterSet)
					{
						*globalsSlotPtr(GLOBALS_SLOT_VOL_ITER) = &volIter;
						actualVolIterSet = true;
					}
				} else { 
					// handle error, possibly by breaking out of the loop 
				}
			}
			VFSFileClose(dirRef);
		} else { 
			// handle directory open error here 
		}
		MemPtrFree(fileName); 
	}

	ListType *list = GetObjectPtr(RomSelectorList);

	LstSetListChoices(list, romFileName, romCount);
	LstSetSelection(list, -1);
	LstDrawList(list);
}

static void LaunchRom(void)
{
		UInt32 processorType, winMgrVer, prevDepth, desiredDepth = 16, screenPixelW, screenPixelH, screenStride;
		Int16 lstSelection;

		lstSelection = LstGetSelection(GetObjectPtr(RomSelectorList));

		if (noListSelection == lstSelection)
		{
			SysFatalAlert("Must select rom!");
		}
		
		if (errNone == FtrGet(sysFileCSystem, sysFtrNumProcessorID, &processorType) && 
				errNone == FtrGet(sysFtrCreator, sysFtrNumWinVersion, &winMgrVer) &&
				sysFtrNumProcessorIsARM(processorType) &&
				winMgrVer >= 4 &&
				errNone == WinScreenMode(winScreenModeGet, NULL, NULL, &prevDepth, NULL) &&
				errNone == WinScreenMode(winScreenModeSet, NULL, NULL, &desiredDepth, NULL)
			) {
				
			if (errNone == WinScreenGetAttribute(winScreenWidth, &screenPixelW) &&
					errNone == WinScreenGetAttribute(winScreenHeight, &screenPixelH) &&
					errNone == WinScreenGetAttribute(winScreenRowBytes, &screenStride) &&
					screenPixelW >= 160 && screenPixelH >= 144) {
				
				struct PalmosData *pd;
				UInt8 mult = 0;
				MemHandle mh;
				
				//find the multiple
				while (160 * mult <= screenPixelW && 144 * mult <= screenPixelH)
					mult++;
				
				pd = MemPtrNew(sizeof(struct PalmosData));
				if (pd) {
					
					UInt16 vrn;
					
					if (!gameFind(pd, &vrn, lstSelection))
						ErrAlertCustom(0, "canot find the game", NULL, NULL);
					else {
						
						UInt32 mask;
						
						pd->framebuffer = swapPtr(BmpGetBits(WinGetBitmap(WinGetDisplayWindow())));
						pd->framebufferStride = swap32(screenStride / sizeof(UInt16));
						pd->sizeMultiplier = mult - 1;
						pd->frameDithering = 3;
						
						//set up key map
						MemSet(pd->keyMapping, sizeof(pd->keyMapping), 0);
						pd->keyMapping[__builtin_ctzl(keyBitHard1)] = KEY_BIT_START;
						pd->keyMapping[__builtin_ctzl(keyBitHard2)] = KEY_BIT_SEL;
						pd->keyMapping[__builtin_ctzl(keyBitHard3)] = KEY_BIT_A;
						pd->keyMapping[__builtin_ctzl(keyBitHard4)] = KEY_BIT_B;
						pd->keyMapping[__builtin_ctzl(keyBitPageUp)] = KEY_BIT_UP;
						pd->keyMapping[__builtin_ctzl(vchrRockerUp)] = KEY_BIT_UP;
						pd->keyMapping[__builtin_ctzl(keyBitPageDown)] = KEY_BIT_DOWN;
						pd->keyMapping[__builtin_ctzl(vchrRockerDown)] = KEY_BIT_DOWN;
						pd->keyMapping[__builtin_ctzl(vchrRockerLeft)] = KEY_BIT_LEFT;
						pd->keyMapping[__builtin_ctzl(vchrRockerRight)] = KEY_BIT_RIGHT;

						mask = KeySetMask(0);

						if (!runRelocateableArmlet(MemHandleLock(mh = DmGet1Resource('ARMC', 0)), pd, NULL))
							ErrAlertCustom(0, "Failed to load and relocate the ARM code", NULL, NULL);
						
						KeySetMask(mask);
						
						MemHandleUnlock(mh);
						DmReleaseResource(mh);
					
						if (pd->ramSize && !gameSave(pd, vrn))
							ErrAlertCustom(0, "Failed to save the game state", NULL, NULL);
						
						MemChunkFree(swapPtr(pd->ramBuffer));
						FtrPtrFree('____', '__');
					}
					
					MemPtrFree(pd);
				}
			}
			(void)WinScreenMode(winScreenModeSet, NULL, NULL, &prevDepth, NULL);
		}
}

static Boolean RomSelectorDoCommand(UInt16 command)
{
	Boolean handled = false;

	switch (command)
	{
	case RomSelectorLaunchButton:
	{
		LaunchRom();
		handled = true;
		break;
	}

	default:
		break;
	}

	return handled;
}

Boolean RomSelectorFormHandleEvent(EventType * eventP)
{
	Boolean handled = false;
	FormType *frmP;

	switch (eventP->eType)
	{
	// case menuEvent:
	// 	return MainFormDoCommand(eventP->data.menu.itemID);

	case frmOpenEvent:
		frmP = FrmGetActiveForm();
		FrmDrawForm(frmP);
		RomSelectorInit(frmP);
		handled = true;
		break;

	case frmUpdateEvent:
		/*
		 * To do any custom drawing here, first call
		 * FrmDrawForm(), then do your drawing, and
		 * then set handled to true.
		 */
		break;

	case ctlSelectEvent:
	{
		return RomSelectorDoCommand(eventP->data.menu.itemID);
	}
	default:
			break;
	}

	return handled;
}