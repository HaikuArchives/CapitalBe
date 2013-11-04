#ifndef REGSITERVIEW_H
#define REGSITERVIEW_H

#include <View.h>
#include <ListView.h>
#include <Box.h>
#include "Notifier.h"
#include "TransactionView.h"

class CheckView;

#define M_SHOW_ACCOUNT_SETTINGS 'acst'

class RegisterView : public BView, public Observer
{
public:
	RegisterView(BRect frame, const char *name, int32 resize, int32 flags);
	~RegisterView(void);
	void MessageReceived(BMessage *msg);
	void AttachedToWindow(void);
	void HandleNotify(const uint64 &value, const BMessage *msg);
	void SelectAccount(const int32 &index);
	
	bool SelectNextTransaction(void) { return fTransactionView->SelectNext(); }
	bool SelectPreviousTransaction(void) { return fTransactionView->SelectPrevious(); }
	bool SelectFirstTransaction(void) { return fTransactionView->SelectFirst(); }
	bool SelectLastTransaction(void) { return fTransactionView->SelectLast(); }
	
private:
	CheckView *fCheckView;
	BListView *fAccountView;
	BScrollView *fAccountScroller;
	TransactionView *fTransactionView;
	BBox *fTrackBox;
};

#endif
