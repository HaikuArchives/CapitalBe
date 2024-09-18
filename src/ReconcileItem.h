#ifndef RECONCILE_ITEM_H
#define RECONCILE_ITEM_H

#include "TransactionData.h"

#include <ListItem.h>


class ReconcileItem : public BStringItem {
public:
	ReconcileItem(const TransactionData& trans);
	~ReconcileItem();

	void DrawItem(BView* owner, BRect frame, bool complete = false);

	TransactionData* GetTransaction() { return &fTransaction; }

	void SetReconciled(bool value);
	bool IsReconciled() const;

	void SyncToTransaction();
	void RevertTransaction() { fTransaction.SetStatus(TRANS_OPEN); }

private:
	TransactionData fTransaction;
	uint8 fValue;
};

#endif
