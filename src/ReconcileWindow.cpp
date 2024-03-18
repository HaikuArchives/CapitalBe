#include <Catalog.h>
#include <LayoutBuilder.h>
#include <MessageFilter.h>
#include <ScrollView.h>
#include <String.h>

#include "CBLocale.h"
#include "CurrencyBox.h"
#include "DAlert.h"
#include "Database.h"
#include "DateBox.h"
#include "LanguageRoster.h"
#include "MsgDefs.h"
#include "Preferences.h"
#include "ReconcileItem.h"
#include "ReconcileWindow.h"
#include "TimeSupport.h"
#include "Transaction.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ReconcileWindow"

enum {
	M_TOGGLE_DEPOSIT = 'tgdp',
	M_TOGGLE_CHECK,
	M_TOGGLE_CHARGE,
	M_SET_BALANCES,
	M_RECONCILE,
	M_RESET,
	M_AUTORECONCILE
};

/*
class ReconcileFilter : public BMessageFilter
{
public:
	ReconcileFilter(ReconcileWindow *checkview);
	~ReconcileFilter(void);
	virtual filter_result Filter(BMessage *msg, BHandler **target);

private:
	ReconcileWindow *fWindow;
};
*/

ReconcileWindow::ReconcileWindow(const BRect frame, Account* account)
	: BWindow(
		  frame, "", B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		  B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS
	  )
{
	BString temp;
	fCurrentDate = GetCurrentDate();
	//	AddCommonFilter(new ReconcileFilter(this));

	if (account) {
		temp = B_TRANSLATE("Reconcile:");
		temp << " " << account->Name();
		SetTitle(temp.String());
		gDatabase.AddObserver(this);
	}
	fAccount = account;


	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	AddShortcut('Q', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

	BView* back = new BView("backview", B_WILL_DRAW);
	back->SetViewColor(240, 240, 240);
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0).SetInsets(0).Add(back).End();

	temp = B_TRANSLATE("Date:");
	temp += " ";
	float width = back->StringWidth(temp.String());

	BString datestr;
	gDefaultLocale.DateToString(GetCurrentDate(), datestr);
	fDate = new DateBox("dateentry", temp.String(), datestr.String(), NULL);
	fDate->GetFilter()->SetMessenger(new BMessenger(this));


	temp = B_TRANSLATE("Starting balance:");
	temp += " ";
	fOpening = new CurrencyBox("starting", temp.String(), NULL, new BMessage(M_SET_BALANCES));
	fOpening->GetFilter()->SetMessenger(new BMessenger(this));

	temp = B_TRANSLATE("Ending balance:");
	temp += " ";
	fClosing = new CurrencyBox("closing", temp.String(), NULL, new BMessage(M_SET_BALANCES));
	fClosing->GetFilter()->SetMessenger(new BMessenger(this));

	temp = B_TRANSLATE("Bank charges:");
	temp += " ";
	fCharges = new CurrencyBox("charges", temp.String(), NULL, NULL);
	fCharges->GetFilter()->SetMessenger(new BMessenger(this));

	temp = B_TRANSLATE("Interest earned:");
	temp += " ";
	fInterest = new CurrencyBox("interest", temp.String(), NULL, NULL);
	fInterest->GetFilter()->SetMessenger(new BMessenger(this));

	fDepositList = new BListView("depositlist", B_SINGLE_SELECTION_LIST);
	fDepositList->SetFlags(fDepositList->Flags() | B_FULL_UPDATE_ON_RESIZE);
	fDepositList->SetInvocationMessage(new BMessage(M_TOGGLE_DEPOSIT));
	fDepScroll = new BScrollView("fDepScroll", fDepositList, 0, false, true);
	fDepScroll->SetViewColor(back->ViewColor());

	fCheckList = new BListView("checklist", B_SINGLE_SELECTION_LIST);
	fCheckList->SetFlags(fDepositList->Flags() | B_FULL_UPDATE_ON_RESIZE);
	fCheckList->SetInvocationMessage(new BMessage(M_TOGGLE_CHECK));
	fCheckScroll = new BScrollView("fCheckScroll", fCheckList, 0, false, true);
	fCheckScroll->SetViewColor(back->ViewColor());

	fChargeList = new BListView("chargelist", B_SINGLE_SELECTION_LIST);
	fChargeList->SetFlags(fDepositList->Flags() | B_FULL_UPDATE_ON_RESIZE);
	fChargeList->SetInvocationMessage(new BMessage(M_TOGGLE_CHARGE));
	fChargeScroll = new BScrollView("fChargeScroll", fChargeList, 0, false, true);
	fChargeScroll->SetViewColor(back->ViewColor());

	BString label;

	gCurrentLocale.CurrencyToString(fDepositTotal, label);
	temp = B_TRANSLATE("Total deposits:");
	temp << " " << label;

	fDepLabel = new BStringView("deplabel", temp.String());
	fDepLabel->SetAlignment(B_ALIGN_RIGHT);

	gCurrentLocale.CurrencyToString(fCheckTotal, label);
	temp = B_TRANSLATE("Total checks:");
	temp << " " << label;

	fCheckLabel = new BStringView("checklabel", temp.String());
	fCheckLabel->SetAlignment(B_ALIGN_RIGHT);

	gCurrentLocale.CurrencyToString(fChargeTotal, label);
	temp = B_TRANSLATE("Total charges:");
	temp << " " << label;
	fChargeLabel = new BStringView("chargelabel", temp.String());
	fChargeLabel->SetAlignment(B_ALIGN_RIGHT);

	fReconcile = new BButton("reconcile", B_TRANSLATE("Reconcile"), new BMessage(M_RECONCILE));
	fCancel = new BButton("cancel", B_TRANSLATE("Cancel"), new BMessage(B_QUIT_REQUESTED));
	fReset = new BButton("reset", B_TRANSLATE("Reset"), new BMessage(M_RESET));
	fAutoReconcile =
		new BButton("autoreconcile", B_TRANSLATE("Quick balance"), new BMessage(M_AUTORECONCILE));

	prefsLock.Lock();
	BString rechelp = gAppPath;
	prefsLock.Unlock();
	rechelp << "helpfiles/" << gCurrentLanguage->Name() << "/Reconcile Window Help";
	fHelpButton = new HelpButton("rechelp", rechelp.String());

	temp = B_TRANSLATE("Unreconciled total");
	temp += ":";
	fTotalLabel = new BStringView("totallabel", temp.String());

	account->DoForEachTransaction(AddReconcileItems, this);

	fDate->MakeFocus(true);

	BLayoutBuilder::Group<>(back, B_VERTICAL, 0)
		.SetInsets(10)
		.AddGrid(1.0f, 1.0f)
		.Add(fDate, 0, 0)
		.Add(fOpening, 1, 0, 2)
		.Add(fClosing, 3, 0, 2)
		.End()
		.AddGrid()
		.Add(fCharges, 0, 0)
		.Add(fInterest, 1, 0)
		.End()
		.AddGroup(B_HORIZONTAL)
		.AddGroup(B_VERTICAL, 0)
		.Add(fDepScroll)
		.Add(fDepLabel)
		.End()
		.AddGroup(B_VERTICAL, 0)
		.Add(fCheckScroll)
		.Add(fCheckLabel)
		.End()
		.AddGroup(B_VERTICAL, 0)
		.Add(fChargeScroll)
		.Add(fChargeLabel)
		.End()
		.End()
		.AddGrid(1.0f, 1.0f)
		.Add(fTotalLabel, 0, 0)
		.Add(fAutoReconcile, 0, 1)
		.Add(fHelpButton, 1, 1)
		.AddGlue(2, 1, 2)
		.Add(fReset, 4, 1)
		.AddGlue(5, 1)
		.Add(fCancel, 6, 1)
		.Add(fReconcile, 7, 1)
		.End()
		.End();
}

