#ifndef QTITEM_H
#define QTITEM_H

#include <TextView.h>
#include "Notifier.h"
#include "Budget.h"

class QuickTrackerItem : public BTextView, public Observer
{
public:
	QuickTrackerItem(const BRect &rect, const char *name, 
					uint32 resize = B_FOLLOW_LEFT | B_FOLLOW_TOP,
					uint32 flags = B_WILL_DRAW);
	virtual ~QuickTrackerItem(void);
	virtual void HandleNotify(const uint64 &value, const BMessage *msg);
	virtual void Configure(void);
	virtual void AttachedToWindow(void);
};

class QTNetWorthItem : public QuickTrackerItem
{
public:
	QTNetWorthItem(const BRect &rect, const char *name, 
					uint32 resize = B_FOLLOW_LEFT | B_FOLLOW_TOP,
					uint32 flags = B_WILL_DRAW);
	~QTNetWorthItem(void);
	void HandleNotify(const uint64 &value, const BMessage *msg);
	void AttachedToWindow(void);
	void SetObserving(const bool &value);
	
protected:
	void Calculate(void);
	bool fIgnore;
};

class QTBudgetCategoryItem : public QuickTrackerItem
{
public:
	QTBudgetCategoryItem(const char *category, const BRect &rect, const char *name, 
					uint32 resize = B_FOLLOW_LEFT | B_FOLLOW_TOP,
					uint32 flags = B_WILL_DRAW);
	~QTBudgetCategoryItem(void);
	void HandleNotify(const uint64 &value, const BMessage *msg);
	void AttachedToWindow(void);
	void SetObserving(const bool &value);
	
protected:
	void Calculate(void);
	bool fIgnore;
	BudgetEntry fEntry;
};

#endif
