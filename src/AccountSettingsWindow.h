/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	humdinger (Joachim Seemer)
 */
#ifndef NEW_ACCOUNT_WINDOW_H
#define NEW_ACCOUNT_WINDOW_H

#include <Button.h>
#include <CheckBox.h>
#include <View.h>
#include <Window.h>

#include "CurrencyBox.h"
#include "DateBox.h"
#include "Fixed.h"
#include "TransactionData.h"

class AutoTextControl;
class CurrencyPrefView;
class Account;

class AccountSettingsWindow : public BWindow {
public:
	AccountSettingsWindow(Account* name);
	void MessageReceived(BMessage* msg);

private:
	friend class NewAccountFilter;

	bool _GetOpeningTransaction();
	void _UpdateStates();

	AutoTextControl* fAccountName;
	DateBox* fOpeningDate;
	CurrencyBox* fOpeningAmount;
	BButton* fOK;
	CurrencyPrefView* fPrefView;
	Account* fAccount;
	TransactionData fOpeningTransaction;
	BCheckBox* fUseDefault;
};

#endif	// NEW_ACCOUNT_WINDOW_H
