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
 *	waddlesplash (Augustin Cavalier)
 */
#include "AccountSettingsWindow.h"
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <MessageFilter.h>

#include "AutoTextControl.h"
#include "CBLocale.h"
#include "CalendarButton.h"
#include "Database.h"
#include "Help.h"
#include "MsgDefs.h"
#include "PrefWindow.h"
#include "Preferences.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Account"

// clang-format off
enum {
	M_EDIT_ACCOUNT_SETTINGS = 'east',
	M_DATA_CHANGED = 'dtch',
	M_NEGATIVE_AMOUNT = 'nega',
	M_TOGGLE_USE_DEFAULT = 'tgud'
};
// clang-format on

AccountSettingsWindow::AccountSettingsWindow(Account* account)
	: BWindow(BRect(0, 0, 1, 1), B_TRANSLATE("Account settings"), B_FLOATING_WINDOW_LOOK,
		  B_MODAL_APP_WINDOW_FEEL,
		  B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS
			  | B_CLOSE_ON_ESCAPE),
	  fAccount(account)
{
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	AddShortcut('Q', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

	fOpeningTransactionExists = false;

	if (fAccount == NULL)
		SetTitle(B_TRANSLATE("New account"));
	else
		fOpeningTransactionExists = _GetOpeningTransaction();

	fAccountName = new AutoTextControl("accname", B_TRANSLATE("Name:"),
		(fAccount ? fAccount->Name() : NULL), new BMessage(M_DATA_CHANGED));
	fAccountName->SetCharacterLimit(32);
	fAccountName->TextView()->SetExplicitMinSize(
		BSize(be_plain_font->StringWidth("QuiteALongAccountName"), B_SIZE_UNSET));

	fOpeningDate = new DateBox("opendate", NULL, NULL, new BMessage(M_DATA_CHANGED));
	CalendarButton* calendarButton = new CalendarButton(fOpeningDate);

	fOpeningAmount = new CurrencyBox("openamount", NULL, NULL, new BMessage(M_DATA_CHANGED));

	fNegativeButton = new BButton("\xcc\xb5", new BMessage(M_NEGATIVE_AMOUNT));
	fNegativeButton->SetBehavior(BButton::B_TOGGLE_BEHAVIOR);
	fNegativeButton->SetToolTip(B_TRANSLATE("Use a negative opening balance."));
	float height;
	fOpeningAmount->GetPreferredSize(NULL, &height);
	BSize size(height - 2, height);
	fNegativeButton->SetExplicitSize(size);

	if (fOpeningTransactionExists) {
		fOpeningDate->SetDate(fOpeningTransaction.Date());
		fOpeningDate->Validate();
		BString tempstr;
		bool negative = fOpeningTransaction.Amount() < 0;
		gCurrentLocale.CurrencyToString(
			negative ? fOpeningTransaction.Amount().InvertAsCopy() : fOpeningTransaction.Amount(),
			tempstr);
		fOpeningAmount->SetText(tempstr.String());
		if (negative) {
			fNegativeButton->SetValue(B_CONTROL_ON);
			PostMessage(M_NEGATIVE_AMOUNT);
		}
	}

	fUseDefault = new BCheckBox("usedefault", B_TRANSLATE("Use system currency format"),
		new BMessage(M_TOGGLE_USE_DEFAULT));
	if (!fAccount || fAccount->IsUsingDefaultLocale())
		fUseDefault->SetValue(B_CONTROL_ON);

	Locale templocale;
	if (fAccount)
		templocale = fAccount->GetLocale();
	fPrefView = new CurrencyPrefView("prefview", &templocale);

	HelpButton* helpButton = new HelpButton("start.html", "#new-account");

	fOK = new BButton("okbutton", B_TRANSLATE("OK"), new BMessage(M_EDIT_ACCOUNT_SETTINGS));

	if (strlen(fAccountName->Text()) < 1)
		fOK->SetEnabled(false);

	BButton* cancel
		= new BButton("cancelbutton", B_TRANSLATE("Cancel"), new BMessage(B_QUIT_REQUESTED));

	SetDefaultButton(fOK);

	if (!fAccount || fAccount->IsUsingDefaultLocale())
		fPrefView->Hide();

	// clang-format off
	BView* calendarWidget = new BView("calendarwidget", B_WILL_DRAW);
	BLayoutBuilder::Group<>(calendarWidget, B_HORIZONTAL, -2)
		.Add(fOpeningDate)
		.Add(calendarButton)
		.End();

	BView* amountWidget = new BView("amountwidget", B_WILL_DRAW);
	BLayoutBuilder::Group<>(amountWidget, B_HORIZONTAL, -2)
		.Add(fOpeningAmount)
		.Add(fNegativeButton)
		.End();

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGrid(B_USE_SMALL_SPACING)
			.Add(fAccountName->CreateLabelLayoutItem(), 0, 0)
			.Add(fAccountName->CreateTextViewLayoutItem(), 1, 0)
			.Add(new BStringView(NULL, B_TRANSLATE("Opening date:")), 0, 1)
			.Add(calendarWidget, 1, 1)
			.Add(new BStringView(NULL, B_TRANSLATE("Opening balance:")), 0, 2)
			.Add(amountWidget, 1, 2)
			.End()
		.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING)
			.Add(fUseDefault)
			.Add(fPrefView)
			.End()
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.AddGlue(0)
			.Add(helpButton)
			.AddGlue(1)
			.Add(cancel)
			.Add(fOK)
			.End()
		.End();
	// clang-format on

	CenterIn(Frame());
	fAccountName->MakeFocus(true);
}


