#ifndef TRANSFERWINDOW_H
#define TRANSFERWINDOW_H

#include "Account.h"
#include "AccountListItem.h"
#include "Fixed.h"
#include <Button.h>
#include <ListView.h>
#include <StringView.h>
#include <TextControl.h>
#include <View.h>
#include <Window.h>
#include <time.h>

#define M_CREATE_TRANSFER 'crtn'
#define M_EDIT_TRANSFER 'edtn'

class DateBox;
class CurrencyBox;

class TransferWindow : public BWindow {
  public:
	TransferWindow(BHandler *target);
	TransferWindow(BHandler *target, Account *src, Account *dest, const Fixed &amount);
	void MessageReceived(BMessage *msg);
	void SetMessage(BMessage msg);

  private:
	friend class TransferFilter;

	void InitObject(Account *src, Account *dest, const Fixed &amount);
	void HandleOKButton(void);

	BTextControl *fMemo;
	DateBox *fDate;
	CurrencyBox *fAmount;

	BButton *fCancel, *fOK;
	BStringView *fFromLabel, *fToLabel;
	BListView *fSourceList, *fDestList;

	BMessenger fMessenger;
	BMessage fMessage;
};

#endif
