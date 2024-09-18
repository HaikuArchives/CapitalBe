#ifndef TRANSACTIONVIEW_H
#define TRANSACTIONVIEW_H

#include <ListView.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <StringView.h>
#include <View.h>

#include "Account.h"
#include "Notifier.h"
#include "TransactionData.h"

#define M_TRANSACTION_SELECTED 'trsl'
#define M_TRANSACTION_INVOKED 'trin'

class TransactionItem;


class TransactionList : public BListView {
public:
	TransactionList();
	~TransactionList();

	virtual void MessageReceived(BMessage* message);
	void MouseDown(BPoint position);

private:
	void _ShowPopUpMenu(BPoint screen);
	bool fShowingPopUpMenu;
};


class TransactionContext : public BPopUpMenu {
public:
	TransactionContext(const char* name, BMessenger target);
	virtual ~TransactionContext();

private:
	BMessenger fTarget;
};


class TransactionView : public BView, public Observer {
public:
	TransactionView();
	~TransactionView();

	void AttachedToWindow();

	void SetAccount(Account* acc, BMessage* msg = NULL);
	TransactionItem* AddTransaction(const TransactionData& trans, const int32& index = -1);
	void DeleteTransaction(const int32& index);
	void EditTransaction();

	void Draw(BRect updateRect);
	void MessageReceived(BMessage* message);

	void HandleNotify(const uint64& value, const BMessage* msg);
	void FrameResized(float width, float height);

	bool SelectNext();
	bool SelectPrevious();
	bool SelectFirst();
	bool SelectLast();

private:
	int32 _FindItemForID(const uint32& id);
	int32 _FindIndexForDate(const time_t& time, const char* payee);
	void _CalculatePeriod(int32 period, time_t& start, time_t& end);
	BString _GenerateQueryCommand(int32 accountID, BMessage* message = NULL);

	TransactionList* fListView;
	BObjectList<TransactionItem>* fItemList;
	Transaction* fCurrent;
	BStringView *fCategoryLabel, *fMemoLabel;
	BScrollView* fScrollView;
};

#endif
