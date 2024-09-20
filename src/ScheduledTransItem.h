/*
 * Copyright 2009. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 */
#ifndef SCHEDULEDTRANS_ITEM_H
#define SCHEDULEDTRANS_ITEM_H

#include "ScheduledTransData.h"
#include <ListItem.h>
#include <time.h>

class Account;

// void InitTransactionItemLayout(BView *owner);

class ScheduledTransItem : public BListItem {
public:
	ScheduledTransItem(const ScheduledTransData& data);

	void DrawItem(BView* owner, BRect frame, bool complete = false);
	void Update(BView* owner, const BFont* finfo);

	uint32 GetID() const { return fID; }
	const char* GetDate() const { return fDate.String(); }
	const char* GetPayee() const { return fPayee.String(); }
	void SetData(const TransactionData& trans);

private:
	Account* fAccount;
	BString fType;
	BString fPayee;
	BString fAmount;
	BString fCategory;
	BString fMemo;
	BString fDate;
	BString fCount;
	uint32 fID;
};

#endif // SCHEDULEDTRANS_ITEM_H
