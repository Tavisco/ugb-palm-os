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

Boolean FrameSkippingHandleEvent(EventType *eventP)
{
	Boolean handled = false;
	FormPtr fp = FrmGetActiveForm();

	switch (eventP->eType)
	{
	case frmOpenEvent:
		FrmDrawForm(fp);
		handled = true;
		break;

	case ctlSelectEvent:
		return FrameSkippingDoCommand(eventP->data.ctlSelect.controlID);

	case ctlRepeatEvent:
		updateSliderLabel(eventP->data.ctlRepeat.value, fp);
		break;

	default:
		break;
	}

	return handled;
}
