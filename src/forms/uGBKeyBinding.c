#include <PalmOS.h>
#include "../uGB.h"
#include "../UiResourceIDs.h"

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
	UInt16 currentPrefSize, latestPrefSize, abortBtnIndex, okBtnIndex;
	Int16 prefsVersion = noPreferenceFound;
	EventType event;
	FormType *frm;
	struct UgbPrefs *prefs;

	if (selection < 0 || selection > 7)
	{
		SysFatalAlert("Invalid keybind selection");
	}

	currentPrefSize = 0;
	latestPrefSize = sizeof(struct UgbPrefs);

	prefs = MemPtrNew(latestPrefSize);
	MemSet(prefs, latestPrefSize, 0);
	MemSet(prefs->keys, sizeof(prefs->keys), 0);

	prefsVersion = PrefGetAppPreferences(APP_CREATOR, PREFERENCES_ID, NULL, &currentPrefSize, true);

	if (prefsVersion == noPreferenceFound){
		SysFatalAlert("No preferences detected!");
	} else if (currentPrefSize != latestPrefSize) {
		// If the preference size is invalid, return an error
		SysFatalAlert("Preferences are corrupted!");
	} else {
		// Get the application preferences
		PrefGetAppPreferences(APP_CREATOR, PREFERENCES_ID, prefs, &latestPrefSize, true);
	}

	noKey = KeyCurrentState();
	newKey = noKey;

	mask = KeySetMask(0);

	SetStatusLabel("Hold button NOW\ror press abort.");
	frm = FrmGetActiveForm();
	abortBtnIndex = FrmGetObjectIndex(frm, KeyBindingAbortBtn);
	okBtnIndex = FrmGetObjectIndex(frm, KeyBindingOkBtn);
	FrmShowObject(frm,  abortBtnIndex);
	FrmHideObject(frm, okBtnIndex);
	while (noKey == newKey)
	{
		newKey = KeyCurrentState();
		EvtGetEvent(&event, 30);
		FrmHandleEvent(frm, &event);

		if (event.eType == lstSelectEvent || (event.eType == ctlSelectEvent && event.data.ctlSelect.controlID == KeyBindingAbortBtn))
		{
			SetStatusLabel("Aborted!\rReady to bind next\rcontrol.");
			KeySetMask(mask);
			MemPtrFree(prefs);
			FrmHideObject(frm, abortBtnIndex);
			FrmShowObject(frm,  okBtnIndex);
			return;
		}
	}

	prefs->keys[selection] = newKey;
	prefs->keyBinded = true;

	PrefSetAppPreferences(APP_CREATOR, PREFERENCES_ID, PREFERENCES_LAST_VER, prefs, latestPrefSize, true); 

	SetStatusLabel("Saved!\rReady to bind next\rcontrol.");
	KeySetMask(mask);
	MemPtrFree(prefs);
	FrmHideObject(frm, abortBtnIndex);
	FrmShowObject(frm,  okBtnIndex);
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
