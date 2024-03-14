#ifndef TRANSACTION_ITEM_H
#define TRANSACTION_ITEM_H


#include "TransactionData.h"

#include <ListItem.h>
#include <String.h>


void InitTransactionItemLayout(BView* owner);

class TransactionItem : public BListItem {
public:
	TransactionItem(const TransactionData& trans);
	void DrawItem(BView* owner, BRect frame, bool complete = false);
	void Update(BView* owner, const BFont* finfo);

	uint32 GetID(void) const { return fID; }

	time_t GetDate(void) const { return fDate; }

	const char* GetPayee(void) const { return fPayee.String(); }

	void SetData(const TransactionData& trans);

private:
	Account* fAccount;
	BString fCategory;
	BString fMemo;
	BString fPayee;
	BString fType;
	Fixed fAmount;

	bigtime_t fTimeStamp;
	time_t fDate;
	uint8 fStatus;
	uint32 fID;
};

#endif
