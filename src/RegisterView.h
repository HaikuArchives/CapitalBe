/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	dospuntos (Johan Wagenheim)
 *	humdinger (Joachim Seemer)
 *	raefaldhia (Raefaldhi Amartya Junior)
 */
#ifndef REGSITERVIEW_H
#define REGSITERVIEW_H

#include <Box.h>
#include <ListView.h>
#include <View.h>

#include "AccountListView.h"
#include "Notifier.h"
#include "TransactionView.h"

class CheckView;
class FilterView;

// clang-format off
enum {
	M_SHOW_ACCOUNT_SETTINGS = 'acst',
	M_SELECT_ACCOUNT = 'slac',
	M_SELECT_CURRENT = 'slcu',
};

typedef enum {
	ALL_TRANSACTIONS = 0,
	THIS_MONTH,
	LAST_MONTH,
	THIS_QUARTER,
	LAST_QUARTER,
	THIS_YEAR,
	LAST_YEAR
} filter_period_field;
// clang-format on

class RegisterView : public BView, public Observer {
public:
	RegisterView(const char* name, int32 flags);
	~RegisterView();

	void AttachedToWindow();
	void MessageReceived(BMessage* msg);

	void HandleNotify(const uint64& value, const BMessage* msg);
	void SelectAccount(const int32& index);
	void SetCheckFields(TransactionData data);

	bool SelectNextTransaction() { return fTransactionView->SelectNext(); }
	bool SelectPreviousTransaction() { return fTransactionView->SelectPrevious(); }
	bool SelectFirstTransaction() { return fTransactionView->SelectFirst(); }
	bool SelectLastTransaction() { return fTransactionView->SelectLast(); }

private:
	CheckView* fCheckView;
	AccountList* fAccountView;
	FilterView* fFilterView;
	BScrollView* fAccountScroller;
	BStringView* fTransactionlabel;
	TransactionView* fTransactionView;
	BBox* fTrackBox;
};

#endif // REGSITERVIEW_H
