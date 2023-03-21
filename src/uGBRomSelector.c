#include <PalmOS.h>
#include <VFSMgr.h>
#include "uGB.h"
#include "UiResourceIDs.h"

static void RomSelectorInit(FormType *frmP)
{
	UInt16 vrn, romCount, i;
	Char **romFileName;
	UInt32 volIter = vfsIteratorStart;
	Err err = errNone;

	romCount = 0;

	romFileName = *globalsSlotPtr(GLOBALS_SLOT_ROMS_LIST);

	while (volIter != vfsIteratorStop && errNone == VFSVolumeEnumerate(&vrn, &volIter)) 
	{
		FileInfoType info; 
		FileRef dirRef; 
		UInt32 dirIterator; 
		Char *fileName = MemPtrNew(40);  // should check for err 
		
		// open the directory first, to get the directory reference 
		// volRefNum must have already been defined 
		err = VFSFileOpen(vrn, "/PALM/Programs/uGB", vfsModeRead, &dirRef); 
		if(err == errNone) { 
			info.nameP = fileName;    // point to local buffer 
			info.nameBufLen = 40; 
			dirIterator = vfsIteratorStart;
			while (dirIterator != vfsIteratorStop) { 
				// Get the next file 
				err = VFSDirEntryEnumerate (dirRef, &dirIterator, &info); 
				if (err == errNone && info.attributes != vfsFileAttrDirectory) { 
					// Do something with the directory entry information 
					// Pull the attributes from info.attributes 
					// The file name is in fileName 
					StrCopy(romFileName[romCount], fileName);
					romCount++;
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
		// return MainFormDoCommand(eventP->data.menu.itemID);
	}
	default:
			break;
	}

	return handled;
}