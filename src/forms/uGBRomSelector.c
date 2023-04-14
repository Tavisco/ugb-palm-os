#include <PalmOS.h>
#include <VFSMgr.h>
#include <PceNativeCall.h>
#include "../uGB.h"
#include "../UiResourceIDs.h"
#include "../palmosArmData.h"
#include "../gb.h"


static void LoadPlayer(void)
{
	Int16 lstSelection;
	Char **romFileNameList = globalsSlotVal(GLOBALS_SLOT_ROMS_LIST);
	Char *romFileName = MemPtrNew(MAX_FILENAME_LENGTH);

	lstSelection = LstGetSelection(GetObjectPtr(RomSelectorList));

	if (noListSelection == lstSelection)
	{
		FrmAlert(MustSelectRomAlert);
		return;
	}

	MemSet(romFileName, MAX_FILENAME_LENGTH, 0);
	StrCopy(romFileName, romFileNameList[lstSelection]);

	*globalsSlotPtr(GLOBALS_SLOT_ROM_FILE_NAME) = romFileName;

	FrmGotoForm(PlayerForm);
}

static void RomSelectorInit(FormType *frmP)
{
	UInt16 vrn, romCount, i;
	UInt32 ptrInt;
	UInt32 volIter = vfsIteratorStart;
	Err err = errNone;
	Char **romFileNameList = globalsSlotVal(GLOBALS_SLOT_ROMS_LIST);

	romCount = 0;

	while (volIter != vfsIteratorStop && errNone == VFSVolumeEnumerate(&vrn, &volIter)) 
	{
		FileInfoType info; 
		FileRef dirRef; 
		UInt32 dirIterator; 
		Char *fileName = MemPtrNew(MAX_FILENAME_LENGTH); // should check for err 
		
		// open the directory first, to get the directory reference 
		err = VFSFileOpen(vrn, UGB_BASE_PATH, vfsModeRead, &dirRef); 
		if(err == errNone) { 
			info.nameP = fileName; // point to local buffer 
			info.nameBufLen = MAX_FILENAME_LENGTH; 
			dirIterator = vfsIteratorStart;
			while (dirIterator != vfsIteratorStop) { 
				// Get the next file 
				err = VFSDirEntryEnumerate (dirRef, &dirIterator, &info); 
				if (err == errNone && info.attributes != vfsFileAttrDirectory) {
					StrCopy(romFileNameList[romCount], fileName);
					romCount++;
					if (romCount == MAX_ROMS-1)
					{
						FrmAlert(RomQuntityLimitAlert);
						break;
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

	LstSetListChoices(list, romFileNameList, romCount);
	LstSetSelection(list, -1);
	LstDrawList(list);
}

static void InitForm(void)
{
	FormPtr fp = FrmGetActiveForm();
	FrmDrawForm(fp);
	RomSelectorInit(fp);
}

void OpenAboutDialog(void)
{
	FormType * frmP;

	/* Clear the menu status from the display */
	MenuEraseStatus(0);

	/* Display the About Box. */
	frmP = FrmInitForm (AboutForm);
	FrmDoDialog (frmP);
	FrmDeleteForm (frmP);
}

Boolean RomSelectorDoCommand(UInt16 command)
{
	Boolean handled = false;

	switch (command)
	{
	case RomSelectorLaunchButton:
		{
			LoadPlayer();
			handled = true;
			break;
		}
	case RomSelectorMenuItemAbout:
		{
			OpenAboutDialog();
			handled = true;
			break;
		}
	case RomSelectorMenuItemKeys:
		{
			FrmPopupForm(KeyBindingForm);
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
	

	switch (eventP->eType)
	{
		case frmOpenEvent:
			InitForm();
			handled = true;
			break;

		case menuEvent:
			return RomSelectorDoCommand(eventP->data.menu.itemID);

		case ctlSelectEvent:
			return RomSelectorDoCommand(eventP->data.menu.itemID);
		
		default:
				break;
	}

	return handled;
}