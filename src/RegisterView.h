#ifndef REGSITERVIEW_H
#define REGSITERVIEW_H

#include <Box.h>
#include <ListView.h>
#include <View.h>
#include "Notifier.h"
#include "TransactionView.h"

class CheckView;
class FilterView;

enum {
	M_SHOW_ACCOUNT_SETTINGS = 'acst',
	M_SELECT_ACCOUNT = 'slac',
	M_SELECT_CURRENT = 'slcu',
};

typedef enum {
	ALL_TRANSACTIONS = 0,
	THIS_MONTH,
	LAST_MONTH,
	THIS_QUARTER,
	LAST_QUARTER,
	THIS_YEAR,
	LAST_YEAR
} filter_period_field;


class RegisterView : public BView, public Observer {
public:
	RegisterView(const char* name, int32 flags);
	~RegisterView(void);
	void MessageReceived(BMessage* msg);
	void AttachedToWindow(void);
	void HandleNotify(const uint64& value, const BMessage* msg);
	void SelectAccount(const int32& index);

	bool SelectNextTransaction(void) { return fTransactionView->SelectNext(); }

	bool SelectPreviousTransaction(void) { return fTransactionView->SelectPrevious(); }

	bool SelectFirstTransaction(void) { return fTransactionView->SelectFirst(); }

	bool SelectLastTransaction(void) { return fTransactionView->SelectLast(); }

private:
	CheckView* fCheckView;
	FilterView* fFilterView;
	BListView* fAccountView;
	BScrollView* fAccountScroller;
	BStringView* fTransactionlabel;
	TransactionView* fTransactionView;
	BBox* fTrackBox;
};

#endif
