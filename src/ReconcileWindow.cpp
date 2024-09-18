#include <Catalog.h>
#include <GridLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <MessageFilter.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <String.h>

#include "CBLocale.h"
#include "CalendarButton.h"
#include "CurrencyBox.h"
#include "DAlert.h"
#include "Database.h"
#include "DateBox.h"
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
	M_TOGGLE_CHARGE,
	M_SET_BALANCES,
	M_FORMAT_AMOUNT,
	M_RECONCILE,
	M_RESET,
	M_AUTORECONCILE
};

/*
class ReconcileFilter : public BMessageFilter
{
public:
	ReconcileFilter(ReconcileWindow *checkview);
	~ReconcileFilter();
	virtual filter_result Filter(BMessage *msg, BHandler **target);

private:
	ReconcileWindow *fWindow;
};
*/

ReconcileWindow::ReconcileWindow(const BRect frame, Account* account)
	:
	BWindow(frame, "", B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE | B_CLOSE_ON_ESCAPE)
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

	BStringView* dateLabel
		= new BStringView("datelabel", B_TRANSLATE_CONTEXT("Date", "CommonTerms"));
	BString datestr;
	gDefaultLocale.DateToString(fCurrentDate, datestr);
	fDate = new DateBox("dateentry", NULL, datestr.String(), NULL);
	fDate->GetFilter()->SetMessenger(new BMessenger(this));

	CalendarButton* calendarButton = new CalendarButton(fDate);

	float maxwidth = 0;
	BString label(B_TRANSLATE("Starting balance"));
	fOpening = new CurrencyBox("starting", label, NULL, new BMessage(M_SET_BALANCES));
	fOpening->GetFilter()->SetMessenger(new BMessenger(this));
	fOpening->Validate(true);
	maxwidth = MAX(maxwidth, be_plain_font->StringWidth(label));

	label = B_TRANSLATE("Ending balance");
	fClosing = new CurrencyBox("closing", label, NULL, new BMessage(M_SET_BALANCES));
	fClosing->GetFilter()->SetMessenger(new BMessenger(this));
	fClosing->Validate(true);
	maxwidth = MAX(maxwidth, be_plain_font->StringWidth(label));

	label = B_TRANSLATE("Bank charges");
	fCharges = new CurrencyBox("charges", label, NULL, new BMessage(M_FORMAT_AMOUNT));
	fCharges->GetFilter()->SetMessenger(new BMessenger(this));
	fCharges->Validate(true);
	maxwidth = MAX(maxwidth, be_plain_font->StringWidth(label));

	label = B_TRANSLATE("Interest earned");
	fInterest = new CurrencyBox("interest", label, NULL, new BMessage(M_FORMAT_AMOUNT));
	fInterest->GetFilter()->SetMessenger(new BMessenger(this));
	fInterest->Validate(true);
	maxwidth = MAX(maxwidth, be_plain_font->StringWidth(label));

	fDepositList = new BListView("depositlist", B_SINGLE_SELECTION_LIST);
	fDepositList->SetFlags(fDepositList->Flags() | B_FULL_UPDATE_ON_RESIZE);
	fDepositList->SetInvocationMessage(new BMessage(M_TOGGLE_DEPOSIT));
	fDepScroll = new BScrollView("fDepScroll", fDepositList, 0, false, true);

	fChargeList = new BListView("chargelist", B_SINGLE_SELECTION_LIST);
	fChargeList->SetFlags(fDepositList->Flags() | B_FULL_UPDATE_ON_RESIZE);
	fChargeList->SetInvocationMessage(new BMessage(M_TOGGLE_CHARGE));
	fChargeScroll = new BScrollView("fChargeScroll", fChargeList, 0, false, true);

	BString total;

	gCurrentLocale.CurrencyToString(fDepositTotal, total);
	temp = B_TRANSLATE("Total deposits:");
	temp << " " << total;

	fDepLabel = new BStringView("deplabel", temp.String());
	fDepLabel->SetAlignment(B_ALIGN_CENTER);
	fDepLabel->SetExplicitSize(
		BSize(be_plain_font->StringWidth(temp) + be_plain_font->StringWidth("100.00.000,00"),
			B_SIZE_UNSET));

	gCurrentLocale.CurrencyToString(fChargeTotal, total);
	temp = B_TRANSLATE("Total charges:");
	temp << " " << total;
	fChargeLabel = new BStringView("chargelabel", temp.String());
	fChargeLabel->SetAlignment(B_ALIGN_CENTER);
	fChargeLabel->SetExplicitSize(
		BSize(be_plain_font->StringWidth(temp) + be_plain_font->StringWidth("100.000.000,00"),
			B_SIZE_UNSET));

	fReconcile = new BButton("reconcile", B_TRANSLATE("Reconcile"), new BMessage(M_RECONCILE));
	fCancel = new BButton("cancel", B_TRANSLATE("Cancel"), new BMessage(B_QUIT_REQUESTED));
	fReset = new BButton("reset", B_TRANSLATE("Reset"), new BMessage(M_RESET));
	fAutoReconcile
		= new BButton("autoreconcile", B_TRANSLATE("Quick balance"), new BMessage(M_AUTORECONCILE));

	fHelpButton = new HelpButton("reconcile.html", NULL);

	fAccount->GetLocale().CurrencyToString(fTotal + fDifference, total);
	temp = "";
	temp.SetToFormat(B_TRANSLATE("Unreconciled total: %s"), total.String());
	fTotalLabel = new BStringView("totallabel", temp.String());
	fTotalLabel->SetAlignment(B_ALIGN_CENTER);

	account->DoForEachTransaction(AddReconcileItems, this);

	fDate->MakeFocus(true);

	// clang-format off
	BView* calendarWidget = new BView("calendarwidget", B_WILL_DRAW);
	BLayoutBuilder::Group<>(calendarWidget, B_HORIZONTAL, -2)
		.Add(fDate)
		.Add(calendarButton)
		.End();

	BGridLayout* gridLayout = BGridLayoutBuilder(B_USE_DEFAULT_SPACING, 1.0f)
		.Add(dateLabel, 0, 0)
		.Add(calendarWidget, 0, 1)
		.Add(fOpening->CreateLabelLayoutItem(), 1, 0)
		.Add(fOpening->CreateTextViewLayoutItem(), 1, 1)
		.Add(fClosing->CreateLabelLayoutItem(), 2, 0)
		.Add(fClosing->CreateTextViewLayoutItem(), 2, 1)
		.Add(fCharges->CreateLabelLayoutItem(), 3, 0)
		.Add(fCharges->CreateTextViewLayoutItem(), 3, 1)
		.Add(fInterest->CreateLabelLayoutItem(), 4, 0)
		.Add(fInterest->CreateTextViewLayoutItem(), 4, 1);

	for (int32 i = 0; i < gridLayout->CountColumns(); i++)
		gridLayout->SetMinColumnWidth(i, maxwidth);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(gridLayout)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
		.AddGrid(B_USE_DEFAULT_SPACING, 1.0f)
			.Add(fDepLabel, 0, 0)
			.Add(fDepScroll, 0, 1)
			.Add(fChargeLabel, 1, 0)
			.Add(fChargeScroll, 1, 1)
			.End()
		.AddGroup(B_HORIZONTAL)
			.Add(fTotalLabel)
			.End()
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
		.AddGroup(B_HORIZONTAL)
			.Add(fAutoReconcile)
			.Add(fHelpButton)
			.AddGlue()
			.Add(fReset)
			.AddGlue()
			.Add(fCancel)
			.Add(fReconcile)
			.End()
		.End();
	// clang-format on
}


