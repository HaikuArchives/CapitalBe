#ifndef TRANSACTIONVIEW_H
#define TRANSACTIONVIEW_H

#include <set>
#include <ListView.h>
#include <ScrollView.h>
#include <View.h>
#include <StringView.h>
#include "Account.h"
#include "TransactionData.h"
#include "Notifier.h"

#define M_TRANSACTION_SELECTED 'trsl'
#define M_TRANSACTION_INVOKED 'trin'

class TransactionItem;

class TransactionView : public BView, public Observer
{
public:
	TransactionView(BRect frame);
	~TransactionView(void);
	
	void AttachedToWindow(void);
	
	void SetAccount(Account *acc);
	TransactionItem *AddTransaction(const TransactionData &trans, 
									const int32 &index=-1);
	void DeleteTransaction(const int32 &index);
	void EditTransaction();
	
	void Draw(BRect updateRect);
	void MessageReceived(BMessage* message);
	
	void HandleNotify(const uint64 &value, const BMessage *msg);
	void FrameResized(float width, float height);
	
	bool SelectNext(void);
	bool SelectPrevious(void);
	bool SelectFirst(void);
	bool SelectLast(void);
	
private:
	int32 FindItemForID(const uint32 &id);
	int32 FindIndexForDate(const time_t &time, const char *payee);
	
	BListView *fListView;
	BObjectList<TransactionItem> *fItemList;
	Transaction *fCurrent;
	BStringView *fCategoryLabel,*fMemoLabel;
	BScrollView *fScrollView;
};

#endif
