/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	raefaldhia (Raefaldhi Amartya Junior)
 */
#ifndef QTITEM_H
#define QTITEM_H

#include "Budget.h"
#include "Notifier.h"

#include <TextView.h>

class QuickTrackerItem : public BTextView, public Observer {
public:
	QuickTrackerItem(const char* name, uint32 flags = B_WILL_DRAW);
	virtual ~QuickTrackerItem();

	virtual void AttachedToWindow();

	virtual void HandleNotify(const uint64& value, const BMessage* msg);
	virtual void Configure();
};

class QTNetWorthItem : public QuickTrackerItem {
public:
	QTNetWorthItem(const char* name, uint32 flags = B_WILL_DRAW);
	~QTNetWorthItem();

	void AttachedToWindow();

	void HandleNotify(const uint64& value, const BMessage* msg);
	void SetObserving(const bool& value);

protected:
	void _Calculate();
	bool fIgnore;
};

class QTBudgetCategoryItem : public QuickTrackerItem {
public:
	QTBudgetCategoryItem(const char* category, const char* name, uint32 flags = B_WILL_DRAW);
	~QTBudgetCategoryItem();

	void AttachedToWindow();
	void HandleNotify(const uint64& value, const BMessage* msg);
	void SetObserving(const bool& value);

protected:
	void _Calculate();
	bool fIgnore;
	BudgetEntry fEntry;
};

#endif // QTITEM_H
