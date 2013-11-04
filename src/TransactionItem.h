#ifndef TRANSACTION_ITEM_H
#define TRANSACTION_ITEM_H

#include <ListItem.h>

class TransactionData;

void InitTransactionItemLayout(BView *owner);

class TransactionItem : public BListItem
{
public:
	TransactionItem(const TransactionData &trans);
	void DrawItem(BView *owner, BRect frame, bool complete = false);
	void Update(BView* owner, const BFont* finfo);
	uint32 GetID(void) const { return fID; }
	time_t GetDate(void) const { return fDate; }
	const char *GetPayee(void) const { return fPayee.String(); }
	void SetData(const TransactionData &trans);
private:
	time_t fDate;
	Account *fAccount;
	BString fType;
	BString fPayee;
	Fixed 	fAmount;
	BString fCategory;
	BString fMemo;
	uint8 fStatus;
	uint32 fID;
	bigtime_t fTimeStamp;
};

#endif