ReconcileWindow::~ReconcileWindow(void)
{
	prefsLock.Lock();
	gPreferences.RemoveData("reconcileframe");
	gPreferences.AddRect("reconcileframe", Frame());
	prefsLock.Unlock();
}

void
ReconcileWindow::FrameResized(float w, float h)
{
	// We implement our own resizing routines because all the controls need to be resized in a
	// proportional manner of the window being resized, such as the 3 listviews each taking up just
	// a little less than 1/3 of the window's width
	/*
		fDate->ResizeTo(w * fDateMultiplier, fDate->Frame().Height());
		fOpening->ResizeTo(w * fOpeningMultiplier, fOpening->Frame().Height());
		fOpening->MoveTo(fDate->Frame().right + 10,fOpening->Frame().top);
		fClosing->MoveTo(fOpening->Frame().right + 10,fClosing->Frame().top);
		fClosing->ResizeTo(w - 10 - fClosing->Frame().left, fClosing->Frame().Height());


		fCharges->ResizeTo((w/2)-15,fCharges->Frame().Height());
		fInterest->MoveTo(fCharges->Frame().right + 10,fInterest->Frame().top);
		fInterest->ResizeTo(w - 10 - fInterest->Frame().left,fInterest->Frame().Height());

		float listwidth = (Bounds().Width() - 40)/3;
		float height = fDepScroll->Frame().Height();

		fDepScroll->ResizeTo(listwidth, height);
		fCheckScroll->ResizeTo(listwidth, height);
		fChargeScroll->ResizeTo(listwidth, height);

		float top = fCheckList->Parent()->Frame().top;
		fCheckScroll->MoveTo(fDepScroll->Frame().right + 10,top);
		fChargeScroll->MoveTo(fCheckScroll->Frame().right + 10,top);

		fDepLabel->MoveTo(fDepScroll->Frame().left, fDepScroll->Frame().bottom+5);
		fCheckLabel->MoveTo(fCheckScroll->Frame().left, fCheckScroll->Frame().bottom+5);
		fChargeLabel->MoveTo(fChargeScroll->Frame().left, fChargeScroll->Frame().bottom+5);

		fDepLabel->ResizeTo(fDepScroll->Frame().Width(),fDepLabel->Frame().Height());
		fCheckLabel->ResizeTo(fCheckScroll->Frame().Width(),fCheckLabel->Frame().Height());
		fChargeLabel->ResizeTo(fChargeScroll->Frame().Width(),fChargeLabel->Frame().Height());
	*/
}

