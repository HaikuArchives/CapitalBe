/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	bitigchi (Emir Sari)
 *	humdinger (Joachim Seemer)
 *	dospuntos (Johan Wagenheim)
 *	emily
 *	raefaldhia
 */
#include <Catalog.h>
#include <DateFormat.h>
#include <GridLayout.h>
#include <LayoutBuilder.h>
#include <Messenger.h>
#include <String.h>
#include <Window.h>

#include <stdlib.h>

#include "Account.h"
#include "CalendarButton.h"
#include "CategoryBox.h"
#include "CategoryButton.h"
#include "CheckView.h"
#include "CurrencyBox.h"
#include "Database.h"
#include "DateBox.h"
#include "MsgDefs.h"
#include "NavTextBox.h"
#include "PayeeBox.h"
#include "TimeSupport.h"
#include "TransactionData.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CheckView"

// clang-format off
enum {
	M_ENTER_TRANSACTION = 'entr'
};
// clang-format on

CheckView::CheckView(const char* name, int32 flags)
	: BView(name, flags | B_FRAME_EVENTS)
{
	BStringView* dateLabel
		= new BStringView("datelabel", B_TRANSLATE_CONTEXT("Date", "CommonTerms"));
	dateLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fDate = new DateBox("dateentry", "", NULL, new BMessage(M_DATE_CHANGED));

	CalendarButton* calendarButton = new CalendarButton(fDate);

	BStringView* payeeLabel
		= new BStringView("payeelabel", B_TRANSLATE_CONTEXT("Payee", "CommonTerms"));
	payeeLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fPayee = new PayeeBox("payeeentry", "", NULL, new BMessage(M_PAYEE_CHANGED));
	fPayee->SetCharacterLimit(63);

	BStringView* amountLabel
		= new BStringView("amountlabel", B_TRANSLATE_CONTEXT("Amount", "CommonTerms"));
	amountLabel->SetExplicitSize(BSize(StringWidth("$10,000,000,000.00"), B_SIZE_UNSET));
	fAmount = new CurrencyBox("amountentry", "", "", new BMessage(M_AMOUNT_CHANGED));

	BStringView* categoryLabel
		= new BStringView("categorylabel", B_TRANSLATE_CONTEXT("Category", "CommonTerms"));
	categoryLabel->SetExplicitSize(BSize(StringWidth("aVeryLongCategoryName"), B_SIZE_UNSET));
	fCategory = new CategoryBox("categoryentry", "", NULL, new BMessage(M_CATEGORY_CHANGED));

	CategoryButton* categoryButton = new CategoryButton(fCategory);

	BStringView* memoLabel
		= new BStringView("memolabel", B_TRANSLATE_CONTEXT("Memo", "CommonTerms"));
	memoLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fMemo = new NavTextBox("memoentry", "", NULL, new BMessage(M_MEMO_CHANGED));
	fMemo->TextView()->DisallowChar(B_ESCAPE);
	fMemo->SetCharacterLimit(63);

	fEnter = new BButton("enterbutton", B_TRANSLATE("Enter"), new BMessage(M_ENTER_TRANSACTION));
	fEnter->SetExplicitMaxSize(BSize(B_SIZE_UNSET, B_SIZE_UNLIMITED));

#ifndef ENTER_NAVIGATION
	fEnter->MakeDefault(true);
#endif

	//	#ifndef ENTER_NAVIGATION
	//	fEnter->MoveBy(0,-8);
	//	#endif

	gDatabase.AddObserver(this);

	// clang-format off
	BView* calendarWidget = new BView("calendarwidget", B_WILL_DRAW);
	BLayoutBuilder::Group<>(calendarWidget, B_HORIZONTAL, -2)
		.Add(fDate)
		.Add(calendarButton)
		.End();

	BView* categoryWidget = new BView("categorywidget", B_WILL_DRAW);
	BLayoutBuilder::Group<>(categoryWidget, B_HORIZONTAL, -2)
		.Add(fCategory)
		.Add(categoryButton)
		.End();

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(0)
		.AddGrid(1.0f, 0.0f)
			.SetColumnWeight(2, 2.0f)
			.SetColumnWeight(3, 1.0f)
			.Add(dateLabel, 0, 0)
			.Add(calendarWidget, 0, 1)
			.Add(payeeLabel, 1, 0)
			.Add(fPayee, 1, 1)
			.Add(amountLabel, 2, 0)
			.Add(fAmount, 2, 1)
			.Add(categoryLabel, 0, 2)
			.Add(categoryWidget, 0, 3)
			.Add(memoLabel, 1, 2, 2)
			.Add(fMemo, 1, 3, 2)
			.Add(fEnter, 3, 1, 1, 3)
			.End()
		.End();
	// clang-format on
}


