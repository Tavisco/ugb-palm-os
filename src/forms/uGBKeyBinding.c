#include <PalmOS.h>
#include <VFSMgr.h>
#include <PceNativeCall.h>
#include "../uGB.h"
#include "../UiResourceIDs.h"
#include "../palmosArmData.h"
#include "../gb.h"


Boolean KeyBindingDoCommand(UInt16 command)
{
	Boolean handled = false;

	switch (command)
	{
	case KeyBindingSaveBtn:
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
			handled = true;
			break;

		case ctlSelectEvent:
			return KeyBindingDoCommand(eventP->data.ctlSelect.controlID);
		
		default:
				break;
	}

	return handled;
}
