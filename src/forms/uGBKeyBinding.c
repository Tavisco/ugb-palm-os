#include <PalmOS.h>
#include <VFSMgr.h>
#include <PceNativeCall.h>
#include "../uGB.h"
#include "../UiResourceIDs.h"
#include "../palmosArmData.h"
#include "../gb.h"

static void ListenForKey(Int16 selection)
{

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
	case KeyBindingCancelBtn:
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