void
AccountSettingsWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_PREVIOUS_FIELD:
		{
			if (fOpeningDate->ChildAt(0)->IsFocus())
				fAccountName->MakeFocus(true);
			else if (fOpeningAmount->ChildAt(0)->IsFocus())
				fOpeningDate->MakeFocus(true);
			break;
		}
		case M_NEXT_FIELD:
		{
			if (fOpeningDate->ChildAt(0)->IsFocus())
				fOpeningAmount->MakeFocus(true);
			else if (fOpeningAmount->ChildAt(0)->IsFocus())
				fUseDefault->MakeFocus(true);
			break;
		}
		case M_EDIT_ACCOUNT_SETTINGS:
		{
			_UpdateStates();
			if (!fOK->IsEnabled())
				break;

			Locale customLocale;
			fPrefView->GetSettings(customLocale);

			bool newAccount = false;
			if (!fAccount) {
				newAccount = true;
				gDatabase.AddAccount(fAccountName->Text(), ACCOUNT_BANK, "Open",
					fUseDefault->Value() == B_CONTROL_ON ? NULL : &customLocale);
				fAccount = gDatabase.AccountByName(fAccountName->Text());
			} else {
				if (strcmp(fAccountName->Text(), fAccount->Name()) != 0)
					gDatabase.RenameAccount(fAccount, fAccountName->Text());

				if (fUseDefault->Value() != B_CONTROL_ON) {
					fAccount->UseDefaultLocale(false);
					fAccount->SetLocale(customLocale);
				} else
					fAccount->UseDefaultLocale(true);
			}

			// Opening balance date and amount not empty, create opening transaction.
			if (strlen(fOpeningAmount->Text()) > 0 && strlen(fOpeningDate->Text()) > 0) {
				const char* type = fNegativeButton->Value() == B_CONTROL_OFF ? "DEP" : "ATM";
				fOpeningTransaction.Set(fAccount, fOpeningDate->Text(), type, NULL,
					fOpeningAmount->Text(), B_TRANSLATE_CONTEXT("Opening balance", "CommonTerms"),
					NULL, fOpeningTransaction.Status());
				try {
					if (fOpeningTransactionExists)
						gDatabase.RemoveTransaction(fOpeningTransaction.GetID());

					gDatabase.AddTransaction(fOpeningTransaction);
				} catch (CppSQLite3Exception& e) {
					debugger(e.errorMessage());
				}
			}

			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case M_TOGGLE_USE_DEFAULT:
		{
			bool useDefault = fUseDefault->Value() == B_CONTROL_ON;

			if (useDefault)
				fPrefView->Hide();
			else
				fPrefView->Show();

			break;
		}
		case M_DATA_CHANGED:
		{
			_UpdateStates();
			break;
		}
		case M_NEGATIVE_AMOUNT:
		{
			rgb_color color = fNegativeButton->Value() == B_CONTROL_OFF
								  ? ui_color(B_CONTROL_TEXT_COLOR)
								  : gNegativeColor;
			fOpeningAmount->TextView()->SetFontAndColor(be_plain_font, B_FONT_ALL, &color);
			break;
		}
		default:
		{
			BWindow::MessageReceived(msg);
			break;
		}
	}
}