void
ReconcileWindow::MessageReceived(BMessage* msg)
{
	int32 index;
	ReconcileItem* selection;
	BString label, temp;

	switch (msg->what) {
	case M_PREVIOUS_FIELD: {
		if (fDate->TextView()->IsFocus())
			fReconcile->MakeFocus(true);
		else if (fOpening->TextView()->IsFocus())
			fDate->MakeFocus(true);
		else if (fClosing->TextView()->IsFocus())
			fOpening->MakeFocus(true);
		else if (fCharges->TextView()->IsFocus())
			fClosing->MakeFocus(true);
		else if (fInterest->TextView()->IsFocus())
			fCharges->MakeFocus(true);
		break;
	}
	case M_NEXT_FIELD: {
		if (fDate->TextView()->IsFocus())
			fOpening->MakeFocus(true);
		else if (fOpening->TextView()->IsFocus())
			fClosing->MakeFocus(true);
		else if (fClosing->TextView()->IsFocus())
			fCharges->MakeFocus(true);
		else if (fCharges->TextView()->IsFocus())
			fInterest->MakeFocus(true);
		else if (fInterest->TextView()->IsFocus())
			fDepositList->MakeFocus(true);

		break;
	}
	case M_RECONCILE: {
		ApplyChargesAndInterest();

		int32 i;
		ReconcileItem* item;
		for (i = 0; i < fDepositList->CountItems(); i++) {
			item = (ReconcileItem*)fDepositList->ItemAt(i);
			if (item->IsReconciled())
				item->SyncToTransaction();
		}

		for (i = 0; i < fCheckList->CountItems(); i++) {
			item = (ReconcileItem*)fCheckList->ItemAt(i);
			if (item->IsReconciled())
				item->SyncToTransaction();
		}

		for (i = 0; i < fChargeList->CountItems(); i++) {
			item = (ReconcileItem*)fChargeList->ItemAt(i);
			if (item->IsReconciled())
				item->SyncToTransaction();
		}

		BMessage notify;
		fAccount->Notify(WATCH_REDRAW | WATCH_ACCOUNT, &notify);
		PostMessage(B_QUIT_REQUESTED);
		break;
	}
	case M_RESET: {
		int32 i;
		ReconcileItem* item;
		for (i = 0; i < fDepositList->CountItems(); i++) {
			item = (ReconcileItem*)fDepositList->ItemAt(i);
			if (item->IsReconciled()) {
				item->RevertTransaction();
				fDepositList->InvalidateItem(i);
			}
		}

		for (i = 0; i < fCheckList->CountItems(); i++) {
			item = (ReconcileItem*)fCheckList->ItemAt(i);
			if (item->IsReconciled()) {
				item->RevertTransaction();
				fCheckList->InvalidateItem(i);
			}
		}

		for (i = 0; i < fChargeList->CountItems(); i++) {
			item = (ReconcileItem*)fChargeList->ItemAt(i);
			if (item->IsReconciled()) {
				item->RevertTransaction();
				fChargeList->InvalidateItem(i);
			}
		}
		break;
	}
	case M_TOGGLE_DEPOSIT: {
		index = fDepositList->CurrentSelection();
		selection = (ReconcileItem*)fDepositList->ItemAt(index);
		if (selection) {
			if (selection->IsReconciled()) {
				selection->SetReconciled(false);
				fDepositTotal -= selection->GetTransaction()->Amount();
				fTotal -= selection->GetTransaction()->Amount();
			}
			else {
				selection->SetReconciled(true);
				fDepositTotal += selection->GetTransaction()->Amount();
				fTotal += selection->GetTransaction()->Amount();
			}
			fDepositList->InvalidateItem(index);

			fAccount->GetLocale().CurrencyToString(fDepositTotal, label);
			temp.SetToFormat(B_TRANSLATE("Total deposits: %s"), label);
			fDepLabel->SetText(label.String());

			fAccount->GetLocale().CurrencyToString(fTotal + fDifference, label);
			temp = "";
			temp.SetToFormat(B_TRANSLATE("Unreconciled total: %s"), label);
			fTotalLabel->SetText(label.String());

			if ((fTotal + fDifference) == 0)
				fReconcile->SetEnabled(true);
			else
				fReconcile->SetEnabled(false);
		}
		break;
	}
	case M_TOGGLE_CHECK: {
		index = fCheckList->CurrentSelection();
		selection = (ReconcileItem*)fCheckList->ItemAt(index);
		if (selection) {
			if (selection->IsReconciled()) {
				selection->SetReconciled(false);
				fCheckTotal += selection->GetTransaction()->Amount();
				fTotal -= selection->GetTransaction()->Amount();
			}
			else {
				selection->SetReconciled(true);
				fCheckTotal -= selection->GetTransaction()->Amount();
				fTotal += selection->GetTransaction()->Amount();
			}
			fCheckList->InvalidateItem(index);

			fAccount->GetLocale().CurrencyToString(fCheckTotal, label);
			temp.SetToFormat(B_TRANSLATE("Total checks: %s"), label);
			fCheckLabel->SetText(label.String());

			fAccount->GetLocale().CurrencyToString(fTotal + fDifference, label);
			temp = "";
			temp.SetToFormat(B_TRANSLATE("Unreconciled total: %s"), label);
			fTotalLabel->SetText(label.String());

			if ((fTotal + fDifference) == 0)
				fReconcile->SetEnabled(true);
			else
				fReconcile->SetEnabled(false);
		}
		break;
	}
	case M_TOGGLE_CHARGE: {
		index = fChargeList->CurrentSelection();
		selection = (ReconcileItem*)fChargeList->ItemAt(index);
		if (selection) {
			if (selection->IsReconciled()) {
				selection->SetReconciled(false);
				fChargeTotal += selection->GetTransaction()->Amount();
				fTotal -= selection->GetTransaction()->Amount();
			}
			else {
				selection->SetReconciled(true);
				fChargeTotal -= selection->GetTransaction()->Amount();
				fTotal += selection->GetTransaction()->Amount();
			}
			fChargeList->InvalidateItem(index);

			fAccount->GetLocale().CurrencyToString(fChargeTotal, label);
			temp.SetToFormat(B_TRANSLATE("Total charges: %s"), label);
			fChargeLabel->SetText(label.String());

			fAccount->GetLocale().CurrencyToString(fTotal + fDifference, label);
			temp = "";
			temp.SetToFormat(B_TRANSLATE("Unreconciled total: %s"), label);
			fTotalLabel->SetText(label.String());

			if ((fTotal + fDifference) == 0)
				fReconcile->SetEnabled(true);
			else
				fReconcile->SetEnabled(false);
		}
		break;
	}
	case M_SET_BALANCES: {
		Fixed fixed, fixed2;
		if (gCurrentLocale.StringToCurrency(fOpening->Text(), fixed) != B_OK ||
			gCurrentLocale.StringToCurrency(fClosing->Text(), fixed2) != B_OK)
			break;

		fDifference = fixed - fixed2;

		gCurrentLocale.CurrencyToString(fTotal + fDifference, label);
		label.Prepend(" ");
		label.Prepend("Unreconciled total:");
		fTotalLabel->SetText(label.String());

		if ((fTotal + fDifference) == 0)
			fReconcile->SetEnabled(true);
		else
			fReconcile->SetEnabled(false);
		break;
	}
	case M_AUTORECONCILE: {
		AutoReconcile();
		BMessage notify;
		fAccount->Notify(WATCH_REDRAW | WATCH_ACCOUNT, &notify);

		break;
	}
	default:
		BWindow::MessageReceived(msg);
	}
}

