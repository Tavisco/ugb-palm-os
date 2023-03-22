








static Boolean gameFind(struct PalmosData *pd, UInt16 *cardVrnP)
{
	UInt32 volIter = vfsIteratorStart;
	FileRef fGame, fSave;
	Boolean ret = false;
	UInt16 vrn;
	Err e;
	
	while (volIter != vfsIteratorStop && errNone == VFSVolumeEnumerate(&vrn, &volIter)) {
		
		e = VFSFileOpen(vrn, "/game.gb", vfsModeRead, &fGame);
		if (e == errNone) {
			
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
		}
	}
	
	return ret;
}


// UInt32 PilotMain(UInt16 cmd, void *cmdPBP, UInt16 flags)
// {
// 	if (!cmd) {
		

// 	}
// 	return 0;
// }
