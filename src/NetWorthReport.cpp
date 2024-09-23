/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	bitigchi (Emir Sari)
 *	dospuntos (Johan Wagenheim)
 *	humdinger (Joachim Seemer)
 */
#include "Account.h"
#include "CBLocale.h"
#include "ColumnListView.h"
#include "ColumnTypes.h"
#include "Database.h"
#include "ReportGrid.h"
#include "ReportWindow.h"
#include "TimeSupport.h"

#include <Catalog.h>
#include <FormattingConventions.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "NetWorthReport"


void
ReportWindow::_ComputeNetWorth()
{
	// Total of all accounts
	// Calculate the number of columns and the starting date for each one
	BObjectList<time_t> timelist(20, true);

	if (fSubtotalMode != SUBTOTAL_NONE) {
		for (time_t t = fStartDate; t < fEndDate;) {
			time_t* item = new time_t(t);
			timelist.AddItem(item);

			switch (fSubtotalMode) {
				case SUBTOTAL_MONTH:
				{
					t = IncrementDateByMonth(t);
					break;
				}
				case SUBTOTAL_QUARTER:
				{
					t = IncrementDateByQuarter(t);
					break;
				}
				case SUBTOTAL_YEAR:
				{
					t = IncrementDateByYear(t);
					break;
				}
				default:
				{
					t = fEndDate;
					break;
				}
			}
		}
	}
	timelist.AddItem(new time_t(fEndDate));

	ReportGrid accountgrid(1, timelist.CountItems());
	BString longestname(B_TRANSLATE("Total worth"));
	int longestnamelength = longestname.CountChars();


	for (int32 subtotal_index = 0; subtotal_index < timelist.CountItems(); subtotal_index++) {
		time_t subtotal_start = *((time_t*)timelist.ItemAt(subtotal_index));

		char rowtitle[128];
		struct tm* timestruct = localtime(&subtotal_start);

		BString datestring;
		gDefaultLocale.DateToString(subtotal_start, datestring);
		accountgrid.SetRowTitle(subtotal_index, datestring);

		int length = strlen(rowtitle);
		if (length > longestnamelength) {
			longestname = rowtitle;
			longestnamelength = length;
		}


		Fixed accounttotal;

		for (int32 i = 0; i < fAccountList->CountItems(); i++) {
			AccountItem* item = (AccountItem*)fAccountList->ItemAt(i);
			if (!item)
				continue;

			accounttotal += item->account->BalanceAt(subtotal_start);
		}  // end for each account

		accountgrid.SetValue(0, subtotal_index, accounttotal);
	}

	// Now that we have all the data, we need to set up the rows and columns for the report grid

	BColumn* col = new BStringColumn(B_TRANSLATE_CONTEXT("Date", "CommonTerms"),
		fGridView->StringWidth(longestname.String()) + 20, 10, 300, B_TRUNCATE_END);
	fGridView->AddColumn(col, 0);
	col = new BStringColumn(
		B_TRANSLATE("Total"), fGridView->StringWidth("$100,000,000.00"), 10, 300, B_TRUNCATE_END);
	fGridView->AddColumn(col, 1);

	fGridView->AddRow(new BRow());
	BRow* titlerow = new BRow();
	fGridView->AddRow(titlerow);
	titlerow->SetField(new BStringField(B_TRANSLATE("Total worth")), 0);
	fGridView->AddRow(new BRow());

	// Now that the grid is set up, start adding data to the grid
	for (int32 rowindex = 0; rowindex < accountgrid.CountItems(); rowindex++) {
		BRow* row = new BRow();
		fGridView->AddRow(row);

		BStringField* catname = new BStringField(accountgrid.RowTitle(rowindex));
		row->SetField(catname, 0);

		BString temp;
		Fixed f;

		accountgrid.ValueAt(0, rowindex, f);
		gCurrentLocale.CurrencyToString(f, temp);

		BStringField* amountfield = new BStringField(temp.String());
		row->SetField(amountfield, 1);
	}
}
