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

#include "Fixed.h"
#include <Button.h>
#include <CheckBox.h>
#include <View.h>
#include <Window.h>

class AutoTextControl;
class CurrencyPrefView;
class Account;

class AccountSettingsWindow : public BWindow {
public:
	AccountSettingsWindow(Account* name);
	void MessageReceived(BMessage* msg);

private:
	friend class NewAccountFilter;

	AutoTextControl* fAccountName;
	BButton* fOK;
	CurrencyPrefView* fPrefView;
	Account* fAccount;
	BCheckBox* fUseDefault;
};

#endif // NEW_ACCOUNT_WINDOW_H