void
ReconcileWindow::HandleNotify(const uint64& value, const BMessage* msg)
{
	bool unlock = false;
	if (!IsLocked()) {
		unlock = true;
		Lock();
	}

	// This should prevent a rather spectacular slowdown if the user decides to import some accounts
	// while reconciling. Bizarre and unthinkable, but people is people. :P
	if (value & WATCH_MASS_EDIT) {
		if (IsWatching(WATCH_TRANSACTION)) {
			RemoveWatch(WATCH_ALL);
			AddWatch(WATCH_MASS_EDIT);
		}
		else {
			AddWatch(WATCH_ALL);
		}

		if (unlock)
			Unlock();
		return;
	}

	Account* acc = NULL;
	if ((value & WATCH_ACCOUNT) && (value & WATCH_DELETE)) {
		if ((msg->FindPointer("item", (void**)&acc) == B_OK) && (acc == fAccount))
			PostMessage(B_QUIT_REQUESTED);
	}
	else if ((value & WATCH_TRANSACTION)) {
		if (value & WATCH_DELETE) {
			uint32 id;
			if (msg->FindInt32("id", (int32*)&id) == B_OK) {
				ReconcileItem* deleteditem;
				BListView* itemlist;

				deleteditem = FindItemForID(fDepositList, id);
				if (deleteditem)
					itemlist = fDepositList;
				else {
					deleteditem = FindItemForID(fCheckList, id);
					if (deleteditem)
						itemlist = fCheckList;
					else {
						deleteditem = FindItemForID(fChargeList, id);
						if (deleteditem)
							itemlist = fChargeList;
						else {
							ShowBug("No list for ID in ReconcileWindow::HandleNotify");
							if (unlock)
								Unlock();
							return;
						}
					}
				}

				itemlist->RemoveItem(deleteditem);
				delete deleteditem;
			}
		}
		else if (value & WATCH_CREATE) {
			uint32 accountid;
			if (msg->FindInt32("accountid", (int32*)&accountid) != B_OK ||
				accountid != fAccount->GetID()) {
				if (unlock)
					Unlock();
				return;
			}

			TransactionData* data;
			if (msg->FindPointer("item", (void**)&data) == B_OK) {
				ReconcileItem* newitem = new ReconcileItem(*data);

				if (data->Type().TypeCode() == TRANS_DEP)
					InsertTransactionItem(fDepositList, newitem);
				else if (data->Type().TypeCode() == TRANS_NUMERIC)
					InsertTransactionItem(fCheckList, newitem);
				else
					InsertTransactionItem(fChargeList, newitem);
			}
		}
	}
	if (unlock)
		Unlock();
}

