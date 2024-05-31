#ifndef NEW_ACCOUNT_WINDOW_H
#define NEW_ACCOUNT_WINDOW_H

#include <Button.h>
#include <CheckBox.h>
#include <View.h>
#include <Window.h>
#include "Fixed.h"
#include "LocaleView.h"

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
	LocaleView* fLocaleView;
	Account* fAccount;
	BCheckBox* fUseDefault;
};

#endif
