#include <PalmOS.h>
#include "../uGB.h"
#include "../UiResourceIDs.h"

static void saveValuesToPrefs(void)
{
	UInt16 latestPrefSize;
	struct UgbPrefs *prefs;

	latestPrefSize = sizeof(struct UgbPrefs);
	prefs = MemPtrNew(latestPrefSize);
	MemSet(prefs, latestPrefSize, 0);

	PrefGetAppPreferences(APP_CREATOR, PREFERENCES_ID, prefs, &latestPrefSize, true);

	prefs->showFPS = CtlGetValue(GetObjectPtr(DisplayOptionsShowFpsCheckbox));

	PrefSetAppPreferences(APP_CREATOR, PREFERENCES_ID, PREFERENCES_LAST_VER, prefs, latestPrefSize, true);
	MemPtrFree(prefs);
}

static void loadShowFpsCheckbox(FormPtr fp)
{
	UInt16 latestPrefSize;
	struct UgbPrefs *prefs;

	latestPrefSize = sizeof(struct UgbPrefs);
	prefs = MemPtrNew(latestPrefSize);
	MemSet(prefs, latestPrefSize, 0);

	PrefGetAppPreferences(APP_CREATOR, PREFERENCES_ID, prefs, &latestPrefSize, true);

	CtlSetValue(GetObjectPtr(DisplayOptionsShowFpsCheckbox), prefs->showFPS);

	MemPtrFree(prefs);
}

Boolean DisplayOptionsDoCommand(UInt16 command)
{
	Boolean handled = false;

	switch (command)
	{
	case DisplayOptionsOkBtn:
	{
		saveValuesToPrefs();
		FrmReturnToForm(0);
		handled = true;
		break;
	}

	default:
		break;
	}

	return handled;
}

Boolean DisplayOptionsHandleEvent(EventType *eventP)
{
	Boolean handled = false;
	FormPtr fp = FrmGetActiveForm();

	switch (eventP->eType)
	{
	case frmOpenEvent:
		FrmDrawForm(fp);
		loadShowFpsCheckbox(fp);
		handled = true;
		break;

	case ctlSelectEvent:
		return DisplayOptionsDoCommand(eventP->data.ctlSelect.controlID);

	default:
		break;
	}

	return handled;
}
