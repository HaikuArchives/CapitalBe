#ifndef RECONCILE_ITEM_H
#define RECONCILE_ITEM_H

#include <ListItem.h>

class TransactionData;

class ReconcileItem : public BStringItem
{
public:
	ReconcileItem(const TransactionData &trans);
	~ReconcileItem(void);
	void DrawItem(BView *owner, BRect frame, bool complete = false);
	TransactionData *GetTransaction(void) { return &fTransaction; }
	void SetReconciled(bool value);
	bool IsReconciled(void) const;
	
	void SyncToTransaction(void);
	void RevertTransaction(void) { fTransaction.SetStatus(TRANS_OPEN); }
private:
	TransactionData fTransaction;
	uint8 fValue;
};

#endif