bool
ReconcileWindow::QuitRequested(void)
{
	gDatabase.RemoveObserver(this);
	return true;
}

void
ReconcileWindow::ApplyChargesAndInterest(void)
{
	Fixed charge;
	if (strlen(fCharges->Text()) > 0 &&
		gCurrentLocale.StringToCurrency(fCharges->Text(), charge) == B_OK) {
		TransactionData chargetrans(
			fAccount, fDate->Text(), "ATM", B_TRANSLATE("Bank charge"), fCharges->Text(),
			B_TRANSLATE("Bank Charge"), NULL, TRANS_RECONCILED
		);
		gDatabase.AddTransaction(chargetrans);
	}

	Fixed interest;
	if (strlen(fInterest->Text()) > 0 &&
		gCurrentLocale.StringToCurrency(fInterest->Text(), interest) == B_OK) {
		TransactionData interesttrans(
			fAccount, fDate->Text(), B_TRANSLATE("DEP"), B_TRANSLATE("Account interest"),
			fInterest->Text(), B_TRANSLATE("Account interest"), NULL, TRANS_RECONCILED
		);
		gDatabase.AddTransaction(interesttrans);
	}
}

bool
ReconcileWindow::AutoReconcile(void)
{
	// We are going to attempt to automatically reconcile the account. We will do
	// this by adding up the values of all transactions unreconciled before the
	// statement date. If they balance, we can notify the user that we were successful
	// and we've saved him/her quite a bit of time in finance handling. If not,
	// we can tell the user that it failed and the conditions under which it works.
	time_t statdate;
	int32 i;
	ReconcileItem* item;

	if (gDefaultLocale.StringToDate(fDate->Text(), statdate) != B_OK) {
		// Do we have an empty date box?
		if (strlen(fDate->Text()) < 1) {
			ShowAlert(
				B_TRANSLATE("Date is missing."),
				B_TRANSLATE("You need to enter the date for the statement to Quick Balance.")
			);
			return false;
		}
	}

	Fixed dep, chrg, chk, bankchrg, interest;
	BList list;

	if (strlen(fCharges->Text()) > 0) {
		if (gCurrentLocale.StringToCurrency(fCharges->Text(), bankchrg) == B_OK)
			bankchrg.Invert();
		else {
			ShowAlert(
				B_TRANSLATE("CapitalBe didn't understand the amount for Bank Charges."),
				B_TRANSLATE("There may be a typo or the wrong kind of currency symbol "
							"for this account.")
			);
			return false;
		}
	}

	if (strlen(fInterest->Text()) > 0) {
		if (gCurrentLocale.StringToCurrency(fInterest->Text(), interest) != B_OK) {
			ShowAlert(
				B_TRANSLATE("CapitalBe didn't understand the amount for Interest Earned."),
				B_TRANSLATE("There may be a typo or the wrong kind of currency symbol "
							"for this account.")
			);
			return false;
		}
	}

	for (i = 0; i < fDepositList->CountItems(); i++) {
		item = (ReconcileItem*)fDepositList->ItemAt(i);
		if (item->GetTransaction()->Date() >= statdate)
			break;

		if (!item->IsReconciled()) {
			dep += item->GetTransaction()->Amount();
			list.AddItem(item);
		}
	}

	for (i = 0; i < fCheckList->CountItems(); i++) {
		item = (ReconcileItem*)fCheckList->ItemAt(i);
		if (item->GetTransaction()->Date() >= statdate)
			break;

		if (!item->IsReconciled()) {
			chk += item->GetTransaction()->Amount();
			list.AddItem(item);
		}
	}

	for (i = 0; i < fChargeList->CountItems(); i++) {
		item = (ReconcileItem*)fChargeList->ItemAt(i);
		if (item->GetTransaction()->Date() >= statdate)
			break;

		if (!item->IsReconciled()) {
			chrg += item->GetTransaction()->Amount();
			list.AddItem(item);
		}
	}

	if (dep + chk + chrg + bankchrg + interest + fDifference == 0) {
		for (i = 0; i < list.CountItems(); i++) {
			item = (ReconcileItem*)list.ItemAt(i);
			item->SetReconciled(true);
		}
		ApplyChargesAndInterest();
		ShowAlert(B_TRANSLATE("Success!"), B_TRANSLATE("Quick balance successful!"), B_IDEA_ALERT);
		PostMessage(B_QUIT_REQUESTED);
		return true;
	}

	ShowAlert(
		B_TRANSLATE("Couldn't quick balance."),
		B_TRANSLATE("Quick Balance failed. This doesn't mean "
					"that you did something wrong - it's just that Quick Balance works on "
					"simpler cases in balancing an account than this one. Sorry.")
	);
	return false;
}

