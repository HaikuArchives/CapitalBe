#include <Catalog.h>
#include <GridLayout.h>
#include <LayoutBuilder.h>
#include <Messenger.h>
#include <String.h>
#include <Window.h>
#include <stdlib.h>

#include "Account.h"
#include "CategoryBox.h"
#include "CheckNumBox.h"
#include "CheckView.h"
#include "CurrencyBox.h"
#include "DAlert.h"
#include "Database.h"
#include "DateBox.h"
#include "MainWindow.h"
#include "MsgDefs.h"
#include "NavTextBox.h"
#include "PayeeBox.h"
#include "Preferences.h"
#include "TimeSupport.h"
#include "TransactionData.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CheckView"


enum
{
	M_ENTER_TRANSACTION = 'entr'
};

CheckView::CheckView(const char* name, int32 flags) : BView(name, flags | B_FRAME_EVENTS)
{
	fDateLabel = new BStringView("datelabel", B_TRANSLATE("Date"));
	fDate = new DateBox("dateentry", "", NULL, new BMessage(M_DATE_CHANGED));

	fTypeLabel = new BStringView("typelabel", B_TRANSLATE("Type"));
	fType = new CheckNumBox("typeentry", "", NULL, new BMessage(M_TYPE_CHANGED));

	fPayeeLabel = new BStringView("payeelabel", B_TRANSLATE("Payee"));
	fPayee = new PayeeBox("payeeentry", "", NULL, new BMessage(M_PAYEE_CHANGED));

	fAmountLabel = new BStringView("amountlabel", B_TRANSLATE("Amount"));
	fAmount = new CurrencyBox("amountentry", "", "", new BMessage(M_AMOUNT_CHANGED));

	fCategoryLabel = new BStringView("categorylabel", B_TRANSLATE("Category"));
	fCategory = new CategoryBox("categoryentry", "", NULL, new BMessage(M_CATEGORY_CHANGED));

	fMemoLabel = new BStringView("memolabel", B_TRANSLATE("Memo"));
	fMemo = new NavTextBox("memoentry", "", NULL, new BMessage(M_MEMO_CHANGED));
	fMemo->TextView()->DisallowChar(B_ESCAPE);
	fMemo->SetCharacterLimit(21);

	prefsLock.Lock();
	BString rechelp = gAppPath;
	prefsLock.Unlock();
	rechelp << "helpfiles/" << B_TRANSLATE_COMMENT("English","Path to localized helpfiles. Only translate if available in your language.") <<  "/Main Window Help";
	fHelpButton = new HelpButton("rechelp", rechelp.String());

	fEnter = new BButton("enterbutton", B_TRANSLATE("Enter"), new BMessage(M_ENTER_TRANSACTION));

#ifndef ENTER_NAVIGATION
	fEnter->MakeDefault(true);
#endif

	//	#ifndef ENTER_NAVIGATION
	//	fEnter->MoveBy(0,-8);
	//	#endif

	gDatabase.AddObserver(this);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(0)
		.AddGrid(1.0f, 0.0f)
		.Add(fDateLabel, 0, 0)
		.Add(fDate, 0, 1, 2)
		.Add(fTypeLabel, 2, 0)
		.Add(fType, 2, 1)
		.Add(fPayeeLabel, 3, 0)
		.Add(fPayee, 3, 1, 2)
		.Add(fAmountLabel, 5, 0)
		.Add(fAmount, 5, 1, 2)
		.Add(fCategoryLabel, 0, 2)
		.Add(fCategory, 0, 3, 3)
		.Add(fMemoLabel, 3, 2)
		.Add(fMemo, 3, 3, 4)
		.Add(fHelpButton, 0, 5)
		.Add(fEnter, 6, 5)
		.End()
		.End();
}

CheckView::~CheckView(void) {}

void
CheckView::AttachedToWindow(void)
{
	SetViewColor(Parent()->ViewColor());
	fDate->GetFilter()->SetMessenger(new BMessenger(this));
	fPayee->GetFilter()->SetMessenger(new BMessenger(this));
	fType->GetFilter()->SetMessenger(new BMessenger(this));
	fAmount->GetFilter()->SetMessenger(new BMessenger(this));
	fCategory->GetFilter()->SetMessenger(new BMessenger(this));
	fMemo->GetFilter()->SetMessenger(new BMessenger(this));

	fEnter->SetTarget(this);
}

