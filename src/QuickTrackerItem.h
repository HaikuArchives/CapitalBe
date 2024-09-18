#ifndef QTITEM_H
#define QTITEM_H

#include "Budget.h"
#include "Notifier.h"
#include <TextView.h>

class QuickTrackerItem : public BTextView, public Observer {
public:
	QuickTrackerItem(const char* name, uint32 flags = B_WILL_DRAW);
	virtual ~QuickTrackerItem();

	virtual void HandleNotify(const uint64& value, const BMessage* msg);
	virtual void Configure();
	virtual void AttachedToWindow();
};

class QTNetWorthItem : public QuickTrackerItem {
public:
	QTNetWorthItem(const char* name, uint32 flags = B_WILL_DRAW);
	~QTNetWorthItem();

	void HandleNotify(const uint64& value, const BMessage* msg);
	void AttachedToWindow();
	void SetObserving(const bool& value);

protected:
	void Calculate();
	bool fIgnore;
};

class QTBudgetCategoryItem : public QuickTrackerItem {
public:
	QTBudgetCategoryItem(const char* category, const char* name, uint32 flags = B_WILL_DRAW);
	~QTBudgetCategoryItem();
	void HandleNotify(const uint64& value, const BMessage* msg);
	void AttachedToWindow();
	void SetObserving(const bool& value);

protected:
	void Calculate();
	bool fIgnore;
	BudgetEntry fEntry;
};

#endif
