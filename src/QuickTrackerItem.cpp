/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	bitigchi (Emir Sari)
 *	dospuntos (Johan Wagenheim)
 *	raefaldhia (Raefaldhi Amartya Junior)
 */
#include "QuickTrackerItem.h"
#include "Account.h"
#include "CBLocale.h"
#include "Database.h"
#include "Fixed.h"

#include <Catalog.h>
#include <String.h>

#include <vector>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"


// Calculates the user's net worth by adding up the balances of all accounts
QTNetWorthItem::QTNetWorthItem(const char* name, uint32 flags)
	: QuickTrackerItem(name, flags),
	  fIgnore(false)
{
	for (int32 i = 0; i < gDatabase.CountAccounts(); i++) {
		Account* account = gDatabase.AccountAt(i);
		account->AddObserver(this);
	}
}


QTNetWorthItem::~QTNetWorthItem()
{
	for (int32 i = 0; i < gDatabase.CountAccounts(); i++) {
		Account* account = gDatabase.AccountAt(i);
		account->RemoveObserver(this);
	}
}


void
QTNetWorthItem::AttachedToWindow()
{
	QuickTrackerItem::AttachedToWindow();
	_Calculate();
}


void
QTNetWorthItem::SetObserving(const bool& value)
{
	if (IsObserving() != value) {
		Observer::SetObserving(value);
		_Calculate();
	}
}


void
QTNetWorthItem::HandleNotify(const uint64& value, const BMessage* msg)
{
	/*
		TODO: Make QTNetWorthItem ignore mass edits
		if(value & WATCH_MASS_EDIT)
		{
			// Something which will massively change the database will happen. We will
			// receive two of them, so we will start ignoring when we receive the first
			// one and stop the second time. We do this because each call to Calculate
			// resultes in a hit on the database for each account
			fIgnore = !fIgnore;
		}

		if(fIgnore)
			return;
	*/
	if (value & WATCH_ACCOUNT) {
		Account* acc = NULL;
		if (msg->FindPointer("item", (void**)&acc) != B_OK)
			return;

		if (value & WATCH_CREATE) {
			acc->AddObserver(this);
			_Calculate();
		} else if (value & WATCH_DELETE) {
			acc->RemoveObserver(this);
			if (gDatabase.CountAccounts() >= 1) {
				if (Window())
					Window()->Lock();

				_Calculate();

				if (Window())
					Window()->Unlock();
				return;
			}
		} else if (value & WATCH_LOCALE)
			_Calculate();
	}
	if (value & WATCH_SELECT)
		_Calculate();
}


void
QTNetWorthItem::_Calculate()
{
	BString balanceText, balanceLabel;
	Fixed balance;

	if (gDatabase.CountAccounts() == 0) {  // No accounts
		if (Window())
			Window()->Lock();

		SetText(B_TRANSLATE("<No accounts>"));

		if (Window())
			Window()->Unlock();
		return;
	}

	BString command;
	std::vector<BString> currencies;
	BString currency;
	CppSQLite3Query query;


	// Get list of currencies
	command = "SELECT DISTINCT currencysymbol FROM accountlocale";
	query = gDatabase.DBQuery(command.String(), "Database::Calculate");

	while (!query.eof()) {
		currency = query.getStringField(0);
		currencies.push_back(currency);
		query.nextRow();
	}

	// Get sum of default currency accounts that are open:
	command
		= "SELECT a.accountid FROM accountlist AS a LEFT JOIN accountlocale AS al ON "
		  "a.accountid = al.accountid WHERE al.accountid IS NULL AND a.status = \"Open\" "
		  "OR a.status = \"open\";";
	query = gDatabase.DBQuery(command.String(), "Database::Calculate");

	balance = 0;
	bool accountsFound = false;
	while (!query.eof()) {
		accountsFound = true;
		Account* account = gDatabase.AccountByID(query.getIntField(0));
		balance += account->Balance();
		query.nextRow();
	}

	if (accountsFound && gDefaultLocale.CurrencyToString(balance, balanceText) == B_OK)
		balanceLabel << B_TRANSLATE("Balance") << ": " << balanceText << "\n";

	// Get sum of other currency accounts:
	for (int32 i = 0; i < currencies.size(); i++) {
		command
			= "SELECT a1.accountid FROM accountlist AS a1 JOIN accountlocale AS a2 ON "
			  "a1.accountid=a2.accountid WHERE a2.currencysymbol = \"";
		command << currencies.at(i).String() << "\" AND a1.status = \"Open\";";
		query = gDatabase.DBQuery(command.String(), "Database::Calculate");

		balance = 0;
		Locale accLocale;

		while (!query.eof()) {
			Account* account = gDatabase.AccountByID(query.getIntField(0));
			balance += account->Balance();
			accLocale = account->GetLocale();
			query.nextRow();
		}

		if (accLocale.CurrencyToString(balance, balanceText) == B_OK)
			balanceLabel << B_TRANSLATE("Balance") << ": " << balanceText << "\n";
	}

	if (Window()) {
		Window()->Lock();
		SetText(balanceLabel.String());
		Invalidate();
		Window()->Unlock();
	}
}

