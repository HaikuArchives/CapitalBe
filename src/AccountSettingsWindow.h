#ifndef NEW_ACCOUNT_WINDOW_H
#define NEW_ACCOUNT_WINDOW_H

#include <Window.h>
#include <View.h>
#include <Button.h>
#include <CheckBox.h>
#include "Fixed.h"

class AutoTextControl;
class CurrencyPrefView;
class Account;

class AccountSettingsWindow : public BWindow
{
public:
	AccountSettingsWindow(Account *name);
	void MessageReceived(BMessage *msg);
	
private:
	friend class NewAccountFilter;
	
	AutoTextControl *fAccountName;
	BButton *fCancel, *fOK;
	CurrencyPrefView *fPrefView;
	Account *fAccount;
	BCheckBox	*fUseDefault;
};

#endif
