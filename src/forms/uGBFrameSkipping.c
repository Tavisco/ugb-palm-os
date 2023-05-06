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

	default:
		break;
	}

	return handled;
}