void
CheckView::MessageReceived(BMessage* msg)
{
	int32 start;
	BString string;
	switch (msg->what) {
		case M_TYPE_AUTOCOMPLETE:
		{
			msg->FindInt32("start", &start);
			msg->FindString("string", &string);
			fType->SetText(string.String());
			fType->TextView()->Select(start, string.CountChars());
			break;
		}
		case M_PAYEE_AUTOCOMPLETE:
		{
			msg->FindInt32("start", &start);
			msg->FindString("string", &string);
			fPayee->SetText(string.String());
			fPayee->TextView()->Select(start, string.CountChars());
			break;
		}
		case M_CATEGORY_AUTOCOMPLETE:
		{
			msg->FindInt32("start", &start);
			msg->FindString("string", &string);
			fCategory->SetText(string.String());
			fCategory->TextView()->Select(start, string.CountChars());
			break;
		}

#ifdef ENTER_NAVIGATION

		case M_ENTER_NAVIGATION:
		{
			if (!fMemo->ChildAt(0)->IsFocus()) {
				DoNextField();
				break;
			}

			// fall through to the next case
		}

#endif

		case M_ENTER_TRANSACTION:
		{
			// The text filter sends this message whenever the user hits Enter
			// from the Memo field. The CheckView instance should do whatever is
			// needed to post the transaction into the register

			if (!fDate->Validate() || !fType->Validate() || !fPayee->Validate() ||
				!fAmount->Validate() || !fCategory->Validate())
				break;

			Account* acc = gDatabase.CurrentAccount();
			if (!acc)
				break;

			TransactionData trans(acc, fDate->Text(), fType->Text(), fPayee->Text(),
				fAmount->Text(), fCategory->Text(), fMemo->Text(), real_time_clock_usecs());


			gDatabase.AddTransaction(trans);
			acc->SetCurrentTransaction(trans.GetID());

			if (trans.Type().TypeCode() == TRANS_NUMERIC)
				acc->SetLastCheckNumber(atol(trans.Type().Type()));

			MakeEmpty();

			gDatabase.GetTransaction(trans.GetID(), trans);
			fDate->SetDate(trans.Date());

			BString str;
			gDefaultLocale.DateToString(fDate->GetDate(), str);
			fDate->SetText(str.String());
			fDate->MakeFocus(true);
			break;
		}
		case M_PREVIOUS_FIELD:
		{
			if (fDate->ChildAt(0)->IsFocus()) {
				if (fDate->Validate(false)) {
					if (gDatabase.CurrentAccount() && strlen(fDate->Text()) > 0) {
						time_t date;
						gDefaultLocale.StringToDate(fDate->Text(), date);
						fDate->SetDate(date);
					}
					fMemo->MakeFocus(true);
				}
				break;
			} else if (fType->ChildAt(0)->IsFocus())
				fDate->MakeFocus(true);
			else if (fPayee->ChildAt(0)->IsFocus()) {
				if (fPayee->Validate(false))
					fType->MakeFocus(true);
			} else if (fAmount->ChildAt(0)->IsFocus()) {
				if (fAmount->Validate(false))
					fPayee->MakeFocus(true);
			} else if (fCategory->ChildAt(0)->IsFocus()) {
				//				if(fCategory->Validate())
				fAmount->MakeFocus(true);
			}
			if (fMemo->ChildAt(0)->IsFocus())
				fCategory->MakeFocus(true);
			break;
		}
		case M_NEXT_FIELD:
		{
			DoNextField();
			break;
		}
		default:
		{
			BView::MessageReceived(msg);
		}
	}
}

void
CheckView::SetFields(const char* date, const char* type, const char* payee, const char* amount,
	const char* category, const char* memo)
{
	fDate->SetText(date);
	fType->SetText(type);
	fPayee->SetText(payee);
	fAmount->SetText(amount);
	fCategory->SetText(category);
	fMemo->SetText(memo);
}

void
CheckView::HandleNotify(const uint64& value, const BMessage* msg)
{
	if (value & WATCH_SELECT) {
		if (value & WATCH_ACCOUNT) {
			MakeEmpty();

			Account* account;
			if (msg->FindPointer("item", (void**)&account) == B_OK) {
				if (account && !account->HasObserver(this))
					account->AddObserver(this);
			}

			if (!account) {
				// This happens when the last account is deleted -- there
				// is no longer a current account
				fDate->MakeFocus(true);
				return;
			}


			if (account->CountTransactions() > 0) {
				TransactionData data;
				gDatabase.GetTransaction(account->GetID(), data);
				fDate->SetDate(data.Date());

				BString text;
				gDefaultLocale.DateToString(data.Date(), text);
				fDate->SetText(text.String());
			} else {
				BString text;
				gDefaultLocale.DateToString(GetCurrentDate(), text);
				fDate->SetText(text.String());
			}

			fDate->MakeFocus(true);
		} else if (value & WATCH_TRANSACTION) {
			uint32 id;
			if (msg->FindInt32("id", (int32*)&id) == B_OK) {
				if (gDatabase.CurrentAccount()) {
					TransactionData data;
					gDatabase.GetTransaction(id, data);
					fDate->SetDate(data.Date());

					BString text;
					gDefaultLocale.DateToString(data.Date(), text);
					fDate->SetText(text.String());
				}
			}
		}
	}
}

void
CheckView::MakeEmpty(void)
{
	fDate->SetText("");
	fType->SetText("");
	fPayee->SetText("");
	fAmount->SetText("");
	fCategory->SetText("");
	fMemo->SetText("");
}

void
CheckView::MakeFocus(bool value)
{
	fDate->MakeFocus(value);
}

void
CheckView::FrameResized(float width, float height)
{
}

void
CheckView::DoNextField(void)
{
	if (fDate->ChildAt(0)->IsFocus()) {
		if (fDate->Validate(false)) {
			if (gDatabase.CurrentAccount() && strlen(fDate->Text()) > 0) {
				time_t date;
				gDefaultLocale.StringToDate(fDate->Text(), date);
				fDate->SetDate(date);
			}
		}
		fType->MakeFocus(true);
	} else if (fType->ChildAt(0)->IsFocus())
		fPayee->MakeFocus(true);
	else if (fPayee->ChildAt(0)->IsFocus()) {
		if (fPayee->Validate(false))
			fAmount->MakeFocus(true);
	} else if (fAmount->ChildAt(0)->IsFocus()) {
		if (fAmount->Validate(false))
			fCategory->MakeFocus(true);
	} else if (fCategory->ChildAt(0)->IsFocus()) {
		// TODO: don't force entering a transaction when going to the
		// split window via key editing
		if (strcmp(fCategory->Text(), "Split") == 0) {
			Window()->PostMessage(M_ENTER_TRANSACTION, this);
			Window()->PostMessage(M_EDIT_TRANSACTION);
			return;
		}

		fMemo->MakeFocus(true);
	} else if (fMemo->ChildAt(0)->IsFocus()) {
		fEnter->MakeFocus(true);
	} else {
		// We should *never* be here
		ShowBug("M_NEXT_FIELD received for unknown view in CheckView");
	}
}
