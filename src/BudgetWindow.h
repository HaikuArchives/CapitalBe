/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	dospuntos (Johan Wagenheim)
 */
#ifndef BUDGETWIN_H
#define BUDGETWIN_H

#include <Box.h>
#include <Button.h>
#include <ListItem.h>
#include <ListView.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <RadioButton.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>
#include <time.h>

#include "Budget.h"
#include "ColumnListView.h"
#include "ReportGrid.h"

class CurrencyBox;

class BudgetWindow : public BWindow {
public:
	BudgetWindow(const BRect& frame);
	~BudgetWindow();
	void MessageReceived(BMessage* msg);

private:
	void _BuildStatsAndEditor();
	void _BuildBudgetSummary();
	void _BuildCategoryList();

	void _RefreshCategories();
	void _RefreshBudgetSummary();
	void _RefreshBudgetGrid();

	void _GenerateBudget(const bool& zero);
	void _CalcStats(const char* cat, Fixed& high, Fixed& low, Fixed& avg);
	void _HandleCategorySelection();
	void _SetPeriod(const BudgetPeriod& period);

	BMenuBar* fBar;

	BColumnListView* fCategoryList;
	BRow *fIncomeRow, *fSpendingRow;

	BColumnListView* fBudgetSummary;
	BRow *fSummaryIncomeRow, *fSummarySpendingRow, *fSummaryTotalRow;

	BBox* fCatBox;
	CurrencyBox* fAmountBox;
	BStringView* fAmountLabel;
	BRadioButton *fMonthly, *fWeekly, *fQuarterly, *fAnnually;

	BColumnListView* fCatStat;
	BRow *fStatAverageRow, *fStatHighestRow, *fStatLowestRow;

	ReportGrid fIncomeGrid, fSpendingGrid;
	BString fDecimalSymbol;
};

#endif	// BUDGETWIN_H