ReconcileItem*
ReconcileWindow::FindItemForID(BListView* target, const uint32& id)
{
	for (int32 i = 0; i < target->CountItems(); i++) {
		ReconcileItem* temp = (ReconcileItem*)target->ItemAt(i);
		if (temp->GetTransaction()->GetID() == id)
			return temp;
	}
	return NULL;
}

void
ReconcileWindow::InsertTransactionItem(BListView* target, ReconcileItem* item)
{
	TransactionData* itemdata = item->GetTransaction();

	for (int32 i = 0; i < target->CountItems(); i++) {
		ReconcileItem* temp = (ReconcileItem*)target->ItemAt(i);
		TransactionData* tempdata = temp->GetTransaction();

		if (itemdata->Date() < tempdata->Date() ||
			(itemdata->Date() == tempdata->Date() &&
			 strcmp(itemdata->Payee(), tempdata->Payee()) < 1)) {
			target->AddItem(item, i);
			return;
		}
	}

	target->AddItem(item);
}

void
AddReconcileItems(const TransactionData& data, void* ptr)
{
	if (data.Status() == TRANS_RECONCILED)
		return;

	ReconcileWindow* win = (ReconcileWindow*)ptr;

	switch (data.Type().TypeCode()) {
	case TRANS_NUMERIC: {
		win->fCheckList->AddItem(new ReconcileItem(data));
		break;
	}

	case TRANS_DEP: {
		win->fDepositList->AddItem(new ReconcileItem(data));
		break;
	}

	case TRANS_ATM:
	case TRANS_XFER:
	default: {
		win->fChargeList->AddItem(new ReconcileItem(data));
		break;
	}
	}
}

