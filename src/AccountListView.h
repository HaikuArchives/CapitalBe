#ifndef ACCOUNTLISTITEM_H
#define ACCOUNTLISTITEM_H

#include <ListItem.h>
#include <ListView.h>
#include <PopUpMenu.h>

class Account;


class AccountList : public BListView {
public:
	AccountList(void);
	~AccountList(void);

	virtual	void MessageReceived(BMessage* message);
	void MouseDown(BPoint position);

private:
	void ShowPopUpMenu(BPoint screen);
	bool fShowingPopUpMenu;
};


class AccountContext : public BPopUpMenu {
public:
	AccountContext(const char* name, BMessenger target);
	virtual ~AccountContext(void);

private:
	BMessenger fTarget;
};


class AccountListItem : public BListItem {
public:
	AccountListItem(Account* acc);
	void SetEnabled(bool enabled);

	bool IsEnabled(void) const { return fEnabled; }

	void DrawItem(BView* owner, BRect frame, bool complete = false);
	void Update(BView* owner, const BFont* finfo);

	Account* GetAccount(void) const { return fAccount; }

private:
	Account* fAccount;
	bool fEnabled;
};

#endif
