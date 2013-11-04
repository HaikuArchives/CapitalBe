#ifndef SCHEDULEDTRANS_ITEM_H
#define SCHEDULEDTRANS_ITEM_H

#include <ListItem.h>
#include <time.h>
#include "ScheduledTransData.h"

class Account;

//void InitTransactionItemLayout(BView *owner);

class ScheduledTransItem : public BListItem
{
public:
	ScheduledTransItem(const ScheduledTransData &data);
	
	void DrawItem(BView *owner, BRect frame, bool complete = false);
	void Update(BView* owner, const BFont* finfo);
	uint32 GetID(void) const { return fID; }
	const char * GetDate(void) const { return fDate.String(); }
	const char *GetPayee(void) const { return fPayee.String(); }
	void SetData(const TransactionData &trans);
	
private:
	Account *fAccount;
	BString fType;
	BString fPayee;
	BString fAmount;
	BString fCategory;
	BString fMemo;
	BString fDate;
	BString fCount;
	uint32 fID;
};

#endif
