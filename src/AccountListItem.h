#ifndef ACCOUNTLISTITEM_H
#define ACCOUNTLISTITEM_H

#include <ListItem.h>

class Account;

class AccountListItem : public BListItem {
  public:
	AccountListItem(Account *acc);
	void SetEnabled(bool enabled);

	bool IsEnabled(void) const { return fEnabled; }

	void DrawItem(BView *owner, BRect frame, bool complete = false);
	void Update(BView *owner, const BFont *finfo);

	Account *GetAccount(void) const { return fAccount; }

  private:
	Account *fAccount;
	bool fEnabled;
};

#endif
