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
	TransactionList(void);
	~TransactionList(void);

	virtual void MessageReceived(BMessage* message);
	void MouseDown(BPoint position);

private:
	void ShowPopUpMenu(BPoint screen);
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
	TransactionView(void);
	~TransactionView(void);

	void AttachedToWindow(void);

	void SetAccount(Account* acc, BMessage* msg = NULL);
	TransactionItem* AddTransaction(const TransactionData& trans, const int32& index = -1);
	void DeleteTransaction(const int32& index);
	void EditTransaction();

	void Draw(BRect updateRect);
	void MessageReceived(BMessage* message);

	void HandleNotify(const uint64& value, const BMessage* msg);
	void FrameResized(float width, float height);

	bool SelectNext(void);
	bool SelectPrevious(void);
	bool SelectFirst(void);
	bool SelectLast(void);

private:
	int32 FindItemForID(const uint32& id);
	int32 FindIndexForDate(const time_t& time, const char* payee);
	void CalculatePeriod(int32 period, time_t& start, time_t& end);
	BString GenerateQueryCommand(int32 accountID, BMessage* message = NULL);

	TransactionList* fListView;
	BObjectList<TransactionItem>* fItemList;
	Transaction* fCurrent;
	BStringView *fCategoryLabel, *fMemoLabel;
	BScrollView* fScrollView;
};

#endif
