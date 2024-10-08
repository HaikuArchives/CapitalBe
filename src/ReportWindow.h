/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	waddlesplash (Augustin Cavalier)
 */
#ifndef REPORTWINDOW_H
#define REPORTWINDOW_H

#include "Account.h"
#include "DStringList.h"
#include "Notifier.h"

#include <ColumnListView.h>
#include <Font.h>
#include <ListItem.h>
#include <ListView.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <ScrollView.h>
#include <TextControl.h>
#include <Window.h>

#include <time.h>

class StickyDrawButton;
class DateBox;

// This is ordered from smallest interval to largest for a reason. :^)
// Note that if this is changed, you need to also change the BUDGET_ enum in
// Budget.h. The BudgetReport code depends on them having the same values
// clang-format off
enum {
	SUBTOTAL_WEEK = 0,
	SUBTOTAL_MONTH,
	SUBTOTAL_QUARTER,
	SUBTOTAL_YEAR,
	SUBTOTAL_NONE
};

enum {
	REPORT_CASH_FLOW = 0,
	REPORT_NET_WORTH,
	REPORT_TRANSACTIONS,
	REPORT_BUDGET
};
// clang-format on

class ReportWindow : public BWindow, public Observer {
public:
	ReportWindow(BRect frame);

	void MessageReceived(BMessage* msg);
	void FrameResized(float w, float h);
	bool QuitRequested();

	void HandleNotify(const uint64& value, const BMessage* msg);

private:
	void _AddAccount(Account* acc);
	void _FixGridScrollbar();
	void _RenderReport();

	void _ComputeCashFlow();
	void _ComputeNetWorth();
	void _ComputeTransactions();
	void _ComputeBudget();

	void _CalcCategoryString();

	BMenuField* fReportField;
	BMenuField* fSubtotalField;
	BListView* fAccountList;
	BListView* fCategoryList;
	BColumnListView* fGridView;
	BView* fGraphView;
	BScrollView* fCategoryScroller;
	DateBox *fStartDateBox, *fEndDateBox;
	StickyDrawButton* fGraphButton;

	uint8 fSubtotalMode;
	uint8 fReportMode;
	time_t fStartDate, fEndDate;

	BFont fTitleFont, fHeaderFont;
	BString fAccountString, fCategoryString;
};

class AccountItem : public BStringItem {
public:
	AccountItem(Account* acc)
		: BStringItem("")
	{
		if (acc)
			SetText(acc->Name());
		account = acc;
	}

	Account* account;
};


#endif	// REPORTWINDOW_H