ReconcileWindow::~ReconcileWindow()
{
	prefsLock.Lock();
	gPreferences.RemoveData("reconcileframe");
	gPreferences.AddRect("reconcileframe", Frame());
	prefsLock.Unlock();
}


void
ReconcileWindow::MessageReceived(BMessage* msg)
{
	int32 index;
	ReconcileItem* selection;
	BString label, temp;

	switch (msg->what) {
		case M_PREVIOUS_FIELD:
		{
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
		case M_NEXT_FIELD:
		{
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
		case M_RECONCILE:
		{
			ApplyChargesAndInterest();

			int32 i;
			ReconcileItem* item;
			for (i = 0; i < fDepositList->CountItems(); i++) {
				item = (ReconcileItem*)fDepositList->ItemAt(i);
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
		case M_RESET:
		{
			int32 i;
			ReconcileItem* item;
			for (i = 0; i < fDepositList->CountItems(); i++) {
				item = (ReconcileItem*)fDepositList->ItemAt(i);
				if (item->IsReconciled()) {
					item->RevertTransaction();
					fDepositList->InvalidateItem(i);
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
		case M_TOGGLE_DEPOSIT:
		{
			index = fDepositList->CurrentSelection();
			selection = (ReconcileItem*)fDepositList->ItemAt(index);
			if (selection) {
				if (selection->IsReconciled()) {
					selection->SetReconciled(false);
					fDepositTotal -= selection->GetTransaction()->Amount();
					fTotal -= selection->GetTransaction()->Amount();
				} else {
					selection->SetReconciled(true);
					fDepositTotal += selection->GetTransaction()->Amount();
					fTotal += selection->GetTransaction()->Amount();
				}
				fDepositList->InvalidateItem(index);

				fAccount->GetLocale().CurrencyToString(fDepositTotal, label);
				temp.SetToFormat(B_TRANSLATE("Total deposits: %s"), label.String());
				fDepLabel->SetText(temp.String());

				fAccount->GetLocale().CurrencyToString(fTotal + fDifference, label);
				temp = "";
				temp.SetToFormat(B_TRANSLATE("Unreconciled total: %s"), label.String());
				fTotalLabel->SetText(temp.String());

				if ((fTotal + fDifference) == 0)
					fReconcile->SetEnabled(true);
				else
					fReconcile->SetEnabled(false);
			}
			break;
		}
		case M_TOGGLE_CHARGE:
		{
			index = fChargeList->CurrentSelection();
			selection = (ReconcileItem*)fChargeList->ItemAt(index);
			if (selection) {
				if (selection->IsReconciled()) {
					selection->SetReconciled(false);
					fChargeTotal += selection->GetTransaction()->Amount();
					fTotal -= selection->GetTransaction()->Amount();
				} else {
					selection->SetReconciled(true);
					fChargeTotal -= selection->GetTransaction()->Amount();
					fTotal += selection->GetTransaction()->Amount();
				}
				fChargeList->InvalidateItem(index);

				fAccount->GetLocale().CurrencyToString(fChargeTotal, label);
				temp.SetToFormat(B_TRANSLATE("Total charges: %s"), label.String());
				fChargeLabel->SetText(temp);

				fAccount->GetLocale().CurrencyToString(fTotal + fDifference, label);
				temp = "";
				temp.SetToFormat(B_TRANSLATE("Unreconciled total: %s"), label.String());
				fTotalLabel->SetText(temp);

				if ((fTotal + fDifference) == 0)
					fReconcile->SetEnabled(true);
				else
					fReconcile->SetEnabled(false);
			}
			break;
		}
		case M_SET_BALANCES:
		{
			Fixed fixed, fixed2;
			if (gCurrentLocale.StringToCurrency(fOpening->Text(), fixed) != B_OK
				|| gCurrentLocale.StringToCurrency(fClosing->Text(), fixed2) != B_OK)
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

			fOpening->Validate(true);
			fClosing->Validate(true);
			break;
		}
		case M_FORMAT_AMOUNT:
		{
			fCharges->Validate(true);
			fInterest->Validate(true);
			break;
		}
		case M_AUTORECONCILE:
		{
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
		} else
			AddWatch(WATCH_ALL);

		if (unlock)
			Unlock();
		return;
	}

	Account* acc = NULL;
	if ((value & WATCH_ACCOUNT) && (value & WATCH_DELETE)) {
		if ((msg->FindPointer("item", (void**)&acc) == B_OK) && (acc == fAccount))
			PostMessage(B_QUIT_REQUESTED);
	} else if (value & WATCH_TRANSACTION) {
		if (value & WATCH_DELETE) {
			uint32 id;
			if (msg->FindInt32("id", (int32*)&id) == B_OK) {
				ReconcileItem* deleteditem;
				BListView* itemlist;

				deleteditem = FindItemForID(fDepositList, id);
				if (deleteditem)
					itemlist = fDepositList;
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

				itemlist->RemoveItem(deleteditem);
				delete deleteditem;
			}
		} else if (value & WATCH_CREATE) {
			uint32 accountid;
			if (msg->FindInt32("accountid", (int32*)&accountid) != B_OK
				|| accountid != fAccount->GetID()) {
				if (unlock)
					Unlock();
				return;
			}

			TransactionData* data;
			if (msg->FindPointer("item", (void**)&data) == B_OK) {
				ReconcileItem* newitem = new ReconcileItem(*data);

				if (data->Type().TypeCode() == TRANS_DEP)
					InsertTransactionItem(fDepositList, newitem);
				else
					InsertTransactionItem(fChargeList, newitem);
			}
		}
	}
	if (unlock)
		Unlock();
}


bool
ReconcileWindow::QuitRequested()
{
	gDatabase.RemoveObserver(this);
	return true;
}


void
ReconcileWindow::ApplyChargesAndInterest()
{
	Fixed charge;
	if (strlen(fCharges->Text()) > 0
		&& gCurrentLocale.StringToCurrency(fCharges->Text(), charge) == B_OK) {
		TransactionData chargetrans(fAccount, fDate->Text(), "ATM", B_TRANSLATE("Bank charge"),
			fCharges->Text(), B_TRANSLATE("Bank charge"), NULL, TRANS_RECONCILED);
		gDatabase.AddTransaction(chargetrans);
	}

	Fixed interest;
	if (strlen(fInterest->Text()) > 0
		&& gCurrentLocale.StringToCurrency(fInterest->Text(), interest) == B_OK) {
		TransactionData interesttrans(fAccount, fDate->Text(), B_TRANSLATE("DEP"),
			B_TRANSLATE("Account interest"), fInterest->Text(), B_TRANSLATE("Account interest"),
			NULL, TRANS_RECONCILED);
		gDatabase.AddTransaction(interesttrans);
	}
}


bool
ReconcileWindow::AutoReconcile()
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
			ShowAlert(B_TRANSLATE("Date is missing"),
				B_TRANSLATE("You need to enter the date for the statement to 'Quick balance'."));
			return false;
		}
	}

	Fixed dep, chrg, chk, bankchrg, interest;
	BList list;

	if (strlen(fCharges->Text()) > 0) {
		if (gCurrentLocale.StringToCurrency(fCharges->Text(), bankchrg) == B_OK)
			bankchrg.Invert();
		else {
			ShowAlert(B_TRANSLATE("CapitalBe didn't understand the amount for 'Bank charges'"),
				B_TRANSLATE("There may be a typo or the wrong kind of currency symbol "
							"for this account."));
			return false;
		}
	}

	if (strlen(fInterest->Text()) > 0) {
		if (gCurrentLocale.StringToCurrency(fInterest->Text(), interest) != B_OK) {
			ShowAlert(B_TRANSLATE("CapitalBe didn't understand the amount for 'Interest earned'"),
				B_TRANSLATE("There may be a typo or the wrong kind of currency symbol "
							"for this account."));
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

	ShowAlert(B_TRANSLATE("Couldn't Quick balance"),
		B_TRANSLATE("Quick balance failed. This doesn't mean "
					"that you did something wrong - it's just that 'Quick balance' works on "
					"simpler cases in balancing an account than this one. Sorry."));
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

		if (itemdata->Date() < tempdata->Date()
			|| (itemdata->Date() == tempdata->Date()
				&& strcmp(itemdata->Payee(), tempdata->Payee()) < 1)) {
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
		case TRANS_DEP:
		{
			win->fDepositList->AddItem(new ReconcileItem(data));
			break;
		}

		case TRANS_ATM:
		case TRANS_XFER:
		default:
		{
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

ReconcileFilter::~ReconcileFilter()
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
