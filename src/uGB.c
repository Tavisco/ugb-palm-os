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
			FrmAlert (RomIncompatibleAlert);
		
			// Palm OS 1.0 will continuously relaunch this app unless we switch to 
			// another safe one.
			if (romVersion < OS2Version)
				AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
			
		}
		return sysErrRomIncompatible;
	}
	return errNone;
}

static void InitGlobals(void)
{
    UInt16 i;
	SharedVariables *sharedVars;

	sharedVars = (SharedVariables *)MemPtrNew(sizeof(SharedVariables));
	if (sharedVars == NULL) {
        SysFatalAlert("Not enough memory to make shared vars!");
    }
    
    // Allocate memory for the array of rom file names
    sharedVars->romFileName = (Char **)MemPtrNew(MAX_ROMS * sizeof(Char *));
    if (sharedVars->romFileName == NULL) {
        SysFatalAlert("Not enough heap!");
    }
    
    // Allocate memory for each rom file name
    for (i = 0; i < MAX_ROMS; i++) {
        sharedVars->romFileName[i] = (Char *)MemPtrNew(40 * sizeof(Char));
        if (sharedVars->romFileName[i] == NULL) {
            // Free all previously allocated memory and return an error code
            for (UInt16 j = 0; j < i; j++) {
                MemPtrFree(sharedVars->romFileName[j]);
            }
            MemPtrFree(sharedVars->romFileName);
            SysFatalAlert("Failed to allocate memory!");
        }
        MemSet(sharedVars->romFileName[i], 40, 0);
    }

	if (errNone != FtrSet(appFileCreator, ftrShrVarsNum, (UInt32)sharedVars))
	{
		SysFatalAlert("Failed to set sharedVars!");
	}
}

static void AppStop(void)
{
	SharedVariables *sharedVars;
	UInt32 ptrInt;
	UInt16 i;

	FtrGet(appFileCreator, ftrShrVarsNum, &ptrInt);
	sharedVars = (SharedVariables *)ptrInt;

	for (i=0; i < MAX_ROMS; i++)
	{
		MemPtrFree(sharedVars->romFileName[i]);
	}

	MemPtrFree(sharedVars->romFileName);
	MemPtrFree(sharedVars);
	FtrUnregister(appFileCreator, ftrShrVarsNum);
	FrmCloseAllForms();
}

UInt32 __attribute__((noinline)) PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	Err error;

	if (cmd == sysAppLaunchCmdNormalLaunch) {
		error = RomVersionCompatible(launchFlags);
		if (error) 
			return error; 

		InitGlobals();
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