bool
AccountSettingsWindow::_GetOpeningTransaction()
{
	BString command;
	BString category;
	CppSQLite3Buffer sqlBuf;

	category << "%" << B_TRANSLATE_CONTEXT("Opening balance", "CommonTerms") << "%";
	sqlBuf.format("%Q", category.String());	 // Make sure the string is escaped

	command << "SELECT * FROM account_" << fAccount->GetID() << " WHERE"
			<< " LOWER(category) LIKE LOWER(" << sqlBuf << ")"
			<< " ORDER BY date, payee;";

	CppSQLite3Query query = gDatabase.DBQuery(command.String(), "Find opening transaction");

	bool found = false;
	uint32 currentid = 0, newid = 0;
	if (!query.eof()) {
		found = true;
		currentid = query.getIntField(1);
		newid = query.getIntField(1);
		fOpeningTransaction.SetID(currentid);
		fOpeningTransaction.SetDate(atol(query.getStringField(2)));
		fOpeningTransaction.SetType(query.getStringField(3));
		fOpeningTransaction.SetPayee(query.getStringField(4));
		fOpeningTransaction.SetAccount(fAccount);

		Fixed f;
		f.SetPremultiplied(atol(query.getStringField(5)));
		fOpeningTransaction.AddCategory(query.getStringField(6), f, true);

		if (!query.fieldIsNull(7))
			fOpeningTransaction.SetMemoAt(
				fOpeningTransaction.CountCategories() - 1, query.getStringField(7));

		BString status = query.getStringField(8);
		if (status.ICompare("Reconciled") == 0)
			fOpeningTransaction.SetStatus(TRANS_RECONCILED);
		else if (status.ICompare("Cleared") == 0)
			fOpeningTransaction.SetStatus(TRANS_CLEARED);
		else
			fOpeningTransaction.SetStatus(TRANS_OPEN);
	}
	return found;
}


void
AccountSettingsWindow::_UpdateStates()
{
	bool nameEmpty = strlen(fAccountName->Text()) < 1;
	bool dateEmpty = strlen(fOpeningDate->Text()) < 1;
	bool amountEmpty = strlen(fOpeningAmount->Text()) < 1;

	fAccountName->MarkAsInvalid(nameEmpty);

	if (!dateEmpty)
		fOpeningDate->Validate();

	if (!amountEmpty)
		fOpeningAmount->Validate();

	if (dateEmpty && amountEmpty) {
		fOpeningDate->MarkAsInvalid(false);
		fOpeningAmount->MarkAsInvalid(false);
	} else {
		fOpeningDate->MarkAsInvalid(dateEmpty);
		fOpeningAmount->MarkAsInvalid(amountEmpty);
	}

	fOK->SetEnabled(!nameEmpty & !(dateEmpty ^ amountEmpty));
}