/*
ReconcileFilter::ReconcileFilter(ReconcileWindow *win)
 : BMessageFilter(B_PROGRAMMED_DELIVERY, B_ANY_SOURCE,B_KEY_DOWN),
	fWindow(win)
{
}

ReconcileFilter::~ReconcileFilter(void)
{
}

filter_result ReconcileFilter::Filter(BMessage *msg, BHandler **target)
{
	int32 mod;
	if(msg->FindInt32("modifiers",&mod)==B_OK)
	{
		if( (mod & B_COMMAND_KEY) || (mod & B_COMMAND_KEY) ||
			(mod & B_OPTION_KEY) )
			return B_DISPATCH_MESSAGE;
	}

	BView *v=dynamic_cast<BView*>(*target);
	if(!v || strcmp("_input_",v->Name())!=0)
		return B_DISPATCH_MESSAGE;

	BTextControl *text = dynamic_cast<BTextControl*>(v->Parent());
	if(!text)
		return B_DISPATCH_MESSAGE;

	if(text==fWindow->fDate)
	{
		int32 rawchar;
		msg->FindInt32("raw_char",&rawchar);
//		if(rawchar==B_ENTER)
//		{
//			fWindow->PostMessage(M_NEXT_FIELD);
//			return B_SKIP_MESSAGE;
//		}

		// Weed out navigation keys
		if(rawchar<32 && rawchar!=B_PAGE_UP && rawchar!=B_PAGE_DOWN)
			return B_DISPATCH_MESSAGE;

		int32 start, end;
		text->TextView()->GetSelection(&start,&end);

		BString string;
		if(rawchar=='+')
		{
			if(strlen(text->Text())>0)
				fWindow->fCurrentDate = IncrementDateByDay(fWindow->fCurrentDate);

			gDefaultLocale.DateToString(fWindow->fCurrentDate,string);
			text->SetText(string.String());
			text->TextView()->SelectAll();
			return B_SKIP_MESSAGE;
		}
		else
		if(rawchar=='-')
		{
			if(strlen(text->Text())>0)
				fWindow->fCurrentDate = DecrementDateByDay(fWindow->fCurrentDate);
			gDefaultLocale.DateToString(fWindow->fCurrentDate,string);
			text->SetText(string.String());
			text->TextView()->SelectAll();
			return B_SKIP_MESSAGE;
		}
		else
		if(rawchar==B_PAGE_UP)
		{
			if(strlen(text->Text())>0)
			{
				if(mod & B_SHIFT_KEY)
					fWindow->fCurrentDate = IncrementDateByYear(fWindow->fCurrentDate);
				else
					fWindow->fCurrentDate = IncrementDateByMonth(fWindow->fCurrentDate);
			}
			gDefaultLocale.DateToString(fWindow->fCurrentDate,string);
			text->SetText(string.String());
			text->TextView()->SelectAll();
			return B_SKIP_MESSAGE;
		}
		else
		if(rawchar==B_PAGE_DOWN)
		{
			if(strlen(text->Text())>0)
			{
				if(mod & B_SHIFT_KEY)
					fWindow->fCurrentDate = DecrementDateByYear(fWindow->fCurrentDate);
				else
					fWindow->fCurrentDate = DecrementDateByMonth(fWindow->fCurrentDate);
			}
			gDefaultLocale.DateToString(fWindow->fCurrentDate,string);
			text->SetText(string.String());
			text->TextView()->SelectAll();
			return B_SKIP_MESSAGE;
		}
	}

	return B_DISPATCH_MESSAGE;
}
*/