// Calculates the budget variance for one category
QTBudgetCategoryItem::QTBudgetCategoryItem(const char* category, const char* name, uint32 flags)
	: QuickTrackerItem(name, flags),
	  fIgnore(false)
{
	for (int32 i = 0; i < gDatabase.CountAccounts(); i++) {
		Account* account = gDatabase.AccountAt(i);
		account->AddObserver(this);
	}

	gDatabase.GetBudgetEntry(category, fEntry);
}


QTBudgetCategoryItem::~QTBudgetCategoryItem()
{
	for (int32 i = 0; i < gDatabase.CountAccounts(); i++) {
		Account* account = gDatabase.AccountAt(i);
		account->RemoveObserver(this);
	}
}


void
QTBudgetCategoryItem::AttachedToWindow()
{
	QuickTrackerItem::AttachedToWindow();
	_Calculate();
}


void
QTBudgetCategoryItem::SetObserving(const bool& value)
{
	if (IsObserving() != value) {
		Observer::SetObserving(value);
		_Calculate();
	}
}


void
QTBudgetCategoryItem::HandleNotify(const uint64& value, const BMessage* msg)
{
	/*
		TODO: Make QTBudgetCategoryItem ignore mass edits
		if(value & WATCH_MASS_EDIT)
		{
			// Something which will massively change the database will happen. We will
			// receive two of them, so we will start ignoring when we receive the first
			// one and stop the second time. We do this because each call to Calculate
			// resultes in a hit on the database for each account
			fIgnore = !fIgnore;
		}

		if(fIgnore)
			return;
	*/
	if (value & WATCH_ACCOUNT) {
		Account* acc = NULL;
		if (msg->FindPointer("item", (void**)&acc) != B_OK)
			return;

		if (value & WATCH_CREATE) {
			acc->AddObserver(this);
		} else if (value & WATCH_DELETE) {
			acc->RemoveObserver(this);
			if (gDatabase.CountAccounts() == 1) {
				if (Window())
					Window()->Lock();

				BString label, temp;
				temp << B_TRANSLATE("Account total") << ": ";
				if (gCurrentLocale.CurrencyToString(Fixed(), label) == B_OK)
					temp << label;
				SetText(temp.String());

				if (Window())
					Window()->Unlock();
				return;
			}
		}
	}
	if (!(value & WATCH_SELECT))
		_Calculate();
}


void
QTBudgetCategoryItem::_Calculate()
{
	BString label, temp;
	Fixed variance;

	temp << B_TRANSLATE("Budget") << ": " << fEntry.name;

	if (gDefaultLocale.CurrencyToString(variance, label) == B_OK) {
		temp << " \n" << label;

		if (Window()) {
			Window()->Lock();
			SetText(label.String());
			Invalidate();
			Window()->Unlock();
		}
	}
}


QuickTrackerItem::QuickTrackerItem(const char* name, uint32 flags)
	: BTextView(name, flags)
{
	MakeEditable(false);
	MakeSelectable(false);
	gDatabase.AddObserver(this);
}


QuickTrackerItem::~QuickTrackerItem()
{
	gDatabase.RemoveObserver(this);
}


void
QuickTrackerItem::AttachedToWindow()
{
	SetViewUIColor(Parent()->ViewUIColor());
}


void
QuickTrackerItem::HandleNotify(const uint64& value, const BMessage* msg)
{
	// Does nothing by default - hook function for child classes
}


void
QuickTrackerItem::Configure()
{
	// Does nothing by default - hook function for child classes
}
