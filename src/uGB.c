#include <PalmOS.h>
#include "UiResourceIDs.h"
#include "uGB.h"

void * GetObjectPtr(UInt16 objectID)
{
	FormType * frmP;
	UInt16 idx;

	frmP = FrmGetActiveForm();
	if (!frmP)
		return NULL;

	idx = FrmGetObjectIndex(frmP, objectID);
	if (idx == frmInvalidObjectId)
		return NULL;

	return FrmGetObjectPtr(frmP, idx);
}

static Boolean AppHandleEvent(EventType * eventP)
{
	UInt16 formId;
	FormType *frmP;

	if (eventP->eType == frmLoadEvent)
	{
		/* Load the form resource. */
		formId = eventP->data.frmLoad.formID;
		frmP = FrmInitForm(formId);
		FrmSetActiveForm(frmP);

		/*
		 * Set the event handler for the form.  The handler of the
		 * currently active form is called by FrmHandleEvent each
		 * time is receives an event.
		 */
		switch (formId)
		{
			case RomSelectorForm:
				FrmSetEventHandler(frmP, RomSelectorFormHandleEvent);
				break;

			case PlayerForm:
				FrmSetEventHandler(frmP, PlayerFormHandleEvent);
				break;

			case KeyBindingForm:
				FrmSetEventHandler(frmP, KeyBindingHandleEvent);
				break;

			case FrameSkippingForm:
				FrmSetEventHandler(frmP, FrameSkippingHandleEvent);
				break;

			case DisplayOptionsForm:
				FrmSetEventHandler(frmP, DisplayOptionsHandleEvent);
				break;
		}
		return true;
	}

	return false;
}

static void AppEventLoop(void)
{
	UInt16 error;
	EventType event;

	do
	{
		/* change timeout if you need periodic nilEvents */
		EvtGetEvent(&event, evtWaitForever);

		if (! SysHandleEvent(&event))
		{
			if (! MenuHandleEvent(0, &event, &error))
			{
				if (! AppHandleEvent(&event))
				{
					FrmDispatchEvent(&event);
				}
			}
		}
	} while (event.eType != appStopEvent);
}