CheckView::~CheckView()
{
	gDatabase.RemoveObserver(this);
	for (int32 i = 0; i < gDatabase.CountAccounts(); i++) {
		Account* account = gDatabase.AccountAt(i);
		account->RemoveObserver(this);
	}
}


void
CheckView::AttachedToWindow()
{
	SetViewColor(Parent()->ViewColor());
	fDate->GetFilter()->SetMessenger(new BMessenger(this));
	fPayee->GetFilter()->SetMessenger(new BMessenger(this));
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
		case M_PAYEE_AUTOCOMPLETE:
		{
			msg->FindInt32("start", &start);
			msg->FindString("string", &string);
			fPayee->SetText(string.String());
			fPayee->TextView()->Select(start, string.Length());
			break;
		}
		case M_CATEGORY_AUTOCOMPLETE:
		{
			msg->FindInt32("start", &start);
			msg->FindString("string", &string);
			fCategory->SetText(string.String());
			fCategory->TextView()->Select(start, string.Length());
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

			if (!fDate->Validate() || !fPayee->Validate() || !fAmount->Validate()
				|| !fCategory->Validate())
				break;

			Account* acc = gDatabase.CurrentAccount();
			if (!acc)
				break;

			TransactionData trans(acc, fDate->Text(), fCategory->GetType(), fPayee->Text(),
				fAmount->Text(), fCategory->Text(), fMemo->Text(), real_time_clock_usecs());

			gDatabase.AddTransaction(trans);
			acc->SetCurrentTransaction(trans.GetID());

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
			} else if (fPayee->ChildAt(0)->IsFocus()) {
				if (fPayee->Validate(false))
					fDate->MakeFocus(true);
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
			_DoNextField();
			break;
		}
		default:
		{
			BView::MessageReceived(msg);
		}
	}
}


void
CheckView::ClearAllFields()
{
	fDate->SetText("");
	fPayee->SetText("");
	fAmount->SetText("");
	fCategory->SetText("");
	fMemo->SetText("");
}


void
CheckView::SetFields(const char* date, const char* type, const char* payee, const char* amount,
	const char* category, const char* memo)
{
	time_t tDate;
	gDefaultLocale.StringToDate(date, tDate);
	fDate->SetDate(tDate);
	fDate->SetText(date);
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
					ClearAllFields();

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
CheckView::MakeEmpty()
{
	fDate->SetText("");
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
CheckView::_DoNextField()
{
	if (fDate->ChildAt(0)->IsFocus()) {
		if (fDate->Validate(false)) {
			if (gDatabase.CurrentAccount() && strlen(fDate->Text()) > 0) {
				time_t date;
				gDefaultLocale.StringToDate(fDate->Text(), date);
				fDate->SetDate(date);
			}
		}
		fPayee->MakeFocus(true);
	} else if (fPayee->ChildAt(0)->IsFocus()) {
		if (fPayee->Validate(false))
			fAmount->MakeFocus(true);
	} else if (fAmount->ChildAt(0)->IsFocus()) {
		if (fAmount->Validate(false))
			fCategory->MakeFocus(true);
	} else if (fCategory->ChildAt(0)->IsFocus()) {
		fMemo->MakeFocus(true);
	} else if (fMemo->ChildAt(0)->IsFocus()) {
		fEnter->MakeFocus(true);
	} else {
		// We should *never* be here
		ShowBug("M_NEXT_FIELD received for unknown view in CheckView");
	}
}


void
CheckView::SetFieldsEnabled(bool enabled)
{
	fDate->SetEnabled(enabled);
	if (fDate->LockLooper())
		fDate->MakeFocus(enabled);
	fDate->UnlockLooper();

	fPayee->SetEnabled(enabled);
	fAmount->SetEnabled(enabled);
	fCategory->SetEnabled(enabled);
	fMemo->SetEnabled(enabled);
	fEnter->SetEnabled(enabled);
}
