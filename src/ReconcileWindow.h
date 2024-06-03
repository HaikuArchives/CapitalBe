#ifndef RECONCILEWINDOW_H
#define RECONCILEWINDOW_H

#include <Button.h>
#include <ListItem.h>
#include <ListView.h>
#include <Message.h>
#include <StringView.h>
#include <TextControl.h>
#include <View.h>
#include <Window.h>

#include "Account.h"
#include "HelpButton.h"
#include "Notifier.h"

class DateBox;
class CurrencyBox;
class ReconcileItem;

void AddReconcileItems(const TransactionData& data, void* recwin);

class ReconcileWindow : public BWindow, public Observer {
public:
	ReconcileWindow(const BRect frame, Account* account);
	~ReconcileWindow(void);
	void MessageReceived(BMessage* msg);
	void HandleNotify(const uint64& value, const BMessage* msg);
	bool QuitRequested(void);

private:
	friend class ReconcileFilter;
	friend void AddReconcileItems(const TransactionData& data, void* ptr);

	void ApplyChargesAndInterest(void);
	ReconcileItem* FindItemForID(BListView* target, const uint32& id);
	void InsertTransactionItem(BListView* target, ReconcileItem* item);
	bool AutoReconcile(void);

	Account* fAccount;

	BListView *fDepositList, *fCheckList, *fChargeList;
	CurrencyBox *fOpening, *fClosing, *fCharges, *fInterest;
	DateBox* fDate;
	BButton *fReset, *fReconcile, *fCancel, *fAutoReconcile;

	// fTotal is the sum of all of the deposits, checks, and charges
	// The way to tell if we are done is if fDifference + fTotal == 0
	Fixed fDepositTotal, fCheckTotal, fChargeTotal, fDifference, fTotal;
	BStringView *fDepLabel, *fCheckLabel, *fChargeLabel, *fTotalLabel, *fDateLabel, *fChargesLabel;
	BScrollView *fDepScroll, *fCheckScroll, *fChargeScroll;

	HelpButton* fHelpButton;

	float fDateMultiplier, fOpeningMultiplier, fClosingMultiplier;
	time_t fCurrentDate;
};

#endif