static Err RomVersionCompatible(UInt16 launchFlags) {
   UInt32 romVersion;
   UInt32 requiredVersion = sysMakeROMVersion(3,0,0,sysROMStageRelease,0);
   UInt32 OS2Version = sysMakeROMVersion(2,0,0,sysROMStageRelease,0);

	// See if we're on the minimum required version of the ROM or later.
   FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	if (romVersion < requiredVersion) {
		if ((launchFlags & (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) {
			FrmAlert(PalmOSIncompatibleAlert);
		
			// Palm OS 1.0 will continuously relaunch this app unless we switch to 
			// another safe one.
			if (romVersion < OS2Version)
				AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
			
		}
		return sysErrRomIncompatible;
	}
	return errNone;
}

static Err DeviceCompatible(void) {
	UInt32 processorType, winMgrVer, screenPixelW, screenPixelH;

	if (
		errNone == FtrGet(sysFileCSystem, sysFtrNumProcessorID, &processorType) && 
		sysFtrNumProcessorIsARM(processorType) &&
		errNone == FtrGet(sysFtrCreator, sysFtrNumWinVersion, &winMgrVer) &&
		winMgrVer >= 4 &&
		errNone == WinScreenGetAttribute(winScreenWidth, &screenPixelW) &&
		errNone == WinScreenGetAttribute(winScreenHeight, &screenPixelH) &&
		screenPixelW >= 160 && screenPixelH >= 144
	)
	{
		return errNone;
	}

	FrmAlert(DeviceIncompatibleAlert);
	return sysErrRomIncompatible;
}

static void InitGlobals(void)
{
	Char **romFileNameList;
	UInt16 i;

    // Allocate memory for the array of rom file names
    romFileNameList = (Char **)MemPtrNew(MAX_ROMS * sizeof(Char *));
    if (romFileNameList == NULL) {
        SysFatalAlert("Not enough heap!");
    }
    
    // Allocate memory for each rom file name
    for (i = 0; i < MAX_ROMS; i++) {
        romFileNameList[i] = (Char *)MemPtrNew(MAX_FILENAME_LENGTH * sizeof(Char));
        if (romFileNameList[i] == NULL) {
            // Free all previously allocated memory and return an error code
            for (UInt16 j = 0; j < i; j++) {
                MemPtrFree(romFileNameList[j]);
            }
            MemPtrFree(romFileNameList);
            SysFatalAlert("Failed to allocate global memory!");
        }
        MemSet(romFileNameList[i], MAX_FILENAME_LENGTH, 0);
    }

	*globalsSlotPtr(GLOBALS_SLOT_ROMS_LIST) = romFileNameList;
}

static void AppStop(void)
{
	UInt16 i;
	Char **romFileNameList;
	Char *romFileName;

	// Cleanup ROMs file list
	romFileNameList = globalsSlotVal(GLOBALS_SLOT_ROMS_LIST);
	for (i=0; i < MAX_ROMS; i++)
	{
		MemPtrFree(romFileNameList[i]);
	}
	MemPtrFree(romFileNameList);

	// Cleanup save file name
	romFileName = globalsSlotVal(GLOBALS_SLOT_ROM_FILE_NAME);
	if (romFileName)
		MemPtrFree(romFileName);

	FrmCloseAllForms();
}

static void InitPreferences(void)
{
	Int16 prefsVersion = noPreferenceFound;
	struct UgbPrefs *prefs;
	UInt16 currentPrefSize, latestPrefSize;

	currentPrefSize = 0;
	latestPrefSize = sizeof(struct UgbPrefs);

	prefs = MemPtrNew(latestPrefSize);
	if (!prefs)
	{
		SysFatalAlert("Failed to allocate memory to UgbKeyBindingPrefs");
	}
	MemSet(prefs, latestPrefSize, 0);
	MemSet(prefs->keys, sizeof(prefs->keys), 0);

	prefsVersion = PrefGetAppPreferences(APP_CREATOR, PREFERENCES_ID, NULL, &currentPrefSize, true);

	if (prefsVersion == noPreferenceFound){
		// If no preference is found, set default values
		prefs->keyBinded = false;
		prefs->frameSkipping = DEFAULT_FRAME_SKIPPING_VALUE;

		PrefSetAppPreferences(APP_CREATOR, PREFERENCES_ID, PREFERENCES_LAST_VER, prefs, latestPrefSize, true);
	} else if (currentPrefSize != latestPrefSize) {
		if (prefsVersion != PREFERENCES_LAST_VER)
		{
			// App updated
			if (prefsVersion < 2)
			{
				prefs->frameSkipping = DEFAULT_FRAME_SKIPPING_VALUE;
				prefs->showFPS = false;
			}

			PrefSetAppPreferences(APP_CREATOR, PREFERENCES_ID, PREFERENCES_LAST_VER, prefs, latestPrefSize, true);
		} else {
			SysFatalAlert("KeyMapping preferences is corrupted!");
		}
	}

	MemPtrFree(prefs);
}

UInt32 __attribute__((noinline)) PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	Err error;

	if (cmd == sysAppLaunchCmdNormalLaunch) {
		error = RomVersionCompatible(launchFlags);
		if (error) 
			return error;

		error = DeviceCompatible();
		if (error)
		 	return error;

		InitGlobals();
		InitPreferences();
		FrmGotoForm(RomSelectorForm);
		AppEventLoop();

		AppStop();
	}

	return errNone;
}

UInt32 __attribute__((section(".vectors"), used)) __Startup__(void);
UInt32 __attribute__((section(".vectors"), used)) __Startup__(void)
{
	SysAppInfoPtr appInfoP;
	void *prevGlobalsP;
	void *globalsP;
	UInt32 ret;

	SysAppStartup(&appInfoP, &prevGlobalsP, &globalsP);
	ret = PilotMain(appInfoP->cmd, appInfoP->cmdPBP, appInfoP->launchFlags);
	SysAppExit(appInfoP, prevGlobalsP, globalsP);
	
	return ret;
}