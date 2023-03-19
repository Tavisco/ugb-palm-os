#include <PalmOS.h>
#include "UiResourceIDs.h"
#include "uGB.h"

static Boolean AppHandleEvent(EventType * eventP)
{
	UInt16 formId;
	FormType * frmP;

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


UInt32 __attribute__((noinline)) PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	Err error;

	if (cmd == sysAppLaunchCmdNormalLaunch) {
		//error = AppStart();
		//if (error)
		//	return error;

		/*
		 * start application by opening the main form
		 * and then entering the main event loop
		 */
		FrmGotoForm(RomSelectorForm);
		AppEventLoop();

		//AppStop();
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