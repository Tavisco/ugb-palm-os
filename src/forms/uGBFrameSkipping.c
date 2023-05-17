#include <PalmOS.h>
#include "../uGB.h"
#include "../UiResourceIDs.h"

Boolean FrameSkippingDoCommand(UInt16 command)
{
	Boolean handled = false;

	switch (command)
	{
	case FrameSkippingOkBtn:
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

static void updateSliderLabel(UInt16 value, FormPtr fp)
{
	Char *sliderValue;

	sliderValue = MemPtrNew(3);

	StrIToA(sliderValue, value);
	FrmHideObject(fp, FrmGetObjectIndex(fp, FrameSkippingCurrValueLbl));
	FrmCopyLabel(fp, FrameSkippingCurrValueLbl, sliderValue);
	FrmShowObject(fp, FrmGetObjectIndex(fp, FrameSkippingCurrValueLbl));

	MemPtrFree(sliderValue);
}

static void updatePrefsWithValue(UInt16 value)
{
	UInt16 latestPrefSize;
	struct UgbPrefs *prefs;

	latestPrefSize = sizeof(struct UgbPrefs);
	prefs = MemPtrNew(latestPrefSize);
	MemSet(prefs, latestPrefSize, 0);

	PrefGetAppPreferences(APP_CREATOR, PREFERENCES_ID, prefs, &latestPrefSize, true);

	prefs->frameDithering = (UInt8)value + 1;
	PrefSetAppPreferences(APP_CREATOR, PREFERENCES_ID, PREFERENCES_LAST_VER, prefs, latestPrefSize, true);
	MemPtrFree(prefs);
}

static void setSliderValue(FormPtr fp)
{
	UInt16 latestPrefSize;
	struct UgbPrefs *prefs;

	latestPrefSize = sizeof(struct UgbPrefs);
	prefs = MemPtrNew(latestPrefSize);
	MemSet(prefs, latestPrefSize, 0);

	PrefGetAppPreferences(APP_CREATOR, PREFERENCES_ID, prefs, &latestPrefSize, true);

	updateSliderLabel(prefs->frameDithering - 1, fp);
	FrmSetControlValue(fp, FrmGetObjectIndex(fp, FrameSkippingSlider), prefs->frameDithering - 1);

	MemPtrFree(prefs);
}

Boolean FrameSkippingHandleEvent(EventType *eventP)
{
	Boolean handled = false;
	FormPtr fp = FrmGetActiveForm();

	switch (eventP->eType)
	{
	case frmOpenEvent:
		FrmDrawForm(fp);
		setSliderValue(fp);
		handled = true;
		break;

	case ctlSelectEvent:
		return FrameSkippingDoCommand(eventP->data.ctlSelect.controlID);

	case ctlRepeatEvent:
		updateSliderLabel(eventP->data.ctlRepeat.value, fp);
		updatePrefsWithValue(eventP->data.ctlRepeat.value);
		break;

	default:
		break;
	}

	return handled;
}
