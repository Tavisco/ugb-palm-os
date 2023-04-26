#include <PalmOS.h>
#include <VFSMgr.h>
#include <PceNativeCall.h>
#include "../uGB.h"
#include "../UiResourceIDs.h"
#include "../palmosArmData.h"
#include "../gb.h"

static void SetStatusLabel(Char *status)
{
	FormPtr fp = FrmGetActiveForm();

	FrmHideObject(fp, FrmGetObjectIndex(fp, KeyBindingStatusLbl));
	FrmCopyLabel(fp, KeyBindingStatusLbl, status);
	FrmShowObject(fp, FrmGetObjectIndex(fp, KeyBindingStatusLbl));
}

static void ListenForKey(Int16 selection)
{
	UInt32 noKey, newKey, mask;
	UInt16 currentPrefSize, latestPrefSize;
	Int16 prefsVersion = noPreferenceFound;
	UgbKeyBindingPrefs *prefs;

	currentPrefSize = 0;
	latestPrefSize = sizeof(UgbKeyBindingPrefs);

	prefs = MemPtrNew(latestPrefSize);
	MemSet(prefs, latestPrefSize, 0);
	MemSet(prefs->keys, sizeof(prefs->keys), 0);

	prefsVersion = PrefGetAppPreferences(APP_CREATOR, KEYMAPPING_PREF_ID, NULL, &currentPrefSize, true);

	if (prefsVersion == noPreferenceFound){
		// If no preference is found, set virtualKeysOnly to true by default
		prefs->virtualKeysOnly = false;
	} else if (currentPrefSize != latestPrefSize) {
		// If the preference size is invalid, return an error
		SysFatalAlert("KeyMapping preferences is invalid!");
	} else {
		// Get the application preferences
		PrefGetAppPreferences(APP_CREATOR, KEYMAPPING_PREF_ID, &prefs, &latestPrefSize, true);
	}

	noKey = KeyCurrentState();
	newKey = noKey;

	mask = KeySetMask(0);

	SetStatusLabel("Press button NOW");
	while (noKey == newKey)
	{
		newKey = KeyCurrentState();
	}

	prefs->keys[selection] = newKey;

	PrefSetAppPreferences(APP_CREATOR, KEYMAPPING_PREF_ID, KEYMAPPING_PREF_LAST_VER, &prefs, latestPrefSize, true); 

	SetStatusLabel("Saved!\rReady to bind next\rcontrol.");
	KeySetMask(mask);
	MemPtrFree(prefs);
}

static void UnselectList(void)
{
	ListType *list;

	list = GetObjectPtr(KeyBindingList);
	LstSetSelection(list, noListSelection);
}

Boolean KeyBindingDoCommand(UInt16 command)
{
	Boolean handled = false;

	switch (command)
	{
	case KeyBindingOkBtn:
	{
		FrmReturnToForm(0);
		handled = true;
		break;
	}


	default:
		break;
	}

	return handled;
}

Boolean KeyBindingHandleEvent(EventType *eventP)
{
	Boolean handled = false;
	FormPtr fp = FrmGetActiveForm();

	switch (eventP->eType)
	{
	case frmOpenEvent:
		FrmDrawForm(fp);
		UnselectList();
		handled = true;
		break;

	case ctlSelectEvent:
		return KeyBindingDoCommand(eventP->data.ctlSelect.controlID);

	case lstSelectEvent:
		ListenForKey(eventP->data.lstSelect.selection);
		break;

	default:
		break;
	}

	return handled;
}
